//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:												  	//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

#include <stdint.h>
#include "moduloMCP3004.h"
#include "moduloMQ9.h"

extern int _printf(const char *format, ...);

// Convierte lectura ADC (0..1023) a escala intermedia (mV aprox)
static int adc_a_ppm(int raw)
{
	return (raw * MQ9_ADC_ESCALA) / MQ9_ADC_BITS;
}

static int mq9_ppm_from_scaled(int escalado, int div)
{
	if (div == 0) return 0;
	// Ajuste por divisor específico de cada gas
	return (10 * escalado) / div;
}

int lee_MQ9_CO(uint8_t canal)
{
	// Lectura del canal y conversion a ppm para CO
	int raw = lee_MCP(canal);
	int escalado = adc_a_ppm(raw);
	return mq9_ppm_from_scaled(escalado, MQ9_CO_DIV);
}

int lee_MQ9_CH4(uint8_t canal)
{
	// Lectura del canal y conversion a ppm para CH4
	int raw = lee_MCP(canal);
	int escalado = adc_a_ppm(raw);
	return mq9_ppm_from_scaled(escalado, MQ9_CH4_DIV);
}

void lee_gas(void)
{
	// Arranca la fase de calentamiento del sensor
	estado = 0;
	ENABLE_5V;
	IRQEN |= IRQ_TIMER;
	MAX_COUNT = 1 * 1000000;
}

void lee_CO(void)
{
	// Mide CO y configura el siguiente intervalo de espera
	int read_CO = lee_MQ9_CO(MCP3004_CH0);

	DISABLE_5V_1V4;
	ENABLE_1V4;

	IRQEN |= IRQ_TIMER;
	MAX_COUNT = 1 * 1000000;

	estado = 1;

	_printf("Midiendo CO, esperar 60 segundos...\n");
	_printf("CO medido: %d ppm \n", read_CO);
}

void lee_CH4(void)
{
	// Mide CH4 y finaliza la secuencia
	int read_CH4 = lee_MQ9_CH4(MCP3004_CH0);

	DISABLE_5V_1V4;

	estado = 2;

	_printf("Midiendo CH4, esperar 90 segundos...\n");
	_printf("CH4 medido: %d ppm \n", read_CH4);
}
