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
/**
 * @file
 * @brief KMS API
 */
#ifndef PLANES_KMS_H
#define PLANES_KMS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <xf86drmMode.h>

#ifdef __cplusplus
extern "C" {
#endif

struct kms_device {
	int fd;

	struct kms_screen **screens;
	unsigned int num_screens;

	struct kms_crtc **crtcs;
	unsigned int num_crtcs;

	struct kms_plane **planes;
	unsigned int num_planes;

	drmModeAtomicReqPtr atomic_request;
	bool modeset_needed;
};

/**
 * Open and probe the KMS device.
 *
 * @param fd File descriptor for the DRM device.
 */
struct kms_device *kms_device_open(int fd);

/**
 * Close and cleanup the KMS device.
 *
 * @param device The KMS device.
 */
void kms_device_close(struct kms_device *device);

/**
 * Dump debug information about the KMS device.
 *
 * @param device The KMS device.
 */
void kms_device_dump(struct kms_device *device);

/**
 * Commit the DRM state changes.
 *
 * @param device The KMS device.
 */
int kms_device_flush(struct kms_device *device, uint32_t flags);

struct kms_framebuffer {
	struct kms_device *device;

	unsigned int width;
	unsigned int height;
	unsigned int pitch;
	uint32_t format;
	size_t size;

	uint32_t handle;
	uint32_t id;

	int prime_fd;

	void *ptr;
};

struct drm_object;

struct kms_screen {
	struct kms_device *device;
	struct drm_object *drm_obj;
	bool connected;
	uint32_t type;
	uint32_t id;

	unsigned int width;
	unsigned int height;
	char *name;

	drmModeModeInfo mode;
};

struct kms_plane {
	struct kms_device *device;
	struct kms_crtc *crtc;
	struct drm_object *drm_obj;
	unsigned int type;
	uint32_t id;

	uint32_t *formats;
	unsigned int num_formats;
};

/**
 * Return the DRM format for the specific string equivalent.
 *
 * @param name The string representation of the DRM format.
 */
uint32_t kms_format_val(const char *name);

#ifdef __cplusplus
}
#endif

#endif
