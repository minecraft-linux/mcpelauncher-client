#pragma once

#include <string>
#include <string.h>
#include <functional>
#include <key_mapping.h>
#include <game_window.h>

struct TextInputHandler {
public:
    using TextCallback = std::function<void(std::string)>;
    using CaretCallback = std::function<void(int)>;

private:
    bool usesLegacyTextInput = false;
    bool enabled = false, multiline = false, altPressed = false;
    std::string currentText;
    size_t currentTextPosition = 0;
    size_t currentTextPositionUTF = 0;
    size_t currentTextCopyPosition = 0;
    size_t currentTextCopyPositionUTF = 0;
    TextCallback textUpdateCallback;
    CaretCallback caretPositionCallback;
    constexpr static char spaces[] = " -_#/\\!@$%^&*();:'\"?.,";
    size_t enabledNo = 0;
    std::string lastInput;
    bool keepOnce = false;
    bool shiftPressed = false;

public:
    TextInputHandler(TextCallback cb, CaretCallback caretCb) {
        textUpdateCallback = std::move(cb);
        caretPositionCallback = std::move(caretCb);
    }

    bool isEnabled() const { return enabled; }

    size_t getEnabledNo() const { return enabledNo; }

    bool isMultiline() const { return multiline; }

    void enable(std::string text, bool multiline);

    void update(std::string text);

    void disable();

    void onTextInput(std::string const& val);

    void onKeyPressed(KeyCode key, KeyAction action, int mods);

    std::string getCopyText() const;

    std::string getText() const { return currentText; };

    int getCursorPosition() const { return currentTextPositionUTF; }

    int getCopyPosition() const { return currentTextCopyPositionUTF; }

    void setCursorPosition(int pos);

    void setKeepLastCharOnce();

    bool getKeepLastCharOnce();

    bool getUsesLegacyTextInput() {
        return usesLegacyTextInput;
    };
};
