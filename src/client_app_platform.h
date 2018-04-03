#pragma once

#include <mcpelauncher/app_platform.h>
#include <game_window.h>

class ClientAppPlatform : public LauncherAppPlatform {

private:
    static const char* TAG;

    std::shared_ptr<GameWindow> window;

    static void replaceVtableEntry(void* src, void* dest);

    template <typename T, typename T2>
    static void replaceVtableEntry(T src, T2 dest) {
        replaceVtableEntry(PatchUtils::memberFuncCast(src), PatchUtils::memberFuncCast(dest));
    }

public:
    static void** myVtable;
    static void initVtable(void* lib);

    ClientAppPlatform();

    void setWindow(std::shared_ptr<GameWindow> window) {
        this->window = std::move(window);
    }

    void hideMousePointer();
    void showMousePointer();

    void pickImage(ImagePickingCallback& callback);
    void pickFile(FilePickerSettings& callback);
    bool supportsFilePicking() { return true; }

    void setFullscreenMode(int mode);

};