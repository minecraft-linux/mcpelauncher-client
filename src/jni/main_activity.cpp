#include "main_activity.h"

#include <unistd.h>
#ifndef __APPLE__
#include <sys/sysinfo.h>
#else
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif
#include <file_picker_factory.h>
#include <game_window_manager.h>
#include "uuid.h"
#include <climits>
#include <sstream>

#include <log.h>

FakeJni::JInt BuildVersion::SDK_INT = 27;
std::shared_ptr<FakeJni::JString> BuildVersion::RELEASE = std::make_shared<FakeJni::JString>("AndroidX");

std::shared_ptr<FakeJni::JString> MainActivity::createUUID() {
    return UUID::randomUUID()->toString();
}

FakeJni::JLong MainActivity::getUsedMemory() {
#ifdef __APPLE__
    uint64_t page_size;
    size_t len = sizeof(page_size);
    sysctlbyname("hw.pagesize", &page_size, &len, NULL, 0);

    struct vm_statistics64 stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&stat, &count);

    double page_K = page_size / (double)1024;
    return (stat.active_count + stat.wire_count) * page_K * 1000;
#else
    FILE* file = fopen("/proc/self/statm", "r");
    if(file == nullptr)
        return 0L;
    int pageSize = getpagesize();
    long long pageCount = 0L;
    fscanf(file, "%lld", &pageCount);
    fclose(file);
    return pageCount * pageSize;
#endif
}

FakeJni::JLong MainActivity::getFreeMemory() {
#ifdef __APPLE__
    uint64_t page_size;
    size_t len = sizeof(page_size);
    sysctlbyname("hw.pagesize", &page_size, &len, NULL, 0);

    struct vm_statistics64 stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&stat, &count);

    double page_K = page_size / (double)1024;
    return stat.free_count * page_K * 1000;
#else
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long long total = memInfo.freeram;
    total *= memInfo.mem_unit;
    return total;
#endif
}

FakeJni::JLong MainActivity::getTotalMemory() {
#ifdef __APPLE__
    uint64_t memsize;
    size_t len = sizeof(memsize);
    sysctlbyname("hw.memsize", &memsize, &len, NULL, 0);
    return memsize;
#else
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    long long total = memInfo.totalram;
    total *= memInfo.mem_unit;
    return total;
#endif
}

FakeJni::JLong MainActivity::getMemoryLimit() {
    return getTotalMemory();
}

FakeJni::JLong MainActivity::getAvailableMemory() {
    return getTotalMemory();
}

void MainActivity::pickImage(FakeJni::JLong callback) {
    try {
        auto picker = FilePickerFactory::createFilePicker();
        picker->setTitle("Select image");
        picker->setFileNameFilters({"*.png"});
        if(picker->show()) {
            auto method = getClass().getMethod("(JLjava/lang/String;)V", "nativeOnPickImageSuccess");
            FakeJni::LocalFrame frame;
            method->invoke(frame.getJniEnv(), this, callback, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(picker->getPickedFile())));
        } else {
            auto method = getClass().getMethod("(J)V", "nativeOnPickImageCanceled");
            FakeJni::LocalFrame frame;
            method->invoke(frame.getJniEnv(), this, callback);
        }
    } catch(const std::exception& e) {
        GameWindowManager::getManager()->getErrorHandler()->onError("FilePickerFactory", std::string("Failed to open the file-picker details: ") + e.what());
        auto method = getClass().getMethod("(J)V", "nativeOnPickImageCanceled");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), this, callback);
    }
}

void MainActivity::initializeXboxLive(FakeJni::JLong xalinit, FakeJni::JLong xblinit) {
    auto method = getClass().getMethod("(JJ)V", "nativeInitializeXboxLive");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), this, xalinit, xblinit);
}

void MainActivity::requestIntegrityToken(std::shared_ptr<FakeJni::JString> str) {
    auto method = getClass().getMethod("(Ljava/lang/String;)V", "nativeSetIntegrityToken");
    FakeJni::LocalFrame frame;
    method->invoke(frame.getJniEnv(), this, createUUID());
}

void MainActivity::launchUri(std::shared_ptr<FakeJni::JString> url) {
    int pid;
    if ((pid = fork())) {
    } else {
    #ifdef __APPLE__
        execl("/usr/bin/open", "/usr/bin/open", url->asStdString().c_str(), NULL);
    #else
        execl("/usr/bin/xdg-open", "/usr/bin/xdg-open", url->asStdString().c_str(), NULL);
    #endif
        _Exit(0);
    }
}

void MainActivity::setClipboard(std::shared_ptr<FakeJni::JString> tocopy) {
    window->setClipboardText(tocopy->asStdString());
}

