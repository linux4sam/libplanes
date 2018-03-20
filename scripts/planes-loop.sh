#!/bin/sh

eval "$DEMO_LEAVE"

cd $(dirname $0)
./planes-loop.py

sleep 1

eval "$DEMO_ENTER"
