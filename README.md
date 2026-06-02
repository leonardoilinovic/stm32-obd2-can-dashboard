# STM32 OBD-II/CAN Vehicle Diagnostics Dashboard

A standalone embedded vehicle diagnostics system based on an STM32 microcontroller.  
The device connects to a car through the EOBD/OBD-II connector, communicates with the ECU over the CAN bus, displays live vehicle data on a TFT screen, and streams the data to a Python dashboard for visualization and CSV logging.

## Project overview

This project was developed as my BSc thesis at the University of Rijeka, Faculty of Engineering.

The system reads live vehicle parameters such as:

- engine RPM
- vehicle speed
- coolant temperature
- intake air temperature
- throttle position
- MAF
- MAP
- lambda sensor value
- engine runtime
- estimated fuel rate

The embedded part runs on an STM32L476RG microcontroller and communicates over CAN using the OBD-II/EOBD protocol.  
A 3.5" ILI9488 TFT display with LVGL is used for local real-time visualization, while a Python desktop dashboard receives data over UART/virtual COM port and plots the values live.

## System architecture

```text
Car OBD-II / EOBD Port
        |
        | CAN bus
        v
TJA1051 CAN Transceiver
        |
        v
STM32L476RG
   |              |
   | SPI          | UART / USB Virtual COM Port
   v              v
ILI9488 TFT       Python Dashboard
LVGL GUI          CSV Logger + Live Graphs
