#include <stdint.h>

// Funciones de comunicación SPI (declaraciones externas)
extern uint8_t spixfer(uint8_t data);

// Definiciones de registros BME680 (deberían estar en un header)
#define RESET_BME       0xE0
#define STATUS_BME      0x73
#define CTRL_HUM        0x72
#define CTRL_MEAS       0x74
#define CTRL_GAS_1      0x71
#define gas_wait0       0x64
#define res_heat0       0x5A
#define CS_BME680       0b10

static int32_t bme680_t_fine = 0;

/**
 * @brief Inicializa el módulo BME680
 * 
 * Configura los parámetros de oversampling para humedad, temperatura y presión,
 * así como los ajustes para las mediciones de gas.
 */
void startBME680(void) {
    // Reset de los registros del módulo
    writeBME680(0xB6, RESET_BME);
    writeBME680(0x10, STATUS_BME);
    
    // 1. Set humidity oversampling to 1x by writing 0b001 to osrs_h
    writeBME680(0x01, CTRL_HUM);
    
    // 2. Set temperature oversampling to 2x by writing 0b010 to osrs_t
    // 3. Set pressure oversampling to 16x by writing 0b101 to osrs_p
    writeBME680(0x54, CTRL_MEAS);
    
    // 4. Set gas_wait_0 to 0x59 to select 100 ms heat up duration
    writeBME680(0x59, gas_wait0);
    
    // 5. Set the corresponding heater set-point by writing the target heater resistance to res_heat_0
    writeBME680(0x00, res_heat0);
    
    // 6. Set nb_conv to 0x0 to select the previously defined heater settings
    // 7. Set run_gas_l to 1 to enable gas measurements
    writeBME680(0x10, CTRL_GAS_1);
    
    // 8. Set mode to 0b01 to trigger a single measurement
    writeBME680(0x55, CTRL_MEAS);
}

/**
 * @brief Escribe un dato en un registro del BME680
 * 
 * @param data Dato a escribir
 * @param dir Dirección del registro
 */
void writeBME680(char data, char dir) {
    SPISS = CS_BME680;              // Seleccionamos el chipSelect
    spixfer(0x00 | dir);            // Pasamos dirección
    spixfer(data);                  // Pasamos dato
    SPISS = 0b11;                   // Devolvemos a 1 chip select
}

/**
 * @brief Lee un dato de un registro del BME680
 * 
 * @param dir Dirección del registro a leer
 * @return Valor leído del registro
 */
char readBME680(char dir) {
    unsigned char readDATA;
    
    SPISS = CS_BME680;              // Seleccionamos chip select
    spixfer(0x80 | dir);            // Lectura del SPI
    readDATA = spixfer(0);          // Enviamos 0
    SPISS = 0b11;
    
    return readDATA;
}

/**
 * @brief Calcula y retorna la temperatura compensada
 * 
 * @return Temperatura compensada en centígrados * 100
 */
int returnTemp(void) {
    int32_t par_t1, par_t2, par_t3, temp_adc;
    int32_t var1, var2, var3, t_fine, temp_comp;
    
    // Entramos a la página 0 de los registros
    writeBME680(0x00, STATUS_BME);
    
    par_t1 = (int32_t)((readBME680(0xEA) << 8) | readBME680(0xE9));
    par_t2 = (int32_t)((readBME680(0x8B) << 8) | readBME680(0x8A));
    par_t3 = (int32_t)readBME680(0x8C);
    
    // Entramos a la página 1 de los registros
    writeBME680(0x10, STATUS_BME);
    
    temp_adc = (int32_t)((readBME680(0x22) << 12) | 
                         (readBME680(0x23) << 4) | 
                         (readBME680(0x24) >> 4));
    
    var1 = ((int32_t)temp_adc >> 3) - ((int32_t)par_t1 << 1);
    var2 = (var1 * (int32_t)par_t2) >> 11;
    var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12) * ((int32_t)par_t3 << 4)) >> 14;
    t_fine = var2 + var3;
    bme680_t_fine = t_fine;
    temp_comp = ((t_fine * 5) + 128) >> 8;
    
    return temp_comp;
}

/**
 * @brief Calcula y retorna la presión compensada
 * 
 * @return Presión compensada en Pascales
 */
