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
 * @brief Planes FB API
 */
#ifndef PLANES_FB_H
#define PLANES_FB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct kms_device;

/**
 * Given a GEM name, map a framebuffer.
 *
 * This is not wraped in a struct plane_data because this is nothing more than a
 * framebuffer and a size in byts of the buffer. It cannot be configured and
 * things like pixel format and resolution have to be obtained through other
 * means.
 *
 * @param device The already created KMS device.
 * @param name The global GEM name, actually an integer.
 * @param size Returned size in bytes of the buffer.
 */
void* fb_gem_map(struct kms_device* device, uint32_t name, uint32_t* size);

/**
 * Opposite of gem_map().
 *
 * @param buf The pointer returnd by gem_map().
 * @param size The size returnd by gem_map().
 */
void fb_gem_unmap(void* buf, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
