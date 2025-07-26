#!/usr/bin/bash

if [ ! -d cmake-build ]; then
    mkdir cmake-build
fi

if ! (
    cd cmake-build || exit 1
    cmake .. || exit 1
    make -j"$(nproc)" || exit 1
); then
    echo "Failed to build compositor"
    exit 1
fi

WAYLAND_DISPLAY=wayland-99 cmake-build/compositor --startup-app xfce4-terminal
