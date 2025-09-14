#pragma once
extern "C" {
    #include "stm32g431xx.h"
}
#include "uart_template.hpp"
#include "device_defines.hpp"
#include <type_traits>


/**
 * @brief ADC peripheral selection
 *
 * Specifies which ADC instance to use on STM32G4 series.
 */
enum class AdcInstance : uint32_t {
    Adc1, /**< ADC1 peripheral */
    Adc2  /**< ADC2 peripheral */
};

/**
 * @brief ADC prescaler selection
 *
 * Determines the division factor of the ADC input clock relative to system clock.
 */
enum class AdcPsc : uint32_t { 
    div1   = 0,  /**< Divide by 1 */
    div2   = 1,  /**< Divide by 2 */
    div4   = 2,  /**< Divide by 4 */
    div6   = 3,  /**< Divide by 6 */
    div8   = 4,  /**< Divide by 8 */
    div10  = 5,  /**< Divide by 10 */
    div12  = 6,  /**< Divide by 12 */
    div16  = 7,  /**< Divide by 16 */
    div32  = 8,  /**< Divide by 32 */
    div64  = 9,  /**< Divide by 64 */
    div128 = 10, /**< Divide by 128 */
    div256 = 11  /**< Divide by 256 */
};

/**
 * @brief ADC clock mode
 *
 * Selects whether the ADC uses asynchronous clock or synchronous clock with division.
 */
enum class AdcClkMode : uint32_t {
    Asynchronous     = 0, /**< Independent ADC clock */
    SynchronousDiv1  = 1, /**< Synchronous clock, divide by 1 */
    SynchronousDiv2  = 2, /**< Synchronous clock, divide by 2 */
    SynchronousDiv4  = 3  /**< Synchronous clock, divide by 4 */
};

/**
 * @brief ADC error codes
 *
 * Indicates the result of ADC operations like initialization or conversion.
 */
enum class AdcError : uint8_t {
    None,             /**< No error */
    Calibration,      /**< Error during calibration */
    NotReady,         /**< ADC not ready */
    ConversionTimeout /**< ADC conversion timed out */
};

/**
 * @brief Special ADC channels
 *
 * Allows selection of internal channels such as VBAT, temperature sensor, or VREF.
 */
enum class AdcSpecialChannel : uint32_t {
    None  = 0,                    /**< No special channel */
    VBAT  = ADC_CCR_VBATSEL,      /**< Battery voltage channel */
    TEMP  = ADC_CCR_VSENSESEL,    /**< Temperature sensor channel */
    VREF  = ADC_CCR_VREFEN        /**< Internal reference voltage channel */
};

/**
 * @brief ADC channel numbers
 *
 * Selects which ADC channel to convert. Matches the hardware ADC input pins.
 */
enum class AdcChannel : uint32_t {
    CH0  = 0,  CH1  = 1,  CH2  = 2,  CH3  = 3,  CH4  = 4,
    CH5  = 5,  CH6  = 6,  CH7  = 7,  CH8  = 8,  CH9  = 9,
    CH10 = 10, CH11 = 11, CH12 = 12, CH13 = 13, CH14 = 14,
    CH15 = 15, CH16 = 16, CH17 = 17, CH18 = 18
};

/**
 * @brief ADC sampling times
 *
 * Determines how many ADC clock cycles the sample-and-hold circuit uses.
 */
enum class AdcSamplTime : uint8_t {
    CYCLES_2_5   = 0b000, /**< 2.5 ADC clock cycles */
    CYCLES_6_5   = 0b001, /**< 6.5 ADC clock cycles */
    CYCLES_12_5  = 0b010, /**< 12.5 ADC clock cycles */
    CYCLES_24_5  = 0b011, /**< 24.5 ADC clock cycles */
    CYCLES_47_5  = 0b100, /**< 47.5 ADC clock cycles */
    CYCLES_92_5  = 0b101, /**< 92.5 ADC clock cycles */
    CYCLES_247_5 = 0b110, /**< 247.5 ADC clock cycles */
    CYCLES_640_5 = 0b111  /**< 640.5 ADC clock cycles */
};

