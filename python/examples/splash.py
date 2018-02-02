#!/usr/bin/python

# Copyright (C) 2017 Microchip Technology Inc.  All rights reserved.
#   Joshua Henderson <joshua.henderson@microchip.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

from planes import *
import os
import random
import time

def phw(width):
    global device
    return width / 800.0 * device.get_screens(0).width

def phh(height):
    global device
    return height / 480.0 * device.get_screens(0).height

fd = os.open("/dev/dri/card0", os.O_RDWR)
device = kms_device_open(fd);

logo = plane_create(device, 0, 0,
                    int(phw(120.0)), int(phh(28.0)),
                    kms_format_val("DRM_FORMAT_ARGB8888"))
plane_set_pos(logo, int(phw(10.0)), int(phh(10.0)))
render_fb_image(logo.fb, "logo.png")

logo2 = plane_create(device, 0, 1,
                     int(phw(55.0)), int(phh(55.0)),
                     kms_format_val("DRM_FORMAT_ARGB8888"))
plane_set_pos(logo2,
              device.get_screens(0).width - int(phw(55.0)), 0)
render_fb_image(logo2.fb, "logo55.png")

plane = plane_create(device, 0, 2,
                     device.get_screens(0).width,
                     device.get_screens(0).height,
                     kms_format_val("DRM_FORMAT_XRGB8888"))
render_fb_text(plane.fb, int(phw(50)), int(phh(200)), "libplanes Demo", 0xFF0000FF, phw(40))

msgs = [
    'libplanes is where it\'s at.',
    'This uses libplanes for all animations.',
    'This is a Python script using libplanes.',
    'libplanes makes use of hardware LCD planes.',
    'Scale, rotate, alpha blend, crop, pan ...',
    'Amazin\' graphics with powerful frameworks.',
    'No CPU is required for many 2D effects.',
    'No CPU is required for many 2D effects.',
    'No CPU is required for many 2D effects.',
    'Awesome 3D parallax effects.',
    'libplanes with Qt for responsive interfaces.',
    'No videos. This is all realtime animation.',
    'See your Microchip representative today.'
]

srandom = random.SystemRandom()
render_fb_text(plane.fb, int(phw(50)), int(phh(300)), srandom.choice(msgs), 0xFFFFFFFF, phw(30))

plane_apply(logo)
plane_apply(logo2)

for i in range(1, 120, 1):
    plane_set_scale(plane, i / 100.0)
    plane_apply(plane)
    time.sleep(0.02)

for i in range(119, 90, -1):
    plane_set_scale(plane, i / 100.0)
    plane_apply(plane)
    time.sleep(0.03)

for i in range(91, 101, 1):
    plane_set_scale(plane, i / 100.0)
    plane_apply(plane)
    time.sleep(0.01)

time.sleep(5)
