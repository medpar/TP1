//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:												  	//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

#include "moduloMCP3004.h"

// Funcion para leer el ADC MCP3004
static int lee_MCP_ADC(uint8_t canal, uint8_t single)
{
	if (canal >= MCP_CANALES) return 0;

	// Envia comando de lectura por SPI (single o diferencial)
	uint8_t datos[3] = {0, 0, 0};
	datos[0] = 0x01;
	datos[1] = single ? 0x80 : 0x00;
	datos[1] |= (canal << 4);

	// Seleccion del ADC y transferencia SPI
	SPISS = MCP_CS;
	datos[0] = spi_transf(datos[0]);
	datos[1] = spi_transf(datos[1]);
	datos[2] = spi_transf(datos[2]);
	SPISS = 0b11;

	return ((datos[1] << 8) | datos[2]) & MCP_MAXIMO;
}

// Funcion para leer el ADC MCP3004 en modo single-ended
int lee_MCP(uint8_t canal)
{
	return lee_MCP_ADC(canal, 1);
}

// Funcion para leer el ADC MCP3004 en modo diferencial
int lee_MCP_diferencial(uint8_t canal)
{
	return lee_MCP_ADC(canal, 0);
}

// Funcion para leer el ADC MCP3004 en modo multiple
void lee_MCP_multiple(const uint8_t *canales, uint8_t numCanales, int *readings)
{
	uint8_t i;
	for (i = 0; i < numCanales; i++) {
		readings[i] = lee_MCP(canales[i]);
	}
}

// Funcion para leer el ADC MCP3004 en modo registro
int lee_MCP_reg(unsigned char canal)
{
	return lee_MCP((uint8_t)canal);
}