/**
 * @brief ADC timing parameters
 *
 * Holds the calculated wait times for calibration and regulator stabilization.
 */
struct AdcWaitTimes {
    uint32_t t_cal = 0; /**< Calibration wait time in ticks */
    uint32_t t_reg = 0; /**< Regulator stabilization wait time in ticks */
};

/**
 * @brief Template class for STM32 ADC1/ADC2 peripheral configuration and control.
 *
 * This class provides a compile-time configurable ADC driver for STM32G4 series.
 * Supports optional logging through a UART driver. The ADC is fully initialized
 * at construction and can be used to perform conversions on a selected channel.
 *
 * @tparam Instance      ADC peripheral instance (Adc1 or Adc2)
 * @tparam SYSTEM_CLOCK  System clock frequency in Hz
 * @tparam psc           ADC prescaler (AdcPsc enum)
 * @tparam clk           ADC clock mode (Asynchronous or SynchronousDiv1/2/4)
 * @tparam specChannel   Optional special channel (temperature, VBAT, VREF) or None
 * @tparam channel       ADC channel for conversion (AdcChannel enum)
 * @tparam tsamp         ADC sampling time (AdcSamplTime enum)
 * @tparam UartDriver    Optional UART driver type for logging. Default = void (no logging)
 *
 * @note  ADC clock and prescaler are configured at initialization.
 * @note  Optional logging occurs only if UartDriver is provided and a pointer is passed.
 * @note  This class initializes the ADC hardware in the constructor.
 * 
 * @code
 * 
 * // Example 1: ADC with logging
 * Uart<2, 115200> uartLog;
 * uartLog.init(128000000);
 *
 * Adc12<AdcInstance::Adc1,
 *       128000000,
 *       AdcPsc::div16,
 *       AdcClkMode::SynchronousDiv4,
 *       AdcSpecialChannel::TEMP,
 *       AdcChannel::CH17,
 *       AdcSamplTime::CYCLES_640_5,
 *       Uart<2,115200>
 * > tempSensor(uartLog);
 *
 * // Example 2: ADC without logging
 * Adc12<> adcNoLog;
 *
 * // Access initialization status
 * if (adcNoLog.errorStatus != AdcError::None) {
 *     // handle error
 * }
 * @endcode
 */

template<AdcInstance Instance = AdcInstance::Adc1, 
        AdcPsc psc = AdcPsc::div8, 
        AdcClkMode clk = AdcClkMode::SynchronousDiv4, 
        AdcSpecialChannel specChannel = AdcSpecialChannel::TEMP, 
        AdcChannel channel = AdcChannel::CH17, 
        AdcSamplTime tsamp = AdcSamplTime::CYCLES_640_5,
        typename UartDriver = void
>
class Adc12 {
public:
    /** @brief Last error status of ADC initialization or operation */
    AdcError errorStatus;
        /**
     * @brief Default constructor
     *
     * Initializes the ADC peripheral with template parameters.
     * If no UART is provided, logging is disabled.
     * @note Updates errorStatus with initialization result.
     */
    Adc12(){
        errorStatus = init();
    }

    /**
     * @brief Constructor with optional UART logging
     *
     * @param uart Reference to UART driver for logging initialization messages.
     *             Pass a valid UART instance to enable logging.
     * @note Constructor enabled only if UartDriver != void.
     */
    template <typename U = UartDriver,
          typename = std::enable_if_t<!std::is_void_v<U>>>
    explicit Adc12(U& uart) : uart_(&uart) {
        errorStatus = init();
    }

    /** @brief Destructor disables adc and special channel  */
    ~Adc12() {
        // Disable ADC
        getInstance()->CR &= ~ADC_CR_ADEN;
        //Optionally clear special channel
        if (specChannel != AdcSpecialChannel::None) {
            ADC12_COMMON->CCR &= ~static_cast<uint32_t>(specChannel);
        }
    }

