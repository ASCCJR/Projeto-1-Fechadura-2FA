/**
 * @file feedback.c
 * @brief Implementação do módulo de feedback ao usuário.
 * Orquestra outros drivers (buzzer, matriz, led rgb) para criar
 * respostas audiovisuais complexas e significativas.
 */

#include "feedback.h"
#include "buzzer.h"
#include "matriz.h"
#include "rgb_led.h"
#include "configura_geral.h" // Para PWM_MAX_DUTY
#include "pico/time.h"      // Para get_absolute_time, absolute_time_diff_us


// --- Variáveis Estáticas Globais (visíveis apenas neste arquivo) ---

// Variáveis de controle para feedback_visual_erro_update
static int erro_frame_atual = 0;
static absolute_time_t erro_ultimo_frame_tempo;
#define ERRO_FRAME_DELAY_US 200000 // 200ms por fase (ligado/desligado)

// Variáveis de controle para feedback_visual_timeout_update
static int timeout_frame_atual = 0;
static absolute_time_t timeout_ultimo_frame_tempo;
#define TIMEOUT_FRAME_DELAY_US 200000 // 200ms por fase (ligado/desligado)

// Variáveis de controle para feedback_visual_fechando_update
static int fechando_frame_atual = 0;
static absolute_time_t fechando_ultimo_frame_tempo;
#define FECHANDO_FRAME_DELAY_US 400000      // 400ms para a primeira fase
#define FECHANDO_INTERVALO_FINAL_US 150000  // 150ms para a segunda fase


// --- Implementação do Feedback Sonoro (Bloqueante) ---

/**
 * @brief Toca a melodia de sucesso.
 */
void feedback_tocar_sucesso() {
    buzzer_tocar_melodia_sucesso();
}

/**
 * @brief Toca a melodia de erro.
 */
void feedback_tocar_erro() {
    buzzer_tocar_melodia_erro();
}

/**
 * @brief Toca os bipes de timeout.
 */
void feedback_tocar_timeout() {
    buzzer_play_tone(880, 100);
    buzzer_play_tone(0, 50); // Pausa
    buzzer_play_tone(880, 100);
    buzzer_play_tone(0, 50); // Pausa
    buzzer_play_tone(880, 100);
}


// --- Implementação do Feedback Visual (Não-Bloqueante) ---

/**
 * @brief Atualiza a animação visual de erro na matriz e no LED.
 * Deve ser chamada repetidamente no loop principal.
 * @return true se a animação foi concluída, false caso contrário.
 */
bool feedback_visual_erro_update(void) {
    if (erro_frame_atual == 0) { // Início da animação
        erro_ultimo_frame_tempo = get_absolute_time(); // Registra o tempo de início
        matriz_desenhar_x(); // Desenha o X
        rgb_led_set_color(PWM_MAX_DUTY, 0, 0); // LED Vermelho
        erro_frame_atual = 1; // Próximo frame será para desligar
        return false; // Animação ainda não terminou
    }

    // Verifica se o tempo mínimo para o próximo frame já passou
    if (absolute_time_diff_us(erro_ultimo_frame_tempo, get_absolute_time()) < ERRO_FRAME_DELAY_US) {
        return false; // Ainda não é hora do próximo frame
    }

    erro_ultimo_frame_tempo = get_absolute_time(); // Atualiza o tempo do último frame

    if (erro_frame_atual < 6) { // A animação tem 3 ciclos (ligado/desligado/ligado)
        if (erro_frame_atual % 2 != 0) { // Frames 1, 3, 5: Desliga (para o intervalo)
            matriz_limpar();
            rgb_led_set_color(0, 0, 0);
        } else { // Frames 2, 4, 6: Liga (para o brilho)
            matriz_desenhar_x();
            rgb_led_set_color(PWM_MAX_DUTY, 0, 0);
        }
        erro_frame_atual++; // Avança para o próximo frame
        return false; // Animação ainda não terminou
    } else {
        // Animação completa! Reseta o estado e limpa os visuais.
        erro_frame_atual = 0; // Prepara para a próxima vez que for chamada
        matriz_limpar();      // Garante que a matriz termine limpa
        rgb_led_set_color(0, 0, 0); // Garante que o LED termine desligado
        return true; // Animação concluída
    }
}

