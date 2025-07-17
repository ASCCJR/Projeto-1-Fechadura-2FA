/**
 * @file servo.h
 * @brief Arquivo de cabeçalho para o driver de um servo motor SG90/MG90.
 * Declara as funções para inicializar e controlar o movimento do servo.
 */

#ifndef SERVO_H
#define SERVO_H

#include "pico/stdlib.h"     // Para tipos básicos como int
#include "hardware/pwm.h"    // Para controle PWM
#include "configura_geral.h" // Para acessar SERVO_PIN


/**
 * @brief Inicializa o pino do servo em modo GPIO padrão.
 * O sinal PWM será ativado apenas quando o servo precisar se mover.
 * Deve ser chamada uma vez na inicialização do sistema.
 */
void servo_init(void);

/**
 * @brief Inicia o movimento do servo para um ângulo específico.
 * Esta função liga o sinal PWM e retorna imediatamente (não-bloqueante).
 * O servo permanecerá na posição até que `servo_stop_move()` seja chamada
 * ou um novo `servo_start_move()` seja acionado.
 * @param angle O ângulo desejado em graus (tipicamente entre 0 e 180, dependendo do servo).
 */
void servo_start_move(int angle);

/**
 * @brief Para o sinal PWM do servo.
 * Retorna o pino para a função de GPIO padrão para desenergizar o servo,
 * eliminando o jitter e economizando energia.
 */
void servo_stop_move(void);

#endif // SERVO_H