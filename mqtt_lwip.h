/**
 * @file mqtt_lwip.h
 * @brief Interface publica do cliente MQTT baseado em lwIP.
 */

#ifndef MQTT_LWIP_H
#define MQTT_LWIP_H

#include "pico/stdlib.h"
#include "lwip/err.h"
#include "lwip/apps/mqtt.h"

/**
 * @brief Inicializa o cliente MQTT e inicia a conexao com o broker.
 * @note Deve ser chamada no Core 1.
 */
void iniciar_mqtt_cliente(void);

/**
 * @brief Publica mensagem em um topico MQTT.
 * @param topico Topico de destino.
 * @param mensagem Payload a ser enviado.
 */
void publicar_mensagem_mqtt(const char *topico, const char *mensagem);

/**
 * @brief Informa se existe publicacao MQTT em andamento.
 * @return true quando a pilha ainda esta publicando; caso contrario, false.
 */
bool mqtt_is_publishing(void);

#endif // MQTT_LWIP_H