#!/usr/bin/bash

if [ ! -d cmake-build ]; then
    mkdir cmake-build
fi

(
    cd cmake-build || exit 1
    cmake ..
    make -j $(nproc)
)

WAYLAND_DISPLAY=wayland-99 cmake-build/compositor --startup-app xfce4-terminal >> log.txt &
