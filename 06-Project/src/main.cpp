#include "stm32g431xx.h"
#include <cstdint>
#include "gpio_template.hpp"
#include "uart_template.hpp"

/**
 * Requirements
 * -------------
 * f_max = 42 MHz (if all ADCs in single ended mode)
 * f_min = 0.14 MHz (if all ADCs in single ended mode)
 * t_cal = 116 [1/f_ADC] (calibration time)
 * t_ADCVREG_STUP = 2 us ( ADC voltage regulator start-up time)
 * t_CONV = 0.25 - 10.883 us (conversion time)
 * t_s = 2.5 - 604.5 [1/f_ADC] (sampling time)
 * Clock input from PLL_P -> HSI / PLL_M(2) * PLL_N(32) / PLL_P(?)  = 16 MHz / 2 * 32 / PLL_P = 256 / PLL_P = 64 Mhz -> PLL_P = 4
 */

using Led = Gpio<GpioPort::A, 5, GpioMode::Output>;
using Uart2 = Uart<2, 115200>;

const uint32_t system_clock = 128'000'000;
const uint32_t pllp_clock = 64'000'000;
uint32_t adc_clock;

enum class AdcClkMode : uint32_t {
    Asynchronous = 0,
    SynchronousDiv1 = 1,
    SynchronousDiv2 = 2,
    SynchronousDiv4 = 3,
};

enum class AdcPsc : uint32_t { 
    div1 = 0,
    div2 = 1,
    div4 = 2,
    div6 = 3,
    div8 = 4,
    div10 = 5,
    div12 = 6,
    div16 = 7,
    div32 = 8,
    div64 = 9,
    div128 = 10,
    div256 = 11

};

enum class AdcSpecialChannel : uint32_t {
    None = 0,
    VBAT = ADC_CCR_VBATSEL,
    TEMP = ADC_CCR_VSENSESEL,
    VREF = ADC_CCR_VREFEN
};

enum class AdcChannel :uint32_t {
    CH0 = 0,
    CH1 = 1,
    CH2 = 2,
    CH3 = 3,
    CH4 = 4,
    CH5 = 5,
    CH6 = 6,
    CH7 = 7,
    CH8 = 8,
    CH9 = 9,
    CH10 = 10,
    CH11 = 11,
    CH12 = 12,
    CH13 = 13,
    CH14 = 14,
    CH15 = 15,
    CH16 = 16,
    CH17 = 17,
    CH18 = 18,

};

enum class AdcSamplTime : uint8_t {
    CYCLES_2_5   = 0b000, // 2.5 ADC clock cycles
    CYCLES_6_5   = 0b001, // 6.5 ADC clock cycles
    CYCLES_12_5  = 0b010, // 12.5 ADC clock cycles
    CYCLES_24_5  = 0b011, // 24.5 ADC clock cycles
    CYCLES_47_5  = 0b100, // 47.5 ADC clock cycles
    CYCLES_92_5  = 0b101, // 92.5 ADC clock cycles
    CYCLES_247_5 = 0b110, // 247.5 ADC clock cycles
    CYCLES_640_5 = 0b111  // 640.5 ADC clock cycles
};

enum class AdcError : uint8_t { None, Calibration, NotReady, ConversionTimeout};

//Divider access and LUT
constexpr uint32_t adc_psc_values[] = {
    1, 2, 4, 6, 8, 10, 12, 16, 32, 64, 128, 256
};

constexpr uint32_t to_divider(AdcPsc psc) {
    return adc_psc_values[static_cast<uint32_t>(psc)];
}



struct AdcWaitTimes {
    uint32_t t_cal = 0;
    uint32_t t_reg = 0;
    

};

void clock_enable(AdcPsc psc, AdcClkMode clk);
void power_enable();
AdcError calibration(AdcWaitTimes& times_init);
void init_wait(AdcWaitTimes& times_init, AdcPsc psc, AdcClkMode clk);
static inline uint32_t us_to_ticks(uint32_t us, uint32_t f_hclk_hz);
void enable_special_channel(AdcSpecialChannel channel);
void configure_channel(AdcChannel channel, AdcSamplTime tsamp);
AdcError adc_enalbe();
inline void wait_ticks(uint32_t ticks);
uint32_t adc_ticks_to_hclk(uint32_t adc_ticks, uint32_t f_hclk, uint32_t f_adc);
AdcError start_conversion(uint16_t& out);



int main(void) {
    AdcWaitTimes times;

    Led::init();
    Uart2::init(system_clock);
    uint16_t data_raw;

    //Set adc to 500 kHz
    clock_enable(AdcPsc::div8, AdcClkMode::SynchronousDiv4);
    power_enable();
    init_wait(times,AdcPsc::div128, AdcClkMode::SynchronousDiv4);
    auto err = calibration(times);
    //if(err == AdcError::None) Led::set();
    enable_special_channel(AdcSpecialChannel::TEMP);
    configure_channel(AdcChannel::CH17, AdcSamplTime::CYCLES_640_5);
    err = adc_enalbe();
    //if(err == AdcError::None) Led::set();
    err = start_conversion(data_raw);
    if(err == AdcError::None) Led::set();
    Uart2::sendInt(static_cast<int16_t>(data_raw));
    

    

    while(1){

    }
}



