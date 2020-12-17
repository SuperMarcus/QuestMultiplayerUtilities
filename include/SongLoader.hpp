#pragma once

#include "utils/typedefs.h"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"

// Using the currently released SongLoader APIs

std::string GetCustomLevelHash(GlobalNamespace::StandardLevelInfoSaveData* level, std::string customLevelPath);
GlobalNamespace::StandardLevelInfoSaveData* LoadCustomLevelInfoSaveData(Il2CppString* customLevelPath);
GlobalNamespace::CustomPreviewBeatmapLevel* LoadCustomPreviewBeatmapLevelAsync(Il2CppString* customLevelPath, GlobalNamespace::StandardLevelInfoSaveData* standardLevelInfoSaveData);
