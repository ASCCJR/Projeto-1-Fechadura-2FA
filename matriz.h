/**
 * @file matriz.h
 * @brief Arquivo de cabeçalho para o driver da matriz de LEDs WS2812B (Neopixel).
 */

#ifndef MATRIZ_H
#define MATRIZ_H

#include "pico/stdlib.h"

// --- Funções de Inicialização e Controle Básico ---
void matriz_init();
void matriz_limpar();

// --- Funções de Desenho de Padrões Estáticos (Não-Bloqueantes) ---
void matriz_desenhar_x();
void matriz_desenhar_circulo(uint8_t r, uint8_t g, uint8_t b);
void matriz_desenhar_exclamacao();
void matriz_desenhar_ponto_central(uint8_t r, uint8_t g, uint8_t b);
void matriz_desenhar_digitos(int quantidade);

// --- Funções de Animação Não-Bloqueantes (Baseadas em Estados/Frames) ---
bool matriz_animacao_sucesso_update(void);
bool matriz_animacao_circulo_tempo_update(int tempo_restante_s);
void matriz_iniciar_animacao_fogo(void);
void matriz_atualizar_animacao_fogo(void);
void matriz_parar_animacao_fogo(void);

#endif // MATRIZ_H