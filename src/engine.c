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
#include "cjson/cJSON.h"
#include "common.h"
#include "planes/engine.h"
#include "planes/kms.h"
#include "planes/draw.h"
#include "script.h"

#include <cairo.h>
#include <drm_fourcc.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>

static void configure_plane(struct plane_data* plane, uint32_t colors[2],
			    bool vgradient, bool mesh, const char* filename,
			    const char* filename_raw)
{
	int x;

	if (mesh)
		render_fb_mesh_pattern(plane->fb);
	else if (vgradient)
		render_fb_vgradient(plane->fb, colors[0], colors[1]);
	else
		render_fb_checker_pattern(plane->fb, colors[0], colors[1]);

	if (filename)
		render_fb_image(plane->fb, filename);

	if (filename_raw)
		render_fb_image_raw(plane->fb, filename_raw);

	for (x = 0; x < 255;x++)
		if (strlen(plane->text[x].str))
			render_fb_text(plane->fb, plane->text[x].x, plane->text[x].y,
				       plane->text[x].str, plane->text[x].color,
				       plane->text[x].size);
}

/*
 * Take the given file path and compute a full path for filename relative to it.
 * @return The returned valid pointer must be freed.
 */
static const char* reldir(const char* path, const char* filename)
{
	char* res = 0;
	char* path2;
	char* dir;

	if (filename && strlen(filename) && filename[0] != '/') {
		path2 = strdup(path);
		dir = dirname(path2);

		res = malloc(strlen(dir) + strlen(filename) + 2);
		if (res)
			sprintf(res, "%s/%s", dir, filename);
		free(path2);
		return res;
	}

	return filename;
}

static cJSON* load_config(const char* filename)
{
	cJSON* root = NULL;
	FILE* f = fopen(filename, "rb");

	if (f) {
		long length;
		char* buffer;
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length+1);
		if (buffer) {
			length = fread(buffer, 1, length, f);
			buffer[length] = 0;
		}
		fclose(f);

		root = cJSON_Parse(buffer);
		if (!root)
			LOG("error: parse error at: %s\n", cJSON_GetErrorPtr());
		free(buffer);
	}

	return root;
}

struct
{
	const char* s;
	int v;
} move_map[] = {
	{"x-warp", MOVE_X_WARP},
	{"x-bounce", MOVE_X_BOUNCE},
	{"x-bounce-custom", MOVE_X_BOUNCE_CUSTOM},
	{"x-bounce-out", MOVE_X_BOUNCE_OUT},
	{"y-warp", MOVE_Y_WARP},
	{"y-bounce", MOVE_Y_BOUNCE},
	{"y-bounce-custom", MOVE_Y_BOUNCE_CUSTOM},
	{"y-bounce-out", MOVE_Y_BOUNCE_OUT},
	{"panx-bounce", MOVE_PANX_BOUNCE},
	{"pany-bounce", MOVE_PANY_BOUNCE},
	{"panx-warp", MOVE_PANX_WARP},
	{"pany-warp", MOVE_PANY_WARP},
	{"scaler", MOVE_SCALER},
	{"sprite", MOVE_SPRITE},
};

struct
{
	const char* s;
	int v;
} transform_map[] = {
	{"flip-horizontal", TRANSFORM_FLIP_HORIZONTAL},
	{"flip-vertical", TRANSFORM_FLIP_VERTICAL},
	{"rotate-clockwise", TRANSFORM_ROTATE_CLOCKWISE},
	{"rotate-cclockwise", TRANSFORM_ROTATE_CCLOCKWISE},
};

static int logical_to_physical_width(lua_State* state)
{
	double width = lua_tonumber(state, 1);
	double screen_width = script_getvar("SCREEN_WIDTH");
	double logical_width = script_getvar("LOGICAL_WIDTH");

	lua_pushnumber(state, width / logical_width * screen_width);

	return 1;
}

static int logical_to_physical_height(lua_State* state)
{
	double height = lua_tonumber(state, 1);
	double screen_height = script_getvar("SCREEN_HEIGHT");
	double logical_height = script_getvar("LOGICAL_HEIGHT");

	lua_pushnumber(state, height / logical_height * screen_height);

	return 1;
}

