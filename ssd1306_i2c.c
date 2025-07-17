/**
 * @file ssd1306_i2c.c
 * @brief Implementação de funções para controle de displays OLED SSD1306 via interface I²C.
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "ssd1306_font.h"
#include "ssd1306_i2c.h"

// Protótipos de funções estáticas
static void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character);
static inline int ssd1306_get_font(uint8_t character);
static void ssd1306_send_command(uint8_t command);
static void ssd1306_send_command_list(uint8_t *ssd, int number);
static void ssd1306_send_buffer(uint8_t ssd[], int buffer_length);


// Implementações

void calculate_render_area_buffer_length(struct render_area *area) {
    area->buffer_length = (area->end_column - area->start_column + 1) * (area->end_page - area->start_page + 1);
}

static void ssd1306_send_command(uint8_t command) {
    uint8_t buffer[2] = {0x80, command};
    i2c_write_blocking(i2c1, ssd1306_i2c_address, buffer, 2, false);
}

static void ssd1306_send_command_list(uint8_t *ssd, int number) {
    for (int i = 0; i < number; i++) {
        ssd1306_send_command(ssd[i]);
    }
}

static void ssd1306_send_buffer(uint8_t ssd[], int buffer_length) {
    uint8_t temp_buffer[buffer_length + 1];
    temp_buffer[0] = 0x40;
    memcpy(temp_buffer + 1, ssd, buffer_length);
    i2c_write_blocking(i2c1, ssd1306_i2c_address, temp_buffer, buffer_length + 1, false);
}

void ssd1306_init() {
    uint8_t commands[] = {
        ssd1306_set_display, ssd1306_set_memory_mode, 0x00,
        ssd1306_set_display_start_line, ssd1306_set_segment_remap | 0x01, 
        ssd1306_set_mux_ratio, ssd1306_height - 1,
        ssd1306_set_common_output_direction | 0x08, ssd1306_set_display_offset,
        0x00, ssd1306_set_common_pin_configuration,
#if ((ssd1306_width == 128) && (ssd1306_height == 64))
    0x12,
#else
    0x02,
#endif
        ssd1306_set_display_clock_divide_ratio, 0x80, ssd1306_set_precharge,
        0xF1, ssd1306_set_vcomh_deselect_level, 0x30, ssd1306_set_contrast,
        0xFF, ssd1306_set_entire_on, ssd1306_set_normal_display,
        ssd1306_set_charge_pump, 0x14,
        ssd1306_set_display | 0x01,
    };
    ssd1306_send_command_list(commands, count_of(commands));
}

void render_on_display(uint8_t *ssd, struct render_area *area) {
    uint8_t commands[] = {
        ssd1306_set_column_address, area->start_column, area->end_column,
        ssd1306_set_page_address, area->start_page, area->end_page
    };
    ssd1306_send_command_list(commands, count_of(commands));
    ssd1306_send_buffer(ssd, area->buffer_length);
}

static inline int ssd1306_get_font(uint8_t character) {
    switch(character) {
        case 'A' ... 'Z': return character - 'A' + 1;
        case '0' ... '9': return character - '0' + 27;
        case 'a' ... 'z': return character - 'a' + 37;
        case '.': return 63;
        case ':': return 64;
        case '#': return 65;
        case '!': return 66;
        case '?': return 67;
        case 0xC3: return 68; // Ã
        case 0xC2: return 69; // Â
        case 0xC1: return 70; // Á
        case 0xC0: return 71; // À
        case 0xC9: return 72; // É
        case 0xCA: return 73; // Ê
        case 0xCD: return 74; // Í
        case 0xD3: return 75; // Ó
        case 0xD4: return 76; // Ô
        case 0xD5: return 77; // Õ
        case 0xDA: return 78; // Ú
        case 0xC7: return 79; // Ç
        case 0xE7: return 80; // ç
        case 0xE3: return 81; // ã
        case 0xE1: return 82; // á
        case 0xE0: return 83; // à
        case 0xE2: return 84; // â
        case 0xE9: return 85; // é
        case 0xEA: return 86; // ê
        case 0xED: return 87; // í
        case 0xF3: return 88; // ó
        case 0xF4: return 89; // ô
        case 0xFA: return 90; // ú
        case ',':  return 91;
        case '-':  return 92;
        default:   return 0; // caractere vazio/inválido
    }
}

static void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) {
        return;
    }
    y = y / 8;
    int idx = ssd1306_get_font(character);
    int fb_idx = y * 128 + x;
    for (int i = 0; i < 8; i++) {
        ssd[fb_idx++] = font[idx * 8 + i];
    }
}

void ssd1306_draw_utf8_multiline(uint8_t *ssd, int16_t x, int16_t y, const char *utf8_string) {
    const int max_width = ssd1306_width;
    const int max_height = ssd1306_height;
    const int char_width = 8;
    const int char_height = 8;

    while (*utf8_string && y <= (max_height - char_height)) {
        uint8_t c = (uint8_t)*utf8_string;
        if ((c & 0x80) == 0) { // ASCII
            ssd1306_draw_char(ssd, x, y, c);
            utf8_string++;
        }
        else if ((c & 0xE0) == 0xC0) { // UTF-8 de 2 bytes (para caracteres acentuados)
            uint8_t first = (uint8_t)*utf8_string++;
            uint8_t second = (uint8_t)*utf8_string++;
            uint8_t latin1 = ((first & 0x1F) << 6) | (second & 0x3F);
            ssd1306_draw_char(ssd, x, y, latin1);
        }
        else { // Ignora outros caracteres multi-byte
            utf8_string++;
        }
        x += char_width;
        if (x > (max_width - char_width)) {
            x = 0;
            y += char_height;
        }
    }
}