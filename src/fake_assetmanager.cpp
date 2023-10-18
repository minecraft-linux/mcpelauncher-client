#include <cstdio>
#include <cstring>
#include <cerrno>
#include <FileUtil.h>
#include <sys/types.h>
#include <dirent.h>
#include <log.h>
#include <libc_shim.h>
#include <android/compat.h>
#include "fake_assetmanager.h"

struct AAsset {
    std::string buffer;
    off64_t offset = 0;
};
struct AAssetDir {
    DIR *dir;
    dirent *ent;
    std::string dirname;
    std::string currentFileName;
};

FakeAssetManager::FakeAssetManager(std::string rootDir) {
    if(!rootDir.empty() && *rootDir.rbegin() != '/')
        rootDir += '/';
    this->rootDir = std::move(rootDir);
}

namespace fake_assetmanager {

AAsset *AAssetManager_open(FakeAssetManager *amgr, const char *filename, int mode) {
    std::string fullPath;
    if(filename == NULL) {
#ifndef NDEBUG
        Log::trace("AAssetManager", "Opening file NULLPTR failed.\n");
#endif
        return nullptr;
    }

    if(filename[0] != '/') {
        fullPath = amgr->rootDir + filename;
    } else {
        // Ignore full paths, the game tries to open user data files with the AAssetManager
        return nullptr;
    }

#ifndef NDEBUG
    Log::trace("AAssetManager", "Opening file '%s' as '%s'\n", filename, fullPath.c_str());
#endif

    std::string content;
    if(!FileUtil::readFile(fullPath, content))
        return nullptr;

    auto ret = new AAsset;
    ret->buffer = content;
    return ret;
}

AAssetDir *AAssetManager_openDir(FakeAssetManager *amgr, const char *dirname) {
    if(dirname == NULL) {
#ifndef NDEBUG
        Log::trace("AAssetManager", "Opening directory NULLPTR failed.\n");
#endif
        return nullptr;
    }

    std::string fullPath;
    if(dirname[0] != '/') {
        fullPath = amgr->rootDir + dirname;
    } else {
        // Ignore full paths, the game tries to open user data files with the AAssetManager
        return nullptr;
    }

#ifndef NDEBUG
    Log::trace("AAssetManager", "Opening directory '%s' as '%s'\n", dirname, fullPath.c_str());
#endif

    DIR *d = opendir(fullPath.c_str());
    if(!d)
        return nullptr;

    auto ret = new AAssetDir;
    ret->dir = d;
    ret->ent = nullptr;
    ret->dirname = dirname;
    return ret;
}

void AAsset_close(AAsset *asset) {
    delete asset;
}

int AAsset_isAllocated(AAsset *asset) {
    return true;
}

ssize_t AAsset_read(AAsset *asset, void *buf, size_t count) {
    if(asset->offset > asset->buffer.size()) {
        return 0;
    }
    size_t max_len = asset->buffer.size() - asset->offset;
    if(count > max_len) {
        count = max_len;
    }
    if(count == 0) {
        return 0;
    }
    memcpy(buf, &asset->buffer[asset->offset], count);
    asset->offset += count;
    return (ssize_t)count;
}

off64_t AAsset_seek64(AAsset *asset, off64_t offset, int whence) {
    off64_t cur_pos = asset->offset;
    off64_t max_pos = asset->buffer.size();
    off64_t new_offset;

    if(whence == SEEK_SET) {
        new_offset = offset;
    } else if(whence == SEEK_CUR) {
        new_offset = cur_pos + offset;
    } else if(whence == SEEK_END) {
        new_offset = max_pos + offset;
    }
    if(new_offset < 0 || new_offset > max_pos)
        return -1;
    asset->offset = new_offset;
    return new_offset;
}

off_t AAsset_seek(AAsset *asset, off_t offset, int whence) {
    return (off_t)AAsset_seek64(asset, offset, whence);
}

off64_t AAsset_getLength64(AAsset *asset) {
    return (off64_t)asset->buffer.size();
}

off_t AAsset_getLength(AAsset *asset) {
    return (off_t)asset->buffer.size();
}

off64_t AAsset_getRemainingLength64(AAsset *asset) {
    return (off64_t)(asset->buffer.size() - asset->offset);
}

off_t AAsset_getRemainingLength(AAsset *asset) {
    return (off_t)(asset->buffer.size() - asset->offset);
}

const void *AAsset_getBuffer(AAsset *asset) {
    return asset->buffer.c_str();
}

void AAssetDir_close(AAssetDir *assetDir) {
    if(assetDir)
        closedir(assetDir->dir);
    delete assetDir;
}

void AAssetDir_rewind(AAssetDir *assetDir) {
    rewinddir(assetDir->dir);
}

const char *AAssetDir_getNextFileName(AAssetDir *assetDir) {
    if(!assetDir)
        return nullptr;
    assetDir->ent = readdir(assetDir->dir);
    if(!assetDir->ent)
        return nullptr;
    std::string cname = assetDir->ent->d_name;
    if(cname == "." || cname == "..") {
        return AAssetDir_getNextFileName(assetDir);
    }
    assetDir->currentFileName = cname;
#ifndef NDEBUG
    Log::trace("AAssetDir", "'%s' getNextFileName '%s'\n", assetDir->dirname.data(), assetDir->currentFileName.data());
#endif
    return assetDir->currentFileName.data();
}

}  // namespace fake_assetmanager

void FakeAssetManager::initHybrisHooks(std::unordered_map<std::string, void *> &syms) {
    using namespace fake_assetmanager;
    syms["AAssetManager_open"] = (void *)AAssetManager_open;
    syms["AAssetManager_openDir"] = (void *)AAssetManager_openDir;
    syms["AAsset_close"] = (void *)AAsset_close;
    syms["AAsset_isAllocated"] = (void *)AAsset_isAllocated;
    syms["AAsset_read"] = (void *)AAsset_read;
    syms["AAsset_seek64"] = (void *)AAsset_seek64;
    syms["AAsset_seek"] = (void *)AAsset_seek;
    syms["AAsset_getLength64"] = (void *)AAsset_getLength64;
    syms["AAsset_getLength"] = (void *)AAsset_getLength;
    syms["AAsset_getRemainingLength64"] = (void *)AAsset_getRemainingLength64;
    syms["AAsset_getRemainingLength"] = (void *)AAsset_getRemainingLength;
    syms["AAsset_getBuffer"] = (void *)AAsset_getBuffer;
    syms["AAssetDir_close"] = (void *)AAssetDir_close;
    syms["AAssetDir_rewind"] = (void *)AAssetDir_rewind;
    syms["AAssetDir_getNextFileName"] = (void *)AAssetDir_getNextFileName;
}
