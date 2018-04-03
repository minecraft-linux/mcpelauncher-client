#pragma once

#include <mcpelauncher/app_platform.h>
#include <game_window.h>

class ClientAppPlatform : public LauncherAppPlatform {

private:
    static const char* TAG;

    std::shared_ptr<GameWindow> window;

public:
    void setWindow(std::shared_ptr<GameWindow> window) {
        window = std::move(window);
    }

    void hideMousePointer() override;
    void showMousePointer() override;

    void pickImage(ImagePickingCallback& callback) override;
    void pickFile(FilePickerSettings& callback) override;
    bool supportsFilePicking() override { return true; }

    void setFullscreenMode(int mode) override;

};