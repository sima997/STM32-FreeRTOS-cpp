#include <cstdint>
#include "adc12_template.hpp"
#include "uart_template.hpp"

/*
using UartDriver = Uart<2,115200>;
using AdcTemp = Adc12<AdcInstance::Adc1,
                     128000000,
                     AdcPsc::div16, 
                     AdcClkMode::SynchronousDiv4, 
                     AdcSpecialChannel::TEMP,
                     AdcChannel::CH17,
                     AdcSamplTime::CYCLES_640_5
                     >;
*/
int main(void) {

    static Uart<2,115200> UartLog;
    UartLog.init(128000000);
    UartLog.send("Hello\r\n");

    static Adc12<AdcInstance::Adc1,
                     AdcPsc::div16, 
                     AdcClkMode::SynchronousDiv4, 
                     AdcSpecialChannel::TEMP,
                     AdcChannel::CH17,
                     AdcSamplTime::CYCLES_640_5,
                     Uart<2,115200>
                     > TempSensor(UartLog);




    //AdcTemp temp_sensor;


    while(1) {

    }
}