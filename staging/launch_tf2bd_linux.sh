#!/bin/sh
# very simple script to load stuff like libmh-stuff.so
# SDL_VIDEODRIVER="x11" as wayland just... crashes lol
SDL_VIDEODRIVER="x11" LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./ ./tf2_bot_detector