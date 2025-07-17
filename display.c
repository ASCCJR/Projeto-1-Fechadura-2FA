#include "display.h"
#include "configura_geral.h"
#include "ssd1306_i2c.h" // Inclui diretamente a API de baixo nível
#include <string.h> // Adicione esta linha para a função memset

// Definição e alocação de memória para o buffer do OLED e a área de renderização,
// visíveis apenas dentro deste arquivo (display.c)
static uint8_t buffer_oled[ssd1306_buffer_length];
static struct render_area area;

// Função auxiliar estática para limpar o buffer e a tela.
// "static" significa que ela só é visível dentro deste arquivo.
static void display_clear() {
    memset(buffer_oled, 0, ssd1306_buffer_length);
    render_on_display(buffer_oled, &area);
}

// Implementação da função de inicialização
void display_init() {
    // Inicializa o barramento I2C na porta i2c1
    i2c_init(i2c1, 400 * 1000);

    // Define os pinos SDA e SCL para a função I2C
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Inicializa o controlador do display
    ssd1306_init();

    // Define a área de renderização para a tela inteira
    area.start_column = 0;
    area.end_column = ssd1306_width - 1;
    area.start_page = 0;
    area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&area);

    // Limpa o display ao iniciar
    display_clear();
}

// Implementação da função de exibir mensagens
void display_show_message(const char *line1, const char *line2, const char *line3) {
    // Primeiro, limpa o buffer com zeros.
    memset(buffer_oled, 0, ssd1306_buffer_length);

    // Desenha cada linha no buffer, se ela não for nula
    if (line1) {
        ssd1306_draw_utf8_multiline(buffer_oled, 0, 0, line1);
    }
    if (line2) {
        ssd1306_draw_utf8_multiline(buffer_oled, 0, 28, line2);
    }
    if (line3) {
        ssd1306_draw_utf8_multiline(buffer_oled, 0, 56, line3);
    }

    // Finalmente, envia o buffer pronto para a tela de uma vez
    render_on_display(buffer_oled, &area);
}