    /**
     * @brief Starts a single ADC conversion and retrieves the result.
     *
     * This function triggers an ADC conversion on the configured channel,
     * waits for the conversion to complete (EOC = End Of Conversion),
     * and reads the converted value into the provided output variable.
     *
     * @param[out] out  Reference to a uint16_t variable where the ADC result will be stored.
     *
     * @return AdcError Returns:
     *         - AdcError::None if the conversion succeeded,
     *         - AdcError::ConversionTimeout if the conversion did not complete within the timeout.
     *
     * @note The timeout is calculated using the system clock to avoid indefinite blocking.
     * @note This function directly accesses ADC1 registers; modify if using a different ADC instance.
     */
    AdcError start_conversion(uint16_t& out) {
        auto error = AdcError::None;

        // Clear EOC flag before starting
        ADC1->ISR |= ADC_ISR_EOC;

        // Start ADC conversion
        ADC1->CR |= ADC_CR_ADSTART;

        // Wait until conversion completes or timeout expires
        uint32_t timeout = us_to_ticks(5);
        while(!(ADC1->ISR & ADC_ISR_EOC) && timeout--);

        // Check for timeout
        if(!timeout) {
            // Update internal status
            error = AdcError::ConversionTimeout; 
            return error;
        } 

        // Read ADC conversion result
        out = static_cast<uint16_t>(ADC1->DR & 0xFFFF);
        
        return error;
    }

private:
    /** @brief Precomputed ADC wait times (t_calibration, t_regulator) */
    static inline AdcWaitTimes times;
     /** @brief Pointer to UART driver (nullptr if logging disabled) */
    std::conditional_t<std::is_void_v<UartDriver>, std::nullptr_t, UartDriver*> uart_{nullptr};

    /** @brief Lookup table for ADC prescaler division values */
    static constexpr uint32_t adc_psc_values[] = {
        1, 2, 4, 6, 8, 10, 12, 16, 32, 64, 128, 256
    };

     /** @brief Convert enum prescaler to numeric divider */
    static constexpr uint32_t to_divider() {
        return adc_psc_values[static_cast<uint32_t>(psc)];
    };

    /**
     * @brief Returns pointer to ADC hardware instance
     * @return ADC_TypeDef* corresponding to template Instance
     */
    static constexpr ADC_TypeDef* getInstance() {
        if constexpr (Instance == AdcInstance::Adc1) return ADC1;
        else if constexpr(Instance == AdcInstance::Adc2) return ADC2;
        else static_assert(Instance == AdcInstance::Adc1 || Instance == AdcInstance::Adc2, "ADC instance not supported.");
    }

    /**
     * @brief Initialize ADC hardware
     * @return AdcError status of initialization
     */
    AdcError init() {
        clock_enable();
        power_enable();
        init_wait(times);
        auto err = calibration(times);
        if( err != AdcError::None) return err;
        enable_special_channel();
        configure_channel();
        err = adc_enable();
        if( err != AdcError::None) return err;
        log("Succesfull ADC initialization!");

        return AdcError::None;
    }

    /** @brief Enable ADC clock and configure prescaler & clock mode */
    static void clock_enable() {
        RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
        uint32_t ccr = ADC12_COMMON->CCR & ~(ADC_CCR_PRESC_Msk | ADC_CCR_CKMODE_Msk);
        ccr |= (static_cast<uint32_t>(psc) << ADC_CCR_PRESC_Pos);
        if(clk != AdcClkMode::Asynchronous) {
            ccr |= (static_cast<uint32_t>(clk) << ADC_CCR_CKMODE_Pos);
        }
        ADC12_COMMON->CCR = ccr;

    }

    /** @brief Power up ADC, exit deep power-down, enable regulator */
    void power_enable() {
        getInstance()->CR &= ~ ADC_CR_DEEPPWD;
        getInstance()->CR |= ADC_CR_ADVREGEN;
        uint32_t timeout = us_to_ticks(30);
        while(timeout--);
    }

