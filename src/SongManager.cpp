#include <string>
#include <utility>
#include <fstream>

#include "utils/typedefs.h"
#include "utils/il2cpp-functions.hpp"
#include "utils/il2cpp-utils.hpp"
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
#include "UnityEngine/Sprite.hpp"

#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/IBeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/IAnnotatedBeatmapLevelCollection.hpp"
#include "GlobalNamespace/BeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp"
#include "GlobalNamespace/CustomLevelLoader.hpp"
#include "GlobalNamespace/FileCompressionHelper.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/AlwaysOwnedContentContainerSO.hpp"
#include "GlobalNamespace/BeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/IBeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"

#include "System/Threading/CancellationTokenSource.hpp"
#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"
#include "System/Threading/Tasks/ITaskCompletionAction.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Collections/Generic/HashSet_1.hpp"
#include "System/Collections/Generic/List_1.hpp"
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

const std::string SongManager::MultiplayerCacheSongPackID = "custom_levelpack_MultiplayerCache";
std::map<std::string, GlobalNamespace::CustomPreviewBeatmapLevel*> SongManager::cachedPreviewLevels;
std::unordered_set<std::string> SongManager::downloadingLevels;
std::map<UnityWebRequestAsyncOperation*, SongManager::InternalResponseHandler> SongManager::handlerPool;

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
    return getLevelPreviewByID(levelIdCsStr);
}

