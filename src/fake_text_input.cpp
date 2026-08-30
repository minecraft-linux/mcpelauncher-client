#include "fake_text_input.h"
#include "android/game_activity.h"
#include "android/game_text_input.h"
#include "utf8_util.h"
#include <log.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>

#ifndef NDEBUG
#define FAKE_TEXT_INPUT_DEBUG(...) Log::debug("FakeTextInput", __VA_ARGS__)
#define FAKE_TEXT_INPUT_INFO(...) Log::info("FakeTextInput", __VA_ARGS__)
#define FAKE_TEXT_INPUT_ERROR(...) Log::error("FakeTextInput", __VA_ARGS__)
#else
#define FAKE_TEXT_INPUT_DEBUG(...) ((void)0)
#define FAKE_TEXT_INPUT_INFO(...) ((void)0)
#define FAKE_TEXT_INPUT_ERROR(...) ((void)0)
#endif

struct GameTextInput {
    mutable std::recursive_mutex mutex;
    GameActivity* activity = nullptr;
    FakeTextInput::EventCallback activityCallback;
    GameTextInputEventCallback eventCallback = nullptr;
    void* eventContext = nullptr;
    GameTextInputImeInsetsCallback insetsCallback = nullptr;
    void* insetsContext = nullptr;
    ARect insets{};
    std::string text;
    size_t cursor = 0;
    size_t selectionAnchor = 0;
    GameTextInputSpan composing = {SPAN_UNDEFINED, SPAN_UNDEFINED};
    bool enabled = false;
    bool showKeyboardCalled = false;
    bool multiline = false;
    size_t enabledNo = 0;
    bool repeatNextTextEvent = false;
    size_t eventSerial = 0;
};

