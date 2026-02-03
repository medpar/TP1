#include "moduloMCP3004.h"

static int MCP3004_ReadADC(uint8_t channel, uint8_t single)
{
	if (channel >= MCP3004_CHANNELS) return 0;

	uint8_t data[3] = {0, 0, 0};
	data[0] = 0x01;
	data[1] = single ? 0x80 : 0x00;
	data[1] |= (channel << 4);

	SPISS = ADC_CS;
	data[0] = spixfer(data[0]);
	data[1] = spixfer(data[1]);
	data[2] = spixfer(data[2]);
	SPISS = 0b11;

	return ((data[1] << 8) | data[2]) & MCP3004_MAX_VALUE;
}

int MCP3004_Read(uint8_t channel)
{
	return MCP3004_ReadADC(channel, 1);
}

int MCP3004_DifferentialRead(uint8_t channel)
{
	return MCP3004_ReadADC(channel, 0);
}

void MCP3004_ReadMultiple(const uint8_t *channels, uint8_t numChannels, int *readings)
{
	uint8_t i;
	for (i = 0; i < numChannels; i++) {
		readings[i] = MCP3004_Read(channels[i]);
	}
}

int MCP3004_Read_Reg(unsigned char channel)
{
	return MCP3004_Read((uint8_t)channel);
}

int readMCP3004(unsigned char channel)
{
	return MCP3004_Read((uint8_t)channel);
}
