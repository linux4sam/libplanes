%module planes
%{
#include <planes/kms.h>
#include <planes/plane.h>
#include <planes/engine.h>
#include <planes/draw.h>
%}

typedef unsigned int uint32_t;

%include <planes/kms.h>
%include <planes/plane.h>
%include <planes/engine.h>
%include <planes/draw.h>

%inline %{
	struct kms_screen _kms_device_get_screen(struct kms_device *device, int index)
	{
		return *(device->screens[index]);
	}
%}

%extend kms_device {
	struct kms_screen get_screens(int index)
	{
		return *($self->screens[index]);
	}
}

%inline %{
	struct kms_framebuffer _plane_data_get_fb(struct plane_data *plane, int index)
	{
		return *(plane->fbs[index]);
	}
%}

%extend plane_data {
	struct kms_framebuffer get_fb(int index)
	{
		return *($self->fbs[index]);
	}
}
