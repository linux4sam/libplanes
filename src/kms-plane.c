/*
 * Copyright © 2014 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "p_kms.h"

static int kms_plane_update(struct kms_plane *plane, struct kms_framebuffer *fb,
			    uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h,
			    int crtc_x, int crtc_y, int crtc_w, int crtc_h);

static int kms_plane_probe(struct kms_plane *plane)
{
	struct kms_device *device = plane->device;
	drmModeObjectPropertiesPtr props;
	drmModePlane *p;
	unsigned int i;

	p = drmModeGetPlane(device->fd, plane->id);
	if (!p)
		return -ENODEV;

	/* TODO: allow dynamic assignment to CRTCs */
	if (p->crtc_id == 0) {
		for (i = 0; i < device->num_crtcs; i++) {
			if (p->possible_crtcs & (1 << i)) {
				p->crtc_id = device->crtcs[i]->id;
				break;
			}
		}
	}

	for (i = 0; i < device->num_crtcs; i++) {
		if (device->crtcs[i]->id == p->crtc_id) {
			plane->crtc = device->crtcs[i];
			break;
		}
	}

	plane->formats = calloc(p->count_formats, sizeof(uint32_t));
	if (!plane->formats) {
		drmModeFreePlane(p);
		return -ENOMEM;
	}

	for (i = 0; i < p->count_formats; i++)
		plane->formats[i] = p->formats[i];

	plane->num_formats = p->count_formats;

	drmModeFreePlane(p);

	props = drmModeObjectGetProperties(device->fd, plane->id,
					   DRM_MODE_OBJECT_PLANE);
	if (!props)
		return -ENODEV;

	for (i = 0; i < props->count_props; i++) {
		drmModePropertyPtr prop;

		prop = drmModeGetProperty(device->fd, props->props[i]);
		if (prop) {
			if (strcmp(prop->name, "type") == 0)
				plane->type = props->prop_values[i];

			drmModeFreeProperty(prop);
		}
	}

	drmModeFreeObjectProperties(props);

	return 0;
}

struct kms_plane *kms_plane_create(struct kms_device *device, uint32_t id)
{
	struct kms_plane *plane;

	plane = calloc(1, sizeof(*plane));
	if (!plane)
		return NULL;

	plane->device = device;
	plane->id = id;

	plane->drm_obj = malloc(sizeof(*(plane->drm_obj)));
	if (!plane->drm_obj) {
		kms_plane_free(plane);
		return NULL;
	}

	plane->drm_obj->id = id;
	drm_obj_get_properties(device->fd, plane->drm_obj, DRM_MODE_OBJECT_PLANE);

	kms_plane_probe(plane);

	return plane;
}

int kms_plane_remove(struct kms_plane *plane)
{
	return kms_plane_update(plane, NULL, 0, 0, 0, 0, 0, 0, 0, 0);
}

void kms_plane_free(struct kms_plane *plane)
{
	if (!plane)
		return;

	drm_obj_free(plane->drm_obj);
	free(plane->formats);
	free(plane);
}

static int kms_plane_update(struct kms_plane *plane, struct kms_framebuffer *fb,
			    uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h,
			    int crtc_x, int crtc_y, int crtc_w, int crtc_h)
{
	struct kms_device *device = plane->device;
	int fb_id = fb ? fb->id : 0;
	int crtc_id = fb_id ? plane->crtc->id : 0;
	int ret;

	if (!device->atomic_request) {
		device->atomic_request = drmModeAtomicAlloc();
		if (!device->atomic_request) {
			LOG("error: drmModeAtomicAlloc failed\n");
			return -ENOMEM;
		}
	}

	LOG("kms_plane_update: fd_id=%d crtc_id=%d src_x=%d src_y=%d src_w=%d src_h=%d crtc_x=%d crtc_y=%d crtc_w=%d crtc_h=%d\n",
		fb_id, crtc_id, src_x, src_y, src_w, src_h, crtc_x, crtc_y, crtc_w, crtc_h);

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "FB_ID", fb_id);
	if (ret) {
		LOG("error: can't set FB_ID property (%d)\n", ret);
		goto out;
	}

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "CRTC_ID", crtc_id);
	if (ret) {
		LOG("error: can't set CRTC_ID property (%d)\n", ret);
		goto out;
	}

	if (!fb_id || !crtc_id)
		goto out;

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "SRC_X", src_x << 16);
	if (ret) {
		LOG("error: can't set SRC_X property\n");
		goto out;
	}

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "SRC_Y", src_y << 16);
	if (ret) {
		LOG("error: can't set SRC_Y property\n");
		goto out;
	}

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "SRC_W", src_w << 16);
	if (ret) {
		LOG("error: can't set SRC_W property\n");
		goto out;
	}

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "SRC_H", src_h << 16);
	if (ret) {
		LOG("error: can't set SRC_H property\n");
		goto out;
	}

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "CRTC_X", crtc_x);
	if (ret) {
		LOG("error: can't set CRTC_X property\n");
		goto out;
	}

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "CRTC_Y", crtc_y);
	if (ret) {
		LOG("error: can't set CRTC_Y property\n");
		goto out;
	}

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "CRTC_W", crtc_w);
	if (ret) {
		LOG("error: can't set CRTC_W property\n");
		goto out;
	}

	ret = drm_obj_set_property(device->atomic_request, plane->drm_obj, "CRTC_H", crtc_h);
	if (ret) {
		LOG("error: can't set CRTC_H property\n");
		goto out;
	}

out:
	if (ret) {
		drmModeAtomicFree(device->atomic_request);
		device->atomic_request = NULL;
	}

	return ret;
}

int kms_plane_set(struct kms_plane *plane, struct kms_framebuffer *fb,
		  int x, int y, double scale_x, double scale_y)
{
	int w = fb->width * scale_x;
	int h = fb->height * scale_y;

	return kms_plane_update(plane, fb, 0, 0, fb->width, fb->height, x, y, w, h);
}

int kms_plane_set_pan(struct kms_plane *plane, struct kms_framebuffer *fb,
		      int x, int y, uint32_t px, uint32_t py, uint32_t pw,
		      uint32_t ph, double scale_x, double scale_y)
{
	int w = pw * scale_x;
	int h = ph * scale_y;

	return kms_plane_update(plane, fb, px, py, pw, ph, x, y, w, h);
}

bool kms_plane_supports_format(struct kms_plane *plane, uint32_t format)
{
	unsigned int i;

	for (i = 0; i < plane->num_formats; i++)
		if (plane->formats[i] == format)
			return true;

	return false;
}
