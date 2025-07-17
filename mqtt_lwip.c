/**
 * @file mqtt_lwip.c
 */

#include "mqtt_lwip.h"
#include "configura_geral.h"
#include "lwip/apps/mqtt.h"
#include "pico/multicore.h"
#include <string.h>
#include <stdio.h>

mqtt_client_t *mqtt_client_data;

static char mqtt_incoming_topic[128];
static bool publicacao_em_andamento = false;

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_sub_cb(void *arg, err_t result);
static void mqtt_pub_request_cb(void *arg, err_t err);


static void mqtt_connection_cb(mqtt_client_t *client_inst, void *arg, mqtt_connection_status_t status) {
    (void)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        multicore_fifo_push_blocking(FIFO_CMD_MQTT_CONECTADO << 16);
        mqtt_set_inpub_callback(client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, NULL);
        static char topico_comando[100];
        snprintf(topico_comando, sizeof(topico_comando), "%s/%s", DEVICE_ID, TOPICO_BASE_COMANDO_ESTADO);
        mqtt_subscribe(client_inst, topico_comando, 1, mqtt_sub_cb, (void *)topico_comando);
    } else {
    }
}

static void mqtt_sub_cb(void *arg, err_t result) {
    (void)arg;
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    (void)arg;
    strncpy(mqtt_incoming_topic, topic, sizeof(mqtt_incoming_topic) - 1);
    mqtt_incoming_topic[sizeof(mqtt_incoming_topic) - 1] = '\0';
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    (void)arg;
    char payload[128];
    memcpy(payload, data, len);
    payload[len] = '\0';

    char topic_esperado[100];
    snprintf(topic_esperado, sizeof(topic_esperado), "%s/%s", DEVICE_ID, TOPICO_BASE_COMANDO_ESTADO);

    if (strcmp(mqtt_incoming_topic, topic_esperado) == 0) {
        if (strcmp(payload, "ADMIN_SENHA") == 0) {
            uint32_t pacote = (FIFO_CMD_MUDAR_ESTADO << 16) | MODO_ADMIN_AGUARDANDO_CARTAO;
            multicore_fifo_push_blocking(pacote);
        }
        else if (strcmp(payload, "INCENDIO") == 0) {
            uint32_t pacote = (FIFO_CMD_MUDAR_ESTADO << 16) | MODO_EMERGENCIA_INCENDIO;
            multicore_fifo_push_blocking(pacote);
        }
    }
}

static void mqtt_pub_request_cb(void *arg, err_t err) {
    (void)arg;
    publicacao_em_andamento = false;
}

void iniciar_mqtt_cliente() {
    mqtt_client_data = mqtt_client_new();
    char client_id[32];
    snprintf(client_id, sizeof(client_id), "%s_client", DEVICE_ID);
    struct mqtt_connect_client_info_t ci = { .client_id = client_id };
    ip_addr_t broker_ip;
    ip4addr_aton(MQTT_BROKER_IP, &broker_ip);
    mqtt_client_connect(mqtt_client_data, &broker_ip, MQTT_BROKER_PORT, mqtt_connection_cb, 0, &ci);
}

void publicar_mensagem_mqtt(const char *topico, const char *mensagem) {
    if (!mqtt_client_data || !mqtt_client_is_connected(mqtt_client_data) || publicacao_em_andamento) {
        return;
    }
    err_t err = mqtt_publish(mqtt_client_data, topico, mensagem, strlen(mensagem), 1, 0, mqtt_pub_request_cb, NULL);
    if (err == ERR_OK) {
        publicacao_em_andamento = true;
    } else {
    }
}

bool mqtt_is_publishing(void) {
    return publicacao_em_andamento;
}