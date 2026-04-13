# Instruções Operacionais para IA (Projeto BitDogLock 2FA)

Este arquivo orienta agentes de IA no VS Code a preparar ambiente, validar dependências e executar testes básicos deste projeto após um clone limpo.

## Objetivo

Garantir que o projeto:
- compile sem erros;
- tenha broker MQTT e Node-RED prontos;
- permita teste de autenticação 2FA (cartão de cor + senha) e comando MQTT;
- rode o fluxo de gravação na BitDogLab quando o hardware estiver conectado.

## Contexto do projeto

- Firmware C para Raspberry Pi Pico W (BitDogLab).
- Build com CMake + Ninja (task de compilação já existe no workspace).
- Integração com MQTT e Node-RED.
- Broker padrão: Mosquitto.
- Autenticação de duplo fator: sensor TCS34725 (cartão colorido) + teclado matricial 4x4 (senha numérica 4 dígitos).

## Modo de execução da IA (obrigatório)

- Executar o fluxo na ordem, sem pular passos.
- Não tentar gravar na placa antes de build aprovado.
- Não tentar validar firmware MQTT antes de validar MQTT local no host.
- Quando faltar dado local (SSID/senha/IP/porta), solicitar ao usuário de forma objetiva e seguir após resposta.
- Preferir `secrets.local.h` e `configura_local.h` para ajustes por máquina.
- Tratar falha de flash com build OK como bloqueio físico/conexão, não como erro de código.

## Fluxo que a IA deve seguir

0. Validar configuração local obrigatória (`secrets.local.h` e `configura_local.h`).
1. Detectar ferramentas instaladas.
2. Instalar somente o que faltar.
3. Validar versões e serviços.
4. Executar build do firmware.
5. Validar MQTT local com publish/subscribe.
6. Iniciar Node-RED.
7. Orientar teste com hardware real (BOOTSEL ou CMSIS-DAP).

## Passo 0: Configuração local obrigatória (clone limpo)

- Verificar se existem `secrets.local.h` e `configura_local.h` na raiz do projeto.
- Se não existirem, copiar:
  - `secrets.local.example.h` -> `secrets.local.h`
  - `configura_local.example.h` -> `configura_local.h`
- Antes de tentar conexão Wi-Fi/MQTT real, confirmar que `secrets.local.h` não está com placeholders.
- Se houver placeholder, pedir ao usuário SSID/senha e IP/porta do broker para preencher os arquivos locais.

## Passo 1: Detecção inicial (Windows)

Verificar se existem:
- winget
- cmake
- ninja
- pasta `%USERPROFILE%\\.pico-sdk` (toolchain instalada pela extensão Raspberry Pi Pico)
- node
- npm
- mosquitto
- mosquitto_pub
- mosquitto_sub

Se node/npm não forem encontrados logo após instalação, considerar PATH desatualizado na sessão atual e usar caminho absoluto:
- C:\Program Files\nodejs\node.exe
- C:\Program Files\nodejs\npm.cmd

## Passo 2: Instalação automática mínima

Se faltar Node.js:
- Instalar `OpenJS.NodeJS.20` via winget.

Se faltar CMake:
- Instalar `Kitware.CMake` via winget.

Se faltar Ninja (e não houver ninja local do Pico SDK):
- Instalar `Ninja-build.Ninja` via winget.

Se faltar toolchain da Pico (`%USERPROFILE%\\.pico-sdk` ausente):
- Solicitar instalação/configuração da extensão Raspberry Pi Pico no VS Code e concluir o setup de ferramentas da extensão.
- Só prosseguir para o build após a toolchain estar disponível.

Se faltar broker MQTT:
- Instalar `EclipseFoundation.Mosquitto` via winget.

Se faltar Node-RED:
- Executar `npm install -g --unsafe-perm node-red`.

Boas práticas:
- Aceitar contratos de origem/pacote no winget para evitar prompt interativo.
- Não instalar ferramentas extras sem necessidade.

## Passo 3: Validação de Runtime

- Confirmar versões de node e npm.
- Confirmar serviço `mosquitto` em execução.
- Confirmar conectividade local na porta 1883 (ou 1884 se em modo user-level).
- Testar MQTT real:
  - publicar em um tópico de teste;
  - assinar o mesmo tópico e validar recebimento.

Exemplo de teste rápido (ajustar porta para 1884 se necessário):
- Subscriber: `mosquitto_sub -h 127.0.0.1 -p 1883 -t copilot/test -C 1`
- Publisher: `mosquitto_pub -h 127.0.0.1 -p 1883 -t copilot/test -m ok`