namespace {
std::mutex inputsMutex;
std::unordered_map<GameActivity*, std::unique_ptr<GameTextInput>> activityInputs;
GameTextInput* standaloneInput = nullptr;

GameTextInput* findInput(GameActivity* activity) {
    std::lock_guard<std::mutex> lock(inputsMutex);
    auto entry = activityInputs.find(activity);
    return entry == activityInputs.end() ? nullptr : entry->second.get();
}

std::string escapeText(const std::string& text) {
    std::ostringstream out;
    const size_t shown = std::min<size_t>(text.size(), 96);
    for(size_t i = 0; i < shown; ++i) {
        const auto ch = static_cast<unsigned char>(text[i]);
        if(ch >= 0x20 && ch < 0x7f && ch != '\\' && ch != '"') {
            out << static_cast<char>(ch);
        } else {
            out << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<unsigned int>(ch) << std::dec;
        }
    }
    if(shown < text.size())
        out << "...";
    return out.str();
}

void logStateObject(const char* operation, const GameTextInput* input,
                    const GameTextInputState* state) {
    if(!state) {
        FAKE_TEXT_INPUT_INFO("%s input=%p state=<null>", operation,
                             (const void*)input);
        return;
    }

    std::string text;
    bool truncated = false;
    if(state->text_UTF8) {
        if(state->text_length > 0) {
            const size_t suppliedLength = static_cast<size_t>(state->text_length);
            const size_t loggedLength = std::min<size_t>(suppliedLength, 96);
            text.assign(state->text_UTF8, loggedLength);
            truncated = suppliedLength > loggedLength;
        } else {
            const size_t loggedLength = strnlen(state->text_UTF8, 96);
            text.assign(state->text_UTF8, loggedLength);
            truncated = loggedLength == 96;
        }
    }

    FAKE_TEXT_INPUT_INFO(
        "%s input=%p state={text_UTF8=\"%s%s\", text_length=%d, "
        "selection={start=%d,end=%d}, composingRegion={start=%d,end=%d}}",
        operation, (const void*)input, escapeText(text).c_str(), truncated ? "..." : "",
        state->text_length, state->selection.start, state->selection.end,
        state->composingRegion.start, state->composingRegion.end);
}

void logState(const char* operation, const GameTextInput* input) {
    FAKE_TEXT_INPUT_INFO(
        "%s input=%p activity=%p enabled=%d generation=%zu length=%zu "
        "selection=[%zu,%zu] composing=[%d,%d] text=\"%s\"",
        operation, (const void*)input, input ? (void*)input->activity : nullptr,
        input ? input->enabled : false, input ? input->enabledNo : 0,
        input ? input->text.size() : 0, input ? input->selectionAnchor : 0,
        input ? input->cursor : 0, input ? input->composing.start : SPAN_UNDEFINED,
        input ? input->composing.end : SPAN_UNDEFINED,
        input ? escapeText(input->text).c_str() : "");
}

size_t clampPosition(const GameTextInput& input, int32_t position, size_t fallback) {
    if(position == SPAN_UNDEFINED)
        return fallback;
    size_t result = std::min<size_t>(std::max<int32_t>(position, 0), input.text.size());
    while(result > 0 && result < input.text.size() &&
          (input.text[result] & 0b11000000) == 0b10000000)
        --result;
    return result;
}

bool hasExactlyOneCharacter(const std::string& text) {
    if(text.empty())
        return false;

    const auto lead = static_cast<unsigned char>(text.front());
    size_t characterLength;
    if((lead & 0x80) == 0)
        characterLength = 1;
    else if((lead & 0xe0) == 0xc0)
        characterLength = 2;
    else if((lead & 0xf0) == 0xe0)
        characterLength = 3;
    else if((lead & 0xf8) == 0xf0)
        characterLength = 4;
    else
        return false;

    if(text.size() != characterLength)
        return false;
    for(size_t i = 1; i < characterLength; ++i) {
        if((static_cast<unsigned char>(text[i]) & 0xc0) != 0x80)
            return false;
    }
    return true;
}

bool hasSlashAndExactlyOneCharacter(const std::string& text) {
    return text.size() > 1 && text.front() == '/' &&
           hasExactlyOneCharacter(text.substr(1));
}

void getState(GameTextInput* input, GameTextInputGetStateCallback callback, void* context) {
    if(!input || !callback)
        return;

    std::string text;
    GameTextInputSpan selection;
    GameTextInputSpan composing;
    {
        std::lock_guard<std::recursive_mutex> lock(input->mutex);
        logState("getState", input);
        text = input->text;
        selection = {
            static_cast<int32_t>(input->selectionAnchor),
            static_cast<int32_t>(input->cursor),
        };
        composing = input->composing;
    }
    GameTextInputState state{text.c_str(), static_cast<int32_t>(text.size()), selection, composing};
    callback(context, &state);
}

void setState(GameTextInput* input, const GameTextInputState* state) {
    if(!input || !state)
        return;

    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    size_t length = 0;
    if(state->text_UTF8) {
        length = state->text_length > 0
                     ? static_cast<size_t>(state->text_length)
                     : std::strlen(state->text_UTF8);
        // GameTextInputState documents text_length without the terminator, but
        // some Bedrock builds pass the backing-buffer size instead. Keeping
        // that terminator inside std::string makes subsequently typed text
        // invisible because it is inserted after the first NUL.
        while(length > 0 && state->text_UTF8[length - 1] == '\0')
            --length;
    }

    // MOJANG TEXT INPUT WORKAROUND START
    // Remove this block once the game reliably processes the first text event
    // after replacing the editor contents.
    input->repeatNextTextEvent = true;
    // MOJANG TEXT INPUT WORKAROUND END

    input->text.assign(state->text_UTF8 ? state->text_UTF8 : "", length);
    const size_t suppliedSelectionAnchor =
        clampPosition(*input, state->selection.start, input->text.size());
    const size_t suppliedCursor =
        clampPosition(*input, state->selection.end, suppliedSelectionAnchor);
    const bool applyInitialCaretWorkaround =
        !input->showKeyboardCalled &&
        (hasExactlyOneCharacter(input->text) ||
         hasSlashAndExactlyOneCharacter(input->text)) &&
        (suppliedSelectionAnchor != input->text.size() ||
         suppliedCursor != input->text.size());
    if(applyInitialCaretWorkaround) {
        input->selectionAnchor = input->text.size();
        input->cursor = input->text.size();
        input->repeatNextTextEvent = true;
        FAKE_TEXT_INPUT_DEBUG("forcing initial short-text caret to end=%zu",
                              input->text.size());
    } else {
        input->selectionAnchor = suppliedSelectionAnchor;
        input->cursor = suppliedCursor;
    }
    if(state->composingRegion.start == SPAN_UNDEFINED || state->composingRegion.end == SPAN_UNDEFINED) {
        input->composing = {SPAN_UNDEFINED, SPAN_UNDEFINED};
    } else {
        input->composing.start =
            static_cast<int32_t>(clampPosition(*input, state->composingRegion.start, 0));
        input->composing.end =
            static_cast<int32_t>(clampPosition(*input, state->composingRegion.end, input->composing.start));
    }
    FAKE_TEXT_INPUT_DEBUG(
        "setState suppliedLength=%d normalizedLength=%zu suppliedSelection=[%d,%d] "
        "suppliedComposing=[%d,%d]",
        state->text_length, length, state->selection.start, state->selection.end,
        state->composingRegion.start, state->composingRegion.end);
    logState("setState result", input);
    if(applyInitialCaretWorkaround)
        FakeTextInput::flushState(input);
}

void dispatchStateChanged(GameTextInput* input, const GameTextInputState& state,
                          const char* delivery) {
    FAKE_TEXT_INPUT_INFO(
        "dispatch event=%zu delivery=%s length=%d selection=[%d,%d]",
        input->eventSerial, delivery, state.text_length, state.selection.start,
        state.selection.end);
    if(input->activity && input->activityCallback)
        input->activityCallback(input->activity, &state);
    if(input->eventCallback)
        input->eventCallback(input->eventContext, &state);
}

void showKeyboard(GameTextInput* input) {
    if(!input)
        return;
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    input->showKeyboardCalled = true;
    if(!input->enabled) {
        input->enabled = true;
        input->enabledNo++;
        input->repeatNextTextEvent = true;
    }
    logState("showKeyboard", input);
}

void hideKeyboard(GameTextInput* input) {
    if(!input)
        return;
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    input->showKeyboardCalled = false;
    if(input->enabled) {
        input->enabled = false;
        input->enabledNo++;
    }
    logState("hideKeyboard", input);
}

void restartKeyboard(GameTextInput* input) {
    if(!input)
        return;
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    input->enabledNo++;
    input->repeatNextTextEvent = true;
    logState("restartKeyboard", input);
}

void onTextInput(GameTextInput* input, const std::string& value) {
    if(!input) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    if(!input->enabled) {
        FAKE_TEXT_INPUT_DEBUG("onTextInput ignored while disabled input=%p bytes=\"%s\"",
                              (void*)input, escapeText(value).c_str());
        return;
    }
    FAKE_TEXT_INPUT_DEBUG("onTextInput input=%p bytes=\"%s\"",
                          (void*)input, escapeText(value).c_str());
    logState("onTextInput before", input);

    if(input->cursor != input->selectionAnchor) {
        const size_t selectionStart = std::min(input->cursor, input->selectionAnchor);
        const size_t selectionEnd = std::max(input->cursor, input->selectionAnchor);
        input->text.erase(selectionStart, selectionEnd - selectionStart);
        input->cursor = input->selectionAnchor = selectionStart;
        if(value.size() == 1 && (value[0] == 8 || value[0] == 127)) {
            FakeTextInput::flushState(input);
            return;
        }
    }

    if(value.size() == 1 && value[0] == 8) {
        if(input->cursor == 0)
            return;
        size_t deleteStart = input->cursor - 1;
        while(deleteStart > 0 && (input->text[deleteStart] & 0b11000000) == 0b10000000)
            --deleteStart;
        input->text.erase(deleteStart, input->cursor - deleteStart);
        input->cursor = input->selectionAnchor = deleteStart;
    } else if(value.size() == 1 && value[0] == 127) {
        if(input->cursor >= input->text.size())
            return;
        size_t deleteEnd = input->cursor + 1;
        while(deleteEnd < input->text.size() &&
              (input->text[deleteEnd] & 0b11000000) == 0b10000000)
            ++deleteEnd;
        input->text.erase(input->cursor, deleteEnd - input->cursor);
        input->selectionAnchor = input->cursor;
    } else {
        input->text.insert(input->cursor, value);
        input->cursor += value.size();
        input->selectionAnchor = input->cursor;
    }
    FakeTextInput::flushState(input);
}

size_t previousCharacter(const std::string& text, size_t position) {
    if(position == 0)
        return 0;
    --position;
    while(position > 0 && (text[position] & 0b11000000) == 0b10000000)
        --position;
    return position;
}

size_t nextCharacter(const std::string& text, size_t position) {
    if(position >= text.size())
        return text.size();
    ++position;
    while(position < text.size() && (text[position] & 0b11000000) == 0b10000000)
        ++position;
    return position;
}

bool isWordSeparator(const std::string& text, size_t position) {
    static constexpr char separators[] = " -_#/\\!@$%^&*();:'\"?.,";
    if(position >= text.size())
        return false;
    const auto ch = static_cast<unsigned char>(text[position]);
    return ch < 0x80 && std::strchr(separators, ch) != nullptr;
}

void onKeyPressed(GameTextInput* input, KeyCode key, KeyAction action, int mods) {
    if(!input || (action != KeyAction::PRESS && action != KeyAction::REPEAT))
        return;

    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    if(!input->enabled)
        return;

    const bool shift = (mods & KEY_MOD_SHIFT) != 0;
    const bool alt = (mods & KEY_MOD_ALT) != 0;
    const size_t oldCursor = input->cursor;
    const size_t oldAnchor = input->selectionAnchor;

    if(key == KeyCode::RIGHT) {
        if(!shift && input->cursor != input->selectionAnchor) {
            input->cursor = std::max(input->cursor, input->selectionAnchor);
        } else if(alt) {
            while(input->cursor < input->text.size() &&
                  isWordSeparator(input->text, input->cursor))
                input->cursor = nextCharacter(input->text, input->cursor);
            while(input->cursor < input->text.size() &&
                  !isWordSeparator(input->text, input->cursor))
                input->cursor = nextCharacter(input->text, input->cursor);
        } else {
            input->cursor = nextCharacter(input->text, input->cursor);
        }
    } else if(key == KeyCode::LEFT) {
        if(!shift && input->cursor != input->selectionAnchor) {
            input->cursor = std::min(input->cursor, input->selectionAnchor);
        } else if(alt) {
            while(input->cursor > 0 &&
                  isWordSeparator(input->text, previousCharacter(input->text, input->cursor)))
                input->cursor = previousCharacter(input->text, input->cursor);
            while(input->cursor > 0 &&
                  !isWordSeparator(input->text, previousCharacter(input->text, input->cursor)))
                input->cursor = previousCharacter(input->text, input->cursor);
        } else {
            input->cursor = previousCharacter(input->text, input->cursor);
        }
    } else if(key == KeyCode::HOME) {
        input->cursor = 0;
    } else if(key == KeyCode::END) {
        input->cursor = input->text.size();
    } else {
        return;
    }

    if(!shift)
        input->selectionAnchor = input->cursor;
    if(input->cursor != oldCursor || input->selectionAnchor != oldAnchor) {
        FAKE_TEXT_INPUT_DEBUG("caret key=%d shift=%d alt=%d", static_cast<int>(key),
                              shift, alt);
        FakeTextInput::flushState(input);
    }
}

}  // namespace

