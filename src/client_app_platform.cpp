#include "client_app_platform.h"

#include <iostream>
#include <minecraft/ImagePickingCallback.h>
#include <minecraft/FilePickerSettings.h>
#include <file_picker_factory.h>

const char* ClientAppPlatform::TAG = "ClientAppPlatform";

void ClientAppPlatform::hideMousePointer() {
    window->setCursorDisabled(true);
}
void ClientAppPlatform::showMousePointer() {
    window->setCursorDisabled(false);
}

void ClientAppPlatform::pickImage(ImagePickingCallback &callback) {
    Log::trace(TAG, "pickImage");
    auto picker = FilePickerFactory::createFilePicker();
    picker->setTitle("Select image");
    picker->setFileNameFilters({ "*.png" });
    if (picker->show())
        callback.onImagePickingSuccess(picker->getPickedFile());
    else
        callback.onImagePickingCanceled();
}

void ClientAppPlatform::pickFile(FilePickerSettings &settings) {
    std::cout << "pickFile\n";
    std::cout << "- title: " << settings.pickerTitle << "\n";
    std::cout << "- type: " << (int) settings.type << "\n";
    std::cout << "- file descriptions:\n";
    for (FilePickerSettings::FileDescription &d : settings.fileDescriptions) {
        std::cout << " - " << d.ext << " " << d.desc << "\n";
    }

    auto picker = FilePickerFactory::createFilePicker();
    picker->setTitle(settings.pickerTitle.std());
    if (settings.type == FilePickerSettings::PickerType::SAVE)
        picker->setMode(FilePicker::Mode::SAVE);
    std::vector<std::string> filters;
    for (auto const& filter : settings.fileDescriptions)
        filters.push_back(std::string("*.") + filter.ext.std());
    picker->setFileNameFilters(filters);
    if (picker->show())
        settings.pickedCallback(settings, picker->getPickedFile());
    else
        settings.cancelCallback(settings);
}

void ClientAppPlatform::setFullscreenMode(int mode) {
    Log::trace(TAG, "Changing fullscreen mode: %i", mode);
    window->setFullscreen(mode != 0);
}
