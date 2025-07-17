/**
 * @file ssd1306_i2c.h
 * @brief Definições e utilitários para controle de displays OLED SSD1306 via barramento I²C.
 */

#ifndef SSD1306_I2C_H
#define SSD1306_I2C_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// --- Configurações do Display ---
#define ssd1306_height 64 // Altura do display em pixels
#define ssd1306_width 128 // Largura do display em pixels
#define ssd1306_i2c_address _u(0x3C) // Endereço I2C padrão

// --- Constantes Calculadas ---
#define ssd1306_page_height 8
#define ssd1306_n_pages (ssd1306_height / ssd1306_page_height)
#define ssd1306_buffer_length (ssd1306_n_pages * ssd1306_width)

// --- Comandos do Controlador SSD1306 ---
#define ssd1306_set_memory_mode _u(0x20)
#define ssd1306_set_column_address _u(0x21)
#define ssd1306_set_page_address _u(0x22)
#define ssd1306_set_display_start_line _u(0x40)
#define ssd1306_set_contrast _u(0x81)
#define ssd1306_set_charge_pump _u(0x8D)
#define ssd1306_set_segment_remap _u(0xA0)
#define ssd1306_set_entire_on _u(0xA4)
#define ssd1306_set_normal_display _u(0xA6)
#define ssd1306_set_mux_ratio _u(0xA8)
#define ssd1306_set_display _u(0xAE)
#define ssd1306_set_common_output_direction _u(0xC0)
#define ssd1306_set_display_offset _u(0xD3)
#define ssd1306_set_display_clock_divide_ratio _u(0xD5)
#define ssd1306_set_precharge _u(0xD9)
#define ssd1306_set_common_pin_configuration _u(0xDA)
#define ssd1306_set_vcomh_deselect_level _u(0xDB)

// --- Estruturas de Dados ---
struct render_area {
    uint8_t start_column;
    uint8_t end_column;
    uint8_t start_page;
    uint8_t end_page;
    int buffer_length;
};

// --- Funções Públicas do Driver ---
void ssd1306_init();
void render_on_display(uint8_t *ssd, struct render_area *area);
void calculate_render_area_buffer_length(struct render_area *area);
void ssd1306_draw_utf8_multiline(uint8_t *ssd, int16_t x, int16_t y, const char *utf8_string);

#endif // SSD1306_I2C_H