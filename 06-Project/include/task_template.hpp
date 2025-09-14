#pragma once
#include <cstdint>

enum class TaskError {
    None = 0,
    Adc,
    Uart,
    I2c,
    Queue
};

//Forward declaration of WatchdogSupervisor
class WatchdogSupervisor;

class Task {
public :
    Task(const char* name) : name_(name), aliveBit_(0) {}
    virtual void run() = 0;

    virtual void init() {};

    static void taskEntry(void* pvParams) {
        Task* t = static_cast<Task*>(pvParams);
        t->init();
        t->run();
    }

    void registerToWatchdog(uint32_t bit);

    void notifyAlive();

    uint32_t getAliveBit() const { return aliveBit_; }
private :
    const char* name_;
    uint32_t aliveBit_;
};