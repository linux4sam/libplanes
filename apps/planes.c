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
 * This application uses libdrm directly to allocate planes based on a
 * configuration file and then performs various operations on the planes.
 */
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xf86drm.h>

#include "planes/engine.h"
#include "planes/kms.h"

static void usage(const char* base)
{
	fprintf(stderr, "Usage: %s [OPTION]...\n", base);
	fprintf(stderr, "Load a config file, configure planes, and run the engine.\n\n");
	fprintf(stderr, "  -h, --help\t\t\tShow this menu.\n");
	fprintf(stderr, "  -v, --verbose\t\t\tShow verbose output.\n");
	fprintf(stderr, "  -c, --config=CONFIG\t\tSet the config file to read.\n");
	fprintf(stderr, "  -d, --device=DEVICE\t\tSet the DRI device to open.\n");
	fprintf(stderr, "  -f, --frames=MAX_FRAMES\tSet the maximum number of frames to render and then exit.\n");
}

static struct kms_device *device = NULL;

static void exit_handler(int s) {
	kms_device_close(device);
	exit(1);
}

int main(int argc, char *argv[])
{
	static const char opts[] = "hvc:d:f:";
	static struct option options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "verbose", no_argument, 0, 'v' },
		{ "config", required_argument, 0, 'c' },
		{ "device", required_argument, 0, 'd' },
		{ "frames", required_argument, 0, 'f' },
		{ 0, 0, 0, 0 },
	};
	bool verbose = false;
	unsigned int i;
	int opt, idx;
	int fd;
	const char* config_file = "default.config";
	const char* device_file = "atmel-hlcdc";
	uint32_t framedelay = 33;
	uint32_t max_frames = 0;
	struct plane_data** planes;
	struct stat s;
	struct sigaction sig_handler;

	if (stat(config_file, &s) &&
	    !stat("/usr/share/planes/default.config", &s)) {
		config_file = "/usr/share/planes/default.config";
	}

	while ((opt = getopt_long(argc, argv, opts, options, &idx)) != -1) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'v':
			verbose = true;
			break;
                case 'c':
			config_file = optarg;
			break;
		case 'd':
			device_file = optarg;
			break;
		case 'f':
			max_frames = strtoll(optarg, NULL, 0);
			break;
		default:
			fprintf(stderr, "error: unknown option \"%c\"\n", opt);
			return 1;
		}
	}

	if (optind < argc) {
		usage(argv[0]);
		return 1;
	}

	sig_handler.sa_handler = exit_handler;
	sigemptyset(&sig_handler.sa_mask);
	sig_handler.sa_flags = 0;
	sigaction(SIGINT, &sig_handler, NULL);

	fd = drmOpen(device_file, NULL);
	if (fd < 0) {
		fprintf(stderr, "error: open() failed: %m\n");
		return 1;
	}

	device = kms_device_open(fd);
	if (!device)
		return 1;

	if (verbose) {
		kms_device_dump(device);
	}

	planes = calloc(device->num_planes, sizeof(struct plane_data*));

	if (!engine_load_config(config_file, device, planes, device->num_planes, &framedelay)) {
		engine_run(device, planes, device->num_planes, framedelay,
			   max_frames);
	} else {
		fprintf(stderr, "error: failed to load config file %s\n", config_file);
	}

	for (i = 0; i < device->num_planes;i++) {
		if (planes[i])
			plane_free(planes[i]);
	}

	kms_device_close(device);
	drmClose(fd);
	free(planes);

	return 0;
}
