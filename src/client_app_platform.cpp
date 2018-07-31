#include "client_app_platform.h"

#include <iostream>
#include <minecraft/ImagePickingCallback.h>
#include <minecraft/FilePickerSettings.h>
#include <file_picker_factory.h>
#include <hybris/dlfcn.h>

const char* ClientAppPlatform::TAG = "ClientAppPlatform";

void** ClientAppPlatform::myVtable = nullptr;

void ClientAppPlatform::initVtable(void* lib) {
    if (LauncherAppPlatform::myVtable == nullptr)
        LauncherAppPlatform::initVtable(lib);
    void** vt = LauncherAppPlatform::myVtable;

    // get vtable size
    int size;
    for (size = 0; ; size++) {
        if (vt[size] == nullptr)
            break;
    }
    Log::trace("AppPlatform", "Vtable size = %i", size);

    myVtable = (void**) ::operator new((size + 1) * sizeof(void*));
    myVtable[size] = nullptr;
    memcpy(&myVtable[0], &vt[0], size * sizeof(void*));

    replaceVtableEntry(&LauncherAppPlatform::hideMousePointer, &ClientAppPlatform::hideMousePointer);
    replaceVtableEntry(&LauncherAppPlatform::showMousePointer, &ClientAppPlatform::showMousePointer);
    replaceVtableEntry(&LauncherAppPlatform::pickImage, &ClientAppPlatform::pickImage);
    replaceVtableEntry(&LauncherAppPlatform::pickFile, &ClientAppPlatform::pickFile);
    replaceVtableEntry(&LauncherAppPlatform::setFullscreenMode, &ClientAppPlatform::setFullscreenMode);
}

void ClientAppPlatform::replaceVtableEntry(void* src, void* dest) {
    for (int i = 0; ; i++) {
        if (myVtable[i] == nullptr)
            break;
        if (myVtable[i] == src) {
            myVtable[i] = dest;
            return;
        }
    }
}

ClientAppPlatform::ClientAppPlatform() {
    vtable = myVtable;
}

void ClientAppPlatform::hideMousePointer() {
    if (window)
        window->setCursorDisabled(true);
}
void ClientAppPlatform::showMousePointer() {
    if (window)
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
