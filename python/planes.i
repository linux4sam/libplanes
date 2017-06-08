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
