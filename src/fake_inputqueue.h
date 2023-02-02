#pragma once

#include <android/input.h>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>

struct FakeInputEvent {
    int32_t source, type;
    int32_t deviceId = 0;

    FakeInputEvent(int32_t source, int32_t type, int32_t deviceId = 0) : source(source), type(type), deviceId(deviceId) {}
};

struct FakeKeyEvent : FakeInputEvent {
    int32_t action, keyCode;

    FakeKeyEvent(int32_t action, int32_t keyCode) : FakeInputEvent(AINPUT_SOURCE_KEYBOARD, AINPUT_EVENT_TYPE_KEY), action(action), keyCode(keyCode) {}
    FakeKeyEvent(int32_t source, int32_t deviceId, int32_t action, int32_t keyCode) : FakeInputEvent(source, AINPUT_EVENT_TYPE_KEY, deviceId), action(action), keyCode(keyCode) {}
};

struct GamepadData {
    float axis[8];
    bool button[16];

    GamepadData();
};

struct FakeMotionEvent : FakeInputEvent {
    int32_t action;
    int32_t pointerId;
    float x, y;
    GamepadData data;

    FakeMotionEvent(int32_t action, int32_t pointerId, float x, float y) : FakeInputEvent(AINPUT_SOURCE_TOUCHSCREEN, AINPUT_EVENT_TYPE_MOTION), action(action), pointerId(pointerId), x(x), y(y) {}
    FakeMotionEvent(int32_t source, int32_t deviceId, int32_t action, int32_t pointerId, float x, float y, GamepadData data) : FakeInputEvent(source, AINPUT_EVENT_TYPE_MOTION, deviceId), action(action), pointerId(pointerId), x(x), y(y), data(data) {}
};

class FakeInputQueue {
private:
    struct EventStorage {
        EventStorage() {
            type = Type::EMPTY;
        }
        EventStorage(FakeKeyEvent&& keyEvent) {
            type = Type::KEY;
            new (&Storage) FakeKeyEvent(std::move(keyEvent));
        }
        EventStorage(FakeMotionEvent&& motionEvent) {
            type = Type::MOTION;
            new (&Storage) FakeMotionEvent(std::move(motionEvent));
        }
        EventStorage(EventStorage&& storage) {
            type = storage.type;
            if(type == Type::MOTION) {
                new (&Storage) FakeMotionEvent(std::move(*(FakeMotionEvent*)storage.Storage));
            } else if(type == Type::KEY) {
                new (&Storage) FakeKeyEvent(std::move(*(FakeKeyEvent*)storage.Storage));
            }
        }
        enum class Type {
            EMPTY,
            MOTION,
            KEY,
        } type;
        union EventUnion
        {
            FakeKeyEvent keyEvent;
            FakeMotionEvent motionEvent;
        };
        char Storage[sizeof(EventUnion)];
        ~EventStorage() {
            if(type == Type::MOTION) {
                ((EventUnion*)Storage)->motionEvent.~FakeMotionEvent();
            } else if(type == Type::KEY) {
                ((EventUnion*)Storage)->keyEvent.~FakeKeyEvent();
            }
        }
    };
    std::mutex sync;

    std::deque<EventStorage> events;

public:
    FakeInputQueue() {
        events.resize(50);
        events.clear();
    }
    static void initHybrisHooks(std::unordered_map<std::string, void *> &syms);

    bool hasEvents() {
        std::lock_guard<std::mutex> lock(sync);
        return !events.empty();
    }

    int getEvent(FakeInputEvent **event);

    void finishEvent(FakeInputEvent *event);

    void addEvent(FakeKeyEvent event);

    void addEvent(FakeMotionEvent event);
};
