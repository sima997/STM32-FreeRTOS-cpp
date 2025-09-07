#include "FreeRTOS.h"
#include "queue.h"

template <typename T>
class Queue {
public:
    explicit Queue(size_t length) {
        q = xQueueCreate(length, sizeof(T));
    }

    bool send(const T& item, TickType_t wait = portMAX_DELAY) {
        return xQueueSend(q, &item, wait) == pdPASS;
    }

    bool send_from_ISR(const T& item, BaseType_t* higherPrioWoken = nullptr) {
        return xQueueSendFromISR(q, &item, higherPrioWoken) == pdPASS;
    }

    bool receive(T& item, TickType_t wait = portMAX_DELAY) {
        return xQueueReceive(q, &item, wait) == pdPASS;
    }

    bool receive_from_ISR(T& item, BaseType_t* higherPrioWoken = nullptr) {
        return xQueueReceiveFromISR(q, &item, higherPrioWoken) == pdPASS;
    }

    QueueHandle_t handle() const { return q; }

private:
    QueueHandle_t q;
};
