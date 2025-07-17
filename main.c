/**
 * @file main.c
 * @brief Gerenciamento principal do sistema BitDogLock.
 * @details Esta versão inclui a sincronização do LED RGB com a animação do
 * círculo de tempo, além de todos os outros ajustes e animações. O código
 * gerencia uma fechadura eletrônica com autenticação de dois fatores (cartão colorido + senha),
 * utilizando um Raspberry Pi Pico. O Núcleo 0 cuida da lógica da fechadura e periféricos,
 * enquanto o Núcleo 1 gerencia a conectividade Wi-Fi e MQTT.
 */

// --- Inclusões de Bibliotecas ---
#include "configura_geral.h" // Arquivo de configuração geral do projeto (ex: pinos)
#include <stdio.h>
#include <string.h>
#include <math.h>

// Bibliotecas do SDK do Pico
#include "pico/multicore.h"      // Para gerenciamento dos dois núcleos do RP2040
#include "pico/cyw43_arch.h"     // Para controle do chip Wi-Fi (CYW43)
#include "hardware/gpio.h"       // Controle de pinos de I/O de propósito geral
#include "hardware/pwm.h"        // Para controle de PWM (ex: LED RGB, servo)
#include "hardware/i2c.h"        // Para comunicação I2C (ex: display, sensor de cor)
#include "pico/time.h"           // Funções de tempo e timers

// Drivers dos módulos de hardware específicos do projeto
#include "display.h"   // Driver para o display OLED
#include "mqtt_lwip.h" // Funções para comunicação MQTT sobre a pilha lwIP
#include "matriz.h"    // Driver para a matriz de LED
#include "keypad.h"    // Driver para o teclado matricial
#include "tcs34725.h"  // Driver para o sensor de cor TCS34725
#include "rgb_led.h"   // Driver para o LED RGB
#include "buzzer.h"    // Driver para o buzzer
#include "servo.h"     // Driver para o servo motor
#include "feedback.h"  // Funções de feedback ao usuário (visual e sonoro)

// --- Definições de Tempo e Limiares ---
#define SERVO_MOVE_DURATION_US 500000       // Duração do movimento do servo (0.5s)
#define TIMEOUT_SENHA_S 15                  // Tempo limite para digitar a senha (15s)
#define TEMPO_AUTO_TRAVA_S 20               // Tempo para a fechadura travar automaticamente (20s)
#define DISPLAY_UPDATE_INTERVAL_US 1000000  // Intervalo de atualização do display (1s)
#define TEMPO_MSG_PADRAO_US 4000000         // Duração de exibição de mensagens padrão (4s)
#define HEARTBEAT_INTERVAL_US 30000000      // Intervalo para enviar sinal de "estou vivo" via MQTT (30s)
#define MQTT_PUB_MIN_DELAY_US 50000         // Atraso mínimo entre publicações MQTT para evitar flooding

// --- Estruturas de Dados Globais ---

/**
 * @brief Estrutura para um timer não-bloqueante.
 * @details Permite verificar se um período de tempo passou sem parar a execução do código.
 */
typedef struct {
    bool ativo;                 // Indica se o timer está em contagem
    absolute_time_t inicio;     // Momento em que o timer foi iniciado
    uint64_t duracao_us;        // Duração total do timer em microssegundos
} TimerNaoBloqueante;

/**
 * @brief Estrutura para controlar o efeito de pulso do LED RGB.
 */
typedef struct {
    bool ativo;             // Indica se o efeito está ativo
    absolute_time_t inicio; // Momento de início do pulso para cálculo do brilho
    uint8_t r, g, b;        // Cor base do pulso (0-255)
} EfeitoPulso;

/**
 * @brief Estrutura principal que armazena todo o estado da fechadura.
 * @details Centraliza todas as variáveis que definem o comportamento atual do sistema.
 */
typedef struct {
    volatile enum ModoOperacao modo_atual; // O modo de operação atual (ex: MODO_ESPERA). 'volatile' pois é alterado pela FIFO.
    enum CorDetectada cor_ativa;          // A cor do cartão que iniciou a operação atual.
    bool status_aberto;                   // TRUE se a fechadura está aberta, FALSE se fechada.
    bool modo_foi_inicializado;           // Flag para executar código de inicialização de um modo apenas uma vez.

    char senha_digitada[5];               // Buffer para armazenar a senha (4 dígitos + terminador nulo).
    int digitos_count;                    // Contador de quantos dígitos da senha já foram inseridos.

    // Instâncias dos timers não-bloqueantes para diferentes funções
    TimerNaoBloqueante timer_servo;
    TimerNaoBloqueante timer_timeout_senha;
    TimerNaoBloqueante timer_auto_trava;
    TimerNaoBloqueante timer_display_update;
    TimerNaoBloqueante timer_geral;         // Timer para mensagens temporárias.
    TimerNaoBloqueante timer_alarme_beep;   // Timer para o beep do alarme de incêndio.

    // Flags para controlar o estado das animações visuais
    bool animacao_erro_ativa;
    bool animacao_timeout_ativa;
    bool animacao_fechando_ativa;
    bool animacao_sucesso_ativa;
    bool animacao_digitacao_ativa;
    bool animacao_circulo_tempo_ativa;
    bool animacao_fogo_ativa;

    EfeitoPulso efeito_pulso;               // Estado do efeito de pulso do LED RGB.
    TimerNaoBloqueante timer_heartbeat;     // Timer para o envio periódico do heartbeat.
} EstadoFechadura;

// --- Variáveis de Estado Global ---
char SENHA_VERDE[5] = "1337";        // Senha padrão para o cartão verde
char SENHA_VERMELHA[5] = "8008";     // Senha padrão para o cartão vermelho
char SENHA_AZUL[5] = "4242";         // Senha padrão para o cartão azul
static EstadoFechadura fechadura;    // Instância global da estrutura de estado da fechadura

