#include <stdint.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef signed char  s8;
typedef signed short s16;
typedef signed int   s32;


//-- Registros mapeados
#define UARTDAT  (*(volatile uint8_t*)0xE0000080)
#define UARTSTA  (*(volatile uint32_t*)0xE0000084)
#define UARTBAUD (*(volatile uint32_t*)0xE0000084)

#define UART1DAT  (*(volatile uint8_t*)0xE0000090)
#define UART1STA  (*(volatile uint32_t*)0xE0000094)
#define UART1BAUD (*(volatile uint32_t*)0xE0000094)


#define UART2DAT  (*(volatile uint8_t*)0xE00000A0)
#define UART2STA  (*(volatile uint32_t*)0xE00000A4)
#define UART2BAUD (*(volatile uint32_t*)0xE00000A4)


#define SPIDAT	 (*(volatile uint32_t*)0xE0000070)
#define SPICTL	 (*(volatile uint32_t*)0xE0000074)
#define SPISTA	 (*(volatile uint32_t*)0xE0000074)
#define SPISS	 (*(volatile uint32_t*)0xE0000078)

#define SPI1DAT	 (*(volatile uint32_t*)0xE0000060)
#define SPI1CTL	 (*(volatile uint32_t*)0xE0000064)
#define SPI1STA	 (*(volatile uint32_t*)0xE0000064)
#define SPI1SS	 (*(volatile uint32_t*)0xE0000068)

#define TCNT     (*(volatile uint32_t*)0xE0000040) 

#define GPOUT  (*(volatile uint8_t*)0xE0000030) 
#define GPIN   (*(volatile uint8_t*)0xE0000034) 

#define IRQEN	 (*(volatile uint32_t*)0xE0000020)

#define IRQVECT0 (*(volatile uint32_t*)0xE0000000)
#define IRQVECT1 (*(volatile uint32_t*)0xE0000004)
#define IRQVECT2 (*(volatile uint32_t*)0xE0000008)
#define IRQVECT3 (*(volatile uint32_t*)0xE000000C)
#define IRQVECT4 (*(volatile uint32_t*)0xE0000010)
#define IRQVECT5 (*(volatile uint32_t*)0xE0000014)
#define IRQVECT6 (*(volatile uint32_t*)0xE0000018)
#define IRQVECT7 (*(volatile uint32_t*)0xE000001C)



void delay_loop(uint32_t val);	// (3 + 3*val) cyclesç

#define CCLK (18000000)
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


#define putchar(d) _putch(d)
#include "printf.c"

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

// Cambios Clara y Miguel

uint8_t udat0[32];
volatile uint8_t rdix0, wrix0;

uint8_t udat1[32];
volatile uint8_t rdix1, wrix1;

uint8_t udat2[32];
volatile uint8_t rdix2, wrix2;



uint8_t _getchUART()
{
	uint8_t d;
	while(rdix0==wrix0);
	d=udat0[rdix0++];
	rdix0 &=31;
	return d;
}

uint8_t _getchUART1()
{
    uint8_t d;
    while (rdix1 == wrix1);
    d = udat1[rdix1++];
    rdix1 &= 31;
    return d;
}

uint8_t _getchUART2()
{
	uint8_t d;
	while(rdix2==wrix2);
	d=udat2[rdix2++];
	rdix2 &=31;
	return d;
}
 
uint8_t hascharUART0(){return wrix0 - rdix0;}


uint8_t hascharUART1(){return wrix1 - rdix1;}

uint8_t hascharUART2(){return wrix2 - rdix2;}


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

//interrupciones
void __attribute__((interrupt ("machine"))) irq1_handler()
{
	_printf("\nTRAP at 0x%x\n",getMEPC());
}

//timer
void __attribute__((interrupt ("machine"))) irq2_handler()
{
	IRQEN = 0b00000000;
	
}

//uart rx
void __attribute__((interrupt ("machine"))) irq3_handler()
{
    udat0[wrix0++] = UARTDAT;
    wrix0 &= 31;
}


//uart tx
void  __attribute__((interrupt ("machine"))) irq4_handler(){
	static uint8_t a=32;
	UARTDAT=a;
	if (++a>=128) a=32;
}

//uart1 rx
void __attribute__((interrupt ("machine"))) irq5_handler()
{
    udat1[wrix1++] = UART1DAT;
    wrix1 &= 31;
}


//uart1 tx
void  __attribute__((interrupt ("machine"))) irq6_handler(){
	static uint8_t a=32;
	UART1DAT=a;
	if (++a>=128) a=32;
}


//uart2 rx
void __attribute__((interrupt ("machine"))) irq7_handler()
{
    udat2[wrix2++] = UART2DAT;
    wrix2 &= 31;
}


