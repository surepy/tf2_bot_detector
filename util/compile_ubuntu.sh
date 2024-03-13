#!/bin/bash

# tries to compile in archlinux, i guess.

# deps
sudo apt install --no-install-recommends libtbb-dev libgl-dev gcc-11 g++-11 libsdl2-dev build-essential git make \
              pkg-config cmake ninja-build libasound2-dev libpulse-dev \ 
              libaudio-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
              libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev \ 
              libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libglu1-mesa-dev libgles2-mesa-dev \ 
              libegl1-mesa-dev libdbus-1-dev libudev-dev \
              libpipewire-0.3-dev libwayland-dev libdecor-0-dev 

# dumb script
git clone https://github.com/surepy/tf2_bot_detector.git tf2_bot_detector --recurse-submodules

mkdir build/x64-release
cd build/x64-release

# fixme: DTF2BD_ENABLE_DISCORD_INTEGRATION
cmake -DCMAKE_BUILD_TYPE=Release -DTF2BD_ENABLE_DISCORD_INTEGRATION=false ../../
cmake --build . --config Release
