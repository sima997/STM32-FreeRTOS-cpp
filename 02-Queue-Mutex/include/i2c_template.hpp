#pragma once

#include <cstdint>
#include <span>
#include "stm32g431xx.h"
#include "gpio_template.hpp"

/**
* I2C template driver (STM32G4, 7-bit addressing)
* - constexpr timing generation (no magic constants)
* - timeouts to avoid deadlocks
* - rich error reporting via enum class
* - size_t-safe loops, pass-by-ref single-byte read
*
* Template parameters:
* Instance : I2C instance number (1 = I2C1 specialization below)
* PclkHz : peripheral clock frequency in Hz (e.g., 128'000'000)
* SpeedHz : target I2C bus speed in Hz (100k, 400k, 1M)
* TimeoutCycles : spin-wait iteration budget for each wait
*/

/**
 * I2C Instance 1
 * SDA: PB9
 * SCL: PB8
 * 7-bit addressing mode
 */
//I2C errors
enum class I2cError : uint8_t {
    None = 0,
    Nack,
    Timeout,
    BusError,
    ArbitrationLost,
    Overrun,
    InvalidLength,

};
 
//General implementation for I2C pins
template<int Instance>
struct I2cPins;

//Specialization for I2C1 (PB9 SDA, PB8 SCL, AF4)
template<> struct I2cPins<1> {
    using SDA = Gpio<'B', 9, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF4, GpioOType::OpenDrain>;
    using SCL = Gpio<'B', 8, GpioMode::Alt, GpioSpeed::VeryHigh, GpioPull::None, GpioAF::AF4, GpioOType::OpenDrain>;

    //Get instance
    static I2C_TypeDef* i2c() { return I2C1; };

    //Enable clock to I2C
    static void enableClock() { 
        //Enable clock to peripheral
        RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
        //Select clock source as System Clock
        RCC->CCIPR &= ~RCC_CCIPR_I2C1SEL_Msk;
        RCC->CCIPR |= RCC_CCIPR_I2C1SEL_0; 
    }
};

struct I2cTiming {
    uint32_t presc;
    uint32_t scll;
    uint32_t sclh;
    uint32_t sdadel;
    uint32_t scldel;

    constexpr uint32_t value() const {
        return (presc << 28) | (scldel << 20) | (sdadel << 16) | (sclh << 8) | scll;
    }
};

// NOTE: This function implements simplified I2C timing calculation (refine as needed).
constexpr I2cTiming calc_i2c_timing(uint32_t pclk_hz, uint32_t i2c_hz) {
    //Fixed delays
    uint32_t sdadel = 2; // ~> data hold
    uint32_t scldel = 4; // ~> data setup


    //Target is ~50% duty cycle. TIMINGR encodes SCLL/SCLH as (cycles-1).
    //Find a prescaler such that period fits into 8-bit L/H fields.
    //period_cycles = pclk / ((presc+1) * i2c_hz)
    uint32_t presc = 0;
    uint32_t period_cycles = 0;


    for (uint32_t p = 0; p <= 15; ++p) {
    uint32_t cycles = pclk_hz / ((p + 1) * i2c_hz);
    if (cycles >= 4 && cycles <= 510) { //leave room for L/H split
    presc = p;
    period_cycles = cycles;
    break;
    }
    }


    if (period_cycles == 0) {
    //Fallback: slow down aggressively
    presc = 15;
    period_cycles = pclk_hz / ((presc + 1) * i2c_hz);
    if (period_cycles < 4) period_cycles = 4;
    if (period_cycles > 510) period_cycles = 510;
    }


    //50/50 duty (rounded)
    uint32_t half = period_cycles / 2;
    if (half == 0) half = 1;


    uint32_t sclh = half - 1;
    uint32_t scll = (period_cycles - half) - 1;


    //Clamp to field ranges using lambda functions
    auto clamp8 = [](uint32_t v){ return v > 255 ? 255u : v; };
    auto clamp4 = [](uint32_t v){ return v > 15 ? 15u : v; };


    return I2cTiming{
    .presc = clamp4(presc),
    .scll = clamp8(scll),
    .sclh = clamp8(sclh),
    .sdadel = clamp4(sdadel),
    .scldel = clamp4(scldel)
    };
}


template<int Instance, uint32_t PclkHz, uint32_t SpeedHz = 100'000, uint32_t TimeoutCycles = 100'000>
struct I2C {

    using Pins = I2cPins<Instance>;

    //Get instance
    static inline I2C_TypeDef* I2C_BASE() { return Pins::i2c(); }

    //Used registers masks
    struct CR1Mask { static constexpr uint32_t PE = I2C_CR1_PE; };

