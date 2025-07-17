/**
 * @file tcs34725.c
 * @brief Implementação do driver para o sensor de cor I2C TCS34725.
 */

#include "tcs34725.h"

// --- Definições Internas de Registradores do Sensor ---

// O bit de comando deve ser setado para '1' para indicar ao sensor
// que estamos acessando um de seus registradores ou iniciando uma transação de dados.
#define TCS34725_COMMAND_BIT 0x80

// Endereços dos Registradores
#define TCS34725_ENABLE_REG   0x00 // Registro de habilitação (liga/desliga o sensor e ADCs)
#define TCS34725_ATIME_REG    0x01 // Registro de tempo de integração do ADC
#define TCS34725_CONTROL_REG  0x0F // Registro de controle (ganho do sensor)
#define TCS34725_ID_REG       0x12 // Registro de ID do dispositivo
#define TCS34725_CDATAL_REG   0x14 // Endereço inicial dos dados de cor (Clear, Low Byte)


// --- Implementação das Funções Públicas ---

/**
 * @brief Configura o sensor TCS34725 com parâmetros padrão e o liga.
 * Checa o ID do dispositivo para garantir a comunicação.
 * @param i2c_port A instância do I2C já inicializada onde o sensor está conectado.
 * @return true se o sensor foi inicializado com sucesso, false caso contrário.
 */
bool tcs34725_init(i2c_inst_t* i2c) {
    // 1. Verifica o ID do chip para garantir a comunicação com o sensor correto.
    uint8_t id_reg = TCS34725_COMMAND_BIT | TCS34725_ID_REG;
    uint8_t chip_id;
    int ret = i2c_write_blocking(i2c, TCS34725_ADDR, &id_reg, 1, true); // Envia endereço do registro ID
    if (ret < 0) return false; // Erro de escrita I2C
    ret = i2c_read_blocking(i2c, TCS34725_ADDR, &chip_id, 1, false);     // Lê o ID
    if (ret < 0) return false; // Erro de leitura I2C

    // O ID de um TCS34725 é 0x44 e de um TCS34727 é 0x4D. Ambos são compatíveis.
    if (chip_id != 0x44 && chip_id != 0x4D) {
        return false; // ID não corresponde ao esperado
    }

    // 2. Configura o tempo de integração do ADC (ATIME).
    // O valor 0xEB resulta em (256 - 235) * 2.4ms = 50.4ms de tempo de integração.
    uint8_t atime_cmd[] = {TCS34725_COMMAND_BIT | TCS34725_ATIME_REG, 0xEB};
    ret = i2c_write_blocking(i2c, TCS34725_ADDR, atime_cmd, 2, false);
    if (ret < 0) return false;

    // 3. Configura o ganho (gain) do amplificador para 1x (0x00).
    uint8_t control_cmd[] = {TCS34725_COMMAND_BIT | TCS34725_CONTROL_REG, 0x00};
    ret = i2c_write_blocking(i2c, TCS34725_ADDR, control_cmd, 2, false);
    if (ret < 0) return false;
    
    // 4. Habilita o oscilador interno (PON - Power ON) e os conversores ADC (AEN - RGBC Enable).
    uint8_t enable_cmd[] = {TCS34725_COMMAND_BIT | TCS34725_ENABLE_REG, 0x03}; // PON=1, AEN=1
    ret = i2c_write_blocking(i2c, TCS34725_ADDR, enable_cmd, 2, false);
    if (ret < 0) return false;

    // Pequena pausa para a primeira conversão ser estabilizada após habilitar o sensor.
    sleep_ms(3); 

    return true; // Inicialização bem-sucedida
}

/**
 * @brief Lê os valores brutos dos quatro canais de cor do sensor.
 * @param i2c_port A instância do I2C onde o sensor está conectado.
 * @param colors Ponteiro para uma estrutura tcs34725_color_data_t onde os dados lidos serão armazenados.
 */
void tcs34725_read_colors(i2c_inst_t* i2c, tcs34725_color_data_t* colors) {
    uint8_t buffer[8]; // Buffer para armazenar os 8 bytes de dados (2 bytes por canal: CL, CH, RL, RH, GL, GH, BL, BH)
    
    // Define o registrador inicial a partir do qual queremos ler (Clear Data Low Byte)
    // O bit de comando é setado para indicar que é uma operação de leitura de registro.
    uint8_t start_reg = TCS34725_COMMAND_BIT | TCS34725_CDATAL_REG;

    // Pede ao sensor para ler 8 bytes em sequência a partir do registrador inicial.
    // 'true' no primeiro write_blocking mantém o controle do barramento I2C, permitindo um "repeated start"
    // antes da leitura, o que é comum para ler múltiplos bytes de registradores.
    i2c_write_blocking(i2c, TCS34725_ADDR, &start_reg, 1, true); 
    i2c_read_blocking(i2c, TCS34725_ADDR, buffer, 8, false); // Lê os 8 bytes

    // Os dados chegam como pares de bytes (Low, High). Recombina-os em valores de 16 bits.
    // (High Byte << 8) | Low Byte
    colors->clear = (buffer[1] << 8) | buffer[0];
    colors->red   = (buffer[3] << 8) | buffer[2];
    colors->green = (buffer[5] << 8) | buffer[4];
    colors->blue  = (buffer[7] << 8) | buffer[6];
}