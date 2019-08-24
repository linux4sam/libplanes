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
 * @brief Planes API
 */
#ifndef PLANES_PLANE_H
#define PLANES_PLANE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct kms_plane;
struct kms_framebuffer;
struct kms_device;

#define MAX_TEXT_STR_LEN 2048

/**
 * @brief Plane configuration.
 *
 * This includes configuration for the plane as well as parameters for the
 * demo engine.
 *
 * The basic operation to change plane configuration is to call the apropriate
 * functions to change properties, and then commit it all with a call to
 * plane_apply().
 */
struct plane_data
{
	/** The type of plane. */
	int type;
	/** Optional human name for the plane. */
	char name[255];
	/** Pointer to the underlying KMS plane object. */
	struct kms_plane* plane;
	/** Pointer to the underlying KMS framebuffer object. */
	struct kms_framebuffer** fbs;
	/** The framebuffer.  Must call plane_map() to allocate this. */
	void** bufs;
	unsigned int front_buf;
	/** The number of framebuffers. */
	uint32_t buffer_count;
	/** Plane type index, starting at zero. */
	unsigned int index;
	/** X coordinate of the plane. */
	int x;
	/** Y coordinate of the plane. */
	int y;
	/** Rotation degrees of the plane. */
	int rotate_degrees;
	int rotate_degrees_applied;
	/** Scale of the plane.  1.0 is no scale. */
	double scale;
	/** GEM name of the plane. */
	uint32_t* gem_names;
	/** Alpha value of the plane.  0 to 255. */
	uint32_t alpha;
	uint32_t alpha_applied;

	int move_flags;
	int transform_flags;

	struct
	{
		int xspeed;
		int yspeed;
		int xmin;
		int xmax;
		int ymin;
		int ymax;
	} move;

	struct
	{
		int x;
		int y;
		int width;
		int height;
		int xspeed;
		int yspeed;
	} pan;

	struct
	{
		double max;
		double min;
		double speed;
	} scaler;

	struct
	{
		int x;
		int y;
		int width;
		int height;
		int count;
		int index;
		int speed;
		int runningspeed;
	} sprite;

	struct
	{
		char str[MAX_TEXT_STR_LEN];
		int x;
		int y;
		uint32_t color;
		float size;
	} text[255];
};

/**
 * Create a plane.
 *
 * @param device The already created KMS device.
 * @param type The type of plane: DRM_PLANE_TYPE_PRIMARY,DRM_PLANE_TYPE_OVERLAY,
 *             DRM_PLANE_TYPE_CURSOR
 * @param index The index of the plane.
 * @param width The width in pixels of the plane.
 * @param height The height in pixels of the plane.
 * @param format A DRM format, or zero to automatically choose.
 */
struct plane_data* plane_create(struct kms_device* device, int type, int index,
				int width, int height, uint32_t format);

/**
 * Create a plane with one of more framebuffers.
 *
 * When more than one framebuffer is allocated, plane_flip() can be used to
 * iterate through them on the plane.
 *
 * @param device The already created KMS device.
 * @param type The type of plane: DRM_PLANE_TYPE_PRIMARY,DRM_PLANE_TYPE_OVERLAY,
 *             DRM_PLANE_TYPE_CURSOR
 * @param index The index of the plane.
 * @param width The width in pixels of the plane.
 * @param height The height in pixels of the plane.
 * @param format A DRM format, or zero to automatically choose.
 * @param buffer_count The number of buffers to allocate.
 */
struct plane_data* plane_create_buffered(struct kms_device* device, int type,
					 int index, int width, int height,
					 uint32_t format, uint32_t buffer_count);

/**
 * Free a plane allocated with plane_create() or plane_create_buffered().
 *
 * @param plane The plane.
 */
void plane_free(struct plane_data* plane);

/**
 * Set the rotate value of the plane.
 *
 * @param plane The plane.
 * @param degrees Degrees floored to the nearest 90 degree increment.
 * @see plane_apply()
 */
int plane_set_rotate(struct plane_data* plane, uint32_t degrees);

/**
 * Set the alpha value of the plane.
 *
 * @param plane The plane.
 * @param degrees Alpha value from 0 (transparent) to 255 (opaque).
 * @see plane_apply()
 */
int plane_set_alpha(struct plane_data* plane, uint32_t alpha);

/**
 * Set the plane position.
 * You must call plane_apply() to commit the change.
 *
 * @param x X coordinate of the plane.
 * @param y Y coordinate of the plane.
 * @see plane_apply()
 */
void plane_set_pos(struct plane_data* plane, int x, int y);

