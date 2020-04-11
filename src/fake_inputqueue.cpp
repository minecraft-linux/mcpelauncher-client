#include "fake_inputqueue.h"

#include <hybris/hook.h>
#include <stdexcept>

void FakeInputQueue::initHybrisHooks() {
    hybris_hook("AInputQueue_getEvent", (void *) +[](AInputQueue *queue, AInputEvent **outEvent) {
        return ((FakeInputQueue *) (void *) queue)->getEvent((FakeInputEvent **) (void **) outEvent);
    });
    hybris_hook("AInputQueue_finishEvent", (void *) +[](AInputQueue *queue, AInputEvent *event, int handled) {
        ((FakeInputQueue *) (void *) queue)->finishEvent((FakeInputEvent *) (void *) event);
    });
    hybris_hook("AInputQueue_preDispatchEvent", (void *) +[]() {
        return 0;
    });
    hybris_hook("AInputEvent_getSource", (void *) +[](const AInputEvent *event) {
        return ((const FakeInputEvent *) (const void *) event)->source;
    });
    hybris_hook("AInputEvent_getType", (void *) +[](const AInputEvent *event) {
        return ((const FakeInputEvent *) (const void *) event)->type;
    });
    hybris_hook("AKeyEvent_getAction", (void *) +[](const AInputEvent *event) {
        return ((const FakeKeyEvent *) (const void *) event)->action;
    });
    hybris_hook("AKeyEvent_getKeyCode", (void *) +[](const AInputEvent *event) {
        return ((const FakeKeyEvent *) (const void *) event)->keyCode;
    });
    hybris_hook("AMotionEvent_getAction", (void *) +[](const AInputEvent *event) {
        return ((const FakeMotionEvent *) (const void *) event)->action;
    });
    hybris_hook("AMotionEvent_getPointerCount", (void *) +[](const AInputEvent *event) {
        return 1;
    });
    hybris_hook("AMotionEvent_getPointerId", (void *) +[](const AInputEvent *event) {
        return ((const FakeMotionEvent *) (const void *) event)->pointerId;
    });
    hybris_hook("AMotionEvent_getX", (void *) +[](const AInputEvent *event) {
        return ((const FakeMotionEvent *) (const void *) event)->x;
    });
    hybris_hook("AMotionEvent_getY", (void *) +[](const AInputEvent *event) {
        return ((const FakeMotionEvent *) (const void *) event)->y;
    });
    hybris_hook("AMotionEvent_getRawX", (void *) +[](const AInputEvent *event) {
        return ((const FakeMotionEvent *) (const void *) event)->x;
    });
    hybris_hook("AMotionEvent_getRawY", (void *) +[](const AInputEvent *event) {
        return ((const FakeMotionEvent *) (const void *) event)->y;
    });
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
    motionEvents.push_back(event);
}