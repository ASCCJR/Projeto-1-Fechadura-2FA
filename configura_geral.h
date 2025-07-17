/**
 * @file configura_geral.h
 * @brief Definições de configuração para o projeto BitDogLock 2FA.
 * Contém mapeamento de pinos, tópicos MQTT, comandos FIFO
 * e enums para estados e tipos de mensagem.
 */

#ifndef CONFIGURA_GERAL_H
#define CONFIGURA_GERAL_H

#include "pico/stdlib.h" // Para tipos básicos como uint, bool
#include "secrets.h"     // Contém credenciais Wi-Fi

// --- Definições de Hardware e Pinos (Raspberry Pi Pico) ---

// LEDs RGB (Cátodo Comum)
#define LED_R 13
#define LED_G 11
#define LED_B 12

// Buzzer Passivo
#define BUZZER_PIN 21

// Servo Motor
#define SERVO_PIN 2

// Matriz de LEDs 5x5 WS2812B (Neopixel)
#define MATRIZ_PIN 7

// Teclado Matricial 4x4
#define KEYPAD_ROW0_PIN 4
#define KEYPAD_ROW1_PIN 8
#define KEYPAD_ROW2_PIN 9
#define KEYPAD_ROW3_PIN 16
#define KEYPAD_COL0_PIN 17
#define KEYPAD_COL1_PIN 18
#define KEYPAD_COL2_PIN 19
#define KEYPAD_COL3_PIN 20

// Display OLED 0.96" 128x64
#define SDA_PIN 14
#define SCL_PIN 15

// Sensor de Cor I2C (TCS34725)
#define TCS34725_SDA_PIN 0
#define TCS34725_SCL_PIN 1

// I2C0 (direita): Os pinos são GPIO0 (SDA) e GPIO1 (SCL)

// I2C1 (esquerda): Os pinos são GPIO2 (SDA) e GPIO3 (SCL)

// Valor máximo do Duty Cycle para PWM (16 bits)
#define PWM_MAX_DUTY 0xFFFF


// --- Configurações de Rede e MQTT ---
#define DEVICE_ID "bitdoglab_02"    // ID único do dispositivo para MQTT
#define MQTT_BROKER_IP "192.168.0.18"
#define MQTT_BROKER_PORT 1883


// Definições de Timers (em microssegundos)
#define TEMPO_MSG_PADRAO_US 2000000 // 2.0 segundos

// --- Tópicos MQTT ---
#define TOPICO_BASE_COMANDO_ESTADO "comando/estado"
#define TOPICO_STATUS "status"
#define TOPICO_HISTORICO "historico"
#define TOPICO_HEARTBEAT "heartbeat"


// --- Comandos FIFO para Comunicação Inter-Core (Core0 <-> Core1) ---
#define FIFO_CMD_WIFI_CONECTADO 0xFFFE
#define FIFO_CMD_PUBLICAR_MQTT 0xADD0
#define FIFO_CMD_MUDAR_ESTADO 0xE5A0
#define FIFO_CMD_MQTT_CONECTADO 0xBEEF


// --- Enumerações de Estado e Tipos ---

/**
 * @enum ModoOperacao
 * @brief Define os diferentes modos de operação do sistema.
 */
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

/**
 * @enum CorDetectada
 * @brief Cores de cartão que podem ser detectadas pelo sensor.
 */
enum CorDetectada {
    COR_NENHUMA,
    COR_VERDE,
    COR_VERMELHA,
    COR_AZUL
};

/**
 * @enum WifiStatus
 * @brief Status da conexão Wi-Fi.
 */
enum WifiStatus {
    WIFI_STATUS_FAIL,
    WIFI_STATUS_SUCCESS
};

/**
 * @enum MQTT_MSG_TYPE
 * @brief Tipos de mensagens MQTT para categorizar publicações.
 */
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


// --- Variáveis Globais Externas ---
extern char SENHA_VERDE[5];
extern char SENHA_VERMELHA[5];
extern char SENHA_AZUL[5];

#endif // CONFIGURA_GERAL_H