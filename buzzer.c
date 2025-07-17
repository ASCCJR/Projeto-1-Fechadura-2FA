/**
 * @file buzzer.c
 * @brief Implementação do driver do buzzer passivo usando PWM.
 * Permite a geração de tons específicos e melodias pré-definidas.
 */

#include "buzzer.h"
#include "hardware/clocks.h" // Necessário para clock_get_hz


// --- Definições Internas de Notas Musicais (Frequências em Hz) ---
#define NOTE_C3  131
#define NOTE_DS3 156
#define NOTE_C5  523
#define NOTE_E5  659
#define NOTE_G5  784


// --- Funções de Controle Básico do Buzzer (Não-Bloqueantes) ---

/**
 * @brief Inicializa o pino do buzzer como saída e o desliga.
 */
void buzzer_init() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0); // Garante que o pino esteja em LOW inicialmente
}

/**
 * @brief Inicia a emissão de um bipe contínuo com tom fixo (aprox. 1kHz).
 * Não-bloqueante. Usado para feedback rápido, como o de digitação.
 */
void buzzer_start_beep() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    
    // Configura um tom fixo para o bipe rápido (aprox. 1000 Hz)
    // Freq = clock_do_sistema / (divisor * wrap)
    // Para 1000 Hz, com clock de 125MHz, podemos usar div=125, wrap=1000.
    pwm_set_clkdiv(slice_num, 125.0); // Divisor
    pwm_set_wrap(slice_num, 1000);   // Período (wrap)
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), 500); // 50% duty cycle
    pwm_set_enabled(slice_num, true);
}

/**
 * @brief Para a emissão do bipe contínuo e desliga o buzzer.
 */
void buzzer_stop_beep() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice_num, false);
    // Retorna o pino para função de GPIO normal para garantir silêncio total
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_SIO);
    gpio_put(BUZZER_PIN, 0); // Garante que o pino esteja em LOW
}


// --- Funções de Reprodução de Tons e Melodias (Bloqueantes) ---

/**
 * @brief Toca um tom com frequência e duração específicas.
 * Bloqueante: pausa a execução durante a duração do tom.
 * @param frequency A frequência do tom em Hz (0 para silêncio/pausa).
 * @param duration_ms A duração do tom em milissegundos.
 */
void buzzer_play_tone(uint16_t frequency, uint16_t duration_ms) {
    // Se a frequência for 0, trata-se de um silêncio (pausa)
    if (frequency == 0) {
        sleep_ms(duration_ms);
        return;
    }

    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    
    // Configura o PWM para a frequência desejada.
    // Freq = clock_do_sistema / (divisor * wrap)
    // Usamos um divisor de 64.0f para um bom balanço entre precisão e alcance do 'wrap'.
    float div = 64.0f;
    uint32_t clock = clock_get_hz(clk_sys); // Frequência do clock do sistema (ex: 125MHz)
    uint32_t wrap = (uint32_t)(clock / (div * frequency)); // Valor de wrap para a frequência desejada

    pwm_set_clkdiv(slice_num, div);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), wrap / 2); // 50% duty cycle
    pwm_set_enabled(slice_num, true);

    sleep_ms(duration_ms); // **Bloqueante:** Pausa a execução pela duração do tom
    
    buzzer_stop_beep(); // Para o tom ao final da duração
}

/**
 * @brief Toca uma melodia curta de sucesso.
 * Bloqueante: a execução é pausada durante a melodia.
 */
void buzzer_tocar_melodia_sucesso() {
    buzzer_play_tone(NOTE_C5, 100);
    buzzer_play_tone(NOTE_E5, 100);
    buzzer_play_tone(NOTE_G5, 120);
}

/**
 * @brief Toca uma melodia curta de erro/falha.
 * Bloqueante: a execução é pausada durante a melodia.
 */
void buzzer_tocar_melodia_erro() {
    buzzer_play_tone(NOTE_DS3, 200);
    buzzer_play_tone(NOTE_C3, 300);
}