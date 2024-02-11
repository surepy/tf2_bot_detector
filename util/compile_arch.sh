#!/bin/bash

# tries to compile in archlinux, i guess.

# deps
sudo pacman -Sy cmake make git gcc curl zip unzip tar cmake ninja
sudo yay -Sy discord-game-sdk

# dumb script
git clone https://github.com/surepy/tf2_bot_detector.git tf2_bot_detector --recurse-submodules

mkdir build/x64-release
cd build/x64-release

cmake -DCMAKE_BUILD_TYPE=Release -DDISCORD_GAME_SDK=/usr/lib/discord_game_sdk.so -DDISCORD_CPP_GAME_SDK=/usr/lib/discord_game_sdk.so ../../
cmake --build . --config Release
