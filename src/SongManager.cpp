#include <string>
#include <utility>
#include <fstream>

#include "utils/typedefs.h"
#include "utils/il2cpp-functions.hpp"
#include "SongManager.hpp"
#include "private.hpp"
#include "config/config-utils.hpp"
#include "modloader.hpp"
#include "zip.h"
#include "SongLoader.hpp"

#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/DownloadHandler.hpp"
#include "UnityEngine/Resources.hpp"

#include "GlobalNamespace/IBeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/IAnnotatedBeatmapLevelCollection.hpp"
#include "GlobalNamespace/BeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/FileCompressionHelper.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"

#include "System/Threading/CancellationTokenSource.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Threading/Tasks/ITaskCompletionAction.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Action.hpp"
#include "System/Action_1.hpp"
#include "System/Func_1.hpp"
#include "System/Func_2.hpp"
#include "System/Runtime/CompilerServices/TaskAwaiter_1.hpp"

using namespace QuestMultiplayer;
using namespace UnityEngine;
using namespace UnityEngine::Networking;
using namespace GlobalNamespace;
using namespace rapidjson;
using namespace System;
using namespace System::Threading;
using namespace System::Threading::Tasks;

std::map<UnityWebRequestAsyncOperation*, std::function<void(UnityWebRequest*)>> SongManager::handlerPool;

SongManager::SongManager() = default;

GlobalNamespace::BeatmapLevelsModel *SongManager::getLevelsModel() {
    if (!_levelsModel) {
        _levelsModel = Resources::FindObjectsOfTypeAll<BeatmapLevelsModel*>()->values[0];
    }

    return _levelsModel;
}

SongManager& SongManager::sharedInstance() {
    static SongManager instance = SongManager();
    return instance;
}

GlobalNamespace::IPreviewBeatmapLevel *SongManager::getLevelPackByID(const std::string& id) {
    auto levelIdCsStr = il2cpp_utils::createcsstr(id);
    return getLevelPackByID(levelIdCsStr);
}

GlobalNamespace::IPreviewBeatmapLevel *SongManager::getLevelPackByID(Il2CppString* id) {
    try {
        auto model = getLevelsModel();

        return model->GetLevelPreviewForLevelId(id);
    } catch (...) {
        getLogger().info("SongManager::getLevelPackByID: Level pack with ID %ls was not found.", &id->m_firstChar);
        return nullptr;
    }
}

std::string SongManager::getLevelHashFromID(const std::string &id) {
    auto sepIndex = id.find_last_of('_');

    if (sepIndex != std::string::npos) {
        return id.substr(sepIndex + 1);
    }

    return id;
}

void SongManager::webRequestCompletionForwarder(UnityWebRequestAsyncOperation* op) {
    auto& callback = SongManager::handlerPool[op];
    auto request = op->webRequest;

    auto responseCode = request->get_responseCode();
    auto responseStatusStr = to_utf8(csstrtostr(UnityWebRequest::GetHTTPStatusString(responseCode)));
    getLogger().info("Server responded with status %s", responseStatusStr.data());

    if (callback) {
        callback(request);
        handlerPool.erase(op);
    } else {
        getLogger().error("Web request completed but no handler is set to handle it.");
    }
}

void SongManager::performWebRequest(std::string url, std::function<void(UnityWebRequest*)> completionHandler) {
    auto csUrl = il2cpp_utils::createcsstr(url);
    auto webRequest = UnityWebRequest::Get(csUrl);

    auto userAgent = "QuestMultiplayer/" + getModInfo().version;
    webRequest->SetRequestHeader(il2cpp_utils::createcsstr("User-Agent"), il2cpp_utils::createcsstr(userAgent));

    getLogger().info("Sending web request to \"%s\"...", url.data());

    auto webRequestOp = webRequest->SendWebRequest();
    handlerPool[webRequestOp] = std::move(completionHandler);
    webRequestOp->add_completed(il2cpp_utils::MakeDelegate<System::Action_1<AsyncOperation*>*>(
        classof(System::Action_1<AsyncOperation*>*),
        webRequestOp,
        SongManager::webRequestCompletionForwarder
    ));
}

void SongManager::downloadLevelByHash(const std::string& hash, std::function<void()> completionHandler) {
    auto requestUrl = "https://beatsaver.com/api/maps/by-hash/" + hash;
    performWebRequest(requestUrl, [=](UnityWebRequest* request) {
        auto downloadHandler = request->get_downloadHandler();
        auto responseText = to_utf8(csstrtostr(downloadHandler->GetText()));

        Document beatmapInfo;
        beatmapInfo.Parse(responseText.data());

        if (beatmapInfo.IsObject() && beatmapInfo.HasMember("directDownload") && beatmapInfo.HasMember("name")) {
            std::string downloadUrl = std::string("https://beatsaver.com") + beatmapInfo["directDownload"].GetString();
            std::string name = beatmapInfo["name"].GetString();
            getLogger().info("Downloading song %s from \"%s\"...", name.data(), downloadUrl.data());
            SongManager::downloadLevelArchive(downloadUrl, hash, completionHandler);
        } else {
            getLogger().warning("Failed to retrieve beatmap information");
            completionHandler();
        }
    });
}

