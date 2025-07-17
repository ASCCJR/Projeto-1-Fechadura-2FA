/**
 * @file buzzer.h
 * @brief Arquivo de cabeçalho para o driver do buzzer passivo.
 * Declara funções para inicialização e controle do buzzer,
 * permitindo a geração de tons e melodias.
 */

#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"    // Para tipos básicos, sleep_ms
#include "hardware/pwm.h"   // Para controle PWM
#include "configura_geral.h" // Para acessar BUZZER_PIN (localização do pino do buzzer)


// --- Funções de Controle Básico do Buzzer (Não-Bloqueantes) ---

/**
 * @brief Inicializa o pino do buzzer como saída e o desliga.
 * Deve ser chamada uma vez na inicialização do sistema.
 */
void buzzer_init();

/**
 * @brief Inicia a emissão de um bipe contínuo com tom fixo (aprox. 1kHz).
 * Não-bloqueante. Deve ser parado com buzzer_stop_beep().
 */
void buzzer_start_beep();

/**
 * @brief Para a emissão do bipe contínuo e desliga o buzzer.
 * Não-bloqueante.
 */
void buzzer_stop_beep();


// --- Funções de Reprodução de Tons e Melodias (Bloqueantes) ---

/**
 * @brief Toca um tom com frequência e duração específicas.
 * Bloqueante: pausa a execução durante a duração do tom.
 * @param frequency A frequência do tom em Hz (0 para silêncio/pausa).
 * @param duration_ms A duração do tom em milissegundos.
 */
void buzzer_play_tone(uint16_t frequency, uint16_t duration_ms);

/**
 * @brief Toca uma melodia curta de sucesso.
 * Bloqueante: a execução é pausada durante a melodia.
 */
void buzzer_tocar_melodia_sucesso(void);

/**
 * @brief Toca uma melodia curta de erro/falha.
 * Bloqueante: a execução é pausada durante a melodia.
 */
void buzzer_tocar_melodia_erro(void);


#endif // BUZZER_H