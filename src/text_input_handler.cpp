#include "text_input_handler.h"
#include "utf8_util.h"

void TextInputHandler::enable(std::string text, bool multiline) {
    enabled = true;
    this->multiline = multiline;
    update(std::move(text));
}

void TextInputHandler::update(std::string text) {
    currentText = std::move(text);
    currentTextPosition = currentText.size();
    currentTextPositionUTF = UTF8Util::getLength(currentText.c_str(), currentTextPosition);
    currentTextCopyPosition = currentTextPosition;
}

void TextInputHandler::disable() {
    currentText.clear();
    currentTextPosition = 0;
    currentTextPositionUTF = 0;
    currentTextCopyPosition = 0;
    enabled = false;
}

void TextInputHandler::onTextInput(std::string const &text) {
    if(!enabled) {
        textUpdateCallback(text);
        return;
    }
    if(text.size() == 1 && text[0] == 8) {  // backspace
        if(currentTextPositionUTF <= 0)
            return;
        auto deleteStart = currentTextPosition - 1;
        if (altPressed) {
            while (deleteStart > 0) {
                deleteStart--;
                if (deleteStart < 1 || strchr(spaces, currentText[deleteStart - 1])) {
                    break;
                }
            }
            currentTextPositionUTF = deleteStart;
        } else {
            deleteStart = currentTextPosition - 1;
            currentTextPositionUTF--;
        }
        while(deleteStart > 0 && (currentText[deleteStart] & 0b11000000) == 0b10000000)
            deleteStart--;
        currentText.erase(currentText.begin() + deleteStart, currentText.begin() + currentTextPosition);
        currentTextPosition = deleteStart;
    } else if(text.size() == 1 && text[0] == 127) {  // delete key
        if(currentTextPosition >= currentText.size())
            return;
        auto deleteEnd = currentTextPosition + 1;
        while(deleteEnd < currentText.size() && (currentText[deleteEnd] & 0b11000000) == 0b10000000)
            deleteEnd++;
        currentText.erase(currentText.begin() + currentTextPosition, currentText.begin() + deleteEnd);
    } else {
        currentText.insert(currentText.begin() + currentTextPosition, text.begin(), text.end());
        currentTextPosition += text.size();
        currentTextPositionUTF += UTF8Util::getLength(text.c_str(), text.size());
    }
    textUpdateCallback(currentText);
    currentTextCopyPosition = currentTextPosition;
}

void TextInputHandler::onKeyPressed(KeyCode key, KeyAction action) {
    if(key == KeyCode::LEFT_SHIFT || key == KeyCode::RIGHT_SHIFT)
        shiftPressed = (action != KeyAction::RELEASE);
    if(key == KeyCode::LEFT_ALT || key == KeyCode::RIGHT_ALT)
        altPressed = (action != KeyAction::RELEASE);
    
    if(action != KeyAction::PRESS && action != KeyAction::REPEAT)
        return;
    if(key == KeyCode::RIGHT) {
        if(currentTextPosition >= currentText.size())
            return;
        if (altPressed) {
            while (currentTextPosition < currentText.size()) {
                currentTextPosition++;
                if (currentTextPosition >= currentText.size() || strchr(spaces, currentText[currentTextPosition])) {
                    break;
                }
            }
            currentTextPositionUTF = UTF8Util::getLength(currentText.c_str(), currentTextPosition);
        } else {
            currentTextPosition++;
            while(currentTextPosition < currentText.size() &&
                (currentText[currentTextPosition] & 0b11000000) == 0b10000000)
                currentTextPosition++;
            currentTextPositionUTF++;
        }
    } else if(key == KeyCode::LEFT) {
        if(currentTextPosition <= 0)
            return;
        if (altPressed) {
            while (currentTextPosition > 0) {
                currentTextPosition--;
                if (strchr(spaces, currentText[currentTextPosition - 1])) {
                    break;
                }
            }
            currentTextPositionUTF = UTF8Util::getLength(currentText.c_str(), currentTextPosition);
        } else {
            currentTextPosition--;
            while(currentTextPosition > 0 && (currentText[currentTextPosition] & 0b11000000) == 0b10000000)
                currentTextPosition--;
            currentTextPositionUTF--;
        }
    } else if(key == KeyCode::HOME) {
        currentTextPosition = 0;
        currentTextPositionUTF = 0;
    } else if(key == KeyCode::END) {
        currentTextPosition = currentText.size();
        currentTextPositionUTF = UTF8Util::getLength(currentText.c_str(), currentTextPosition);
    }
    if(!shiftPressed)
        currentTextCopyPosition = currentTextPosition;
}

std::string TextInputHandler::getCopyText() const {
    if(currentTextCopyPosition != currentTextPosition) {
        size_t start = std::min(currentTextPosition, currentTextCopyPosition);
        size_t end = std::max(currentTextPosition, currentTextCopyPosition);
        return currentText.substr(start, end - start);
    } else {
        return currentText;
    }
}