// --- Protótipos de Funções (declarações antecipadas) ---
void timer_iniciar(TimerNaoBloqueante *timer, uint64_t duracao_us);
bool timer_expirou(TimerNaoBloqueante *timer);
void led_iniciar_pulso(uint8_t r, uint8_t g, uint8_t b);
void led_parar_pulso();
void solicitar_publicacao_mqtt(enum MQTT_MSG_TYPE tipo_msg, enum CorDetectada cor);
void verificar_fifo(void);
void inicia_hardware();
void set_rgb_solid(uint16_t r, uint16_t g, uint16_t b);
void start_rgb_pulse_and_matrix_center(uint8_t r, uint8_t g, uint8_t b);
void reset_visual_state();
void acionar_fechamento();
void acionar_abertura();
void desativar_modo_emergencia(void);
enum CorDetectada detectar_cor_cartao(tcs34725_color_data_t colors);
void handle_modo_espera();
void handle_modo_aguarda_senha();
void handle_modo_aberto();
void handle_admin_aguardando_cartao();
void handle_admin_aguardando_nova_senha();
void funcao_wifi_nucleo1();
void inicia_core1();

// --- Implementação das Funções ---

/**
 * @brief Inicia ou reinicia um timer não-bloqueante.
 * @param timer Ponteiro para a estrutura do timer.
 * @param duracao_us Duração desejada para o timer em microssegundos.
 */
void timer_iniciar(TimerNaoBloqueante *timer, uint64_t duracao_us) {
    timer->ativo = true;
    timer->inicio = get_absolute_time();
    timer->duracao_us = duracao_us;
}

/**
 * @brief Verifica se um timer não-bloqueante já expirou.
 * @details Se o timer expirou, ele é desativado e a função retorna true.
 * @param timer Ponteiro para a estrutura do timer.
 * @return true se o tempo passou, false caso contrário.
 */
bool timer_expirou(TimerNaoBloqueante *timer) {
    if (!timer->ativo) return false;
    if (absolute_time_diff_us(timer->inicio, get_absolute_time()) >= timer->duracao_us) {
        timer->ativo = false; // Desativa o timer após expirar
        return true;
    }
    return false;
}

/**
 * @brief Ativa o efeito de pulso para o LED RGB.
 * @param r Componente vermelho da cor (0-255).
 * @param g Componente verde da cor (0-255).
 * @param b Componente azul da cor (0-255).
 */
void led_iniciar_pulso(uint8_t r, uint8_t g, uint8_t b) {
    fechadura.efeito_pulso.ativo = true;
    fechadura.efeito_pulso.inicio = get_absolute_time();
    fechadura.efeito_pulso.r = r;
    fechadura.efeito_pulso.g = g;
    fechadura.efeito_pulso.b = b;
}

/**
 * @brief Para o efeito de pulso e desliga o LED RGB.
 */
void led_parar_pulso() {
    fechadura.efeito_pulso.ativo = false;
    rgb_led_set_color(0, 0, 0); // Apaga o LED
}

/**
 * @brief Envia uma solicitação de publicação MQTT para o Núcleo 1 através da FIFO.
 * @details Empacota o tipo de mensagem e a cor associada em um único valor de 32 bits.
 * @param tipo_msg O tipo de mensagem a ser enviada (definido no enum MQTT_MSG_TYPE).
 * @param cor A cor associada ao evento (definido no enum CorDetectada).
 */
void solicitar_publicacao_mqtt(enum MQTT_MSG_TYPE tipo_msg, enum CorDetectada cor) {
    // Empacota o tipo de mensagem e a cor em um valor de 16 bits
    uint16_t valor = (uint16_t)((tipo_msg & 0xFF) | ((cor & 0xFF) << 8));
    // Empacota o comando e o valor em um pacote de 32 bits
    uint32_t pacote = (FIFO_CMD_PUBLICAR_MQTT << 16) | valor;
    // Envia o pacote para o Núcleo 1 de forma bloqueante
    multicore_fifo_push_blocking(pacote);
}

/**
 * @brief Verifica se há dados na FIFO vindos do Núcleo 1.
 * @details Usado para receber comandos do Núcleo 1, como mudar o estado da fechadura (ex: modo admin, emergência).
 */
void verificar_fifo(void) {
    if (multicore_fifo_rvalid()) { // Há dados para ler?
        uint32_t pacote = multicore_fifo_pop_blocking();
        uint16_t comando = pacote >> 16;
        uint16_t valor = pacote & 0xFFFF;

        if (comando == FIFO_CMD_MUDAR_ESTADO) {
            if (valor == MODO_EMERGENCIA_INCENDIO) {
                // Lógica para alternar (ligar/desligar) o modo de emergência
                if (fechadura.modo_atual == MODO_EMERGENCIA_INCENDIO) {
                    desativar_modo_emergencia();
                } else {
                    fechadura.modo_atual = (enum ModoOperacao)valor;
                    fechadura.modo_foi_inicializado = false;
                }
            } else { // Outras mudanças de estado
                fechadura.modo_atual = (enum ModoOperacao)valor;
                fechadura.modo_foi_inicializado = false;
                // Limpa a senha ao mudar de estado para evitar resíduos
                fechadura.digitos_count = 0;
                memset(fechadura.senha_digitada, 0, sizeof(fechadura.senha_digitada));
            }
        }
    }
}

/**
 * @brief Inicia o processo de fechamento da tranca.
 * @details Mostra mensagem, ativa animação, move o servo, e atualiza o estado e MQTT.
 */
void acionar_fechamento() {
    display_show_message(NULL, "Fechado", NULL);
    fechadura.animacao_fechando_ativa = true;
    fechadura.animacao_circulo_tempo_ativa = false;
    set_rgb_solid(PWM_MAX_DUTY, 0, 0); // LED vermelho sólido
    servo_start_move(0); // Move servo para a posição de fechado
    timer_iniciar(&fechadura.timer_servo, SERVO_MOVE_DURATION_US);
    fechadura.status_aberto = false;
    solicitar_publicacao_mqtt(MSG_STATUS_SISTEMA_FECHADO, COR_NENHUMA);
    fechadura.modo_atual = MODO_ESPERA; // Retorna ao modo de espera
    fechadura.modo_foi_inicializado = false;
}

/**
 * @brief Inicia o processo de abertura da tranca após sucesso na autenticação.
 * @details Toca som de sucesso, ativa animações, move o servo e inicia o timer de auto-travamento.
 */
