# STM32 Firmware - can_auto

This folder contains the STM32 firmware project for the OBD-II/CAN vehicle diagnostics dashboard.

## Target Hardware

- STM32L476RG
- TJA1051 CAN transceiver
- ILI9488 3.5" TFT display
- OBD-II / EOBD vehicle connector

## Main Functions

- Sends OBD-II Mode 01 PID requests over CAN
- Receives ECU responses over bxCAN
- Decodes live vehicle parameters
- Displays values on a TFT screen using LVGL
- Streams decoded data over UART / USB virtual COM port

## Important Source Files

- `Core/Src/main.c` - main application logic, peripheral initialization and main loop
- `Core/Src/obd_data.c` - OBD-II data handling and decoded vehicle parameters
- `Core/Src/ili9488.c` - TFT display driver
- `Core/LVGL_Port/` - LVGL display and input porting files
- `can_auto.ioc` - STM32CubeMX project configuration

## Opening the Project

Open this folder in STM32CubeIDE and import it as an existing STM32CubeIDE project.

The project was created for STM32L476RG and uses STM32 HAL.