void MainActivity::share(std::shared_ptr<FakeJni::JString> title, std::shared_ptr<FakeJni::JString> string, std::shared_ptr<FakeJni::JString> url) {
    if ((title->asStdString().find("\"") == std::string::npos) && (title->asStdString().find("\\") == std::string::npos) && (string->asStdString().find("\"") == std::string::npos) && (string->asStdString().find("\\") == std::string::npos)) {
        int pid;
        if ((pid = fork())) {
        } else {
        #ifdef __APPLE__
            execl("/usr/bin/osascript", "/usr/bin/osascript", "-e", ("display alert \"" + title->asStdString() + "\" message \"" + string->asStdString() + "\n" + url->asStdString() + "\"").c_str(), NULL);
        #else
            execl("/usr/bin/zenity", "/usr/bin/zenity", "--info", "--title", title->asStdString().c_str(), "--text", (string->asStdString() + "\n" + url->asStdString()).c_str(), NULL);
        #endif
            _Exit(0);
        }
    }
}

FakeJni::JLong MainActivity::initializeXboxLive2(FakeJni::JLong xalinit, FakeJni::JLong xblinit) {
    auto method = getClass().getMethod("(JJ)V", "nativeInitializeXboxLive");
    FakeJni::LocalFrame frame;
    auto ret = method->invoke(frame.getJniEnv(), this, xalinit, xblinit);
    return ret.j;
}

FakeJni::JLong MainActivity::initializeLibHttpClient(FakeJni::JLong init) {
    auto method = getClass().getMethod("(J)J", "nativeinitializeLibHttpClient");
    if(!method) {
        method = getClass().getMethod("(J)J", "nativeInitializeLibHttpClient");
    }
    FakeJni::LocalFrame frame;
    auto ret = method->invoke(frame.getJniEnv(), this, init);
    return ret.j;
}

std::shared_ptr<FakeJni::JIntArray> MainActivity::getImageData(std::shared_ptr<FakeJni::JString> filename) {
    if(!stbi_load_from_memory || !stbi_image_free)
        return 0;
    int width, height, channels;
    std::ifstream f(filename->asStdString().data());
    if(!f.is_open())
        return 0;
    std::stringstream s;
    s << f.rdbuf();
    auto buf = s.str();
    auto image = stbi_load_from_memory((unsigned char*)buf.data(), buf.length(), &width, &height, &channels, 4);
    if(!image)
        return 0;
    auto ret = std::make_shared<FakeJni::JIntArray>(2 + width * height);
    (*ret)[0] = width;
    (*ret)[1] = height;

    for(int x = 0; x < width * height; x++) {
        (*ret)[2 + x] = (image[x * 4 + 2]) | (image[x * 4 + 1] << 8) | (image[x * 4 + 0] << 16) | (image[x * 4 + 3] << 24);
    }
    stbi_image_free(image);
    return ret;
}

std::shared_ptr<FakeJni::JByteArray> MainActivity::getFileDataBytes(std::shared_ptr<FakeJni::JString> path) {
    return std::make_shared<FakeJni::JByteArray>();
}

void MainActivity::startPlayIntegrityCheck() {
    // auto method = PlayIntegrity::getDescriptor()->getMethod("()V", "nativePlayIntegrityComplete");
    // FakeJni::LocalFrame frame;
    // method->invoke(frame.getJniEnv(), PlayIntegrity::getDescriptor().get());
}

void MainActivity::openFile() {
    try {
        auto picker = FilePickerFactory::createFilePicker();
        picker->setTitle("Select file");
        if(picker->show()) {
            auto method = getClass().getMethod("(Ljava/lang/String;)V", "nativeOnPickFileSuccess");
            FakeJni::LocalFrame frame;
            method->invoke(frame.getJniEnv(), this, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(picker->getPickedFile())));
        } else {
            auto method = getClass().getMethod("()V", "nativeOnPickFileCanceled");
            FakeJni::LocalFrame frame;
            method->invoke(frame.getJniEnv(), this);
        }
    } catch(const std::exception& e) {
        GameWindowManager::getManager()->getErrorHandler()->onError("FilePickerFactory", std::string("Failed to open the file-picker details: ") + e.what());
        auto method = getClass().getMethod("()V", "nativeOnPickFileCanceled");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), this);
    }
}
void MainActivity::saveFile(std::shared_ptr<FakeJni::JString> fileName) {
    try {
        auto picker = FilePickerFactory::createFilePicker();
        picker->setMode(FilePicker::Mode::SAVE);
        picker->setTitle("Select file");
        picker->setFileName(fileName->asStdString());
        if(picker->show()) {
            auto method = getClass().getMethod("(Ljava/lang/String;)V", "nativeOnPickFileSuccess");
            FakeJni::LocalFrame frame;
            method->invoke(frame.getJniEnv(), this, frame.getJniEnv().createLocalReference(std::make_shared<FakeJni::JString>(picker->getPickedFile())));
        } else {
            auto method = getClass().getMethod("()V", "nativeOnPickFileCanceled");
            FakeJni::LocalFrame frame;
            method->invoke(frame.getJniEnv(), this);
        }
    } catch(const std::exception& e) {
        GameWindowManager::getManager()->getErrorHandler()->onError("FilePickerFactory", std::string("Failed to open the file-picker details: ") + e.what());
        auto method = getClass().getMethod("()V", "nativeOnPickFileCanceled");
        FakeJni::LocalFrame frame;
        method->invoke(frame.getJniEnv(), this);
    }
}

