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
#include <unistd.h>
#include <drm_fourcc.h>
#include <sys/ioctl.h>
#include <xf86drm.h>

#include "common.h"
#include "p_kms.h"

static const char *const connector_names[] = {
	"Unknown",
	"VGA",
	"DVI-I",
	"DVI-D",
	"DVI-A",
	"Composite",
	"SVIDEO",
	"LVDS",
	"Component",
	"9PinDIN",
	"DisplayPort",
	"HDMI-A",
	"HDMI-B",
	"TV",
	"eDP",
	"Virtual",
	"DSI",
};

static void kms_device_probe_screens(struct kms_device *device)
{
	unsigned int counts[ARRAY_SIZE(connector_names)];
	struct kms_screen *screen;
	drmModeRes *res;
	int i;

	memset(counts, 0, sizeof(counts));

	res = drmModeGetResources(device->fd);
	if (!res)
		return;

	device->screens = calloc(res->count_connectors, sizeof(screen));
	if (!device->screens) {
		drmModeFreeResources(res);
		return;
	}

	for (i = 0; i < res->count_connectors; i++) {
		unsigned int *count;
		const char *type;
		int len;

		screen = kms_screen_create(device, res->connectors[i]);
		if (!screen)
			continue;

		/* assign a unique name to this screen */
		type = connector_names[screen->type];
		count = &counts[screen->type];

		len = snprintf(NULL, 0, "%s-%u", type, *count);

		screen->name = malloc(len + 1);
		if (!screen->name) {
			free(screen);
			continue;
		}

		snprintf(screen->name, len + 1, "%s-%u", type, *count);
		(*count)++;

		device->screens[i] = screen;
		device->num_screens++;
	}

	drmModeFreeResources(res);
}

static void kms_device_probe_crtcs(struct kms_device *device)
{
	struct kms_crtc *crtc;
	drmModeRes *res;
	int i;

	res = drmModeGetResources(device->fd);
	if (!res)
		return;

	device->crtcs = calloc(res->count_crtcs, sizeof(crtc));
	if (!device->crtcs) {
		drmModeFreeResources(res);
		return;
	}

	for (i = 0; i < res->count_crtcs; i++) {
		crtc = kms_crtc_create(device, res->crtcs[i]);
		if (!crtc)
			continue;

		device->crtcs[i] = crtc;
		device->num_crtcs++;
	}

	drmModeFreeResources(res);
}

static void kms_device_probe_planes(struct kms_device *device)
{
	struct kms_plane *plane;
	drmModePlaneRes *res;
	unsigned int i;

	res = drmModeGetPlaneResources(device->fd);
	if (!res)
		return;

	device->planes = calloc(res->count_planes, sizeof(plane));
	if (!device->planes) {
		drmModeFreePlaneResources(res);
		return;
	}

	for (i = 0; i < res->count_planes; i++) {
		plane = kms_plane_create(device, res->planes[i]);
		if (!plane)
			continue;

		device->planes[i] = plane;
		device->num_planes++;
	}

	drmModeFreePlaneResources(res);
}

void kms_device_probe_framebuffers(struct kms_device *device)
{
	drmModeRes *res;

	res = drmModeGetResources(device->fd);
	if (!res)
		return;

	LOG("fbs count: %d\n", res->count_fbs);
	LOG("crtcs count: %d\n", res->count_crtcs);
	LOG("max_width: %d\n", res->max_width);
	LOG("max_height: %d\n", res->max_height);

	drmModeFreeResources(res);
}

static void kms_device_probe(struct kms_device *device)
{
	kms_device_probe_screens(device);
	kms_device_probe_crtcs(device);

	if (device->num_screens < 1) {
		LOG("no screens found\n");
		return;
	}

	kms_device_probe_planes(device);
	kms_device_probe_framebuffers(device);
}

struct kms_device *kms_device_open(int fd)
{
	struct kms_device *device;
	int err;

	err = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err < 0) {
		LOG("error: drmSetClientCap() failed: %d\n", err);
	}

	device = calloc(1, sizeof(*device));
	if (!device)
		return NULL;

	device->fd = fd;

	kms_device_probe(device);

	return device;
}

