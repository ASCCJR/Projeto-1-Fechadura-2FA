/**
 * @file rgb_led.c
 * @brief Implementação do driver para o LED RGB (cátodo comum).
 * Controla o brilho de cada cor utilizando PWM.
 */

#include "rgb_led.h" // Para o próprio cabeçalho do driver


// --- Implementação das Funções Públicas ---

/**
 * @brief Configura os pinos do LED RGB para operarem com PWM.
 * Obtém os slices de PWM e os habilita.
 */
void rgb_led_init() {
    // Configura os três pinos (R, G, B) para a função de hardware PWM.
    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    gpio_set_function(LED_G, GPIO_FUNC_PWM);
    gpio_set_function(LED_B, GPIO_FUNC_PWM);

    // Obtém o "slice" de PWM para cada pino. No Pico, múltiplos pinos
    // podem compartilhar o mesmo slice (gerador de PWM).
    uint slice_r = pwm_gpio_to_slice_num(LED_R);
    uint slice_g = pwm_gpio_to_slice_num(LED_G);
    uint slice_b = pwm_gpio_to_slice_num(LED_B);

    // Habilita o slice de cada pino.
    // A verificação para evitar habilitar o mesmo slice múltiplas vezes é uma otimização.
    pwm_set_enabled(slice_r, true);
    if (slice_g != slice_r) { // Se o pino G usa um slice diferente do R
        pwm_set_enabled(slice_g, true);
    }
    if (slice_b != slice_r && slice_b != slice_g) { // Se o pino B usa um slice diferente dos anteriores
        pwm_set_enabled(slice_b, true);
    }
}

/**
 * @brief Define a cor do LED RGB ajustando o duty cycle de cada canal.
 * @param r O valor do duty cycle para o canal vermelho (0 a PWM_MAX_DUTY).
 * @param g O valor do duty cycle para o canal verde (0 a PWM_MAX_DUTY).
 * @param b O valor do duty cycle para o canal azul (0 a PWM_MAX_DUTY).
 */
void rgb_led_set_color(uint16_t r, uint16_t g, uint16_t b) {
    // A função pwm_set_gpio_level ajusta o duty cycle do pino especificado.
    // Ela lida internamente com qual slice e canal o pino está usando.
    // Como o LED é cátodo comum, um valor maior de duty cycle significa mais brilho.
    pwm_set_gpio_level(LED_R, r);
    pwm_set_gpio_level(LED_G, g);
    pwm_set_gpio_level(LED_B, b);
}