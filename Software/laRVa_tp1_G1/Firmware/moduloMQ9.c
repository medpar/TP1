#include <stdint.h>

#include "moduloMCP3004.h"
#include "moduloMQ9.h"

extern int _printf(const char *format, ...);

#define MQ9_ADC_BITS 1024
#define MQ9_ADC_SCALE 330
#define MQ9_CO_DIV 7
#define MQ9_CH4_DIV 1

static int mq9_scale_adc(int raw)
{
	return (raw * MQ9_ADC_SCALE) / MQ9_ADC_BITS;
}

static int mq9_ppm_from_scaled(int scaled, int div)
{
	if (div == 0) return 0;
	return (10 * scaled) / div;
}

void readGas(void)
{
	state = 0;
	ENABLE_5V;
	IRQEN |= IRQ_TIMER;
	MAX_COUNT = 1 * 1000000;
}

void readCO(void)
{
	int gas_CO = MCP3004_Read(MCP3004_CH0);
	int scaled = mq9_scale_adc(gas_CO);
	int read_CO = mq9_ppm_from_scaled(scaled, MQ9_CO_DIV);

	DISABLE_5V_1V4;
	ENABLE_1V4;

	IRQEN |= IRQ_TIMER;
	MAX_COUNT = 1 * 1000000;

	state = 1;

	_printf("Medicion de CO en curso, por favor espere 60 seg.\n");
	_printf("Lectura CO: %d ppm \n", read_CO);
	_printf("Medicion de CH4 en curso, por favor espere 90 seg.\n");
}

void readCH4(void)
{
	int gas_CH4 = MCP3004_Read(MCP3004_CH0);
	int scaled = mq9_scale_adc(gas_CH4);
	int read_CH4 = mq9_ppm_from_scaled(scaled, MQ9_CH4_DIV);

	DISABLE_5V_1V4;

	state = 2;

	_printf("Lectura CH4: %d ppm \n", read_CH4);
}
