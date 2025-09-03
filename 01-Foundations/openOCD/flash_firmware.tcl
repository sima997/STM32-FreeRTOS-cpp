# Initialize the interface and target
init
reset halt

# Flash the firmware (adjust filename and address)
flash write_image erase "firmware.bin" 0x08000000

# Reset and run the MCU
reset run

# Close OpenOCD
exit
