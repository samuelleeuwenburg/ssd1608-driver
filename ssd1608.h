#ifndef SSD1608_H
#define SSD1608_H

#include <stdio.h>
#include <stdint.h>

typedef int (*spi_write_blocking_cb)(const uint8_t *data, size_t length);
typedef uint8_t (*pin_get)(void);
typedef void (*pin_set)(uint8_t value);
typedef void (*delay)(size_t ms);

typedef enum {
    ssd1608_250x122,
    ssd1608_400x300,
} ssd1608_type;

typedef enum {
    black_white_red,
    black_white_yellow,
    black_white
} ssd1608_colors;

typedef enum {
    white_px,
    black_px,
    color_px,
} pixel_value;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t cols;
    uint16_t rows;
} dimensions;

typedef struct ssd1608 {
    ssd1608_type type;
    ssd1608_colors colors;
    pixel_value border_color;
    uint8_t *buffer;
    uint8_t *alt_buffer;
    spi_write_blocking_cb write_blocking;
    pin_get busy_get;
    pin_set cs_set;
    pin_set reset_set;
    pin_set dc_set;
    delay delay;
} ssd1608;

ssd1608 ssd1608_new(
    ssd1608_type type,
    ssd1608_colors colors,
    spi_write_blocking_cb write_blocking,
    pin_get busy_get,
    pin_set cs_set,
    pin_set reset_set,
    pin_set dc_set,
    delay delay
);

void ssd1608_destroy(ssd1608 *display);
void ssd1608_render(ssd1608 *display);

uint16_t ssd1608_get_width(ssd1608 *display);
uint16_t ssd1608_get_height(ssd1608 *display);

void ssd1608_set_pixel(ssd1608 *display, size_t x, size_t y, pixel_value value);
void ssd1608_set_pixels(ssd1608 *display, size_t x, size_t y, size_t w, size_t h, pixel_value value);
void ssd1608_draw_bitmap(ssd1608 *display, size_t x, size_t y, size_t w, size_t h, pixel_value value, uint8_t *bitmap);

#endif
