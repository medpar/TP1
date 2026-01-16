///////// Definir para evital la lectura del código del chip ////////
//#define CODEPROT
///////////////////// TIPO de PCB ///////////////////

////////////////////////////////////////////////////////////////////////////////////
// Retardos activos y E/S básica
#define _delay_us(n) delay_loop((n*(PCLK/1000)-6000)/4000)
#define _delay_ms(n) delay_loop((n*(PCLK/1000)-6)/4)
void delay_loop(unsigned int);

/////////////////////////////////////////////////////
#ifndef PGRICE
	#define XTAL_OSC		// External clock reference
#endif
#define PCLK 48000000	// Processor Clock

// UART
#define BAUD	115200
//#define BAUD	1500000

#define SS_L() 		{GPIO0MASK[1<<2]=0;}
#define SS_H() 		{GPIO0MASK[1<<2]=-1;}
#define LED1_ON() 	{GPIO0MASK[1<<4]=0;}
#define LED1_OFF() 	{GPIO0MASK[1<<4]=-1;}
#define LED2_ON() 	{GPIO0MASK[1<<5]=0;}
#define LED2_OFF() 	{GPIO0MASK[1<<5]=-1;}
#define ICERES_L()	{GPIO1MASK[1<<0]=0;}
#define ICERES_H()	{GPIO1MASK[1<<0]=-1;}

#define ICEDONE()	(GPIO1MASK[1<<1])