void acionar_abertura() {
    feedback_tocar_sucesso();
    fechadura.animacao_sucesso_ativa = true;
    matriz_limpar();
    set_rgb_solid(0, PWM_MAX_DUTY, 0); // LED Verde para sucesso
    display_show_message("ACESSO LIBERADO", "Bem-vindo!", NULL);
    servo_start_move(150); // Move servo para a posição de aberto
    timer_iniciar(&fechadura.timer_servo, SERVO_MOVE_DURATION_US);
    fechadura.status_aberto = true;
    fechadura.modo_atual = MODO_ABERTO;
    fechadura.modo_foi_inicializado = false;
    // Inicia contagem regressiva para fechar automaticamente
    timer_iniciar(&fechadura.timer_auto_trava, (uint64_t)TEMPO_AUTO_TRAVA_S * 1000000);
    // Publica o status via MQTT
    solicitar_publicacao_mqtt(MSG_STATUS_SISTEMA_ABERTO, COR_NENHUMA);
    solicitar_publicacao_mqtt(MSG_LOG_ACESSO_OK, fechadura.cor_ativa);
}

/**
 * @brief Lógica para detectar a cor de um cartão usando o sensor TCS34725.
 * @param colors Leituras de R, G, B e Clear do sensor.
 * @return A cor detectada (COR_VERDE, COR_VERMELHA, COR_AZUL) ou COR_NENHUMA.
 */
enum CorDetectada detectar_cor_cartao(tcs34725_color_data_t colors) {
    const int CLEAR_THRESHOLD = 70; // Limiar de luminosidade para evitar leituras falsas no escuro
    if (colors.clear < CLEAR_THRESHOLD) return COR_NENHUMA;
    // Lógica baseada na proporção entre as componentes de cor
    if ((colors.green > colors.red * 1.8) && (colors.green > colors.blue * 1.8)) return COR_VERDE;
    if ((colors.red > colors.green * 2.0) && (colors.red > colors.blue * 2.0)) return COR_VERMELHA;
    if ((colors.blue > colors.green * 1.5) && (colors.blue > colors.red * 2.0)) return COR_AZUL;
    return COR_NENHUMA;
}

/**
 * @brief Reverte o sistema do modo de emergência para o estado normal.
 */
void desativar_modo_emergencia(void) {
    reset_visual_state(); // Reseta todos os indicadores visuais
    fechadura.timer_alarme_beep.ativo = false;
    buzzer_stop_beep();
    matriz_parar_animacao_fogo();
    fechadura.animacao_fogo_ativa = false;
    solicitar_publicacao_mqtt(MSG_LOG_EMERGENCIA_INCENDIO_OFF, COR_NENHUMA);
    acionar_fechamento(); // Fecha a tranca por segurança
    fechadura.modo_atual = MODO_ESPERA;
    fechadura.modo_foi_inicializado = false;
}

// --- Funções Handler da Máquina de Estados ---

/**
 * @brief Gerencia o estado MODO_ESPERA.
 * @details Aguarda a aproximação de um cartão colorido.
 */
void handle_modo_espera() {
    // Bloco de inicialização: executado apenas uma vez quando entra neste modo.
    if (!fechadura.modo_foi_inicializado) {
        solicitar_publicacao_mqtt(MSG_STATUS_AGUARDANDO_CARTAO, COR_NENHUMA);
        matriz_limpar();
        start_rgb_pulse_and_matrix_center(0, 0, 255); // Inicia pulso azul
        fechadura.modo_foi_inicializado = true;
    }
    // Atualiza o display periodicamente
    if (timer_expirou(&fechadura.timer_display_update) || !fechadura.timer_display_update.ativo) {
        display_show_message("BitDogLock 2FA", "Aproxime cartao", NULL);
        timer_iniciar(&fechadura.timer_display_update, DISPLAY_UPDATE_INTERVAL_US);
    }
    // Lê o sensor de cor
    tcs34725_color_data_t colors;
    tcs34725_read_colors(i2c0, &colors);
    enum CorDetectada cor_detectada = detectar_cor_cartao(colors);

    // Se uma cor válida for detectada, muda para o modo de aguardar senha
    if (cor_detectada != COR_NENHUMA) {
        fechadura.cor_ativa = cor_detectada;
        fechadura.timer_display_update.ativo = false; // Para a atualização periódica
        solicitar_publicacao_mqtt(MSG_STATUS_CARTAO_LIDO, fechadura.cor_ativa);
        // Reseta o buffer de senha
        memset(fechadura.senha_digitada, 0, sizeof(fechadura.senha_digitada));
        fechadura.digitos_count = 0;
        // Transição de estado
        fechadura.modo_atual = MODO_AGUARDA_SENHA;
        fechadura.modo_foi_inicializado = false;
        timer_iniciar(&fechadura.timer_timeout_senha, (uint64_t)TIMEOUT_SENHA_S * 1000000);
    }
}

/**
 * @brief Gerencia o estado MODO_AGUARDA_SENHA.
 * @details Aguarda a digitação da senha no teclado. Possui um timeout.
 */
