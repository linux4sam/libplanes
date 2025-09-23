#include "drm-object.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xf86drmMode.h>

#include "common.h"

int drm_obj_get_properties(int fd, struct drm_object *obj, uint32_t type)
{
	obj->props = drmModeObjectGetProperties(fd, obj->id, type);
	if (!obj->props) {
		LOG("error: cannot get object properties (drm object: %p, type: %u)\n", obj, type);
		return -EINVAL;
	}

	obj->props_info = calloc(obj->props->count_props, sizeof(*obj->props_info));
	if (!obj->props_info) {
		drmModeFreeObjectProperties(obj->props);
		return -ENOMEM;
	}

	for (unsigned int i = 0; i < obj->props->count_props; i++)
		obj->props_info[i] = drmModeGetProperty(fd, obj->props->props[i]);

	return 0;
}

int drm_obj_set_property(drmModeAtomicReq *req, struct drm_object *obj, const char *name, uint64_t value)
{
	uint32_t prop_id = 0;
	int ret;

	for (unsigned int i = 0; i < obj->props->count_props; i++) {
		if (!strcmp(obj->props_info[i]->name, name)) {
			prop_id = obj->props_info[i]->prop_id;
			break;
		}
	}

	if (!prop_id) {
		LOG("error: %s property not found\n", name);
		return -ENOENT;
	}

	ret = drmModeAtomicAddProperty(req, obj->id, prop_id, value);
	if (ret < 0)
		return ret;
	else
		/*
		 * drmModeAtomicAddProperty returns an index, as we just want
		 * to know if the property has been set successfully, just
		 * returns zero so the return value can be used in a more
		 * usual way i.e. just testing for zero or not.
		 */
		return 0;
}

void drm_obj_free(struct drm_object *obj)
{
	if (!obj)
		return;

	if (obj->props_info) {
		for (unsigned int i = 0; i < obj->props->count_props; i++)
			drmModeFreeProperty(obj->props_info[i]);

		free(obj->props_info);
	}

	drmModeFreeObjectProperties(obj->props);
	free(obj);
}
