//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:												  	//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

// Definicion de los registros del BME680

#define BME_CS 0b10
#define BME_RESET 0xE0
#define BME_ESTADO 0x73
#define BME_ID 0xD0
#define BME_CONFIG 0x75

#define CTRL_MEAS 0x74
#define CTRL_HUM 0x72
#define CTRL_GAS_1 0x71
#define CTRL_GAS_0 0x70

#define gas_wait0 0x64
#define gas_wait1 0x65
#define gas_wait2 0x66
#define gas_wait3 0x67
#define gas_wait4 0x68
#define gas_wait5 0x69
#define gas_wait6 0x6A
#define gas_wait7 0x6B
#define gas_wait8 0x6C
#define gas_wait9 0x6D

#define res_heat0 0x5A
#define res_heat1 0x5B
#define res_heat2 0x5C
#define res_heat3 0x5D
#define res_heat4 0x5E
#define res_heat5 0x5F
#define res_heat6 0x60
#define res_heat7 0x61
#define res_heat8 0x62
#define res_heat9 0x63

void inicia_BME();
void escribe_BME(char data,char dir);
char lee_BME(char dir);
int devuelve_temp();
int devuelve_pres();
int devuelve_hum(int temp_comp);
