//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:												  	//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

#define EN_5V_UP   (1U << 7)
#define EN_5V_M4   (1U << 6)
#define EN_1V4_M4  (1U << 5)

#define ENABLE_5V       (GPOUT = (GPOUT & ~EN_1V4_M4) | EN_5V_UP | EN_5V_M4)
#define ENABLE_1V4      (GPOUT = (GPOUT & ~(EN_5V_UP | EN_5V_M4)) | EN_1V4_M4)
#define DISABLE_5V_1V4  (GPOUT &= ~(EN_5V_UP | EN_5V_M4 | EN_1V4_M4))

// Constantes de conversion de ADC a ppm
#define MQ9_CO_DIV 7
#define MQ9_CH4_DIV 1
#define MQ9_ADC_BITS 1024
#define MQ9_ADC_ESCALA 330


int lee_MQ9_CO(uint8_t canal);
int lee_MQ9_CH4(uint8_t canal);
void lee_gas();
void lee_CO();
void lee_CH4();
