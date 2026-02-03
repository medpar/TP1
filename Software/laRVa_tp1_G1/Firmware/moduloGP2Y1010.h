//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:													//	
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia				//
//////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>

// Tiempos de muestreo y apagado del sensor
#define G2PY_US_LED 320u
#define G2PY_US_MUESTREO 280u

int lee_GP2Y(uint8_t canal);