void clock_enable(AdcPsc psc, AdcClkMode clk) {
    //Enable clock for ADC1 and 2
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    //Set prescaler
    ADC12_COMMON->CCR &= ~ADC_CCR_PRESC_Msk;
    ADC12_COMMON->CCR |= (static_cast<uint32_t>(psc) << ADC_CCR_PRESC_Pos);
    //Enable ADC in asynchronous mode (hard setting) - set 0
    ADC12_COMMON->CCR &= ~ADC_CCR_CKMODE_Msk;
    if(clk != AdcClkMode::Asynchronous) {
        ADC12_COMMON->CCR |= (static_cast<uint32_t>(clk) << ADC_CCR_CKMODE_Pos);
    }
    
     
}

void power_enable() {
    //Exit deep power down mode
    ADC1->CR &= ~ ADC_CR_DEEPPWD;
    //Enable ADC regulator and wait for t_ADCVREG_STUP = 2 us (set with margin)
    ADC1->CR |= ADC_CR_ADVREGEN;
    uint32_t timeout = us_to_ticks(30, system_clock);
    while(timeout--);

}

AdcError calibration(AdcWaitTimes& times_init) {
    //Disable ADC
    ADC1->CR &= ~ADC_CR_ADEN;
    uint32_t timeout = us_to_ticks(10, system_clock);
    if (ADC1->CR & ADC_CR_ADEN) {
        ADC1->CR |= ADC_CR_ADDIS;
        while (ADC1->CR & ADC_CR_ADEN && timeout--); 
        if (!timeout) return AdcError::NotReady; 
    }
    while (ADC1->ISR & ADC_ISR_ADRDY & timeout--);      

    //Set calibration mode to single ended, enable calibration, wait t_cal =
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |= ADC_CR_ADCAL;
    timeout = times_init.t_cal;
    while ( (ADC1->CR & ADC_CR_ADCAL) ) { /* busy */ }
    if (ADC1->CR & ADC_CR_ADCAL) return AdcError::Calibration;

    return AdcError::None;
}

void init_wait(AdcWaitTimes& times_init, AdcPsc psc, AdcClkMode clk) {
    uint32_t adc_clk = 0;
    if( clk != AdcClkMode::Asynchronous) {
        adc_clk = system_clock >> (static_cast<uint32_t>(clk) - 1);
    
    } else {
        uint32_t pllr_div = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLR_Msk) >> (RCC_PLLCFGR_PLLR_Pos - 1)) + 2;
        uint32_t pllp_div = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLPDIV_Msk) >> (RCC_PLLCFGR_PLLPDIV_Pos));
        adc_clk = (system_clock / to_divider(psc)) * pllr_div / pllp_div;
    }
    times_init.t_cal = adc_ticks_to_hclk(130, system_clock, adc_clk);
    times_init.t_reg = us_to_ticks(3, system_clock);
    
}

void wait_ticks(uint32_t ticks) {
        while(ticks--) {}
}

uint32_t adc_ticks_to_hclk(uint32_t adc_ticks, uint32_t f_hclk, uint32_t f_adc) {
    //Reduce frequency ratio before multiplying, remainder for correction
    uint32_t ratio = f_hclk / f_adc;       
    uint32_t rem   = f_hclk % f_adc;      
    //Base conversion
    uint32_t hclk_ticks = adc_ticks * ratio;
    //Correction
    hclk_ticks += (adc_ticks * rem) / f_adc;

    return hclk_ticks;
}

void enable_special_channel(AdcSpecialChannel channel) {
    if (channel != AdcSpecialChannel::None) {
        ADC12_COMMON->CCR |= static_cast<uint32_t>(channel);
    }
    
}

void configure_channel(AdcChannel channel, AdcSamplTime tsamp) {
    //Set conversion length 1
    ADC1->SQR1 &= ~ ADC_SQR1_L_Msk;
    ADC1->SQR1 |= (1UL << ADC_SQR1_L_Pos);
    //Set channel for conversion
    ADC1->SQR1 &= ~ADC_SQR1_SQ1_Msk;
    ADC1->SQR1 |= (static_cast<uint32_t>(channel) << ADC_SQR1_SQ1_Pos);
    //Set sampling time
    if(channel <= AdcChannel::CH9) {
        ADC1->SMPR1 &= ~(ADC_SMPR1_SMP0 << static_cast<uint32_t>(channel));
        ADC1->SMPR1 |= static_cast<uint32_t>(tsamp) << static_cast<uint32_t>(channel);
    } else {
        uint32_t channel_offset = static_cast<uint32_t>(channel) - static_cast<uint32_t>(AdcChannel::CH9);
        ADC1->SMPR1 &= ~ADC_SMPR1_SMP0 << channel_offset;
        ADC1->SMPR1 |= static_cast<uint32_t>(tsamp) << channel_offset;
    }
    
}

AdcError adc_enalbe() {
    //Clear flag, enable and wait ready
    ADC1->ISR |= ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    uint32_t timeout = us_to_ticks(10, system_clock);
    while(!(ADC1->ISR & ADC_ISR_ADRDY) && timeout-- );
    if(!timeout) return AdcError::NotReady;

    return AdcError::None;
}

AdcError start_conversion(uint16_t& out) {
    //Start conversion, wait EOC, read
    ADC1->CR |= ADC_CR_ADSTART;
    uint32_t timeout = us_to_ticks(5, system_clock);
    while(!(ADC1->ISR & ADC_ISR_EOC) && timeout--);
    if(!timeout) return AdcError::ConversionTimeout;
    out = (ADC1->DR & 0xFFFF);
    return AdcError::None;

}

static inline uint32_t us_to_ticks(uint32_t us, uint32_t f_hclk_hz) {
    return us * (f_hclk_hz / 1000000U);
}