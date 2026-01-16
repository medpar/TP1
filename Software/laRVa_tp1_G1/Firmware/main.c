

#include <stdint.h>

// Modulos externos
#include "moduloMQ9.h"
#include "moduloBME680.h"
#include "moduloTEL0132.h"
#include "lora_sx1262.h"
//#include "SPI.h"
#include "moduloMCP3004.h"
#include "moduloGP2Y1010.h"

// Tipos de datos
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef signed char  s8;
typedef signed short s16;
typedef signed int   s32;

volatile int state;  


//-- Registros mapeados
// UART0
#define UARTDAT  (*(volatile uint8_t*)0xE0000080)
#define UARTSTA  (*(volatile uint32_t*)0xE0000084)
#define UARTBAUD (*(volatile uint32_t*)0xE0000084)

// UART1
#define UART1DAT  (*(volatile uint8_t*)0xE0000090)
#define UART1STA  (*(volatile uint32_t*)0xE0000094)
#define UART1BAUD (*(volatile uint32_t*)0xE0000094)

// UART2
#define UART2DAT  (*(volatile uint8_t*)0xE00000A0)
#define UART2STA  (*(volatile uint32_t*)0xE00000A4)
#define UART2BAUD (*(volatile uint32_t*)0xE00000A4)

// SPI
#define SPIDAT	 (*(volatile uint32_t*)0xE0000070)
#define SPICTRL	 (*(volatile uint32_t*)0xE0000074)
#define SPISTA	 (*(volatile uint32_t*)0xE0000074)
#define SPISS	 (*(volatile uint32_t*)0xE0000078)

//SPI1
#define SPI1DAT	 (*(volatile uint32_t*)0xE0000060)
#define SPI1CTRL (*(volatile uint32_t*)0xE0000064)
#define SPI1STA	 (*(volatile uint32_t*)0xE0000064)
#define SPI1SS	 (*(volatile uint32_t*)0xE0000068)


// TIMER
#define MAX_COUNT (*(volatile uint32_t*)0xE0000040)
#define TIMER     (*(volatile uint32_t*)0xE0000040)


#define GPOUT  (*(volatile uint32_t*)0xE0000030)
#define GPIN   (*(volatile uint32_t*)0xE0000034)

#define LORA_RESET  (1U << 10)
#define L_RX        (1U << 9)
#define L_TX        (1U << 8)

#define IRQ_TIMER     (1U << 1)
#define IRQ_UART0_RX  (1U << 2)
#define IRQ_UART0_TX  (1U << 3)
#define IRQ_UART1_RX  (1U << 4)
#define IRQ_UART1_TX  (1U << 5)
#define IRQ_UART2_RX  (1U << 6)
#define IRQ_UART2_TX  (1U << 7)


//IRQs vectors
#define IRQEN	 (*(volatile uint32_t*)0xE0000020) //Enable
#define IRQVECT0 (*(volatile uint32_t*)0xE0000000) //Trap
#define IRQVECT1 (*(volatile uint32_t*)0xE0000004) //Timer
#define IRQVECT2 (*(volatile uint32_t*)0xE0000008) //UART0 RX
#define IRQVECT3 (*(volatile uint32_t*)0xE000000C) //UART0 TX
#define IRQVECT4 (*(volatile uint32_t*)0xE0000010) //UART1 RX
#define IRQVECT5 (*(volatile uint32_t*)0xE0000014) //UART1 TX
#define IRQVECT6 (*(volatile uint32_t*)0xE0000018) //UART2 RX
#define IRQVECT7 (*(volatile uint32_t*)0xE000001C) //UART2 TX



void delay_loop(uint32_t val);	// (3 + 3*val) cyclesç

#define CCLK (18000000u)
#define _delay_us(n) delay_loop((n*(CCLK/1000)-3000)/3000)
#define _delay_ms(n) delay_loop((n*(CCLK/1000)-30)/3)


void _putch(int c) { 
	while((UARTSTA &2)==0); 
	UARTDAT  = c; 
}


void _puts(const char *p)
{
    while (*p)
       _putch(*(p++)); 
}

/*
uint8_t _getch()
{
	while((UARTSTA&1)==0);
	return UARTDAT;
}

uint8_t haschar() {return UARTSTA&1;}
*/

//FIFOs
uint8_t udat0[32]; 				//FIFO UART0
volatile uint8_t rdix0, wrix0; 	//Read index and write index

uint8_t udat1[32]; 				//FIFO UART1
volatile uint8_t rdix1, wrix1; 	//Read index and write index

uint8_t udat2[32]; 				//FIFO UART2
volatile uint8_t rdix2, wrix2; 	//Read index and write index	

static uint8_t lora_rx_buffer[256];

