#pragma once

#include <stdint.h>

#define ADC_CS 0b01

#define MCP3004_CHANNELS 4
#define MCP3004_MAX_VALUE 1023

#define MCP3004_CH0 0
#define MCP3004_CH1 1
#define MCP3004_CH2 2
#define MCP3004_CH3 3

uint8_t spixfer(uint8_t d);

int MCP3004_Read(uint8_t channel);
int MCP3004_DifferentialRead(uint8_t channel);
void MCP3004_ReadMultiple(const uint8_t *channels, uint8_t numChannels, int *readings);
int MCP3004_Read_Reg(unsigned char channel);

int readMCP3004(unsigned char channel);
