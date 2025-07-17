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
#define DEBOUNCE_INTERVALO_US 200000 // 200ms


// --- Variáveis Estáticas Globais (visíveis apenas neste arquivo) ---

// Variável para controlar o tempo do último toque válido, usada para o debounce.
static absolute_time_t ultimo_toque_valido;

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


// --- Implementação das Funções Públicas ---

/**
 * @brief Inicializa os pinos de GPIO para as linhas e colunas do teclado.
 * Configura os pinos das linhas como entrada com pull-up e os das colunas como saída.
 * Inicia o timer de debounce.
 */
void keypad_init() {
    // Inicializa o timer do debounce com o tempo atual do sistema.
    // Isso permite que a primeira tecla seja registrada imediatamente.
    ultimo_toque_valido = get_absolute_time();

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

    // 1. Lógica de Debounce: Verifica se o intervalo mínimo desde o último toque válido passou.
    if (absolute_time_diff_us(ultimo_toque_valido, agora) < DEBOUNCE_INTERVALO_US) {
        return '\0'; // Intervalo mínimo não passou, ignora leitura para evitar múltiplos toques
    }

    // 2. Lógica de Varredura (Scanning): Percorre as colunas e verifica as linhas.
    for (int c = 0; c < 4; c++) { // Loop através das 4 colunas
        gpio_put(COL_PINS[c], 0); // Ativa a coluna atual (coloca em nível BAIXO)

        // Varre as 4 linhas para ver se alguma foi puxada para nível BAIXO pela coluna ativa.
        for (int r = 0; r < 4; r++) {
            if (!gpio_get(ROW_PINS[r])) { // Se a linha está em LOW, significa que uma tecla foi pressionada
                // Tecla encontrada!

                gpio_put(COL_PINS[c], 1); // Desativa a coluna atual para liberar o pino (muito importante!)
                
                ultimo_toque_valido = agora; // Atualiza o tempo do último toque válido para o debounce
                
                return keymap[r][c]; // Retorna o caractere correspondente da tecla pressionada
            }
        }

        gpio_put(COL_PINS[c], 1); // Desativa a coluna atual antes de testar a próxima
    }

    // Se o loop terminar sem encontrar uma tecla pressionada, retorna '\0'.
    return '\0';
}