void SongManager::downloadLevelArchive(std::string archiveUrl, const std::string& levelHash, std::function<void()> completionHandler) {
    auto levelId = "custom_level_" + levelHash;
    getLogger().info("Downloading level %s from \"%s\"...", levelId.data(), archiveUrl.data());
    performWebRequest(archiveUrl, [=](UnityWebRequest* request) {
        auto downloadHandler = request->get_downloadHandler();
        auto responseData = downloadHandler->GetData();

        if (responseData->Length() > 0) {
            auto levelPath = getLevelDownloadsDirectory() + levelId;
            auto levelArchivePath = getLevelDownloadsDirectory() + levelId + ".zip";

            std::ofstream archiveWriteStream;
            archiveWriteStream.open(levelArchivePath, std::ios::binary | std::ios::out);
            archiveWriteStream.write(reinterpret_cast<const char *>(responseData->values), responseData->Length());
            archiveWriteStream.close();

            if (!direxists(levelPath)) {
                mkpath(levelPath);
            }

            getLogger().info("Extracting archive %s to path %s...", levelArchivePath.data(), levelPath.data());

            int args = 2;
            zip_extract(levelArchivePath.data(), levelPath.data(), +[](const char *name, void *arg) -> int {
                getLogger().info("Extracted file: %s", name);
                return 0;
            }, &args);

            getLogger().info("Level %s downloaded to path %s.", levelId.data(), levelPath.data());
            std::remove(levelArchivePath.data());

            // Load the downloaded song
//            SongManager::sharedInstance().loadLevelFromPath(levelPath);
            SongManager::sharedInstance().updateSongs();
        } else {
            getLogger().info("Failed to download level. Empty response.");
        }

        completionHandler();
    });
}

std::string SongManager::getRootDownloadsDirectory() {
    auto path = string_format("/sdcard/Android/data/%s/cache/multiplayer_downloads/", Modloader::getApplicationId().data());
    if (!direxists(path)) {
        getLogger().info("Creating multiplayer cache directory...");
        mkpath(path);
    }
    return path;
}

std::string SongManager::getLevelDownloadsDirectory() {
    auto path = "/sdcard/BeatSaberSongs/";
    if (!direxists(path)) {
        getLogger().info("Creating CustomSongs downloads cache directory...");
        mkpath(path);
    }
    return path;
}

GlobalNamespace::CustomPreviewBeatmapLevel *SongManager::loadLevelFromPath(std::string path) {
    getLogger().info("Loading song at path: %s...", path.data());

    auto model = getLevelsModel();
    auto levelPathStr = il2cpp_utils::createcsstr(path);
    auto levelInfoData = LoadCustomLevelInfoSaveData(levelPathStr);
    auto levelId = "custom_level_" + GetCustomLevelHash(levelInfoData, path);
    auto songName = to_utf8(csstrtostr(levelInfoData->get_songName()));

    // Make everything lowercased
    std::transform(levelId.begin(), levelId.end(), levelId.begin(), [](auto c) { return std::tolower(c); });

    getLogger().info("Loading preview for song %s at path %s...", songName.data(), path.data());
//    model->
    auto levelPreview = LoadCustomPreviewBeatmapLevelAsync(levelPathStr, levelInfoData);

    if (levelPreview && model) {
        getLogger().info("Registering song %s at path %s...", levelId.data(), path.data());
        auto levelIdStr = il2cpp_utils::createcsstr(levelId);
        model->loadedPreviewBeatmapLevels->Add(levelIdStr, reinterpret_cast<IPreviewBeatmapLevel *>(levelPreview));
    }

    return levelPreview;
}

void SongManager::updateSongs() {
    if (!_songReloadTokenSource) {
        _songReloadTokenSource = CancellationTokenSource::New_ctor();
    }

    // Prepare song loader
    getLogger().info("Reloading custom songs...");
//    Resources::FindObjectsOfTypeAll<LevelFilteringNavigationController*>()->values[0]->UpdateCustomSongs();

    auto model = getLevelsModel();
    auto token = _songReloadTokenSource->get_Token();
    auto reloadTask = model->ReloadCustomLevelPackCollectionAsync(token);
}

AlwaysOwnedContentContainerSO *SongManager::getContentContainer() {
    if (!_contentContainer) {
        _contentContainer = Resources::FindObjectsOfTypeAll<AlwaysOwnedContentContainerSO*>()->values[0];
    }
    return _contentContainer;
}

//IBeatmapLevelPackCollection* SongManager::onCustomSongReloadCompletion(Task_1<IBeatmapLevelPackCollection*>* task) {
//    auto collection = task->get_Result();
//    getLogger().info("Custom song reload completion.");
//    return collection;
//}
