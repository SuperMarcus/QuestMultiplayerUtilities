#include <cstddef>
#include <string>

#include "multiplayer.hpp"
#include "private.hpp"
#include "SongManager.hpp"
#include "PlayerSessionManager.hpp"

#include "utils/typedefs.h"
#include "utils/utils.h"
#include "utils/il2cpp-utils.hpp"

#include "GlobalNamespace/MultiplayerLevelLoader.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/MultiplayerModeSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/BeatmapIdentifierNetSerializable.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/LobbyPlayerDataModel.hpp"

using namespace GlobalNamespace;
using namespace std;
using namespace QuestMultiplayer;

MAKE_HOOK_OFFSETLESS(MultiplayerLevelLoader_LoadLevel, void, MultiplayerLevelLoader* self, BeatmapIdentifierNetSerializable* beatmapId, GameplayModifiers* gameModifiers, float initialStartTime) {
    static bool isAttemptingDownload = false;
    static BeatmapIdentifierNetSerializable* latestSelectedBeatmapId;
    static GameplayModifiers* latestSelectedGameModifiers;
    static float latestSelectedInitialStartTime;

    // Save values
    latestSelectedBeatmapId = beatmapId;
    latestSelectedGameModifiers = gameModifiers;
    latestSelectedInitialStartTime = initialStartTime;

    auto beatmapIdString = to_utf8(csstrtostr(beatmapId->levelID));
    getLogger().info("Loading level %s", beatmapIdString.data());

    auto level = SongManager::sharedInstance().getLevelPreviewByID(beatmapId->levelID);

    if (level) {
        auto casted = reinterpret_cast<Il2CppObject*>(level);
        getLogger().info("Level type is %s", casted->klass->name);
    }

    if (!level && !isAttemptingDownload) {
        getLogger().info("Level pack not found, downloading from BeatSaver!!");
        auto levelHash = SongManager::getLevelHashFromID(beatmapIdString);

        if (SongManager::downloadLevelByHash(levelHash, [&](auto levelPath){
            SongManager::sharedInstance().updateSongs(); // Actually loads the song
            getLogger().info(
                "Custom level downloaded and loaded. Calling LoadLevel with parameters: self=%#08x, beatmapId=%#08x, modifiers=%#08x, initStartTime=%f...",
                self, latestSelectedBeatmapId, latestSelectedGameModifiers, latestSelectedInitialStartTime
            );
            self->LoadLevel(latestSelectedBeatmapId, latestSelectedGameModifiers, latestSelectedInitialStartTime);
        })) {
            getLogger().info("Downloading level...");
            isAttemptingDownload = true;
            return;
        } else {
            getLogger().warning("Download task aborted: level is already being downloaded.");
        }
    }
    isAttemptingDownload = false;

    MultiplayerLevelLoader_LoadLevel(self, beatmapId, gameModifiers, initialStartTime);
}

MAKE_HOOK_OFFSETLESS(MultiplayerModeSelectionFlowCoordinator_DidActivate, void, MultiplayerModeSelectionFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MultiplayerModeSelectionFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if (firstActivation) {
        // Prepare song loader
        SongManager::sharedInstance().updateSongs();
    }
}

MAKE_HOOK_OFFSETLESS(LobbyPlayersDataModel_Activate, void, LobbyPlayersDataModel* self) {
    LobbyPlayersDataModel_Activate(self);
    PlayerSessionManager::sharedInstance().updatePlayerStates();
}

MAKE_HOOK_OFFSETLESS(LobbyPlayersDataModel_HandleMenuRpcManagerSelectedBeatmap, void, LobbyPlayersDataModel* self, Il2CppString* userId, BeatmapIdentifierNetSerializable* beatmapId) {
    LobbyPlayersDataModel_HandleMenuRpcManagerSelectedBeatmap(self, userId, beatmapId);
    PlayerSessionManager::sharedInstance().onRpcSelectedBeatmap(self, userId, beatmapId);
}

void InstallHooks() {
    auto& logger = getLogger();
    logger.info("Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(logger, MultiplayerLevelLoader_LoadLevel, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLevelLoader", "LoadLevel", 3));
    INSTALL_HOOK_OFFSETLESS(logger, MultiplayerModeSelectionFlowCoordinator_DidActivate, il2cpp_utils::FindMethodUnsafe("", "MultiplayerModeSelectionFlowCoordinator", "DidActivate", 3));
    INSTALL_HOOK_OFFSETLESS(logger, LobbyPlayersDataModel_Activate, il2cpp_utils::FindMethod("", "LobbyPlayersDataModel", "Activate"));
    INSTALL_HOOK_OFFSETLESS(logger, LobbyPlayersDataModel_HandleMenuRpcManagerSelectedBeatmap, il2cpp_utils::FindMethodUnsafe("", "LobbyPlayersDataModel", "HandleMenuRpcManagerSelectedBeatmap", 2));
}
