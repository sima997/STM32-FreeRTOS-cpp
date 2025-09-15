#pragma once

extern "C" {
    #include "stm32g431xx.h"
}
#include <cstdint>
#include "gpio_template.hpp"
#include "device_defines.hpp"

/**
 * @brief UART driver error codes
 */
enum class UartError {
    None,       /**< No error occurred */
    TxTimeout   /**< Transmission timeout (TX register not ready) */
};

/**
 * @brief Supported UART interrupt sources
 */
enum class UartIRQ {
    RXFNE,  /**< Receive buffer not empty (RXNE) interrupt */
    IDLE,   /**< IDLE line detected interrupt */
    TC,     /**< Transmission complete interrupt */
    NONE    /**< No interrupt (default mode) */
};

/**
 * @brief UART pin mapping and peripheral helpers
 *
 * Template is specialized per UART instance (1, 2, 3).
 * Provides GPIO pin configuration, peripheral base pointer,
 * and functions to enable/disable peripheral clock.
 *
 * @tparam Instance UART instance number (1, 2, or 3)
 */
template<int Instance>
struct UartPins;

// -----------------------------------------------------------------------------
// USART1 mapping: PA9=TX, PA10=RX
// -----------------------------------------------------------------------------
/**
 * @brief UART1 pin mapping and peripheral helpers
 *
 * - TX: PA9, Alternate function AF7
 * - RX: PA10, Alternate function AF7
 *
 * Provides access to the USART1 peripheral and its clock control.
 */
template<> struct UartPins<1> {
    using Tx = Gpio<GpioPort::A, 9,  GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<GpioPort::A, 10, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;

    static USART_TypeDef* uart() { return USART1; }
    static void enableClock() { RCC->APB2ENR |= RCC_APB2ENR_USART1EN; }
    static void disableClock() { RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN; }
};

// -----------------------------------------------------------------------------
// USART2 mapping: PA2=TX, PA3=RX
// -----------------------------------------------------------------------------
/**
 * @brief UART2 pin mapping and peripheral helpers
 *
 * - TX: PA2, Alternate function AF7
 * - RX: PA3, Alternate function AF7
 *
 * Provides access to the USART2 peripheral and its clock control.
 */
template<> struct UartPins<2> {
    using Tx = Gpio<GpioPort::A, 2, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<GpioPort::A, 3, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;

    static USART_TypeDef* uart() { return USART2; }
    static void enableClock() { RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; }
    static void disableClock() { RCC->APB1ENR1 &= ~RCC_APB1ENR1_USART2EN; }
};

// -----------------------------------------------------------------------------
// USART3 mapping: PB10=TX, PB11=RX
// -----------------------------------------------------------------------------
/**
 * @brief UART3 pin mapping and peripheral helpers
 *
 * - TX: PB10, Alternate function AF7
 * - RX: PB11, Alternate function AF7
 *
 * Provides access to the USART3 peripheral and its clock control.
 */
template<> struct UartPins<3> {
    using Tx = Gpio<GpioPort::B, 10, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;
    using Rx = Gpio<GpioPort::B, 11, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF7>;

    static USART_TypeDef* uart() { return USART3; }
    static void enableClock() { RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN; }
    static void disableClock() { RCC->APB1ENR1 &= ~RCC_APB1ENR1_USART3EN; }
};


// --------------------------------------------
// Singleton guard per peripheral instance
// --------------------------------------------
template<int Instance>
struct UartInstanceGuard {
    static inline bool constructed = false;
};

/**
 * @file uart_template.hpp
 * @brief Compile-time configurable UART driver for STM32G4 series using RAII GPIOs.
 *
 * Provides a lightweight, template-based UART driver supporting optional IRQs,
 * TX/RX operations, and integer/string printing over USART peripherals.
 *
 * @tparam Instance  USART instance number (1, 2, or 3)
 * @tparam Baudrate  Communication baud rate (e.g., 115200)
 * @tparam irq       Optional IRQ mode (UartIRQ::RXFNE, IDLE, TC, or NONE)
 * @tparam priority  NVIC priority for IRQ (0=highest, 15=lowest)
 *
 * @note This driver uses RAII for GPIO pins. Peripheral clock is automatically
 *       enabled at construction and disabled in destructor.
 *
 * @code
 * // Example: USART2 at 115200 baud, no interrupts
 * Uart<2, 115200> uart2;
 *
 * // Send a string
 * uart2.send("Hello World!\n");
 *
 * // Send an integer
 * uart2.sendInt(12345);
 *
 * // Send single character with timeout
 * if(uart2.sendChar('A', 500) != UartError::None) {
 *     // handle TX timeout
 * }
 * @endcode
 */