    struct CR2Mask {
        static constexpr uint32_t AUTOEND   = I2C_CR2_AUTOEND;
        static constexpr uint32_t START     = I2C_CR2_START;
        static constexpr uint32_t RD_WRN    = I2C_CR2_RD_WRN; //1=read, 0=write
    };

    struct ISRMask {
        static constexpr uint32_t TXIS  = I2C_ISR_TXIS;
        static constexpr uint32_t RXNE  = I2C_ISR_RXNE;
        static constexpr uint32_t NACK  = I2C_ISR_NACKF;
        static constexpr uint32_t STOP  = I2C_ISR_STOPF;
        static constexpr uint32_t TC    = I2C_ISR_TC;
        static constexpr uint32_t BERR  = I2C_ISR_BERR;
        static constexpr uint32_t ARLO  = I2C_ISR_ARLO;
        static constexpr uint32_t OVR   = I2C_ISR_OVR;
    };

    struct ICRMask {
        static constexpr uint32_t NACKCF    = I2C_ICR_NACKCF;
        static constexpr uint32_t STOPCF    = I2C_ICR_STOPCF;
        static constexpr uint32_t BERRCF    = I2C_ICR_BERRCF;
        static constexpr uint32_t ARLOCF    = I2C_ICR_ARLOCF;
        static constexpr uint32_t OVRCF     = I2C_ICR_OVRCF;
    };


   
    //Initialize I2C peripheral
    static I2cError init() {
        Pins::SDA::init();
        Pins::SCL::init();
        Pins::enableClock();

        //Reset control registers
        I2C_BASE()->CR1 = 0; 
        I2C_BASE()->CR2 = 0; 

        //Calculate timing
        constexpr auto timing = calc_i2c_timing(PclkHz, SpeedHz);
        I2C_BASE()->TIMINGR = timing.value();

        //Enable I2C peripheral
        I2C_BASE()->CR1 |= CR1Mask::PE;

        return I2cError::None;

    }

    static I2cError write(uint8_t address, std::span<const uint8_t> tx) {
        if(tx.size() > 255) return I2cError::InvalidLength;
        clear_all_flags();
        
        //Write address, data lenght and controller mode (auto end)
        I2C_BASE()->CR2 = ((static_cast<uint32_t>(address) & 0x7F) << 1) 
                        | (static_cast<uint32_t>(tx.size()) << 16) 
                        | CR2Mask::AUTOEND;
        //Generate start condition
        I2C_BASE()->CR2 |= CR2Mask::START;      

        //Send data
        for(size_t i = 0; i < tx.size(); i++) {
            if( auto e = wait_for_or_error(ISRMask::TXIS | ISRMask::NACK)) return e;
            if( auto e = check_bus_errors(); e != I2cError::None) return e;
            if( I2C_BASE()->ISR & ISRMask::NACK) return handle_nack();
            while(!(I2C_BASE()->ISR & (ISRMask::TXIS |  ISRMask::Nack))) {/*Spin until TXE or NACKF*/}
            I2C_BASE()->TXDR = tx[i]; //Write next byte
        }
        //Wait for stop flag -> clear
        if( auto e = wait_for_or_error(ISRMask::STOP)) return e;
        I2C_BASE()->ICR = ICRMask::STOPCF;
        
        return I2cError::None;
    }

    
     //Single byte write overload
    static I2cError write(uint8_t address, uint8_t tx_byte) {
        return write(address, std::span<const uint8_t>(&tx_byte,1));
    }

    

    static I2cError read(uint8_t address, std::span<uint8_t> rx) {
        if(rx.size() > 255) return I2cError::InvalidLength;
        clear_all_flags();

        I2C_BASE()->CR2 = ((static_cast<uint32_t>(address) &0x7F) << 1)
                        | (static_cast<uint32_t>(rx.size()) << 16) 
                        | CR2Mask::AUTOEND | CR2Mask::RD_WRN;
        //Generate start condition
        I2C_BASE()->CR2 |= CR2Mask::START; 

        for(size_t i = 0; i < rx.size(); i++) {
            if( auto e = wait_for_or_error(ISRMask::RXNE | ISRMask::NACK)) return e;
            if( auto e = check_bus_errors(); e != I2cError::None) return e;
            if( I2C_BASE()->ISR & ISRMask::NACK) return handle_nack();
            rx[i] = static_cast<uint8_t>(I2C_BASE()->RXDR);
        }

        if (auto e = wait_for_or_error(ISRMask::STOP)) return e;
        I2C_BASE()->ICR = ICRMask::STOPCF;

        return I2cError::None; 
    }

