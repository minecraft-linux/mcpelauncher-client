#include <cstdio>
#include <cstring>
#include <cerrno>
#include <FileUtil.h>
#include <sys/types.h>
#include <dirent.h>
#include <log.h>
#include <android/compat.h>
#include "fake_assetmanager.h"

struct AAsset {
    std::string buffer;
    size_t offset = 0;
};
struct AAssetDir {
    DIR *dir;
    dirent *ent;
};

FakeAssetManager::FakeAssetManager(std::string rootDir) {
    if (!rootDir.empty() && *rootDir.rbegin() != '/')
        rootDir += '/';
    this->rootDir = std::move(rootDir);
}

namespace fake_assetmanager {

AAsset *AAssetManager_open(FakeAssetManager *amgr, const char* filename, int mode) {
    std::string fullPath = amgr->rootDir + filename;
    Log::trace("AAssetManager", "AAssetManager_open %s\n", fullPath.c_str());

    std::string content;
    if (!FileUtil::readFile(fullPath, content))
        return nullptr;

    auto ret = new AAsset;
    ret->buffer = content;
    return ret;
}

AAssetDir *AAssetManager_openDir(FakeAssetManager *amgr, const char *dirname) {
    std::string fullPath = amgr->rootDir + dirname;
    Log::trace("AAssetManager", "AAssetManager_openDir %s\n", fullPath.c_str());

    DIR *d = opendir(fullPath.c_str());
    if (!d)
        return nullptr;

    auto ret = new AAssetDir;
    ret->dir = d;
    ret->ent = nullptr;
    return ret;
}

void AAsset_close(AAsset *asset) {
    delete asset;
}

int AAsset_isAllocated(AAsset *asset) {
    return true;
}

int AAsset_read(AAsset *asset, void* buf, size_t count) {
    if (asset->offset > asset->buffer.size())
        return 0;
    count = std::min(count, asset->buffer.size() - asset->offset);
    memcpy(buf, &asset->buffer[asset->offset], count);
    asset->offset += count;
    return (int) count;
}

off64_t AAsset_seek64(AAsset *asset, off64_t offset, int whence) {
    size_t newOffset = 0;
    if (whence == SEEK_SET) {
        newOffset = (size_t) offset;
    } else if (whence == SEEK_CUR) {
        newOffset = (size_t) (asset->offset + offset);
    } else if (whence == SEEK_END) {
        newOffset = asset->buffer.size() - offset;
    }
    if ((ssize_t) newOffset < 0)
        return -EINVAL;
    return (off64_t) newOffset;
}

off_t AAsset_seek(AAsset *asset, off_t offset, int whence) {
    return (off_t) AAsset_seek64(asset, offset, whence);
}

off64_t AAsset_getLength64(AAsset *asset) {
    return (off64_t) asset->buffer.size();
}

off_t AAsset_getLength(AAsset *asset) {
    return (off_t) asset->buffer.size();
}

off64_t AAsset_getRemainingLength64(AAsset *asset) {
    return (off64_t) (asset->buffer.size() - asset->offset);
}

off_t AAsset_getRemainingLength(AAsset *asset) {
    return (off_t) (asset->buffer.size() - asset->offset);
}

const void *AAsset_getBuffer(AAsset *asset) {
    return asset->buffer.c_str();
}

void AAssetDir_close(AAssetDir *assetDir) {
    if (assetDir)
        closedir(assetDir->dir);
    delete assetDir;
}


void AAssetDir_rewind(AAssetDir *assetDir) {
    rewinddir(assetDir->dir);
}

const char *AAssetDir_getNextFileName(AAssetDir *assetDir) {
    if (!assetDir)
        return nullptr;
    assetDir->ent = readdir(assetDir->dir);
    if (!assetDir->ent)
        return nullptr;
    return assetDir->ent->d_name;
}

}

void FakeAssetManager::initHybrisHooks(std::unordered_map<std::string, void*> &syms) {
    using namespace fake_assetmanager;
    syms["AAssetManager_open"] = (void *) AAssetManager_open;
    syms["AAssetManager_openDir"] = (void *) AAssetManager_openDir;
    syms["AAsset_close"] = (void *) AAsset_close;
    syms["AAsset_isAllocated"] = (void *) AAsset_isAllocated;
    syms["AAsset_read"] = (void *) AAsset_read;
    syms["AAsset_seek64"] = (void *) AAsset_seek64;
    syms["AAsset_seek"] = (void *) AAsset_seek;
    syms["AAsset_getLength64"] = (void *) AAsset_getLength64;
    syms["AAsset_getLength"] = (void *) AAsset_getLength;
    syms["AAsset_getRemainingLength64"] = (void *) AAsset_getRemainingLength64;
    syms["AAsset_getRemainingLength"] = (void *) AAsset_getRemainingLength;
    syms["AAsset_getBuffer"] = (void *) AAsset_getBuffer;
    syms["AAssetDir_close"] = (void *) AAssetDir_close;
    syms["AAssetDir_rewind"] = (void *) AAssetDir_rewind;
    syms["AAssetDir_getNextFileName"] = (void *) AAssetDir_getNextFileName;
}