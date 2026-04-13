/**
 * @file secrets.h
 * @brief Credenciais de Wi-Fi com suporte a override local por maquina.
 */

#ifndef SECRETS_H
#define SECRETS_H

#if defined(__has_include)
#if __has_include("secrets.local.h")
#include "secrets.local.h"
#endif
#endif

#ifndef WIFI_SSID
#define WIFI_SSID "SEU_SSID_AQUI"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "SUA_SENHA_AQUI"
#endif

#endif // SECRETS_H