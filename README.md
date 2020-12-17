QuestMultiplayerUtilities (Alpha)
=================================

A set of in-game utilities for Quest BeatSaber multiplayer experience.

**Warning: This mod is still work in progress. Use at your own risk!**

## Features

- [x] Automatically download missing songs from BeatSaver;
- [ ] (WIP) ServerBrowser integration;
- [ ] (WIP) PC MultiplayerExtension compatibility (modded states, etc.).

## Requirements

This mod requires darknight1050's unreleased [SongLoader](https://github.com/darknight1050/SongLoader).

## Building

1. Run `qpm restore`;
2. Grab `libsongloader_0_1_0.so` from [SongLoader's repository](https://github.com/darknight1050/SongLoader/actions) and place it in `extern/`;
3. Run `./build.sh` if you're using Linux or macOS.

## Credits

* Libraries: [SongLoader](https://github.com/darknight1050/SongLoader), [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook), [codegen](https://github.com/sc2ad/BeatSaber-Quest-Codegen), [custom-types](https://github.com/sc2ad/Il2CppQuestTypePatching)