static double lua_evaluate(const char* expr, struct kms_device* device)
{
	int cookie;
	char *msg = NULL;
	double y = 0.;

	if (!script_init()) {
		LOG("can't init lua\n");
		return y;
	}
	cookie = script_load(expr, &msg);
	if (msg) {
		LOG("can't load expr: %s\n", msg);
		goto error;
	}

	script_setfunc("physicalw", logical_to_physical_width);
	script_setfunc("physicalh", logical_to_physical_height);
	script_setvar("SCREEN_WIDTH", device->screens[0]->width);
	script_setvar("SCREEN_HEIGHT", device->screens[0]->height);
	script_setvar("LOGICAL_WIDTH", 800);
	script_setvar("LOGICAL_HEIGHT", 480);

	y = script_eval(cookie, &msg);
	if (msg) {
		LOG("can't eval: %s\n", msg);
		goto error;
	}

error:
	if (msg)
		free(msg);
	script_unref(cookie);

	return y;
}

static int eval_expr(cJSON* val, struct kms_device* device, int default_value)
{
	if (cJSON_IsNumber(val))
		return val->valueint;
	else if (cJSON_IsString(val)) {
		if (strncmp("0x", val->valuestring, strlen("0x")) == 0)
			return strtol(val->valuestring, NULL, 16);
		else
			return lua_evaluate(val->valuestring, device);
	}
	return default_value;
}

static void add_text_entry(struct plane_data* data, cJSON* t, struct kms_device* device)
{
	int k;
	cJSON* text_str = cJSON_GetObjectItemCaseSensitive(t, "str");
	cJSON* text_x = cJSON_GetObjectItemCaseSensitive(t, "x");
	cJSON* text_y = cJSON_GetObjectItemCaseSensitive(t, "y");
	cJSON* text_color = cJSON_GetObjectItemCaseSensitive(t, "color");
	cJSON* text_size = cJSON_GetObjectItemCaseSensitive(t, "size");

	if (cJSON_IsString(text_str)) {
		for (k = 0; k < 255; k++) {
			if (!strlen(data->text[k].str)) {
				strncpy(data->text[k].str, text_str->valuestring, MAX_TEXT_STR_LEN-1);
				data->text[k].x = eval_expr(text_x, device, 0);
				data->text[k].y = eval_expr(text_y, device, 0);
				if (cJSON_IsString(text_color))
					data->text[k].color = strtoul(text_color->valuestring, NULL, 0);
				else
					data->text[k].color = 0x000000ff;

				data->text[k].size = eval_expr(text_size, device, 24);
				break;
			}
		}
	}
}

