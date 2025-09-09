extern "C" {
    #include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
}

#include "uart_template.hpp"
#include "gpio_template.hpp"
#include <cstring>
#include "task_template.hpp"


//Using templates
using Uart2 = Uart<2, 115200>;
using Led = Gpio<GpioPort::A, 5, GpioMode::Output>;

//Global task handle
TaskHandle_t ProcessTaskHandle;

//UART buffer
char rx_buffer[64] = {};
uint8_t idx = 0;

// USART2 ISR
extern "C" void USART2_IRQHandler(void) {
    if(USART2->ISR & USART_ISR_RXNE) {
        BaseType_t xHigherPriorityTaskWoken;
        char c = (USART2->RDR & 0xFF);
        if (c == '\n' || c == '\r') {
            rx_buffer[idx] = '\0';
            idx = 0;
            if(ProcessTaskHandle != nullptr) vTaskNotifyGiveFromISR(ProcessTaskHandle, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        }else {
            rx_buffer[idx++] = c;
        }
    }
    
    
}

// Process task
template<typename LedPin, typename UartInstance>
class ProcessTask : public Task {
    public:
        ProcessTask(UartInstance& uart) : uart_(uart) {}

        void init() override {
            LedPin::init();
        }

        void run() override {
            uart_.send("Commands: LED ON, LED OFF\n");

            for (;;) {
                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                
                if (strcmp(rx_buffer, "LED ON") == 0) {
                    Led::set();
                    uart_.send("OK\n");
                } else if (strcmp(rx_buffer, "LED OFF") == 0) {
                    Led::clear();
                    uart_.send("OK\n");
                } else {
                    uart_.send("ERROR\n");
                }
                    
            }
        }


    private:
        UartInstance& uart_;
};



// Main
int main() {
    // UART init
    Uart2 uart2;
    uart2.init(128000000);

    
    // Enable RX interrupt with safe FreeRTOS priority
    uart2.enableIRQ(UartIRQ::RXFNE, 11);

    //Instantiate process task
    static ProcessTask<Led, Uart2> process(uart2);

    // Create task
    xTaskCreate(Task::taskEntry, "Process", 256, &process, 1, &ProcessTaskHandle);

    __enable_irq();

    vTaskStartScheduler();

    while (1) {}
}