### Troubleshooting Wi-Fi e MQTT (obrigatório quando travar em "Conectando MQTT")

Se a placa sair de Wi-Fi e ficar presa em "Conectando MQTT", a IA deve:

1. Verificar IP atual da máquina host e confirmar se bate com `MQTT_BROKER_IP` (preferir `configura_local.h`).
2. Confirmar que os overrides locais realmente têm prioridade no firmware:
  - em `configura_geral.h`, após incluir `configura_local.h`, os `#define` padrão de `DEVICE_ID`, `MQTT_BROKER_IP` e `MQTT_BROKER_PORT` devem estar protegidos com `#ifndef`;
  - se não estiverem, o firmware pode continuar tentando conectar no IP/porta antigos mesmo com `configura_local.h` correto.
3. Confirmar que o broker está escutando em rede local, e não apenas localhost.
   - Se `Get-NetTCPConnection` mostrar apenas `127.0.0.1`/`::1`, a Pico não conseguirá conectar.
4. Quando houver permissão de administrador, ajustar `mosquitto.conf` para listener em rede local (ex.: `listener 1883 0.0.0.0`) e reiniciar serviço.
5. Quando NÃO houver permissão de administrador, usar fallback sem admin:
   - subir broker local com `mosquitto.local.conf` em porta 1884 (`listener 1884 0.0.0.0`, `allow_anonymous true`);
   - ajustar `MQTT_BROKER_PORT` em `configura_local.h` para 1884;
   - ajustar broker do Node-RED para 1884.
6. Rodar broker em modo verboso e validar log de conexão do cliente (mensagem tipo `New client connected ... bitdoglab_02_client`).
7. Se ainda falhar, testar hotspot 2.4 GHz no celular e verificar isolamento de clientes (AP/client isolation).

## Passo 4: Node-RED

- Perguntar ao usuário se deseja abrir o Node-RED no navegador interno do VS Code ou no navegador externo do sistema.
- Se preferir navegador interno, abrir no VS Code (Simple Browser) em http://127.0.0.1:1880/.
- Se preferir navegador externo, abrir o navegador padrão em http://127.0.0.1:1880/.
- Iniciar Node-RED.
- Confirmar que o editor responde em http://127.0.0.1:1880/.
- Importar `dashboard_projeto1.json` e fazer `Deploy`.
- Para visualizar o dashboard, usar a URL http://127.0.0.1:1880/ui (o endereço sem /ui abre o editor de fluxos).

### Troubleshooting Node-RED Dashboard (obrigatório quando houver erro de importação)

Se ao importar o fluxo aparecer "Imported unrecognised types" para `ui_*` (ex.: `ui_tab`, `ui_group`, `ui_gauge`, `ui_text`, `ui_button`), a IA deve:

1. Verificar se o pacote de dashboard está instalado no userDir ativo do Node-RED (`%USERPROFILE%\.node-red`).
2. Se não estiver instalado, executar:
   - `npm install node-red-dashboard` (dentro de `%USERPROFILE%\.node-red`).
3. Reiniciar o Node-RED após instalação de nodes.
4. Confirmar no log que o dashboard carregou (mensagem semelhante a `Dashboard version ... started at /ui`).
5. Pedir para o usuário recarregar o editor (Ctrl+F5), reimportar o JSON e fazer `Deploy`.
6. Abrir/confirmar o dashboard em http://127.0.0.1:1880/ui.

Observações:
- Se o usuário vir apenas "Flow 1" e os quadrados dos nodes, ele está no editor de fluxos, não no dashboard.
- Não concluir falha do projeto antes de validar o endpoint `/ui` e o deploy do fluxo.

### Troubleshooting de texto corrompido (mojibake)

Se o dashboard mostrar textos como `Ã`, `ðŸ` ou acentos/emojis quebrados:

1. Confirmar que o arquivo `dashboard_projeto1.json` está em UTF-8.
2. Reimportar o fluxo no Node-RED e fazer `Deploy`.
3. Se o deploy for via API REST, ler e enviar o JSON em UTF-8 explícito.
  - exemplo em PowerShell: `Get-Content -Raw -Encoding utf8` e envio do body como bytes UTF-8.
4. Validar no runtime os textos principais:
  - `🔑 Fechadura 2FA`
  - `🔥 ALARME DE INCÊNDIO 🔥`
  - `Último sinal de vida recebido às:`

