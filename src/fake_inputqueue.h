#pragma once

#include <android/input.h>
#include <deque>

struct FakeInputEvent {
    int32_t source, type;

    FakeInputEvent(int32_t source, int32_t type) : source(source), type(type) {}
};

struct FakeKeyEvent : FakeInputEvent {
    int32_t action, keyCode;

    FakeKeyEvent(int32_t action, int32_t keyCode) : FakeInputEvent(AINPUT_SOURCE_KEYBOARD, AINPUT_EVENT_TYPE_KEY), action(action), keyCode(keyCode) {}
};

struct FakeMotionEvent : FakeInputEvent {
    int32_t action;
    int32_t pointerId;
    float x, y;

    FakeMotionEvent(int32_t action, int32_t pointerId, float x, float y) : FakeInputEvent(AINPUT_SOURCE_TOUCHSCREEN, AINPUT_EVENT_TYPE_MOTION), action(action), pointerId(pointerId), x(x), y(y) {}
};

class FakeInputQueue {

private:
    std::deque<FakeKeyEvent> keyEvents;
    std::deque<FakeMotionEvent> motionEvents;

public:
    static void initHybrisHooks();

    bool hasEvents() const { return !keyEvents.empty() || !motionEvents.empty(); }

    int getEvent(FakeInputEvent **event);

    void finishEvent(FakeInputEvent *event);


    void addEvent(FakeKeyEvent event);

    void addEvent(FakeMotionEvent event);

};