#include "fake_inputqueue.h"

#include <stdexcept>
#include "armhfrewrite.h"

static float _AMotionEvent_getX(const AInputEvent *event, size_t pointerIndex) {
    return ((const FakeMotionEvent *)(const void *)event)->x;
}

static float _AMotionEvent_getY(const AInputEvent *event, size_t pointerIndex) {
    return ((const FakeMotionEvent *)(const void *)event)->y;
}

static float _AMotionEvent_getAxisValue(const AInputEvent *event, int32_t axis, size_t pointerIndex) {
    return ((const FakeMotionEvent *)(const void *)event)->axisFunction(axis);
}

FakeInputQueue::FakeInputQueue() {
    keyEvents.resize(50);
    keyEvents.resize(0);
    motionEvents.resize(50);
    motionEvents.resize(0);
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
    if(!keyEvents.empty()) {
        currentEvent.keyEvent = std::move(keyEvents.front());
        *event = &currentEvent.keyEvent;
        keyEvents.pop_front();
        return 0;
    }
    if(!motionEvents.empty()) {
        currentEvent.motionEvent = std::move(motionEvents.front());
        *event = &currentEvent.motionEvent;
        motionEvents.pop_front();
        return 0;
    }
    return -1;
}

void FakeInputQueue::finishEvent(FakeInputEvent *event) {

}

void FakeInputQueue::addEvent(FakeKeyEvent event) {
    keyEvents.push_back(event);
}

void FakeInputQueue::addEvent(FakeMotionEvent event) {
    motionEvents.push_back(std::move(event));
}
