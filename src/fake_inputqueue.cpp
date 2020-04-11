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
}


int FakeInputQueue::getEvent(FakeInputEvent **event) {
    if (keyEvents.empty())
        return -1;
    *event = &keyEvents.front();
    return 0;
}

void FakeInputQueue::finishEvent(FakeInputEvent *event) {
    if (&keyEvents.front() != event)
        throw std::runtime_error("finishEvent: the event is not the event on the front of queue");
    keyEvents.pop_front();
}

void FakeInputQueue::addEvent(FakeKeyEvent event) {
    keyEvents.push_back(event);
}