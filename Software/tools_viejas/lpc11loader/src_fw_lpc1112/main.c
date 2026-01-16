//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
// Firmware para el micro LPC1112 de las placas FPGA ICECREAM
// (Testboard, RAM, Pin All Socketed ;)
//
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//  definimos tipos de datos de anchura de bits concretos

typedef unsigned char  u8;	// Sin signo
typedef unsigned short u16;
typedef unsigned int   u32;

typedef signed char  s8;	// Con signo
typedef signed short s16;
typedef signed int   s32;

////////////////////////////////////////////////////////////////////////////////////
// Cabeceras con definiciones
#include "lpc111x.h"	// del micro
#include "system.h"		// del sistema
#include "init.c"

///////////////////////////////////////////////////////////////
//						Retardo Activo
// Ojo, cuando se ejecuta desde Flash el alineamiento influye
// mucho en la velocidad de ejecución del bucle
void delay_loop(unsigned int) __attribute__ (( naked, aligned(8) ));
void delay_loop(unsigned int d)
{
	asm volatile ("Ldelay_loop: sub r0,#1");
	asm volatile ("  bne Ldelay_loop");
	asm volatile ("  bx lr");
}

// UART putch & getch

void U0putch(int d)
{
	while(!(U0LSR&0x20));
	U0THR=d;
}

u32 U0getch()
{
	while(!(U0LSR&1));
	return U0RBR;
}

//#define putchar(d) U0putch(d)
//#include "printf.c"

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
//								BITBANGING  SPI
//	- Necesario, al menos cuando se quiere configurar la FPGA directamente
//  - Versión en ensamblador para predictibilidad y máxima velocidad
//	- Ejecución desde la RAM interna para evitar los retardos de la FLASH del micro
//    (Las rutinas críticas están en la sección ".data")
//  - Nota: la UART como mucho transfiere datos a 3Mbit/s y emplea 10 bits/byte
//    lo que supone 3.33 us entre datos. El bus SPI debe ser más rápido.
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// FLASH como esclavo SPI
// (Dout por PIO0.8, Din por PIO0.9, SCK por PIO0.6)
/*u32 SPIbyte(u32 dato)
{
	u32 i;

	dato<<=1; 
	i=8; do {
		GPIO0MASK[(1<<6)|(1<<8)]=dato&0x100;	// ICE_SO=bit MSB, SCK=0;
		dato<<=1;
		GPIO0MASK[(1<<6)]=(1<<6);				// SCK =1
		if (GPIO0MASK[(1<<9)]) dato++;			// ICE_SI -> resultado
	}while (--i);
	GPIO0MASK[(1<<6)]=0;	// terminamos con reloj en bajo
	return dato&0xff;
}
*/

/*
// Versión rápida de sólo salida (~4Mbit/s)
void SPIout(u32 dato)
{
	u32 i;

	dato<<=1; 
	i=8; do {
		GPIO0MASK[(1<<6)|(1<<8)]=dato&0x100;	// ICE_SO=bit MSB, SCK=0;
		dato<<=1;
		GPIO0MASK[(1<<6)]=(1<<6);				// SCK =1
	}while (--i);
	GPIO0MASK[(1<<6)]=0;	// terminamos con reloj en bajo
}
*/

////////////////////////////////////////////////////////////////////////////////////
// Salida SPI hacia Flash serie (Dout por PIO0.8, SCK por PIO0.6)
// 7 ciclos/bit => 6.86 Mbit/s (1.42us/byte)

void __attribute__((naked,section(".data"))) SPIout(u32 dato)
{
asm volatile(
"	ldr	r1,=GPIO0MASK+((1<<6)|(1<<8))*4		\n"
"	mov	r2,#0x40		\n"

"	lsl	r3,r0,#1		\n"	// Bit 7
"	bic	r3,r2			\n"	// SCK=L
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"	// SCK=H
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#2		\n"	// Bit 7
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#3		\n"	// Bit 5
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#4		\n"	// Bit 4
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#5		\n"	// Bit 3
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#6		\n"	// Bit 2
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#7		\n"	// Bit 1
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#8		\n"	// Bit 0
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	mov	r3,#0			\n"	// SCK en bajo
"	str	r3,[r1]			\n"

"	bx	lr				\n"
"	.ltorg				\n"
);
}

/*
u32 SPIin()
{
	u32 i,dato=0;

	i=8; do {
		GPIO0MASK[(1<<6)]=0;	// SCK=0;
		dato<<=1;
		GPIO0MASK[(1<<6)]=-1;	// SCK =1
		if (GPIO0MASK[(1<<9)]) dato++;			// ICE_SI -> resultado
	}while (--i);
	GPIO0MASK[(1<<6)]=0;	// terminamos con reloj en bajo
	return dato;
}
*/