void FakeTextInput::registerActivity(GameActivity* activity, EventCallback callback) {
    std::unique_ptr<GameTextInput> input;
    std::lock_guard<std::mutex> lock(inputsMutex);
    const bool adoptedStandalone = standaloneInput != nullptr;
    if(standaloneInput) {
        input.reset(standaloneInput);
        standaloneInput = nullptr;
    } else {
        input = std::make_unique<GameTextInput>();
    }
    input->activity = activity;
    input->activityCallback = std::move(callback);
    FAKE_TEXT_INPUT_DEBUG("registerActivity activity=%p input=%p adoptedStandalone=%d",
                          (void*)activity, (void*)input.get(), adoptedStandalone);
    activityInputs[activity] = std::move(input);
}

void FakeTextInput::unregisterActivity(GameActivity* activity) {
    std::lock_guard<std::mutex> lock(inputsMutex);
    auto entry = activityInputs.find(activity);
    FAKE_TEXT_INPUT_DEBUG("unregisterActivity activity=%p input=%p", (void*)activity,
                          entry == activityInputs.end() ? nullptr : (void*)entry->second.get());
    activityInputs.erase(activity);
}

bool FakeTextInput::isEnabled(GameActivity* activity) {
    auto* input = findInput(activity);
    if(!input)
        return false;
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    return input->enabled;
}

