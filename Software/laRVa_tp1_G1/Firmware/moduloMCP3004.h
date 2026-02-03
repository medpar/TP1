//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:													//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>

// Seleccion de chip para el ADC MCP3004
#define MCP_CS 0b01

#define MCP_CANALES 4
#define MCP_MAXIMO 1023

// Canales del MCP3004
#define MCP_0 0
#define MCP_1 1
#define MCP_2 2
#define MCP_3 3

uint8_t spi_transf(uint8_t d);

// Lecturas del ADC
int lee_MCP(uint8_t canal);
int lee_MCP_diferencial(uint8_t canal);
void lee_MCP_multiple(const uint8_t *canales, uint8_t numCanales, int *readings);
int lee_MCP_reg(unsigned char canal);
