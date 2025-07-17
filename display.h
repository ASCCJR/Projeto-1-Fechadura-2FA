#ifndef DISPLAY_H
#define DISPLAY_H

// Inicializa o display OLED e o barramento I2C. Deve ser chamada uma vez.
void display_init();

// Limpa o display e exibe até três linhas de texto.
// As linhas podem ser NULL para não desenhar nada naquela posição.
void display_show_message(const char *line1, const char *line2, const char *line3);

#endif // DISPLAY_H