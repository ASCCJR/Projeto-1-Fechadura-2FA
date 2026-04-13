/**
 * @file keypad.c
 * @brief Implementação do driver para o teclado matricial 4x4.
 * Inclui uma rotina de varredura não-bloqueante com debounce por software.
 */

#include "keypad.h"
#include "configura_geral.h" // Para as definições dos pinos do teclado
#include "hardware/gpio.h"   // Para controle de GPIO
#include "pico/time.h"       // Para absolute_time_t, get_absolute_time, absolute_time_diff_us


// --- Definições Internas do Módulo ---

// Intervalo de debounce para evitar múltiplos registros de uma única tecla.
// Uma nova tecla só será aceita após este intervalo desde o último toque válido.
#define DEBOUNCE_INTERVALO_US 150000 // 150ms


// --- Variáveis Estáticas Globais (visíveis apenas neste arquivo) ---

// Estado do debounce por borda (edge-trigger):
// - leitura_bruta_anterior: última leitura instantânea do hardware
// - tecla_estavel: estado já estabilizado após debounce
// - instante_ultima_mudanca: momento da última mudança na leitura bruta
static char leitura_bruta_anterior;
static char tecla_estavel;
static absolute_time_t instante_ultima_mudanca;

// Mapeamento dos pinos das linhas (ROWs) e colunas (COLs) do teclado.
// Estes pinos são definidos em configura_geral.h.
const uint ROW_PINS[4] = {KEYPAD_ROW0_PIN, KEYPAD_ROW1_PIN, KEYPAD_ROW2_PIN, KEYPAD_ROW3_PIN};
const uint COL_PINS[4] = {KEYPAD_COL0_PIN, KEYPAD_COL1_PIN, KEYPAD_COL2_PIN, KEYPAD_COL3_PIN};

// Mapeamento de caracteres para a matriz física do teclado 4x4.
// keymap[row][col]
const char keymap[4][4] = {
    {'D', 'C', 'B', 'A'}, // Linha 0 (Ex: D = [0,0])
    {'#', '9', '6', '3'}, // Linha 1
    {'0', '8', '5', '2'}, // Linha 2
    {'*', '7', '4', '1'}  // Linha 3
};

/**
 * @brief Faz uma varredura bruta do teclado (sem debounce).
 * @return Tecla detectada ou '\0' se nenhuma tecla estiver pressionada.
 */
static char keypad_scan_raw(void) {
    for (int c = 0; c < 4; c++) {
        gpio_put(COL_PINS[c], 0);
        for (int r = 0; r < 4; r++) {
            if (!gpio_get(ROW_PINS[r])) {
                gpio_put(COL_PINS[c], 1);
                return keymap[r][c];
            }
        }
        gpio_put(COL_PINS[c], 1);
    }
    return '\0';
}


// --- Implementação das Funções Públicas ---

/**
 * @brief Inicializa os pinos de GPIO para as linhas e colunas do teclado.
 * Configura os pinos das linhas como entrada com pull-up e os das colunas como saída.
 * Inicia o timer de debounce.
 */
void keypad_init() {
    // Inicializa o estado do debounce por borda.
    absolute_time_t agora = get_absolute_time();
    leitura_bruta_anterior = '\0';
    tecla_estavel = '\0';
    instante_ultima_mudanca = agora;

    // Configura os pinos das linhas (ROWs) como entrada com resistor de pull-up interno.
    // O pull-up garante que a linha esteja em nível ALTO quando nenhuma tecla é pressionada.
    for (int i = 0; i < 4; i++) {
        gpio_init(ROW_PINS[i]);
        gpio_set_dir(ROW_PINS[i], GPIO_IN);
        gpio_pull_up(ROW_PINS[i]);
    }

    // Configura os pinos das colunas (COLs) como saída.
    // Inicia-os em nível ALTO. Para ativar uma coluna para varredura, ela é puxada para LOW.
    for (int i = 0; i < 4; i++) {
        gpio_init(COL_PINS[i]);
        gpio_set_dir(COL_PINS[i], GPIO_OUT);
        gpio_put(COL_PINS[i], 1); // Garante que todas as colunas estão desativadas (HIGH)
    }
}

/**
 * @brief Lê o estado do teclado de forma não-bloqueante com debounce.
 * Realiza uma varredura das colunas e lê o estado das linhas para identificar a tecla pressionada.
 * @return Retorna o caractere da tecla pressionada, ou '\0' (nulo) 
 * se nenhuma tecla for pressionada ou se o tempo de debounce não passou desde o último toque.
 */
char keypad_get_key() {
    absolute_time_t agora = get_absolute_time();
    char leitura_bruta = keypad_scan_raw();

    // 1. Se a leitura bruta mudou, reinicia a janela de debounce.
    if (leitura_bruta != leitura_bruta_anterior) {
        leitura_bruta_anterior = leitura_bruta;
        instante_ultima_mudanca = agora;
        return '\0';
    }

    // 2. Aguarda a leitura ficar estável pelo tempo de debounce.
    if (absolute_time_diff_us(instante_ultima_mudanca, agora) < DEBOUNCE_INTERVALO_US) {
        return '\0';
    }

    // 3. Gera evento apenas na borda de subida (tecla pressionada), não em repetição por tecla segurada.
    if (tecla_estavel != leitura_bruta) {
        tecla_estavel = leitura_bruta;
        if (tecla_estavel != '\0') {
            return tecla_estavel;
        }
    }

    return '\0';
}