/**
 * @brief Atualiza a animação visual de timeout na matriz e no LED.
 * Deve ser chamada repetidamente no loop principal.
 * @return true se a animação foi concluída, false caso contrário.
 */
bool feedback_visual_timeout_update(void) {
    if (timeout_frame_atual == 0) { // Início da animação
        timeout_ultimo_frame_tempo = get_absolute_time();
        matriz_desenhar_exclamacao();
        rgb_led_set_color(PWM_MAX_DUTY, PWM_MAX_DUTY, 0); // Amarelo
        timeout_frame_atual = 1; // Próximo frame será para desligar
        return false; // Animação ainda não terminou
    }

    // Verifica se o tempo mínimo para o próximo frame já passou
    if (absolute_time_diff_us(timeout_ultimo_frame_tempo, get_absolute_time()) < TIMEOUT_FRAME_DELAY_US) {
        return false; // Ainda não é hora do próximo frame
    }

    timeout_ultimo_frame_tempo = get_absolute_time(); // Atualiza o tempo do último frame

    if (timeout_frame_atual < 6) { // A animação tem 3 ciclos (ligado/desligado/ligado)
        if (timeout_frame_atual % 2 != 0) { // Frames 1, 3, 5: Desliga (para o intervalo)
            matriz_limpar();
            rgb_led_set_color(0, 0, 0);
        } else { // Frames 2, 4, 6: Liga (para o brilho)
            matriz_desenhar_exclamacao();
            rgb_led_set_color(PWM_MAX_DUTY, PWM_MAX_DUTY, 0); // Amarelo
        }
        timeout_frame_atual++; // Avança para o próximo frame
        return false; // Animação ainda não terminou
    } else {
        // Animação completa! Reseta o estado e limpa os visuais.
        timeout_frame_atual = 0; // Prepara para a próxima vez que for chamada
        matriz_limpar();          // Garante que a matriz termine limpa
        rgb_led_set_color(0, 0, 0); // Garante que o LED termine desligado
        return true; // Animação concluída
    }
}

/**
 * @brief Atualiza o feedback visual de fechamento da porta.
 * Deve ser chamada repetidamente no loop principal.
 * @return true se a animação foi concluída, false caso contrário.
 */
bool feedback_visual_fechando_update(void) {
    if (fechando_frame_atual == 0) { // Início da animação
        fechando_ultimo_frame_tempo = get_absolute_time();
        matriz_desenhar_circulo(200, 0, 0); // Círculo Vermelho (brilho 200)
        rgb_led_set_color(PWM_MAX_DUTY, 0, 0); // Vermelho
        fechando_frame_atual = 1; // Próximo frame para desligar
        return false; // Animação ainda não terminou
    }

    // Verifica se o tempo mínimo para o próximo frame já passou
    uint64_t current_delay = (fechando_frame_atual == 1) ? FECHANDO_FRAME_DELAY_US : FECHANDO_INTERVALO_FINAL_US;
    if (absolute_time_diff_us(fechando_ultimo_frame_tempo, get_absolute_time()) < current_delay) {
        return false; // Ainda não é hora do próximo frame
    }

    fechando_ultimo_frame_tempo = get_absolute_time(); // Atualiza o tempo do último frame

    if (fechando_frame_atual == 1) { // Fase 2: Desliga LED e matriz
        matriz_limpar();
        rgb_led_set_color(0, 0, 0);
        fechando_frame_atual = 2; // Último frame, para finalizar
        return false; // Animação ainda não terminou
    } else {
        // Animação completa! Reseta o estado.
        fechando_frame_atual = 0; // Prepara para a próxima vez que for chamada
        // matriz_limpar() e rgb_led_set_color(0,0,0) já foram feitos no frame 1,
        // ou serão gerenciados pelo bloco centralizado (main.c) se necessário.
        return true; // Animação concluída
    }
}