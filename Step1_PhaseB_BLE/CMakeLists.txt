# Phase B: BLE Communication Data Logger
cmake_minimum_required(VERSION 3.16)

# Include Phase A components
set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_LIST_DIR}/../Step1_PhaseA_DataLogger/components"
    "${CMAKE_CURRENT_LIST_DIR}/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(phase_b_ble_datalogger)