    /** @brief Perform ADC calibration (single-ended) */
    AdcError calibration(AdcWaitTimes& times_init) {
        getInstance()->CR &= ~ADC_CR_ADEN;
        uint32_t timeout = us_to_ticks(10);
        if (getInstance()->CR & ADC_CR_ADEN) {
            getInstance()->CR |= ADC_CR_ADDIS;
            while (getInstance()->CR & ADC_CR_ADEN && timeout--); 
            if (!timeout) return AdcError::NotReady; 
        }
        while (getInstance()->ISR & ADC_ISR_ADRDY & timeout--);      

        getInstance()->CR &= ~ADC_CR_ADCALDIF;
        getInstance()->CR |= ADC_CR_ADCAL;
        timeout = times_init.t_cal;
        while ( (getInstance()->CR & ADC_CR_ADCAL && timeout--) ) {}
        if (getInstance()->CR & ADC_CR_ADCAL) return AdcError::Calibration;

        return AdcError::None;
    }

    /** @brief Enable optional special channel if configured */
    static void enable_special_channel() {
        if (specChannel != AdcSpecialChannel::None) {
            ADC12_COMMON->CCR |= static_cast<uint32_t>(specChannel);
        }
    }

    /** @brief Configure ADC conversion (fixed single conversion) channel and sampling time */
    void configure_channel() {
        getInstance()->SQR1 &= ~ ADC_SQR1_L_Msk;
        getInstance()->SQR1 |= (1UL << ADC_SQR1_L_Pos);
        getInstance()->SQR1 &= ~ADC_SQR1_SQ1_Msk;
        getInstance()->SQR1 |= (static_cast<uint32_t>(channel) << ADC_SQR1_SQ1_Pos);
        if (channel <= AdcChannel::CH9) {
            getInstance()->SMPR1 &= ~(0b111UL << (3 * static_cast<uint32_t>(channel)));
            getInstance()->SMPR1 |= static_cast<uint32_t>(tsamp) << (3 * static_cast<uint32_t>(channel));
        } else {
            uint32_t ch = static_cast<uint32_t>(channel) - 10;
            getInstance()->SMPR2 &= ~(0b111UL << (3 * ch));
            getInstance()->SMPR2 |= static_cast<uint32_t>(tsamp) << (3 * ch);
        }

    }

    /** @brief Enable ADC and wait until ready */
    AdcError adc_enable() {
        getInstance()->ISR |= ADC_ISR_ADRDY;
        getInstance()->CR |= ADC_CR_ADEN;
        uint32_t timeout = us_to_ticks(10);
        while(!(getInstance()->ISR & ADC_ISR_ADRDY) && timeout-- );
        if(!timeout) return AdcError::NotReady;

        return AdcError::None;
    }
 
    /** @brief Initialize ADC wait times based on clock frequency */
    static void init_wait(AdcWaitTimes& times_init) {
        uint32_t adc_clk = 0;
        if( clk != AdcClkMode::Asynchronous) {
            adc_clk = SYSTEM_CLOCK >> (static_cast<uint32_t>(clk) - 1);
        
        } else {
            uint32_t pllr_div = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLR_Msk) >> (RCC_PLLCFGR_PLLR_Pos - 1)) + 2;
            uint32_t pllp_div = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLPDIV_Msk) >> (RCC_PLLCFGR_PLLPDIV_Pos));
            adc_clk = (SYSTEM_CLOCK / to_divider()) * pllr_div / pllp_div;
        }
        times_init.t_cal = adc_ticks_to_hclk(130, adc_clk);
        times_init.t_reg = us_to_ticks(3);  
    }

    /** @brief Convert ADC ticks to HCLK ticks */
    static uint32_t adc_ticks_to_hclk(uint32_t adc_ticks, uint32_t f_adc) {
        uint32_t ratio = SYSTEM_CLOCK / f_adc;       
        uint32_t rem   = SYSTEM_CLOCK % f_adc;      
        //Base conversion
        uint32_t hclk_ticks = adc_ticks * ratio;
        //Correction
        hclk_ticks += (adc_ticks * rem) / f_adc;

        return hclk_ticks;
    }

    /** @brief Convert microseconds to system ticks */
    static inline uint32_t us_to_ticks(uint32_t us) {
        return us * (SYSTEM_CLOCK / 1000000U);
    }

    /** @brief Send log message if UART logging is enabled */
    void log(const char* msg) {
        if constexpr (std::is_void_v<UartDriver>) {
        } else if (uart_) {
            uart_->send(msg);
        }
    }
};