#include "fake_inputqueue.h"

#include <stdexcept>
#include "armhfrewrite.h"
#include "game_window.h"

static float _AMotionEvent_getX(const AInputEvent *event, size_t pointerIndex) {
    return ((const FakeMotionEvent *)(const void *)event)->x;
}

static float _AMotionEvent_getY(const AInputEvent *event, size_t pointerIndex) {
    return ((const FakeMotionEvent *)(const void *)event)->y;
}

static float deadzone(float f) {
    return f < 0.02 && f > -0.02 ? 0.f : f;
}

static float _AMotionEvent_getAxisValue(const AInputEvent *event, int32_t axis, size_t pointerIndex) {
    auto && gp = ((const FakeMotionEvent *)(const void *)event)->data;
    if(axis == AMOTION_EVENT_AXIS_X)
        return deadzone(gp.axis[(int)GamepadAxisId::LEFT_X]);
    if(axis == AMOTION_EVENT_AXIS_Y)
        return deadzone(gp.axis[(int)GamepadAxisId::LEFT_Y]);
    if(axis == AMOTION_EVENT_AXIS_RX)
        return deadzone(gp.axis[(int)GamepadAxisId::RIGHT_X]);
    if(axis == AMOTION_EVENT_AXIS_RY)
        return deadzone(gp.axis[(int)GamepadAxisId::RIGHT_Y]);
    if(axis == AMOTION_EVENT_AXIS_BRAKE)
        return deadzone(gp.axis[(int)GamepadAxisId::LEFT_TRIGGER]);
    if(axis == AMOTION_EVENT_AXIS_GAS)
        return deadzone(gp.axis[(int)GamepadAxisId::RIGHT_TRIGGER]);
    if(axis == AMOTION_EVENT_AXIS_HAT_X) {
        if(gp.button[(int)GamepadButtonId::DPAD_LEFT])
            return -1.f;
        if(gp.button[(int)GamepadButtonId::DPAD_RIGHT])
            return 1.f;
        return 0.f;
    }
    if(axis == AMOTION_EVENT_AXIS_HAT_Y) {
        if(gp.button[(int)GamepadButtonId::DPAD_UP])
            return -1.f;
        if(gp.button[(int)GamepadButtonId::DPAD_DOWN])
            return 1.f;
        return 0.f;
    }

    return 0;
}

void FakeInputQueue::initHybrisHooks(std::unordered_map<std::string, void *> &syms) {
    syms["AInputQueue_getEvent"] = (void *)+[](AInputQueue *queue, AInputEvent **outEvent) {
        return ((FakeInputQueue *)(void *)queue)->getEvent((FakeInputEvent **)(void **)outEvent);
    };
    syms["AInputQueue_finishEvent"] = (void *)+[](AInputQueue *queue, AInputEvent *event, int handled) {
        ((FakeInputQueue *)(void *)queue)->finishEvent((FakeInputEvent *)(void *)event);
    };
    syms["AInputQueue_preDispatchEvent"] = (void *)+[]() {
        return 0;
    };
    syms["AInputEvent_getSource"] = (void *)+[](const AInputEvent *event) {
        return ((const FakeInputEvent *)(const void *)event)->source;
    };
    syms["AInputEvent_getType"] = (void *)+[](const AInputEvent *event) {
        return ((const FakeInputEvent *)(const void *)event)->type;
    };
    syms["AInputEvent_getDeviceId"] = (void *)+[](const AInputEvent *event) {
        return ((const FakeInputEvent *)(const void *)event)->deviceId;
    };
    syms["AKeyEvent_getAction"] = (void *)+[](const AInputEvent *event) {
        return ((const FakeKeyEvent *)(const void *)event)->action;
    };
    syms["AKeyEvent_getKeyCode"] = (void *)+[](const AInputEvent *event) {
        return ((const FakeKeyEvent *)(const void *)event)->keyCode;
    };
    syms["AKeyEvent_getRepeatCount"] = (void *)+[](const AInputEvent *event) {
        return (int32_t)0;
    };
    syms["AKeyEvent_getMetaState"] = (void *)+[](const AInputEvent *event) {
        return (int32_t)0;
    };
    syms["AMotionEvent_getAction"] = (void *)+[](const AInputEvent *event) {
        return ((const FakeMotionEvent *)(const void *)event)->action;
    };
    syms["AMotionEvent_getPointerCount"] = (void *)+[](const AInputEvent *event) {
        return 1;
    };
    syms["AMotionEvent_getButtonState"] = (void *)+[](const AInputEvent *event) {
        return 0;
    };
    syms["AMotionEvent_getPointerId"] = (void *)+[](const AInputEvent *event) {
        return ((const FakeMotionEvent *)(const void *)event)->pointerId;
    };
    syms["AMotionEvent_getX"] = reinterpret_cast<void *>(ARMHFREWRITE(_AMotionEvent_getX));
    syms["AMotionEvent_getY"] = reinterpret_cast<void *>(ARMHFREWRITE(_AMotionEvent_getY));
    syms["AMotionEvent_getRawX"] = reinterpret_cast<void *>(ARMHFREWRITE(_AMotionEvent_getX));
    syms["AMotionEvent_getRawY"] = reinterpret_cast<void *>(ARMHFREWRITE(_AMotionEvent_getY));
    syms["AMotionEvent_getAxisValue"] = reinterpret_cast<void *>(ARMHFREWRITE(_AMotionEvent_getAxisValue));
}

int FakeInputQueue::getEvent(FakeInputEvent **event) {
    std::lock_guard<std::mutex> lock(sync);
    if(!events.empty()) {
        *event = (FakeInputEvent *)events.front().Storage;
        return 0;
    }
    return -1;
}

void FakeInputQueue::finishEvent(FakeInputEvent *event) {
    std::lock_guard<std::mutex> lock(sync);
    if(!events.empty() && (FakeInputEvent *)events.front().Storage == event) {
        events.pop_front();
        return;
    }
    throw std::runtime_error("finishEvent: the event is not the event on the front of queue");
}

void FakeInputQueue::addEvent(FakeKeyEvent event) {
    std::lock_guard<std::mutex> lock(sync);
    events.emplace_back(std::move(event));
}

void FakeInputQueue::addEvent(FakeMotionEvent event) {
    std::lock_guard<std::mutex> lock(sync);
    events.emplace_back(std::move(event));
}
