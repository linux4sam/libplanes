/*
 * Copyright (C) 2017 Microchip Technology Inc.  All rights reserved.
 *   Joshua Henderson <joshua.henderson@microchip.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "common.h"
#include "p_kms.h"
#include "planes/plane.h"

#include <drm_fourcc.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>

static uint32_t choose_format(struct kms_plane* plane)
{
	unsigned int i;

	/* preferred default formats */
	static const uint32_t formats[] = {
		DRM_FORMAT_ARGB8888,
		DRM_FORMAT_XRGB8888,
		DRM_FORMAT_XBGR8888,
	};

	for (i = 0; i < ARRAY_SIZE(formats); i++)
		if (kms_plane_supports_format(plane, formats[i]))
			return formats[i];

	return 0;
}

static uint32_t create_gem_name(struct kms_framebuffer* fb)
{
	int err;
	struct drm_gem_flink flink;
	memset(&flink, 0, sizeof(flink));
	flink.handle = fb->handle;
	err = drmIoctl(fb->device->fd, DRM_IOCTL_GEM_FLINK, &flink);
	if (err < 0) {
		LOG("error: could not flink: %s\n",
		    strerror(-err));
		return 0;
	}

	LOG("fb 0x%x: created GEM name %d\n", fb->id, flink.name);

	return flink.name;
}

struct plane_data* plane_create(struct kms_device* device, int type, int index,
				int width, int height, uint32_t format)
{
	return plane_create_buffered(device, type, index, width, height, format, 1);
}

struct plane_data* plane_create_buffered(struct kms_device* device, int type,
					 int index, int width, int height,
					 uint32_t format, uint32_t buffer_count)
{
	uint32_t fb;

	struct plane_data* plane = calloc(1, sizeof(struct plane_data));
	if (!plane) {
		LOG("error: failed to allocate plane\n");
		goto abort;
	}

	plane->fbs = calloc(buffer_count, sizeof(struct kms_framebuffer*));
	plane->bufs = calloc(buffer_count, sizeof(void*));
	plane->prime_fds = calloc(buffer_count, sizeof(int));
	plane->gem_names = calloc(buffer_count, sizeof(uint32_t*));

	if (!plane->fbs || !plane->bufs || !plane->prime_fds || !plane->gem_names) {
		LOG("error: failed to allocate plane\n");
		goto abort;
	}

	plane->plane = kms_device_find_plane_by_type(device, type, index);

	if (!plane->plane) {
		LOG("error: no plane found by type and index %d:%d\n", type, index);
		goto abort;
	}

	plane->type = type;
	plane->buffer_count = buffer_count;

	if (!format) {
		format = choose_format(plane->plane);
		if (!format) {
			LOG("error: no matching format found\n");
			goto abort;
		}
	}
	else
	{
		if (!kms_plane_supports_format(plane->plane, format)) {
			LOG("error: format unsupported by plane: %s\n", kms_format_str(format));
			goto abort;
		}
	}

	LOG("plane 0x%x: allocating fb format %s with res %dx%d\n",
	    plane->plane->id, kms_format_str(format), width, height);

	for (fb = 0; fb < plane->buffer_count; fb++) {
		plane->fbs[fb] = kms_framebuffer_create(device, width, height, format);
		if (!plane->fbs[fb]) {
			LOG("error: failed to create fb\n");
			goto abort;
		}
		plane->prime_fds[fb] = -1;
	}

	plane->index = index;
	plane->alpha = 255;
	plane->scale_x = 1.0;
	plane->scale_y = 1.0;

	for (fb = 0; fb < plane->buffer_count;fb++) {
		plane->gem_names[fb] = create_gem_name(plane->fbs[fb]);
	}

    /*
     * It is important to set the rotation and alpha properties when requesting
     * a plane as we don't know if it had been used before, and so we don't
     * know it's current state.
     */
    plane_apply_rotate(plane, plane->rotate_degrees);
    plane_apply_alpha(plane, plane->alpha);

	return plane;
abort:
	plane_free(plane);

	return NULL;
}

static void plane_fb_free(struct plane_data* plane)
{
	uint32_t fb;

	plane_fb_unmap(plane);
	plane_fb_unexport(plane);

	for (fb = 0; fb < plane->buffer_count; fb++) {
		if (plane->fbs[fb]) {
			kms_framebuffer_free(plane->fbs[fb]);
			plane->fbs[fb] = NULL;
		}
	}
}

