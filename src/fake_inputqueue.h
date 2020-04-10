#pragma once

#include <android/input.h>
#include <deque>

struct FakeInputEvent {
    int32_t type;

    FakeInputEvent(int32_t type) : type(type) {}
};

struct FakeKeyEvent : FakeInputEvent {
    int32_t action, keyCode;

    FakeKeyEvent(int32_t action, int32_t keyCode) : FakeInputEvent(AINPUT_EVENT_TYPE_KEY), action(action), keyCode(keyCode) {}
};

class FakeInputQueue {

private:
    std::deque<FakeKeyEvent> keyEvents;

public:
    static void initHybrisHooks();

    bool hasEvents() const { return !keyEvents.empty(); }

    int getEvent(FakeInputEvent **event);

    void finishEvent(FakeInputEvent *event);


    void addEvent(FakeKeyEvent event);

};