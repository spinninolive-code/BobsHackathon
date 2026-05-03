
3 µCs project communicating with each other using a CAN bus to store data from 3 sensors units in some flash memory
    Master node: ESP32-S3 + SPI sensing unit + CAN bus + 8MB of PSRAM + 16MB of NAND flash
    Slave node 0: STM32-U3 + SPI sensing unit + CAN bus
    Slave node 1: STM32-U3 + SPI sensing unit + CAN bus + BMS + LiPo battery
Generate the plan to design code & test each µC program (3 programs: one for each µC)

Project phase definition:
phase A:


This project has multiple phase, You will create a new directory for this project and multiple sub-directories for each phase. Choose the name wisely.


sensor data temporal offset / bias calibration is done using the Unexpected Maker Pro S3 board as a reference clock source.

This project is an IoT sensors data-logger that stores its acquired content to an SD card and can communicate it to an external station (a device or a web-server).

This project has multiple phase, You will create a new directory for this project and multiple sub-directories for each phase. Choose the name wisely.

It is made up of the following harware components:
- an Unexpected Maker Pro S3 16MB board (full documentation here: https://esp32s3.com/pros3d.html & here https://github.com/UnexpectedMaker/esp32s3/tree/main/series_d)
  This card is fitted with an ESP32-S3 microcontroller, 16MB of NAND flash memory, 8MB of PSRAM, 2MB of SRAM as the main active components. The microcontroller datasheet is here: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- two 10-DoF sensors-module boards each made up of
    - an Invensense ICM-42688-P accelerometer/gyro sensors, full documentation here (https://invensense.tdk.com/download-resource/ds-000347-icm-42688-p-datasheet, and https://invensense.tdk.com/download-resource/ug-icm-42688-p-software-user-guide-icm-42688-p)
    - a Memsic MMC5983MA magnetometer, full documentation here (https://www.memsic.com/magnetometer-5 and here https://www.memsic.com/Public/Uploads/uploadfile/files/20220119/MMC5983MADatasheetRevA.pdf)
    - an STMicro LPS22HB pressure sensor, full documentation here (https://www.st.com/en/mems-and-sensors/lps22hb.html#overview, https://www.st.com/resource/en/datasheet/lps22hb.pdf, and here https://github.com/STMicroelectronics/stm32-lps22hb)
    
- an SD card module (full documentation here: https://www.adafruit.com/product/4682)
- a 220mAh LiPo battery
- tri DIP Switches (3 on/off mrchanical switch in a row) to select the mode to be used, ALCOSWITCH DIP ADEN03TTU04 full documentation here (https://www.te.com/en/product-2454982-2.html)
- a mechanical switch to turn on/off the battery power.

These components are connected the following way: 
    - the SD card module is connected to the Unexpected Maker Pro S3 board (digital pins)
    - the DIP switches are connected to the Unexpected Maker Pro S3 board (digital pins) 
    - each sensor module is connected to the Unexpected Maker Pro S3 board using the SPI bus (digital pins 35, 36, 37) and a dedicated CS pin (two digital pins to be chosen)

Choose the pins to be used on the Unexpected Maker Pro S3 to attach to for the sensors modules, switches and the SD card.
choose the SD card pin wisely in order to maximanize the transfer rate speed (sdio, number of lanes ..), but remember that the SPI bus is not shared with the SD card module

Acquisition buffering and storage:
On the Unexpected Maker Pro S3 board, there are both PSRAM & NAND flash, they will be used to buffer and temporarily store the acquired data before writing them to the SD card according to the selected mode and the size constraints of both the PSRAM, the NAND flash. The size and methodology use to buffer the data must be carefully chosen to avoid data loss and to ensure the best possible performance.
When the user press the "store" button (or set the switches to the store state), the data are written to the SD card and the buffer is cleared. This is also true during acquisition mode (start mode) when the NAND flash reaches its limit (automatic SD card write to prevent exceeding the NAND memory limit and loosing data)

Battery monitoring and charging:
The microcontroller handle the battery voltage monitoring and charging state. Charging is done using the board usb port. The battery is charged when the board is connected to a power source (usb or battery) and the battery voltage is below the charging threshold.

Project phase definition:
phase A: the acquired data are stored locally on the SD card attached to the Unexpected Maker Pro S3 board.
phase B: Add the following functionnalities to the phase A : the acquired data are sent to an android app (to be developed) using BLE.
   - the android app is compatible with old android phones (Android 5.0.1, samsung galaxy S4 GT-I9500) 
   - the android app can store the received data locally on the phone on the main device drive or an internal SD card
   - the android app can start/stop the data acquisition on the Unexpected Maker Pro S3 board using BLE
phase C: add the following functionnalities to the android app of the phase B & modify accordingly the Unexpected Maker Pro S3 microcontroller code
   - the android app can visualize the data in real time using a chart
   - the communication between the android app and the Unexpected Maker Pro S3 board is using WiFi (ESP32 WiFi module). The mobile phone is the access point (using potentially wifi direct) and the Unexpected Maker Pro S3 board is the client.

For both phase B and C: the app will be installed using the mobile phone usb port or the internal SD card, NOT using the play store.

User/system mode definition:
For project phase A: 
    The start/stop sensor acquisition is triggered by selecting a mode using the DIP switch wired to the Unexpected Maker Pro S3 board (digital pins).
    The data is stored in a CSV file on the SD card (littleFS format) and the file name is generated using the date and time of the acquisition.
For phase B and C:
    The android app will be able to start/stop the data acquisition on the Unexpected Maker Pro S3 board using BLE and visualize the user/system mode as an HMI (press start/stop set the mode to "acquisition" and press stop to set the mode to "idle").
For all phases: create an app interface to start and close the BLE or the wifi connection properly.

Generate the plan to design code & test this project.
Make an seperate FSD (functionnal specification document) file for each phases the project.
Code it in C and/or C++ with real-time constraints. You can optimize the task preemption by using freeRTOS or not. You decide what's best in order to achieve the best sensors acquisition and have the finest recordings and best microcontroller performance.
Name each sensors data wisely so that each sensors variables can be identified easily by 
- sensor module name
- sensor type
- sensor axis or unit of measurement
ESP IDF will likely be used for the ESP32-S3 board.

Make it work

Drill me with questions until we reach an understanding of the project.
