#include <string>
#include <utility>
#include <functional>

#include "utils/typedefs.h"
#include "private.hpp"
#include "PlayerSessionManager.hpp"
#include "SongManager.hpp"

#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/MultiplayerModeSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/MultiplayerSessionManager.hpp"

#include "UnityEngine/Resources.hpp"

#include "System/Action.hpp"
#include "System/Action_1.hpp"

using namespace QuestMultiplayer;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace il2cpp_utils;
using namespace System;

PlayerSessionManager &QuestMultiplayer::PlayerSessionManager::sharedInstance() {
    static PlayerSessionManager singleton = PlayerSessionManager();
    return singleton;
}

GlobalNamespace::IMultiplayerSessionManager *PlayerSessionManager::getSessionManager() {
    auto multManager = Resources::FindObjectsOfTypeAll<MultiplayerSessionManager*>()->values[0];
    return reinterpret_cast<IMultiplayerSessionManager *>(multManager);
//    if (!_sessionManager) {
//        auto multManager = Resources::FindObjectsOfTypeAll<MultiplayerSessionManager*>()->values[0];
//        _sessionManager = reinterpret_cast<IMultiplayerSessionManager *>(multManager);
//        getLogger().info("IMultiplayerSessionManager=%#08x", _sessionManager);
//    }
//
//    return Resources::FindObjectsOfTypeAll<MultiplayerSessionManager*>()->values[0];
}

void PlayerSessionManager::updatePlayerStates() {
    auto sessionManager = getSessionManager();
    sessionManager->SetLocalPlayerState(createcsstr("modded"), true);
    sessionManager->SetLocalPlayerState(createcsstr("customsongs"), true); // Can't disable custom songs
    sessionManager->SetLocalPlayerState(createcsstr("enforcemods"), isEnforcingMods);

    if (!didRegisterEvents) {
        getLogger().info("Registering multiplayer session events...");
        sessionManager->add_connectedEvent(MakeDelegate<Action*>(
            classof(Action*),
            (Il2CppObject*) nullptr,
            PlayerSessionManager::onConnectedEvent
        ));
        sessionManager->add_connectionFailedEvent(MakeDelegate<Action_1<ConnectionFailedReason>*>(
            classof(Action_1<ConnectionFailedReason>*),
            (Il2CppObject*) nullptr,
            PlayerSessionManager::onConnectionFailedEvent
        ));
        sessionManager->add_playerConnectedEvent(MakeDelegate<Action_1<IConnectedPlayer*>*>(
            classof(Action_1<IConnectedPlayer*>*),
            (Il2CppObject*) nullptr,
            PlayerSessionManager::onPlayerConnectedEvent
        ));
        sessionManager->add_playerDisconnectedEvent(MakeDelegate<Action_1<IConnectedPlayer*>*>(
            classof(Action_1<IConnectedPlayer*>*),
            (Il2CppObject*) nullptr,
            PlayerSessionManager::onPlayerDisconnectedEvent
        ));
        sessionManager->add_playerStateChangedEvent(MakeDelegate<Action_1<IConnectedPlayer*>*>(
            classof(Action_1<IConnectedPlayer*>*),
            (Il2CppObject*) nullptr,
            PlayerSessionManager::onPlayerStateChangedEvent
        ));
        sessionManager->add_disconnectedEvent(MakeDelegate<Action_1<DisconnectedReason>*>(
            classof(Action_1<DisconnectedReason>*),
            (Il2CppObject*) nullptr,
            PlayerSessionManager::onDisconnectedEvent
        ));
        didRegisterEvents = true;
    }
}

void PlayerSessionManager::onConnectedEvent(Il2CppObject*) {
    getLogger().info("Server connected.");
}

void PlayerSessionManager::onConnectionFailedEvent(Il2CppObject *, GlobalNamespace::ConnectionFailedReason reason) {
    getLogger().info("Connection failed for reason: %d", reason.value);
}

void PlayerSessionManager::onPlayerConnectedEvent(Il2CppObject *, GlobalNamespace::IConnectedPlayer* player) {
    if (!player) return;
    getLogger().info("Player connected.");
}

void PlayerSessionManager::onPlayerDisconnectedEvent(Il2CppObject *, GlobalNamespace::IConnectedPlayer* player) {
    if (!player) return;
    getLogger().info("Player disconnected.");
}

void PlayerSessionManager::onPlayerStateChangedEvent(Il2CppObject *, GlobalNamespace::IConnectedPlayer* player) {
    if (!player) return;
    getLogger().info("Player state changed.");
}

void PlayerSessionManager::onDisconnectedEvent(Il2CppObject *, DisconnectedReason reason) {
    getLogger().info("Server disconnected because of reason: %d", reason.value);
}

void PlayerSessionManager::onRpcSelectedBeatmap(GlobalNamespace::LobbyPlayersDataModel* model, Il2CppString *playerIdRaw, GlobalNamespace::BeatmapIdentifierNetSerializable *beatmapId) {
    auto selectedLevelID = to_string(beatmapId->get_levelID());
    auto playerID = to_string(playerIdRaw);

    if (selectedLevelID.empty()) {
        _playerSelectedLevelIDs.erase(playerID);
        return;
    }

    _playerSelectedLevelIDs[playerID] = selectedLevelID;
    auto levelPreview = SongManager::sharedInstance().getLevelPreviewByID(beatmapId->get_levelID());

    if (!levelPreview) {
        getLogger().info("Player %s selected a beatmap %s that wasn't loaded locally.", playerID.data(), selectedLevelID.data());
        if (SongManager::downloadLevelByID(selectedLevelID, std::bind(std::mem_fn(&PlayerSessionManager::onBeatmapDownloadCompletion), this, model, playerIdRaw, beatmapId, std::placeholders::_1))) {
            getLogger().info("Multiplayer Utilities will try to download this song...");
        } else {
            getLogger().info("Multiplayer Utilities weren't able to download this song because download may have started.");
        }
    }
}

void PlayerSessionManager::onBeatmapDownloadCompletion(GlobalNamespace::LobbyPlayersDataModel* model, Il2CppString *playerIdRaw, GlobalNamespace::BeatmapIdentifierNetSerializable *beatmapId, std::optional<std::string> downloadResult) {
    if (!downloadResult.has_value()) {
        getLogger().warning("Level download failed!");
        return;
    }

    auto selectedLevelID = to_string(beatmapId->get_levelID());
    auto playerID = to_string(playerIdRaw);
    auto& downloadedBeatmapPath = downloadResult.value();

    if (_playerSelectedLevelIDs[playerID] == selectedLevelID) {
        getLogger().info("Loading custom level\"%s\" for player selected beatmap...", selectedLevelID.data());
        auto previewBeatmap = SongManager::loadLevelFromPath(downloadedBeatmapPath);

        if (previewBeatmap) {
            auto characteristic = model->beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(beatmapId->get_beatmapCharacteristicSerializedName());
            model->SetPlayerBeatmapLevel(playerIdRaw, reinterpret_cast<IPreviewBeatmapLevel*>(previewBeatmap), beatmapId->get_difficulty(), characteristic);
        }
    } else {
        getLogger().info("Abort loading level \"%s\" because the player has selected a different beatmap.", selectedLevelID.data());
    }
}
