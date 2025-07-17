/**
 * @file mqtt_lwip.h
 * @brief Arquivo de cabeçalho para o módulo de comunicação MQTT usando a pilha LWIP.
 * Declara as funções públicas para inicializar o cliente MQTT, publicar mensagens
 * e verificar o status da publicação.
 */

#ifndef MQTT_LWIP_H
#define MQTT_LWIP_H

#include "pico/stdlib.h"    // Para tipos básicos
#include "lwip/err.h"       // Para tipos de erro do LWIP
#include "lwip/apps/mqtt.h" // Para tipos e funções do cliente MQTT LWIP

/**
 * @brief Inicia o cliente MQTT e tenta conectar ao broker.
 * Configura os callbacks para conexão e recebimento de mensagens.
 * Deve ser chamada no Core 1.
 */
void iniciar_mqtt_cliente(void);

/**
 * @brief Publica uma mensagem MQTT em um tópico específico.
 * A publicação será enfileirada se uma publicação anterior ainda estiver em andamento.
 * @param topico O tópico MQTT para publicar.
 * @param mensagem A mensagem a ser publicada.
 */
void publicar_mensagem_mqtt(const char *topico, const char *mensagem);

/**
 * @brief Verifica se há uma publicação MQTT em andamento.
 * Usado para evitar sobrecarga de publicações.
 * @return true se uma publicação estiver em andamento, false caso contrário.
 */
bool mqtt_is_publishing(void);

#endif // MQTT_LWIP_H