int returnPressure(void) {
    int32_t par_p1, par_p2, par_p3, par_p4, par_p5;
    int32_t par_p6, par_p7, par_p8, par_p9, par_p10;
    int32_t press_adc, press_comp;
    int32_t var1, var2, var3;
    int32_t t_fine = bme680_t_fine;
    
    // Entramos a la página 0 de los registros
    writeBME680(0x00, STATUS_BME);
    
    par_p1 = (int32_t)((readBME680(0x8F) << 8) | readBME680(0x8E));
    par_p2 = (int32_t)((readBME680(0x91) << 8) | readBME680(0x90));
    par_p3 = (int32_t)readBME680(0x92);
    par_p4 = (int32_t)((readBME680(0x95) << 8) | readBME680(0x94));
    par_p5 = (int32_t)((readBME680(0x97) << 8) | readBME680(0x96));
    par_p6 = (int32_t)readBME680(0x99);
    par_p7 = (int32_t)readBME680(0x98);
    par_p8 = (int32_t)((readBME680(0x9D) << 8) | readBME680(0x9C));
    par_p9 = (int32_t)((readBME680(0x9F) << 8) | readBME680(0x9E));
    par_p10 = (int32_t)readBME680(0xA0);
    
    // Entramos a la página 1 de los registros
    writeBME680(0x10, STATUS_BME);
    
    press_adc = (int32_t)((readBME680(0x1F) << 12) | 
                          (readBME680(0x20) << 4) | 
                          (readBME680(0x21) >> 4));
    
    var1 = ((int32_t)t_fine >> 1) - 64000;
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)par_p6) >> 2;
    var2 = var2 + ((var1 * (int32_t)par_p5) << 1);
    var2 = (var2 >> 2) + ((int32_t)par_p4 << 16);
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)par_p3 << 5)) >> 3) + 
           (((int32_t)par_p2 * var1) >> 1);
    var1 = var1 >> 18;
    var1 = ((32768 + var1) * (int32_t)par_p1) >> 15;
    press_comp = 1048576 - press_adc;
    press_comp = (uint32_t)((press_comp - (var2 >> 12)) * ((uint32_t)3125));
    
    if (press_comp >= (1 << 30)) {
        press_comp = ((press_comp / (uint32_t)var1) << 1);
    } else {
        press_comp = ((press_comp << 1) / (uint32_t)var1);
    }
    
    var1 = ((int32_t)par_p9 * (int32_t)(((press_comp >> 3) * (press_comp >> 3)) >> 13)) >> 12;
    var2 = ((int32_t)(press_comp >> 2) * (int32_t)par_p8) >> 13;
    var3 = ((int32_t)(press_comp >> 8) * (int32_t)(press_comp >> 8) * 
            (int32_t)(press_comp >> 8) * (int32_t)par_p10) >> 17;
    press_comp = (int32_t)(press_comp) + ((var1 + var2 + var3 + ((int32_t)par_p7 << 7)) >> 4);
    
    return press_comp;
}

/**
 * @brief Calcula y retorna la humedad compensada
 * 
 * @param temp_comp Temperatura compensada previamente calculada
 * @return Humedad relativa compensada (valor * 1000)
 */
int returnHUMIDITY(int temp_comp) {
    int32_t par_h1, par_h2, par_h3, par_h4, par_h5, par_h6, par_h7;
    int32_t hum_adc, temp_scaled, hum_comp;
    int32_t var1, var2, var3, var4, var5, var6;
    
    // Entramos a la página 0 de los registros
    writeBME680(0x00, STATUS_BME);
    
    par_h1 = (int32_t)((readBME680(0xE3) << 4) | (readBME680(0xE2) & 0b00001111));
    par_h2 = (int32_t)((readBME680(0xE1) << 4) | (readBME680(0xE2) >> 4));
    par_h3 = (int32_t)readBME680(0xE4);
    par_h4 = (int32_t)readBME680(0xE5);
    par_h5 = (int32_t)readBME680(0xE6);
    par_h6 = (int32_t)readBME680(0xE7);
    par_h7 = (int32_t)readBME680(0xE8);
    
    // Entramos a la página 1 de los registros
    writeBME680(0x10, STATUS_BME);
    
    hum_adc = (int32_t)((readBME680(0x25) << 8) | readBME680(0x26));
    temp_scaled = (int32_t)temp_comp;
    
    var1 = (int32_t)hum_adc - (int32_t)((int32_t)par_h1 << 4) - 
           (((temp_scaled * (int32_t)par_h3) / ((int32_t)100)) >> 1);
    var2 = ((int32_t)par_h2 * 
            (((temp_scaled * (int32_t)par_h4) / ((int32_t)100)) + 
             (((temp_scaled * ((temp_scaled * (int32_t)par_h5) / ((int32_t)100))) >> 6) / 
              ((int32_t)100)) + 
             ((int32_t)(1 << 14)))) >> 10;
    var3 = var1 * var2;
    var4 = (((int32_t)par_h6 << 7) + ((temp_scaled * (int32_t)par_h7) / ((int32_t)100))) >> 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;
    hum_comp = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;
    
    return hum_comp;
}
