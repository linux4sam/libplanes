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
	%pythoncode %{
		def get_screens(self, index):
			return _planes._kms_device_get_screen(self, index)
	%}
}
