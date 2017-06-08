#!/bin/sh

. /opt/ApplicationLauncher/demo.config

eval "$DEMO_LEAVE"

(
    cd /usr/share/planes/
    for f in *.config;
    do
	planes -f 1000 -c $f;
    done
) &
pid=$!

while true; do

    if ! evtest --query /dev/input/event0 EV_KEY 11; then
	echo "killing $pid ..."
	kill -9 $pid
	killall -9 planes
	break
    fi

    if ! kill -0 $pid > /dev/null 2>&1; then
	echo "pid $pid gone ..."
	break
    fi

    sleep 0.01
done

sleep 1

eval "$DEMO_ENTER"
