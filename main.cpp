#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "fz1_capture.pio.h"
#include "font.h"

// SPI OLED Definitions (SSD1309)
#define SPI_PORT spi0
#define PIN_CS   17
#define PIN_SDA  19
#define PIN_SCL  18
#define PIN_DC   20
#define PIN_RES  21

// FZ-1 Input pins start at GP0
#define FZ1_PIN_BASE 0
#define FZ1_PIN_COUNT 7

// SSD1309 Commands
#define OLED_DISPLAYOFF       0xAE
#define OLED_DISPLAYON        0xAF
#define OLED_SETCONTRAST      0x81
#define OLED_NORMALDISPLAY    0xA6
#define OLED_INVERTDISPLAY    0xA7
#define OLED_DISPLAYALLON_RESUME 0xA4
#define OLED_SETMULTIPLEX     0xA8
#define OLED_SETDISPLAYOFFSET 0xD3
#define OLED_SETDISPLAYCLOCKDIV 0xD5
#define OLED_SETPRECHARGE     0xD9
#define OLED_SETCOMPINS       0xDA
#define OLED_SETVCOMDETECT    0xDB
#define OLED_SETSTARTLINE     0x40
#define OLED_SEGREMAP         0xA0
#define OLED_COMSCANDEC       0xC8
#define OLED_PAGEADDRSET      0xB0
#define OLED_COLUMNADDRSETH   0x10
#define OLED_COLUMNADDRSETL   0x00

// FZ-1 Protocol
#define FZ_GRAPHIC  0x02
#define FZ_TEXT     0x03
#define FZ_CONTRAST 0x0C
#define FZ_RESET    0x0A

#define HORIZONTAL_OFFSET 3
#define PACKET_GAP_US 2000
#define PACKET_IDLE_FLUSH_US 50000

// Nibble bit-reversal LUT (matching reference nibbleflip)
static const uint8_t nibbleflip[16] = {
    0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,
    0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F
};

// Text memory for XOR operations
static uint8_t text_mem[8][24];

// State from last command packet
static uint8_t last_cmd = 0;
static uint8_t last_flags = 0;
static uint8_t last_col = 0;
static uint8_t last_row = 0;


// ===== SPI OLED Low-Level =====

void oled_send_cmd(uint8_t cmd) {
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
}

void oled_send_data(uint8_t data) {
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 1);
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);
}

void oled_send_data_buf(const uint8_t *buf, size_t len) {
    gpio_put(PIN_CS, 0);
    gpio_put(PIN_DC, 1);
    spi_write_blocking(SPI_PORT, buf, len);
    gpio_put(PIN_CS, 1);
}

void oled_set_cursor(uint8_t page, uint8_t col) {
    oled_send_cmd(OLED_PAGEADDRSET | page);
    oled_send_cmd(OLED_COLUMNADDRSETL | (col & 0x0F));
    oled_send_cmd(OLED_COLUMNADDRSETH | ((col >> 4) & 0x0F));
}

void oled_init() {
    spi_init(SPI_PORT, 10 * 1000 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCL, GPIO_FUNC_SPI);
    gpio_init(PIN_CS); gpio_set_dir(PIN_CS, GPIO_OUT); gpio_put(PIN_CS, 1);
    gpio_init(PIN_DC); gpio_set_dir(PIN_DC, GPIO_OUT); gpio_put(PIN_DC, 0);
    gpio_init(PIN_RES); gpio_set_dir(PIN_RES, GPIO_OUT);

    // Reset sequence
    gpio_put(PIN_RES, 1); sleep_ms(1);
    gpio_put(PIN_RES, 0); sleep_ms(15);
    gpio_put(PIN_RES, 1); sleep_ms(15);

    // Init sequence closely matching reference (SSD1309)
    oled_send_cmd(OLED_DISPLAYOFF);

    oled_send_cmd(0xAD);  // Master Configuration
    oled_send_cmd(0x8E);  // External VCC

    oled_send_cmd(OLED_SETMULTIPLEX);
    oled_send_cmd(0x3F);  // 1/64 duty

    oled_send_cmd(OLED_SETDISPLAYOFFSET);
    oled_send_cmd(0x00);

    oled_send_cmd(OLED_SETSTARTLINE | 0x00);

    oled_send_cmd(OLED_NORMALDISPLAY);  // 0xA6 - Normal (not inverted)

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
    oled_send_cmd(OLED_COMSCANDEC);  // 0xC8 - COM63 to COM0
    oled_send_cmd(OLED_SEGREMAP);    // 0xA0 - No segment remap (matching reference)

    sleep_ms(100);
}

