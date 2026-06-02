#include "main.h"     // za HAL_GPIO_TogglePin
#include "lvgl.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdint.h>

// Fallback symbols
#ifndef LV_SYMBOL_FORWARD
# define LV_SYMBOL_FORWARD  LV_SYMBOL_RIGHT
#endif
#ifndef LV_SYMBOL_DROP
# define LV_SYMBOL_DROP     LV_SYMBOL_TINT
#endif
#ifndef LV_SYMBOL_DROPLET
# define LV_SYMBOL_DROPLET  ""
#endif

#define SCR_W      480
#define SCR_H      320
#define RPM_MAX    6000
#define SPD_MAX    200

#define ARC_R      55

// Ručni pomaci za pozicioniranje brojeva
static const int16_t OFFSET_X = 105;  // negativno = lijevo, pozitivno = desno
static const int16_t OFFSET_Y = +  155; // negativno = gore,    pozitivno = dolje

// Forward
static inline int32_t deg_from_val(int32_t v, int32_t max);
static inline void pol2xy(lv_obj_t * parent, int32_t deg, int32_t R, int32_t * x, int32_t * y);
static inline void set_label(lv_obj_t * l, const char * fmt, ...);
static void create_scale(lv_obj_t * parent, int32_t max);
static void screen_event_cb(lv_event_t * e);

// Globalni objekti
static lv_obj_t *cont_rpm;
static lv_obj_t *cont_spd;

static lv_obj_t *arc_rpm, *lab_rpm_big;
static lv_obj_t *arc_spd, *lab_spd_big;
static lv_obj_t *lab_speed, *lab_cool, *lab_intk, *lab_thr,
                *lab_fuel, *lab_map, *lab_lambda, *lab_run;
static bool show_rpm = true;

// Pretvori vrijednost u kut
static inline int32_t deg_from_val(int32_t v, int32_t max) {
    return 270 - (360LL * v) / max;
}

// Polar → kartiz + apsolutni offset
static inline void pol2xy(lv_obj_t *parent, int32_t deg, int32_t R, int32_t *x, int32_t *y) {
    // Dinamički centar containera
    lv_coord_t w = lv_obj_get_width(parent);
    lv_coord_t h = lv_obj_get_height(parent);
    float cx = w / 2.0f;
    float cy = h / 2.0f;

    float rad = deg * M_PI / 180.0f;
    *x = (int32_t)(cx + R * cosf(rad)) + OFFSET_X;
    *y = (int32_t)(cy - R * sinf(rad)) + OFFSET_Y;
}

// formatirani label
static inline void set_label(lv_obj_t *l, const char *fmt, ...) {
    char buf[48];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    lv_label_set_text(l, buf);
}

