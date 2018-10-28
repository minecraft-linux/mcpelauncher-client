#include "client_app_platform.h"

#include <iostream>
#include <minecraft/ImagePickingCallback.h>
#include <minecraft/FilePickerSettings.h>
#include <minecraft/MinecraftGame.h>
#include <file_picker_factory.h>
#include <hybris/dlfcn.h>
#include <minecraft/Keyboard.h>
#include "utf8_util.h"

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
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::pickImageOld), &ClientAppPlatform::pickImageOld);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::pickFile), &ClientAppPlatform::pickFile);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::setFullscreenMode), &ClientAppPlatform::setFullscreenMode);
    vtr.replace("_ZN11AppPlatform12showKeyboardERKSsibbbiRK4Vec2", &ClientAppPlatform::showKeyboard);
    vtr.replace("_ZN11AppPlatform17updateTextBoxTextERKSs", &ClientAppPlatform::updateTextBoxText);
    vtr.replace("_ZN11AppPlatform12hideKeyboardEv", &ClientAppPlatform::hideKeyboard);
    vtr.replace("_ZNK11AppPlatform17supportsClipboardEv", &ClientAppPlatform::supportsClipboard);
    vtr.replace("_ZNK11AppPlatform12setClipboardERKSs", &ClientAppPlatform::setClipboard);
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

void ClientAppPlatform::pickImage(std::shared_ptr<ImagePickingCallback> callback) {
    Log::trace(TAG, "pickImage");
    auto picker = FilePickerFactory::createFilePicker();
    picker->setTitle("Select image");
    picker->setFileNameFilters({ "*.png" });
    if (picker->show())
        callback->onImagePickingSuccess(picker->getPickedFile());
    else
        callback->onImagePickingCanceled();
}

void ClientAppPlatform::pickImageOld(ImagePickingCallback &callback) {
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

void ClientAppPlatform::showKeyboard(mcpe::string const &text, int i, bool b, bool b2, bool b3, int i2, Vec2 const &v) {
    AppPlatform::showKeyboard(text, i, b, b2, b3, i2, v);
    currentText = text.std();
    currentTextPosition = currentText.size();
    currentTextPositionUTF = UTF8Util::getLength(currentText.c_str(), currentTextPosition);
    Keyboard::_inputCaretLocation->push_back(currentTextPositionUTF);
}

void ClientAppPlatform::updateTextBoxText(mcpe::string const &text) {
    currentText = text.std();
}

void ClientAppPlatform::hideKeyboard() {
    AppPlatform::hideKeyboard();
    currentText.clear();
    currentTextPosition = 0;
    currentTextPositionUTF = 0;
}

void ClientAppPlatform::onKeyboardText(MinecraftGame &game, std::string const &text) {
    if (text.size() == 1 && text[0] == 8) { // backspace
        if (currentTextPositionUTF <= 0)
            return;
        currentTextPositionUTF--;
        auto deleteStart = currentTextPosition - 1;
        while (deleteStart > 0 && (currentText[deleteStart] & 0b11000000) == 0b10000000)
            deleteStart--;
        currentText.erase(currentText.begin() + deleteStart, currentText.begin() + currentTextPosition);
        currentTextPosition = deleteStart;
    } else if (text.size() == 1 && text[0] == 127) { // delete key
        if (currentTextPosition >= currentText.size())
            return;
        auto deleteEnd = currentTextPosition + 1;
        while (deleteEnd < currentText.size() && (currentText[deleteEnd] & 0b11000000) == 0b10000000)
            deleteEnd++;
        currentText.erase(currentText.begin() + currentTextPosition, currentText.begin() + deleteEnd);
    } else {
        currentText.insert(currentText.begin() + currentTextPosition, text.begin(), text.end());
        currentTextPosition += text.size();
        currentTextPositionUTF += UTF8Util::getLength(text.c_str(), text.size());
    }
    game.setTextboxText(currentText, 0);
    Keyboard::_inputCaretLocation->push_back(currentTextPositionUTF);
}

void ClientAppPlatform::onKeyboardDirectionKey(DirectionKey key) {
    if (key == DirectionKey::RightKey) {
        if (currentTextPosition >= currentText.size())
            return;
        currentTextPosition++;
        while (currentTextPosition < currentText.size() &&
               (currentText[currentTextPosition] & 0b11000000) == 0b10000000)
            currentTextPosition++;
        currentTextPositionUTF++;
    } else if (key == DirectionKey::LeftKey) {
        if (currentTextPosition <= 0)
            return;
        currentTextPosition--;
        while (currentTextPosition > 0 && (currentText[currentTextPosition] & 0b11000000) == 0b10000000)
            currentTextPosition--;
        currentTextPositionUTF--;
    } else if (key == DirectionKey::HomeKey) {
        currentTextPosition = 0;
        currentTextPositionUTF = 0;
    } else if (key == DirectionKey::EndKey) {
        currentTextPosition = currentText.size();
        currentTextPositionUTF = UTF8Util::getLength(currentText.c_str(), currentTextPosition);
    }
    Keyboard::_inputCaretLocation->push_back(currentTextPositionUTF);
}

void ClientAppPlatform::copyCurrentText() {
    window->setClipboardText(currentText);
}

void ClientAppPlatform::setClipboard(mcpe::string const &text) {
    window->setClipboardText(text.std());
}