void plane_free(struct plane_data* plane)
{
	if (plane) {
		plane_fb_free(plane);

		if (plane->fbs)
			free(plane->fbs);
		if (plane->bufs)
			free(plane->bufs);
		if (plane->prime_fds)
			free(plane->prime_fds);
		if (plane->gem_names)
			free(plane->gem_names);
		free(plane);
	}
}

int plane_fb_reallocate(struct plane_data* plane,
			int width, int height, uint32_t format)
{
	uint32_t fb;

	if (!format) {
		format = choose_format(plane->plane);
		if (!format) {
			LOG("error: no matching format found\n");
			goto abort;
		}
	}

	plane_fb_free(plane);

	for (fb = 0; fb < plane->buffer_count; fb++) {
		plane->fbs[fb] = kms_framebuffer_create(plane->plane->device, width, height, format);
		if (!plane->fbs[fb]) {
			LOG("error: failed to create fb\n");
			goto abort;
		}
	}

	return 0;

abort:
	return -1;
}

static int set_plane_prop(struct kms_plane* plane, const char* name, uint32_t value)
{
	struct kms_device* device = plane->device;
	drmModeObjectPropertiesPtr props;
	unsigned int i;
	int status = -1;

	props = drmModeObjectGetProperties(device->fd, plane->id,
					   DRM_MODE_OBJECT_PLANE);
	if (!props)
		return -ENODEV;

	for (i = 0; i < props->count_props; i++) {
		drmModePropertyPtr prop;

		prop = drmModeGetProperty(device->fd, props->props[i]);
		if (prop) {
			if (strcmp(prop->name, name) == 0) {
				status = drmModeObjectSetProperty(device->fd,
								  plane->id,
								  DRM_MODE_OBJECT_PLANE,
								  prop->prop_id,
								  value);
			}

			drmModeFreeProperty(prop);
		}
	}

	drmModeFreeObjectProperties(props);

	return status;
}

int plane_apply_rotate(struct plane_data* plane, uint32_t degrees)
{
	int rotate_index = degrees / 90;
	if (set_plane_prop(plane->plane, "rotation", 1 << rotate_index)) {
		LOG("error: failed to apply plane rotate\n");
		return -1;
	}

	plane->rotate_degrees = degrees;
	plane->rotate_degrees_applied = degrees;

	return 0;
}

int plane_apply_alpha(struct plane_data* plane, uint32_t alpha)
{
	if (plane->plane->type == DRM_PLANE_TYPE_PRIMARY)
		return -1;

	if (set_plane_prop(plane->plane, "alpha", alpha)) {
		LOG("error: failed to apply plane alpha %d\n", alpha);
		return -1;
	}

	plane->alpha = alpha;
	plane->alpha_applied = alpha;

	return 0;
}

int plane_set_rotate(struct plane_data* plane, uint32_t degrees)
{
	if (plane->rotate_degrees % 90 ||
	    plane->rotate_degrees > 270 ||
	    plane->rotate_degrees < 0) {
		LOG("error: failed to set plane rotate degrees\n");
		return -1;
	}

	plane->rotate_degrees = degrees;

	return 0;
}

int plane_set_alpha(struct plane_data* plane, uint32_t alpha)
{
	if (alpha > 255) {
		LOG("error: failed to set plane alpha %d\n", alpha);
		return -1;
	}

	plane->alpha = alpha;

	return 0;
}

void plane_set_pos(struct plane_data* plane, int x, int y)
{
	plane->x = x;
	plane->y = y;
}

void plane_set_scale(struct plane_data* plane, double scale)
{
	plane_set_scale_independent(plane, scale, scale);
}

void plane_set_scale_independent(struct plane_data* plane, double scale_x, double scale_y)
{
	plane->scale_x = scale_x;
	plane->scale_y = scale_y;
}

double plane_scale_x(struct plane_data* plane)
{
	return plane->scale_x;
}

double plane_scale_y(struct plane_data* plane)
{
	return plane->scale_y;
}

