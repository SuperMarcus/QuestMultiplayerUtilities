#pragma once

#include <map>

#include "GlobalNamespace/IMultiplayerSessionManager.hpp"
#include "GlobalNamespace/ConnectionFailedReason.hpp"
#include "GlobalNamespace/IConnectedPlayer.hpp"
#include "GlobalNamespace/DisconnectedReason.hpp"
#include "GlobalNamespace/BeatmapIdentifierNetSerializable.hpp"
#include "GlobalNamespace/LobbyPlayersDataModel.hpp"

namespace QuestMultiplayer {
    class PlayerSessionManager {
    public:
        static PlayerSessionManager& sharedInstance();

        void updatePlayerStates();
        GlobalNamespace::IMultiplayerSessionManager* getSessionManager();

    private:
        GlobalNamespace::IMultiplayerSessionManager* _sessionManager {};
        std::map<std::string, std::string> _playerSelectedLevelIDs;

        bool isEnforcingMods = false;
        bool didRegisterEvents = false;

        static void onConnectedEvent(Il2CppObject*);
        static void onConnectionFailedEvent(Il2CppObject*, GlobalNamespace::ConnectionFailedReason);
        static void onPlayerConnectedEvent(Il2CppObject*, GlobalNamespace::IConnectedPlayer*);
        static void onPlayerDisconnectedEvent(Il2CppObject*, GlobalNamespace::IConnectedPlayer*);
        static void onPlayerStateChangedEvent(Il2CppObject*, GlobalNamespace::IConnectedPlayer*);
        static void onDisconnectedEvent(Il2CppObject*, GlobalNamespace::DisconnectedReason);

    public:
        void onRpcSelectedBeatmap(GlobalNamespace::LobbyPlayersDataModel* model, Il2CppString* playerId, GlobalNamespace::BeatmapIdentifierNetSerializable* beatmapId);
        void onBeatmapDownloadCompletion(GlobalNamespace::LobbyPlayersDataModel* model, Il2CppString* playerId, GlobalNamespace::BeatmapIdentifierNetSerializable* beatmapId, std::optional<std::string> downloadResult);
    };
}
