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

/*
 * This is a simple application that, when given a GEM name, will take a
 * screenshot of the framebuffer and save it to the specified PNG file.
 *
 * Basically, this is a screenshot program, but works only at the plane level.
 * However, the framebuffer does not necessary relect what is shown by display
 * hardware due to things like crop, pan, rotation, alpha blending, etc.
 */
#include "planes/engine.h"
#include "planes/fb.h"
#include "planes/kms.h"
#include <cairo.h>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xf86drm.h>

static void usage(const char* base)
{
	fprintf(stderr, "Usage: %s [OPTION]...\n", base);
	fprintf(stderr, "Grab a screenshot of a specific plane using a GEM name.\n\n");
	fprintf(stderr, "  -h, --help\t\t\tShow this menu.\n");
	fprintf(stderr, "  -v, --verbose\t\t\tShow verbose output.\n");
	fprintf(stderr, "  -n, --name=GEM_NAME\n");
	fprintf(stderr, "  -f, --filename=IMAGE_FILE\n");
	fprintf(stderr, "  -x, --width=WIDTH\n");
	fprintf(stderr, "  -y, --height=HEIGHT\n");
	fprintf(stderr, "  -d, --device=DEVICE\n");
}

int main(int argc, char* argv[])
{
	static const char opts[] = "hvn:f:x:y:d:";
	static struct option options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "verbose", no_argument, 0, 'v' },
		{ "name", required_argument, 0, 'n' },
		{ "filename", required_argument, 0, 'f' },
		{ "width", required_argument, 0, 'x' },
		{ "height", required_argument, 0, 'y' },
		{ "device", required_argument, 0, 'd' },
		{ 0, 0, 0, 0 },
	};
	struct kms_device* device;
	bool verbose = false;
	int opt, idx;
	int fd;
	int name = 1;
	void* ptr;
	const char* filename = "grab.png";
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t size = 0;
	const char* device_file = "atmel-hlcdc";
	cairo_t* cr;
	cairo_surface_t* surface;

	while ((opt = getopt_long(argc, argv, opts, options, &idx)) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			name = atoi(optarg);
			break;
		case 'x':
			width = strtoll(optarg, NULL, 0);
			break;
		case 'y':
			height = strtoll(optarg, NULL, 0);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		case 'd':
			device_file = optarg;
			break;
		default:
			fprintf(stderr, "unknown option \"%c\"\n", opt);
			return 1;
		}
	}

	if (optind < argc) {
		usage(argv[0]);
		return -1;
	}

	if (!width || !height) {
		fprintf(stderr, "width and height must be set\n");
		return -1;
	}

	fd = drmOpen(device_file, NULL);
	if (fd < 0) {
		fprintf(stderr, "open() failed: %m\n");
		return -1;
	}

	device = kms_device_open(fd);
	if (!device)
		return -1;

	ptr = fb_gem_map(device, name, &size);
	if (!ptr) {
		fprintf(stderr, "failed to map gem handle\n");
		goto abort;
	}

	if (verbose) {
		kms_device_dump(device);
	}

	surface = cairo_image_surface_create_for_data(ptr,
						      CAIRO_FORMAT_ARGB32,
						      width, height,
						      cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width));
	cr = cairo_create(surface);

	cairo_surface_write_to_png(surface,
				   filename);

	if (verbose) {
		printf("cairo status=%s\n", cairo_status_to_string(cairo_status(cr)));
	}

	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	fb_gem_unmap(ptr, size);

abort:
	kms_device_close(device);
	drmClose(fd);

	return 0;
}
