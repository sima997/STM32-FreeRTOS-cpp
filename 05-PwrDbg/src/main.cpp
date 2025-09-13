extern "C" {
    #include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
}

#include "uart_template.hpp"
#include "gpio_template.hpp"
#include <cstring>
#include "task_template.hpp"
#include "adc12_template.hpp"
#include "watchdog.hpp"
#include "device_defines.hpp"

//Global handles
QueueHandle_t adc_queue;

//Using templates
using Uart2 = Uart<2, 115200>;
using Led = Gpio<GpioPort::A, 5, GpioMode::Output>;
using Adc_temp = Adc12<AdcInstance::Adc1,
                     AdcPsc::div16, 
                     AdcClkMode::SynchronousDiv4, 
                     AdcSpecialChannel::TEMP,
                     AdcChannel::CH17,
                     AdcSamplTime::CYCLES_640_5
                     >;

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
        BlinkTask(LedPin& led) : Task("Blink"), led_(led) {}
        void init() override {
            
        }

        void run() override {
            
            for (;;) {
                
                led_.toggle();
                notifyAlive();
                vTaskDelay(pdMS_TO_TICKS(500));
                    
            }
        }


    private:
        LedPin& led_;
        
};

//Sensor task
template<typename Adc>
class SensorTask : public Task {
public:
    SensorTask(Adc& adc) : Task("Sensor"), adc_(adc) {}
    void init() override {
        
    }


    void run() override {
        uint16_t adc_data = 0;
        for(;;){
            adc_.start_conversion(adc_data);
            if(xQueueSend(adc_queue,&adc_data,0) == pdPASS) {
                /**/
            }
            notifyAlive();
            vTaskDelay(pdMS_TO_TICKS(800));
        }
        
        

    }
private:
    Adc& adc_;
};

//Uart tas
template<typename UartDriver>
class LogTask : public Task {
public:
    LogTask(UartDriver& uart) : Task("LogTask"), uart_(uart) {}

    void init() override {
        
    }

    void run() override {
        uint16_t temp_data = 0;
        for(;;) {
            uart_.send("Device Alive!\n");
            if(xQueueReceive(adc_queue, &temp_data,pdMS_TO_TICKS(2)) == pdPASS) {
                uart_.send("Temperature ADC code: ");
                uart_.sendInt(temp_data);
                uart_.sendChar('\n');
            }else {
                uart_.send("Queue doesn't contain data\n");
            }
            notifyAlive();
            vTaskDelay(pdMS_TO_TICKS(800));
            
        }
    }
private:
    UartDriver& uart_;
};



// Main
int main() {
    //Create queue
    adc_queue = xQueueCreate(1, sizeof(uint16_t));

    //Led
    Led led_green;

    // UART init
    Uart2 uart2;
    

    // Adc init (no internal uart functionality)
    Adc_temp adc_temp;

    WatchdogSupervisor::init(1000);

    
    
    //Instantiate tasks
    static BlinkTask<Led> blink(led_green);
    static SensorTask<Adc_temp> sensor(adc_temp);
    static LogTask<Uart2> log(uart2);
    

    WatchdogSupervisor::registerTask(&blink);
    WatchdogSupervisor::registerTask(&sensor);
    WatchdogSupervisor::registerTask(&log);

    // Create tasks
    xTaskCreate(Task::taskEntry, "Process", 256, &blink, 1, nullptr);
    xTaskCreate(Task::taskEntry, "Sensor", 512, &sensor, 1, nullptr);
     xTaskCreate(Task::taskEntry, "Log", 512, &log, 1, nullptr);

    //Create watchdog task
    xTaskCreate(WatchdogSupervisor::taskEntry, "Watchdog",128, nullptr, 2, nullptr);

    __enable_irq();

    vTaskStartScheduler();

    while (1) {}
}
