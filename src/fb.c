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

#include "planes/fb.h"
#include "planes/kms.h"
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <xf86drm.h>

static bool m_v = false;

#define LOG(stream, format, ...) if (m_v) fprintf(stream, format, ##__VA_ARGS__)

void* fb_gem_map(struct kms_device* device, uint32_t name, uint32_t* size)
{
	struct drm_gem_open gem;
	struct drm_mode_map_dumb args;
	int err;
	void* ptr;

	memset(&gem, 0, sizeof(gem));
	gem.name = name;
	err = drmIoctl(device->fd, DRM_IOCTL_GEM_OPEN, &gem);
	if (err < 0) {
		LOG(stderr, "could not flink %s\n", strerror(-err));
		return NULL;
	}

	memset(&args, 0, sizeof(args));
	args.handle = gem.handle;

	err = drmIoctl(device->fd, DRM_IOCTL_MODE_MAP_DUMB, &args);
	if (err < 0) {
		LOG(stderr, "could not map dumb %s\n", strerror(-errno));
		return NULL;
	}

	if (size)
		*size = gem.size;

	ptr = mmap(0, gem.size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   device->fd, args.offset);
	if (ptr == MAP_FAILED) {
		LOG(stderr, "could not mmap dumb %s\n", strerror(-errno));
		return NULL;
	}

	return ptr;
}

void fb_gem_unmap(void* buf, uint32_t size)
{
	munmap(buf, size);

	/*
	 * TODO: missing call to drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &open_arg.handle);
	 */
}
