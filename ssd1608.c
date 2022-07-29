#include "ssd1608.h"
#include <stdlib.h>

#define PAGE_LENGTH 8
#define LUTS_LENGTH 70

#define MASTER_ACTIVATE 0x20
#define SOFT_RESET      0x12
#define DRIVER_CONTROL  0x01
#define DATA_MODE       0x11
#define SET_RAM_X_POS   0x44
#define SET_RAM_Y_POS   0x45
#define SET_RAM_X_COUNT 0x4e
#define SET_RAM_Y_COUNT 0x4f
#define SOURCE_VOLTAGE  0x04
#define DISP_CTRL2      0x22
#define WRITE_DUMMY     0x3a
#define WRITE_GATELINE  0x3b
#define WRITE_VCOM      0x2c
#define WRITE_LUT       0x32
#define WRITE_BORDER    0x3c
#define WRITE_RAM       0x24
#define WRITE_ALT_RAM   0x26

const uint8_t luts_black[LUTS_LENGTH] = {
    0b01001000, 0b10100000, 0b00010000, 0b00010000, 0b00010011, 0b00000000, 0b00000000,
    0b01001000, 0b10100000, 0b10000000, 0b00000000, 0b00000011, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b01001000, 0b10100101, 0b00000000, 0b10111011, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0x10, 0x04, 0x04, 0x04, 0x04,
    0x10, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x08, 0x08, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t luts_red[LUTS_LENGTH] = {
    0b01001000, 0b10100000, 0b00010000, 0b00010000, 0b00010011, 0b00000000, 0b00000000,
    0b01001000, 0b10100000, 0b10000000, 0b00000000, 0b00000011, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b01001000, 0b10100101, 0b00000000, 0b10111011, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0x40, 0x0C, 0x20, 0x0C, 0x06,
    0x10, 0x08, 0x04, 0x04, 0x06,
    0x04, 0x08, 0x08, 0x10, 0x10,
    0x02, 0x02, 0x02, 0x40, 0x20,
    0x02, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t luts_yellow[LUTS_LENGTH] = {
    0b11111010, 0b10010100, 0b10001100, 0b11000000, 0b11010000, 0b00000000, 0b00000000,
    0b11111010, 0b10010100, 0b00101100, 0b10000000, 0b11100000, 0b00000000, 0b00000000,
    0b11111010, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b11111010, 0b10010100, 0b11111000, 0b10000000, 0b01010000, 0b00000000, 0b11001100,
    0b10111111, 0b01011000, 0b11111100, 0b10000000, 0b11010000, 0b00000000, 0b00010001,
    0x40, 0x10, 0x40, 0x10, 0x08,
    0x08, 0x10, 0x04, 0x04, 0x10,
    0x08, 0x08, 0x03, 0x08, 0x20,
    0x08, 0x04, 0x00, 0x00, 0x10,
    0x10, 0x08, 0x08, 0x00, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
};

static dimensions get_dimensions(ssd1608_type t) {
    dimensions d = { .width = 0, .height = 0, .cols = 0, .rows = 0 };

    switch (t) {
    case ssd1608_250x122:
        d.width = 250;
        d.height = 122;
        d.cols = 136;
        d.rows = 250;
        break;
    case ssd1608_400x300:
        d.width = 400;
        d.height = 300;
        d.cols = 400;
        d.rows = 300;
        break;
    }

    return d;
}

uint16_t ssd1608_get_width(ssd1608 *display) {
    dimensions d = get_dimensions(display->type);
    return d.width;
}

uint16_t ssd1608_get_height(ssd1608 *display) {
    dimensions d = get_dimensions(display->type);
    return d.width;
}

static size_t get_buffer_len(ssd1608_type t) {
    dimensions d = get_dimensions(t);
    return (d.cols / PAGE_LENGTH) * d.rows;
}

ssd1608 ssd1608_new(
    ssd1608_type type,
    ssd1608_colors colors,
    spi_write_blocking_cb write_blocking,
    pin_get busy_get,
    pin_set cs_set,
    pin_set reset_set,
    pin_set dc_set,
    delay delay
) {
    size_t buffer_len = get_buffer_len(type);

    uint8_t *buffer = calloc(buffer_len, sizeof(uint8_t));
    uint8_t *alt_buffer = calloc(buffer_len, sizeof(uint8_t));

    ssd1608 display = {
        .type = type,
        .colors = colors,
        .border_color = white_px,
        .buffer = buffer,
        .alt_buffer = alt_buffer,
        .write_blocking = write_blocking,
        .busy_get = busy_get,
        .cs_set = cs_set,
        .reset_set = reset_set,
        .dc_set = dc_set,
        .delay = delay,
    };

    return display;
}

void ssd1608_destroy(ssd1608 *display) {
    free(display->buffer);
    free(display->alt_buffer);
}

static void send_data(ssd1608 *display, const uint8_t *data, size_t length) {
    display->dc_set(1);
    display->cs_set(0);
    display->write_blocking(data, length);
    display->cs_set(1);
}

static void send_command(ssd1608 *display, uint8_t command, const uint8_t *data, size_t length) {
    uint8_t buf[1] = { command };
    display->dc_set(0);
    display->cs_set(0);
    display->write_blocking(buf, 1);
    display->cs_set(1);

    if (data != NULL) {
        send_data(display, data, length);
    }
}

static void wait_for_busy_pin(ssd1608 *display) {
    while (display->busy_get()) {
        display->delay(10);
    }
}

static void setup(ssd1608 *display) {
    display->reset_set(0);
    display->delay(500);
    display->reset_set(1);
    display->delay(500);

    send_command(display, SOFT_RESET, NULL, 0);
    display->delay(1000);
    wait_for_busy_pin(display);
}


void ssd1608_set_pixel(ssd1608 *display, size_t original_x, size_t original_y, pixel_value value) {
    dimensions d = get_dimensions(display->type);
    size_t buffer_len = get_buffer_len(display->type);

    // return early when out of bounds
    if (original_x > d.width || original_y > d.height) {
        return;
    }

    // apply rotation
    size_t y = display->type == ssd1608_250x122 ? original_x : original_y;
    size_t x = display->type == ssd1608_250x122 ? d.width - 1 - original_y : original_x;

    // find the correct byte/bit
    size_t byte = x / PAGE_LENGTH + 1 + y * (buffer_len / d.rows);
    size_t shifted_value = 1 << (7 - x % PAGE_LENGTH);

    switch (value) {
    case white_px:
        display->buffer[byte] = display->buffer[byte] | shifted_value;
        display->alt_buffer[byte] = display->alt_buffer[byte] & ~shifted_value;
        break;
    case black_px:
        display->buffer[byte] = display->buffer[byte] & ~shifted_value;
        display->alt_buffer[byte] = display->alt_buffer[byte] & ~shifted_value;
        break;
    case color_px:
        display->buffer[byte] = display->buffer[byte] | shifted_value;
        display->alt_buffer[byte] = display->alt_buffer[byte] | shifted_value;
        break;
    }
}

void ssd1608_draw_bitmap(ssd1608 *display, size_t start_x, size_t start_y, size_t w, size_t h, pixel_value value, uint8_t *bitmap) {
    for (size_t x = 0; x < w; ++x) {
        uint8_t byte = bitmap[x];
        for (size_t y = 0; y < h; ++y) {
            uint8_t bit = byte >> y & 0x01;
            if (bit == 1) {
                ssd1608_set_pixel(display, start_x + x, start_y + y, value);
            }
        }
    }
}

void ssd1608_set_pixels(ssd1608 *display, size_t start_x, size_t start_y, size_t w, size_t h, pixel_value value) {
    for (size_t x = 0; x < w; ++x) {
        for (size_t y = 0; y < h; ++y) {
            ssd1608_set_pixel(display, start_x + x, start_y + y, value);
        }
    }
}

void ssd1608_render(ssd1608 *display) {
    dimensions d = get_dimensions(display->type);
    size_t buffer_len = get_buffer_len(display->type);

    setup(display);

    uint8_t buff[4] = {0x00, 0x00, 0x00, 0x00};

    buff[0] = 0x54;
    send_command(display, 0x74, buff, 1); // # Set Analog Block Control

    buff[0] = 0x3b;
    send_command(display, 0x7e, buff, 1);  // # Set Digital Block Control

    buff[0] = (d.rows - 1) & 0xff;
    buff[1] = (d.rows - 1) >> 8;
    buff[2] = 0x00;
    send_command(display, DRIVER_CONTROL, buff, 3);

    buff[0] = 0x1b;
    send_command(display, WRITE_DUMMY, buff, 1);

    buff[0] = 0x0b;
    send_command(display, WRITE_GATELINE, buff, 1);

    buff[0] = 0x03;
    send_command(display, DATA_MODE, buff, 1);

    buff[0] = 0x00;
    buff[1] = d.cols / 8 - 1;
    send_command(display, SET_RAM_X_POS, buff, 2);

    buff[0] = 0x00;
    buff[1] = 0x00;
    buff[2] = (d.rows - 1) & 0xff;
    buff[3] = (d.rows - 1) >> 8;
    send_command(display, SET_RAM_Y_POS, buff, 4);

    // buff[0] = 0x70;
    buff[0] = 0x3c;
    send_command(display, WRITE_VCOM, buff, 1);

    // lut data
    switch (display->colors) {
    case black_white_red:
        buff[0] = 0x30;
        buff[1] = 0xac;
        buff[2] = 0x22;
        send_command(display, SOURCE_VOLTAGE, buff, 3);

        send_command(display, WRITE_LUT, luts_red, LUTS_LENGTH);
        break;

    case black_white_yellow:
        buff[0] = 0x07;
        buff[1] = 0xac;
        buff[2] = 0x32;
        send_command(display, SOURCE_VOLTAGE, buff, 3);

        send_command(display, WRITE_LUT, luts_yellow, LUTS_LENGTH);
        break;

    case black_white:
        send_command(display, WRITE_LUT, luts_black, LUTS_LENGTH);
        break;
    }

    // border color
    switch (display->border_color) {
    case black_px:
        buff[0] = 0b00000000;
        send_command(display, WRITE_BORDER, buff, 1);
        break;
    case white_px:
        buff[0] = 0b00110001;
        send_command(display, WRITE_BORDER, buff, 1);
        break;
    case color_px:
        if (display->colors == black_white_red) {
            buff[0] = 0b01110011;
            send_command(display, WRITE_BORDER, buff, 1);
        } else if (display->colors == black_white_yellow) {
            buff[0] = 0b00110011;
            send_command(display, WRITE_BORDER, buff, 1);
        }
        break;
    }

    // black & white
    buff[0] = 0x00;
    send_command(display, SET_RAM_X_COUNT, buff, 1);
    buff[0] = 0x00;
    buff[1] = 0x00;
    send_command(display, SET_RAM_Y_COUNT, buff, 2);
    send_command(display, WRITE_RAM, display->buffer, buffer_len - 1);

    // color
    buff[0] = 0x00;
    send_command(display, SET_RAM_X_COUNT, buff, 1);
    buff[0] = 0x00;
    buff[1] = 0x00;
    send_command(display, SET_RAM_Y_COUNT, buff, 2);
    send_command(display, WRITE_ALT_RAM, display->alt_buffer, buffer_len - 1);

    wait_for_busy_pin(display);

    buff[0] = 0xc7;
    send_command(display, MASTER_ACTIVATE, buff, 1);
    send_command(display, MASTER_ACTIVATE, NULL, 0);
}
