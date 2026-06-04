#pragma once

#include <stddef.h>
#include <stdint.h>

#define OLED_WIDTH 128
#define OLED_PAGES 8

void oled_init();
void oled_send_cmd(uint8_t cmd);
void oled_send_data(uint8_t data);
void oled_send_data_buf(const uint8_t *buf, size_t len);
void oled_set_cursor(uint8_t page, uint8_t col);
void oled_clear();
void oled_write(uint8_t page, uint8_t col, const uint8_t *data, size_t len);
void oled_fill(uint8_t value);
