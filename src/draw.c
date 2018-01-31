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
#include "planes/draw.h"
#include "planes/engine.h"

#include <drm_fourcc.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static bool m_v = false;

#define LOG(stream, format, ...) if (m_v) fprintf(stream, format, ##__VA_ARGS__)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static cairo_format_t drm2cairo(uint32_t format)
{
	switch (format)
	{
	case DRM_FORMAT_RGB565:
		return CAIRO_FORMAT_RGB16_565;
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
		return CAIRO_FORMAT_ARGB32;
	}
	return CAIRO_FORMAT_INVALID;
}

int render_fb_text(struct kms_framebuffer* fb, int x, int y, const char* text,
		   uint32_t color, float size)
{
	void* ptr;
	int err;
	cairo_t* cr;
	cairo_surface_t* surface;
	struct rgba_color rgba;
	cairo_format_t cairo_format = drm2cairo(fb->format);

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		LOG(stderr, "error: kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return -1;
	}

	surface = cairo_image_surface_create_for_data(ptr,
						      cairo_format,
						      fb->width, fb->height,
						      cairo_format_stride_for_width(cairo_format, fb->width));
	cr = cairo_create(surface);

	parse_color(color, &rgba);
	cairo_set_source_rgba(cr, rgba.r, rgba.g, rgba.b, rgba.a);
	cairo_select_font_face(cr, "Serif", CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, size);
	cairo_move_to(cr, x, y);
	cairo_show_text(cr, text);
	cairo_stroke(cr);

	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	return 0;
}

static int draw_checker_pattern(cairo_t *cr, uint32_t colors[2], cairo_format_t cairo_format)
{
	cairo_t* cr2;
	cairo_surface_t* surface;
	const uint32_t block_size = 16;
	uint32_t x, y;

	surface = cairo_image_surface_create(cairo_format,
					     block_size * 2, block_size * 2);
	if (!surface)
		return -1;

	cr2 = cairo_create(surface);
	if (!cr2)
		return -1;


	/*
	 * The algorithm used here is to just draw 4 squares of the pattern on a
	 * temp surface and then paint it to the real surface using
	 * CAIRO_EXTEND_REPEAT.
	 */

	for (y = 0; y < block_size * 2; y++) {
		for (x = 0; x < block_size * 2; x++) {
			uint32_t color = (y / block_size) ^
				(x / block_size);

			struct rgba_color rgba;
			parse_color(colors[color & 1], &rgba);
			cairo_set_source_rgba(cr2, rgba.r, rgba.g, rgba.b, rgba.a);
			cairo_set_operator(cr2, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle (cr2, x, y, block_size, block_size);
			cairo_fill(cr2);
		}
	}

	cairo_save(cr);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_REPEAT);
	cairo_paint(cr);
	cairo_restore(cr);

	cairo_surface_destroy(surface);
	cairo_destroy(cr2);

	return 0;
}

static int draw_vertical_gradient(cairo_t *cr, uint32_t colors[2])
{
	cairo_surface_t* surface = cairo_get_target(cr);
	int width = cairo_image_surface_get_width(surface);
	int height = cairo_image_surface_get_height(surface);
	cairo_pattern_t *pat;
	struct rgba_color rgba1, rgba2;

	pat = cairo_pattern_create_linear(width/2, 0, width/2, height);

	parse_color(colors[0], &rgba1);
	cairo_pattern_add_color_stop_rgb(pat, 0.0, rgba1.r, rgba1.g, rgba1.b);
	cairo_pattern_add_color_stop_rgb(pat, 1.0, rgba1.r, rgba1.g, rgba1.b);

	parse_color(colors[1], &rgba2);
	cairo_pattern_add_color_stop_rgb(pat, 0.5, rgba2.r, rgba2.g, rgba2.b);

	cairo_save(cr);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_set_source(cr, pat);
	cairo_fill(cr);
	cairo_restore(cr);

	cairo_pattern_destroy(pat);

	return 0;
}

