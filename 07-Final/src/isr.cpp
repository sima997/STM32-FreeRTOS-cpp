#include "stm32g431xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// FreeRTOS queue for received command characters
extern QueueHandle_t command_queue;

/**
 * @brief USART2 interrupt service routine
 * 
 * Handles RX (receive) events. Reads the received character from the
 * USART2 data register and sends it to a FreeRTOS queue using the ISR-safe API.
 * 
 * This ISR assumes that the queue has been properly created and initialized.
 */
extern "C" void USART2_IRQHandler(void) {
    // Check if RXNE (Receive Not Empty) flag is set
    if (USART2->ISR & USART_ISR_RXNE) {
        // Read the received character (clears RXNE flag)
        char c = static_cast<char>(USART2->RDR & 0xFF);

        // Send the character to the queue in an ISR-safe manner
        if (command_queue != nullptr) {
            xQueueSendToBackFromISR(command_queue, &c, nullptr);
        }
    }
}