void oled_clear() {
    for (int y = 0; y < 8; y++) {
        oled_set_cursor(y, 0);
        for (int x = 0; x < 128; x++) {
            oled_send_data(0);
        }
        for (int j = 0; j < 24; j++) {
            text_mem[y][j] = ' ';
        }
    }
}

// ===== FZ-1 Display Functions (matching reference) =====

// Column mapping: remove the gap in the middle of the FZ-1's address space
static uint8_t calc_column(uint8_t col) {
    uint8_t col1 = col >> 1;
    uint8_t column = (col1 & 64) ? col1 : col1 + 16;
    return column + HORIZONTAL_OFFSET;
}

void display_char_fz1(uint8_t row, uint8_t col, uint8_t chr) {
    if (row >= 8 || (col / 12) >= 24) {
        return;
    }

    uint8_t column = calc_column(col);
    uint8_t page = 7 - row;

    oled_set_cursor(page, column);
    oled_send_data_buf((const uint8_t*)font[chr], 6);
    text_mem[row][col / 12] = chr;
}

void display_new_fz1(uint8_t row, uint8_t col, const uint8_t* data, uint8_t len) {
    if (row >= 8) {
        return;
    }

    uint8_t column = calc_column(col);
    uint8_t page = 7 - row;

    oled_set_cursor(page, column);
    oled_send_data_buf(data, len);
}

void display_xor_fz1(uint8_t row, uint8_t col, const uint8_t* data, uint8_t len) {
    if (row >= 8 || (col / 12) >= 24) {
        return;
    }

    uint8_t column = calc_column(col);
    uint8_t page = 7 - row;
    uint8_t ch = text_mem[row][col / 12];

    oled_set_cursor(page, column);
    uint8_t xor_buf[6];
    uint8_t xlen = (len < 6) ? len : 6;
    for (int x = 0; x < xlen; x++) {
        xor_buf[x] = font[ch][x] ^ data[x];
    }
    oled_send_data_buf(xor_buf, xlen);
}

// ===== FZ-1 Protocol Handlers =====

void process_command_packet(const uint8_t* nibbles, uint32_t len) {
    if (len < 6) {
        return;
    }

    // Nibbles: [0]=CMD, [1]=FLAGS, [2]=COLL, [3]=COLH, [4]=ROWL, [5]=ROWH
    last_cmd   = nibbles[0];
    last_flags = nibbles[1];
    last_col   = nibbles[2] | (nibbles[3] << 4);
    last_row   = nibbles[4] | (nibbles[5] << 4);

    // Row adjustment from flags (reference: if bit0 set and row < 4, add 4)
    if ((last_flags & 0x01) && last_row < 4)
        last_row += 4;

    // Command phase only stores parameters.
    // Character codes and graphic data arrive in the subsequent data phase.
}

void process_data_packet(const uint8_t* nibbles, uint32_t len) {
    if (len < 2) {
        return;
    }

    if (last_cmd == FZ_TEXT) {
        // Text data: 2 nibbles = character code
        uint8_t chr = nibbles[0] | (nibbles[1] << 4);
        display_char_fz1(last_row, last_col, chr);

    } else if (last_cmd == FZ_GRAPHIC) {
        // Graphic data: pairs of nibbles = pixel bytes
        uint32_t byte_len = len / 2;
        if (byte_len > 128) byte_len = 128;

        uint8_t data_bytes[128];
        for (uint32_t i = 0; i < byte_len; i++) {
            data_bytes[i] = (nibbles[i * 2] << 4) | nibbles[i * 2 + 1];
        }

        bool operation = (last_flags & 0x08) != 0;

        if (operation) {
            display_new_fz1(last_row, last_col, data_bytes, byte_len);
        } else {
            display_xor_fz1(last_row, last_col, data_bytes, byte_len);
        }
    }
}

