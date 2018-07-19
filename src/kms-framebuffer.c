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

#include "common.h"
#include "p_kms.h"
#include "xf86drm.h"
#include <drm_fourcc.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

struct kms_framebuffer *kms_framebuffer_create(struct kms_device *device,
					       unsigned int width,
					       unsigned int height,
					       uint32_t format)
{
	uint32_t handles[4] = { 0 }, pitches[4] = { 0 }, offsets[4] = { 0 };
	struct drm_mode_create_dumb args;
	struct kms_framebuffer *fb;
	unsigned int virtual_height;
	int err;

	switch (format)
	{
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
		virtual_height = height * 3 / 2;
		break;
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_YUV422:
	case DRM_FORMAT_YVU422:
		virtual_height = height * 2;
		break;
	case DRM_FORMAT_YUV444:
	case DRM_FORMAT_YVU444:
		virtual_height = height * 3;
		break;
	default:
		virtual_height = height;
		break;
	}

	LOG("fb virtual size: %d\n", width * virtual_height);

	fb = calloc(1, sizeof(*fb));
	if (!fb)
		return NULL;

	fb->device = device;
	fb->width = width;
	fb->height = height;
	fb->format = format;

	memset(&args, 0, sizeof(args));
	args.width = width;
	args.height = virtual_height;

	args.bpp = kms_format_bpp(format);
	if (!args.bpp) {
		free(fb);
		return NULL;
	}

	err = drmIoctl(device->fd, DRM_IOCTL_MODE_CREATE_DUMB, &args);
	if (err < 0) {
		free(fb);
		return NULL;
	}

	fb->handle = args.handle;
	fb->pitch = args.pitch;
	fb->size = args.size;

	switch (format)
	{
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
		offsets[0] = 0;
		handles[0] = fb->handle;
		pitches[0] = fb->pitch;
		break;
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV61:
		offsets[0] = 0;
		handles[0] = fb->handle;
		pitches[0] = fb->pitch;
		pitches[1] = pitches[0];
		offsets[1] = pitches[0] * height;
		handles[1] = fb->handle;
		break;
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
		handles[0] = fb->handle;
		pitches[0] = fb->pitch;
		offsets[0] = 0;
		handles[1] = fb->handle;
		pitches[1] = pitches[0] / 2;
		offsets[1] = pitches[0] * height;
		handles[2] = fb->handle;
		pitches[2] = pitches[1];
		offsets[2] = offsets[1] + pitches[1] * height / 2;
		break;
	case DRM_FORMAT_YVU422:
	case DRM_FORMAT_YUV422:
		offsets[0] = 0;
		handles[0] = fb->handle;
		pitches[0] = fb->pitch;
		offsets[1] = pitches[0] * height;
		handles[1] = fb->handle;
		pitches[1] = pitches[0] / 2;
		offsets[2] = offsets[1] + pitches[1] * height / 1;
		handles[2] = fb->handle;
		pitches[2] = pitches[1];
		break;
	case DRM_FORMAT_YVU444:
	case DRM_FORMAT_YUV444:
		offsets[0] = 0;
		handles[0] = fb->handle;
		pitches[0] = fb->pitch;
		offsets[1] = pitches[0] * height;
		handles[1] = fb->handle;
		pitches[1] = pitches[0] / 1;
		offsets[2] = offsets[1] + pitches[1] * height / 1;
		handles[2] = fb->handle;
		pitches[2] = pitches[1];
		break;
	default:
		handles[0] = fb->handle;
		pitches[0] = fb->pitch;
		offsets[0] = 0;
		break;
	}

	/* attempt drmModeAddFB2(), and fallback to drmModeAddFB() */
	err = drmModeAddFB2(device->fd, width, height,
			    format,
			    handles, pitches, offsets,
			    &fb->id, 0);
	if (err) {
		LOG("fallback to drmModeAddFB()\n");
		err = drmModeAddFB(device->fd, width, height,
				   args.bpp, args.bpp,
				   fb->pitch, fb->handle, &fb->id);
	}
	if (err < 0) {
		LOG("failed to add fb: %d\n", err);
		kms_framebuffer_free(fb);
		return NULL;
	}

	return fb;
}

void kms_framebuffer_free(struct kms_framebuffer *fb)
{
	struct kms_device *device = fb->device;
	struct drm_mode_destroy_dumb args;
	int err;

	if (fb->id) {
		err = drmModeRmFB(device->fd, fb->id);
		if (err < 0) {
			/* not much we can do now */
			LOG("error: drmModeRmFB: %d\n", err);
		}
	}

	memset(&args, 0, sizeof(args));
	args.handle = fb->handle;

	err = drmIoctl(device->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &args);
	if (err < 0) {
		/* not much we can do now */
	}

	free(fb);
}

int kms_framebuffer_map(struct kms_framebuffer *fb, void **ptrp)
{
	struct kms_device *device = fb->device;
	struct drm_mode_map_dumb args;
	void *ptr;
	int err;

	if (fb->ptr) {
		*ptrp = fb->ptr;
		return 0;
	}

	memset(&args, 0, sizeof(args));
	args.handle = fb->handle;

	err = drmIoctl(device->fd, DRM_IOCTL_MODE_MAP_DUMB, &args);
	if (err < 0)
		return -errno;

	ptr = mmap(0, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   device->fd, args.offset);
	if (ptr == MAP_FAILED)
		return -errno;

	*ptrp = fb->ptr = ptr;

	return 0;
}

void kms_framebuffer_unmap(struct kms_framebuffer *fb)
{
	if (fb->ptr) {
		munmap(fb->ptr, fb->size);
		fb->ptr = NULL;
	}
}