void kms_device_close(struct kms_device *device)
{
	unsigned int i;

	for (i = 0; i < device->num_planes; i++)
		kms_plane_free(device->planes[i]);

	free(device->planes);

	for (i = 0; i < device->num_crtcs; i++)
		kms_crtc_free(device->crtcs[i]);

	free(device->crtcs);

	for (i = 0; i < device->num_screens; i++)
		kms_screen_free(device->screens[i]);

	free(device->screens);

	if (device->fd >= 0)
		close(device->fd);

	free(device);
}

struct kms_plane *kms_device_find_plane_by_type(struct kms_device *device,
						uint32_t type,
						unsigned int index)
{
	unsigned int i;

	for (i = 0; i < device->num_planes; i++) {
		if (device->planes[i]->type == type) {
			if (index == 0)
				return device->planes[i];

			index--;
		}
	}

	return NULL;
}

#define FORMAT_CASE(f) case f: return #f;

const char* kms_format_str(uint32_t format)
{
	switch (format)
	{
	FORMAT_CASE(DRM_FORMAT_C8)
	FORMAT_CASE(DRM_FORMAT_R8)
#ifdef DRM_FORMAT_R16
	FORMAT_CASE(DRM_FORMAT_R16)
#endif
	FORMAT_CASE(DRM_FORMAT_RG88)
	FORMAT_CASE(DRM_FORMAT_GR88)
#ifdef DRM_FORMAT_RG1616
	FORMAT_CASE(DRM_FORMAT_RG1616)
#endif
#ifdef DRM_FORMAT_GR1616
	FORMAT_CASE(DRM_FORMAT_GR1616)
#endif
	FORMAT_CASE(DRM_FORMAT_RGB332)
	FORMAT_CASE(DRM_FORMAT_BGR233)
	FORMAT_CASE(DRM_FORMAT_XRGB4444)
	FORMAT_CASE(DRM_FORMAT_XBGR4444)
	FORMAT_CASE(DRM_FORMAT_RGBX4444)
	FORMAT_CASE(DRM_FORMAT_BGRX4444)
        FORMAT_CASE(DRM_FORMAT_ARGB4444)
        FORMAT_CASE(DRM_FORMAT_ABGR4444)
	FORMAT_CASE(DRM_FORMAT_RGBA4444)
	FORMAT_CASE(DRM_FORMAT_BGRA4444)
	FORMAT_CASE(DRM_FORMAT_XRGB1555)
	FORMAT_CASE(DRM_FORMAT_XBGR1555)
	FORMAT_CASE(DRM_FORMAT_RGBX5551)
	FORMAT_CASE(DRM_FORMAT_BGRX5551)
	FORMAT_CASE(DRM_FORMAT_ARGB1555)
	FORMAT_CASE(DRM_FORMAT_ABGR1555)
	FORMAT_CASE(DRM_FORMAT_RGBA5551)
	FORMAT_CASE(DRM_FORMAT_BGRA5551)
	FORMAT_CASE(DRM_FORMAT_RGB565)
	FORMAT_CASE(DRM_FORMAT_BGR565)
	FORMAT_CASE(DRM_FORMAT_RGB888)
	FORMAT_CASE(DRM_FORMAT_BGR888)
	FORMAT_CASE(DRM_FORMAT_XRGB8888)
	FORMAT_CASE(DRM_FORMAT_XBGR8888)
	FORMAT_CASE(DRM_FORMAT_RGBX8888)
	FORMAT_CASE(DRM_FORMAT_BGRX8888)
	FORMAT_CASE(DRM_FORMAT_ARGB8888)
	FORMAT_CASE(DRM_FORMAT_ABGR8888)
	FORMAT_CASE(DRM_FORMAT_RGBA8888)
	FORMAT_CASE(DRM_FORMAT_BGRA8888)
	FORMAT_CASE(DRM_FORMAT_XRGB2101010)
	FORMAT_CASE(DRM_FORMAT_XBGR2101010)
	FORMAT_CASE(DRM_FORMAT_RGBX1010102)
	FORMAT_CASE(DRM_FORMAT_BGRX1010102)
	FORMAT_CASE(DRM_FORMAT_ARGB2101010)
	FORMAT_CASE(DRM_FORMAT_ABGR2101010)
	FORMAT_CASE(DRM_FORMAT_RGBA1010102)
	FORMAT_CASE(DRM_FORMAT_BGRA1010102)
	FORMAT_CASE(DRM_FORMAT_YUYV)
	FORMAT_CASE(DRM_FORMAT_YVYU)
	FORMAT_CASE(DRM_FORMAT_UYVY)
	FORMAT_CASE(DRM_FORMAT_VYUY)
	FORMAT_CASE(DRM_FORMAT_AYUV)
#ifdef DRM_FORMAT_XRGB8888_A8
	FORMAT_CASE(DRM_FORMAT_XRGB8888_A8)
#endif
#ifdef DRM_FORMAT_XBGR8888_A8
	FORMAT_CASE(DRM_FORMAT_XBGR8888_A8)
#endif
#ifdef DRM_FORMAT_RGBX8888_A8
	FORMAT_CASE(DRM_FORMAT_RGBX8888_A8)
#endif
#ifdef DRM_FORMAT_BGRX8888_A8
	FORMAT_CASE(DRM_FORMAT_BGRX8888_A8)
#endif
#ifdef DRM_FORMAT_RGB888_A8
	FORMAT_CASE(DRM_FORMAT_RGB888_A8)
#endif
#ifdef DRM_FORMAT_BGR888_A8
	FORMAT_CASE(DRM_FORMAT_BGR888_A8)
#endif
#ifdef DRM_FORMAT_RGB565_A8
	FORMAT_CASE(DRM_FORMAT_RGB565_A8)
#endif
#ifdef DRM_FORMAT_BGR565_A8
	FORMAT_CASE(DRM_FORMAT_BGR565_A8)
#endif
	FORMAT_CASE(DRM_FORMAT_NV12)
	FORMAT_CASE(DRM_FORMAT_NV21)
	FORMAT_CASE(DRM_FORMAT_NV16)
	FORMAT_CASE(DRM_FORMAT_NV61)
	FORMAT_CASE(DRM_FORMAT_NV24)
	FORMAT_CASE(DRM_FORMAT_NV42)
	FORMAT_CASE(DRM_FORMAT_YUV410)
	FORMAT_CASE(DRM_FORMAT_YVU410)
	FORMAT_CASE(DRM_FORMAT_YUV411)
	FORMAT_CASE(DRM_FORMAT_YVU411)
	FORMAT_CASE(DRM_FORMAT_YUV420)
	FORMAT_CASE(DRM_FORMAT_YVU420)
	FORMAT_CASE(DRM_FORMAT_YUV422)
	FORMAT_CASE(DRM_FORMAT_YVU422)
	FORMAT_CASE(DRM_FORMAT_YUV444)
	FORMAT_CASE(DRM_FORMAT_YVU444)
	}

	return "unknown";
}

