#include "thread_mover.h"

ThreadMover ThreadMover::instance;

void ThreadMover::hookLibC(std::unordered_map<std::string, void *> &syms) {
    using pthread_create_fn = int (*)(void *, const void *, void *(*)(void *), void *);
    static pthread_create_fn pthread_create_orig = (pthread_create_fn)syms["pthread_create"];
    syms["pthread_create"] = (void *)+[](void *thread, const void *attr, void *(*start_routine)(void *), void *arg) {
        bool expected = false;
        if(ThreadMover::instance.main_thread_started.compare_exchange_strong(expected, true)) {
            ThreadMover::instance.main_thread_promise.set_value({start_routine, arg});
            return 0;
        }
        return pthread_create_orig(thread, attr, start_routine, arg);
    };
}

void ThreadMover::executeMainThread() {
    auto info = ThreadMover::instance.main_thread_promise.get_future().get();
    info.main_thread_fn(info.main_thread_arg);
}
