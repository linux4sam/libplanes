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
#ifndef PLANES_P_KMS_H
#define PLANES_P_KMS_H

#include "planes/kms.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <xf86drmMode.h>

#ifdef __cplusplus
extern "C" {
#endif

struct kms_plane *kms_device_find_plane_by_type(struct kms_device *device,
						uint32_t type,
						unsigned int index);

struct kms_crtc {
	struct kms_device *device;
	uint32_t id;
};

struct kms_crtc *kms_crtc_create(struct kms_device *device, uint32_t id);
void kms_crtc_free(struct kms_crtc *crtc);

struct kms_framebuffer *kms_framebuffer_create(struct kms_device *device,
					       unsigned int width,
					       unsigned int height,
					       uint32_t format);
void kms_framebuffer_free(struct kms_framebuffer *fb);
int kms_framebuffer_map(struct kms_framebuffer *fb, void **ptrp);
void kms_framebuffer_unmap(struct kms_framebuffer *fb);
int kms_framebuffer_export(struct kms_framebuffer *fb, int *prime_fd);

struct kms_screen *kms_screen_create(struct kms_device *device, uint32_t id);
void kms_screen_free(struct kms_screen *screen);

int kms_screen_set(struct kms_screen *screen, struct kms_crtc *crtc,
		   struct kms_framebuffer *fb);

struct kms_plane *kms_plane_create(struct kms_device *device, uint32_t id);
void kms_plane_free(struct kms_plane *plane);
int kms_plane_remove(struct kms_plane *plane);

int kms_plane_set(struct kms_plane *plane, struct kms_framebuffer *fb,
		  int x, int y, double scale_x, double scale_y);
int kms_plane_set_pan(struct kms_plane *plane, struct kms_framebuffer *fb,
		      int x, int y, uint32_t px, uint32_t py, uint32_t pw, uint32_t ph, double scale_x, double scale_y);
bool kms_plane_supports_format(struct kms_plane *plane, uint32_t format);

void kms_device_probe_framebuffers(struct kms_device *device);

const char* kms_format_str(uint32_t format);
int kms_format_bpp(uint32_t format);

#ifdef __cplusplus
}
#endif

#endif