GlobalNamespace::IPreviewBeatmapLevel *SongManager::getLevelPreviewByID(Il2CppString* id) {
    try {
        auto model = getLevelsModel();
        return model->GetLevelPreviewForLevelId(id);
    } catch (...) {
        getLogger().info("SongManager::getLevelPreviewByID: Level pack with ID %ls was not found.", &id->m_firstChar);
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

void SongManager::performWebRequest(std::string url, SongManager::InternalResponseHandler completionHandler) {
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

bool SongManager::downloadLevelByHash(const std::string& hash, DownloadCompletionHandler completionHandler) {
    if (levelIsDownloaded(hash)) {
        completionHandler(customLevelPath(hash));
        return true;
    }

    if (downloadingLevels.contains(hash)) {
        getLogger().info("Level %s is already downloading.", hash.c_str());
        return false;
    }

    auto requestUrl = "https://beatsaver.com/api/maps/by-hash/" + hash;
    performWebRequest(requestUrl, [=](UnityWebRequest* request) {
        // Remove level from downloading songs
        SongManager::downloadingLevels.erase(hash);

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
            completionHandler(std::nullopt);
        }
    });
    downloadingLevels.insert(hash);

    return true;
}

bool SongManager::downloadLevelArchive(std::string archiveUrl, const std::string& levelHash, DownloadCompletionHandler completionHandler) {
    if (levelIsDownloaded(levelHash)) {
        completionHandler(customLevelPath(levelHash));
        return true;
    }

    if (downloadingLevels.contains(levelHash)) {
        getLogger().info("Level %s is already downloading.", levelHash.c_str());
        return false;
    }

    auto levelId = "custom_level_" + levelHash;
    getLogger().info("Downloading level %s from \"%s\"...", levelId.data(), archiveUrl.data());
    performWebRequest(archiveUrl, [=](UnityWebRequest* request) {
        // Remove level from downloading songs
        SongManager::downloadingLevels.erase(levelHash);

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

            // Call the completion handler with level path
            completionHandler(levelPath);
        } else {
            getLogger().info("Failed to download level. Empty response.");
            completionHandler(std::nullopt);
        }
    });
    downloadingLevels.insert(levelHash);

    return true;
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
    if (cachedPreviewLevels.contains(path)) {
        getLogger().info("Level at path %s has been cached.", path.data());
        return cachedPreviewLevels[path];
    }

    getLogger().info("Loading song at path: %s...", path.data());

    auto levelPathStr = il2cpp_utils::createcsstr(path);
    auto levelInfoData = LoadCustomLevelInfoSaveData(levelPathStr);
    auto levelId = "custom_level_" + GetCustomLevelHash(levelInfoData, path);
    auto songName = to_utf8(csstrtostr(levelInfoData->get_songName()));

    // Make everything lowercased
    std::transform(levelId.begin(), levelId.end(), levelId.begin(), [](auto c) { return std::tolower(c); });

    getLogger().info("Loading preview for song %s at path %s...", songName.data(), path.data());

    auto levelPreview = LoadCustomPreviewBeatmapLevelAsync(levelPathStr, levelInfoData);
    cachedPreviewLevels[path] = levelPreview;

    // Register the song
//    sharedInstance().registerPreviewBeatmap(levelPreview);

    return levelPreview;
}

void SongManager::updateSongs() {
    // Prepare song loader
    getLogger().info("Reloading custom songs...");

    auto model = getLevelsModel();

    _songReloadTokenSource = CancellationTokenSource::New_ctor();
    auto token = _songReloadTokenSource->get_Token();
    model->ReloadCustomLevelPackCollectionAsync(token);
}

AlwaysOwnedContentContainerSO *SongManager::getContentContainer() {
    if (!_contentContainer) {
        _contentContainer = Resources::FindObjectsOfTypeAll<AlwaysOwnedContentContainerSO*>()->values[0];
    }

    return _contentContainer;
}

bool SongManager::levelIsDownloaded(const std::string& hash) {
    return direxists(customLevelPath(hash));
}

std::string SongManager::customLevelPath(const std::string& hash) {
    return getLevelDownloadsDirectory() + "custom_level_" + hash;
}

bool SongManager::downloadLevelByID(const std::string& beatmapId, DownloadCompletionHandler completionHandler) {
    auto levelHash = getLevelHashFromID(beatmapId);
    return downloadLevelByHash(levelHash, std::move(completionHandler));
}

void SongManager::registerPreviewBeatmap(CustomPreviewBeatmapLevel* previewLevel) {
    auto levelID = previewLevel->get_levelID();
    auto levelIDStr = to_string(levelID);

    auto model = getLevelsModel();

    // Should have been added by SongLoader
//    auto ownedContainer = getContentContainer();
//    ownedContainer->get_alwaysOwnedBeatmapLevelIds()->Add(levelID);

    if (!model->customLevelPackCollection) {
        getLogger().info("Cannot register preview level: customLevelPackCollection does not exists.");
        return;
    }

    auto customPackArray = model->customLevelPackCollection->get_beatmapLevelPacks();
    auto customPackArrayLen = customPackArray->Length();
    CustomBeatmapLevelPack* multiplayerCacheLevelPack = nullptr;

    for (int i = 0; i < customPackArrayLen; ++i) {
        auto customPack = customPackArray->values[i];
        auto customPackID = to_string(customPack->get_packID());
        if (customPackID == SongManager::MultiplayerCacheSongPackID) {
            getLogger().info("Found existing pack!");
            multiplayerCacheLevelPack = reinterpret_cast<CustomBeatmapLevelPack*>(customPack);
            break;
        }
    }

    if (multiplayerCacheLevelPack) {
        getLogger().info("Adding preview to existing cache level pack...");

        auto originalLength = multiplayerCacheLevelPack->customBeatmapLevelCollection->customPreviewBeatmapLevels->Length();
        auto resizedLevelsArray = reinterpret_cast<::Array<CustomPreviewBeatmapLevel*>*>(il2cpp_functions::array_new(classof(CustomPreviewBeatmapLevel*), originalLength + 1));

        for (int i = 0; i < originalLength; ++i) {
            resizedLevelsArray->values[i] = multiplayerCacheLevelPack->customBeatmapLevelCollection->customPreviewBeatmapLevels->values[i];
        }

        resizedLevelsArray->values[originalLength] = previewLevel;
        multiplayerCacheLevelPack->customBeatmapLevelCollection->customPreviewBeatmapLevels = resizedLevelsArray;
    } else {
        getLogger().info("Creating Multiplayer cache level pack...");

        auto customPackCollectionClass = il2cpp_functions::object_get_class(reinterpret_cast<Il2CppObject *>(model->customLevelPackCollection));
        if (!il2cpp_functions::class_is_assignable_from(classof(BeatmapLevelPackCollection*), customPackCollectionClass)) {
            getLogger().error("Cannot register preview beatmap: customLevelPackCollection is not compatible with class BeatmapLevelPackCollection.");
            return;
        }

        auto levelPackCollectionContent = reinterpret_cast<::Array<IBeatmapLevelPack*>*>(il2cpp_functions::array_new(classof(IBeatmapLevelPack*), 1));
        auto levelPackContent = reinterpret_cast<::Array<CustomPreviewBeatmapLevel*>*>(il2cpp_functions::array_new(classof(CustomPreviewBeatmapLevel*), 1));
        auto levelPackCollection = CustomBeatmapLevelCollection::New_ctor(levelPackContent);

        multiplayerCacheLevelPack = CustomBeatmapLevelPack::New_ctor(
            il2cpp_utils::createcsstr(SongManager::MultiplayerCacheSongPackID),
            il2cpp_utils::createcsstr("Multiplayer Downloads"),
            il2cpp_utils::createcsstr("Multiplayer Downloads"),
            getBuiltInLevelLoader()->defaultPackCover,
            levelPackCollection
        );
        levelPackCollectionContent->values[0] = reinterpret_cast<IBeatmapLevelPack *>(multiplayerCacheLevelPack);
        levelPackContent->values[0] = previewLevel;

        auto customLevelPackCollection = reinterpret_cast<BeatmapLevelPackCollection*>(model->customLevelPackCollection);
        auto originalNumberOfPacks = customLevelPackCollection->beatmapLevelPacks->Length();
        auto numberOfCustomPacks = customLevelPackCollection->beatmapLevelPacks->Length() + 1;
        auto resizedLevelPackArray = reinterpret_cast<::Array<IBeatmapLevelPack*>*>(il2cpp_functions::array_new(classof(IBeatmapLevelPack*), numberOfCustomPacks));

        for (int i = 0; i < originalNumberOfPacks; ++i) {
            resizedLevelPackArray->values[i] = customLevelPackCollection->beatmapLevelPacks->values[i];
        }

        resizedLevelPackArray->values[originalNumberOfPacks] = reinterpret_cast<IBeatmapLevelPack *>(multiplayerCacheLevelPack);
    }

    getLogger().info("Updating previews...");
    model->UpdateLoadedPreviewLevels();
}

GlobalNamespace::CustomLevelLoader *SongManager::getBuiltInLevelLoader() {
    if (!_crippledLoader) {
        _crippledLoader = Resources::FindObjectsOfTypeAll<CustomLevelLoader*>()->values[0];
    }

    return _crippledLoader;
}
