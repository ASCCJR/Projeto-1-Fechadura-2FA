/**
 * @file servo.c
 * @brief Implementação do driver para controle de servo motor.
 * Ativa e desativa a função PWM do pino sob demanda para otimizar
 * o consumo de energia e evitar jitter no servo quando inativo.
 */

#include "servo.h"


// --- Implementação das Funções Públicas ---

/**
 * @brief Inicializa o pino do servo em modo GPIO padrão.
 * O pino será configurado para PWM apenas em `servo_start_move()`.
 */
void servo_init() {
    gpio_init(SERVO_PIN);
    gpio_set_dir(SERVO_PIN, GPIO_OUT); // Define como saída digital
}

/**
 * @brief Inicia o movimento do servo para um ângulo específico.
 * Ativa o PWM no pino do servo e configura a largura de pulso para o ângulo.
 * @param angle O ângulo desejado em graus (0-180).
 */
void servo_start_move(int angle) {
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM); // Ativa a função PWM no pino

    // Configura o PWM para operar em 50Hz, que é o padrão para a maioria dos servos.
    // Frequência do clock do sistema (padrão 125MHz) / (divisor * wrap) = 50Hz
    // Com div = 62.5 e wrap = 40000: 125,000,000 / (62.5 * 40000) = 50 Hz.
    pwm_set_clkdiv(slice_num, 62.5f); 
    pwm_set_wrap(slice_num, 40000);

    // Converte o ângulo (0-180 graus) para a largura de pulso correspondente.
    // Servos tipicamente usam pulsos de 1ms (0 graus) a 2ms (180 graus).
    // Para um wrap de 40000 (período de 20ms): 1ms = 2000 ticks, 2ms = 4000 ticks.
    // A fórmula mapeia o range de 0-180 graus para o range de pulso de 2000 a 4000.
    int valor_pulso = (angle * 2000 / 180) + 2000;
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(SERVO_PIN), valor_pulso);

    pwm_set_enabled(slice_num, true); // Liga o PWM para iniciar o movimento
}

/**
 * @brief Para o sinal PWM do servo.
 * Desliga o PWM e retorna o pino para a função de GPIO comum para parar o servo.
 */
void servo_stop_move() {
    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    
    pwm_set_enabled(slice_num, false); // Desliga o sinal PWM

    // Retorna o pino para a função de GPIO padrão. Isso é crucial para:
    // 1. Parar completamente o envio de pulsos.
    // 2. Reduzir o consumo de energia (o servo não tenta manter a posição).
    // 3. Evitar "jitter" (vibrações finas) quando o servo não está ativo.
    gpio_set_function(SERVO_PIN, GPIO_FUNC_SIO);
}