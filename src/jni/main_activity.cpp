#include "main_activity.h"
#include "../settings.h"

#include <unistd.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/vmmeter.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#else
#include <sys/sysinfo.h>
#endif
#include <file_picker_factory.h>
#include <game_window_manager.h>
#include "uuid.h"
#include <climits>
#include <sstream>
#include <fstream>
#include <android/keycodes.h>
#include "../core_patches.h"

#include <log.h>

FakeJni::JInt BuildVersion::SDK_INT = 32;
std::shared_ptr<FakeJni::JString> BuildVersion::RELEASE = std::make_shared<FakeJni::JString>("AndroidX");

std::shared_ptr<FakeJni::JString> MainActivity::createUUID() {
    return UUID::randomUUID()->toString();
}

FakeJni::JFloat MainActivity::getPixelsPerMillimeter() {
    // assume 96 DPI for now with GUI scale of 2
    return (96 / 25.4f) * 2 * Settings::scale;
}

FakeJni::JInt MainActivity::getPlatformDpi() {
    // assume 96 DPI for now with GUI scale of 2
    return 96 * 2 * Settings::scale;
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
#elif defined(__FreeBSD__)
    int mib[4] = {CTL_VM, KERN_PROC, KERN_PROC_PID, getpid()};
    struct kinfo_proc info = {};
    size_t size = sizeof(info);
    if(sysctl(mib, 4, &info, &size, NULL, 0) != 0)
        return 0L;
    // from usr.bin/top/machine.c macro PROCSIZE
    return info.ki_size / 1024;
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
#elif defined(__FreeBSD__)
    uint64_t page_size;
    size_t len = sizeof(page_size);
    sysctlbyname("hw.pagesize", &page_size, &len, NULL, 0);

    int mib[2] = {CTL_VM, VM_TOTAL};
    struct vmtotal info = {};
    size_t size = sizeof(info);
    if(sysctl(mib, 2, &info, &size, NULL, 0) != 0)
        return 0L;

    double page_K = page_size / (double)1024;
    return info.t_free * page_K * 1000;
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
#elif defined(__FreeBSD__)
    uint64_t page_size;
    size_t len = sizeof(page_size);
    sysctlbyname("hw.pagesize", &page_size, &len, NULL, 0);

    int mib[2] = {CTL_VM, VM_TOTAL};
    struct vmtotal info = {};
    size_t size = sizeof(info);
    if(sysctl(mib, 2, &info, &size, NULL, 0) != 0)
        return 0L;

    double page_K = page_size / (double)1024;
    return info.t_vm * page_K * 1000;
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
    if((pid = fork())) {
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
    if((title->asStdString().find("\"") == std::string::npos) && (title->asStdString().find("\\") == std::string::npos) && (string->asStdString().find("\"") == std::string::npos) && (string->asStdString().find("\\") == std::string::npos)) {
        int pid;
        if((pid = fork())) {
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

void MainActivity::shareFile(std::shared_ptr<FakeJni::JString> title, std::shared_ptr<FakeJni::JString> string, std::shared_ptr<FakeJni::JString> path) {
    auto picker = FilePickerFactory::createFilePicker();
    picker->setMode(FilePicker::Mode::SAVE);
    picker->setTitle(title->asStdString());
    std::string pathStr = path->asStdString();
    picker->setFileName(pathStr.substr(pathStr.find_last_of("/\\") + 1));
    if(picker->show()) {
        std::ifstream src(pathStr, std::ios::binary);
        std::ofstream dst(picker->getPickedFile(), std::ios::binary);
        dst << src.rdbuf();
        src.close();
        dst.close();
    }
}

FakeJni::JInt MainActivity::getCaretPosition() {
    ignoreNextHideKeyboard = false;
    if(textInput) {
        return textInput->getCursorPosition();
    }
    return -1;
}

void MainActivity::setCaretPosition(FakeJni::JInt pos) {
    if(textInput)
        textInput->setCursorPosition(pos);
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

// Allow Marketplace Content bigger than 432MB to download
FakeJni::JBoolean MainActivity::supportsSizeQuery(std::shared_ptr<FakeJni::JString> path) {
    // const char* rpath = path->data();
    // printf("supportsSizeQuery: %s\n", rpath);
    return true;
}

FakeJni::JLong MainActivity::getAllocatableBytes(std::shared_ptr<FakeJni::JString> path) {
    // const char* rpath = path->data();
    // printf("getAllocatableBytes: %s\n", rpath);
    return 1024LL * 1024LL * 1024LL * 1024LL;
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

KeyCode mapAndroidToMinecraftKey(int keyCode) {
    if(keyCode >= AKEYCODE_F1 && keyCode <= AKEYCODE_F12)
        return (KeyCode)(keyCode - AKEYCODE_F1 + (int)KeyCode::FN1);
    if(keyCode >= AKEYCODE_0 && keyCode <= AKEYCODE_9)
        return (KeyCode)(keyCode - AKEYCODE_0 + (int)KeyCode::NUM_0);
    if(keyCode >= AKEYCODE_NUMPAD_0 && keyCode <= AKEYCODE_NUMPAD_9)
        return (KeyCode)(keyCode - AKEYCODE_NUMPAD_0 + (int)KeyCode::NUMPAD_0);
    if(keyCode >= AKEYCODE_A && keyCode <= AKEYCODE_Z)
        return (KeyCode)(keyCode - AKEYCODE_A + (int)KeyCode::A);
    switch(keyCode) {
    case AKEYCODE_DEL:
        return KeyCode::BACKSPACE;
    case AKEYCODE_TAB:
        return KeyCode::TAB;
    case AKEYCODE_ENTER:
        return KeyCode::ENTER;
    case AKEYCODE_SHIFT_LEFT:
        return KeyCode::LEFT_SHIFT;
    case AKEYCODE_SHIFT_RIGHT:
        return KeyCode::RIGHT_SHIFT;
    case AKEYCODE_CTRL_LEFT:
        return KeyCode::LEFT_CTRL;
    case AKEYCODE_CTRL_RIGHT:
        return KeyCode::RIGHT_CTRL;
    case AKEYCODE_BREAK:
        return KeyCode::PAUSE;
    case AKEYCODE_CAPS_LOCK:
        return KeyCode::CAPS_LOCK;
    case AKEYCODE_ESCAPE:
        return KeyCode::ESCAPE;
    case AKEYCODE_PAGE_UP:
        return KeyCode::PAGE_UP;
    case AKEYCODE_PAGE_DOWN:
        return KeyCode::PAGE_DOWN;
    case AKEYCODE_MOVE_END:
        return KeyCode::END;
    case AKEYCODE_MOVE_HOME:
        return KeyCode::HOME;
    case AKEYCODE_DPAD_LEFT:
        return KeyCode::LEFT;
    case AKEYCODE_DPAD_UP:
        return KeyCode::UP;
    case AKEYCODE_DPAD_RIGHT:
        return KeyCode::RIGHT;
    case AKEYCODE_DPAD_DOWN:
        return KeyCode::DOWN;
    case AKEYCODE_INSERT:
        return KeyCode::INSERT;
    case AKEYCODE_FORWARD_DEL:
        return KeyCode::DELETE;
    case AKEYCODE_NUM_LOCK:
        return KeyCode::NUM_LOCK;
    case AKEYCODE_SCROLL_LOCK:
        return KeyCode::SCROLL_LOCK;
    case AKEYCODE_SEMICOLON:
        return KeyCode::SEMICOLON;
    case AKEYCODE_EQUALS:
        return KeyCode::EQUAL;
    case AKEYCODE_COMMA:
        return KeyCode::COMMA;
    case AKEYCODE_MINUS:
        return KeyCode::MINUS;
    case AKEYCODE_PERIOD:
        return KeyCode::PERIOD;
    case AKEYCODE_SLASH:
        return KeyCode::SLASH;
    case AKEYCODE_GRAVE:
        return KeyCode::GRAVE;
    case AKEYCODE_LEFT_BRACKET:
        return KeyCode::LEFT_BRACKET;
    case AKEYCODE_BACKSLASH:
        return KeyCode::BACKSLASH;
    case AKEYCODE_RIGHT_BRACKET:
        return KeyCode::RIGHT_BRACKET;
    case AKEYCODE_APOSTROPHE:
        return KeyCode::APOSTROPHE;
    case AKEYCODE_META_LEFT:
        return KeyCode::LEFT_SUPER;
    case AKEYCODE_META_RIGHT:
        return KeyCode::RIGHT_SUPER;
    case AKEYCODE_ALT_LEFT:
        return KeyCode::LEFT_ALT;
    case AKEYCODE_ALT_RIGHT:
        return KeyCode::RIGHT_ALT;
    case AKEYCODE_NUMPAD_ENTER:
        return KeyCode::ENTER;
    case AKEYCODE_NUMPAD_SUBTRACT:
        return KeyCode::NUMPAD_SUBTRACT;
    case AKEYCODE_NUMPAD_MULTIPLY:
        return KeyCode::NUMPAD_MULTIPLY;
    case AKEYCODE_NUMPAD_ADD:
        return KeyCode::NUMPAD_ADD;
    case AKEYCODE_NUMPAD_DIVIDE:
        return KeyCode::NUMPAD_DIVIDE;
    case AKEYCODE_NUMPAD_DOT:
        return KeyCode::NUMPAD_DECIMAL;
    }
    if(keyCode < 256)
        return (KeyCode)keyCode;
    return KeyCode::UNKNOWN;
}

int mapAndroidMeta(int meta) {
    int outMeta = 0;
    if(meta & AMETA_SHIFT_ON) {
        outMeta |= KEY_MOD_SHIFT;
    }
    if(meta & AMETA_CTRL_ON) {
        outMeta |= KEY_MOD_CTRL;
    }
    if(meta & AMETA_META_ON) {
        outMeta |= KEY_MOD_SUPER;
    }
    if(meta & AMETA_ALT_ON) {
        outMeta |= KEY_MOD_ALT;
    }
    if(meta & AMETA_CAPS_LOCK_ON) {
        outMeta |= KEY_MOD_CAPSLOCK;
    }
    if(meta & AMETA_NUM_LOCK_ON) {
        outMeta |= KEY_MOD_NUMLOCK;
    }
    return outMeta;
}

FakeJni::JInt MainActivity::getKeyFromKeyCode(FakeJni::JInt keyCode, FakeJni::JInt metaState, FakeJni::JInt deviceId) {
    if(!Settings::enable_keyboard_autofocus_patches_1_20_60) {
        return 0;
    }

#ifdef __APPLE__
    int modCTRL = metaState & AMETA_META_ON;
#else
    int modCTRL = metaState & AMETA_CTRL_ON;
#endif
    if(modCTRL && keyCode == AKEYCODE_V && (!textInput || !textInput->isEnabled())) {
        if(Settings::enable_keyboard_autofocus_paste_patches_1_20_60) {
            CorePatches::setPendingDelayedPaste();
            return 'v';
        }
        return 0;
    }

    if(keyCode >= AKEYCODE_F1 && keyCode <= AKEYCODE_F12) {
        return 0;
    }
    auto ret = lastChar;
    switch(keyCode) {
    case AKEYCODE_FORWARD_DEL:
    case AKEYCODE_SHIFT_LEFT:
    case AKEYCODE_SHIFT_RIGHT:
    case AKEYCODE_ALT_LEFT:
    case AKEYCODE_ALT_RIGHT:
    case AKEYCODE_CTRL_LEFT:
    case AKEYCODE_CTRL_RIGHT:
    case AKEYCODE_CAPS_LOCK:
    case AKEYCODE_META_LEFT:
    case AKEYCODE_META_RIGHT:
    case AKEYCODE_ESCAPE:
    case AKEYCODE_ENTER:
    case AKEYCODE_VOLUME_UP:
    case AKEYCODE_VOLUME_DOWN:
    case AKEYCODE_VOLUME_MUTE:
    case AKEYCODE_DPAD_LEFT:
    case AKEYCODE_DPAD_RIGHT:
    case AKEYCODE_DPAD_UP:
    case AKEYCODE_DPAD_UP_LEFT:
    case AKEYCODE_DPAD_UP_RIGHT:
    case AKEYCODE_DPAD_DOWN:
    case AKEYCODE_DPAD_DOWN_LEFT:
    case AKEYCODE_DPAD_DOWN_RIGHT:
    case AKEYCODE_UNKNOWN:
        return 0;
    }

    uint32_t gameWindowKeycode = window->getKeyFromKeyCode(mapAndroidToMinecraftKey(keyCode), mapAndroidMeta(metaState));
    if(gameWindowKeycode != 0) {
        return gameWindowKeycode;
    }

    lastChar = 0;
    return ret;
}

void MainActivity::setLastChar(FakeJni::JInt sym) {
    lastChar = sym;
}

void MainActivity::lockCursor() {
    CorePatches::hideMousePointer();
}

void MainActivity::unlockCursor() {
    CorePatches::showMousePointer();
}

std::shared_ptr<FakeJni::JString> CharBuffer::toString() {
    return this->data;
}

std::shared_ptr<Charset> Charset::forName(std::shared_ptr<FakeJni::JString> arg0) {
    return std::make_shared<Charset>();
}

std::shared_ptr<CharBuffer> Charset::decode(std::shared_ptr<jnivm::ByteBuffer> arg0) {
    return std::make_shared<CharBuffer>(std::make_shared<FakeJni::JString>(std::string((const char*)arg0->buffer, arg0->capacity)));
}

TextInputState::TextInputState(std::shared_ptr<FakeJni::JString> text, FakeJni::JInt selectionStart_in, FakeJni::JInt selectionEnd_in,
                               FakeJni::JInt composingRegionStart_in, FakeJni::JInt composingRegionEnd_in)
    : text(text), selectionStart(selectionStart_in), selectionEnd(selectionEnd_in),
      composingRegionStart(composingRegionStart_in), composingRegionEnd(composingRegionEnd_in) {
}

void TextInputConnection::setState(std::shared_ptr<TextInputState> arg0) {
    this->state = arg0;
    if(textInput) {
        textInput->update(this->state ? this->state->text->asStdString() : "");
        textInput->setCursorPosition(this->state ? this->state->selectionEnd : 0);
    }
}

void TextInputConnection::setSoftKeyboardActive(FakeJni::JBoolean arg0, FakeJni::JInt arg1) {
    if(arg0) {
        if(textInput)
            textInput->enable(this->state ? this->state->text->asStdString() : "", false);
    } else {
        if(textInput)
            textInput->disable();
    }
}

void TextInputConnection::restartInput() {
    
}