////////////////////////////////////////////////////////////////////////////////////
// Entrada SPI desde Flash serie (Din por PIO0.9, SCK por PIO0.6)
// 8 ciclos/bit => 6 Mbit/s (1.79us/byte)

u32 __attribute__((naked,section(".data"))) SPIin()
{
asm volatile(
"	push {r4,r5,lr}					\n"
"	ldr	r1,=GPIO0MASK+(1<<6)*4		\n"
"	ldr	r2,=GPIO0MASK+(1<<9)*4		\n"
"	mov	r3,#0						\n"

"	str	r3,[r1]						\n"	// SCK L
"	mov	r0,#0						\n"
"	mvn	r4,r3						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D7

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D6

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D5

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D4

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D3

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D2

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D1

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D0

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	pop {r4,r5,pc}					\n"
"	.ltorg							\n"

);
}

////////////////////////////////////////////////////////////////////////////////////
// Entrada SPI desde Flash serie (Din por PIO0.8, SCK por PIO0.6)
// 8 ciclos/bit => 6 Mbit/s (1.79us/byte)

u32 __attribute__((naked,section(".data"))) SPIRin()
{
asm volatile(
"	push {r4,r5,lr}					\n"
"	ldr	r1,=GPIO0MASK+(1<<6)*4		\n"
"	ldr	r2,=GPIO0MASK+(1<<8)*4		\n"
"	mov	r3,#0						\n"

"	str	r3,[r1]						\n"	// SCK L
"	mov	r0,#0						\n"
"	mvn	r4,r3						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D7

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D6

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D5

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D4

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D3

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D2

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D1

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	str	r4,[r1]						\n" // SCK H
"	ldr	r5,[r2]						\n"	// Muestreo D0

"	str	r3,[r1]						\n"	// SCK L
"	add	r5,r4						\n" // Acarreo si r5!=0
"	adc	r0,r0						\n"
"	pop {r4,r5,pc}					\n"
"	.ltorg							\n"

);
}


///////////////////////////////////////////////////////////////
// FPGA como esclavo SPI
// La Flash debe estar deseleccionada
// (Dout por PIO0.9, SCK por PIO0.6)
// No se necesitan leer datos de entrada

/*void  SPIRout(u32 dato)
{
//	u32 i;

	dato<<=2; 
	i=8; do {
		GPIO0MASK[(1<<6)|(1<<9)]=dato&0x200;	// ICE_SI=bit MSB, SCK=0;
		dato<<=1; GPIO0MASK[(1<<6)]=-1;				// SCK =1
	}while (--i);
}
*/

////////////////////////////////////////////////////////////////////////////////////
// Salida SPI hacia FPGA (Dout por PIO0.9, SCK por PIO0.6)
// 7 ciclos/bit => 6.86 Mbit/s (1.35 us/byte)

void __attribute__((naked,section(".data"))) SPIRout(u32 dato)
{
asm volatile(
"	ldr	r1,=GPIO0MASK+((1<<6)|(1<<9))*4		\n"
"	mov	r2,#0x40		\n"

"	lsl	r3,r0,#2		\n"	// Bit 7
"	bic	r3,r2			\n"	// SCK=L
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"	// SCK=H
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#3		\n"	// Bit 6
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#4		\n"	// Bit 5
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#5		\n"	// Bit 4
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#6		\n"	// Bit 3
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#7		\n"	// Bit 2
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#8		\n"	// Bit 1
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"
"	str	r3,[r1]			\n"

"	lsl	r3,r0,#9		\n"	// Bit 0
"	bic	r3,r2			\n"
"	str	r3,[r1]			\n"
"	orr	r3,r2			\n"	// SCK queda en alto al final
"	str	r3,[r1]			\n"

"	bx	lr				\n"
"	.ltorg				\n"

);
}

// nada para paca ICECREAM
// -DPRGICE para placa PRGICE
// -DMASTER para placa MASTER TELECO (MISO y MOSI intercambiados)

#ifndef MASTER
  #define spi_out SPIout
  #define spi_in  SPIin
  #define spi_rout SPIRout
#else
  #define spi_out SPIRout
  #define spi_in  SPIRin
  #define spi_rout SPIout
#endif



//////////////////////////////////////////////////////////////////////////////////////////
// rutinas de soporte a la programación de la FLASH
//////////////////////////////////////////////////////////////////////////////////////////

////////////////////// Comando Write Enable ///////////////////
// Pone a 1 el flag WE, permitiendo la ejecución del siguiente
// comando de Borrado o Programación

