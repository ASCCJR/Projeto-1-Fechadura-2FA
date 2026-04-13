/**
 * @file configura_geral.h
 * @brief Configuracoes globais de hardware, rede, topicos e estados do firmware.
 */

#ifndef CONFIGURA_GERAL_H
#define CONFIGURA_GERAL_H

#include "pico/stdlib.h"
#include "secrets.h"

// --- Override local por maquina (arquivo ignorado no git) ---
#if defined(__has_include)
#if __has_include("configura_local.h")
#include "configura_local.h"
#endif
#endif

// --- Hardware: pinos e perifericos ---
#define LED_R 13
#define LED_G 11
#define LED_B 12

#define BUZZER_PIN 21
#define SERVO_PIN 2
#define MATRIZ_PIN 7

#define KEYPAD_ROW0_PIN 4
#define KEYPAD_ROW1_PIN 8
#define KEYPAD_ROW2_PIN 9
#define KEYPAD_ROW3_PIN 16
#define KEYPAD_COL0_PIN 17
#define KEYPAD_COL1_PIN 18
#define KEYPAD_COL2_PIN 19
#define KEYPAD_COL3_PIN 20

#define SDA_PIN 14
#define SCL_PIN 15

#define TCS34725_SDA_PIN 0
#define TCS34725_SCL_PIN 1

#define PWM_MAX_DUTY 0xFFFF

// --- Rede e MQTT ---
#ifndef DEVICE_ID
#define DEVICE_ID "bitdoglab_02"
#endif

#ifndef MQTT_BROKER_IP
#define MQTT_BROKER_IP "127.0.0.1"
#endif

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 1884
#endif

// --- Tempos ---
#define TEMPO_MSG_PADRAO_US 4000000 // 4.0 segundos

#ifndef DEBOUNCE_INTERVALO_US
#define DEBOUNCE_INTERVALO_US 150000 // 150ms
#endif

// --- Delays de animacao da matriz ---
#ifndef SUCESSO_FRAME_DELAY_MS
#define SUCESSO_FRAME_DELAY_MS 120 // ms entre frames da animacao de sucesso
#endif

#ifndef FOGO_FRAME_DELAY_US
#define FOGO_FRAME_DELAY_US 100000 // us entre frames da animacao de fogo
#endif

// --- Topicos MQTT ---
#define TOPICO_BASE_COMANDO_ESTADO "comando/estado"
#define TOPICO_STATUS "status"
#define TOPICO_HISTORICO "historico"
#define TOPICO_HEARTBEAT "heartbeat"

// --- Comandos FIFO inter-core ---
#define FIFO_CMD_WIFI_CONECTADO 0xFFFE
#define FIFO_CMD_PUBLICAR_MQTT 0xADD0
#define FIFO_CMD_MUDAR_ESTADO 0xE5A0
#define FIFO_CMD_MQTT_CONECTADO 0xBEEF

// --- Estados e tipos ---
enum ModoOperacao {
    MODO_ESPERA,
    MODO_AGUARDA_SENHA,
    MODO_ABERTO,
    MODO_ADMIN_AGUARDANDO_CARTAO,
    MODO_ADMIN_AGUARDANDO_NOVA_SENHA,
    MODO_MSG_TIMEOUT,
    MODO_MSG_ACESSO_NEGADO,
    MODO_ADMIN_MSG_SUCESSO,
    MODO_ADMIN_MSG_ERRO_FORMATO,
    MODO_ADMIN_MSG_CANCELADO,
    MODO_EMERGENCIA_INCENDIO
};

enum CorDetectada {
    COR_NENHUMA,
    COR_VERDE,
    COR_VERMELHA,
    COR_AZUL
};

enum WifiStatus {
    WIFI_STATUS_FAIL,
    WIFI_STATUS_SUCCESS
};

enum MQTT_MSG_TYPE {
    MSG_STATUS_AGUARDANDO_CARTAO,
    MSG_STATUS_CARTAO_LIDO,
    MSG_STATUS_AGUARDANDO_SENHA,
    MSG_STATUS_SISTEMA_ABERTO,
    MSG_STATUS_SISTEMA_FECHADO,
    MSG_STATUS_MODO_ADMIN,
    MSG_LOG_ACESSO_OK,
    MSG_LOG_ACESSO_FALHA,
    MSG_LOG_EVENTO_TIMEOUT_SENHA,
    MSG_LOG_EVENTO_AUTO_LOCK,
    MSG_LOG_OPERACAO_CANCELADA,
    MSG_LOG_ADMIN_INICIADO,
    MSG_LOG_ADMIN_SENHA_ALTERADA,
    MSG_LOG_EMERGENCIA_INCENDIO_ON,
    MSG_LOG_EMERGENCIA_INCENDIO_OFF,
    MSG_LOG_HEARTBEAT
};

// --- Senhas ativas em memoria ---
extern char SENHA_VERDE[5];
extern char SENHA_VERMELHA[5];
extern char SENHA_AZUL[5];

#endif // CONFIGURA_GERAL_H