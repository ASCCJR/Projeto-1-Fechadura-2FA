/**
 * @file feedback.h
 * @brief Arquivo de cabeçalho para o módulo de feedback ao usuário.
 * Declara funções que orquestram respostas visuais e sonoras usando
 * os periféricos (matriz, LED RGB, buzzer).
 */

#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "pico/stdlib.h" // Para tipos básicos como bool, uint8_t

// --- Funções de Feedback Sonoro ---

/**
 * @brief Toca a melodia de sucesso.
 */
void feedback_tocar_sucesso(void);

/**
 * @brief Toca a melodia de erro.
 */
void feedback_tocar_erro(void);

/**
 * @brief Toca os bipes de timeout.
 */
void feedback_tocar_timeout(void);


// --- Funções de Feedback Visual (Não-Bloqueantes) ---
// As funções *_update progridem a animação por um frame e retornam true quando a animação é concluída.

/**
 * @brief Atualiza a animação visual de erro na matriz e no LED.
 * Deve ser chamada repetidamente no loop principal.
 * @return true se a animação foi concluída, false caso contrário.
 */
bool feedback_visual_erro_update(void);

/**
 * @brief Atualiza a animação visual de timeout na matriz e no LED.
 * Deve ser chamada repetidamente no loop principal.
 * @return true se a animação foi concluída, false caso contrário.
 */
bool feedback_visual_timeout_update(void);

/**
 * @brief Atualiza o feedback visual de fechamento da porta.
 * Deve ser chamada repetidamente no loop principal.
 * @return true se a animação foi concluída, false caso contrário.
 */
bool feedback_visual_fechando_update(void);


#endif // FEEDBACK_H