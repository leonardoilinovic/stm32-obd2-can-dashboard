	#include <stdio.h>
	#include "lvgl.h"
	#include "lv_port_disp.h"
	#include "main.h"
	#include <string.h>

	// SPI handle (SPI2)
	extern SPI_HandleTypeDef hspi2;
	extern UART_HandleTypeDef huart2;

	// --- Dostupne i u main.c ---
	#define MY_DISP_HOR_RES    480
	#define MY_DISP_VER_RES    320
	void ili9488_init(void);
	void ili9488_set_address(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
	void ili9488_send_data(uint8_t *data, size_t len);
	static void ili9488_clear_screen(void);   /* prototip */


	// Buffers
	static lv_color_t buf1[MY_DISP_HOR_RES * 30];
	static uint8_t line_buf[MY_DISP_HOR_RES * 3];   /* max širina * 3 bajta */
	static lv_display_t * disp;

	// GPIO pinovi
	#define ILI9488_DC_GPIO_Port  GPIOA
	#define ILI9488_DC_Pin        GPIO_PIN_9     // PA9

	#define ILI9488_CS_GPIO_Port  GPIOA
	#define ILI9488_CS_Pin        GPIO_PIN_8     // PA8

	#define ILI9488_RST_GPIO_Port GPIOA
	#define ILI9488_RST_Pin       GPIO_PIN_10    // PA10

	#define ILI9488_LED_GPIO_Port GPIOA
	#define ILI9488_LED_Pin       GPIO_PIN_11    // PA11


	// === Display funkcije ===
	void ili9488_send_cmd(uint8_t cmd) {
		HAL_GPIO_WritePin(ILI9488_DC_GPIO_Port, ILI9488_DC_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(ILI9488_CS_GPIO_Port, ILI9488_CS_Pin, GPIO_PIN_RESET);
		HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
		HAL_GPIO_WritePin(ILI9488_CS_GPIO_Port, ILI9488_CS_Pin, GPIO_PIN_SET);
	}

	void ili9488_send_data(uint8_t *data, size_t len) {

		HAL_GPIO_WritePin(ILI9488_DC_GPIO_Port, ILI9488_DC_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(ILI9488_CS_GPIO_Port, ILI9488_CS_Pin, GPIO_PIN_RESET);
		HAL_SPI_Transmit(&hspi2, data, len, HAL_MAX_DELAY);
		HAL_GPIO_WritePin(ILI9488_CS_GPIO_Port, ILI9488_CS_Pin, GPIO_PIN_SET);
	}

	void ili9488_reset(void) {
		HAL_GPIO_WritePin(ILI9488_RST_GPIO_Port, ILI9488_RST_Pin, GPIO_PIN_RESET);
		HAL_Delay(20);
		HAL_GPIO_WritePin(ILI9488_RST_GPIO_Port, ILI9488_RST_Pin, GPIO_PIN_SET);
		HAL_Delay(120);
	}

	void ili9488_init(void) {
		ili9488_reset();
		// Upali pozadinsko osvjetljenje ako je spojeno na GPIO
		HAL_GPIO_WritePin(ILI9488_LED_GPIO_Port, ILI9488_LED_Pin, GPIO_PIN_SET);

		uint8_t data;
		ili9488_send_cmd(0xE0); // Positive Gamma Correction
		uint8_t pgc[] = {0x00, 0x07, 0x10, 0x09, 0x17, 0x0B, 0x41, 0x89, 0x4B, 0x0A, 0x0C, 0x0E, 0x18, 0x1B, 0x0F};
		ili9488_send_data(pgc, sizeof(pgc));

		ili9488_send_cmd(0xE1); // Negative Gamma Correction
		uint8_t ngc[] = {0x00, 0x17, 0x1A, 0x04, 0x0E, 0x06, 0x2F, 0x45, 0x43, 0x02, 0x0A, 0x09, 0x32, 0x36, 0x0F};
		ili9488_send_data(ngc, sizeof(ngc));

		ili9488_send_cmd(0xC0); data = 0x17; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0xC1); data = 0x12; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0xC5); data = 0x20; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0x36); data = 0x28; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0x3A); data = 0x66; ili9488_send_data(&data, 1); // 18-bit pixel
		ili9488_send_cmd(0xB0); data = 0x80; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0xB1); data = 0xA0; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0xB4); data = 0x02; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0xB6); uint8_t dfc[] = {0x02, 0x02}; ili9488_send_data(dfc, 2);
		ili9488_send_cmd(0xE9); data = 0x00; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0xF7); data = 0xA9; ili9488_send_data(&data, 1);
		ili9488_send_cmd(0x11); // Sleep Out
		HAL_Delay(120);
		ili9488_send_cmd(0x29); // Display ON
	}


	void ili9488_set_address(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
		uint8_t data[4];

		ili9488_send_cmd(0x2A); // Column Address
		data[0] = x1 >> 8; data[1] = x1 & 0xFF;
		data[2] = x2 >> 8; data[3] = x2 & 0xFF;
		ili9488_send_data(data, 4);

		ili9488_send_cmd(0x2B); // Page Address
		data[0] = y1 >> 8; data[1] = y1 & 0xFF;
		data[2] = y2 >> 8; data[3] = y2 & 0xFF;
		ili9488_send_data(data, 4);

		ili9488_send_cmd(0x2C); // Write Memory
	}


	void my_flush_cb(lv_display_t * disp_drv,
					 const lv_area_t * area,
					 uint8_t * px_map)
	{
		int32_t w = area->x2 - area->x1 + 1;
		int32_t h = area->y2 - area->y1 + 1;

		const uint16_t *src = (const uint16_t *)px_map;

		for (int32_t y = 0; y < h; y++) {

			/* 1. Adresa – samo JEDNA linija */
			ili9488_set_address(area->x1, area->y1 + y,
								area->x2, area->y1 + y);

			/* 2. Priprema linije (RGB565 → RGB888) */
			uint8_t *dst = line_buf;
			for (int32_t x = 0; x < w; x++) {
				uint16_t c = *src++;

	#if LV_COLOR_16_SWAP
				c = (c >> 8) | (c << 8);
	#endif
				uint8_t r = (c >> 11) & 0x1F;
				uint8_t g = (c >>  5) & 0x3F;
				uint8_t b =  c        & 0x1F;

				/* R-G-B redoslijed – ako boje ispadnu zamijenjene,
				   jednostavno zamijeni r i b ovdje. */
				*dst++ = (r << 3) | (r >> 2);
				*dst++ = (g << 2) | (g >> 4);
				*dst++ = (b << 3) | (b >> 2);
			}

			/* 3. Slanje – CS LOW samo za ovu liniju */
			HAL_GPIO_WritePin(ILI9488_DC_GPIO_Port, ILI9488_DC_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(ILI9488_CS_GPIO_Port, ILI9488_CS_Pin, GPIO_PIN_RESET);

			HAL_SPI_Transmit(&hspi2, line_buf, w * 3, HAL_MAX_DELAY);

			HAL_GPIO_WritePin(ILI9488_CS_GPIO_Port, ILI9488_CS_Pin, GPIO_PIN_SET);
		}

		lv_display_flush_ready(disp_drv);
	}


	static void ili9488_clear_screen(void)
	{
		/* Postavi cijeli ekran – RGB888 = crno (0,0,0) */
		ili9488_set_address(0, 0, MY_DISP_HOR_RES - 1, MY_DISP_VER_RES - 1);

		HAL_GPIO_WritePin(ILI9488_DC_GPIO_Port, ILI9488_DC_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(ILI9488_CS_GPIO_Port, ILI9488_CS_Pin, GPIO_PIN_RESET);

		uint8_t triplet[3] = {0x00, 0x00, 0x00};
		for (uint32_t i = 0; i < MY_DISP_HOR_RES * MY_DISP_VER_RES; i++)
			HAL_SPI_Transmit(&hspi2, triplet, 3, HAL_MAX_DELAY);

		HAL_GPIO_WritePin(ILI9488_CS_GPIO_Port, ILI9488_CS_Pin, GPIO_PIN_SET);
	}



	// Inicijalizacija prikaza
	void lv_port_disp_init(void) {
		ili9488_init(); // reset i init kontrolera
		ili9488_clear_screen();
		disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
		lv_display_set_flush_cb(disp, my_flush_cb);

		const char *info = "lv_display_set_flush_cb called\r\n";
		HAL_UART_Transmit(&huart2, (uint8_t*)info, strlen(info), HAL_MAX_DELAY);

		size_t buf_size = MY_DISP_HOR_RES * 30 * 2;

		lv_display_set_buffers(disp, buf1, NULL, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

		lv_display_set_default(disp);
		lv_obj_invalidate(lv_scr_act());
	}




