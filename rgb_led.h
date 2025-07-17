/**
 * @file rgb_led.h
 * @brief Arquivo de cabeçalho para o driver do LED RGB (cátodo comum).
 * Declara as funções públicas para inicializar e controlar a cor do LED através de PWM.
 */

#ifndef RGB_LED_H
#define RGB_LED_H

#include "pico/stdlib.h"     // Para tipos básicos como uint16_t
#include "hardware/pwm.h"    // Para controle PWM
#include "configura_geral.h" // Para acessar os pinos do LED (LED_R, LED_G, LED_B) e PWM_MAX_DUTY


/**
 * @brief Configura os pinos do LED RGB para operarem com PWM.
 * Deve ser chamada uma vez na inicialização do sistema.
 */
void rgb_led_init(void);

/**
 * @brief Define a cor do LED RGB.
 * @param r O valor do duty cycle para o canal vermelho (0 a PWM_MAX_DUTY).
 * @param g O valor do duty cycle para o canal verde (0 a PWM_MAX_DUTY).
 * @param b O valor do duty cycle para o canal azul (0 a PWM_MAX_DUTY).
 */
void rgb_led_set_color(uint16_t r, uint16_t g, uint16_t b);

#endif // RGB_LED_H