//uart2 tx
void  __attribute__((interrupt ("machine"))) irq8_handler(){
	static uint8_t a=32;
	UART2DAT=a;
	if (++a>=128) a=32;
}



// --------------------------------------------------------

uint32_t spixfer (uint32_t d)
{
	SPIDAT=d;
	while(SPISTA&1);
	return SPIDAT;
}

// --------------------------------------------------------

#define BAUD 115200
#define NULL ((void *)0)

uint32_t getw()
{
	uint32_t i;
	i=_getchUART();
	i|=_getchUART()<<8;
	i|=_getchUART()<<16;
	i|=_getchUART()<<24;
	return i;
}

uint32_t getw1()
{
	uint32_t i;
	i=_getchUART1();
	i|=_getchUART1()<<8;
	i|=_getchUART1()<<16;
	i|=_getchUART1()<<24;
	return i;
}

uint32_t getw2()
{
	uint32_t i;
	i=_getchUART2();
	i|=_getchUART2()<<8;
	i|=_getchUART2()<<16;
	i|=_getchUART2()<<24;
	return i;
}


uint8_t *_memcpy(uint8_t *pdst, uint8_t *psrc, uint32_t nb)
{
	if (nb) do {*pdst++=*psrc++; } while (--nb);
	return pdst;
}

//CHANGE CLARA MIGUEL
void main()
{

	char c,buf[17];
	uint8_t *p;
	unsigned int i,j;
	int n;
	void (*pcode)();
	uint32_t *pi;
	uint16_t *ps;
/*
    UARTBAUD=(CCLK+BAUD/2)/BAUD -1;	
	c = UARTDAT;		// Clear RX garbage
	
	IRQVECT2=(uint32_t)irq2_handler;
    IRQEN = (1<<2);
*/	
	
	UARTBAUD=(CCLK+BAUD/2)/BAUD -1;	
	UART1BAUD = (CCLK+BAUD/2)/BAUD -1;
	UART2BAUD = (CCLK+BAUD/2)/BAUD -1;

	//_delay_ms(100);
	
	c = UARTDAT;		// Clear RX garbage
	c = UART1DAT;		// Clear RX garbage
	c = UART2DAT;		// Clear RX garbage
	
	IRQVECT0=(uint32_t)irq1_handler;	
	IRQVECT1=(uint32_t)irq2_handler;
	IRQVECT2=(uint32_t)irq3_handler;
	IRQVECT3=(uint32_t)irq4_handler;
	IRQVECT4=(uint32_t)irq5_handler;
	IRQVECT5=(uint32_t)irq6_handler;
	IRQVECT6=(uint32_t)irq7_handler;
	IRQVECT7=(uint32_t)irq8_handler;
	IRQEN = (1<<2); 
	
	asm volatile ("ecall");
	asm volatile ("ebreak");
	_puts(menutxt);
	_puts("Hola mundo\n");
	
	while (1)
	{
			_puts("Command [123dx]> ");
			char cmd = _getchUART();
			if (cmd > 32 && cmd < 127)
				_putch(cmd);
			_puts("\n");

			switch (cmd)
			{
			case '1':
			    _puts(menutxt);
				break;
			case '2':
				IRQEN^=2;	// Toggle IRQ enable for UART TX
				_delay_ms(100);
				break;
			case 'x':
				_puts("Upload APP from serial port (<crtl>-F) and execute\n");
				if(getw()!=0x66567270) break;
				p=(uint8_t *)getw();
				n=getw();
				i=getw();
				if (n) {
					do { *p++=_getchUART(); } while(--n);
				}

				if (i>255) {
					pcode=(void (*)())i;
					pcode();
				} 
				break;
			case  'c':
				_puts("Upload APP from serial port (<crtl>-F) and execute\n");
				if(getw1()!=0x66567270) break;
				p=(uint8_t *)getw1();
				n=getw1();
				i=getw1();
				if (n) {
					do { *p++=_getchUART1(); } while(--n);
				}

				if (i>255) {
					pcode=(void (*)())i;
					pcode();
				} 
				break;
			
			case  'v':
				_puts("Upload APP from serial port (<crtl>-F) and execute\n");
				if(getw2()!=0x66567270) break;
				p=(uint8_t *)getw2();
				n=getw2();
				i=getw2();
				if (n) {
					do { *p++=_getchUART2(); } while(--n);
				}

				if (i>255) {
					pcode=(void (*)())i;
					pcode();
				} 
				break;
			
			case 'q':
				asm volatile ("jalr zero,zero");
			case 't':
				break;
			default:
				continue;
			}
	}
}
