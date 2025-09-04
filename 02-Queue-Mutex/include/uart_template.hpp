#pragma once

#include <cstdint>
#include "stm32g431xx.h"
#include "gpio_template.hpp"

//Instance pins concetrated in template structure
//Generic declaration (no implementation here):
template<int Instrance>
struct UartPins;

// Specialization for Instance = 1:
template<> struct UartPins<1> {
    //Configure pins 
    using Tx = Gpio<'A', 9, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<'A', 9, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;

    //Return USART1 base
    static USART_TypeDef* uart() { return USART1; }

    //Enable clock for USART1
    static void enableClock() { RCC->APB2ENR |= RCC_APB2ENR_USART1EN; }
};

// Specialization for Instance = 2:
template<> struct UartPins<2> {
    // Configure pins for ST-Link VCP (PA2 = TX, PA3 = RX, AF7)
    using Tx = Gpio<'A', 2, GpioMode::Alt, GpioSpeed::VeryHigh,
                    GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<'A', 3, GpioMode::Alt, GpioSpeed::VeryHigh,
                    GpioPull::None, GpioAF::AF7>;

    // Return USART2 base
    static USART_TypeDef* uart() { return USART2; }

    // Enable clock for USART2
    static void enableClock() { RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; }
};
// Specialization for Instance = 3:
template<> struct UartPins<3> {
    //Configure pins 
    using Tx = Gpio<'B', 9, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<'B', 8, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;

    //Return USART1 base
    static USART_TypeDef* uart() { return USART3; }

    //Enable clock for USART3
    static void enableClock() { RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN; }
};


template<int Instance, int Baudrate>
struct Uart{
    static inline USART_TypeDef* U() {return UartPins<Instance>::uart();}

    static void init(uint32_t apb_clk_hz) {

        //Initialize pins (enable clocks)
        UartPins<Instance>::Tx::init();
        UartPins<Instance>::Rx::init();

        //Enable clock to USARTx
        UartPins<Instance>::enableClock();

        //Reset control registers
        U()->CR1 = 0;
        U()->CR2 = 0;
        U()->CR3 = 0;

        //Calculate and write baudrate 
        U()->BRR = apb_clk_hz / Baudrate;

        //8N1, no parity, oversampling by 16 (default), enable TX/RX, UE
        U()->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    }

    static void sendChar(char c) {
        while(!(U()->ISR & (USART_ISR_TXE_TXFNF)));
        U()->TDR = c;
    }

    static void send(const char* str) {
        while(*str) sendChar(*str++);
    }

    static void sendInt(int16_t val){
        char buffer[12];
        intToStr(val, buffer);
        send(buffer);
    }

        /**
     * Converts Integer to String
     */
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
};