void handle_modo_aguarda_senha() {
    // Bloco de inicialização
    if (!fechadura.modo_foi_inicializado) {
        solicitar_publicacao_mqtt(MSG_STATUS_AGUARDANDO_SENHA, fechadura.cor_ativa);
        set_rgb_solid(PWM_MAX_DUTY, PWM_MAX_DUTY, 0); // LED Amarelo para entrada de senha
        fechadura.modo_foi_inicializado = true;
        fechadura.animacao_digitacao_ativa = true;
    }
    // Atualiza o display com o tempo restante
    if (timer_expirou(&fechadura.timer_display_update) || !fechadura.timer_display_update.ativo) {
        int64_t diff_us = absolute_time_diff_us(fechadura.timer_timeout_senha.inicio, get_absolute_time());
        int tempo_restante = TIMEOUT_SENHA_S - (diff_us / 1000000);
        if (tempo_restante < 0) tempo_restante = 0;
        char linha1[20], linha3[20];
        // Monta a mensagem do display baseada na cor ativa
        switch (fechadura.cor_ativa) {
            case COR_VERDE:    sprintf(linha1, "Senha (Verde):"); break;
            case COR_VERMELHA: sprintf(linha1, "Senha (Vermelho):"); break;
            case COR_AZUL:     sprintf(linha1, "Senha (Azul):"); break;
            default:           sprintf(linha1, "Digite a senha:"); break;
        }
        sprintf(linha3, "Tempo: %ds", tempo_restante);
        display_show_message(linha1, fechadura.senha_digitada, linha3);
        timer_iniciar(&fechadura.timer_display_update, DISPLAY_UPDATE_INTERVAL_US);
    }
    // Verifica se o tempo para digitar a senha esgotou
    if (timer_expirou(&fechadura.timer_timeout_senha)) {
        feedback_tocar_timeout();
        display_show_message("OPERAÇÃO EXPIRADA", "Tempo esgotado", NULL);
        solicitar_publicacao_mqtt(MSG_LOG_EVENTO_TIMEOUT_SENHA, fechadura.cor_ativa);
        
        // SINCRONIZAÇÃO: Define o LED RGB para amarelo, acompanhando a animação de timeout.
        set_rgb_solid(PWM_MAX_DUTY, 20000, 0); 
        
        fechadura.animacao_timeout_ativa = true;
        fechadura.animacao_digitacao_ativa = false;
        fechadura.modo_atual = MODO_MSG_TIMEOUT; // Muda para um estado de mensagem temporária
        fechadura.modo_foi_inicializado = false;
        return; // Sai da função imediatamente
    }
    // Lê uma tecla do keypad
    char tecla = keypad_get_key();
    if (tecla != '\0') { // Se uma tecla foi pressionada
        buzzer_play_tone(1500, 50); // Beep de feedback
        if (tecla == '#') { // Tecla de confirmação
            bool senha_valida = false;
            // Valida a senha digitada contra a senha correta para a cor
            switch (fechadura.cor_ativa) {
                case COR_VERDE:    if (strcmp(fechadura.senha_digitada, SENHA_VERDE) == 0) senha_valida = true; break;
                case COR_VERMELHA: if (strcmp(fechadura.senha_digitada, SENHA_VERMELHA) == 0) senha_valida = true; break;
                case COR_AZUL:     if (strcmp(fechadura.senha_digitada, SENHA_AZUL) == 0) senha_valida = true; break;
                default: senha_valida = false; break;
            }
            if (senha_valida) {
                acionar_abertura(); // Senha correta, abre a tranca
            } else { // Senha incorreta
                feedback_tocar_erro();
                display_show_message("ACESSO NEGADO", "Senha Incorreta", NULL);
                solicitar_publicacao_mqtt(MSG_LOG_ACESSO_FALHA, fechadura.cor_ativa);
                
                // SINCRONIZAÇÃO: Define o LED RGB para vermelho, acompanhando a animação de erro.
                set_rgb_solid(PWM_MAX_DUTY, 0, 0); 
                
                fechadura.animacao_erro_ativa = true;
                fechadura.animacao_digitacao_ativa = false;
                fechadura.modo_atual = MODO_MSG_ACESSO_NEGADO; // Estado de mensagem de erro
                fechadura.modo_foi_inicializado = false;
            }
        } else if (tecla == '*') { // Tecla de cancelamento
            solicitar_publicacao_mqtt(MSG_LOG_OPERACAO_CANCELADA, fechadura.cor_ativa);
            fechadura.animacao_digitacao_ativa = false;
            fechadura.modo_atual = MODO_ESPERA; // Volta ao início
            fechadura.modo_foi_inicializado = false;
        } else if (fechadura.digitos_count < (sizeof(fechadura.senha_digitada) - 1)) {
            // Adiciona o dígito pressionado à senha
            fechadura.senha_digitada[fechadura.digitos_count++] = tecla;
            fechadura.senha_digitada[fechadura.digitos_count] = '\0'; // Mantém o terminador nulo
        }
    }
}

/**
 * @brief Gerencia o estado MODO_ABERTO.
 * @details Mantém a tranca aberta e exibe uma contagem regressiva para o travamento automático.
 */
void handle_modo_aberto() {
    // Bloco de inicialização
    if (!fechadura.modo_foi_inicializado) {
        // A cor inicial é definida no acionar_abertura(). Aqui apenas ativamos a animação.
        fechadura.animacao_circulo_tempo_ativa = true;
        fechadura.modo_foi_inicializado = true;
    }
    // Atualiza o display com o tempo restante para fechar
    if (timer_expirou(&fechadura.timer_display_update) || !fechadura.timer_display_update.ativo) {
        int64_t diff_us = absolute_time_diff_us(fechadura.timer_auto_trava.inicio, get_absolute_time());
        int tempo_restante = TEMPO_AUTO_TRAVA_S - (diff_us / 1000000);
        if (tempo_restante < 0) tempo_restante = 0;
        char linha2_buffer[25];
        sprintf(linha2_buffer, "Travando em: %ds", tempo_restante);
        display_show_message("Sistema Aberto", linha2_buffer, NULL);
        timer_iniciar(&fechadura.timer_display_update, DISPLAY_UPDATE_INTERVAL_US);
    }
    // Verifica se o tempo para auto-travamento expirou
    if (timer_expirou(&fechadura.timer_auto_trava)) {
        solicitar_publicacao_mqtt(MSG_LOG_EVENTO_AUTO_LOCK, COR_NENHUMA);
        fechadura.animacao_circulo_tempo_ativa = false;
        acionar_fechamento(); // Inicia o fechamento
    }
}

/**
 * @brief Gerencia o estado MODO_ADMIN_AGUARDANDO_CARTAO.
 * @details Primeiro passo do modo admin: aguarda o cartão a ser configurado.
 */
void handle_admin_aguardando_cartao() {
    // Bloco de inicialização
    if (!fechadura.modo_foi_inicializado) {
        solicitar_publicacao_mqtt(MSG_LOG_ADMIN_INICIADO, COR_NENHUMA);
        display_show_message("--- MODO ADMIN ---", "Aproxime o cartao", "a ser configurado");
        solicitar_publicacao_mqtt(MSG_STATUS_MODO_ADMIN, COR_NENHUMA);
        matriz_limpar();
        start_rgb_pulse_and_matrix_center(255, 0, 255); // Inicia pulso roxo/magenta
        fechadura.modo_foi_inicializado = true;
    }
    // Lê o sensor de cor
    tcs34725_color_data_t colors;
    tcs34725_read_colors(i2c0, &colors);
    enum CorDetectada cor_detectada_admin = detectar_cor_cartao(colors);

    // Se um cartão for detectado, avança para o próximo passo do modo admin
    if (cor_detectada_admin != COR_NENHUMA) {
        fechadura.cor_ativa = cor_detectada_admin;
        matriz_limpar();
        memset(fechadura.senha_digitada, 0, sizeof(fechadura.senha_digitada));
        fechadura.digitos_count = 0;
        fechadura.modo_atual = MODO_ADMIN_AGUARDANDO_NOVA_SENHA;
        fechadura.modo_foi_inicializado = false;
    }
}