//Read of UART0
uint8_t _getchUART0()
{
	uint8_t d;
	while(rdix0==wrix0);
	d=udat0[rdix0++];
	rdix0 &=31;
	return d;
}

//Read of UART1(moduloTEL0132)
uint8_t _getchUART1()
{
	uint8_t d;
	while(rdix1==wrix1);
	d=udat1[rdix1++];
	rdix1 &=31;
	return d;
}

//Read of UART2
uint8_t _getchUART2()
{
	uint8_t d;
	while(rdix2==wrix2);
	d=udat2[rdix2++];
	rdix2 &=31;
	return d;
}
 
void *memcpy(void *dst, const void *src, unsigned int n)
{
	unsigned char *d = (unsigned char *)dst;
	const unsigned char *s = (const unsigned char *)src;

	while (n--) {
		*d++ = *s++;
	}
	return dst;
}

extern int _printf(const char *format, ...);

static void debug_sensors(void)
{
	int temp_comp, press_comp, hum_comp;
	int adc0, adc1, adc2, adc3;
	int co_ppm, ch4_ppm;
	int dust_raw;

	startBME680();
	temp_comp  = returnTemp();
	press_comp = returnPressure();
	hum_comp   = returnHUMIDITY(temp_comp);

	adc0 = MCP3004_Read(MCP3004_CH0);
	adc1 = MCP3004_Read(MCP3004_CH1);
	adc2 = MCP3004_Read(MCP3004_CH2);
	adc3 = MCP3004_Read(MCP3004_CH3);

	co_ppm  = MQ9_ReadCOppm(MCP3004_CH0);
	ch4_ppm = MQ9_ReadCH4ppm(MCP3004_CH0);

	dust_raw = GP2Y1010_ReadRaw(MCP3004_CH1);

	_printf("\n------ DEBUG SENSORES ------\n");
	_printf("BME680 T=%02d.%02d C, P=%d Pa, H=%02d.%03d %%\n",
	        temp_comp/100, temp_comp%100,
	        press_comp,
	        hum_comp/1000, hum_comp%1000);
	_printf("ADC CH0=%d CH1=%d CH2=%d CH3=%d\n", adc0, adc1, adc2, adc3);
	_printf("MQ9 CO=%d ppm, CH4=%d ppm\n", co_ppm, ch4_ppm);
	_printf("GP2Y1010 raw=%d\n", dust_raw);
	_printf("----------------------------\n");
	readGPS();
}


#define putchar(d) _putch(d)
#include "printf.c"
#include "moduloMQ9.c"
#include "moduloBME680.c"
#include "moduloTEL0132.c"
#include "lora_sx1262.c"
//#include "SPI.c"
#include "moduloMCP3004.c"
#include "moduloGP2Y1010.c"


const static char *menu="\n" 
"------------------ 	MENU  	  -----------------\n" 
"--------------------------------------------------\n" 
"|                                                |\n" 
"|    Seleccione: s -> Valores de los sensores    |\n" 
"|    Seleccione: f -> Sensor de gases            |\n" 
"|    Seleccione: r -> Recibir LoRa               |\n" 
"|    Seleccione: t -> Transmitir LoRa            |\n" 
"|    Seleccione: g -> GPS                        |\n" 
"|    Seleccione: d -> Debug de sensores          |\n" 
"|    Seleccione: e -> Logo del equipo            |\n" 
"|    Seleccione: q -> Salir                      |\n" 
"|                                                |\n" 
"--------------------------------------------------\n"  
"\n\n";

const static char *menutxt="\n"
"\n\n"
"888                8888888b.  888     888\n"         
"888                888   Y88b 888     888\n"        
"888                888    888 888     888\n"       
"888        8888b.  888   d88P Y88b   d88P 8888b.\n"  
"888           '88b 8888888P'   Y88b d88P     '88b\n"
"888       .d888888 888 T88b     Y88o88P  .d888888\n" 
"888888888 888  888 888  T88b     Y888P   888  888\n"
"888888888 'Y888888 888   T88b     Y8P    'Y888888\n"
"\nIts Alive :-)\n"
"\n";             

const static char *logo="\n" 
"\n\n"
"       | | | |                       ( | )  \n"
"     +---------+                    (  |  ) \n"
"  ---|         |                       |   \n"
"  ---|  VIMMAC |-----------------------|   \n"
"  ---|         |                       |   \n"
"     +---------+                      / \\  \n"
"       | | | |                       /   \\ \n"
"											\n"
"\n"
"   V     V  III  M     M  M     M   AAAAA   CCCCC\n"
"   V     V   I   MM   MM  MM   MM  A     A  C\n"
"    V   V    I   M M M M  M M M M  AAAAAAA  C\n"
"     V V     I   M  M  M  M  M  M  A     A  C\n"
"      V     III  M     M  M     M  A     A  CCCCC\n"
"\n"
"            _____ _____ ____ _   _ \n"
"           |_   _| ____/ ___| | | |\n"
"             | | |  _|| |   | |_| |\n"
"             | | | |__| |___|  _  |\n"
"             |_| |_____\\____|_| |_|\n"
"									\n"
"          ELECTRONICS FOR TP ONE \n"
"\n\n"; 

