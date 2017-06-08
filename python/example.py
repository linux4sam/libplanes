#!/usr/bin/python

from planes import *
import os

fd = os.open("/dev/dri/card0", os.O_RDWR)
device = kms_device_open(fd);
plane = plane_create(device, 0, 0, 800, 480, 0)
plane_apply(plane)

render_fb_checker_pattern(plane.fb, 0x000000ff, 0xffffffff)

raw_input("Press Enter to continue...")
