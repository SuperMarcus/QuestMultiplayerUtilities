#!/bin/bash

rm -f QuestMultiplayerUtilities.zip
zip -j QuestMultiplayerUtilities.zip bmbfmod.json cover.gif
find ./libs/arm64-v8a ! -name 'libmodloader.so' -type f -exec zip -j QuestMultiplayerUtilities.zip {} +
