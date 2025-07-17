/**
 * @file matriz.c
 * @brief Implementação do driver para controle da matriz de LEDs WS2812B (Neopixel).
 */

#include "matriz.h"
#include "configura_geral.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include <string.h>
#include "pico/time.h"
#include <stdlib.h>

// --- Definições Internas ---
#define LED_COUNT 25 // Total de LEDs na matriz 5x5

// --- Variáveis Estáticas Globais ---
static uint32_t matriz_buffer[LED_COUNT] = {0};

// Variáveis para Animação de Sucesso
static int sucesso_frame_atual = 0;
static absolute_time_t sucesso_ultimo_frame_tempo;
#define SUCESSO_FRAME_DELAY_MS 120

// Variáveis para Animação Fogo
static bool fogo_ativo = false;
static absolute_time_t fogo_ultimo_frame_tempo;
#define FOGO_FRAME_DELAY_US 100000 // Atraso entre frames da animação de fogo

// Variáveis para Animação Círculo de Tempo
static uint32_t circ_tempo_cor_ativa = 0;

// --- Protótipos de Funções Estáticas ---
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
static inline void put_pixel(uint32_t pixel_grb);
static void matriz_renderizar();
static uint xy_to_index(uint x, uint y);

// --- Implementações de Funções Estáticas ---
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static void matriz_renderizar() {
    for (int i = 0; i < LED_COUNT; ++i) {
        put_pixel(matriz_buffer[i]);
    }
}

static uint xy_to_index(uint x, uint y) {
    if (y % 2 == 0) {
        return y * 5 + x;
    } else {
        return y * 5 + (4 - x);
    }
}

// --- Funções Públicas (API do Módulo) ---
void matriz_init() {
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, MATRIZ_PIN, 800000, false);
    srand(get_absolute_time());
}

void matriz_limpar() {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    matriz_renderizar();
}

void matriz_desenhar_x() {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    uint32_t cor_vermelha = urgb_u32(150, 0, 0);
    for (int i = 0; i < 5; i++) {
        matriz_buffer[xy_to_index(i, i)] = cor_vermelha;
        matriz_buffer[xy_to_index(4 - i, i)] = cor_vermelha;
    }
    matriz_renderizar();
}

void matriz_desenhar_circulo(uint8_t r, uint8_t g, uint8_t b) {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    uint32_t cor_desejada = urgb_u32(r, g, b);
    for (int x = 1; x <= 3; x++) {
        matriz_buffer[xy_to_index(x, 0)] = cor_desejada;
        matriz_buffer[xy_to_index(x, 4)] = cor_desejada;
    }
    for (int y = 1; y <= 3; y++) {
        matriz_buffer[xy_to_index(0, y)] = cor_desejada;
        matriz_buffer[xy_to_index(4, y)] = cor_desejada;
    }
    matriz_renderizar();
}

void matriz_desenhar_exclamacao() {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    uint32_t cor_amarela = urgb_u32(150, 75, 0);
    matriz_buffer[xy_to_index(2, 0)] = cor_amarela;
    matriz_buffer[xy_to_index(2, 1)] = cor_amarela;
    matriz_buffer[xy_to_index(2, 2)] = cor_amarela;
    matriz_buffer[xy_to_index(2, 4)] = cor_amarela;
    matriz_renderizar();
}

void matriz_desenhar_ponto_central(uint8_t r, uint8_t g, uint8_t b) {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    uint32_t cor_formatada = urgb_u32(r, g, b);
    matriz_buffer[xy_to_index(2, 2)] = cor_formatada;
    matriz_renderizar();
}

void matriz_desenhar_digitos(int quantidade) {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    uint32_t cor_amarela = urgb_u32(150, 75, 0);
    if (quantidade > 4) quantidade = 4;
    for (int i = 0; i < quantidade; i++) {
        matriz_buffer[xy_to_index(i + 1, 2)] = cor_amarela;
    }
    matriz_renderizar();
}

// --- Implementações de Animações Não-Bloqueantes ---

