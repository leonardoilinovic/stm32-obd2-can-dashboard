#ifndef LV_INTERFACE_H
#define LV_INTERFACE_H

#include <stdint.h>
#include "lvgl.h"

/**
 *  Inicijalizira korisničko sučelje (pozvati jednom u main nakon lv_port_disp_init()).
 */
void ux_init(void);

/* ==== API funkcije za dinamičko ažuriranje podataka ==== */

/**
 * Postavi broj okretaja motora (RPM).
 */
void ui_set_rpm(uint16_t rpm);

/**
 * Postavi brzinu vozila (km/h).
 */
void ui_set_speed(uint8_t speed);

/**
 * Postavi temperaturu rashladne tekućine i usisanog zraka (°C).
 */
void ui_set_temp(int8_t coolant, int8_t intake);

/**
 * Postavi otvaranje leptira gasa (postotak).
 */
void ui_set_throttle(uint8_t throttle);

/**
 * Postavi trenutnu potrošnju goriva (L/h).
 */
void ui_set_fuel_rate(float lph);

/**
 * Postavi apsolutni tlak u usisu (kPa).
 */
void ui_set_map(uint8_t map);

/**
 * Postavi lambda sondu (napon u voltima).
 */
void ui_set_lambda(float v);

/**
 * Postavi vrijeme rada motora od pokretanja (sekunde).
 */
void ui_set_runtime(uint32_t s);

#endif /* LV_INTERFACE_H */