int render_fb_checker_pattern(struct kms_framebuffer* fb, uint32_t color1, uint32_t color2)
{
	void* ptr;
	int err;
	cairo_t* cr;
	cairo_surface_t* surface;
	uint32_t colors[2] = { color1, color2 };
	cairo_format_t cairo_format = drm2cairo(fb->format);

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		LOG(stderr, "error: kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return -1;
	}

	surface = cairo_image_surface_create_for_data(ptr,
						      cairo_format,
						      fb->width, fb->height,
						      cairo_format_stride_for_width(cairo_format, fb->width));
	cr = cairo_create(surface);

	draw_checker_pattern(cr, colors, cairo_format);

	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	kms_framebuffer_unmap(fb);

	return 0;
}

/*
 * Based on cairo's test mesh-pattern-control-points.c
 */
static void draw_mesh(cairo_t* cr, int width, int height)
{
	cairo_pattern_t *pattern;
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	cairo_translate(cr, 0, 0);
	pattern = cairo_pattern_create_mesh();
	cairo_mesh_pattern_begin_patch(pattern);
	cairo_mesh_pattern_move_to(pattern,    0,    0);
	cairo_mesh_pattern_line_to(pattern, width,    0);
	cairo_mesh_pattern_line_to(pattern, width, height);
	cairo_mesh_pattern_line_to(pattern,    0, height);
	cairo_mesh_pattern_set_corner_color_rgb(pattern, 0, 1, 0, 0);
	cairo_mesh_pattern_set_corner_color_rgb(pattern, 1, 0, 1, 0);
	cairo_mesh_pattern_set_corner_color_rgb(pattern, 2, 0, 0, 1);
	cairo_mesh_pattern_set_corner_color_rgb(pattern, 3, 1, 1, 0);
	cairo_mesh_pattern_set_control_point(pattern, 0, width * .7, height * .7);
	cairo_mesh_pattern_set_control_point(pattern, 1, width * .9, height * .7);
	cairo_mesh_pattern_set_control_point(pattern, 2, width * .9, height * .9);
	cairo_mesh_pattern_set_control_point(pattern, 3, width * .7, height * .9);
	cairo_mesh_pattern_end_patch(pattern);
	cairo_set_source(cr, pattern);
	cairo_paint(cr);
	cairo_pattern_destroy(pattern);
}

int render_fb_mesh_pattern(struct kms_framebuffer* fb)
{
	void* ptr;
	int err;
	cairo_t* cr;
	cairo_surface_t* surface;
	cairo_format_t cairo_format = drm2cairo(fb->format);

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		LOG(stderr, "error: kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return -1;
	}

	surface = cairo_image_surface_create_for_data(ptr,
						      cairo_format,
						      fb->width, fb->height,
						      cairo_format_stride_for_width(cairo_format, fb->width));
	cr = cairo_create(surface);


	draw_mesh(cr, fb->width, fb->height);

	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	kms_framebuffer_unmap(fb);

	return 0;
}

int render_fb_vgradient(struct kms_framebuffer* fb, uint32_t color1,
			uint32_t color2)
{
	void* ptr;
	int err;
	cairo_t* cr;
	cairo_surface_t* surface;
	uint32_t colors[2] = { color1, color2 };
	cairo_format_t cairo_format = drm2cairo(fb->format);

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		LOG(stderr, "error: kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return -1;
	}

	surface = cairo_image_surface_create_for_data(ptr,
						      cairo_format,
						      fb->width, fb->height,
						      cairo_format_stride_for_width(cairo_format, fb->width));
	cr = cairo_create(surface);

	draw_vertical_gradient(cr, colors);

	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	kms_framebuffer_unmap(fb);

	return 0;
}

