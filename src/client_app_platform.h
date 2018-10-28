#pragma once

#include <mcpelauncher/app_platform.h>
#include <game_window.h>

class ClientAppPlatform : public LauncherAppPlatform {

private:
    static const char* TAG;

    std::shared_ptr<GameWindow> window;

public:
    static void** myVtable;
    static void initVtable(void* lib);

    ClientAppPlatform();

    void setWindow(std::shared_ptr<GameWindow> window) {
        this->window = std::move(window);
    }

    void hideMousePointer();
    void showMousePointer();

    void pickImage(std::shared_ptr<ImagePickingCallback>);
    void pickImageOld(ImagePickingCallback& callback);
    void pickFile(FilePickerSettings& callback);
    bool supportsFilePicking() { return true; }

    void setFullscreenMode(int mode);

};