    //Single byte read overload
    static I2cError read(uint8_t address, uint8_t rx_byte) {
        return read(address, std::span<uint8_t>(&rx_byte,1));
    }

    static I2cError write_read(uint8_t address, std::span<const uint8_t> tx, std::span<uint8_t> rx) {
        if (tx.size() > 255 || rx.size() > 255) return I2cError::InvalidLength;
        clear_all_flags();

        // Write phase, no AUTOEND to allow repeated START
        I2C_BASE()->CR2 = ((static_cast<uint32_t>(address) & 0x7F) << 1) 
                        | ((static_cast<uint32_t>(tx.size()) & 0x7F) << 16);
        //Generate start condition
        I2C_BASE()->CR2 |= CR2Mask::START;      

        //Send data
        for(uint8_t i = 0; i < tx.size(); i++) {
            if (auto e = wait_for_or_error(ISRMask::TXIS | ISRMask::NACK); e != I2cError::None) return e;
            if (auto e = check_bus_errors(); e != I2cError::None) return e;
            if (I2C_BASE()->ISR & ISRMask::NACK) return handle_nack();
            I2C_BASE()->TXDR = tx[i];
}
            
        //Wait for repeated start (transfer complete)
        if(auto e = wait_for_or_error(ISRMask::TC); e != I2cError::None) return e;

        //Read phase with AUTOEND
        I2C_BASE()->CR2 = ((static_cast<uint32_t>(address) & 0x7F) << 1) 
        | (static_cast<uint32_t>(rx.size()) << 16) 
        | CR2Mask::AUTOEND | CR2Mask::RD_WRN;
        //Generate start condition
        I2C_BASE()->CR2 |= CR2Mask::START; 

        for(size_t i = 0; i < rx.size(); i++) {
            if (auto e = wait_for_or_error(ISRMask::RXNE | ISRMask::NACK); e != I2cError::None) return e;
            if (auto e = check_bus_errors(); e != I2cError::None) return e;
            if (I2C_BASE()->ISR & ISRMask::NACK) return handle_nack();            
            rx[i] = static_cast<uint8_t>(I2C_BASE()->RXDR);
        }
        //Wait for stop flag -> clear
        if (auto e = wait_for_or_error(ISRMask::STOP); e != I2cError::None) return e;
        I2C_BASE()->ICR = ICRMask::STOPCF;
        
        return I2cError::None;
    }

    //Single byte write-read overload
    static I2cError write_read(uint8_t address, uint8_t tx_byte, uint8_t& rx_byte) {
        std::span<uint8_t> tx(&tx_byte,1);
        std::span<uint8_t> rx(&rx_byte,1);
        return write_read(address, tx, rx);
    }

   

private:
    static inline void clear_all_flags() {
        //Clear errors from prevoius transfer
        I2C_BASE()->ICR = ICRMask::BERRCF | ICRMask::ARLOCF 
                        | ICRMask::OVRCF | ICRMask::NACKCF 
                        | ICRMask::STOPCF;
    }

    static inline I2cError check_bus_errors() {
        uint32_t isr = I2C_BASE()->ISR;
        if( isr & ISRMask::BERR) {I2C_BASE()->ICR = ICRMask::BERRCF; return I2cError::BusError; }
        if( isr & ISRMask::ARLO) {I2C_BASE()->ICR = ICRMask::ARLOCF; return I2cError::ArbitrationLost; }
        if( isr & ISRMask::OVR)  {I2C_BASE()->ICR = ICRMask::OVRCF; return I2cError::Overrun; }
        return I2cError::None;
    }

    static inline I2cError handle_nack() {
        I2C_BASE()->ICR = ICRMask::NACKCF; // clear NACK
        // Wait for STOP to complete the abort sequence
        if (auto e = wait_for_or_error(ISRMask::STOP); e != I2cError::None) return e; // timeout -> report timeout
        I2C_BASE()->ICR = ICRMask::STOPCF;
        return I2cError::Nack;
    }


    static inline I2cError wait_for_or_error(uint32_t mask) {
        uint32_t budget = TimeoutCycles;
        while (!(I2C_BASE()->ISR & mask)) {
            if (auto e = check_bus_errors(); e != I2cError::None) return e;
            if (--budget == 0) return I2cError::Timeout;
        }
        return I2cError::None;
    }
};

/* ----------------------------- Usage example ------------------------------
using I2C1_100k = I2c<1, 128'000'000, 100'000>;


void app_init() {
I2C1_100k::init();
}


I2cError probe(uint8_t addr7) {
uint8_t dummy;
return I2C1_100k::write_read(addr7, 0x00, dummy); // example register 0x00
}


*/