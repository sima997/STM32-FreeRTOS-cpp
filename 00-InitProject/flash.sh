cp build/firmware.bin openOCD/firmware.bin
cd openOCD/

../../STM32Tools/openOCD/bin/openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -f flash_firmware.tcl