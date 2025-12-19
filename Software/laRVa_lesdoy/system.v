//--------------------------------------------------------------------
// RISC-V things 
// by Jesús Arias (2022)

// Etiqueta: changed_VictorAmalia_FIFO
// Modificado por Víctor Ramos y Amalia Gil (11/2023)
//--------------------------------------------------------------------
/*
	Description:
	A LaRVA RISC-V system with 8KB of internal memory, and one UART
	
	Memory map:
	
	0x00000000 to 0x00001FFF	Internal RAM (with inital contents)
	0x00002000 to 0x1FFFFFFF	the same internal RAM repeated each 8KB
	0x20000000 to 0xDFFFFFFF       xxxx
	0xE0000000 to 0xE00000FF    IO registers
	0xE0000100 to 0xFFFFFFFF    the same IO registers repeated each 256B

	IO register map (all registers accessed as 32-bit words):
	
      address  |      WRITE        |      READ
    -----------|-------------------|---------------
    0xE0000000 | UART TX data      |  UART RX data
    0xE0000004 | UART Baud Divider |  UART flags
	           |                   |
    0xE0000020 | UART TX FIFO	   |  UART RX FIFO		//changed_VictorAmalia_FIFO
    0xE0000024 | UART FIFO divider |  UART FIFO flags	//changed_VictorAmalia_FIFO
		       |                   |
    0xE0000060 |                   |  Cycle counter
	           |                   |
    0xE00000E0 | Interrupt Enable  |  Interrupt enable
    0xE00000E4 | IRQ vector 3 RX   |  					//changed_VictorAmalia_FIFO
    0xE00000E8 | IRQ vector 4 TX   |  					//changed_VictorAmalia_FIFO
    0xE00000F0 | IRQ vector 0 Trap |
    0xE00000F4 | IRQ vector 1 RX   |
    0xE00000F8 | IRQ vector 2 TX   |

    UART Baud Divider: Baud = Fcclk / (DIVIDER+1) , with DIVIDER >=7
    
    UART FLAGS:    bits 31-5  bit 4  bit 3 bit 2 bit 1 bit 0
                     xxxx      OVE    FE    TEND  THRE   DV
        DV:   Data Valid (RX complete if 1. Cleared reading data register)
        THRE: TX Holding register empty (ready to write to data register if 1)
        TEND: TX end (holding reg and shift reg both empty if 1)
        FE:   Frame Error (Stop bit received as 0 if FE=1)
        OVE:  Overrun Error (Character received when DV was still 1)
        (DV and THRE assert interrupt channels #4 and #5 when 1)

    Interrupt enable: Bits 1-0
        bit 0: Enable UART RX interrupt if 1
        bit 1: Enable UART TX interrupt if 1
         
*/

`include "laRVa.v"
`include "uart.v"
`include "uart1.v" 		//changed_VictorAmalia_FIFO
//`define SIMULATION 	//changed_VictorAmalia_FIFO

module SYSTEM (
	input clk,		// Main clock input 25MHz
	input reset,	// Global reset (active high)

	input	rxd,	// UART
	output 	txd,	
	
	input	rxd1,	// UART1	//changed_VictorAmalia_FIFO
	output 	txd1,				//changed_VictorAmalia_FIFO
	
	output sck,		// SPI
	output mosi,
	input  miso,	
	output fssb	// Flash CS

);

wire		cclk;	// CPU clock
assign	cclk=clk;

///////////////////////////////////////////////////////
////////////////////////// CPU ////////////////////////
///////////////////////////////////////////////////////

wire [31:0]	ca;		// CPU Address
wire [31:0]	cdo;	// CPU Data Output
wire [3:0]	mwe;	// Memory Write Enable (4 signals, one per byte lane)
wire irq;
wire [31:2]ivector;	// Where to jump on IRQ
wire trap;			// Trap irq (to IRQ vector generator)

