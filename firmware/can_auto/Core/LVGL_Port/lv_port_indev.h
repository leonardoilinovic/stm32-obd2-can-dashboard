#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#include "lvgl.h"
#include "stm32l4xx_hal.h"

// SPI handle za touch (definiciju imaš u main.c ili nekom portu)
extern SPI_HandleTypeDef hspi3;

// Pinovi na koje si spojio touch
#define TOUCH_CS_GPIO_Port     GPIOA
#define TOUCH_CS_Pin           GPIO_PIN_12
#define TOUCH_IRQ_GPIO_Port    GPIOC
#define TOUCH_IRQ_Pin          GPIO_PIN_0

// Ovo pozovi nakon lv_port_disp_init()
void lv_port_indev_init(void);

#endif // LV_PORT_INDEV_H
