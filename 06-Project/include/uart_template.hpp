#pragma once

#include <cstdint>
#include "stm32g431xx.h"
#include "gpio_template.hpp"

// Interrupt types supported
enum class UartIRQ {
    RXFNE,  // RX not empty
    IDLE,   // Idle line detected
    TC      // Transmission complete
};

// Pin mapping for USART instances
template<int Instance>
struct UartPins;

// USART1 (PA9=TX, PA10=RX)
template<> struct UartPins<1> {
    using Tx = Gpio<GpioPort::A, 9, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<GpioPort::A, 10, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;

    static USART_TypeDef* uart() { return USART1; }
    static void enableClock() { RCC->APB2ENR |= RCC_APB2ENR_USART1EN; }
};

// USART2 (PA2=TX, PA3=RX)
template<> struct UartPins<2> {
    using Tx = Gpio<GpioPort::A, 2, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<GpioPort::A, 3, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;

    static USART_TypeDef* uart() { return USART2; }
    static void enableClock() { RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; }
};

// USART3 (PB10=TX, PB11=RX)
template<> struct UartPins<3> {
    using Tx = Gpio<GpioPort::B, 10, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<GpioPort::B, 11, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;

    static USART_TypeDef* uart() { return USART3; }
    static void enableClock() { RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN; }
};

// -------------------------------------------------
// UART template
// -------------------------------------------------
template<int Instance, int Baudrate>
struct Uart {
    static inline USART_TypeDef* U() { return UartPins<Instance>::uart(); }

    // Initialize UART
    static void init(uint32_t apb_clk_hz) {
        UartPins<Instance>::Tx::init();
        UartPins<Instance>::Rx::init();
        UartPins<Instance>::enableClock();

        // Reset control registers
        U()->CR1 = 0;
        U()->CR2 = 0;
        U()->CR3 = 0;

        // Set baud rate
        U()->BRR = apb_clk_hz / Baudrate;

        // 8N1, enable TX/RX, UE
        U()->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    }

    // Send single char (blocking)
    static void sendChar(char c) {
        while (!(U()->ISR & USART_ISR_TXE_TXFNF));
        U()->TDR = c;
    }

    // Send C-string
    static void send(const char* str) {
        while (*str) sendChar(*str++);
    }
    // Send int as ASCI
    static void sendInt(int16_t val) {
        char buffer[12];
        intToStr(val, buffer);
        send(buffer);
    }

    static void intToStr(int16_t value, char* buffer) {
        char tmp[12];
        int16_t i = 0;
        bool negative = false;

        if(value < 0) {
            negative = true;
            value = -value;
        }

        do {
            tmp[i++] = '0' + (value % 10);
            value /= 10;
        } while (value > 0);

        int j = 0;
        if (negative) buffer[j++] = '-';

        while(i > 0) {
            buffer[j++] = tmp[--i];
        }
        buffer[j] = '\0';
    }

    // Enable IRQ (FreeRTOS-safe)
    static void enableIRQ(UartIRQ irq, uint8_t priority) {
        switch (irq) {
        case UartIRQ::RXFNE:
            U()->CR1 |= USART_CR1_RXNEIE;
            break;
        case UartIRQ::IDLE:
            U()->CR1 |= USART_CR1_IDLEIE;
            break;
        case UartIRQ::TC:
            U()->CR1 |= USART_CR1_TCIE;
            break;
        default: break;
        }

        // NVIC priority: shift according to __NVIC_PRIO_BITS
        int irq_num = 0;
        if constexpr (Instance == 1) irq_num = USART1_IRQn;
        else if constexpr (Instance == 2) irq_num = USART2_IRQn;
        else if constexpr (Instance == 3) irq_num = USART3_IRQn;

        NVIC_SetPriority((IRQn_Type)irq_num, priority);
        NVIC_EnableIRQ((IRQn_Type)irq_num);
    }
};
