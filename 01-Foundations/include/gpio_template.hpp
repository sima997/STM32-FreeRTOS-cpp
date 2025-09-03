#pragma once

#include <cstdint>
#include "stm32g431xx.h"

enum class GpioMode : uint32_t { Input=0b00, Output=0b01, Alt=0b10, Analog=0b11 };
enum class GpioSpeed : uint32_t { Low=0b00, Medium=0b01, High=0b10, VeryHigh=0b11 };
enum class GpioPull : uint32_t { None=0b00, Up=0b01, Down=0b10 };
enum class GpioAF : uint32_t { AF0=0, AF1, AF2, AF3, AF4, AF5, AF6, AF7, AF8, AF9, AF10, AF11, AF12, AF13, AF14, AF15 };
enum class GpioOType : uint32_t {PushPul = 0b0, OpenDrain = 0b1};


template<char Port, uint8_t Pin,
         GpioMode Mode, GpioSpeed Speed=GpioSpeed::Low,
         GpioPull Pull=GpioPull::None, GpioAF AF=GpioAF::AF0, GpioOType OType = GpioOType::PushPul>
class Gpio {
private:
    //Copile time checks
    static_assert(Pin < 16, "Pin number must be 0..15");
    static_assert(Port >= 'A' && Port <= 'G', "Port must be A..G");
    static_assert(static_cast<uint32_t>(Mode) <= 0b11, "Invalid GPIO Mode");
    static_assert(static_cast<uint32_t>(Speed) <= 0b11, "Invalid GPIO Speed");
    static_assert(static_cast<uint32_t>(Pull) <= 0b10, "Invalid GPIO Pull");
    static_assert(static_cast<uint32_t>(AF) <= 15, "Invalid GPIO Alternate Function");
    static_assert(static_cast<uint32_t>(OType) <= 0b1, "Invalid Output Type");
    


    //CMSIS compatible implementation
    static GPIO_TypeDef* port() {
        if constexpr(Port == 'A') return GPIOA;
        else if constexpr(Port == 'B') return GPIOB;
        else if constexpr(Port == 'C') return GPIOC;
        else if constexpr(Port == 'D') return GPIOD;
        else if constexpr(Port == 'E') return GPIOE;
        else if constexpr(Port == 'F') return GPIOF;
        else if constexpr(Port == 'G') return GPIOG;
        
    }
public:

    static void init() {
        //Enable clock
        enableClock();
        
        //Set mode
        setMode(Mode);

        //Set Alternate function if asked
        setAF(AF);

        //Set speed
        setSpeed(Speed);

        //Set pull up/down
        setPull(Pull);

        //Set Output Type
        setOType(OType);
        
        
    }

    static void setMode(GpioMode mode) {
        port()->MODER &= ~(0b11u << (Pin * 2));  // clear
        port()->MODER |=  (static_cast<uint32_t>(mode) << (Pin * 2));  // set
    }

    static void setAF(GpioAF af) {
        if constexpr(Mode == GpioMode::Alt) {
            if constexpr(Pin <= 7) {
                port()->AFR[0] &= ~ (GPIO_AFRL_AFSEL0_Msk << (Pin*4));
                port()->AFR[0] |= (static_cast<uint32_t>(af) << (Pin*4));
            } else {
                port()->AFR[1] &= ~ (GPIO_AFRL_AFSEL0_Msk << ((Pin-8)*4));
                port()->AFR[1] |= (static_cast<uint32_t>(af) << ((Pin-8)*4));
            }
        }
    }

    static void setSpeed(GpioSpeed speed) {
        port()->OSPEEDR &= ~(0b11 << (Pin*2));
        port()->OSPEEDR |= (static_cast<uint32_t>(speed) << (Pin*2));
    }

    static void setPull(GpioPull pull) {
        port()->PUPDR &= ~(0b11 << (Pin*2));
        port()->PUPDR |= (static_cast<uint32_t>(pull) << (Pin*2));
    }

    static void setOType(GpioOType otype) {
        port()->OTYPER |= (static_cast<uint32_t>(otype) << Pin);
    }

    static void set() {
        port()->BSRR = (1u << Pin);
    }

    static void clear() {
        port()->BSRR = (1u << (Pin + 16));
    }

    static void toggle() {
        port()->ODR ^= (1u << Pin);

    }

    static bool read() {
         return (port()->IDR & (1u << Pin)) != 0; 
    }

private:
    static void enableClock() {
        if constexpr(Port == 'A') RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
        else  if constexpr(Port == 'B') RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
        else  if constexpr(Port == 'C') RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
        else  if constexpr(Port == 'D') RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
        else  if constexpr(Port == 'E') RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
        else  if constexpr(Port == 'F') RCC->AHB2ENR |= RCC_AHB2ENR_GPIOFEN;
        else  if constexpr(Port == 'G') RCC->AHB2ENR |= RCC_AHB2ENR_GPIOGEN;
    }


};