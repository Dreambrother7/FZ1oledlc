#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "fz1_capture.pio.h"
#include "font.h"
#include "oled_ssd1309.h"

// FZ-1 Input pins start at GP0
#define FZ1_PIN_BASE 0
#define FZ1_PIN_COUNT 7

// FZ-1 Protocol
#define FZ_GRAPHIC  0x02
#define FZ_TEXT     0x03
#define FZ_CONTRAST 0x0C
#define FZ_RESET    0x0A

#define HORIZONTAL_OFFSET 3
#define PACKET_GAP_US 2000
#define PACKET_IDLE_FLUSH_US 50000
#define SCREENSAVER_IDLE_US (5ULL * 60ULL * 1000ULL * 1000ULL)
#define SCREENSAVER_FRAME_US 90000

// Nibble bit-reversal LUT (matching reference nibbleflip)
static const uint8_t nibbleflip[16] = {
    0x00, 0x08, 0x04, 0x0C, 0x02, 0x0A, 0x06, 0x0E,
    0x01, 0x09, 0x05, 0x0D, 0x03, 0x0B, 0x07, 0x0F
};

// Text memory for XOR operations
static uint8_t text_mem[8][24];
static uint8_t display_buf[OLED_PAGES][OLED_WIDTH];

// State from last command packet
static uint8_t last_cmd = 0;
static uint8_t last_flags = 0;
static uint8_t last_col = 0;
static uint8_t last_row = 0;

static uint32_t rng_state = 0x4F4C4544u;
static uint64_t last_display_change_time = 0;
static bool screensaver_active = false;

static uint32_t rand32() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void display_restore();

static void display_write(uint8_t page, uint8_t col, const uint8_t* data, uint8_t len) {
    if (page >= OLED_PAGES || col >= OLED_WIDTH || len == 0) {
        return;
    }
    if (col + len > OLED_WIDTH) {
        len = OLED_WIDTH - col;
    }

    bool changed = false;
    for (uint8_t i = 0; i < len; i++) {
        if (display_buf[page][col + i] != data[i]) {
            changed = true;
        }
        display_buf[page][col + i] = data[i];
    }

    if (changed) {
        last_display_change_time = time_us_64();
    }

    if (screensaver_active) {
        if (changed) {
            screensaver_active = false;
            display_restore();
        }
        return;
    }

    oled_write(page, col, data, len);
}

static void display_restore() {
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        oled_write(page, 0, display_buf[page], OLED_WIDTH);
    }
}

static void display_clear_state() {
    for (int page = 0; page < OLED_PAGES; page++) {
        for (int col = 0; col < OLED_WIDTH; col++) {
            display_buf[page][col] = 0;
        }
    }
    for (int y = 0; y < 8; y++) {
        for (int j = 0; j < 24; j++) {
            text_mem[y][j] = ' ';
        }
    }
    oled_clear();
    last_display_change_time = time_us_64();
}

static void screensaver_frame() {
    uint8_t page_buf[OLED_WIDTH];

    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        for (uint8_t col = 0; col < OLED_WIDTH; col++) {
            uint8_t stars = 0;
            if ((rand32() & 0x0F) == 0) {
                stars = 1u << (rand32() & 0x07);
            }
            if ((rand32() & 0x3F) == 0) {
                stars |= 1u << (rand32() & 0x07);
            }
            page_buf[col] = stars;
        }
        oled_write(page, 0, page_buf, sizeof(page_buf));
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

    display_write(page, column, (const uint8_t*)font[chr], 6);
    text_mem[row][col / 12] = chr;
}

void display_new_fz1(uint8_t row, uint8_t col, const uint8_t* data, uint8_t len) {
    if (row >= 8) {
        return;
    }

    uint8_t column = calc_column(col);
    uint8_t page = 7 - row;

    display_write(page, column, data, len);
}

void display_xor_fz1(uint8_t row, uint8_t col, const uint8_t* data, uint8_t len) {
    if (row >= 8 || (col / 12) >= 24) {
        return;
    }

    uint8_t column = calc_column(col);
    uint8_t page = 7 - row;
    uint8_t ch = text_mem[row][col / 12];

    uint8_t xor_buf[6];
    uint8_t xlen = (len < 6) ? len : 6;
    for (int x = 0; x < xlen; x++) {
        xor_buf[x] = font[ch][x] ^ data[x];
    }
    display_write(page, column, xor_buf, xlen);
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
    display_clear_state();

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
    uint64_t last_screensaver_frame_time = 0;
    uint32_t ring_tail = 0;
    last_display_change_time = last_nibble_time;

    while (true) {
        uint64_t now = time_us_64();
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

            if (packet_active && (now - last_nibble_time > PACKET_GAP_US)) {
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
            last_nibble_time = now;

        } else if (packet_active && (now - last_nibble_time > PACKET_IDLE_FLUSH_US)) {
            // Timeout → flush packet
            flush_packet(is_command_mode, local_packet_buf, packet_len);
            packet_active = false;
            packet_len = 0;
        } else if (!packet_active && now - last_display_change_time > SCREENSAVER_IDLE_US) {
            if (!screensaver_active || now - last_screensaver_frame_time > SCREENSAVER_FRAME_US) {
                screensaver_frame();
                screensaver_active = true;
                last_screensaver_frame_time = now;
            }
        }
    }
}
