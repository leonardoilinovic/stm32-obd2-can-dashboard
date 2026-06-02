#include "lv_port_indev.h"
#include "lvgl.h"
#include "lv_port_disp.h"  // za MY_DISP_* makroe
#include <stdbool.h>
#include <stdint.h>

#define MY_DISP_HOR_RES 480
#define MY_DISP_VER_RES 320


// Helper za mapiranje 12-bit raw ADC u piksele
static inline uint16_t map_raw_to_px(uint16_t raw, uint16_t in_min, uint16_t in_max,
                                     uint16_t out_min, uint16_t out_max) {
    if (raw < in_min) raw = in_min;
    if (raw > in_max) raw = in_max;
    return (uint16_t)(((uint32_t)(raw - in_min) * (out_max - out_min)) /
                      (in_max - in_min) + out_min);
}

extern SPI_HandleTypeDef hspi2;
#define TOUCH_CS_LOW()   HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET)
#define TOUCH_CS_HIGH()  HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET)

// LVGL v9 read-cb: puni data->point i data->state, vraća false ako nema više podataka
static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t x = 0, y = 0;
    bool pressed = (HAL_GPIO_ReadPin(TOUCH_IRQ_GPIO_Port, TOUCH_IRQ_Pin) == GPIO_PIN_RESET);

    if (pressed) {
        uint8_t cmd, buf[2];

        // Čitanje X
        cmd = 0xD0;
        TOUCH_CS_LOW();
        HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
        HAL_SPI_Receive(&hspi2, buf, 2, HAL_MAX_DELAY);
        TOUCH_CS_HIGH();
        uint16_t rawX = ((buf[0] & 0x0F) << 8) | buf[1];

        // Čitanje Y
        cmd = 0x90;
        TOUCH_CS_LOW();
        HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
        HAL_SPI_Receive(&hspi2, buf, 2, HAL_MAX_DELAY);
        TOUCH_CS_HIGH();
        uint16_t rawY = ((buf[0] & 0x0F) << 8) | buf[1];

        x = map_raw_to_px(rawX, 0, 4095, 0, MY_DISP_HOR_RES);
        y = map_raw_to_px(rawY, 0, 4095, 0, MY_DISP_VER_RES);
    }

    data->state   = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    data->point.x = x;
    data->point.y = y;
}

void lv_port_indev_init(void) {
    lv_indev_t *indev_tp = lv_indev_create();
    lv_indev_set_type(indev_tp, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_tp, touchpad_read);
}

