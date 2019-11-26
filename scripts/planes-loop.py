#!/usr/bin/env python3

from mpio import *
import os
import signal
from threading import Thread
from subprocess import *

abort = False

os.chdir("/usr/share/planes/")

def run():
    global child_pid
    global abort
    if cpu() == 'at91sam9x5':
        configs = ["default.config", "alpha.config", "sprite.config",
                   "alpha2.config", "paper.config", "rotate.config"]
    elif cpu() == 'sama7d6':
        configs = ["alpha.config", "alpha2.config", "default.config",
                   "pan_2overlays.config", "paper.config", "parallax_2overlays.config",
                   "rotate.config", "scale_2overlays.config", "sprite.config",
                   "window_2overlays.config"]
    else:
        configs = ["alpha.config", "alpha2.config", "default.config",
                   "pan.config", "paper.config", "parallax.config",
                   "rotate.config", "scale.config", "sprite.config",
                   "window.config"]
    while not abort:
        for config in configs:
            proc = Popen(["planes", "-f", "500", "-c", config], close_fds=True)
            child_pid = proc.pid
            proc.wait()
            if abort:
                break;
            proc = Popen(["./splash.py"], close_fds=True)
            child_pid = proc.pid
            proc.wait()
            if abort:
                break;

def stop():
    global child_pid
    global abort
    abort = True
    if child_pid is None:
        pass
    else:
        os.kill(child_pid, signal.SIGTERM)

def handler(signum, frame):
    stop()

signal.signal(signal.SIGINT, handler)

thread = Thread(target=run)
thread.start()

device = Input('/dev/input/keyboard0')
while True:
    try:
        (tv_sec, tv_usec, evtype, code, value) = device.read()
    except OSError:
        stop()
        break;

    if evtype == Input.TYPE_EV_KEY and value == 1:
        if code == 11:
            stop()
            break

thread.join()