/**
 * @brief Gerencia o estado MODO_ADMIN_AGUARDANDO_NOVA_SENHA.
 * @details Aguarda a digitação da nova senha de 4 dígitos para o cartão selecionado.
 */
void handle_admin_aguardando_nova_senha() {
    // Bloco de inicialização
    if (!fechadura.modo_foi_inicializado) {
        char linha1_buffer[25];
        sprintf(linha1_buffer, "Nova Senha (%s):",
                fechadura.cor_ativa == COR_VERDE ? "Verde" :
                (fechadura.cor_ativa == COR_VERMELHA ? "Vermelho" : "Azul"));
        display_show_message("--- MODO ADMIN ---", linha1_buffer, "");
        // Para o pulso roxo e define o LED para amarelo sólido.
        set_rgb_solid(PWM_MAX_DUTY, PWM_MAX_DUTY, 0);
        fechadura.modo_foi_inicializado = true;
        fechadura.animacao_digitacao_ativa = true;
    }
    // Atualiza o display periodicamente para mostrar a senha sendo digitada
    if (timer_expirou(&fechadura.timer_display_update) || !fechadura.timer_display_update.ativo) {
        char linha1_buffer[25];
        sprintf(linha1_buffer, "Nova Senha (%s):",
                fechadura.cor_ativa == COR_VERDE ? "Verde" :
                (fechadura.cor_ativa == COR_VERMELHA ? "Vermelho" : "Azul"));
        display_show_message("--- MODO ADMIN ---", linha1_buffer, fechadura.senha_digitada);
        timer_iniciar(&fechadura.timer_display_update, DISPLAY_UPDATE_INTERVAL_US);
    }
    // Lê o teclado
    char tecla = keypad_get_key();
    if (tecla != '\0') {
        buzzer_play_tone(1500, 50);
        if (tecla == '#') { // Confirmação da nova senha
            if (fechadura.digitos_count == 4) { // Verifica se a senha tem exatamente 4 dígitos
                // Atualiza a variável global correspondente com a nova senha
                switch (fechadura.cor_ativa) {
                    case COR_VERDE:    strcpy(SENHA_VERDE, fechadura.senha_digitada); break;
                    case COR_VERMELHA: strcpy(SENHA_VERMELHA, fechadura.senha_digitada); break;
                    case COR_AZUL:     strcpy(SENHA_AZUL, fechadura.senha_digitada); break;
                    default: break;
                }
                display_show_message("SUCESSO!", "Senha Salva.", NULL);
                feedback_tocar_sucesso();
                set_rgb_solid(0, PWM_MAX_DUTY, 0); // LED verde para sucesso
                solicitar_publicacao_mqtt(MSG_LOG_ADMIN_SENHA_ALTERADA, fechadura.cor_ativa);
                fechadura.modo_atual = MODO_ADMIN_MSG_SUCESSO; // Estado de mensagem temporária
                fechadura.modo_foi_inicializado = false;
                fechadura.animacao_digitacao_ativa = false;
            } else { // Erro, senha não tem 4 dígitos
                display_show_message("ERRO", "Senha 4 digitos!", NULL);
                feedback_tocar_erro();
                set_rgb_solid(PWM_MAX_DUTY, 0, 0); // LED vermelho para erro
                fechadura.modo_atual = MODO_ADMIN_MSG_ERRO_FORMATO; // Estado de mensagem de erro
                fechadura.modo_foi_inicializado = false;
                fechadura.animacao_digitacao_ativa = false;
            }
        } else if (tecla == '*') { // Cancelamento
            display_show_message("--- MODO ADMIN ---", "Operacao Cancelada", "");
            solicitar_publicacao_mqtt(MSG_LOG_OPERACAO_CANCELADA, COR_NENHUMA);
            fechadura.modo_atual = MODO_ADMIN_MSG_CANCELADO; // Estado de mensagem temporária
            fechadura.modo_foi_inicializado = false;
            fechadura.animacao_digitacao_ativa = false;
        } else if (fechadura.digitos_count < (sizeof(fechadura.senha_digitada) - 1)) {
            // Adiciona o dígito à nova senha
            fechadura.senha_digitada[fechadura.digitos_count++] = tecla;
            fechadura.senha_digitada[fechadura.digitos_count] = '\0';
        }
    }
}

/**
 * @brief Inicializa todos os periféricos de hardware no Núcleo 0.
 */
