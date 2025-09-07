#pragma once

class Task {
public :
    virtual void run() = 0;

    virtual void init() = 0;

    static void taskEntry(void* pvParams) {
        static_cast<Task*>(pvParams)->run();
    }
};