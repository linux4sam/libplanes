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
#include "script.h"
#include <stdlib.h>
#include <string.h>

/*
 * Based on
 * https://stackoverflow.com/questions/1151127/evaluating-mathematical-expressions
 */

static lua_State* state = NULL;

int script_init(lua_State* newstate)
{
	if (state)
		return 1;

	if (newstate)
		state = newstate;
	else
		state = luaL_newstate();
	if (state)
		luaL_openlibs(state);
	return !!state;
}

int script_load(const char *expr, char **pmsg)
{
	int err;
	char *buf;

	if (!state) {
		if (pmsg)
			*pmsg = strdup("LE library not initialized");
		return LUA_NOREF;
	}
	buf = malloc(strlen(expr)+8);
	if (!buf) {
		if (pmsg)
			*pmsg = strdup("Insufficient memory");
		return LUA_NOREF;
	}
	strcpy(buf, "return ");
	strcat(buf, expr);
	err = luaL_loadstring(state, buf);
	free(buf);
	if (err) {
		if (pmsg)
			*pmsg = strdup(lua_tostring(state,-1));
		lua_pop(state,1);
		return LUA_NOREF;
	}
	if (pmsg)
		*pmsg = NULL;
	return luaL_ref(state, LUA_REGISTRYINDEX);
}

double script_eval(int cookie, char **pmsg)
{
	int err;
	double ret;

	lua_rawgeti(state, LUA_REGISTRYINDEX, cookie);
	err = lua_pcall(state, 0, 1, 0);
	if (err) {
		if (pmsg)
			*pmsg = strdup(lua_tostring(state, -1));
		lua_pop(state, 1);
		return 0;
	}
	if (pmsg)
		*pmsg = NULL;
	ret = lua_tonumber(state, -1);
	lua_pop(state, 1);
	return ret;
}

void script_unref(int cookie)
{
	luaL_unref(state, LUA_REGISTRYINDEX, cookie);
}

void script_setvar(const char *name, double value)
{
	lua_pushnumber(state, value);
	lua_setglobal(state, name);
}

void script_setfunc(const char *name, SCRIPT_CALLBACK callback)
{
	lua_register(state, name, callback);
}

double script_getvar(const char *name)
{
	double ret;

	lua_getglobal(state, name);
	ret = lua_tonumber(state, -1);
	lua_pop(state, 1);
	return ret;
}
