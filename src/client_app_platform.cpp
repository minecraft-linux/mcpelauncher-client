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
    size_t size = LauncherAppPlatform::myVtableSize;

    myVtable = (void**) ::operator new((size + 1) * sizeof(void*));
    myVtable[size] = nullptr;
    memcpy(&myVtable[0], &vt[0], size * sizeof(void*));

    PatchUtils::VtableReplaceHelper vtr (lib, myVtable, vt);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::hideMousePointer), &ClientAppPlatform::hideMousePointer);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::showMousePointer), &ClientAppPlatform::showMousePointer);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::pickImage), &ClientAppPlatform::pickImage);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::pickFile), &ClientAppPlatform::pickFile);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::setFullscreenMode), &ClientAppPlatform::setFullscreenMode);
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
