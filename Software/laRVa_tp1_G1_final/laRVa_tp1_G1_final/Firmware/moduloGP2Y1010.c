#include "GP2Y1010.h"
#include "MCP3004.h"

extern int _printf(const char *format, ...);

static uint32_t gp2y1010_us_to_ticks(uint32_t us)
{
	return (CCLK / 1000000u) * us;
}

int GP2Y1010_ReadRaw(uint8_t channel)
{
	uint32_t sample_ticks = gp2y1010_us_to_ticks(GP2Y1010_SAMPLE_US);
	uint32_t total_ticks = gp2y1010_us_to_ticks(GP2Y1010_LED_ON_US);
	uint32_t start = TIMER;
	uint32_t t_sample;
	uint32_t t_off;
	int adc;

	GPOUT |= M2_ON_OFF;
	while ((uint32_t)(TIMER - start) < sample_ticks) {
	}
	t_sample = TIMER;
	adc = MCP3004_Read(channel);
	while ((uint32_t)(TIMER - start) < total_ticks) {
	}
	t_off = TIMER;

	return adc;
}
