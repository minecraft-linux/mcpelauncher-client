#pragma once

#include <string>
#include <functional>
#include <key_mapping.h>
#include <game_window.h>

struct TextInputHandler {
public:
    using TextCallback = std::function<void(std::string)>;

private:
    bool enabled = false, multiline = false, shiftPressed = false;
    std::string currentText;
    size_t currentTextPosition = 0;
    size_t currentTextPositionUTF = 0;
    size_t currentTextCopyPosition = 0;
    TextCallback textUpdateCallback;

public:
    explicit TextInputHandler(TextCallback cb) : textUpdateCallback(std::move(cb)) {}

    bool isEnabled() const { return enabled; }

    bool isMultiline() const { return multiline; }

    void enable(std::string text, bool multiline);

    void update(std::string text);

    void disable();

    void onTextInput(std::string const &val);

    void onKeyPressed(KeyCode key, KeyAction action);

    std::string getCopyText() const;

    int getCursorPosition() const { return currentTextPositionUTF; }
};
