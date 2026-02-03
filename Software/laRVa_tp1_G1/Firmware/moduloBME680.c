//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:												  	//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

#include <stdint.h>
#include "moduloBME680.h"

// Funciones de comunicación SPI (declaraciones externas)
extern uint8_t spi_transf(uint8_t data);
static int32_t bme680_t_fine = 0;


// Configura los parámetros de oversampling para humedad, temperatura y presión.
void inicia_BME(void) {
    // Reset de los registros del módulo
    escribe_BME(0xB6, BME_RESET);
    escribe_BME(0x10, BME_ESTADO);
    
    // Configuracion de oversampling y gas heater
    escribe_BME(0x01, CTRL_HUM);
    escribe_BME(0x54, CTRL_MEAS);
    escribe_BME(0x59, gas_wait0);
    escribe_BME(0x00, res_heat0);
    escribe_BME(0x10, CTRL_GAS_1);
    escribe_BME(0x55, CTRL_MEAS);
    _delay_ms(100);
}

// Escribe un dato en un registro del BME680
void escribe_BME(char data, char dir) {
    // Escritura SPI: direccion seguida del dato
    SPISS = BME_CS;               
    spi_transf(0x00 | dir);           
    spi_transf(data);                 
    SPISS = 0b11;                   
}

// Lee un dato de un registro del BME680
char lee_BME(char dir) {
    unsigned char readDATA;
    
    // Lectura SPI: direccion con bit de lectura y un dummy para recibir
    SPISS = BME_CS;              
    spi_transf(0x80 | dir);           
    readDATA = spi_transf(0);         
    SPISS = 0b11;
    
    return readDATA;
}

// Calcula y retorna la temperatura compensada
int devuelve_temp(void) {
    int32_t par_t1, par_t2, par_t3, temp_adc;
    int32_t var1, var2, var3, t_fine, temp_comp;
    
    escribe_BME(0x00, BME_ESTADO);
    
    // Coeficientes de calibracion de temperatura
    par_t1 = (int32_t)((lee_BME(0xEA) << 8) | lee_BME(0xE9));
    par_t2 = (int32_t)((lee_BME(0x8B) << 8) | lee_BME(0x8A));
    par_t3 = (int32_t)lee_BME(0x8C);
    
    escribe_BME(0x10, BME_ESTADO);
    
    temp_adc = (int32_t)((lee_BME(0x22) << 12) | 
                         (lee_BME(0x23) << 4) | 
                         (lee_BME(0x24) >> 4));
    
    // Compensacion segun datasheet (t_fine se reutiliza en presion)
    var1 = ((int32_t)temp_adc >> 3) - ((int32_t)par_t1 << 1);
    var2 = (var1 * (int32_t)par_t2) >> 11;
    var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12) * ((int32_t)par_t3 << 4)) >> 14;
    t_fine = var2 + var3;
    bme680_t_fine = t_fine;
    temp_comp = ((t_fine * 5) + 128) >> 8;
    
    return temp_comp;
}

// Calcula y retorna la presión compensada
int devuelve_pres(void) {
    int32_t par_p1, par_p2, par_p3, par_p4, par_p5;
    int32_t par_p6, par_p7, par_p8, par_p9, par_p10;
    int32_t press_adc, press_comp;
    int32_t var1, var2, var3;
    int32_t t_fine = bme680_t_fine;
    
    escribe_BME(0x00, BME_ESTADO);
    
    // Coeficientes de calibracion de presion
    par_p1 = (int32_t)((lee_BME(0x8F) << 8) | lee_BME(0x8E));
    par_p2 = (int32_t)((lee_BME(0x91) << 8) | lee_BME(0x90));
    par_p3 = (int32_t)lee_BME(0x92);
    par_p4 = (int32_t)((lee_BME(0x95) << 8) | lee_BME(0x94));
    par_p5 = (int32_t)((lee_BME(0x97) << 8) | lee_BME(0x96));
    par_p6 = (int32_t)lee_BME(0x99);
    par_p7 = (int32_t)lee_BME(0x98);
    par_p8 = (int32_t)((lee_BME(0x9D) << 8) | lee_BME(0x9C));
    par_p9 = (int32_t)((lee_BME(0x9F) << 8) | lee_BME(0x9E));
    par_p10 = (int32_t)lee_BME(0xA0);
    
    escribe_BME(0x10, BME_ESTADO);
    
    press_adc = (int32_t)((lee_BME(0x1F) << 12) | 
                          (lee_BME(0x20) << 4) | 
                          (lee_BME(0x21) >> 4));
    
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

// Calcula y retorna la humedad compensada
int devuelve_hum(int temp_comp) {
    int32_t par_h1, par_h2, par_h3, par_h4, par_h5, par_h6, par_h7;
    int32_t hum_adc, temp_scaled, hum_comp;
    int32_t var1, var2, var3, var4, var5, var6;
    
    escribe_BME(0x00, BME_ESTADO);
    
    // Coeficientes de calibracion de humedad
    par_h1 = (int32_t)((lee_BME(0xE3) << 4) | (lee_BME(0xE2) & 0b00001111));
    par_h2 = (int32_t)((lee_BME(0xE1) << 4) | (lee_BME(0xE2) >> 4));
    par_h3 = (int32_t)lee_BME(0xE4);
    par_h4 = (int32_t)lee_BME(0xE5);
    par_h5 = (int32_t)lee_BME(0xE6);
    par_h6 = (int32_t)lee_BME(0xE7);
    par_h7 = (int32_t)lee_BME(0xE8);
    
    escribe_BME(0x10, BME_ESTADO);
    
    hum_adc = (int32_t)((lee_BME(0x25) << 8) | lee_BME(0x26));
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
