# BobsHackathon

0. Discover Bob through a couple of exemples
    install CLI aswell
1. Try a simple sub project generation
    - to get hands-on experience
    - to go through the PDCA steps req test build
    - to see how to modify/influence the harness, create a skill to constraint the C code for embedded devices

exemple: 3 µCs project communicating with each other using a CAN bus to store data from 3 sensors units in some flash memory
    Master node: ESP32-S3 + SPI sensing unit + CAN bus + 8MB of PSRAM + 16MB of NAND flash
    Slave node 0: STM32-U3 + SPI sensing unit + CAN bus
    Slave node 1: STM32-U3 + SPI sensing unit + CAN bus + BMS + LiPo battery
Generate the plan to design code & test each µC program (3 programs: one for each µC)

2. The real project (raw architecture)
Product A: the previous exemple refined with addons
    3 microcontrollers µC1, µC2, µC3 and their embedded code
Product B: HMI remote logger + vizualyzer + OTA programmer + edge computing
    microcontrollers µC4 (STM32-N6 + LCD touch screen) & µC5 (ESP32-C5) + their embedded code
    
3. The real project (refined architecture)
Build on it to complicate the requirement and taylored them
Product A: can now run CNN models on the edge (sensors filtering & state classification)
Product B: can now do edge training, inference & distribute them to product A 
Output: 
- project code (5 embedded programs)
- skillsets & harness (modes, skills, rules) for embedded C & fault tolerant code, derived from this project experience (outputing UML graph?)

4. Automation testing using ST demo boards

5. Extend the project to include MCP servers for HIL testing & product design lifecycle improvement (Cloud project with watson-x)