void writeenable()
{
	SS_L();
	spi_out(0x06);
	SS_H();
}

////////////////////// Espera final de escritura ///////////////////
// Se comprueba el flag 'Write in progress' hasta que se hace cero

void waitready() 
{
	u32 j;
	do {
		SS_L(); 
		spi_out(0x05);	// Read status
		j=spi_in();   // Status
		SS_H(); 
		//_printf("st=0x%02x\n",j);
	} while (j&1);
	
}

//////////////////////////////////////////////
/////////////// Interrupciones ///////////////
//////////////////////////////////////////////

void SysTick(void)
{}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//// 								MAIN 
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////



int __attribute__ ((noreturn)) main()
{
	u32 i,j,dir,t,t2;

	//IOCON_R_PIO1_0=1|(1<<7);	// GPIO, Digital
	//IOCON_R_PIO1_1=1|(1<<7);	// GPIO, Digital
	//GPIO1DIR=0;

	IOCON_PIO0_4= 1<<8;	// Standar IO
	IOCON_PIO0_5= 1<<8;	// Standar IO
	IOCON_SWCLK_PIO0_10= 1 | (2<<3);	// GPIO, pull-up
	IOCON_R_PIO0_11=     1 | (2<<3) | (1<<7);	// GPIO, pull-up, digital
#ifndef MASTER
	GPIO0DIR=0x174;	// Pines 2, 4, 5, 6, y 8 como salidas (programación Flash)
#else
	GPIO0DIR=0x274;	// Pines 2, 4, 5, 6, y 9 como salidas (configuración FPGA)
#endif
	GPIO0MASK[0x374]=0x34;	// P02, P04, y P05 en alto

	IOCON_R_PIO1_0= IOCON_R_PIO1_1=1 |(2<<3) | (1<<7);	// GPIO, pull-up, digital
	GPIO1DIR=0x1;	// Pin P1.0 como salida (Reset)
	GPIO1MASK[1]=0;	// RESET en bajo

#ifndef PRGICE
	// CLKOUT
	CLKOUTCLKSEL=3;
	CLKOUTUEN=1;
	CLKOUTCLKDIV=4;		// CLKOUT de 12MHz
	IOCON_PIO0_1=1;		// Función CLKOUT
#endif

	// Timer 32B0: free run (cuenta en microsegundos)
	TMR32B0TCR=3;
	TMR32B0PR=PCLK/1000000-1;
	TMR32B0TCR=1;

	// SYSTIMER (para parpadeo LED)
	SYST_RVR=PCLK/4-1;	// Reload value for 250ms interval
	SYST_CVR=0;			// Clear current value
	SYST_CSR=7;			// Enable interrupt, enable timer, CLK=processor clock

	LED2_ON();

	// Autobaud	
	U0ACR=1;
	t=TMR32B0TC;

	do {	// Espera dato, si en 1 segundo no llega ejecutamos lo que haya en flash
		if (U0LSR&1) {
			i=U0RBR;
			if (i=='A' || i=='a') goto enterprog;
			else U0ACR=1;
		}
	} while ((TMR32B0TC-t)<1000000);

	// Timeout
	U0THR='T'; U0THR='O'; U0THR='\n';
fpgarun:
	SS_H();
	GPIO0DIR=(1<<4)|(1<<5);	// Solo los LEDs como salidas
	_delay_ms(10);
	IOCON_PIO1_7=(2<<3); 	// TXD como gpio, con pull-up
	ICERES_H();
ledrun:
	LED1_OFF();
	ISER=1<<15;			// Interrupción systimer
	while(1) {
		asm volatile("WFI");
		LED2_OFF();
		asm volatile("WFI");
		LED2_ON();
	}
enterprog:
	LED1_ON(); 
	U0putch(i);	// eco del caracter de autobaud

	// Bucle comandos

	while (1) {
		i=U0getch();
		switch(i) {
		case 'Q':	// Pone flash en modo SQI y carga imagen en FPGA
			writeenable();
			SS_L();
			spi_out(0x81);	// Write Volatile Conf Reg
			spi_out(0x83);	// 8-dummy, no-XIP, no-wrap
			SS_H();

			writeenable();
			SS_L();
			spi_out(0x61);	// Write Enhanced Volatile Conf Reg
			spi_out(0x4E);	// Quad, no-dual, no-hold,no-Vpp-acc, 15ohm
			SS_H();

		case 'C':	// Carga memoria configuración en FPGA y ejecuta		
			i =U0getch()<<16;	// Nº de bytes de la imagen
			i|=U0getch()<<8;	
			i|=U0getch();
#ifndef MASTER
			GPIO0DIR=0x274;	// Pines 2, 4, 5, 6, y 9 como salidas (configuración FPGA)
#else
			GPIO0DIR=0x174;	// Pines 2, 4, 5, 6, y 8 como salidas (configuración FPGA)
#endif
			GPIO0MASK[(1<<6)]=-1;	// SCK en alto de partida
			SS_L();
			_delay_us(200);
			ICERES_H();
			_delay_us(1200);
			SS_H();
			spi_rout(0xff);	// 8 pulsos SCK (dummy)
			U0putch(1);	// Pedimos datos al PC

			do {
				spi_rout(U0getch());
			}while(--i);

			// Esperamos CDONE=H
			_delay_us(1000);
			for (i=100;i;i--) {
				if (ICEDONE()) break;
				GPIO0MASK[(1<<6)]=0;	// Pulso en CLK
				GPIO0MASK[(1<<6)]=-1;
			}
			U0putch(i ? 0 : 2);
			while ((U0LSR&(1<<6))==0);	// Espera que termine la UART
			IOCON_PIO1_7=(2<<3); 	// TXD como gpio, con pull-up
			// 49 pulsos de reloj adicionales
			for (i=0;i<49;i++) {
				GPIO0MASK[(1<<6)]=0;	// Pulso en CLK
				GPIO0MASK[(1<<6)]=-1;
			}
			GPIO0DIR=(1<<4)|(1<<5);	// Solo los LEDs como salidas
			goto ledrun;

		case 'X':	// eXecute FPGA code from Flash
			U0THR='X';
			goto fpgarun;

		case 'I':	// Read Flash ID
			t=TMR32B0TC;
			SS_L();
			spi_out(0x9F);	// READ ID
			U0THR=spi_in();
			U0THR=spi_in();
			U0THR=spi_in();
			SS_H();
			t=TMR32B0TC-t; t2=0;
			break;
		case 'T':	// Retorna tiempos último comando (en us)
			U0THR=t>>24; // Big endian
			U0THR=t>>16;
			U0THR=t>>8;
			U0THR=t;
			U0THR=t2>>24; // Big endian
			U0THR=t2>>16;
			U0THR=t2>>8;
			U0THR=t2;
			break;
		case 'B':	// Bulk erase
			t=TMR32B0TC;
			writeenable();
			SS_L();
			spi_out(0xC7);	// Bulk erase
			SS_H(); 
			waitready();
			t=TMR32B0TC-t;
			t2=0;
			U0putch(0);	// OK
			break;
		case 'E':	// sector Erase
			i=U0getch();	// nº del sector
			t=TMR32B0TC;
			writeenable();
			SS_L(); 
			spi_out(0xD8);	// Sector erase
			spi_out(i);
			spi_out(0);
			spi_out(0);
			SS_H(); 
			waitready();
			t=TMR32B0TC-t;
			t2=0;
			U0putch(0);	// OK
			break;
		case 'R':	// READ
			dir =U0getch()<<16;	// Dirección de lectura
			dir|=U0getch()<<8;
			dir|=U0getch();
			i =U0getch()<<16;	// Nº de bytes
			i|=U0getch()<<8;
			i|=U0getch();
			if (!i) i=0x01000000;	// 16MBytes
			t=TMR32B0TC;
			SS_L();
			spi_out(0x03);		// Read 
			spi_out(dir>>16);	// Dir MSB
			spi_out(dir>>8); 	// 
			spi_out(dir); 		// DIR LSB 
			do {
				j=i; if (j>256) j=256;
				i-=j;
				for (;j;j--) U0putch(spi_in()); // Read
				U0getch(); // control de flujo
			}while (i);

			SS_H();	
			t=TMR32B0TC-t;
			t2=0;
			break;
		case 'P':	// Program Page
			dir =U0getch()<<16;
			dir|=U0getch()<<8;
			t=TMR32B0TC;
			writeenable();

			SS_L();
			spi_out(0x02);	 // Page program
			spi_out(dir>>16); 
			spi_out(dir>>8); 
			spi_out(dir); 
			i=256;	do {
				spi_out(U0getch());
			} while(--i);
			SS_H();
			i=TMR32B0TC;
			t=i-t;
			waitready();
			t2=TMR32B0TC-i;
			U0putch(0);		// OK
			break;
		case 'V':	// Firm. Version
			U0putch(VERSION);	
			U0putch(SUBVERSION);	
			break;

		case 'F':	// Divisor CLK
			i=U0getch();
#ifndef PRGICE
			CLKOUTCLKDIV=i;
			U0putch(0);
#else
			U0putch(0xff);	//error
#endif
			break;
		}
	}

}