int plane_apply(struct plane_data* plane)
{
	struct kms_framebuffer* fb = plane->fbs[plane->front_buf];

	if (plane->rotate_degrees != plane->rotate_degrees_applied)
		plane_apply_rotate(plane, plane->rotate_degrees);

	if (plane->alpha != plane->alpha_applied)
		plane_apply_alpha(plane, plane->alpha);

	if (plane->pan.width && plane->pan.height) {
		return kms_plane_set_pan(plane->plane, fb,
					 plane->x, plane->y,
					 plane->pan.x, plane->pan.y,
					 plane->pan.width, plane->pan.height,
					 plane->scale_x, plane->scale_y);
	}

	return kms_plane_set(plane->plane, fb,
			     plane->x, plane->y,
			     plane->scale_x, plane->scale_y);
}

uint32_t plane_width(struct plane_data* plane)
{
	return plane->fbs[0]->width;
}

uint32_t plane_height(struct plane_data* plane)
{
	return plane->fbs[0]->height;
}

uint32_t plane_format(struct plane_data* plane)
{
	return plane->fbs[0]->format;
}

int32_t plane_x(struct plane_data* plane)
{
	return plane->x;
}

int32_t plane_y(struct plane_data* plane)
{
	return plane->y;
}

int32_t plane_rotate(struct plane_data* plane)
{
	return plane->rotate_degrees;
}

uint32_t plane_alpha(struct plane_data* plane)
{
	return plane->alpha;
}

int32_t plane_pan_x(struct plane_data* plane)
{
	return plane->pan.x;
}

int32_t plane_pan_y(struct plane_data* plane)
{
	return plane->pan.y;
}

uint32_t plane_pan_width(struct plane_data* plane)
{
	return plane->pan.width;
}

uint32_t plane_pan_height(struct plane_data* plane)
{
	return plane->pan.height;
}

void plane_set_pan_pos(struct plane_data* plane, int x, int y)
{
	plane->pan.x = x;
	plane->pan.y = y;
}

void plane_set_pan_size(struct plane_data* plane, int width, int height)
{
	plane->pan.width = width;
	plane->pan.height = height;
}

void plane_hide(struct plane_data* plane)
{
	kms_plane_remove(plane->plane);
}

int plane_fb_map(struct plane_data* plane)
{
	uint32_t fb;

	for (fb = 0; fb < plane->buffer_count; fb++) {
		if (!plane->bufs[fb]) {
			int err;

			err = kms_framebuffer_map(plane->fbs[fb], &plane->bufs[fb]);
			if (err < 0) {
				LOG("error: kms_framebuffer_map() failed: %s\n",
				    strerror(-err));
				return -err;
			}
		}
	}

	return 0;
}

void plane_fb_unmap(struct plane_data* plane)
{
	uint32_t fb;

	for (fb = 0; fb < plane->buffer_count; fb++) {
		if (plane->bufs[fb]) {
			kms_framebuffer_unmap(plane->fbs[fb]);
			plane->bufs[fb] = NULL;
		}
	}
}

int plane_fb_export(struct plane_data* plane)
{
	uint32_t fb;

	for (fb = 0; fb < plane->buffer_count; fb++) {
		if (plane->prime_fds[fb] == -1) {
			int err;

			err = kms_framebuffer_export(plane->fbs[fb], &plane->prime_fds[fb]);
			if (err < 0)
			{
				LOG("error: kms_framebuffer_export() failed: %s\n",
				    strerror(-err));
				return err;
			}
		}
	}

	return 0;
}

void plane_fb_unexport(struct plane_data* plane)
{
	uint32_t fb;

	for (fb = 0; fb < plane->buffer_count; fb++)
		plane->prime_fds[fb] = -1;
}

int plane_flip(struct plane_data* plane, uint32_t target)
{
	plane->front_buf = target;

	if (plane->pan.width && plane->pan.height) {
		return kms_plane_set_pan(plane->plane, plane->fbs[plane->front_buf],
					 plane->x, plane->y,
					 plane->pan.x, plane->pan.y,
					 plane->pan.width, plane->pan.height,
					 plane->scale_x, plane->scale_y);
	}

	return kms_plane_set(plane->plane, plane->fbs[plane->front_buf],
			     plane->x, plane->y,
			     plane->scale_x, plane->scale_y);
}

int plane_flip_async(struct plane_data* plane, uint32_t target)
{
	/*
	 * With the switch to the atomic API, this function no longer makes sense
	 * as it just modifies the drm states. The commit of the changes is handled
	 * by kms_device_flush().
	 * Keep it to not cause API breakage. Plan to remove it in the future.
	 */
	return plane_flip(plane, target);
}