size_t FakeTextInput::getEnabledNo(GameActivity* activity) {
    auto* input = findInput(activity);
    if(!input)
        return 0;
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    return input->enabledNo;
}

bool FakeTextInput::isMultiline(GameActivity* activity) {
    auto* input = findInput(activity);
    if(!input)
        return false;
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    return input->multiline;
}

bool FakeTextInput::consumesKey(GameActivity* activity, KeyCode key, int mods) {
    auto* input = findInput(activity);
    if(!input)
        return false;
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    if(!input->enabled)
        return false;

    if(key == KeyCode::LEFT || key == KeyCode::RIGHT || key == KeyCode::HOME ||
       key == KeyCode::END)
        return true;

    const bool control = mods & (KEY_MOD_CTRL | KEY_MOD_SUPER);
    if(control && key != KeyCode::V)
        return false;

    const auto value = static_cast<int>(key);
    const bool printable =
        key == KeyCode::SPACE ||
        (value >= static_cast<int>(KeyCode::NUM_0) && value <= static_cast<int>(KeyCode::NUM_9)) ||
        (value >= static_cast<int>(KeyCode::A) && value <= static_cast<int>(KeyCode::Z)) ||
        (value >= static_cast<int>(KeyCode::NUMPAD_0) && value <= static_cast<int>(KeyCode::NUMPAD_DIVIDE)) ||
        (value >= static_cast<int>(KeyCode::SEMICOLON) && value <= static_cast<int>(KeyCode::APOSTROPHE));
    return printable || key == KeyCode::BACKSPACE || key == KeyCode::DELETE ||
           key == KeyCode::ENTER;
}