/**
 * Set the plane scale.
 * You must call plane_apply() to commit the change.
 *
 * @param scale Scale of the plane, where 1.0 is no scale.
 * @see plane_apply()
 */
void plane_set_scale(struct plane_data* plane, double scale);

/**
 * Apply the current properties of the plane.
 *
 * This must be called after any of the plane_set_* calls to actually apply the
 * configuration.
 *
 * @param plane The plane.
 */
int plane_apply(struct plane_data* plane);

/**
 * Immediately apply the rotate value.
 *
 * @param plane The plane.
 * @param degrees Rotation degrees of the plane.
 */
int plane_apply_rotate(struct plane_data* plane, uint32_t degrees);

/**
 * Immediately apply the alpha value.
 *
 * @param plane The plane.
 */
int plane_apply_alpha(struct plane_data* plane, uint32_t alpha);

/**
 * Get the plane width.
 * @param plane The plane.
 */
uint32_t plane_width(struct plane_data* plane);

/**
 * Get the plane height.
 * @param plane The plane.
 */
uint32_t plane_height(struct plane_data* plane);

/**
 * Get the plane DRM format.
 * @param plane The plane.
 */
uint32_t plane_format(struct plane_data* plane);

/**
 * Get the plane X coordinate.
 * @param plane The plane.
 */
int32_t plane_x(struct plane_data* plane);

/**
 * Get the plane Y coordinate.
 * @param plane The plane.
 */
int32_t plane_y(struct plane_data* plane);

/**
 * Get the plane rotate degrees.
 * @param plane The plane.
 */
int32_t plane_rotate(struct plane_data* plane);

/**
 * Get the plane alpha value.
 * @param plane The plane.
 */
uint32_t plane_alpha(struct plane_data* plane);

/**
 * Get the plane pan X coordinate.
 * @param plane The plane.
 */
int32_t plane_pan_x(struct plane_data* plane);

/**
 * Get the plane pan Y coordinate.
 * @param plane The plane.
 */
int32_t plane_pan_y(struct plane_data* plane);

/**
 * Get the plane pan width.
 * @param plane The plane.
 */
uint32_t plane_pan_width(struct plane_data* plane);

/**
 * Get the plane pan height.
 * @param plane The plane.
 */
uint32_t plane_pan_height(struct plane_data* plane);

/**
 * Set the plane pan position.
 * You must call plane_apply() to commit the change.
 *
 * @note The plane width and height must be non zero for panning to be enabled.
 *
 * @param x X coordinate of the pan.
 * @param y Y coordinate of the pan.
 * @see plane_apply()
 */
void plane_set_pan_pos(struct plane_data* plane, int x, int y);

/**
 * Set the plane pan size.
 * You must call plane_apply() to commit the change.
 *
 * @note The plane width and height must be non zero for panning to be enabled.
 *
 * @param width The width of the pan.
 * @param height The height of the pan.
 * @see plane_apply()
 */
void plane_set_pan_size(struct plane_data* plane, int width, int height);

/**
 * Hide/disable a plane.
 *
 * This does not delete any informaiton about the plane.  It only disables the
 * plane.  To re-enable the plane call plane_apply() again.
 *
 * @param plane The plane.
 */
void plane_hide(struct plane_data* plane);

/**
 * Map the framebuffer.
 *
 * This is not done by default because the process that calls plane_create()
 * or plane_create_buffered() isn't necessarily the one using the framebuffer.
 *
 * @param plane The plane.
 */
int plane_fb_map(struct plane_data* plane);

/**
 * Opposite of plane_fb_map().
 *
 * @param plane The plane.
 */
void plane_fb_unmap(struct plane_data* plane);

/**
 * Reallocate the framebuffer to the specified height, width, and format.
 *
 * @param width The width of the pan.
 * @param height The height of the pan.
 * @param format A DRM format, or zero to automatically choose.
 */
int plane_fb_reallocate(struct plane_data* plane,
			int width, int height, uint32_t format);

/**
 * Perform a flip to the target buffer index as the front buffer.
 *
 * When you allocate a plane, you can specify how many buffers to create for the
 * plane. This is usually going to be 1, 2, or 3.
 *
 * When you allocate more than 1 buffer, you can then start to refresh the
 * screen timed to vertical syncs of the display. This will prevent side effects
 * such as tearing. This is typically called double or tripple buffering based
 * on 2 or 3 buffers, respectively.
 *
 * @param plane The plane.
 * @param target The framebuffer index to switch to.
 */
int plane_flip(struct plane_data* plane, uint32_t target);

/**
 * Same as plane_flip(), but it does it immediately irrelevant of vsync.
 */
int plane_flip_async(struct plane_data* plane, uint32_t target);

#ifdef __cplusplus
}
#endif

#endif
