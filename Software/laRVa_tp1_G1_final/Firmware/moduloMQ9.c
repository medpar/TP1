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

int MQ9_ReadCOppm(uint8_t channel)
{
	int raw = MCP3004_Read(channel);
	int scaled = mq9_scale_adc(raw);
	return mq9_ppm_from_scaled(scaled, MQ9_CO_DIV);
}

int MQ9_ReadCH4ppm(uint8_t channel)
{
	int raw = MCP3004_Read(channel);
	int scaled = mq9_scale_adc(raw);
	return mq9_ppm_from_scaled(scaled, MQ9_CH4_DIV);
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
	int read_CO = MQ9_ReadCOppm(MCP3004_CH0);

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
	int read_CH4 = MQ9_ReadCH4ppm(MCP3004_CH0);

	DISABLE_5V_1V4;

	state = 2;

	_printf("Lectura CH4: %d ppm \n", read_CH4);
}
