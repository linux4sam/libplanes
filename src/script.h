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
#ifndef PLANES_SCRIPT_H
#define PLANES_SCRIPT_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

int script_init(lua_State* newstate);
int script_load(const char *expr, char **pmsg);
double script_eval(int cookie, char **pmsg);
void script_unref(int cookie);
void script_setvar(const char *name, double value);
double script_getvar(const char *name);

typedef int (*SCRIPT_CALLBACK)(lua_State*);
void script_setfunc(const char *name, SCRIPT_CALLBACK callback);

#endif