static struct plane_data* parse_plane(const char* config_file,
				      struct kms_device* device,
				      cJSON* plane)
{
	struct plane_data* data = NULL;
	cJSON* enabled = cJSON_GetObjectItemCaseSensitive(plane, "enabled");
	cJSON* name = cJSON_GetObjectItemCaseSensitive(plane, "name");
	cJSON* type = cJSON_GetObjectItemCaseSensitive(plane, "type");
	cJSON* index = cJSON_GetObjectItemCaseSensitive(plane, "index");
	cJSON* x = cJSON_GetObjectItemCaseSensitive(plane, "x");
	cJSON* y = cJSON_GetObjectItemCaseSensitive(plane, "y");
	cJSON* width = cJSON_GetObjectItemCaseSensitive(plane, "width");
	cJSON* height = cJSON_GetObjectItemCaseSensitive(plane, "height");
	cJSON* alpha = cJSON_GetObjectItemCaseSensitive(plane, "alpha");
	cJSON* format = cJSON_GetObjectItemCaseSensitive(plane, "format");
	cJSON* scale = cJSON_GetObjectItemCaseSensitive(plane, "scale");
	cJSON* rotate = cJSON_GetObjectItemCaseSensitive(plane, "rotate");
	cJSON* image = cJSON_GetObjectItemCaseSensitive(plane, "image");
	cJSON* image_raw = cJSON_GetObjectItemCaseSensitive(plane, "image-raw");
	cJSON* patternarray = cJSON_GetObjectItemCaseSensitive(plane, "pattern");
	cJSON* pattern1 = cJSON_GetArrayItem(patternarray, 0);
	cJSON* pattern2 = cJSON_GetArrayItem(patternarray, 1);
	cJSON* vgradientarray = cJSON_GetObjectItemCaseSensitive(plane, "vgradient");
	cJSON* vgradient1 = cJSON_GetArrayItem(vgradientarray, 0);
	cJSON* vgradient2 = cJSON_GetArrayItem(vgradientarray, 1);
	cJSON* patch = cJSON_GetObjectItemCaseSensitive(plane, "patch");
	cJSON* move_type = cJSON_GetObjectItemCaseSensitive(plane, "move-type");
	cJSON* transformarray = cJSON_GetObjectItemCaseSensitive(plane, "transform");

	cJSON* move_xspeed = cJSON_GetObjectItemCaseSensitive(plane, "move-xspeed");
	cJSON* move_yspeed = cJSON_GetObjectItemCaseSensitive(plane, "move-yspeed");
	cJSON* move_xmin = cJSON_GetObjectItemCaseSensitive(plane, "move-xmin");
	cJSON* move_xmax = cJSON_GetObjectItemCaseSensitive(plane, "move-xmax");
	cJSON* move_ymin = cJSON_GetObjectItemCaseSensitive(plane, "move-ymin");
	cJSON* move_ymax = cJSON_GetObjectItemCaseSensitive(plane, "move-ymax");

	cJSON* pan_x = cJSON_GetObjectItemCaseSensitive(plane, "pan-x");
	cJSON* pan_y = cJSON_GetObjectItemCaseSensitive(plane, "pan-y");
	cJSON* pan_width = cJSON_GetObjectItemCaseSensitive(plane, "pan-width");
	cJSON* pan_height = cJSON_GetObjectItemCaseSensitive(plane, "pan-height");
	cJSON* pan_yspeed = cJSON_GetObjectItemCaseSensitive(plane, "move-panyspeed");
	cJSON* pan_xspeed = cJSON_GetObjectItemCaseSensitive(plane, "move-panxspeed");

	cJSON* scaler_min = cJSON_GetObjectItemCaseSensitive(plane, "scaler-min");
	cJSON* scaler_max = cJSON_GetObjectItemCaseSensitive(plane, "scaler-max");
	cJSON* scaler_speed = cJSON_GetObjectItemCaseSensitive(plane, "scaler-speed");

	cJSON* sprite_x = cJSON_GetObjectItemCaseSensitive(plane, "sprite-x");
	cJSON* sprite_y = cJSON_GetObjectItemCaseSensitive(plane, "sprite-y");
	cJSON* sprite_width = cJSON_GetObjectItemCaseSensitive(plane, "sprite-width");
	cJSON* sprite_height = cJSON_GetObjectItemCaseSensitive(plane, "sprite-height");
	cJSON* sprite_count = cJSON_GetObjectItemCaseSensitive(plane, "sprite-count");
	cJSON* sprite_speed = cJSON_GetObjectItemCaseSensitive(plane, "sprite-speed");

	cJSON* text = cJSON_GetObjectItemCaseSensitive(plane, "text");

	if (cJSON_IsBool(enabled) && cJSON_IsFalse(enabled)) {
		return NULL;
	}

	if (!cJSON_IsString(type)) {
		LOG("error: invalid plane type\n");
		return NULL;
	}

	if (!strcmp("primary", type->valuestring)) {
		const char* filename = 0;
		const char* filename_raw = 0;
		uint32_t colors[2] = {0};
		uint32_t f = 0;
		int idx = 0;
		bool vgradient = false;
		int p = 0;
		int j;

		if (cJSON_IsString(format)) {
			f = kms_format_val(format->valuestring);
			if (!f)
				LOG("error: format unknown\n");
		}

		if (cJSON_IsNumber(index))
			idx = index->valueint;

		data = plane_create(device, DRM_PLANE_TYPE_PRIMARY, idx,
				    eval_expr(width, device,
					      device->screens[0]->width),
				    eval_expr(height, device,
					      device->screens[0]->height),
				    f);
		if (!data) {
			LOG("error: failed to create plane\n");
			return NULL;
		}

		if (cJSON_IsString(name))
			strncpy(data->name, name->valuestring, sizeof(data->name)-1);

		plane_set_pos(data,
			      eval_expr(x, device, 0),
			      eval_expr(y, device, 0));

		if (cJSON_IsString(image))
			filename = reldir(config_file, image->valuestring);
		if (cJSON_IsString(image_raw))
			filename_raw = reldir(config_file, image_raw->valuestring);
		if (cJSON_IsString(pattern1))
			colors[0] = strtoul(pattern1->valuestring, NULL, 0);
		if (cJSON_IsString(pattern2))
			colors[1] = strtoul(pattern2->valuestring, NULL, 0);
		else
			colors[1] = colors[0];
		if (cJSON_IsString(vgradient1) && cJSON_IsString(vgradient2)) {
			colors[0] = strtoul(vgradient1->valuestring, NULL, 0);
			colors[1] = strtoul(vgradient2->valuestring, NULL, 0);
			vgradient = true;
		}

		if (cJSON_IsArray(text)) {
			for (j = 0; j < cJSON_GetArraySize(text);j++) {
				cJSON* t = cJSON_GetArrayItem(text, j);
				add_text_entry(data, t, device);
			}
		} else if (cJSON_IsObject(text)) {
			add_text_entry(data, text, device);
		}

		if (cJSON_IsTrue(patch))
			p = 1;

		if (cJSON_IsNumber(rotate))
			plane_set_rotate(data, rotate->valueint);

		configure_plane(data, colors, vgradient, p, filename, filename_raw);
	}
	else if (!strcmp("overlay", type->valuestring)) {
		const char* filename = 0;
		const char* filename_raw = 0;
		uint32_t colors[2] = {0};
		uint32_t f = 0;
		int j;
		unsigned int k;
		int idx = 0;
		bool vgradient = false;
		int p = 0;

		if (cJSON_IsString(format)) {
			f = kms_format_val(format->valuestring);
			if (!f)
				LOG("error: format unknown\n");
		}

		if (cJSON_IsNumber(index))
			idx = index->valueint;

		data = plane_create(device,
				    DRM_PLANE_TYPE_OVERLAY, idx,
				    eval_expr(width, device,
					      device->screens[0]->width),
				    eval_expr(height, device,
					      device->screens[0]->height),
				    f);
		if (!data) {
			LOG("error: failed to create plane\n");
			return NULL;
		}

		if (cJSON_IsString(name))
			strncpy(data->name, name->valuestring, sizeof(data->name)-1);

		plane_set_pos(data, eval_expr(x, device, 0),
			      eval_expr(y, device, 0));

		data->move.xspeed = eval_expr(move_xspeed, device, 0);
		data->move.yspeed = eval_expr(move_yspeed, device, 0);
		data->pan.yspeed = eval_expr(pan_yspeed, device, 0);
		data->pan.xspeed = eval_expr(pan_xspeed, device, 0);
		data->move.xmin = eval_expr(move_xmin, device, 0);
		data->move.xmax = eval_expr(move_xmax,
					    device,
					    device->screens[0]->width);
		data->move.ymin = eval_expr(move_ymin, device, 0);
		data->move.ymax = eval_expr(move_ymax,
					    device,
					    device->screens[0]->height);
		if (cJSON_IsNumber(scale))
			plane_set_scale(data, scale->valuedouble);

		if (cJSON_IsNumber(rotate))
			plane_set_rotate(data, rotate->valueint);
		if (cJSON_IsNumber(scaler_min))
			data->scaler.min = scaler_min->valuedouble;
		if (cJSON_IsNumber(scaler_max))
			data->scaler.max = scaler_max->valuedouble;
		if (cJSON_IsNumber(scaler_speed))
			data->scaler.speed = scaler_speed->valuedouble;

		if (cJSON_IsString(image))
			filename = reldir(config_file, image->valuestring);
		if (cJSON_IsString(image_raw))
			filename_raw = reldir(config_file, image_raw->valuestring);
		if (cJSON_IsString(pattern1))
			colors[0] = strtoul(pattern1->valuestring, NULL, 0);
		if (cJSON_IsString(pattern2))
			colors[1] = strtoul(pattern2->valuestring, NULL, 0);
		else
			colors[1] = colors[0];
		if (cJSON_IsString(vgradient1) && cJSON_IsString(vgradient2)) {
			colors[0] = strtoul(vgradient1->valuestring, NULL, 0);
			colors[1] = strtoul(vgradient2->valuestring, NULL, 0);
			vgradient = true;
		}

		if (cJSON_IsTrue(patch))
			p = 1;

		plane_set_alpha(data, eval_expr(alpha, device, 255));

		plane_set_pan_pos(data,
				  eval_expr(pan_x,
					    device, 0),
				  eval_expr(pan_y,
					    device, 0));

		plane_set_pan_size(data,
				   eval_expr(pan_width,
					     device, 0),
				   eval_expr(pan_height,
					     device, 0));

		data->sprite.x = eval_expr(sprite_x,
					   device, 0);
		data->sprite.y = eval_expr(sprite_y,
					   device, 0);
		data->sprite.width = eval_expr(sprite_width,
					       device, 0);
		data->sprite.height = eval_expr(sprite_height,
						device, 0);

		// if using sprite, initialize pan values
		if (data->sprite.width && data->sprite.height) {
			plane_set_pan_pos(data, data->sprite.x, data->sprite.y);
			plane_set_pan_size(data, data->sprite.width,
					   data->sprite.height);
		}

		if (cJSON_IsNumber(sprite_count))
			data->sprite.count = sprite_count->valueint;

		if (cJSON_IsNumber(sprite_speed))
			data->sprite.speed = sprite_speed->valueint;
		else
			data->sprite.speed = 1;

		for (j = 0; j < cJSON_GetArraySize(move_type);j++) {
			cJSON* mt = cJSON_GetArrayItem(move_type, j);

			if (cJSON_IsString(mt)) {
				for (k = 0; k < ARRAY_SIZE(move_map); k++) {
					if (!strcmp(move_map[k].s, mt->valuestring))
						data->move_flags |= move_map[k].v;
				}
			}
		}

		for (j = 0; j < cJSON_GetArraySize(transformarray);j++) {
			cJSON* transform = cJSON_GetArrayItem(transformarray, j);

			if (cJSON_IsString(transform)) {
				for (k = 0; k < ARRAY_SIZE(transform_map); k++) {
					if (!strcmp(transform_map[k].s, transform->valuestring))
						data->transform_flags |= transform_map[k].v;
				}
			}
		}

		if (cJSON_IsArray(text)) {
			for (j = 0; j < cJSON_GetArraySize(text);j++) {
				cJSON* t = cJSON_GetArrayItem(text, j);
				add_text_entry(data, t, device);
			}
		} else if (cJSON_IsObject(text)) {
			add_text_entry(data, text, device);
		}

		configure_plane(data, colors, vgradient, p, filename, filename_raw);
	}
	else {
		LOG("error: unknown plane type\n");
	}

	return data;
}

