#pragma once

#include <map>
#include <unordered_set>

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
        /// Song download completion handler type. std::nullopt if download fails, otherwise the path to the downloaded beatmap.
        using DownloadCompletionHandler = std::function<void(std::optional<std::string>)>;
        static const std::string MultiplayerCacheSongPackID;

        SongManager();

        GlobalNamespace::BeatmapLevelsModel* getLevelsModel();
        GlobalNamespace::CustomLevelLoader* getBuiltInLevelLoader();
        GlobalNamespace::AlwaysOwnedContentContainerSO* getContentContainer();
        GlobalNamespace::IPreviewBeatmapLevel* getLevelPreviewByID(Il2CppString* id);
        GlobalNamespace::IPreviewBeatmapLevel* getLevelPackByID(const std::string& id);

        void updateSongs();

        static GlobalNamespace::CustomPreviewBeatmapLevel* loadLevelFromPath(std::string path);
        static bool downloadLevelByHash(const std::string& hash, DownloadCompletionHandler completionHandler);
        static bool downloadLevelByID(const std::string& beatmapId, DownloadCompletionHandler completionHandler);
        static bool downloadLevelArchive(std::string archiveUrl, const std::string& levelHash, DownloadCompletionHandler completionHandler);
        static bool levelIsDownloaded(const std::string& hash);
        static std::string customLevelPath(const std::string& hash);

        static SongManager& sharedInstance();
        static std::string getLevelHashFromID(const std::string& id);
        static std::string getRootDownloadsDirectory();
        static std::string getLevelDownloadsDirectory();

    private:
        using InternalResponseHandler = std::function<void(UnityEngine::Networking::UnityWebRequest*)>;

        GlobalNamespace::BeatmapLevelsModel* _levelsModel {};
        GlobalNamespace::AlwaysOwnedContentContainerSO* _contentContainer {};
        GlobalNamespace::CustomLevelLoader* _crippledLoader {};
        System::Threading::CancellationTokenSource* _songReloadTokenSource {};

        static std::map<std::string, GlobalNamespace::CustomPreviewBeatmapLevel*> cachedPreviewLevels;
        static std::unordered_set<std::string> downloadingLevels;
        static std::map<UnityEngine::Networking::UnityWebRequestAsyncOperation*, InternalResponseHandler> handlerPool;

        static void performWebRequest(std::string url, InternalResponseHandler completionHandler);
        static void webRequestCompletionForwarder(UnityEngine::Networking::UnityWebRequestAsyncOperation* op);

        void registerPreviewBeatmap(GlobalNamespace::CustomPreviewBeatmapLevel* previewBeatmapLevel);
    };
}
