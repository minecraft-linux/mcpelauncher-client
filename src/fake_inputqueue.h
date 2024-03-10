#pragma once

#include <android/input.h>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>

struct FakeInputEvent {
    int32_t source, type;
    int32_t deviceId = 0;

    FakeInputEvent(int32_t source, int32_t type, int32_t deviceId = 0) : source(source), type(type), deviceId(deviceId) {}
};

struct FakeKeyEvent : FakeInputEvent {
    int32_t action, keyCode, metaState;

    FakeKeyEvent(int32_t action, int32_t keyCode, int32_t metaState) : FakeInputEvent(AINPUT_SOURCE_KEYBOARD, AINPUT_EVENT_TYPE_KEY), action(action), keyCode(keyCode), metaState(metaState) {}
    FakeKeyEvent(int32_t source, int32_t deviceId, int32_t action, int32_t keyCode) : FakeInputEvent(source, AINPUT_EVENT_TYPE_KEY, deviceId), action(action), keyCode(keyCode), metaState(0) {}
    FakeKeyEvent() : FakeKeyEvent(0, 0, 0) {}
};

struct FakeMotionEvent : FakeInputEvent {
    int32_t action;
    int32_t pointerId;
    float x, y;
    std::function<float(int32_t axis)> axisFunction;
    int32_t btn = 0, dy = 0;

    FakeMotionEvent(int32_t source, int32_t action, int32_t pointerId, float x, float y) : FakeInputEvent(source, AINPUT_EVENT_TYPE_MOTION), action(action), pointerId(pointerId), x(x), y(y) {}

    FakeMotionEvent(int32_t source, int32_t action, int32_t pointerId, float x, float y, int32_t btn, int32_t dy) : FakeInputEvent(source, AINPUT_EVENT_TYPE_MOTION), action(action), pointerId(pointerId), x(x), y(y), btn(btn), dy(dy) {}

    FakeMotionEvent(int32_t source, int32_t deviceId, int32_t action, int32_t pointerId, float x, float y, std::function<float(int32_t axis)> axisFunction) : FakeInputEvent(source, AINPUT_EVENT_TYPE_MOTION, deviceId), action(action), pointerId(pointerId), x(x), y(y), axisFunction(std::move(axisFunction)) {}

    FakeMotionEvent() : FakeMotionEvent(0, 0, 0, 0, 0) {}
};

class FakeInputQueue {
private:
    std::deque<FakeKeyEvent> keyEvents;
    std::deque<FakeMotionEvent> motionEvents;

public:
    static void initHybrisHooks(std::unordered_map<std::string, void *> &syms);

    FakeInputQueue() {
        // Should avoid a crash caused by a lot of events from gamepad's analog sticks
        motionEvents.resize(100);
        motionEvents.clear();
        keyEvents.resize(100);
        keyEvents.clear();
    }

    bool hasEvents() const { return !keyEvents.empty() || !motionEvents.empty(); }

    int getEvent(FakeInputEvent **event);

    void finishEvent(FakeInputEvent *event);

    void addEvent(FakeKeyEvent event);

    void addEvent(FakeMotionEvent event);
};
