//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:													//	
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia				//
//////////////////////////////////////////////////////////////////

#include "moduloGP2Y1010.h"
#include "moduloMCP3004.h"

extern int _printf(const char *format, ...);

// Convierte microsegundos a ciclos de reloj del sistema
static uint32_t convierte_pasos(uint32_t us)
{
	return (CCLK / 1000000u) * us;
}

// Lee el valor del sensor a través del ADC
int lee_GP2Y(uint8_t canal)
{
	uint32_t pasos_muestreo = convierte_pasos(G2PY_US_MUESTREO);
	uint32_t pasos_totales = convierte_pasos(G2PY_US_LED);
	uint32_t start = TIMER;
	uint32_t t_muestreo;
	uint32_t t_apagado;
	int adc;

	// Enciende el LED IR y espera el tiempo de muestreo antes de leer el ADC
	GPOUT |= M2_ON_OFF;
	while ((uint32_t)(TIMER - start) < pasos_muestreo) {
	}
	t_muestreo = TIMER;

	// Lee el valor del sensor a través del ADC
	adc = lee_MCP(canal);
	while ((uint32_t)(TIMER - start) < pasos_totales) {
	}
	t_apagado = TIMER;

	// Devuelve el valor leído por el ADC
	return adc;
}
