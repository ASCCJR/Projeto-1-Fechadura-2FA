/**
 * @file tcs34725.h
 * @brief Arquivo de cabeçalho para o driver do sensor de cor I2C TCS34725.
 * Declara a API pública (estruturas e funções) para interagir com o sensor.
 */

#ifndef TCS34725_H
#define TCS34725_H

#include "pico/stdlib.h"    // Para tipos básicos como bool, uint16_t
#include "hardware/i2c.h"   // Para o tipo i2c_inst_t

// Endereço I2C padrão para o sensor TCS34725/TCS34727
#define TCS34725_ADDR 0x29

/**
 * @struct tcs34725_color_data_t
 * @brief Estrutura para armazenar os dados brutos dos 4 canais de cor lidos do sensor.
 */
typedef struct {
    uint16_t clear;  ///< Leitura do canal de luz visível (sem filtro).
    uint16_t red;    ///< Leitura do canal de luz vermelha.
    uint16_t green;  ///< Leitura do canal de luz verde.
    uint16_t blue;   ///< Leitura do canal de luz azul.
} tcs34725_color_data_t;

/**
 * @brief Configura o sensor TCS34725 com parâmetros padrão e o liga.
 * Checa o ID do dispositivo para garantir a comunicação.
 * @param i2c_port A instância do I2C já inicializada onde o sensor está conectado.
 * @return true se o sensor foi inicializado com sucesso, false caso contrário.
 */
bool tcs34725_init(i2c_inst_t* i2c_port);

/**
 * @brief Lê os valores brutos dos quatro canais de cor do sensor.
 * Os dados são armazenados na estrutura colors.
 * @param i2c_port A instância do I2C onde o sensor está conectado.
 * @param colors Ponteiro para uma estrutura tcs34725_color_data_t.
 */
void tcs34725_read_colors(i2c_inst_t* i2c_port, tcs34725_color_data_t* colors);

#endif // TCS34725_H