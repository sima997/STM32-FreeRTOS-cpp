


# Final Project: Smart Sensor Hub
## Tasks to implemet
1. Task A: Sensor acquisition
   - RAII ADC driver 
   - Reads temperature from on-chip sensor + simulated humidity
   - Pushes results into a FreeRTOS queue
2. **Task B: Command handler**
   - UART task with RAII driver
   - Accepts commands:
     - `LED ON/OFF`
     - `STATUS`->prints last sensor values
3. **Task C: Data logger**
   - Receives sensor data from queue
   - Formats into text and prints via UART
   - Uses `snprintf` for safe formating
4. **Task D: Watchdog/heartbeat**
   - Kicks IWDG only if all tasks report health
   - Uses FreeRTOS **event groups:** each task set its "I'm alive bit"