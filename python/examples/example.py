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

from mpio import cpu
import planes
import os
import sys

if sys.version_info[0] == 3:
    raw_input = input

# open the DRI device
device = planes.kms_device_open(os.open("/dev/dri/card0", os.O_RDWR));

# create the primary plane
primary = planes.plane_create(device, 1, 0,
                              device.get_screens(0).width,
                              device.get_screens(0).height, 0)

bs = 16

match cpu():
    case 'at91sam9x5':
        nplanes = 1
    case 'sama7d6':
        nplanes = 2
    case _:
        nplanes = 3

# create each of the overlays
overlays = [planes.plane_create(device, 0, i, bs*3, bs*3, 0) for i in range(nplanes)]

# render the primary plane
planes.render_fb_checker_pattern(primary.get_fb(0), 0x000000ff, 0xffffffff)
planes.plane_apply(primary)

# position each of the overlays and render a color
colors = [0xff0000aa, 0x00ff00aa, 0x0000ffaa]
for i, overlay in enumerate(overlays):
    planes.render_fb_checker_pattern(overlay.get_fb(0), colors[i], colors[i])
    planes.plane_set_pos(overlay, bs*3*i, bs*3*i)
    planes.plane_apply(overlay)

raw_input("Press Enter to continue...")