#define FORMAT_IF(f) if (strcmp(name, #f) == 0) return f;

uint32_t kms_format_val(const char *name)
{
	FORMAT_IF(DRM_FORMAT_C8)
	else FORMAT_IF(DRM_FORMAT_R8)
#ifdef DRM_FORMAT_R16
	else FORMAT_IF(DRM_FORMAT_R16)
#endif
	else FORMAT_IF(DRM_FORMAT_RG88)
	else FORMAT_IF(DRM_FORMAT_GR88)
#ifdef DRM_FORMAT_RG1616
	else FORMAT_IF(DRM_FORMAT_RG1616)
#endif
#ifdef DRM_FORMAT_GR1616
	else FORMAT_IF(DRM_FORMAT_GR1616)
#endif
	else FORMAT_IF(DRM_FORMAT_RGB332)
	else FORMAT_IF(DRM_FORMAT_BGR233)
	else FORMAT_IF(DRM_FORMAT_XRGB4444)
	else FORMAT_IF(DRM_FORMAT_XBGR4444)
	else FORMAT_IF(DRM_FORMAT_RGBX4444)
	else FORMAT_IF(DRM_FORMAT_BGRX4444)
        else FORMAT_IF(DRM_FORMAT_ARGB4444)
        else FORMAT_IF(DRM_FORMAT_ABGR4444)
	else FORMAT_IF(DRM_FORMAT_RGBA4444)
	else FORMAT_IF(DRM_FORMAT_BGRA4444)
	else FORMAT_IF(DRM_FORMAT_XRGB1555)
	else FORMAT_IF(DRM_FORMAT_XBGR1555)
	else FORMAT_IF(DRM_FORMAT_RGBX5551)
	else FORMAT_IF(DRM_FORMAT_BGRX5551)
	else FORMAT_IF(DRM_FORMAT_ARGB1555)
	else FORMAT_IF(DRM_FORMAT_ABGR1555)
	else FORMAT_IF(DRM_FORMAT_RGBA5551)
	else FORMAT_IF(DRM_FORMAT_BGRA5551)
	else FORMAT_IF(DRM_FORMAT_RGB565)
	else FORMAT_IF(DRM_FORMAT_BGR565)
	else FORMAT_IF(DRM_FORMAT_RGB888)
	else FORMAT_IF(DRM_FORMAT_BGR888)
	else FORMAT_IF(DRM_FORMAT_XRGB8888)
	else FORMAT_IF(DRM_FORMAT_XBGR8888)
	else FORMAT_IF(DRM_FORMAT_RGBX8888)
	else FORMAT_IF(DRM_FORMAT_BGRX8888)
	else FORMAT_IF(DRM_FORMAT_ARGB8888)
	else FORMAT_IF(DRM_FORMAT_ABGR8888)
	else FORMAT_IF(DRM_FORMAT_RGBA8888)
	else FORMAT_IF(DRM_FORMAT_BGRA8888)
	else FORMAT_IF(DRM_FORMAT_XRGB2101010)
	else FORMAT_IF(DRM_FORMAT_XBGR2101010)
	else FORMAT_IF(DRM_FORMAT_RGBX1010102)
	else FORMAT_IF(DRM_FORMAT_BGRX1010102)
	else FORMAT_IF(DRM_FORMAT_ARGB2101010)
	else FORMAT_IF(DRM_FORMAT_ABGR2101010)
	else FORMAT_IF(DRM_FORMAT_RGBA1010102)
	else FORMAT_IF(DRM_FORMAT_BGRA1010102)
	else FORMAT_IF(DRM_FORMAT_YUYV)
	else FORMAT_IF(DRM_FORMAT_YVYU)
	else FORMAT_IF(DRM_FORMAT_UYVY)
	else FORMAT_IF(DRM_FORMAT_VYUY)
	else FORMAT_IF(DRM_FORMAT_AYUV)
#ifdef DRM_FORMAT_XRGB8888_A8
	else FORMAT_IF(DRM_FORMAT_XRGB8888_A8)
#endif
#ifdef DRM_FORMAT_XBGR8888_A8
	else FORMAT_IF(DRM_FORMAT_XBGR8888_A8)
#endif
#ifdef DRM_FORMAT_RGBX8888_A8
	else FORMAT_IF(DRM_FORMAT_RGBX8888_A8)
#endif
#ifdef DRM_FORMAT_BGRX8888_A8
	else FORMAT_IF(DRM_FORMAT_BGRX8888_A8)
#endif
#ifdef DRM_FORMAT_RGB888_A8
	else FORMAT_IF(DRM_FORMAT_RGB888_A8)
#endif
#ifdef DRM_FORMAT_BGR888_A8
	else FORMAT_IF(DRM_FORMAT_BGR888_A8)
#endif
#ifdef DRM_FORMAT_RGB565_A8
	else FORMAT_IF(DRM_FORMAT_RGB565_A8)
#endif
#ifdef DRM_FORMAT_BGR565_A8
	else FORMAT_IF(DRM_FORMAT_BGR565_A8)
#endif
	else FORMAT_IF(DRM_FORMAT_NV12)
	else FORMAT_IF(DRM_FORMAT_NV21)
	else FORMAT_IF(DRM_FORMAT_NV16)
	else FORMAT_IF(DRM_FORMAT_NV61)
	else FORMAT_IF(DRM_FORMAT_NV24)
	else FORMAT_IF(DRM_FORMAT_NV42)
	else FORMAT_IF(DRM_FORMAT_YUV410)
	else FORMAT_IF(DRM_FORMAT_YVU410)
	else FORMAT_IF(DRM_FORMAT_YUV411)
	else FORMAT_IF(DRM_FORMAT_YVU411)
	else FORMAT_IF(DRM_FORMAT_YUV420)
	else FORMAT_IF(DRM_FORMAT_YVU420)
	else FORMAT_IF(DRM_FORMAT_YUV422)
	else FORMAT_IF(DRM_FORMAT_YVU422)
	else FORMAT_IF(DRM_FORMAT_YUV444)
	else FORMAT_IF(DRM_FORMAT_YVU444)

	return 0;
}