bool matriz_animacao_sucesso_update(void) {
    if (sucesso_frame_atual == 0) {
        sucesso_ultimo_frame_tempo = get_absolute_time();
        memset(matriz_buffer, 0, sizeof(matriz_buffer));
        matriz_buffer[xy_to_index(2, 2)] = urgb_u32(0, 150, 0);
        matriz_renderizar();
        sucesso_frame_atual = 1;
        return false;
    }

    if (absolute_time_diff_us(sucesso_ultimo_frame_tempo, get_absolute_time()) < (uint64_t)SUCESSO_FRAME_DELAY_MS * 1000) {
        return false;
    }

    sucesso_ultimo_frame_tempo = get_absolute_time();
    uint32_t cor_verde = urgb_u32(0, 150, 0);

    switch (sucesso_frame_atual) {
        case 1:
            memset(matriz_buffer, 0, sizeof(matriz_buffer));
            matriz_buffer[xy_to_index(2, 1)] = cor_verde;
            matriz_buffer[xy_to_index(1, 2)] = cor_verde;
            matriz_buffer[xy_to_index(3, 2)] = cor_verde;
            matriz_buffer[xy_to_index(2, 3)] = cor_verde;
            matriz_buffer[xy_to_index(2, 2)] = cor_verde;
            matriz_renderizar();
            sucesso_frame_atual++;
            return false;
        case 2:
            matriz_desenhar_circulo(0, 150, 0);
            sucesso_frame_atual++;
            return false;
        case 3:
            sucesso_frame_atual = 0;
            return true;
        default:
            sucesso_frame_atual = 0;
            return true;
    }
}

bool matriz_animacao_circulo_tempo_update(int tempo_restante_s) {
    uint32_t proxima_cor_a_exibir = 0;
    if (tempo_restante_s > 10) {
        proxima_cor_a_exibir = urgb_u32(0, 150, 0); // Verde
    } else if (tempo_restante_s > 5) {
        proxima_cor_a_exibir = urgb_u32(255, 150, 0); // Amarelo
    } else if (tempo_restante_s >= 0) {
        proxima_cor_a_exibir = urgb_u32(255, 0, 0); // Vermelho
    }

    if (proxima_cor_a_exibir != circ_tempo_cor_ativa) {
        circ_tempo_cor_ativa = proxima_cor_a_exibir;
        matriz_desenhar_circulo((proxima_cor_a_exibir >> 8) & 0xFF, (proxima_cor_a_exibir >> 16) & 0xFF, proxima_cor_a_exibir & 0xFF);
    }
    return false;
}

/**
 * @brief Inicia a animação de fogo na matriz.
 */
void matriz_iniciar_animacao_fogo(void) {
    fogo_ativo = true;
    fogo_ultimo_frame_tempo = get_absolute_time();
    // printf("DEBUG_MATRIZ: Animacao Fogo iniciada.\n"); // Comentado: Debug
}

/**
 * @brief Atualiza um frame da animação de fogo na matriz.
 * Deve ser chamada repetidamente no loop principal.
 */
void matriz_atualizar_animacao_fogo(void) {
    if (!fogo_ativo) return;

    if (absolute_time_diff_us(fogo_ultimo_frame_tempo, get_absolute_time()) < FOGO_FRAME_DELAY_US) {
        return; // Espera pelo delay do frame
    }
    fogo_ultimo_frame_tempo = get_absolute_time();

    // Propaga o "calor" (cores) para cima
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 5; x++) {
            uint32_t cor_abaixo = matriz_buffer[xy_to_index(x, y + 1)];
            uint8_t r = (cor_abaixo >> 8) & 0xFF;
            uint8_t g = (cor_abaixo >> 16) & 0xFF;
            r = (r > 10) ? r - 10 : 0; // Diminui o vermelho
            g = (g > 5) ? g - 5 : 0;   // Diminui o verde
            matriz_buffer[xy_to_index(x, y)] = urgb_u32(r, g, 0);
        }
    }

    // Gera novas chamas na base
    for (int x = 0; x < 5; x++) {
        // Importante: rand() precisa ser inicializado com srand()
        // Isso será feito na próxima verificação.
        int chance = rand() % 100;
        if (chance < 60) { // 60% de chance de acender um pixel na base
            uint8_t r = 200 + (rand() % 56); // Vermelho forte
            uint8_t g = 50 + (rand() % 100); // Verde para laranja
            matriz_buffer[xy_to_index(x, 4)] = urgb_u32(r, g, 0);
        } else {
            matriz_buffer[xy_to_index(x, 4)] = 0; // Pixel desligado na base
        }
    }
    matriz_renderizar();
}

/**
 * @brief Para a animação de fogo e limpa a matriz.
 */
void matriz_parar_animacao_fogo(void) {
    fogo_ativo = false;
    matriz_limpar(); // Limpa a matriz ao parar
    // printf("DEBUG_MATRIZ: Animacao Fogo parada.\n"); // Comentado: Debug
}