int engine_load_config(const char* config_file, struct kms_device* device,
		       struct plane_data** planes, uint32_t num_planes,
		       uint32_t* framedelay)
{
	cJSON* root = load_config(config_file);
	if (root) {
		int i;
		int itarget = 0;
		cJSON* planesarray = cJSON_GetObjectItemCaseSensitive(root, "planes");
		cJSON* delay = cJSON_GetObjectItemCaseSensitive(root, "framedelay");
		if (cJSON_IsNumber(delay))
			*framedelay = delay->valueint;

		for (i = 0; i < cJSON_GetArraySize(planesarray) && itarget < (int)num_planes;i++) {
			cJSON* plane = cJSON_GetArrayItem(planesarray, i);
			struct plane_data* p = parse_plane(config_file, device, plane);
			if (p)
				planes[itarget++] = p;
		}

		for (i = 0; i < (int)num_planes;i++) {
			if (planes[i])
				plane_apply(planes[i]);
		}

		cJSON_Delete(root);
	} else {
		return -1;
	}

	return 0;
}

static int mssleep(uint32_t ms)
{
	struct timespec req, rem;
	req.tv_sec = (time_t)(ms / 1000);
	req.tv_nsec = (ms - ((long)req.tv_sec * 1000L)) * 1000000L;
	return nanosleep(&req , &rem);
}

