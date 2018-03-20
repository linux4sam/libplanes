#!/usr/bin/env python

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

plane = plane_create(device, 0, 2,
                     device.get_screens(0).width,
                     device.get_screens(0).height,
                     kms_format_val("DRM_FORMAT_XRGB8888"))
render_fb_image(plane.fb, os.path.join(os.path.dirname(__file__), 'background5.png'))

msgs = [
    'This demo uses LCD planes for all animations.',
    'This is a Python script using libplanes.',
    'libplanes makes use of hardware LCD planes.',
    'Scale, rotate, alpha blend, crop, pan ...',
    'No CPU is required for many 2D effects.',
    'Awesome 3D parallax effects.',
    'Simple 2D sprite animation.',
    'No videos. This is all realtime animation.'
]

srandom = random.SystemRandom()
render_fb_text(plane.fb, int(phw(50)), int(phh(220)), srandom.choice(msgs), 0xffffffff, phw(30))

for i in range(50, 80, 1):
    plane_set_scale(plane, i / 100.0)
    plane_apply(plane)
    time.sleep(0.01)

for i in range(81, 91, 1):
    plane_set_scale(plane, i / 100.0)
    plane_apply(plane)
    time.sleep(0.03)

for i in range(91, 101, 1):
    plane_set_scale(plane, i / 100.0)
    plane_apply(plane)
    time.sleep(0.05)

time.sleep(5)
