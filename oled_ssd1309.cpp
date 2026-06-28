#include "oled_ssd1309.h"

#include "hardware/spi.h"
#include "pico/stdlib.h"

// SPI OLED Definitions (SSD1309)
#define SPI_PORT spi0

#ifndef OLED_PIN_CS
#define OLED_PIN_CS   17
#endif
#ifndef OLED_PIN_SDA
#define OLED_PIN_SDA  19
#endif
#ifndef OLED_PIN_SCL
#define OLED_PIN_SCL  18
#endif
#ifndef OLED_PIN_DC
#define OLED_PIN_DC   20
#endif
#ifndef OLED_PIN_RES
#define OLED_PIN_RES  21
#endif
#ifndef OLED_ROTATE_180
#define OLED_ROTATE_180 0
#endif

// SSD1309 Commands
#define OLED_DISPLAYOFF       0xAE
#define OLED_DISPLAYON        0xAF
#define OLED_SETCONTRAST      0x81
#define OLED_NORMALDISPLAY    0xA6
#define OLED_DISPLAYALLON_RESUME 0xA4
#define OLED_SETMULTIPLEX     0xA8
#define OLED_SETDISPLAYOFFSET 0xD3
#define OLED_SETDISPLAYCLOCKDIV 0xD5
#define OLED_SETPRECHARGE     0xD9
#define OLED_SETCOMPINS       0xDA
#define OLED_SETVCOMDETECT    0xDB
#define OLED_SETSTARTLINE     0x40
#define OLED_SEGREMAP         0xA0
#define OLED_SEGREMAP_INV     0xA1
#define OLED_COMSCANINC       0xC0
#define OLED_COMSCANDEC       0xC8
#define OLED_PAGEADDRSET      0xB0
#define OLED_COLUMNADDRSETH   0x10
#define OLED_COLUMNADDRSETL   0x00

void oled_send_cmd(uint8_t cmd) {
    gpio_put(OLED_PIN_CS, 0);
    gpio_put(OLED_PIN_DC, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(OLED_PIN_CS, 1);
}

void oled_send_data(uint8_t data) {
    gpio_put(OLED_PIN_CS, 0);
    gpio_put(OLED_PIN_DC, 1);
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(OLED_PIN_CS, 1);
}

void oled_send_data_buf(const uint8_t *buf, size_t len) {
    gpio_put(OLED_PIN_CS, 0);
    gpio_put(OLED_PIN_DC, 1);
    spi_write_blocking(SPI_PORT, buf, len);
    gpio_put(OLED_PIN_CS, 1);
}

void oled_set_cursor(uint8_t page, uint8_t col) {
    oled_send_cmd(OLED_PAGEADDRSET | page);
    oled_send_cmd(OLED_COLUMNADDRSETL | (col & 0x0F));
    oled_send_cmd(OLED_COLUMNADDRSETH | ((col >> 4) & 0x0F));
}

void oled_init() {
    spi_init(SPI_PORT, 10 * 1000 * 1000);
    gpio_set_function(OLED_PIN_SDA, GPIO_FUNC_SPI);
    gpio_set_function(OLED_PIN_SCL, GPIO_FUNC_SPI);
    gpio_init(OLED_PIN_CS); gpio_set_dir(OLED_PIN_CS, GPIO_OUT); gpio_put(OLED_PIN_CS, 1);
    gpio_init(OLED_PIN_DC); gpio_set_dir(OLED_PIN_DC, GPIO_OUT); gpio_put(OLED_PIN_DC, 0);
    gpio_init(OLED_PIN_RES); gpio_set_dir(OLED_PIN_RES, GPIO_OUT);

    gpio_put(OLED_PIN_RES, 1); sleep_ms(1);
    gpio_put(OLED_PIN_RES, 0); sleep_ms(15);
    gpio_put(OLED_PIN_RES, 1); sleep_ms(15);

    oled_send_cmd(OLED_DISPLAYOFF);

    oled_send_cmd(0xAD);  // Master Configuration
    oled_send_cmd(0x8E);  // External VCC

    oled_send_cmd(OLED_SETMULTIPLEX);
    oled_send_cmd(0x3F);  // 1/64 duty

    oled_send_cmd(OLED_SETDISPLAYOFFSET);
    oled_send_cmd(0x00);

    oled_send_cmd(OLED_SETSTARTLINE | 0x00);

    oled_send_cmd(OLED_NORMALDISPLAY);

    oled_send_cmd(OLED_SETVCOMDETECT);
    oled_send_cmd(0x3C);  // ~0.83xVCC

    oled_send_cmd(OLED_DISPLAYALLON_RESUME);

    oled_send_cmd(OLED_SETCONTRAST);
    oled_send_cmd(0x6F);

    oled_send_cmd(OLED_SETDISPLAYCLOCKDIV);
    oled_send_cmd(0xF0);  // 105Hz

    oled_send_cmd(0xD8);  // Area Color mode
    oled_send_cmd(0x05);  // Mono + low power

    oled_send_cmd(OLED_SETCOMPINS);
    oled_send_cmd(0x12);  // Alternative COM

    oled_send_cmd(OLED_SETPRECHARGE);
    oled_send_cmd(0xF2);
    oled_send_cmd(0xFF);

    oled_send_cmd(OLED_DISPLAYON);
#if OLED_ROTATE_180
    oled_send_cmd(OLED_COMSCANINC);
    oled_send_cmd(OLED_SEGREMAP_INV);
#else
    oled_send_cmd(OLED_COMSCANDEC);
    oled_send_cmd(OLED_SEGREMAP);
#endif

    sleep_ms(100);
}

void oled_write(uint8_t page, uint8_t col, const uint8_t *data, size_t len) {
    if (page >= OLED_PAGES || col >= OLED_WIDTH || len == 0) {
        return;
    }
    if (col + len > OLED_WIDTH) {
        len = OLED_WIDTH - col;
    }

    oled_set_cursor(page, col);
    oled_send_data_buf(data, len);
}

void oled_fill(uint8_t value) {
    uint8_t buf[OLED_WIDTH];
    for (int i = 0; i < OLED_WIDTH; i++) {
        buf[i] = value;
    }

    for (int page = 0; page < OLED_PAGES; page++) {
        oled_write(page, 0, buf, sizeof(buf));
    }
}

void oled_clear() {
    oled_fill(0x00);
}