// Skaliranje brojeva i črtica
static void create_scale(lv_obj_t *parent, int32_t max) {
    const lv_font_t *f_big = &lv_font_montserrat_14;
    const lv_font_t *f_sml = &lv_font_montserrat_10;
    int32_t step  = (max == RPM_MAX ? 500 : 20);
    int32_t major = (max == RPM_MAX ? 1000 : 40);

    // Radijusi u odnosu na ARC_R
    const int32_t R_tick_in  = ARC_R + 5;
    const int32_t R_tick_out = ARC_R + 12;
    const int32_t R_txt      = ARC_R + 28;

    for (int32_t v = 0; v <= max; v += step) {
        int32_t d = deg_from_val(v, max);

        // 1) Minor tickovi (crtamo strelicu/kratice)
        if(v % major) {
            int32_t x1, y1, x2, y2;
            pol2xy(parent, d, R_tick_in,  &x1, &y1);
            pol2xy(parent, d, R_tick_out, &x2, &y2);
            lv_point_precise_t pts[2] = { { x1, y1 }, { x2, y2 } };
            lv_obj_t *ln = lv_line_create(parent);
            lv_line_set_points(ln, pts, 2);
            lv_obj_set_style_line_width(ln, 2, 0);
            lv_obj_set_style_line_color(ln, lv_color_white(), 0);
        }

        // 2) Brojke
        int32_t x, y;
        pol2xy(parent, d, R_txt, &x, &y);

        // 2a) Pomakni „0” lijevo i „max” (npr. 6000) desno, da se ne preklapaju
        if(v == 0) {
            x -= 10;  // guramo „0” malo ulijevo
        }
        else if(v == max) {
            x += 10;  // guramo „6000” malo udesno
        }

        // 3) Kreiraj labelu i centriraj je na (x,y)
        lv_obj_t *lbl = lv_label_create(parent);
        lv_label_set_text_fmt(lbl, "%ld", (long)v);
        lv_obj_set_style_text_font(lbl, (v % major ? f_sml : f_big), 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

        // Oduzmemo pola širine/visine tako da labela bude centrirana
        lv_coord_t lw = lv_obj_get_width(lbl);
        lv_coord_t lh = lv_obj_get_height(lbl);
        lv_obj_set_pos(lbl, x - (lw / 2), y - (lh / 2));
    }
}


// Tap callback
static void screen_event_cb(lv_event_t *e) {
    (void)e;
    show_rpm = !show_rpm;

    if(show_rpm) {
        lv_obj_clear_flag(cont_rpm, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag  (cont_spd, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag   (cont_rpm, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag (cont_spd, LV_OBJ_FLAG_HIDDEN);
    }

    // Debug LED
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}

void ux_init(void) {
    /* 1) Dobavi glavni ekran i postavi pozadinu */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa  (scr, LV_OPA_COVER,    0);

    /* 2) Kreiraj RPM container (lijeva polovica) */
    cont_rpm = lv_obj_create(scr);
    lv_obj_remove_style_all(cont_rpm);        // ukloni border & bg
    lv_obj_set_size(cont_rpm, SCR_W/2, SCR_H);
    lv_obj_align(cont_rpm, LV_ALIGN_LEFT_MID, 0, 0);

    /* 2a) Crtaj scale, ARC i veliku brojku unutar RPM containera */
    create_scale(cont_rpm, RPM_MAX);

    arc_rpm = lv_arc_create(cont_rpm);
    lv_obj_set_size(arc_rpm, ARC_R*2, ARC_R*2);
    lv_obj_align(arc_rpm, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_range      (arc_rpm, 0, RPM_MAX);
    lv_arc_set_bg_angles  (arc_rpm, 0, 360);
    lv_arc_set_rotation   (arc_rpm, 90);
    lv_arc_set_mode       (arc_rpm, LV_ARC_MODE_NORMAL);
    lv_obj_remove_style   (arc_rpm, NULL, LV_PART_KNOB);

    lab_rpm_big = lv_label_create(cont_rpm);
    lv_obj_set_style_text_font(lab_rpm_big, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lab_rpm_big, lv_color_white(), 0);
    lv_obj_align(lab_rpm_big, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lab_rpm_big, "0");

    /* 2b) Zakači kratki tap na RPM container */
    lv_obj_add_flag(cont_rpm, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont_rpm, screen_event_cb, LV_EVENT_SHORT_CLICKED, NULL);

    /* 3) Kreiraj Speed container (isto mjesto) */
    cont_spd = lv_obj_create(scr);
    lv_obj_remove_style_all(cont_spd);
    lv_obj_set_size(cont_spd, SCR_W/2, SCR_H);
    lv_obj_align(cont_spd, LV_ALIGN_LEFT_MID, 0, 0);

    create_scale(cont_spd, SPD_MAX);

    arc_spd = lv_arc_create(cont_spd);
    lv_obj_set_size(arc_spd, ARC_R*2, ARC_R*2);
    lv_obj_align(arc_spd, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_range      (arc_spd, 0, SPD_MAX);
    lv_arc_set_bg_angles  (arc_spd, 0, 360);
    lv_arc_set_rotation   (arc_spd, 90);
    lv_arc_set_mode       (arc_spd, LV_ARC_MODE_NORMAL);
    lv_obj_remove_style   (arc_spd, NULL, LV_PART_KNOB);

    lab_spd_big = lv_label_create(cont_spd);
    lv_obj_set_style_text_font(lab_spd_big, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lab_spd_big, lv_color_white(), 0);
    lv_obj_align(lab_spd_big, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lab_spd_big, "0");

    /* 3b) Zakači kratki tap na Speed container */
    lv_obj_add_flag(cont_spd, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont_spd, screen_event_cb, LV_EVENT_SHORT_CLICKED, NULL);

    /* 4) Početno stanje: RPM vidljiv, Speed skriven */
    lv_obj_clear_flag(cont_rpm, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag  (cont_spd, LV_OBJ_FLAG_HIDDEN);

    /* 5) Desna statična kolona s tekstom */
    lv_obj_t *info = lv_obj_create(scr);
    lv_obj_set_size(info, 210, SCR_H - 20);
    lv_obj_align(info, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(info, 4, 0);
    lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(info, 6, 0);

    #define NEW_LAB(id, txt) \
      id = lv_label_create(info); \
      lv_label_set_text(id, txt); \
      lv_obj_set_style_text_color(id, lv_color_white(), 0)

    NEW_LAB(lab_speed , LV_SYMBOL_FORWARD" Speed: -- km/h");
    NEW_LAB(lab_cool  , LV_SYMBOL_TINT   " Coolant: --°C");
    NEW_LAB(lab_thr   , LV_SYMBOL_CHARGE " Throttle: --%");
    NEW_LAB(lab_intk  , "Intake:  --°C");
    NEW_LAB(lab_lambda, "Lambda: -- V");
    NEW_LAB(lab_fuel  , LV_SYMBOL_DROPLET" Fuel: -- L/h");
    NEW_LAB(lab_map   , "MAP: -- kPa");
    NEW_LAB(lab_run   , "Runtime: -- s");
}

// Runtime API
void ui_set_rpm(uint16_t rpm) {
    set_label(lab_rpm_big, "%u", rpm);
    lv_arc_set_value(arc_rpm, rpm);
}

void ui_set_speed(uint8_t v) {
    set_label(lab_speed, LV_SYMBOL_FORWARD" Speed: %u km/h", v);
    set_label(lab_spd_big, "%u", v);
    lv_arc_set_value(arc_spd, v);
}

void ui_set_temp(int8_t c, int8_t i) {
    set_label(lab_cool, LV_SYMBOL_TINT" Coolant: %d°C", c);
    set_label(lab_intk, "Intake:  %d°C", i);
    lv_color_t col = lv_palette_main((c >= 100) ? LV_PALETTE_RED :
                                     (c >= 80) ? LV_PALETTE_ORANGE : LV_PALETTE_GREEN);
    lv_obj_set_style_text_color(lab_cool, col, 0);
}

void ui_set_throttle(uint8_t p)    { set_label(lab_thr , LV_SYMBOL_CHARGE" Throttle: %u%%", p); }
void ui_set_fuel_rate(float f)     { set_label(lab_fuel, LV_SYMBOL_DROPLET" Fuel: %.2f L/h", f); }
void ui_set_map(uint8_t k)         { set_label(lab_map , "MAP: %u kPa", k); }
void ui_set_lambda(float v)        { set_label(lab_lambda, "Lambda: %.2f V", v); }
void ui_set_runtime(uint32_t s)    { set_label(lab_run , "Runtime: %lus", s); }