static cairo_surface_t *
scale_surface(cairo_surface_t *old_surface,
	      int old_width, int old_height,
	      int new_width, int new_height)
{
	cairo_surface_t *new_surface = cairo_surface_create_similar(old_surface,
								    CAIRO_CONTENT_COLOR_ALPHA,
								    new_width,
								    new_height);
	cairo_t *cr = cairo_create (new_surface);

	/* Scale *before* setting the source surface (1) */
	cairo_scale(cr, (double)new_width / old_width, (double)new_height / old_height);
	cairo_set_source_surface(cr, old_surface, 0, 0);

	/* To avoid getting the edge pixels blended with 0 alpha, which would
	 * occur with the default EXTEND_NONE. Use EXTEND_PAD for 1.2 or newer (2)
	 */
	cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_REFLECT);

	/* Replace the destination with the source instead of overlaying */
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

	/* Do the actual drawing */
	cairo_paint (cr);

	cairo_destroy (cr);

	return new_surface;
}

int render_fb_image(struct kms_framebuffer* fb, const char* filename)
{
	void* ptr;
	int err;
	cairo_t* cr;
	cairo_surface_t* surface;
	cairo_surface_t* image;
	cairo_format_t cairo_format = drm2cairo(fb->format);

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		LOG(stderr, "error: kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return -1;
	}

	LOG(stdout, "loading image %s ... ", filename);
	fflush(stdout);

	surface = cairo_image_surface_create_for_data(ptr,
						      cairo_format,
						      fb->width, fb->height,
						      cairo_format_stride_for_width(cairo_format, fb->width));
	cr = cairo_create(surface);

	image = cairo_image_surface_create_from_png(filename);

	LOG("size %dx%d\n",
	    cairo_image_surface_get_width(image),
	    cairo_image_surface_get_height(image));

	if (cairo_image_surface_get_width(image) != (int)fb->width ||
	    cairo_image_surface_get_height(image) != (int)fb->height) {

		LOG("image scaled to %dx%d\n", fb->width, fb->height);
		image = scale_surface(image,
				      cairo_image_surface_get_width(image),
				      cairo_image_surface_get_height(image),
				      fb->width,
				      fb->height);
	}

	cairo_set_source_surface(cr, image, 0, 0);
	cairo_paint(cr);
	cairo_surface_destroy(image);

	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	kms_framebuffer_unmap(fb);

	return 0;
}

/*
static uint32_t reverse32(uint32_t dword)
{
	return ((dword >> 24) & 0x000000FF) |
		((dword >> 8) & 0x0000FF00) |
		((dword << 8) & 0x00FF0000) |
		((dword << 24) & 0xFF000000);
}

static uint16_t reverse16(uint16_t dword)
{
	return ((dword >> 8) & 0x000000FF) |
		((dword << 8) & 0x0000FF00);
}

static void blit8(uint8_t* src, uint32_t swidth, uint32_t sheight,
		  uint8_t* dst, uint32_t dwidth, uint32_t dheight)
{
	uint32_t x, y;
	for (x = 0; x < swidth; x++) {
		for (y = 0; y < sheight; y++) {
			if (x < dwidth && y < dheight) {
				uint32_t soff = (y * swidth) + x;
				uint32_t doff = (y * dwidth) + x;
				*(dst+doff) = *(src+soff);
			}
		}
	}
}

static void blit16(uint16_t* src, uint32_t swidth, uint32_t sheight,
		   uint16_t* dst, uint32_t dwidth, uint32_t dheight,
		   int reverse_endian)
{
	uint32_t x, y;
	for (x = 0; x < swidth; x++) {
		for (y = 0; y < sheight; y++) {
			if (x < dwidth && y < dheight) {
				uint32_t soff = (y * swidth) + x;
				uint32_t doff = (y * dwidth) + x;
				if (reverse_endian)
					*(dst+doff) = reverse16(*(src+soff));
				else
					*(dst+doff) = *(src+soff);
			}
		}
	}
}

static void blit32(uint32_t* src, uint32_t swidth, uint32_t sheight,
		   uint32_t* dst, uint32_t dwidth, uint32_t dheight,
		   int reverse_endian)
{
	uint32_t x, y;
	for (x = 0; x < swidth; x++) {
		for (y = 0; y < sheight; y++) {
			if (x < dwidth && y < dheight) {
				uint32_t soff = (y * swidth) + x;
				uint32_t doff = (y * dwidth) + x;
				if (reverse_endian)
					*(dst+doff) = reverse32(*(src+soff));
				else
					*(dst+doff) = *(src+soff);
			}
		}
	}
}
*/

