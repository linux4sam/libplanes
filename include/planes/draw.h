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
 * @brief Draw API
 *
 * This is a basic drawing API that uses Cairo as the backend.  This is used by
 * the engine, and can be used directly, but for more advanced graphics another
 * framework should be used.
 */
#ifndef PLANES_DRAW_H
#define PLANES_DRAW_H

#include "planes/kms.h"

#include <cairo.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Render text at the specified location in the framebuffer.
 * @note Location is relative to bottom left of text.
 *
 * @param fb The framebuffer.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param text The text to render.
 * @param color RGB color to use for foreground color.
 */
int render_fb_text(struct kms_framebuffer* fb, int x, int y, const char* text,
		   uint32_t color);

/**
 * Render a checker pattern to the framebuffer.
 *
 * @param fb The framebuffer.
 * @param color1 First patterm RGB color.
 * @param color2 Second pattern RGB color.
 */
int render_fb_checker_pattern(struct kms_framebuffer* fb, uint32_t color1,
			      uint32_t color2);

/**
 * Render a mesh pattern to the framebuffer.
 *
 * @param fb The framebuffer.
 */
int render_fb_mesh_pattern(struct kms_framebuffer* fb);

/**
 * Render a vertical gradient to the framebuffer.
 *
 * @param fb The framebuffer.
 * @param color1 First gradient RGB color.
 * @param color2 Second gradient RGB color.
 */
int render_fb_vgradient(struct kms_framebuffer* fb, uint32_t color1,
			uint32_t color2);

/**
 * Load and render the specified image file to the framebuffer.
 *
 * @param fb The framebuffer.
 * @param filename The image file path.
 */
int render_fb_image(struct kms_framebuffer* fb, const char* filename);

/**
 * Load and render a raw image file to the framebuffer (memcpy).
 *
 * @param fb The framebuffer.
 * @param filename The image file path.
 */
int render_fb_image_raw(struct kms_framebuffer* fb, const char* filename);

/**
 * Transform flip the framebuffer horizontaly.
 *
 * @param fb The framebuffer.
 */
int flip_fb_horizontal(struct kms_framebuffer* fb);

/**
 * Transform flip the framebuffer vertically.
 *
 * @param fb The framebuffer.
 */
int flip_fb_vertical(struct kms_framebuffer* fb);

#ifdef __cplusplus
}
#endif

#endif
