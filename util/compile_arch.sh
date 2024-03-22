#!/bin/bash

# tries to compile in archlinux, i guess.

# deps
sudo pacman -Sy cmake make git gcc curl zip unzip tar cmake ninja onetbb

sudo yay -Sy discord-game-sdk
# it doesnt have the lib prefix so just make a symlink
sudo ln -s /usr/lib/discord_game_sdk.so /usr/lib/libdiscord_game_sdk.so

# dumb script
git clone https://github.com/surepy/tf2_bot_detector.git tf2_bot_detector --recurse-submodules

mkdir build/x64-release
cd build/x64-release

cmake -DCMAKE_BUILD_TYPE=Release -DDISCORD_GAME_SDK_INCLUDE=/usr/src/discord-game-sdk ../../
cmake --build . --config Release