## Passo 5: Build do Firmware

Usar a task existente do workspace:
- Compile Project

Se for clone limpo e `build/build.ninja` não existir (ou a task falhar por falta de configuração):
1. Rodar configuração CMake do projeto (preferencialmente pela extensão Raspberry Pi Pico no VS Code).
2. Se necessário, usar fallback via terminal: `cmake -S . -B build -G Ninja`.
3. Reexecutar a task `Compile Project`.

Critério de sucesso:
- Gerar `Projeto1Fechadura2FA.elf` sem erro de compilação/link.

## Passo 6: Gravação na Placa (quando hardware conectado)

Tentar nesta ordem:
1. Run Project (modo BOOTSEL).
2. Flash (OpenOCD/CMSIS-DAP).

Se falhar por dispositivo não detectado:
- informar causa objetiva;
- orientar usuário a conectar em BOOTSEL ou conectar probe CMSIS-DAP;
- não marcar como erro de código enquanto build estiver OK.

## Passo 7: Teste Funcional MQTT do Firmware

Com firmware gravado e placa ligada:
- assinar `bitdoglab_02/#` no Node-RED;
- validar recebimento de:
  - status (estado atual da fechadura: espera, aguardando cartão, aguardando senha, aberto, fechado)
  - historico (log de eventos com timestamps)
  - heartbeat (sinal de "placa viva" periódico)
  - alarme (se houver ativação de modo de emergência)
- publicar comandos MQTT:
  - tópico: `bitdoglab_02/comando/estado`
  - payloads: `ADMIN_SENHA` (alterna modo admin), `INCENDIO` (ativa/desativa alarme de emergência)
- validar resposta visual (LED RGB, matriz de LEDs, display OLED) e eventos MQTT.

Checagem rápida para evitar confusão com firmware de outro projeto:
- Projeto 1 (Fechadura) publica principalmente em `status`, `historico` e `heartbeat`.
- Se aparecer `sensores/temperatura`, `sensores/umidade` e `sensores/luminosidade`, a placa ainda está com firmware da Estufa.

## Passo 8: Teste de Estabilidade

- Evitar enviar comandos MQTT várias vezes em sequência curta durante a validação.
- Testar um comando por vez e aguardar o ciclo completo terminar antes do próximo envio.
- Testar autenticação 2FA real: aproximar cartão → aguardar leitura de cor → digitar senha no teclado.
- Se o sistema travar apenas sob repetição de comandos, tratar isso como problema de robustez do fluxo FIFO, não como falha de sensores.
- Manter o dashboard final sem nodes de teste automáticos que publiquem em loop.

## Checklist rápido de clone limpo (gates)

Antes de encerrar, a IA deve marcar internamente estes gates como OK:

1. Arquivos locais criados e preenchidos (`secrets.local.h`, `configura_local.h`).
2. Dependências mínimas presentes (node/npm, mosquitto, cmake/ninja, toolchain Pico).
3. MQTT local validado com publish/subscribe no host.
4. Build concluído e ELF gerado.
5. Node-RED aberto, fluxo importado e dashboard acessível em `/ui`.
6. Se hardware presente: tentativa de flash executada e resultado classificado (OK ou bloqueio físico).

## Regras de Alteração de Arquivos

- Evitar commits com credenciais. Preferir `secrets.local.h` (ignorado no git).
- Evitar alterar IP de broker em `configura_geral.h` sem avisar o usuário. Preferir `configura_local.h` (ignorado no git).
- Se os arquivos locais não existirem, orientar o usuário a copiar `secrets.local.example.h` -> `secrets.local.h` e `configura_local.example.h` -> `configura_local.h`.
- Se IP do broker local mudar, propor ajuste explícito em `configura_local.h` e recompilar.
- Não reverter mudanças do usuário sem solicitação.

## Formato de Retorno Esperado da IA

Ao final, a IA deve reportar:
1. O que foi detectado como instalado.
2. O que foi instalado automaticamente.
3. Resultado do teste MQTT local.
4. Resultado do build.
5. Resultado da gravação na placa (ou bloqueio físico).
6. Próximos passos objetivos para validação final em bancada.

Template recomendado de saída:

1. Detectado: ...
2. Instalado: ...
3. MQTT local: OK/FAIL (...)
4. Build: OK/FAIL (...)
5. Flash: OK/BLOQUEIO FÍSICO (...)
6. Próximos passos: ...
