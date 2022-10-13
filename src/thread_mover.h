#pragma once

#include <atomic>
#include <future>
#include <unordered_map>

class ThreadMover {
    private:
    static ThreadMover instance;

    std::atomic_bool main_thread_started = false;

    struct main_thread_info {
        void *(*main_thread_fn)(void *);
        void *main_thread_arg;
    };

    std::promise<main_thread_info> main_thread_promise;

    ThreadMover() = default;

    public:
    static void hookLibC(std::unordered_map<std::string, void *> &syms);

    static void executeMainThread();
};