template<int Instance, int Baudrate, UartIRQ irq = UartIRQ::NONE, uint8_t priority = 15>
class Uart {
public:
    using Pins = UartPins<Instance>;  /**< GPIO pin configuration for this UART */
    using Tx = typename Pins::Tx;   /**< Transmit pin type */
    using Rx = typename Pins::Rx;   /**< Receive pin type */

    /**
     * @brief Constructor
     *
     * Initializes UART peripheral with the specified baud rate, 8N1 frame,
     * and optional interrupt configuration. GPIO pins are automatically configured.
     */
    Uart()
        : tx_(), rx_()   // RAII GPIOs
    {
        if (UartInstanceGuard<Instance>::constructed) {
            __builtin_trap();
        }
        UartInstanceGuard<Instance>::constructed = true;

        Pins::enableClock();

        auto U = Pins::uart();

        // Reset control registers
        U->CR1 = 0;
        U->CR2 = 0;
        U->CR3 = 0;

        // Set baud rate
        U->BRR = SYSTEM_CLOCK / Baudrate;

        // 8N1, enable TX/RX, UE
        U->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

        if constexpr(irq != UartIRQ::NONE) {
            enableIRQ();
        }
    }

    /**
     * @brief Destructor
     *
     * Disables UART peripheral and disables its clock. RAII ensures pins are
     * automatically deinitialized.
     */
    ~Uart() {
        auto U = Pins::uart();

        // Disable peripheral
        U->CR1 &= ~USART_CR1_UE;

        Pins::disableClock();
        UartInstanceGuard<Instance>::constructed = false;
    }

    //Non-copyable, non-movable
    Uart(const Uart&) = delete;
    Uart& operator=(const Uart&) = delete;
    Uart(Uart&&) = delete;
    Uart& operator=(Uart&&) = delete;

    /**
     * @brief Send a single character
     * @param c Character to transmit
     * @param timeoutMs Timeout in milliseconds (default 1000)
     * @return UartError::None if successful, UartError::TxTimeout if timed out
     */
    UartError sendChar(char c, uint16_t timeoutMs = 1000) {
        uint32_t timeout = us_to_ticks(static_cast<uint32_t>(timeoutMs));
        auto U = Pins::uart();
        while (!(U->ISR & USART_ISR_TXE_TXFNF) && timeout--);
        if(!timeout) return UartError::TxTimeout;
        U->TDR = c;
        return UartError::None;
    }

    /**
     * @brief Send a null-terminated string
     * @param str Pointer to string
     * @return UartError::None if all characters transmitted successfully,
     *         UartError::TxTimeout if any character timed out
     */
    UartError send(const char* str) {
        auto error = UartError::None;
        while (*str) {
            error = sendChar(*str++, 1000);
            if (error != UartError::None) return error;
        }
        return error;
    }

    /**
     * @brief Send a signed integer as ASCII characters
     * @param val Integer value to send
     * @return UartError::None if successful, UartError::TxTimeout on failure
     */
    UartError sendInt(int16_t val) {
        char buffer[12];
        intToStr(val, buffer);
        auto error = send(buffer);
        if(error != UartError::None) return error;
        return error;
    }

    /**
     * @brief Enable NVIC interrupt for the UART
     *
     * Enables the requested UART interrupt and sets NVIC priority.
     * Only called if template parameter irq != UartIRQ::NONE.
     */
    static void enableIRQ() {
        auto U = Pins::uart();
        switch (irq) {
        case UartIRQ::RXFNE: U->CR1 |= USART_CR1_RXNEIE; break;
        case UartIRQ::IDLE:  U->CR1 |= USART_CR1_IDLEIE; break;
        case UartIRQ::TC:    U->CR1 |= USART_CR1_TCIE;   break;
        }

        int irq_num = 0;
        if constexpr (Instance == 1) irq_num = USART1_IRQn;
        else if constexpr (Instance == 2) irq_num = USART2_IRQn;
        else if constexpr (Instance == 3) irq_num = USART3_IRQn;

        NVIC_SetPriority((IRQn_Type)irq_num, priority);
        NVIC_EnableIRQ((IRQn_Type)irq_num);
    }

private:
    Tx tx_;  /**< RAII-managed transmit pin */
    Rx rx_;  /**< RAII-managed receive pin */

    /**
     * @brief Convert integer to string
     * @param value Integer to convert
     * @param buffer Output buffer (at least 12 bytes)
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

        while(i > 0) buffer[j++] = tmp[--i];
        buffer[j] = '\0';
    }

    /**
     * @brief Convert microseconds to system ticks based on SYSTEM_CLOCK
     * @param us Microseconds
     * @return Number of ticks
     */
    static inline uint32_t us_to_ticks(uint32_t us) {
        return us * (SYSTEM_CLOCK / 1000000U);
    }
};