void engine_run_once(struct kms_device* device, struct plane_data** planes,
		     uint32_t num_planes, uint32_t framedelay)
{
	unsigned int i;
	for (i = 0; i < num_planes;i++) {
		bool trigger = false;
		bool move = false;

		if (!planes[i] || !planes[i]->plane || !planes[i]->fb ||
		    planes[i]->plane->type != DRM_PLANE_TYPE_OVERLAY)
			continue;

		if (planes[i]->move_flags & MOVE_X_WARP) {
			if (planes[i]->sprite.count){
				if ((planes[i]->x + planes[i]->sprite.width >=
				     (int)device->screens[0]->width + planes[i]->sprite.width)) {
					planes[i]->x = planes[i]->sprite.width * -1;
					trigger = true;
				}
				else if (planes[i]->x+planes[i]->move.xspeed <= (-1 * planes[i]->sprite.width)) {
					planes[i]->x = device->screens[0]->width +
						planes[i]->sprite.width;
					trigger = true;
				}
			} else {
				if ((planes[i]->x + plane_width(planes[i]) >
				     device->screens[0]->width + plane_width(planes[i]))) {
					planes[i]->x = planes[i]->fb->width * -1;
					trigger = true;
				}
			}
			planes[i]->x += planes[i]->move.xspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_X_BOUNCE) {
			if (planes[i]->sprite.count){
				if ((planes[i]->x + planes[i]->sprite.width > (int)device->screens[0]->width ||
				     (planes[i]->x < 0))) {
					planes[i]->move.xspeed *= -1;
					trigger = true;
				}
			} else {
				if ((planes[i]->x + plane_width(planes[i]) > device->screens[0]->width ||
				     (planes[i]->x < 0))) {
					planes[i]->move.xspeed *= -1;
					trigger = true;
				}
			}
			planes[i]->x += planes[i]->move.xspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_X_BOUNCE_CUSTOM) {
			if (planes[i]->x > planes[i]->move.xmax ||
			    planes[i]->x < planes[i]->move.xmin) {
				planes[i]->move.xspeed *= -1;
				trigger = true;
			}
			planes[i]->x += planes[i]->move.xspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_X_BOUNCE_OUT) {
			if (planes[i]->sprite.count) {
				if ((planes[i]->x+planes[i]->move.xspeed > (int)device->screens[0]->width) ||
				    (planes[i]->x+planes[i]->move.xspeed < ((int)planes[i]->sprite.width * -1))) {
					planes[i]->move.xspeed *= -1;
					trigger = true;
				}
			} else {
				if ((planes[i]->x+planes[i]->move.xspeed > (int)device->screens[0]->width) ||
				    (planes[i]->x+planes[i]->move.xspeed < ((int)plane_width(planes[i]) * -1))) {
					planes[i]->move.xspeed *= -1;
					trigger = true;
				}
			}
			planes[i]->x += planes[i]->move.xspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_Y_WARP) {
			if ((planes[i]->y + planes[i]->fb->height >
			     device->screens[0]->height + planes[i]->fb->height + planes[i]->fb->height)) {
				planes[i]->y = planes[i]->fb->height * -1;
				trigger = true;
			}
			planes[i]->y += planes[i]->move.yspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_Y_BOUNCE) {
			if (planes[i]->y + planes[i]->fb->height > device->screens[0]->height ||
			    planes[i]->y < 0) {
				planes[i]->move.yspeed *= -1;
				trigger = true;
			}
			planes[i]->y += planes[i]->move.yspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_Y_BOUNCE_CUSTOM) {
			if (planes[i]->y > planes[i]->move.ymax ||
			    planes[i]->y < planes[i]->move.ymin) {
				planes[i]->move.yspeed *= -1;
				trigger = true;
			}
			planes[i]->y += planes[i]->move.yspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_Y_BOUNCE_OUT) {
			if ((planes[i]->y+planes[i]->move.yspeed > (int)device->screens[0]->height) ||
			    (planes[i]->y+planes[i]->move.yspeed < ((int)planes[i]->fb->height * -1))) {
				planes[i]->move.yspeed *= -1;
				trigger = true;
			}
			planes[i]->y += planes[i]->move.yspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_PANX_BOUNCE) {
			if ((planes[i]->pan.x + planes[i]->pan.width > (int)plane_width(planes[i]) ||
			     (planes[i]->pan.x < 0))) {
				planes[i]->pan.xspeed *= -1;
				trigger = true;
			}
			planes[i]->pan.x += planes[i]->pan.xspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_PANY_BOUNCE) {
			if ((planes[i]->pan.y + planes[i]->pan.height > (int)plane_height(planes[i]) ||
			     (planes[i]->pan.y < 0))) {
				planes[i]->pan.yspeed *= -1;
				trigger = true;
			}
			planes[i]->pan.y += planes[i]->pan.yspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_PANX_WARP) {
			if ((planes[i]->pan.x + planes[i]->pan.width >= (int)plane_width(planes[i]) ||
			     (planes[i]->pan.x < 0))) {

				if (planes[i]->pan.xspeed >= 0)
					planes[i]->pan.x = 0;
				else
					planes[i]->pan.x = plane_width(planes[i]) - planes[i]->pan.width;
				trigger = true;
			}

			planes[i]->pan.x += planes[i]->pan.xspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_PANY_WARP) {
			if ((planes[i]->pan.y + planes[i]->pan.height >= (int)plane_height(planes[i]) ||
			     (planes[i]->pan.y < 0))) {

				if (planes[i]->pan.yspeed >= 0)
					planes[i]->pan.y = 0;
				else
					planes[i]->pan.y = plane_height(planes[i]) - planes[i]->pan.height;
				trigger = true;
			}

			planes[i]->pan.y += planes[i]->pan.yspeed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_SCALER) {
			if (planes[i]->scale >= planes[i]->scaler.max ||
			    planes[i]->scale <= planes[i]->scaler.min) {
				planes[i]->scaler.speed *= -1.0;
			}

			planes[i]->scale += planes[i]->scaler.speed;
			move = true;
		}

		if (planes[i]->move_flags & MOVE_SPRITE) {
			if (++planes[i]->sprite.runningspeed == planes[i]->sprite.speed)
			{
				planes[i]->sprite.runningspeed = 0;

				if (++planes[i]->sprite.index >= planes[i]->sprite.count) {
					planes[i]->sprite.index = 0;
				}

				planes[i]->pan.x = planes[i]->sprite.x + (planes[i]->sprite.index * planes[i]->sprite.width);
				planes[i]->pan.y = planes[i]->sprite.y;

				// support sheets that have frames on multiple rows
				if (planes[i]->pan.x + planes[i]->sprite.width >= (int)planes[i]->fb->width) {
					int x = planes[i]->sprite.x + (planes[i]->sprite.index * planes[i]->sprite.width);

					planes[i]->pan.x = (x % planes[i]->fb->width);
					planes[i]->pan.y = ((x / planes[i]->fb->width) * planes[i]->sprite.y) +

						(x / planes[i]->fb->width) * planes[i]->sprite.height;
				}

				planes[i]->pan.width = planes[i]->sprite.width;
				planes[i]->pan.height = planes[i]->sprite.height;
				move = true;
			}
		}

		if (trigger) {
			if (planes[i]->transform_flags & TRANSFORM_FLIP_HORIZONTAL) {
				flip_fb_horizontal(planes[i]->fb);
			}
			if (planes[i]->transform_flags & TRANSFORM_FLIP_VERTICAL) {
				flip_fb_vertical(planes[i]->fb);
			}

			if (planes[i]->transform_flags & TRANSFORM_ROTATE_CLOCKWISE) {
				planes[i]->rotate_degrees += 90;
				if (planes[i]->rotate_degrees > 270)
					planes[i]->rotate_degrees = 0;
				if (plane_set_rotate(planes[i], planes[i]->rotate_degrees)) {
					LOG("error: failed to set plane rotate\n");
				}

			}
			else if (planes[i]->transform_flags & TRANSFORM_ROTATE_CCLOCKWISE) {
				planes[i]->rotate_degrees -= 90;
				if (planes[i]->rotate_degrees < 0)
					planes[i]->rotate_degrees = 270;
				if (plane_set_rotate(planes[i], planes[i]->rotate_degrees)) {
					LOG("error: failed to set plane rotate\n");
				}
			}
		}

		if (move) {
			plane_apply(planes[i]);
		}
	}

	mssleep(framedelay);
}

void engine_run(struct kms_device* device, struct plane_data** planes,
		uint32_t num_planes, uint32_t framedelay, uint32_t max_frames)
{
	uint32_t frame_count = 0;

	while (1) {
		engine_run_once(device, planes, num_planes, framedelay);

		if (max_frames && ++frame_count >= max_frames)
			break;
	}
}

void parse_color(uint32_t in, struct rgba_color* color)
{
	color->r = (double)((in >> 24) & 0xff) / 255.;
	color->g = (double)((in >> 16) & 0xff) / 255.;
	color->b = (double)((in >> 8) & 0xff) / 255.;
	color->a = (double)(in & 0xff) / 255.;
}