// Cambios Clara y Miguel

uint8_t hascharUART0(){return wrix0 - rdix0;}
// uint8_t hascharUART0(){return (wrix0 - rdix0) & 31;} // Opción que soluciona el problema de overflow

uint8_t hascharUART1(){return wrix1 - rdix1;}
// uint8_t hascharUART1(){return (wrix1 - rdix1) & 31;} // Opción que soluciona el problema de overflow

uint8_t hascharUART2(){return wrix2 - rdix2;}
// uint8_t hascharUART2(){return (wrix2 - rdix2) & 31;} // Opción que soluciona el problema de overflow


uint32_t __attribute__((naked)) getMEPC()
{
	asm volatile(
	//"	csrrw	a0,0x341,zero	\n"
	//"	csrrw	zero,0x341,a0	\n"
	"	.word	0x34101573 		\n"
	"	.word   0x34151073		\n"
	"	ret						\n"
	);
}

//Interruptions
void __attribute__((interrupt ("machine"))) irq0_handler()
{
	_printf("\nTRAP at 0x%x\n",getMEPC());
}

//Timer
void __attribute__((interrupt ("machine"))) irq1_handler()
{
	IRQEN &= ~IRQ_TIMER;
	switch(state){
		case 0:
			readCO();
			break;
		case 1:
			readCH4();
			break;
		case 2:
			_printf("Medidas \n");
			break;
		default:
			break;
	}
	
}

//UART0 RX
void __attribute__((interrupt ("machine"))) irq2_handler()
{
    udat0[wrix0++] = UARTDAT;
    wrix0 &= 31;
}


//UART0 TX
void  __attribute__((interrupt ("machine"))) irq3_handler(){
	static uint8_t a=32;
	UARTDAT=a;
	if (++a>=128) a=32;
}

//UART1 RX
void __attribute__((interrupt ("machine"))) irq4_handler()
{
	readGPS();
    IRQEN=0b00000000;
}


//UART1 TX
void  __attribute__((interrupt ("machine"))) irq5_handler(){
	static uint8_t a=32;
	UART1DAT=a;
	if (++a>=128) a=32;
}


//UART2 RX
void __attribute__((interrupt ("machine"))) irq6_handler()
{
    udat2[wrix2++] = UART2DAT;
    wrix2 &= 31;
}


//UART2 TX
void  __attribute__((interrupt ("machine"))) irq7_handler(){
	static uint8_t a=32;
	UART2DAT=a;
	if (++a>=128) a=32;
}



// --------------------------------------------------------

uint8_t spixfer (uint8_t d)
{
	SPIDAT=d;
	while(SPISTA&1);
	return SPIDAT;
}
uint8_t spi1xfer (uint8_t d) 
{ 
SPI1DAT=d; 
while(SPI1STA&1); 
return SPI1DAT; 
}

// --------------------------------------------------------

#define BAUD 115200u
#define BAUD1 9600u
#define BAUD2 115200u
#define NULL ((void *)0)

// Get word from UART0
uint32_t getw()
{
	uint32_t i;
	i=_getchUART0();
	i|=_getchUART0()<<8;
	i|=_getchUART0()<<16;
	i|=_getchUART0()<<24;
	return i;
}

// Get word from UART1
uint32_t getw1()
{
	uint32_t i;
	i=_getchUART1();
	i|=_getchUART1()<<8;
	i|=_getchUART1()<<16;
	i|=_getchUART1()<<24;
	return i;
}

// Get word from UART2
uint32_t getw2()
{
	uint32_t i;
	i=_getchUART2();
	i|=_getchUART2()<<8;
	i|=_getchUART2()<<16;
	i|=_getchUART2()<<24;
	return i;
}


// Copy memory
uint8_t *_memcpy(uint8_t *pdst, uint8_t *psrc, uint32_t nb)
{
	if (nb) do {*pdst++=*psrc++; } while (--nb);
	return pdst;
}

