/**
 * @file keypad.h
 * @brief Arquivo de cabeçalho para o driver do teclado matricial 4x4.
 * Declara as funções públicas para inicializar e ler as teclas pressionadas
 * de forma não-bloqueante.
 */

#ifndef KEYPAD_H
#define KEYPAD_H

#include "pico/stdlib.h" // Para tipos básicos como char

/**
 * @brief Inicializa os pinos de GPIO para as linhas e colunas do teclado.
 * As linhas são configuradas como entrada com pull-up e as colunas como saída.
 * Deve ser chamada uma vez na inicialização do sistema.
 */
void keypad_init(void);

/**
 * @brief Lê o estado do teclado de forma não-bloqueante com debounce.
 * Varre as colunas e lê as linhas para identificar a tecla pressionada.
 * @return Retorna o caractere da tecla pressionada ('\0' se nenhuma tecla
 * for pressionada ou se o tempo de debounce não passou).
 */
char keypad_get_key(void);

#endif // KEYPAD_H