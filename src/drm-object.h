#ifndef PLANES_DRM_OBJECT_H
#define PLANES_DRM_OBJECT_H

#include <xf86drmMode.h>

#ifdef __cplusplus
extern "C" {
#endif

struct drm_object {
	drmModeObjectProperties *props;
	drmModePropertyRes **props_info;
	uint32_t id;
};

int drm_obj_get_properties(int fd, struct drm_object *obj, uint32_t type);

int drm_obj_set_property(drmModeAtomicReq *req, struct drm_object *obj, const char *name, uint64_t value);

void drm_obj_free(struct drm_object *obj);

#ifdef __cplusplus
}
#endif

#endif
