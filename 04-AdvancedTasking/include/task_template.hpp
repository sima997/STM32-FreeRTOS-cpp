#pragma once

class Task {
public :
    virtual void run() = 0;

    virtual void init() = 0;

    static void taskEntry(void* pvParams) {
        Task* t = static_cast<Task*>(pvParams);
        t->init();
        t->run();
    }
};