int render_fb_image_raw(struct kms_framebuffer* fb, const char* filename)
{
	void* ptr;
	int err;
	uint8_t* img;
	FILE* f;

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		LOG(stderr, "error: kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return -1;
	}

	LOG(stdout, "loading image %s ... ", filename);
	fflush(stdout);

	f = fopen(filename, "rb");
	if (f) {
		long length;
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		img = malloc(length+1);
		if (img) {
			length = fread(img, 1, length, f);
			img[length] = 0;
		}
		fclose(f);

		LOG(stdout, "size %ld\n", length);

		memcpy(ptr, img, MIN((uint32_t)length,
				     fb->width * fb->height * kms_format_bpp(fb->format) / 8));

		free(img);
	} else {
		LOG(stderr, "error: failed to open file: %s\n", filename);
	}

	kms_framebuffer_unmap(fb);

	return 0;
}

int flip_fb_horizontal(struct kms_framebuffer* fb)
{
	void* ptr;
	int err;
	cairo_t* cr;
	cairo_t* cr2;
	cairo_surface_t* surface;
	cairo_surface_t* surface2;
	cairo_matrix_t matrix;
	cairo_format_t cairo_format = drm2cairo(fb->format);

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		LOG(stderr, "error: kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return -1;
	}

	surface = cairo_image_surface_create_for_data(ptr,
						      cairo_format,
						      fb->width, fb->height,
						      cairo_format_stride_for_width(cairo_format, fb->width));
	cr = cairo_create(surface);

	surface2 = cairo_image_surface_create(cairo_format,
					      fb->width, fb->height);

	cr2 = cairo_create(surface2);

	cairo_set_source_surface(cr2, surface, 0, 0);
	cairo_paint(cr2);


	cairo_matrix_init_identity(&matrix);
	matrix.xx = -1.0;
	matrix.x0 = fb->width;
	cairo_set_matrix(cr, &matrix);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, surface2, 0, 0);
	cairo_paint(cr);

	cairo_surface_destroy(surface);
	cairo_surface_destroy(surface2);

	cairo_destroy(cr);
	cairo_destroy(cr2);

	kms_framebuffer_unmap(fb);

	return 0;
}

int flip_fb_vertical(struct kms_framebuffer* fb)
{
	void* ptr;
	int err;
	cairo_t* cr;
	cairo_t* cr2;
	cairo_surface_t* surface;
	cairo_surface_t* surface2;
	cairo_matrix_t matrix;
	cairo_format_t cairo_format = drm2cairo(fb->format);

	err = kms_framebuffer_map(fb, &ptr);
	if (err < 0) {
		LOG(stderr, "error: kms_framebuffer_map() failed: %s\n",
			strerror(-err));
		return -1;
	}

	surface = cairo_image_surface_create_for_data(ptr,
						      cairo_format,
						      fb->width, fb->height,
						      cairo_format_stride_for_width(cairo_format, fb->width));
	cr = cairo_create(surface);

	surface2 = cairo_image_surface_create(cairo_format,
					      fb->width, fb->height);

	cr2 = cairo_create(surface2);

	cairo_set_source_surface(cr2, surface, 0, 0);
	cairo_paint(cr2);

	cairo_matrix_init_identity(&matrix);
	matrix.yx = -1.0;
	matrix.y0 = fb->height;
	cairo_set_matrix(cr, &matrix);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, surface2, 0, 0);
	cairo_paint(cr);

	cairo_surface_destroy(surface);
	cairo_surface_destroy(surface2);

	cairo_destroy(cr);
	cairo_destroy(cr2);

	kms_framebuffer_unmap(fb);

	return 0;
}
