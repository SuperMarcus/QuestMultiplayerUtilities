#!/bin/bash

adb shell am force-stop com.beatgames.beatsaber

adb push libs/arm64-v8a/liboldsongloader_dangerous_dontuse.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/
adb push libs/arm64-v8a/libmultiplayer.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/

# Libs
#adb push libs/arm64-v8a/libquestui.so /sdcard/Android/data/com.beatgames.beatsaber/files/libs/
#adb push libs/arm64-v8a/libcustom-types.so /sdcard/Android/data/com.beatgames.beatsaber/files/libs/
#adb push libs/arm64-v8a/libcodegen*.so /sdcard/Android/data/com.beatgames.beatsaber/files/libs/
#adb push libs/arm64-v8a/libbeatsaber-hook*.so /sdcard/Android/data/com.beatgames.beatsaber/files/libs/
