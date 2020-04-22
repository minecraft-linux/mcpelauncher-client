#include "fake_inputqueue.h"

#include <stdexcept>

void FakeInputQueue::initHybrisHooks(std::unordered_map<std::string, void*> &syms) {
    syms["AInputQueue_getEvent"] = (void *) +[](AInputQueue *queue, AInputEvent **outEvent) {
        return ((FakeInputQueue *) (void *) queue)->getEvent((FakeInputEvent **) (void **) outEvent);
    };
    syms["AInputQueue_finishEvent"] = (void *) +[](AInputQueue *queue, AInputEvent *event, int handled) {
        ((FakeInputQueue *) (void *) queue)->finishEvent((FakeInputEvent *) (void *) event);
    };
    syms["AInputQueue_preDispatchEvent"] = (void *) +[]() {
        return 0;
    };
    syms["AInputEvent_getSource"] = (void *) +[](const AInputEvent *event) {
        return ((const FakeInputEvent *) (const void *) event)->source;
    };
    syms["AInputEvent_getType"] = (void *) +[](const AInputEvent *event) {
        return ((const FakeInputEvent *) (const void *) event)->type;
    };
    syms["AInputEvent_getDeviceId"] = (void *) +[](const AInputEvent *event) {
        return ((const FakeInputEvent *) (const void *) event)->deviceId;
    };
    syms["AKeyEvent_getAction"] = (void *) +[](const AInputEvent *event) {
        return ((const FakeKeyEvent *) (const void *) event)->action;
    };
    syms["AKeyEvent_getKeyCode"] = (void *) +[](const AInputEvent *event) {
        return ((const FakeKeyEvent *) (const void *) event)->keyCode;
    };
    syms["AKeyEvent_getRepeatCount"] = (void *) +[](const AInputEvent *event) {
        return (int32_t) 0;
    };
    syms["AKeyEvent_getMetaState"] = (void *) +[](const AInputEvent *event) {
        return (int32_t) 0;
    };
    syms["AMotionEvent_getAction"] = (void *) +[](const AInputEvent *event) {
        return ((const FakeMotionEvent *) (const void *) event)->action;
    };
    syms["AMotionEvent_getPointerCount"] = (void *) +[](const AInputEvent *event) {
        return 1;
    };
    syms["AMotionEvent_getPointerId"] = (void *) +[](const AInputEvent *event) {
        return ((const FakeMotionEvent *) (const void *) event)->pointerId;
    };
    syms["AMotionEvent_getX"] = (void *) +[](const AInputEvent *event, size_t pointerIndex) {
        return ((const FakeMotionEvent *) (const void *) event)->x;
    };
    syms["AMotionEvent_getY"] = (void *) +[](const AInputEvent *event, size_t pointerIndex) {
        return ((const FakeMotionEvent *) (const void *) event)->y;
    };
    syms["AMotionEvent_getRawX"] = (void *) +[](const AInputEvent *event, size_t pointerIndex) {
        return ((const FakeMotionEvent *) (const void *) event)->x;
    };
    syms["AMotionEvent_getRawY"] = (void *) +[](const AInputEvent *event, size_t pointerIndex) {
        return ((const FakeMotionEvent *) (const void *) event)->y;
    };
    syms["AMotionEvent_getAxisValue"] = (void *) +[](const AInputEvent *event, int32_t axis, size_t pointerIndex) {
        return ((const FakeMotionEvent *) (const void *) event)->axisFunction(axis);
    };
}


int FakeInputQueue::getEvent(FakeInputEvent **event) {
    if (!keyEvents.empty()) {
        *event = &keyEvents.front();
        return 0;
    }
    if (!motionEvents.empty()) {
        *event = &motionEvents.front();
        return 0;
    }
    return -1;
}

void FakeInputQueue::finishEvent(FakeInputEvent *event) {
    if (&keyEvents.front() == event) {
        keyEvents.pop_front();
        return;
    }
    if (&motionEvents.front() == event) {
        motionEvents.pop_front();
        return;
    }
    throw std::runtime_error("finishEvent: the event is not the event on the front of queue");
}

void FakeInputQueue::addEvent(FakeKeyEvent event) {
    keyEvents.push_back(event);
}

void FakeInputQueue::addEvent(FakeMotionEvent event) {
    motionEvents.push_back(std::move(event));
}