laRVa cpu (
		.clk     (cclk ),
		.reset   (reset),
		.addr    (ca[31:2] ),
		.wdata   (cdo  ),
		.wstrb   (mwe  ),
		.rdata   (cdi  ),
		.irq     (irq  ),
		.ivector (ivector),
		.trap    (trap)
	);

 
///////////////////////////////////////////////////////
///// Memory mapping
wire iramcs;
wire iocs;
// Internal RAM selected in lower 512MB (0-0x1FFFFFFF)
assign iramcs = (ca[31:29]==3'b000);
// IO selected in last 512MB (0xE0000000-0xFFFFFFFF)
assign iocs   = (ca[31:29]==3'b111);

// Input bus mux
reg [31:0]cdi;	// Not a register
always@*
 casex ({iocs,iramcs})
        2'b01: cdi<=mdo; 
        2'b10: cdi<=iodo;
        default: cdi<=32'hxxxxxxxx;
 endcase

///////////////////////////////////////////////////////
//////////////////// internal memory //////////////////
///////////////////////////////////////////////////////
wire [31:0]	mdo;	// Output data
ram32	 ram0 ( .clk(~cclk), .re(iramcs), .wrlanes(iramcs?mwe:4'b0000),
			.addr(ca[12:2]), .data_read(mdo), .data_write(cdo) );

//////////////////////////////////////////////////
////////////////// Peripherals ///////////////////
//////////////////////////////////////////////////
reg [31:0]tcount=0;
always @(posedge clk) tcount<=tcount+1;

wire uartcs;	// UART  at offset 0x00
wire uart1cs;	// UART1 at offset 0x00 	//changed_VictorAmalia_FIFO
wire spics;		// SPI	at offset 0x20
wire irqcs;		// IRQEN at offset 0xE0
				//		 ...
				// other at offset 0xE0
assign uartcs = iocs&(ca[7:5]==3'b000);
assign uart1cs = iocs&(ca[7:5]==3'b001);	//changed_VictorAmalia_FIFO
//assign spics  = iocs&(ca[7:5]==3'b001);
assign irqcs  = iocs&(ca[7:5]==3'b111);		//changed_VictorAmalia_FIFO

// Peripheral output bus mux
reg [31:0]iodo;	// Not a register
always@*
 casex (ca[7:2])
	6'b000xx0: iodo<={24'hx,uart_do}; 
	6'b000xx1: iodo<={27'hx,ove,fe,tend,thre,dv};
	6'b001xx0: iodo<={24'hx,uart1_do}; 						//changed_VictorAmalia_FIFO
	6'b001xx1: iodo<={27'hx,ove1,fe1,tend1,full,dv1,fpe};	//changed_VictorAmalia_FIFO
	6'b011xxx: iodo<=tcount;
	6'b111xxx: iodo<={30'hx,irqen};
	default: iodo<=32'hxxxxxxxx;
 endcase

/////////////////////////////
// UART

wire tend,thre,dv,fe,ove; // Flags
wire [7:0] uart_do;	// RX output data
wire uwrtx;			// UART TX write
wire urd;			// UART RX read (for flag clearing)
wire uwrbaud;		// UART BGR write
// Register mapping
// Offset 0: write: TX Holding reg
// Offset 0: read strobe: Clear DV, OVE (also reads RX data buffer)
// Offset 1: write: BAUD divider
assign uwrtx   = uartcs & (~ca[2]) & mwe[0];
assign uwrbaud = uartcs & ( ca[2]) & mwe[0] & mwe[1];
assign urd     = uartcs & (~ca[2]) & (mwe==4'b0000); // Clear DV, OVE flgas

UART_CORE #(.BAUDBITS(12)) uart0 ( .clk(cclk), .txd(txd), .rxd(rxd), 
	.d(cdo[15:0]), .wrtx(uwrtx), .wrbaud(uwrbaud),. rd(urd), .q(uart_do),
	.dv(dv), .fe(fe), .ove(ove), .tend(tend), .thre(thre) );

//////////////////////////////////////////

/////////////////////////////
// UART1 changed_VictorAmalia_FIFO

wire fpe,full,dv1,fe1,tend1,ove1; // Flags (fpe = empty)
wire [7:0] uart1_do;	// RX output data
wire u1wrtx;			// UART TX write
wire u1rd;				// UART RX read (for flag clearing)
wire u1wrbaud;			// UART BGR write

assign u1wrtx   = uart1cs & (~ca[2]) & mwe[0];	// Escribir en la UART
assign u1wrbaud = uart1cs & ( ca[2]) & mwe[0] & mwe[1];	// Escribir en el generador de baudios
assign u1rd     = uart1cs & (~ca[2]) & (mwe==4'b0000); // Clear DV, OVE flags

// Instancia de la uart1
UART_FIFO #(.BAUDBITS(12)) uart1 ( .txd(txd1), .tend(tend1), .d(cdo[15:0]), .wrtx(u1wrtx), .wrbaud(u1wrbaud), .q(uart1_do),
				.dv(dv1), .full(full), .empty(fpe), .fe(fe1), .ove(ove1), .rxd(rxd1), 
				.rd(u1rd), .reset(reset), .clk(clk));
	
//end_changed_VictorAmalia_FIFO

//////////////////////////////////////////

//    Interrupt control

// IRQ enable reg
//changed_VictorAmalia_FIFO
reg [3:0]irqen=0;
always @(posedge cclk or posedge reset) begin
	if (reset) 
		irqen<=0; 		// Si el reset se activa se deshabilitan las interrupciones
	else if (irqcs & ((ca[4:2]==3'b000) &mwe[0])) 
		irqen<=cdo[3:0];	// Habilitación de interrupciones
end

// IRQ vectors
reg [31:2]irqvect[0:4];
always @(posedge cclk) 
	if ((irqcs & ~(ca[4:2]==3'b000)) & (mwe==4'b1111)) 
		if (ca[4:2] == 001)			// DV
			irqvect[ca[4:2]]<=cdo[31:2];
		else if (ca[4:2] == 010)	// FULL
			irqvect[ca[4:2]]<=cdo[31:2];

// Enabled IRQs
wire [3:0]irqpen={irqen[3]&full, irqen[2]&dv1, irqen[1]&thre, irqen[0]&dv};	// pending IRQs

// Priority encoder
wire [2:0]vecn = trap      ? 3'b000 : (	// ECALL, EBREAK: highest priority
				 irqpen[0] ? 3'b001 : (	// UART RX
				 irqpen[1] ? 3'b010 : 	// UART TX
				 // DV y FULL tienen la más baja prioridad
				 irqpen[2] ? 3'b011 : 	// UART1 RX DV1 (pero DV es más prioritaria que FULL)
				 irqpen[3] ? 3'b100 : 	// UART1 TX FULL
				 			 3'bxxx ));	
assign ivector = irqvect[vecn];
assign irq = (irqpen!=0)|trap;

endmodule	// System

//end_changed_VictorAmalia_FIFO


//////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------
//-- 32-bit RAM Memory with independent byte-write lanes
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

module ram32
 (	input	clk,
	input	re,
	input	[3:0]	wrlanes,
	input	[10:0]	addr,
	output	[31:0]	data_read,
	input	[31:0] 	data_write
 );

reg [31:0] ram_array [0:2047];
reg [31:0] data_out;
        
assign data_read = data_out;
        
always @(posedge clk) begin
	if (wrlanes[0]) ram_array[addr][ 7: 0] <= data_write[ 7: 0];
	if (wrlanes[1]) ram_array[addr][15: 8] <= data_write[15: 8];
	if (wrlanes[2]) ram_array[addr][23:16] <= data_write[23:16];
	if (wrlanes[3]) ram_array[addr][31:24] <= data_write[31:24];
end

always @(posedge clk) begin
	if (re) data_out <= ram_array[addr];
end

initial begin
`ifdef SIMULATION
	$readmemh("rom.hex", ram_array);
`else
	$readmemh("rand.hex", ram_array);
`endif
end

endmodule

