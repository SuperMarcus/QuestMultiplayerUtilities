#pragma once

#include <map>
#include "utils/typedefs.h"

#include "GlobalNamespace/BeatmapLevelsModel.hpp"
#include "GlobalNamespace/IBeatmapLevelPack.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/AlwaysOwnedContentContainerSO.hpp"

#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/UnityWebRequestAsyncOperation.hpp"

#include "System/Threading/CancellationTokenSource.hpp"
#include "System/Threading/Tasks/Task.hpp"

namespace QuestMultiplayer {
    class SongManager {
    public:
        SongManager();

        GlobalNamespace::BeatmapLevelsModel* getLevelsModel();
        GlobalNamespace::AlwaysOwnedContentContainerSO* getContentContainer();
        GlobalNamespace::IPreviewBeatmapLevel* getLevelPackByID(Il2CppString* id);
        GlobalNamespace::IPreviewBeatmapLevel* getLevelPackByID(const std::string& id);
        GlobalNamespace::CustomPreviewBeatmapLevel* loadLevelFromPath(std::string path);
        void updateSongs();

        static void downloadLevelByHash(const std::string& hash, std::function<void()> completionHandler);
        static void downloadLevelArchive(std::string archiveUrl, const std::string& levelHash, std::function<void()> completionHandler);

        static SongManager& sharedInstance();
        static std::string getLevelHashFromID(const std::string& id);
        static std::string getRootDownloadsDirectory();
        static std::string getLevelDownloadsDirectory();

    private:
        GlobalNamespace::BeatmapLevelsModel* _levelsModel {};
        GlobalNamespace::AlwaysOwnedContentContainerSO* _contentContainer {};
        System::Threading::CancellationTokenSource* _songReloadTokenSource {};

        static std::map<UnityEngine::Networking::UnityWebRequestAsyncOperation*, std::function<void(UnityEngine::Networking::UnityWebRequest*)>> handlerPool;
        static void performWebRequest(std::string url, std::function<void(UnityEngine::Networking::UnityWebRequest*)> completionHandler);
        static void webRequestCompletionForwarder(UnityEngine::Networking::UnityWebRequestAsyncOperation* op);
//        static GlobalNamespace::IBeatmapLevelPackCollection* onCustomSongReloadCompletion(System::Threading::Tasks::Task_1<GlobalNamespace::IBeatmapLevelPackCollection*>* task);
    };
}
