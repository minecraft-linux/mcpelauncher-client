#include "client_app_platform.h"

#include <iostream>
#include <minecraft/ImagePickingCallback.h>
#include <minecraft/FilePickerSettings.h>
#include <minecraft/MinecraftGame.h>
#include <minecraft/legacy/AppPlatform.h>
#include <file_picker_factory.h>
#include <hybris/dlfcn.h>
#include <minecraft/Keyboard.h>
#include <mcpelauncher/minecraft_version.h>
#include "utf8_util.h"
#include "minecraft_game_wrapper.h"

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
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::pickFileOld), &ClientAppPlatform::pickFileOld);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::setFullscreenMode), &ClientAppPlatform::setFullscreenMode);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::swapBuffers), &ClientAppPlatform::swapBuffers);
    vtr.replace(PatchUtils::memberFuncCast(&LauncherAppPlatform::supportsFilePicking), &ClientAppPlatform::supportsFilePicking);
    vtr.replace("_ZN11AppPlatform12showKeyboardERKSsibbbiRKN3glm5tvec2IfLNS2_9precisionE0EEE", &ClientAppPlatform::showKeyboard);
    vtr.replace("_ZN11AppPlatform12showKeyboardERKSsibbbiRK4Vec2", &ClientAppPlatform::showKeyboardLegacy_pre_1_13);
    vtr.replace("_ZN11AppPlatform12showKeyboardERKSsibbbRK4Vec2", &ClientAppPlatform::showKeyboardLegacy);
    vtr.replace("_ZN11AppPlatform17updateTextBoxTextERKSs", &ClientAppPlatform::updateTextBoxText);
    vtr.replace("_ZN11AppPlatform12hideKeyboardEv", &ClientAppPlatform::hideKeyboard);
    vtr.replace("_ZNK11AppPlatform17supportsClipboardEv", &ClientAppPlatform::supportsClipboard);
    vtr.replace("_ZNK11AppPlatform12setClipboardERKSs", &ClientAppPlatform::setClipboard);
    vtr.replace("_ZNK11AppPlatform12supportsMSAAEv", &ClientAppPlatform::supportsMSAA);
}

ClientAppPlatform::ClientAppPlatform() {
    vtable = myVtable;

    // both 0.15.90.7 and 0.15.90.8 are 0.15.90.1 internally; can't target one of them sadly
    if (MinecraftVersion::isAtLeast(0, 15, 90, /*8*/2) && !MinecraftVersion::isAtLeast(1, 13))
        getHardwareInformation().deviceModel = "Linux";
}

void ClientAppPlatform::hideMousePointer() {
    if (window)
        window->setCursorDisabled(true);
}
void ClientAppPlatform::showMousePointer() {
    if (window)
        window->setCursorDisabled(false);
}

void ClientAppPlatform::pickImage(mcpe::shared_ptr<ImagePickingCallback> callback) {
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

void ClientAppPlatform::pickFile(mcpe::shared_ptr<FilePickerSettings> settings) {
    std::cout << "pickFile\n";
    std::cout << "- title: " << settings->pickerTitle << "\n";
    std::cout << "- type: " << (int) settings->type << "\n";
    std::cout << "- file descriptions:\n";
    for (FilePickerSettings::FileDescription &d : settings->fileDescriptions) {
        std::cout << " - " << d.ext << " " << d.desc << "\n";
    }

    auto picker = FilePickerFactory::createFilePicker();
    picker->setTitle(settings->pickerTitle.std());
    if (settings->type == FilePickerSettings::PickerType::SAVE)
        picker->setMode(FilePicker::Mode::SAVE);
    std::vector<std::string> filters;
    for (auto const& filter : settings->fileDescriptions)
        filters.push_back(std::string("*.") + filter.ext.std());
    picker->setFileNameFilters(filters);
    if (picker->show())
        settings->pickedCallback(settings, picker->getPickedFile());
    else
        settings->cancelCallback(settings);
}

void ClientAppPlatform::pickFileOld(Legacy::Pre_1_8::FilePickerSettings &settings) {
    std::cout << "pickFile\n";
    std::cout << "- title: " << settings.pickerTitle << "\n";
    std::cout << "- type: " << (int) settings.type << "\n";
    std::cout << "- file descriptions:\n";
    for (Legacy::Pre_1_8::
    FilePickerSettings::FileDescription &d : settings.fileDescriptions) {
        std::cout << " - " << d.ext << " " << d.desc << "\n";
    }

    auto picker = FilePickerFactory::createFilePicker();
    picker->setTitle(settings.pickerTitle.std());
    if (settings.type == Legacy::Pre_1_8::FilePickerSettings::PickerType::SAVE)
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

void ClientAppPlatform::swapBuffers() {
    window->swapBuffers();
}

void ClientAppPlatform::update() {
    if (isKeyboardVisible())
        Keyboard::_inputCaretLocation->push_back(currentTextPositionUTF);
}

void ClientAppPlatform::showKeyboard(mcpe::string const &text, int i, bool b, bool b2, bool multiline,
        int i2, Vec2 const &v) {
    AppPlatform::showKeyboard(text, i, b, b2, multiline, i2, v);
    currentTextMutliline = multiline;
    updateTextBoxText(text);
}

void ClientAppPlatform::showKeyboardLegacy_pre_1_13(mcpe::string const &text, int i, bool b, bool b2, bool multiline,
        int i2, Vec2 const &v) {
    ((Legacy::Pre_1_2_13::AppPlatform*) (AppPlatform*) this)->showKeyboard(text, i, b, b2, multiline, i2, v);
    currentTextMutliline = multiline;
    updateTextBoxText(text);
}

void ClientAppPlatform::showKeyboardLegacy(mcpe::string const &text, int i, bool b, bool b2, bool multiline,
        Vec2 const &v) {
    ((Legacy::Pre_1_2_10::AppPlatform*) (AppPlatform*) this)->showKeyboard(text, i, b, b2, multiline, v);
    currentTextMutliline = multiline;
    updateTextBoxText(text);
}

void ClientAppPlatform::updateTextBoxText(mcpe::string const &text) {
    currentText = text.std();
    currentTextPosition = currentText.size();
    currentTextPositionUTF = UTF8Util::getLength(currentText.c_str(), currentTextPosition);
    currentTextCopyPosition = currentTextPosition;
    Keyboard::_inputCaretLocation->push_back(currentTextPositionUTF);
}

void ClientAppPlatform::hideKeyboard() {
    AppPlatform::hideKeyboard();
    currentText.clear();
    currentTextPosition = 0;
    currentTextPositionUTF = 0;
    currentTextCopyPosition = 0;
}

void ClientAppPlatform::onKeyboardText(MinecraftGameWrapper &game, std::string const &text) {
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
    currentTextCopyPosition = currentTextPosition;
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
    if (!isShiftPressed)
        currentTextCopyPosition = currentTextPosition;
}

void ClientAppPlatform::onKeyboardShiftKey(bool shiftPressed) {
    isShiftPressed = shiftPressed;
}

void ClientAppPlatform::copyCurrentText() {
    if (currentTextCopyPosition != currentTextPosition) {
        size_t start = std::min(currentTextPosition, currentTextCopyPosition);
        size_t end = std::max(currentTextPosition, currentTextCopyPosition);
        window->setClipboardText(currentText.substr(start, end - start));
    } else {
        window->setClipboardText(currentText);
    }
}

void ClientAppPlatform::setClipboard(mcpe::string const &text) {
    window->setClipboardText(text.std());
}