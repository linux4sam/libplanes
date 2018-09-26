#!/usr/bin/env python

from mpio import *
import glob
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
    else:
	configs = glob.glob("*.config")
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