static bool collapse_duplicated_nibbles(const uint8_t* src, uint32_t src_len, uint8_t* dst, uint32_t* dst_len) {
    if (src_len < 2 || (src_len & 1)) return false;

    for (uint32_t i = 0; i < src_len; i += 2) {
        if ((src[i] & 0x0F) != (src[i + 1] & 0x0F)) return false;
    }

    for (uint32_t i = 0; i < src_len / 2; i++) {
        dst[i] = src[i * 2];
    }
    *dst_len = src_len / 2;
    return true;
}

static void flush_packet(bool is_command_mode, const uint8_t* packet_buf, uint32_t packet_len) {
    if (packet_len == 0) return;

    static uint8_t collapsed_packet_buf[4096];
    const uint8_t* parse_buf = packet_buf;
    uint32_t parse_len = packet_len;

    if (collapse_duplicated_nibbles(packet_buf, packet_len, collapsed_packet_buf, &parse_len)) {
        parse_buf = collapsed_packet_buf;
    }

    if (is_command_mode) {
        process_command_packet(parse_buf, parse_len);
    } else {
        process_data_packet(parse_buf, parse_len);
    }
}

// ===== DMA Ring Buffer =====

#define RING_BUF_SIZE 32768
uint8_t ring_buf[RING_BUF_SIZE] __attribute__((aligned(RING_BUF_SIZE)));
int dma_chan;

void dma_init_ring(PIO pio, uint sm) {
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);

    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
    channel_config_set_ring(&c, true, 15);

    dma_channel_configure(
        dma_chan,
        &c,
        ring_buf,
        (volatile uint8_t*)&pio->rxf[sm],
        0xFFFFFFFF,
        true
    );
}

// ===== Main =====

int main() {
    oled_init();
    oled_clear();

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &fz1_capture_program);
    fz1_capture_program_init(pio, sm, offset, 0, 7);

    pio_sm_set_enabled(pio, sm, true);
    dma_init_ring(pio, sm);

    bool is_command_mode = true;
    bool packet_active = false;
    uint32_t packet_len = 0;
    static uint8_t local_packet_buf[8192];
    uint64_t last_nibble_time = time_us_64();
    uint32_t ring_tail = 0;

    while (true) {
        uint32_t ring_head = ((uint32_t)dma_hw->ch[dma_chan].write_addr - (uint32_t)ring_buf);

        if (ring_tail != ring_head) {
            uint8_t raw_sample = ring_buf[ring_tail];
            ring_tail = (ring_tail + 1) % RING_BUF_SIZE;

            uint8_t payload = raw_sample & 0x7F;

            // Decode nibble: bit-reverse (Pin1=D8→GP0, Pin4=D1→GP3) then invert (active-low)
            // Using the same nibbleflip LUT as the reference implementation
            uint8_t decoded_nibble = nibbleflip[(~payload) & 0x0F];

            // GP6 = #OP: LOW (0) = command mode, HIGH (1) = data mode
            bool current_is_command = ((payload >> 6) & 0x01) == 0;

            if (packet_active && (time_us_64() - last_nibble_time > PACKET_GAP_US)) {
                flush_packet(is_command_mode, local_packet_buf, packet_len);
                packet_active = false;
                packet_len = 0;
            }

            // Detect mode change → process previous packet
            if (packet_active && current_is_command != is_command_mode) {
                flush_packet(is_command_mode, local_packet_buf, packet_len);
                packet_len = 0;
            }

            is_command_mode = current_is_command;
            packet_active = true;

            if (packet_len < sizeof(local_packet_buf)) {
                local_packet_buf[packet_len++] = decoded_nibble;
            }
            last_nibble_time = time_us_64();

        } else if (packet_active && (time_us_64() - last_nibble_time > PACKET_IDLE_FLUSH_US)) {
            // Timeout → flush packet
            flush_packet(is_command_mode, local_packet_buf, packet_len);
            packet_active = false;
            packet_len = 0;
        }
    }
}
