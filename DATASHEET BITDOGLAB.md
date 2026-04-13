# Datasheet BITDOGLAB

## Itens pendentes de validacao

Os seguintes componentes e funcionalidades ainda nao foram totalmente testados ou confirmados:

- Matriz de LEDs RGB (NeoPixel - conector de expansao Dout)
  - Verificar fisicamente se ha um conector de saida de dados (Dout, 5V, GND) para encadeamento de mais LEDs.
- Microfone de eletreto (GP28 / ana-in)
  - Realizar um teste de leitura funcional para confirmar a variacao do sinal ADC com o som.
- Conector IDC de 14 pinos (comunicacao SPI)
  - Testar a funcionalidade SPI usando os pinos GPIO16 (RX), GPIO17 (CSn), GPIO18 (SCK) e GPIO19 (TX) neste conector.
- Barra de terminais (garras jacare) - lado direito (digitais)
  - Confirmar qual GPIO (0, 1, 2 ou 3) corresponde a qual rotulo fisico (3, 2, 1, 0) atraves de teste.
- Barra de terminais (garras jacare) - lado esquerdo (analogica / alimentacao)
  - Confirmar ana-in como GPIO28 atraves de teste de leitura ADC.

## Conector IDC de 14 pinos

| Pino | Funcao |
| --- | --- |
| 1 | GND |
| 2 | 5V |
| 3 | 3V3 |
| 4 | GP8 |
| 5 | GP28 |
| 6 | GP9 |
| 7 | GND (analogico?) |
| 8 | GP4 |
| 9 | GP17 |
| 10 | GP20 (observacao: nos datasheets originais, este pino e listado como GND) |
| 11 | GP16 |
| 12 | GP19 |
| 13 | GND |
| 14 | GP18 |

## Expansao I2C

| Conector | SDA | SCL |
| --- | --- | --- |
| I2C0 | GPIO0 | GPIO1 |
| I2C1 | GPIO2 | GPIO3 |

## Raspberry Pi Pico W

### LED RGB

- Tipo: catodo comum
- Vermelho: GPIO13
- Verde: GPIO11
- Azul: GPIO12
- Resistores:
  - Vermelho: 220 ohm
  - Verde: 220 ohm
  - Azul: 150 ohm

### Botoes

- Botao A: GPIO5, ativo baixo, requer pull-up interno
- Botao B: GPIO6, ativo baixo, requer pull-up interno
- Botao do joystick (SW): GPIO22, ativo baixo, requer pull-up interno

### Buzzers

- Buzzer A: GPIO21, buzzer passivo via transistor
- Buzzer B: GPIO10, buzzer passivo

### Matriz de LEDs RGB

- Tipo: NeoPixel WS2812B 5x5
- Pino de dados (In): GPIO7
- Numero de LEDs: 25 LEDs

### Joystick analogico KY023

- VRy (eixo Y): GPIO26 (ADC0)
- VRx (eixo X): GPIO27 (ADC1)
- Botao SW: GPIO22

### Display OLED

- Controlador: SSD1306
- Tamanho: 128 colunas x 64 linhas
- SDA: GPIO14
- SCL: GPIO15
- Endereco I2C padrao: 0x3C
- Biblioteca necessaria: compativel com drivers SSD1306

### Microfone de eletreto

- Saida analogica: GP28
- Nivel de sinal: nivel medio de 1.65V, faixa de 0V a 3.3V