void FakeTextInput::onTextInput(GameActivity* activity, const std::string& text) {
    ::onTextInput(findInput(activity), text);
}

void FakeTextInput::onKeyPressed(GameActivity* activity, KeyCode key, KeyAction action, int mods) {
    ::onKeyPressed(findInput(activity), key, action, mods);
}

// HACK: workaround first key input not registering until second key pressed
void FakeTextInput::tryFlushRepeatingState(GameActivity* activity) {
    auto* input = findInput(activity);
    if(!input) {
        return;
    }

    if(!input->repeatNextTextEvent) {
        return;
    }
    input->repeatNextTextEvent = false;
    FakeTextInput::flushState(input);
}

void FakeTextInput::flushState(GameTextInput* input) {
    std::lock_guard<std::recursive_mutex> lock(input->mutex);

    GameTextInputState state{
        input->text.c_str(),
        static_cast<int32_t>(input->text.size()),
        {
            static_cast<int32_t>(input->selectionAnchor),
            static_cast<int32_t>(input->cursor),
        },
        input->composing,
    };
    dispatchStateChanged(input, state, "repeat-next-poll");
}

std::string FakeTextInput::getCopyText(GameActivity* activity) {
    auto* input = findInput(activity);
    if(!input)
        return {};
    std::lock_guard<std::recursive_mutex> lock(input->mutex);
    if(!input->enabled)
        return {};
    if(input->cursor == input->selectionAnchor)
        return input->text;
    const size_t start = std::min(input->cursor, input->selectionAnchor);
    const size_t end = std::max(input->cursor, input->selectionAnchor);
    return input->text.substr(start, end - start);
}

