#include <cstddef>
#include <string>

#include "multiplayer.hpp"
#include "private.hpp"
#include "SongManager.hpp"

#include "utils/typedefs.h"
#include "utils/utils.h"
#include "utils/il2cpp-utils.hpp"

#include "GlobalNamespace/MultiplayerLevelLoader.hpp"
#include "GlobalNamespace/MultiplayerModeSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/BeatmapIdentifierNetSerializable.hpp"
#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"

using namespace GlobalNamespace;
using namespace std;
using namespace QuestMultiplayer;

MAKE_HOOK_OFFSETLESS(MultiplayerLevelLoader_LoadLevel, void, MultiplayerLevelLoader* self, BeatmapIdentifierNetSerializable* beatmapId, GameplayModifiers* gameModifiers, float initialStartTime) {
    static bool isAttemptingDownload = false;
    auto beatmapIdString = to_utf8(csstrtostr(beatmapId->levelID));
    getLogger().info("Loading level %s", beatmapIdString.data());

    auto level = SongManager::sharedInstance().getLevelPackByID(beatmapId->levelID);

    if (level) {
        auto casted = reinterpret_cast<Il2CppObject*>(level);
        getLogger().info("Level type is %s", casted->klass->name);
    }

    if (!level && !isAttemptingDownload) {
//    if (level && !isAttemptingDownload) { // TODO: remove test clause
        getLogger().info("Level pack not found, downloading from BeatSaver!!");
        auto levelHash = SongManager::getLevelHashFromID(beatmapIdString);
        isAttemptingDownload = true;
        SongManager::downloadLevelByHash(levelHash, [=](){
            getLogger().info("Returning function, self=%#08x, beatmapId=%#08x, modifiers=%#08x, initStartTime=%f...", self, beatmapId, gameModifiers, initialStartTime);
            self->LoadLevel(beatmapId, gameModifiers, initialStartTime);
        });
        return;
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

void InstallHooks() {
    getLogger().info("Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(MultiplayerLevelLoader_LoadLevel, il2cpp_utils::FindMethodUnsafe("", "MultiplayerLevelLoader", "LoadLevel", 3));
    INSTALL_HOOK_OFFSETLESS(MultiplayerModeSelectionFlowCoordinator_DidActivate, il2cpp_utils::FindMethodUnsafe("", "MultiplayerModeSelectionFlowCoordinator", "DidActivate", 3));
}
