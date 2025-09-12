extern "C" {
    #include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
}

#include "uart_template.hpp"
#include "gpio_template.hpp"
#include <cstring>
#include "task_template.hpp"
#include "watchdog.hpp"


//Using templates
using Uart2 = Uart<2, 115200>;
using Led = Gpio<GpioPort::A, 5, GpioMode::Output>;

//Temperature coeficients
#define TS_CAL1 *((uint16_t*)0x1FFF75A8)  // at 30 °C
#define TS_CAL2 *((uint16_t*)0x1FFF75CA)  // at 110 °C
#define ADC_CCR_VREFEN_Pos     (22U)
#define ADC_CCR_VREFEN_Msk     (0x1UL << ADC_CCR_VREFEN_Pos)

#define ADC_CCR_TSEN_Pos       (23U)
#define ADC_CCR_TSEN_Msk       (0x1UL << ADC_CCR_TSEN_Pos)

//Global temperature 
float temperature = 0.0f;


// Blink task
template<typename LedPin>
class BlinkTask : public Task {
    public:
        BlinkTask() : Task("Blink") {}
        void init() override {
            LedPin::init();
        }

        void run() override {
            
            for (;;) {
                
                Led::toggle();
                notifyAlive();
                vTaskDelay(pdMS_TO_TICKS(500));
                    
            }
        }


    private:
        
};

//Sensor task
class SensorTask : public Task {
public:
    SensorTask() : Task("Sensor") {}
    void init() override {
        // 1. Enable ADC clock
        RCC->CCIPR |= RCC_CCIPR_ADC12SEL_0;
        RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;

        // 2. Exit deep power-down
        ADC1->CR &= ~ADC_CR_DEEPPWD;

        // 3. Enable ADC voltage regulator
        ADC1->CR &= ~ADC_CR_ADVREGEN_Msk;
        ADC1->CR |= (0x1UL << ADC_CR_ADVREGEN_Pos);
        for (volatile int i=0; i<20000; i++);   // >20 µs delay (safe margin)

        // 4. Make sure ADC is disabled
        if (ADC1->CR & ADC_CR_ADEN) {
            ADC1->CR |= ADC_CR_ADDIS;
            while (ADC1->CR & ADC_CR_ADEN);
        }

        // 5. Calibrate (single-ended)
        ADC1->CR |= ADC_CR_ADCAL;
        while (ADC1->CR & ADC_CR_ADCAL);

        // 6. Enable internal channels: VREFINT + Temperature sensor
        ADC12_COMMON->CCR |= ADC_CCR_VREFEN_Msk | ADC_CCR_TSEN_Msk;

        // 7. Set sampling time for channel 17 (temperature sensor)
        //    Must be LONG (> 20 µs) for temp sensor to settle
        ADC1->SMPR2 &= ~ADC_SMPR2_SMP17_Msk;
        ADC1->SMPR2 |= (7U << ADC_SMPR2_SMP17_Pos);  // 640.5 cycles (max)

        // 8. Regular sequence: channel 17 in SQ1
        ADC1->SQR1 = (17U << ADC_SQR1_SQ1_Pos);

        // 9. Enable ADC
        ADC1->ISR = ADC_ISR_ADRDY;      // clear ADRDY
        ADC1->CR |= ADC_CR_ADEN;        // enable
        while (!(ADC1->ISR & ADC_ISR_ADRDY));  // wait until ready
    }


    void run() override {
        
        for(;;) {
            ADC1->CR |= ADC_CR_ADSTART;
            while (!(ADC1->ISR & ADC_ISR_EOC));
            uint16_t read = ADC1->DR;

            temperature =  ((float)(read - TS_CAL1)) * (110.0f - 30.0f) /
            (float)(TS_CAL2 - TS_CAL1) + 30.0f;
            //notifyAlive();
            vTaskDelay(pdMS_TO_TICKS(400));
        }

    }
};



// Main
int main() {
    // UART init
    Uart2 uart2;
    uart2.init(128000000);

    WatchdogSupervisor::init(1000);

    
    
    //Instantiate tasks
    static BlinkTask<Led> blink;
    static SensorTask sensor;

    WatchdogSupervisor::registerTask(&blink);
    //WatchdogSupervisor::registerTask(&sensor);

    // Create tasks
    xTaskCreate(Task::taskEntry, "Process", 256, &blink, 1, nullptr);
    xTaskCreate(Task::taskEntry, "Sensor", 512, &sensor, 1, nullptr);

    //Create watchdog task
    xTaskCreate(WatchdogSupervisor::taskEntry, "Watchdog",128, nullptr, 2, nullptr);

    __enable_irq();

    vTaskStartScheduler();

    while (1) {}
}