void FakeTextInput::initHooks(std::vector<mcpelauncher_hook_t>& hooks) {
    hooks.push_back({"GameActivity_showSoftInput", (void*)+[](GameActivity* activity, uint32_t flags) {
                         FAKE_TEXT_INPUT_DEBUG("GameActivity_showSoftInput activity=%p flags=%u",
                                               (void*)activity, flags);
                         showKeyboard(findInput(activity));
                     }});
    hooks.push_back({"GameActivity_hideSoftInput", (void*)+[](GameActivity* activity, uint32_t flags) {
                         FAKE_TEXT_INPUT_DEBUG("GameActivity_hideSoftInput activity=%p flags=%u",
                                               (void*)activity, flags);
                         hideKeyboard(findInput(activity));
                     }});
    hooks.push_back({"GameActivity_restartInput", (void*)+[](GameActivity* activity) {
                         FAKE_TEXT_INPUT_DEBUG("GameActivity_restartInput activity=%p",
                                               (void*)activity);
                         restartKeyboard(findInput(activity));
                     }});
    hooks.push_back({"GameActivity_setTextInputState",
                     (void*)+[](GameActivity* activity, const GameTextInputState* state) {
                         auto* input = findInput(activity);
                         FAKE_TEXT_INPUT_DEBUG(
                             "GameActivity_setTextInputState activity=%p", (void*)activity);
                         logStateObject("GameActivity_setTextInputState supplied", input, state);
                         setState(input, state);
                     }});
    hooks.push_back({"GameActivity_getTextInputState",
                     (void*)+[](GameActivity* activity, GameTextInputGetStateCallback callback, void* context) {
                         FAKE_TEXT_INPUT_INFO(
                             "game requested state activity=%p input=%p callback=%p context=%p",
                             (void*)activity, (void*)findInput(activity),
                             (void*)callback, context);
                         getState(findInput(activity), callback, context);
                         FAKE_TEXT_INPUT_INFO("game state callback completed");
                     }});
    hooks.push_back({"GameActivity_getTextInput",
                     (void*)+[](const GameActivity* activity) -> GameTextInput* {
                         auto* input = findInput(const_cast<GameActivity*>(activity));
                         FAKE_TEXT_INPUT_DEBUG("GameActivity_getTextInput activity=%p input=%p",
                                               (const void*)activity, (void*)input);
                         return input;
                     }});
    hooks.push_back({"GameActivity_isSoftwareKeyboardVisible", (void*)+[](GameActivity* activity) -> bool {
                         return FakeTextInput::isEnabled(activity);
                     }});
    hooks.push_back(
        {"GameActivity_setImeEditorInfo",
         (void*)+[](GameActivity* activity, GameTextInputType inputType, GameTextInputActionType,
                    GameTextInputImeOptions) {
             if(auto* input = findInput(activity)) {
                 std::lock_guard<std::recursive_mutex> lock(input->mutex);
                 input->multiline = (static_cast<uint32_t>(inputType) & TYPE_TEXT_FLAG_MULTI_LINE) != 0;
                 FAKE_TEXT_INPUT_DEBUG(
                     "GameActivity_setImeEditorInfo activity=%p input=%p inputType=0x%x multiline=%d",
                     (void*)activity, (void*)input, static_cast<unsigned int>(inputType),
                     input->multiline);
             }
         }});

    hooks.push_back({"GameTextInput_init", (void*)+[](JNIEnv*, uint32_t) -> GameTextInput* {
                         std::lock_guard<std::mutex> lock(inputsMutex);
                         if(activityInputs.size() == 1) {
                             auto* input = activityInputs.begin()->second.get();
                             FAKE_TEXT_INPUT_DEBUG("GameTextInput_init returning activity input=%p",
                                                   (void*)input);
                             return input;
                         }
                         if(!standaloneInput)
                             standaloneInput = new GameTextInput;
                         FAKE_TEXT_INPUT_DEBUG("GameTextInput_init returning standalone input=%p",
                                               (void*)standaloneInput);
                         return standaloneInput;
                     }});
    hooks.push_back({"GameTextInput_setInputConnection", (void*)+[](GameTextInput* input, jobject connection) {
                         FAKE_TEXT_INPUT_DEBUG(
                             "GameTextInput_setInputConnection input=%p connection=%p ignored",
                             (void*)input, (void*)connection);
                     }});
    hooks.push_back({"GameTextInput_processEvent", (void*)+[](GameTextInput* input, jobject eventState) {
                         FAKE_TEXT_INPUT_DEBUG(
                             "GameTextInput_processEvent input=%p eventState=%p ignored",
                             (void*)input, (void*)eventState);
                     }});
    hooks.push_back({"GameTextInput_destroy", (void*)+[](GameTextInput* input) {
                         std::lock_guard<std::mutex> lock(inputsMutex);
                         if(input == standaloneInput) {
                             delete standaloneInput;
                             standaloneInput = nullptr;
                         }
                     }});
    hooks.push_back({"GameTextInput_showIme", (void*)+[](GameTextInput* input, uint32_t) {
                         showKeyboard(input);
                     }});
    hooks.push_back({"GameTextInput_hideIme", (void*)+[](GameTextInput* input, uint32_t) {
                         hideKeyboard(input);
                     }});
    hooks.push_back({"GameTextInput_restartInput", (void*)+[](GameTextInput* input) {
                         restartKeyboard(input);
                     }});
    hooks.push_back(
        {"GameTextInput_getState",
         (void*)+[](GameTextInput* input, GameTextInputGetStateCallback callback, void* context) {
             FAKE_TEXT_INPUT_DEBUG("GameTextInput_getState input=%p", (void*)input);
             getState(input, callback, context);
         }});
    hooks.push_back({"GameTextInput_setState", (void*)+[](GameTextInput* input, const GameTextInputState* state) {
                         logStateObject("GameTextInput_setState supplied", input, state);
                         setState(input, state);
                     }});
    hooks.push_back(
        {"GameTextInput_setEventCallback",
         (void*)+[](GameTextInput* input, GameTextInputEventCallback callback, void* context) {
             FAKE_TEXT_INPUT_DEBUG(
                 "GameTextInput_setEventCallback input=%p callback=%p context=%p",
                 (void*)input, (void*)callback, context);
             if(input) {
                 std::lock_guard<std::recursive_mutex> lock(input->mutex);
                 input->eventCallback = callback;
                 input->eventContext = context;
             }
         }});
    hooks.push_back(
        {"GameTextInput_setImeInsetsCallback",
         (void*)+[](GameTextInput* input, GameTextInputImeInsetsCallback callback, void* context) {
             if(input) {
                 std::lock_guard<std::recursive_mutex> lock(input->mutex);
                 input->insetsCallback = callback;
                 input->insetsContext = context;
             }
         }});
    hooks.push_back({"GameTextInput_getImeInsets", (void*)+[](const GameTextInput* input, ARect* insets) {
                         if(input && insets) {
                             std::lock_guard<std::recursive_mutex> lock(input->mutex);
                             *insets = input->insets;
                         }
                     }});
    hooks.push_back({"GameTextInput_processImeInsets", (void*)+[](GameTextInput* input, const ARect* insets) {
                         if(!input || !insets)
                             return;
                         std::lock_guard<std::recursive_mutex> lock(input->mutex);
                         input->insets = *insets;
                         if(input->insetsCallback)
                             input->insetsCallback(input->insetsContext, &input->insets);
                     }});
    hooks.push_back({"GameTextInputState_toJava",
                     (void*)+[](const GameTextInput* input, const GameTextInputState* state) -> jobject {
                         logStateObject("GameTextInputState_toJava supplied", input, state);
                         FAKE_TEXT_INPUT_DEBUG("GameTextInputState_toJava unsupported");
                         return nullptr;
                     }});
    hooks.push_back(
        {"GameTextInputState_fromJava",
         (void*)+[](const GameTextInput* input, jobject state, GameTextInputGetStateCallback callback, void* context) {
             FAKE_TEXT_INPUT_DEBUG(
                 "GameTextInputState_fromJava input=%p state=%p callback=%p context=%p; returning current native state",
                 (const void*)input, (void*)state, (void*)callback, context);
             getState(const_cast<GameTextInput*>(input), callback, context);
         }});
}