//CHANGE CLARA MIGUEL
// Main function
void main()
{

	char c,buf[17];
	uint8_t *p;
	unsigned int i,j;
	int n;
	void (*pcode)();
	uint32_t *pi;
	uint16_t *ps;
	int temp_comp, press_comp, hum_comp;
/*
    UARTBAUD=(CCLK+BAUD/2)/BAUD -1;	
	c = UARTDAT;		// Clear RX garbage
	
	IRQVECT2=(uint32_t)irq2_handler;
    IRQEN = (1<<2);
*/	
	
	SPICTRL=(8<<8)|8; 					// SPI control register 0 
	SPI1CTRL=(8<<8)|8; 					// SPI control register 1 
	SPISS = 0b11;
	SPI1SS = 1;
	UARTBAUD=(CCLK+BAUD/2)/BAUD -1;	    // UART0 baud rate
	UART1BAUD = (CCLK+BAUD1/2)/BAUD1 -1;	// UART1 baud rate (GPS)
	UART2BAUD = (CCLK+BAUD2/2)/BAUD2 -1;	// UART2 baud rate

	//_delay_ms(100);
	
	c = UARTDAT;		// Clear RX garbage
	c = UART1DAT;		// Clear RX garbage
	c = UART2DAT;		// Clear RX garbage
	
	IRQVECT0=(uint32_t)irq0_handler;	
	IRQVECT1=(uint32_t)irq1_handler;
	IRQVECT2=(uint32_t)irq2_handler;
	IRQVECT3=(uint32_t)irq3_handler;
	IRQVECT4=(uint32_t)irq4_handler;
	IRQVECT5=(uint32_t)irq5_handler;
	IRQVECT6=(uint32_t)irq6_handler;
	IRQVECT7=(uint32_t)irq7_handler;

	// Habilitar IRQ de RX en UART0 (bit 2).
	IRQEN = IRQ_UART0_RX;

	// Activar TIMER para las funciones que lo usan como contador.
	MAX_COUNT = 0xFFFFFFFFu;

	asm volatile ("ecall");
	asm volatile ("ebreak");
	_puts(menutxt);
	_puts("Hola mundo\n");
	
	while (1)
	{
		_puts(menu);
		char cmd = _getchUART0();
	
		if (cmd > 32 && cmd < 127)
			_putch(cmd);
			_puts("\n");
	
		switch (cmd)
		{
			case '2':
				IRQEN ^= 3;   // Toggle IRQ enable for UART TX
				_delay_ms(100);
				break;
	
			// Transmisión del LoRa
			case 't':
			if(SX1262_Init()){
				//Tenemos los mismos valores que en la parte de SC
				_printf("Modulo LoRa iniciado\n");
				SX1262_configSetFrequency(868000000);
				SX1262_configSetBandwidth(4);      		// 125KHz
				SX1262_configSetSpreadingFactor(7); 	//SF7
				SX1262_transmit((uint8_t *)"Grupo 1", (int)(sizeof("Grupo 1") - 1));
			}
				else
				{
					_printf("Error al iniciar el LoRa\n");
				}
				break;
	
			// Recepción del LoRa
			case 'r':
				if (SX1262_Init())
				{
					_printf("Modulo LoRa iniciado\n");
					SX1262_configSetFrequency(868000000);
					SX1262_configSetBandwidth(4);      	  //125 kHz
					SX1262_configSetSpreadingFactor(7);  //SF7
					n = SX1262_lora_receive_async(lora_rx_buffer, sizeof(lora_rx_buffer));
					if (n > 0)
					{
						_printf("Paquete recibido (%d bytes): ", n);
						for (i = 0; i < (unsigned int)n; i++)
						{
							_putch(lora_rx_buffer[i]);
						}
						_puts("\n");
					}
					else if (n == 0)
					{
						_printf("Paquete vacio\n");
					}
					else
					{
						_printf("No hay paquete\n");
					}
				}
				else
				{
					_printf("Error al iniciar el LoRa\n");
				}
				break;
	
			// Sensores de humedad, temperatura y presión
			case 's':
				startBME680();
	
				temp_comp  = returnTemp();
				press_comp = returnPressure();
				hum_comp   = returnHUMIDITY(temp_comp);
	
				
				_printf("DATOS OBTENIDOS DE PRESION, TEMPERATURA Y HUMEDAD \n");
				_printf("----------------------------------------------\n");
				_printf("\n");
	
				_printf(" La temperatura es: ");
				_printf("%02d.%d%c", temp_comp / 100, temp_comp % 100, 167);
	
				_printf("\n La presion es: ");
				_printf("%d Pascales", press_comp / 100);
	
				_printf("\n La humedad es: ");
				_printf("%02d.%03d%c", hum_comp / 1000, hum_comp % 1000, 37);
	
				_printf("\n\n");
				_printf("----------------------------------------------\n");
				break;
	
			// GPS
			case 'g':
				readGPS();
				_getchUART0();
				break;

			// Debug completo
			case 'd':
				debug_sensors();
				break;
	
			// Logo
			case 'e':
				_puts(logo);
				break;
	
			
			case 'q':
				return;
	
			// Sensor de gases
			case 'f':
				readGas();
				break;
	
			default:
				continue;	
		}
	}
}
