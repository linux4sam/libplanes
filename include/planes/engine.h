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
/**
 * @file
 * @brief Engine API
 */
#ifndef PLANES_ENGINE_H
#define PLANES_ENGINE_H

#include "planes/plane.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Move types for planes.
 *
 * Move type tells the engine to manipulate some property of the plane each
 * frame.  For example, increment the X position of the plane, or increment the
 * scale of the plane within the specified bounds.
 */
enum {
	MOVE_NONE = 0,
	/** Warp an object in one direction off screen. */
	MOVE_X_WARP = (1<<0),
	/** Bounce on screen width. */
	MOVE_X_BOUNCE = (1<<1),
	/** Bounce on custom min and max. */
	MOVE_X_BOUNCE_CUSTOM = (1<<2),
	/** Bounce on screen width + plane width. */
	MOVE_X_BOUNCE_OUT = (1<<3),
	/** Warp an object in one direction off screen. */
	MOVE_Y_WARP = (1<<4),
	/** Bounce on screen hight. */
	MOVE_Y_BOUNCE = (1<<5),
	/** Bounce the Y position with custom bounds. */
	MOVE_Y_BOUNCE_CUSTOM = (1<<6),
	/** Bounce on screen height + plane height. */
	MOVE_Y_BOUNCE_OUT = (1<<7),
	/** Bounce the pan X at the bounds of framebuffer width. */
	MOVE_PANX_BOUNCE = (1<<8),
	/** Bounce the pan Y at the bounds of framebuffer height. */
	MOVE_PANY_BOUNCE = (1<<9),
	/** Warp the pan X at the bounds of framebuffer width. */
	MOVE_PANX_WARP = (1<<10),
	/** Warp the pan Y at the bounds of framebuffer height. */
	MOVE_PANY_WARP = (1<<11),
	/** Scale the plane between a min and max. */
	MOVE_SCALER = (1<<12),
	/** Similar to pan operations, but with parameters to work on a sprite sheet. */
	MOVE_SPRITE = (1<<13),
};

/**
 * Transform types for planes.
 *
 * The basic engine implementation of transforms is done at a trigger point. A
 * trigger point is defined as when some other move is occuring and it switches
 * direction or hits a boundary condition.  When that occurs, the transform is
 *  applied.
 */
enum {
	TRANSFORM_NONE = 0,
	TRANSFORM_FLIP_HORIZONTAL = (1<<0),
	TRANSFORM_FLIP_VERTICAL = (1<<1),
	TRANSFORM_ROTATE_CLOCKWISE = (1<<2),
	TRANSFORM_ROTATE_CCLOCKWISE = (1<<3),
};

/**
 * Parse a JSON plane configuration file and populate the planes array.
 *
 * See @ref docs/config.md for information on the config file format.
 *
 * @note This will overwrite anything that may already be in the array.
 *
 * @param config_file Config file path to parse.
 * @param device The already created KMS device.
 * @param planes Pre-allocated array.
 * @param num_planes Number of planes in array.
 * @param framedelay Resulting delay in milliseconds for each frame.
 */
int engine_load_config(const char* config_file, struct kms_device *device,
		       struct plane_data** planes, uint32_t num_planes,
		       uint32_t* framedelay);

/**
 * Run the engine over the configurd planes array until max_frames is reached if
 * it is a positive value.
 *
 * @param device The already created KMS device.
 * @param planes Array of plane_data pointers.
 * @param num_planes Number of planes in array.
 * @param framedelay Delay in milliseconds for each frame.
 * @param max_frames The maximum number of frames to run befoe returning.
 */
void engine_run(struct kms_device* device, struct plane_data** planes,
		uint32_t num_planes, uint32_t framedelay, uint32_t max_frames);

/**
 * Execute a single frame of the engine.
 *
 * This is useful for finer gained control of running the engine.
 *
 * @param device The already created KMS device.
 * @param planes Array of plane_data pointers.
 * @param num_planes Number of planes in array.
 * @param framedelay Delay in milliseconds for each frame.
 */
void engine_run_once(struct kms_device* device, struct plane_data** planes,
		     uint32_t num_planes, uint32_t framedelay);

/**
 * @brief RGBA color broken out into floating point components.
 */
struct rgba_color
{
	double r;
	double g;
	double b;
	double a;
};

/**
 * Parse an RGBA integer value into floating point components.
 *
 * The format of the integer is essentially 0xRRGGBBAA, giving each components a
 * range of 0-255.
 *
 * @param in The RGBA value.
 * @param color Resulting floating point components.
 */
void parse_color(uint32_t in, struct rgba_color* color);

#ifdef __cplusplus
}
#endif

#endif