void inicia_hardware() {
    stdio_init_all();       // Inicializa stdio para debug (opcional)
    display_init();         // Display OLED
    rgb_led_init();         // LED RGB
    buzzer_init();          // Buzzer
    servo_init();           // Servo motor
    matriz_init();          // Matriz de LED
    matriz_limpar();        // Limpa a matriz
    keypad_init();          // Teclado

    // Configuração da interface I2C0 para o sensor de cor
    i2c_init(i2c0, 100 * 1000); // 100kHz
    gpio_set_function(TCS34725_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TCS34725_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(TCS34725_SDA_PIN);
    gpio_pull_up(TCS34725_SCL_PIN);

    // Inicializa o sensor de cor e trava em caso de falha
    if (!tcs34725_init(i2c0)) {
        display_show_message("ERRO FATAL", "TCS34725 falhou!", NULL);
        rgb_led_set_color(PWM_MAX_DUTY, 0, 0);
        while (true) { tight_loop_contents(); }
    }

    // Zera a estrutura de estado e define o estado inicial
    memset(&fechadura, 0, sizeof(EstadoFechadura));
    fechadura.modo_atual = MODO_ESPERA;
    fechadura.modo_foi_inicializado = false;
}

/**
 * @brief Lança a função de gerenciamento de Wi-Fi e MQTT para ser executada no Núcleo 1.
 */
void inicia_core1() {
    multicore_launch_core1(funcao_wifi_nucleo1);
}

/**
 * @brief Define uma cor sólida (sem pulso) no LED RGB.
 * @details Para qualquer efeito de pulso ativo antes de definir a nova cor.
 * @param r Valor para o canal vermelho (0-65535).
 * @param g Valor para o canal verde (0-65535).
 * @param b Valor para o canal azul (0-65535).
 */
void set_rgb_solid(uint16_t r, uint16_t g, uint16_t b) {
    led_parar_pulso();
    rgb_led_set_color(r, g, b);
}

/**
 * @brief Inicia o efeito de pulso no LED RGB e um ponto central correspondente na matriz.
 * @param r Componente vermelho da cor (0-255).
 * @param g Componente verde da cor (0-255).
 * @param b Componente azul da cor (0-255).
 */
void start_rgb_pulse_and_matrix_center(uint8_t r, uint8_t g, uint8_t b) {
    led_iniciar_pulso(r, g, b);
}

/**
 * @brief Reseta todos os indicadores visuais e flags de animação para um estado limpo.
 */
void reset_visual_state() {
    rgb_led_set_color(0, 0, 0);
    matriz_limpar();
    led_parar_pulso();
    fechadura.animacao_erro_ativa = false;
    fechadura.animacao_timeout_ativa = false;
    fechadura.animacao_fechando_ativa = false;
    fechadura.animacao_sucesso_ativa = false;
    fechadura.animacao_digitacao_ativa = false;
    fechadura.animacao_circulo_tempo_ativa = false;
    fechadura.animacao_fogo_ativa = false;
}

/**
 * @brief Função principal do programa (executada no Núcleo 0).
 */
int main() {
    inicia_hardware();

    // --- Processo de Conexão Wi-Fi e MQTT ---
    display_show_message("Rede", "Conectando Wi-Fi...", NULL);
    rgb_led_set_color(40000, 15000, 0); // LED Laranja
    inicia_core1(); // Inicia o Núcleo 1 para cuidar da conexão

    // Aguarda a resposta do Núcleo 1 sobre o status da conexão Wi-Fi
    uint32_t fifo_response;
    while (!multicore_fifo_rvalid()) { tight_loop_contents(); }
    fifo_response = multicore_fifo_pop_blocking();
    if ((fifo_response >> 16) != FIFO_CMD_WIFI_CONECTADO || (fifo_response & 0xFFFF) != WIFI_STATUS_SUCCESS) {
        display_show_message("ERRO FATAL", "Falha na conexao", "Wi-Fi");
        rgb_led_set_color(PWM_MAX_DUTY, 0, 0); // LED Vermelho
        while(true) { tight_loop_contents(); } // Trava o sistema
    }

    // Wi-Fi conectado com sucesso
    display_show_message("Rede", "Wi-Fi Conectado!", NULL);
    rgb_led_set_color(0, PWM_MAX_DUTY, 0); // LED Verde
    sleep_ms(1500);

    // Inicia a conexão com o broker MQTT (também no Núcleo 1)
    display_show_message("Rede", "Conectando Broker", "MQTT...");
    rgb_led_set_color(0, 20000, 40000); // LED Ciano

    // Aguarda a resposta do Núcleo 1 sobre a conexão MQTT
    while(true) {
        if (multicore_fifo_rvalid()) {
            fifo_response = multicore_fifo_pop_blocking();
            if ((fifo_response >> 16) == FIFO_CMD_MQTT_CONECTADO) {
                break; // Conectado, sai do loop
            }
        }
        tight_loop_contents();
    }
    
    // Sistema totalmente pronto
    display_show_message("BitDogLock 2FA", "Sistema Pronto", NULL);
    buzzer_tocar_melodia_sucesso();
    sleep_ms(2500);
    reset_visual_state(); // Limpa os indicadores visuais para o início da operação

    // --- Loop Principal de Operação do Sistema (Core 0) ---
    while (true) {
        verificar_fifo(); // Verifica por comandos vindos do Núcleo 1

        // --- Máquina de Estados Principal ---
        switch (fechadura.modo_atual) {
            case MODO_ESPERA: handle_modo_espera(); break;
            case MODO_AGUARDA_SENHA: handle_modo_aguarda_senha(); break;
            case MODO_ABERTO: handle_modo_aberto(); break;
            case MODO_ADMIN_AGUARDANDO_CARTAO: handle_admin_aguardando_cartao(); break;
            case MODO_ADMIN_AGUARDANDO_NOVA_SENHA: handle_admin_aguardando_nova_senha(); break;
            
            // --- Estados de Mensagem Temporária ---
            // Estes estados apenas exibem uma mensagem por um tempo e depois voltam para MODO_ESPERA
            case MODO_MSG_TIMEOUT:
            case MODO_MSG_ACESSO_NEGADO:
            case MODO_ADMIN_MSG_SUCESSO:
            case MODO_ADMIN_MSG_ERRO_FORMATO:
            case MODO_ADMIN_MSG_CANCELADO:
                if (!fechadura.modo_foi_inicializado) {
                    fechadura.modo_foi_inicializado = true;
                    timer_iniciar(&fechadura.timer_geral, TEMPO_MSG_PADRAO_US);
                }
                if (timer_expirou(&fechadura.timer_geral)) {
                    reset_visual_state();
                    fechadura.modo_atual = MODO_ESPERA;
                    fechadura.modo_foi_inicializado = false;
                }
                break;
            
            // --- Estado de Emergência ---
            case MODO_EMERGENCIA_INCENDIO:
                if (!fechadura.modo_foi_inicializado) {
                    display_show_message("EMERGENCIA!", "ALARME DE INCENDIO", "PERIGO!");
                    solicitar_publicacao_mqtt(MSG_LOG_EMERGENCIA_INCENDIO_ON, COR_NENHUMA);
                    matriz_iniciar_animacao_fogo(); // Animação de fogo na matriz
                    fechadura.animacao_fogo_ativa = true;
                    start_rgb_pulse_and_matrix_center(255, 0, 0); // Pulso vermelho
                    fechadura.modo_foi_inicializado = true;
                    timer_iniciar(&fechadura.timer_alarme_beep, 500000); // Inicia timer para o primeiro beep
                    servo_start_move(150); // Abre a tranca
                    timer_iniciar(&fechadura.timer_servo, SERVO_MOVE_DURATION_US);
                }
                // Toca um beep de alarme periodicamente
                if (timer_expirou(&fechadura.timer_alarme_beep)) {
                    buzzer_play_tone(3000, 100);
                    timer_iniciar(&fechadura.timer_alarme_beep, 1000000); // Próximo beep em 1s
                }
                break;

            default: // Caso algum estado inválido ocorra, retorna para o modo de espera
                fechadura.modo_atual = MODO_ESPERA;
                fechadura.modo_foi_inicializado = false;
                break;
        }

        // --- ATUALIZAÇÃO CONTÍNUA DAS ANIMAÇÕES VISUAIS ---
        // As funções de update retornam 'true' quando a animação termina.
        if (fechadura.animacao_erro_ativa) {
            if (feedback_visual_erro_update()) fechadura.animacao_erro_ativa = false;
        }
        if (fechadura.animacao_timeout_ativa) {
            if (feedback_visual_timeout_update()) fechadura.animacao_timeout_ativa = false;
        }
        if (fechadura.animacao_fechando_ativa) {
            if (feedback_visual_fechando_update()) fechadura.animacao_fechando_ativa = false;
        }
        if (fechadura.animacao_sucesso_ativa) {
            if (matriz_animacao_sucesso_update()) fechadura.animacao_sucesso_ativa = false;
        }
        if (fechadura.animacao_digitacao_ativa) {
            // A animação só deve ocorrer nos modos corretos
            if (fechadura.modo_atual == MODO_AGUARDA_SENHA || fechadura.modo_atual == MODO_ADMIN_AGUARDANDO_NOVA_SENHA) {
                matriz_desenhar_digitos(fechadura.digitos_count);
            } else {
                fechadura.animacao_digitacao_ativa = false; // Desativa se estiver no modo errado
            }
        }
        if (fechadura.animacao_circulo_tempo_ativa) {
            int64_t diff_us = absolute_time_diff_us(fechadura.timer_auto_trava.inicio, get_absolute_time());
            int tempo_restante = TEMPO_AUTO_TRAVA_S - (diff_us / 1000000);
            if (tempo_restante < 0) tempo_restante = 0;
            
            // CORREÇÃO: Sincroniza o LED RGB com a cor do círculo de tempo.
            // A cor do LED muda de verde para amarelo e para vermelho conforme o tempo se esgota.
            if (tempo_restante > 10) {
                set_rgb_solid(0, PWM_MAX_DUTY, 0); // Verde
            } else if (tempo_restante > 5) {
                set_rgb_solid(PWM_MAX_DUTY, 20000, 0); // Amarelo/Laranja
            } else {
                set_rgb_solid(PWM_MAX_DUTY, 0, 0); // Vermelho
            }

            matriz_animacao_circulo_tempo_update(tempo_restante);
        }
        if (fechadura.animacao_fogo_ativa) {
            matriz_atualizar_animacao_fogo();
        }

        // --- ATUALIZAÇÃO DO LED RGB PULSANTE ---
        if (fechadura.efeito_pulso.ativo) {
            // Calcula o brilho usando uma onda senoidal para um efeito suave de "respiração"
            float tempo_ms = absolute_time_diff_us(fechadura.efeito_pulso.inicio, get_absolute_time()) / 1000.0f;
            float brilho = (sinf(tempo_ms * (float)M_PI / 1500.0f) + 1.0f) / 2.0f; // Varia entre 0.0 e 1.0
            
            // Aplica o brilho à cor base e converte para o range do PWM (0-65535)
            uint16_t r = (uint16_t)((float)fechadura.efeito_pulso.r * brilho * (PWM_MAX_DUTY / 255.0f));
            uint16_t g = (uint16_t)((float)fechadura.efeito_pulso.g * brilho * (PWM_MAX_DUTY / 255.0f));
            uint16_t b = (uint16_t)((float)fechadura.efeito_pulso.b * brilho * (PWM_MAX_DUTY / 255.0f));
            rgb_led_set_color(r, g, b);
            
            // Sincroniza o ponto central na matriz com o pulso do LED
            if (fechadura.modo_atual == MODO_ESPERA || fechadura.modo_atual == MODO_ADMIN_AGUARDANDO_CARTAO) {
                 matriz_desenhar_ponto_central((uint8_t)(fechadura.efeito_pulso.r * brilho), (uint8_t)(fechadura.efeito_pulso.g * brilho), (uint8_t)(fechadura.efeito_pulso.b * brilho));
            }
        }

        // --- Gerenciamento de Timers Globais ---
        // Envia um "heartbeat" (sinal de vida) para o broker MQTT periodicamente
        if (timer_expirou(&fechadura.timer_heartbeat) || !fechadura.timer_heartbeat.ativo) {
            solicitar_publicacao_mqtt(MSG_LOG_HEARTBEAT, COR_NENHUMA);
            timer_iniciar(&fechadura.timer_heartbeat, HEARTBEAT_INTERVAL_US);
        }

        // Para o servo motor após o tempo de movimento ter passado
        if (timer_expirou(&fechadura.timer_servo)) {
            servo_stop_move();
        }

        tight_loop_contents(); // Cede tempo para outros processos de baixa prioridade
    }
    return 0; // Inalcançável
}

/**
 * @brief Função executada exclusivamente no Núcleo 1.
 * @details Gerencia a conexão Wi-Fi, a conexão com o broker MQTT e o envio de mensagens.
 * Usa uma fila para desacoplar o envio de mensagens da lógica principal do Núcleo 0.
 */
void funcao_wifi_nucleo1() {
    #define QUEUE_SIZE 10 // Tamanho da fila de mensagens a serem publicadas
    typedef struct {
        char topico[100];
        char mensagem[100];
    } publication_t;
    // Fila circular estática para armazenar as publicações
    static publication_t publication_queue[QUEUE_SIZE];
    static int queue_head = 0, queue_tail = 0;
    static TimerNaoBloqueante timer_entre_publicacoes; // Timer para limitar a taxa de envio

    // Inicializa o chip Wi-Fi
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();

    // Tenta conectar ao Wi-Fi e informa o Núcleo 0 do resultado via FIFO
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        multicore_fifo_push_blocking((FIFO_CMD_WIFI_CONECTADO << 16) | WIFI_STATUS_FAIL);
    } else {
        multicore_fifo_push_blocking((FIFO_CMD_WIFI_CONECTADO << 16) | WIFI_STATUS_SUCCESS);
    }

    iniciar_mqtt_cliente(); // Inicializa o cliente MQTT (que tentará conectar)

    while (true) {
        // Verifica se o Núcleo 0 enviou uma solicitação de publicação
        if (multicore_fifo_rvalid()) {
            uint32_t pacote = multicore_fifo_pop_blocking();
            uint16_t comando = pacote >> 16;
            if (comando == FIFO_CMD_PUBLICAR_MQTT) {
                // Desempacota os dados da mensagem
                uint16_t valor = pacote & 0xFFFF;
                uint8_t tipo_msg = valor & 0xFF;
                uint8_t cor_id = (valor >> 8) & 0xFF;
                char msg_buffer[100], cor_str[15], base_topic[100];

                // Converte o ID da cor em uma string
                switch ((enum CorDetectada)cor_id) {
                    case COR_VERDE:    strcpy(cor_str, "Verde"); break;
                    case COR_VERMELHA: strcpy(cor_str, "Vermelho"); break;
                    case COR_AZUL:     strcpy(cor_str, "Azul"); break;
                    default:           strcpy(cor_str, "N/A"); break;
                }

                // Monta a mensagem e o tópico com base no tipo de mensagem
                switch ((enum MQTT_MSG_TYPE)tipo_msg) {
                    // Mensagens de Status
                    case MSG_STATUS_AGUARDANDO_CARTAO: strcpy(base_topic, TOPICO_STATUS); strcpy(msg_buffer, "Aguardando cartao"); break;
                    case MSG_STATUS_CARTAO_LIDO: strcpy(base_topic, TOPICO_STATUS); sprintf(msg_buffer, "Cartao %s lido", cor_str); break;
                    case MSG_STATUS_AGUARDANDO_SENHA: strcpy(base_topic, TOPICO_STATUS); strcpy(msg_buffer, "Aguardando senha"); break;
                    case MSG_STATUS_SISTEMA_ABERTO: strcpy(base_topic, TOPICO_STATUS); strcpy(msg_buffer, "Sistema Aberto"); break;
                    case MSG_STATUS_SISTEMA_FECHADO: strcpy(base_topic, TOPICO_STATUS); strcpy(msg_buffer, "Sistema Fechado"); break;
                    case MSG_STATUS_MODO_ADMIN: strcpy(base_topic, TOPICO_STATUS); strcpy(msg_buffer, "Modo Administracao"); break;
                    // Mensagens de Log/Histórico
                    case MSG_LOG_ACESSO_OK: strcpy(base_topic, TOPICO_HISTORICO); sprintf(msg_buffer, "ACESSO LIBERADO: Cartao %s.", cor_str); break;
                    case MSG_LOG_ACESSO_FALHA: strcpy(base_topic, TOPICO_HISTORICO); sprintf(msg_buffer, "FALHA: Senha incorreta para o Cartao %s.", cor_str); break;
                    case MSG_LOG_EVENTO_TIMEOUT_SENHA: strcpy(base_topic, TOPICO_HISTORICO); strcpy(msg_buffer, "AVISO: Timeout para digitacao da senha."); break;
                    case MSG_LOG_EVENTO_AUTO_LOCK: strcpy(base_topic, TOPICO_HISTORICO); strcpy(msg_buffer, "EVENTO: Travamento automatico do sistema."); break;
                    case MSG_LOG_OPERACAO_CANCELADA: strcpy(base_topic, TOPICO_HISTORICO); strcpy(msg_buffer, "AVISO: Operacao cancelada pelo usuario."); break;
                    case MSG_LOG_ADMIN_INICIADO: strcpy(base_topic, TOPICO_HISTORICO); strcpy(msg_buffer, "ADMIN: Modo de alteracao de senha iniciado."); break;
                    case MSG_LOG_ADMIN_SENHA_ALTERADA: strcpy(base_topic, TOPICO_HISTORICO); sprintf(msg_buffer, "ADMIN: Senha para Cartao %s foi alterada.", cor_str); break;
                    case MSG_LOG_EMERGENCIA_INCENDIO_ON: strcpy(base_topic, TOPICO_HISTORICO); strcpy(msg_buffer, "EMERGENCIA: Alarme de incendio ATIVADO."); break;
                    case MSG_LOG_EMERGENCIA_INCENDIO_OFF: strcpy(base_topic, TOPICO_HISTORICO); strcpy(msg_buffer, "EMERGENCIA: Alarme de incendio desativado."); break;
                    case MSG_LOG_HEARTBEAT: strcpy(base_topic, TOPICO_HEARTBEAT); strcpy(msg_buffer, "ok"); break;
                }
                
                // Adiciona a mensagem à fila se houver espaço
                int next_tail = (queue_tail + 1) % QUEUE_SIZE;
                if (next_tail != queue_head) { // Verifica se a fila não está cheia
                    snprintf(publication_queue[queue_tail].topico, sizeof(publication_queue[queue_tail].topico), "%s/%s", DEVICE_ID, base_topic);
                    strncpy(publication_queue[queue_tail].mensagem, msg_buffer, sizeof(publication_queue[queue_tail].mensagem) - 1);
                    publication_queue[queue_tail].mensagem[sizeof(publication_queue[queue_tail].mensagem) - 1] = '\0';
                    queue_tail = next_tail;
                }
            }
        }
        
        // Verifica se é hora de enviar a próxima mensagem da fila
        if (!mqtt_is_publishing() && queue_head != queue_tail && (timer_expirou(&timer_entre_publicacoes) || !timer_entre_publicacoes.ativo)) {
            publication_t *pub = &publication_queue[queue_head];
            publicar_mensagem_mqtt(pub->topico, pub->mensagem);
            queue_head = (queue_head + 1) % QUEUE_SIZE; // Avança o ponteiro da cabeça da fila
            timer_iniciar(&timer_entre_publicacoes, MQTT_PUB_MIN_DELAY_US); // Inicia o timer de intervalo
        }
        
        // Funções de manutenção da pilha de rede e Wi-Fi
        cyw43_arch_poll();
        sleep_ms(1);
    }
}