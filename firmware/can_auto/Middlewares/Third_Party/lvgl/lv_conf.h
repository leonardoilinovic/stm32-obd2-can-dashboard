/**
 * @file lv_conf.h
 * Configuration file for LVGL v9.x
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   GENERAL SETTINGS
 *====================*/
#define LVGL_VERSION_MAJOR 9
#define LV_CONF_INCLUDE_SIMPLE 1

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DEF_REFR_PERIOD     33    // ~30 FPS
#define LV_DPI_DEF             130

#define LV_USE_OS              LV_OS_NONE
#define LV_USE_USER_DATA       0

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH         16
#define BYTES_PER_PIXEL 2
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_CHROMA_KEY    lv_color_hex(0x00ff00)

/*====================
   DRAW SETTINGS
 *====================*/
#define LV_USE_METER 1
#define LV_USE_DRAW_SW              1
#define LV_DRAW_SW_SUPPORT_RGB565   1
#define LV_DRAW_SW_COMPLEX          1
#define LV_DRAW_SW_DRAW_UNIT_CNT    1

#define LV_USE_GPU_VG_LITE          0
#define LV_USE_PARALLEL_DRAW_DEBUG  0

/*====================
   MEMORY SETTINGS
 *====================*/
#define LV_USE_STDLIB_MALLOC        LV_STDLIB_BUILTIN
#define LV_MEM_SIZE                 (32U * 1024U)  // adjust to ~32K if needed

/*====================
   WIDGETS
 *====================*/
#define LV_USE_LABEL       1
#define LV_USE_BUTTON      1
#define LV_USE_LIST        1
#define LV_USE_SLIDER      1
#define LV_USE_CHECKBOX    1
#define LV_USE_IMAGE       1
#define LV_USE_TABVIEW     1
#define LV_USE_WIN         1
#define LV_USE_SPINNER     1
#define LV_USE_MSGBOX      1
#define LV_USE_LED         1
#define LV_USE_LINE        1
#define LV_USE_KEYBOARD    1
#define LV_USE_CANVAS      1
#define LV_USE_MENU        1
#define LV_USE_ARC         1

/*====================
   THEMES
 *====================*/
#define LV_USE_THEME_DEFAULT       1
#define LV_THEME_DEFAULT_DARK      0
#define LV_THEME_DEFAULT_GROW      1

/*====================
   LAYOUTS
 *====================*/
#define LV_USE_FLEX       1
#define LV_USE_GRID       1

/*====================
   FONTS
 *====================*/
#define LV_FONT_MONTSERRAT_10  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_28  1
#define LV_FONT_DEFAULT            &lv_font_montserrat_14



/*====================
   LOGGING (Optional)
 *====================*/
#define LV_USE_LOG        0

/*====================
   PLATFORM DRIVERS
 *====================*/
#define LV_USE_NUTTX      0
#define LV_USE_LIBUV      0

#endif /* LV_CONF_H */
