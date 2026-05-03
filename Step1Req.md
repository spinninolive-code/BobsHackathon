This project is a sensors data-logger that stores its acquired content to an SD card.
It is made up of the following components:
- a Teensy 3.1 microcontroller board (full documentation here: https://www.pjrc.com/teensy/3.1.html)
- a sensors board made up of(a Bosch BMA250 accelerometer, full documentation here (https://github.com/TinyCircuits/TinyCircuits-TinyShield-Accelerometer-ASD2611 and https://learn.tinycircuits.com/Sensors/Accelerometer_TinyShield_Tutorial/)
- an SD card module (full documentation here: https://www.adafruit.com/product/4682)

The start/stop sensor acquisition is triggered by a connected tri-way mechanical switch to the Teensy board (digital pins).
The data is stored in a CSV file on the SD card.

Generate the plan to design code & test this project.
Choose the pins for the sensors and the SD card.
Make it work

Drill me with questions.

3 µCs project communicating with each other using a CAN bus to store data from 3 sensors units in some flash memory
    Master node: ESP32-S3 + SPI sensing unit + CAN bus + 8MB of PSRAM + 16MB of NAND flash
    Slave node 0: STM32-U3 + SPI sensing unit + CAN bus
    Slave node 1: STM32-U3 + SPI sensing unit + CAN bus + BMS + LiPo battery
Generate the plan to design code & test each µC program (3 programs: one for each µC)