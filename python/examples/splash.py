#!/usr/bin/env python3

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

from mpio import *
import planes
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
device = planes.kms_device_open(fd);

match cpu():
    case 'at91sam9x5':
        nplanes = 1
    case 'sama7d6':
        nplanes = 2
    case _:
        nplanes = 3

plane = planes.plane_create(device, 0, nplanes - 1,
                            device.get_screens(0).width,
                            device.get_screens(0).height,
                            planes.kms_format_val("DRM_FORMAT_XRGB8888"))
planes.render_fb_image(plane.get_fb(0),
                       os.path.join(os.path.dirname(__file__), 'background5.png'))

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
planes.render_fb_text(plane.get_fb(0), int(phw(50)), int(phh(220)),
                      srandom.choice(msgs), 0xffffffff, phw(30))

x = 1.0
s = 50.0
while True:
    s = s + (1.0 * x)
    if s > 100.0:
        s = 100.0
    planes.plane_set_scale(plane, s / 100.0)
    planes.plane_apply(plane)
    if s >= 100.0:
        break
    time.sleep(0.01)
    x = x - 0.01

time.sleep(5)