int kms_format_bpp(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_NV12:
	case DRM_FORMAT_NV21:
	case DRM_FORMAT_NV16:
	case DRM_FORMAT_NV61:
	case DRM_FORMAT_YUV420:
	case DRM_FORMAT_YVU420:
		return 8;

	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_XRGB4444:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_XBGR4444:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_RGBX4444:
	case DRM_FORMAT_BGRA4444:
	case DRM_FORMAT_BGRX4444:
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_XRGB1555:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_XBGR1555:
	case DRM_FORMAT_RGBA5551:
	case DRM_FORMAT_RGBX5551:
	case DRM_FORMAT_BGRA5551:
	case DRM_FORMAT_BGRX5551:
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_UYVY:
	case DRM_FORMAT_VYUY:
	case DRM_FORMAT_YUYV:
	case DRM_FORMAT_YVYU:
		return 16;

	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_RGB888:
		return 24;

	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_ARGB2101010:
	case DRM_FORMAT_XRGB2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_RGBA1010102:
	case DRM_FORMAT_RGBX1010102:
	case DRM_FORMAT_BGRA1010102:
	case DRM_FORMAT_BGRX1010102:
		return 32;
	}

	return 0;
}

void kms_device_dump(struct kms_device *device)
{
	unsigned int i, j;

	printf("Screens: %u\n", device->num_screens);
	for (i = 0; i < device->num_screens; i++) {
		struct kms_screen *screen = device->screens[i];
		const char *status = "disconnected";

		if (screen->connected)
			status = "connected";

		printf("  %u: %x\n", i, screen->id);
		printf("    Status: %s\n", status);
		printf("    Name: %s\n", screen->name);
		printf("    Resolution: %ux%u\n", screen->width,
		       screen->height);
	}

	printf("Planes: %u\n", device->num_planes);
	for (i = 0; i < device->num_planes; i++) {
		struct kms_plane *plane = device->planes[i];
		const char *type = NULL;
		drmModeCrtcPtr crtc;
		drmModeFBPtr fb;

		switch (plane->type) {
		case DRM_PLANE_TYPE_OVERLAY:
			type = "overlay";
			break;
		case DRM_PLANE_TYPE_PRIMARY:
			type = "primary";
			break;
		case DRM_PLANE_TYPE_CURSOR:
			type = "cursor";
			break;
		}

		printf("  %u:\n", i);
		printf("    ID: 0x%x\n", plane->id);
		printf("    CRTC: 0x%x\n", plane->crtc->id);
		printf("    Type: 0x%x (%s)\n", plane->type, type);
		printf("    Formats: ");
		for (j = 0; j < plane->num_formats;j++) {
			printf("%s ", kms_format_str(plane->formats[j]));
		}
		printf("\n");

		crtc = drmModeGetCrtc(device->fd, plane->crtc->id);
		printf("    CRTC Mode: %s %d %d %d %d %d %d %d %d %d 0x%x 0x%x %d\n",
		       crtc->mode.name,
		       crtc->mode.vrefresh,
		       crtc->mode.hdisplay,
		       crtc->mode.hsync_start,
		       crtc->mode.hsync_end,
		       crtc->mode.htotal,
		       crtc->mode.vdisplay,
		       crtc->mode.vsync_start,
		       crtc->mode.vsync_end,
		       crtc->mode.vtotal,
		       crtc->mode.flags,
		       crtc->mode.type,
		       crtc->mode.clock);
		fb = drmModeGetFB(device->fd, crtc->buffer_id);
		if (!fb) {
			fprintf(stderr, "error drmModeGetFB()\n");
		} else {
			printf("    FB Id: 0x%x\n", fb->fb_id);
			printf("    FB Width: %d\n", fb->width);
			printf("    FB Height: %d\n", fb->height);
			printf("    FB Pitch: %d\n", fb->pitch);
			printf("    FB BPP: %d\n", fb->bpp);
			printf("    FB Depth: %d\n", fb->depth);
			printf("    FB Handle: %d\n", fb->handle);
		}
	}
}
