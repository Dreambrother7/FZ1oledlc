#include "pico/stdlib.h"

#include "font.h"
#include "oled_ssd1309.h"

#ifndef OLED_VARIANT_LABEL
#define OLED_VARIANT_LABEL "ORIGINAL"
#endif

static void draw_text(uint8_t page, uint8_t col, const char *text) {
    while (*text && page < OLED_PAGES && col < OLED_WIDTH - 5) {
        oled_write(page, col, (const uint8_t *)font[(uint8_t)*text], 6);
        col += 6;
        text++;
    }
}

static void draw_checkerboard() {
    uint8_t row[OLED_WIDTH];
    for (int page = 0; page < OLED_PAGES; page++) {
        for (int col = 0; col < OLED_WIDTH; col++) {
            bool even_block = (((col / 8) + page) & 1) == 0;
            row[col] = even_block ? 0xAA : 0x55;
        }
        oled_write(page, 0, row, sizeof(row));
    }
}

int main() {
    oled_init();

    while (true) {
        draw_checkerboard();
        sleep_ms(1500);

        oled_clear();
        draw_text(0, 22, "FZ-1 OLED TEST");
        draw_text(2, 13, OLED_VARIANT_LABEL);
        draw_text(4, 13, "SSD1309 SPI OK");
        draw_text(6, 19, "CHECK CONTRAST");
        sleep_ms(2500);

        oled_fill(0xFF);
        sleep_ms(800);

        oled_clear();
        sleep_ms(500);
    }
}
