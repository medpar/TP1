//--------------------------------------------------------------------
// RISC-V things 
// by Jesús Arias (2022)
//--------------------------------------------------------------------
/*
	Description:
    A LaRVA RISC-V system with 16KB of internal memory

Memory map:
    -----------
    0x00000000 to 0x00003FFF      Internal RAM (with initial contents)
    0x00004000 to 0x1FFFFFFF      The same internal RAM repeated each 16KB
    0x20000000 to 0xDFFFFFFF      xxxx
    0xE0000000 to 0xE00000FF      IO registers
    0xE0000100 to 0xFFFFFFFF      The same IO registers repeated each 256B

IO register map (all registers accessed as 32-bit words):

      address      |      WRITE        |      READ
    ------------- | ------------------ | ---------------
    0xE0000080    | UART0 TX data      | UART0 RX data
    0xE0000084    | UART0 Baud Divider | UART0 flags

    0xE0000090    | UART1 TX data      | UART1 RX data
    0xE0000094    | UART1 Baud Divider | UART1 flags

    0xE00000A0    | UART2 TX data      | UART2 RX data
    0xE00000A4    | UART2 Baud Divider | UART2 flags

    0xE0000070    | SPI0 TX data       | SPI0 RX data
    0xE0000074    | SPI0 Control       | SPI0 flags
    0xE0000078    | SPI0 Slave Select  | xxxx

    0xE0000060    | SPI1 TX data       | SPI1 RX data
    0xE0000064    | SPI1 Control       | SPI1 flags
    0xE0000068    | SPI1 Slave Select  | xxxx

    0xE0000040    | MAX_COUNT          | TIMER
    0xE0000030    | GPOUT              | GPOUT
    0xE0000034    | GPOUT              | GPIN
    0xE0000020    | Interrupt Enable   | Interrupt enable

    0xE0000000    | IRQ vector 0 Trap  |
    0xE0000004    | IRQ vector 1 Timer |
    0xE0000008    | IRQ vector 2 RX0   |
    0xE000000C    | IRQ vector 3 TX0   |
    0xE0000010    | IRQ vector 4 RX1   |
    0xE0000014    | IRQ vector 5 TX1   |
    0xE0000018    | IRQ vector 6 RX2   |
    0xE000001C    | IRQ vector 7 TX2   |

    ------
    UART Baud Divider:
        Baud = Fclk / (DIVIDER + 1), with DIVIDER >= 7

    UART FLAGS:
        bits 31-5   bit 4   bit 3   bit 2   bit 1   bit 0
                   xxxx    OVE     FE      TEND    THRE    DV

        DV:   Data Valid (RX complete if 1. Cleared reading data register)
        THRE: TX Holding register empty (ready to write to data register if 1)
        TEND: TX end (holding reg and shift reg both empty if 1)
        FE:   Frame Error (Stop bit received as 0 if FE=1)
        OVE:  Overrun Error (Character received when DV was still 1)
              (DV and THRE can request interrupts when 1)

    ------
    SPI Control:
        bits 31-14     bits 13-8     bits 7-0
            xxxx          DLEN        DIVIDER

        DLEN:     Data length (8 to 32 bits)
        DIVIDER:  SCK frequency = Fclk / (2 * (DIVIDER + 1))

    ------
    SPI Flags:
        bits 31-1   bit 0
           xxxx      BUSY

        BUSY:   SPI exchanging data when 1

    ------
    SPI Slave Select:
        bits 31-2   bit 1   bit 0
            xxxx      ss1     ss0

        ss0:    Selects SPI slave 0 when 0 (active low)
        ss1:    Selects SPI slave 1 when 0 (active low)

    ------
    MAX_COUNT:
        Holds the maximum value of the timer counter. When the timer reaches
        this value it gets reset and requests an interrupt if enabled.
        Writes to MAX_COUNT also reset the timer and its interrupt flag.

    TIMER:
        The current value of the timer (incremented each clock cycle).
        Reads also clear the interrupt flag.

    ------
    GPOUT: General purpose outputs.
	
	// GPOUT definitions
	`define LORA_RESET  (1 << 10)
	`define L_RX        (1 << 9)
	`define L_TX        (1 << 8)
	`define EN_5V_UP    (1 << 7)
	`define EN_5V_M4    (1 << 6)
	`define EN_1V4_M4   (1 << 5)
	`define M2_ON_OFF   (1 << 4)
	`define LED3        (1 << 3)
	`define LED2        (1 << 2)
	`define LED1        (1 << 1)
	`define LED0        (1 << 0)

	// GPIN definitions	
	`define LORA_DIO1   (1 << 1)
	`define LORA_BUSY   (1 << 0)
    ------
    GPIN: General purpose inputs.

    ------
    Interrupt enable:
        bit 0: Not used
        bit 1: Enable TIMER interrupt if 1
        bit 2: Enable UART0 RX interrupt if 1
        bit 3: Enable UART0 TX interrupt if 1
        bit 4: Enable UART1 RX interrupt if 1
        bit 5: Enable UART1 TX interrupt if 1
        bit 6: Enable UART2 RX interrupt if 1
        bit 7: Enable UART2 TX interrupt if 1

    Interrupt Vectors:
        Hold the addresses of the corresponding interrupt service routines.

         
*/

`include "laRVa.v"
`include "uart.v"
`include "spi.v"
`include "timer.v"
	
module SYSTEM (
	input clk,		// Main clock input 25MHz
	input reset,	// Global reset (active high)

	
	input  rxd_0,// UART0
	output txd_0,

	input  rxd_1,// UART1
	output txd_1,

	input  rxd_2,// UART2
	output txd_2,
    
    output sck,  // SPI0
    output mosi, 
    input  miso,  
  
    output sck1, //SPI1 
    input miso1,  
    output mosi1, 
  
    output reg [31:0] gpout, //GPOUT 
    input [31:0] gpin, 
  
    output ADC_CS, 
    output BME_CS, 
    output LORA_CS 
);

wire	cclk;	    // CPU clock
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
wire [31:0] mdo; // memory data output 
ram32 ram0 (  
    .clk       (~cclk),  
	.re        (iramcs),  
    .wrlanes   (iramcs?mwe:4'b0000), 
	.addr      (ca[13:2]), // Máximo 4096 direcciones 
	.data_read (mdo),  
	.data_write(cdo)  
 ); 

//////////////////////////////////////////////////
////////////////// Peripherals ///////////////////
//////////////////////////////////////////////////



// REVISAR ESTO TAMBIEN


wire uart0cs;	// UART0	at offset 0x80
wire uart1cs;	// UART1	at offset 0x90
wire uart2cs;	// UART2	at offset 0xA0

wire spics;		// SPI	(no implementado)
wire irqcs;		// IRQ control at offset 0x00-0x1F
wire irqcsen;	// IRQ control at offset 0x20
wire timercs;
wire spi0cs;
wire spi1cs;
wire gpoutcs;

assign uart0cs = iocs&(ca[7:4]==4'b1000);  // 0xE0000080-0xE000008F
assign uart1cs = iocs&(ca[7:4]==4'b1001);  // 0xE0000090-0xE000009F
assign uart2cs = iocs&(ca[7:4]==4'b1010);  // 0xE00000A0-0xE00000AF

//assign spics  = iocs&(ca[7:5]==3'b011);  // No implementado
assign irqcs   = iocs&(ca[7:5]==3'b000);    // 0xE0000000-0xE000001F
assign irqcsen = iocs&(ca[7:4]==4'b0010);   // 0xE0000020-0xE000002F
assign spi0cs  = iocs&(ca[7:4]==4'b0111);   //0x70 
assign spi1cs  = iocs&(ca[7:4]==4'b0110);   //0x60 
assign timercs = iocs&(ca[7:4]==4'b0100);   //0x40 
assign gpoutcs = iocs&(ca[7:4]==4'b0011);   //0x30 
// Peripheral output bus mux

reg [31:0]iodo;	// Not a register

always@*
 casex (ca[7:2])
    // UARTs
	
	6'b1000x0: iodo<={24'hx,uart0_do};         // UART0 RX data
	6'b1000x1: iodo<={27'hx,ove0,fe0,tend0,thre0,dv0}; // UART0 flags
	6'b1001x0: iodo<={24'hx,uart1_do};         // UART1 RX data
	6'b1001x1: iodo<={27'hx,ove1,fe1,tend1,thre1,dv1}; // UART1 flags
	6'b1010x0: iodo<={24'hx,uart2_do};         // UART2 RX data
	6'b1010x1: iodo<={27'hx,ove2,fe2,tend2,thre2,dv2}; // UART2 flags
	
	
	 // SPI0: 0xE0000070 (RX), 0xE0000074 (flags)
    6'b011100: iodo <= spirx;               // SPI0 RX data
    6'b011101: iodo <= {31'h0, busy};       // SPI0 flags (BUSY)

    // SPI1: 0xE0000060 (RX), 0xE0000064 (flags)
    6'b011000: iodo <= spirx1;              // SPI1 RX data
    6'b011001: iodo <= {31'h0, busy1};      // SPI1 flags

    // GPOUT / GPIN: 0xE0000030, 0xE0000034
    6'b001100: iodo <= {24'h0, gpout};      // GPOUT
    6'b001101: iodo <= {24'h0, gpin};       // GPIN

    // TIMER / IRQ enable: 0xE0000040, 0xE0000020
    6'b010000: iodo<=tcount;                   // Timer counter
	6'b001000: iodo<={24'hx,irqen};            // Interrupt enable

	default:  iodo <= 32'hxxxxxxxx;
		
 endcase //comprobado !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

 //GPOUT Register CHANGED (revisar)
always @(posedge clk) 
    begin 
        if (gpoutcs & mwe[0]) 
            begin 
                gpout <= cdo[31:0]; 
            end 
    end 

/////////////////////////////
// UART0 modificado CLAMIG

wire tend0,thre0,dv0,fe0,ove0; // UART0 Flags

wire [7:0] uart0_do;	// UART0 RX output data
wire uwrtx0;			// UART0 TX write
wire urd0;				// UART0 RX read (for flag clearing)
wire uwrbaud0;			// UART0 BGR write

// Register mapping
// Offset 0: write: TX Holding reg
// Offset 0: read strobe: Clear DV, OVE (also reads RX data buffer)
// Offset 1: write: BAUD divider

assign uwrtx0   = uart0cs & (~ca[2]) & mwe[0];
assign uwrbaud0 = uart0cs & ( ca[2]) & mwe[0] & mwe[1];
assign urd0     = uart0cs & (~ca[2]) & (mwe==4'b0000); // Clear DV, OVE flags

UART_CORE #(.BAUDBITS(12)) uart0 ( .clk(cclk), .txd(txd_0), .rxd(rxd_0), 
	.d(cdo[15:0]), .wrtx(uwrtx0), .wrbaud(uwrbaud0), .rd(urd0), .q(uart0_do),
	.dv(dv0), .fe(fe0), .ove(ove0), .tend(tend0), .thre(thre0) );
	
	
/////////////////////////////
// UART1

wire tend1,thre1,dv1,fe1,ove1; // UART1 Flags

wire [7:0] uart1_do;	// UART1 RX output data
wire uwrtx1;			// UART1 TX write
wire urd1;				// UART1 RX read (for flag clearing)
wire uwrbaud1;			// UART1 BGR write
// Register mapping
// Offset 0: write: TX Holding reg
// Offset 0: read strobe: Clear DV, OVE (also reads RX data buffer)
// Offset 1: write: BAUD divider
assign uwrtx1   = uart1cs & (~ca[2]) & mwe[0];
assign uwrbaud1 = uart1cs & ( ca[2]) & mwe[0] & mwe[1];
assign urd1     = uart1cs & (~ca[2]) & (mwe==4'b0000); // Clear DV, OVE flags

UART_CORE #(.BAUDBITS(12)) uart1 ( .clk(cclk), .txd(txd_1), .rxd(rxd_1), 
	.d(cdo[15:0]), .wrtx(uwrtx1), .wrbaud(uwrbaud1), .rd(urd1), .q(uart1_do),
	.dv(dv1), .fe(fe1), .ove(ove1), .tend(tend1), .thre(thre1) );

/////////////////////////////
// UART2

wire tend2,thre2,dv2,fe2,ove2; // UART2 Flags

wire [7:0] uart2_do;	// UART2 RX output data
wire uwrtx2;			// UART2 TX write
wire urd2;				// UART2 RX read (for flag clearing)
wire uwrbaud2;			// UART2 BGR write
// Register mapping
// Offset 0: write: TX Holding reg
// Offset 0: read strobe: Clear DV, OVE (also reads RX data buffer)
// Offset 1: write: BAUD divider
assign uwrtx2   = uart2cs & (~ca[2]) & mwe[0];
assign uwrbaud2 = uart2cs & ( ca[2]) & mwe[0] & mwe[1];
assign urd2     = uart2cs & (~ca[2]) & (mwe==4'b0000); // Clear DV, OVE flags

UART_CORE #(.BAUDBITS(12)) uart2 ( .clk(cclk), .txd(txd_2), .rxd(rxd_2), 
	.d(cdo[15:0]), .wrtx(uwrtx2), .wrbaud(uwrbaud2), .rd(urd2), .q(uart2_do),
	.dv(dv2), .fe(fe2), .ove(ove2), .tend(tend2), .thre(thre2) );
	
	
//comprobado!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!




/////////////////////////////    
// SPI0
wire spitx;     // SPI write 
wire spictrl; 
wire spisswr; 
wire busy;     // SPI flag 
reg [13:0] spicontrol;  // SPI Control  
wire [31:0] spirx; 
reg [1:0] spi_ss= 2'b11; // SPI Slave Select 


// Decodificacion de direcciones SPI0 (ca[4:2] sobre direcciones palabra):
// 0xE0000070 (0x70) -> ca[4:2] = 3'b100 -> TX data
// 0xE0000074 (0x74) -> ca[4:2] = 3'b101 -> Control
// 0xE0000078 (0x78) -> ca[4:2] = 3'b110 -> Slave Select
assign spitx   = spi0cs & ( ca[4]) & (~ca[3]) & (~ca[2]) & (mwe==4'b1111);  // 0x70
assign spictrl = spi0cs & ( ca[4]) & (~ca[3]) & ( ca[2]) & (mwe==4'b1111);  // 0x74
assign spisswr = spi0cs & ( ca[4]) & ( ca[3]) & (~ca[2]) & (mwe==4'b1111);  // 0x78
 
always @(posedge cclk or posedge reset) 
 begin 
  if (reset) begin
    spicontrol <= 14'h0;
    spi_ss <= 2'b11;
  end else begin
    if (spictrl) 
      spicontrol <= cdo[13:0]; 
    if (spisswr) 
      spi_ss <= cdo[1:0]; 
  end
 end 
  
assign BME_CS = spi_ss[0]; 
assign ADC_CS = spi_ss[1]; 
// LORA_CS se conectará a SPI1 cuando esté implementado

SPI_master spi0 (.clk(cclk), .miso(miso), .wr(spitx), .din(cdo),  
 .divider(spicontrol[7:0]), .bits(spicontrol[13:8]), .sck(sck), 
.mosi(mosi), 
 .busy(busy), .dout(spirx) );

 
///////////////////////////// 
// SPI1 (LORA)
wire spitx1;   
wire spictrl1; 
wire spisswr1; 
wire busy1; 
reg [13:0] spicontrol1; 
wire [31:0] spirx1; 
reg spi_ss1 = 1'b1; 

// Decodificación de direcciones SPI1:
// 0xE0000060 (0x60 = 8'b0110_0000) → ca[4:2] = 3'b000 → TX data
// 0xE0000064 (0x64 = 8'b0110_0100) → ca[4:2] = 3'b001 → Control
// 0xE0000068 (0x68 = 8'b0110_1000) → ca[4:2] = 3'b010 → Slave Select

assign spitx1   = spi1cs & (~ca[4]) & (~ca[3]) & (~ca[2]) & (mwe==4'b1111); // 0x60 TX data
assign spictrl1 = spi1cs & (~ca[4]) & (~ca[3]) & ( ca[2]) & (mwe==4'b1111); // 0x64 Control
assign spisswr1 = spi1cs & (~ca[4]) & ( ca[3]) & (~ca[2]) & (mwe==4'b1111); // 0x68 Slave Select
 
always @(posedge cclk or posedge reset) 
 begin 
  if (reset) begin
    spicontrol1 <= 14'h0;
    spi_ss1 <= 1'b1;
  end else begin
    if (spictrl1) 
      spicontrol1 <= cdo[13:0]; 
    if (spisswr1) 
      spi_ss1 <= cdo[0]; 
  end
 end 
  
assign LORA_CS = spi_ss1; 
SPI_master spi1 (.clk(cclk), .miso(miso1), .wr(spitx1), .din(cdo),  
 .divider(spicontrol1[7:0]), .bits(spicontrol1[13:8]), 
.sck(sck1), .mosi(mosi1), 
 .busy(busy1), .dout(spirx1) );


///////////////////////////// 
// TIMER
wire timer_irq;  
wire [31:0] tcount; 
wire timerwr;
wire timerrd;

assign timerwr = timercs & (mwe==4'b1111);  // Write MAX_COUNT at 0xE0000040
assign timerrd = timercs & (mwe==4'b0000);   // Read TIMER at 0xE0000040

TIMER timer (.clk(cclk), .maxcount(cdo[31:0]), .flagtimer(timer_irq), 
.rd(timerrd), .wr(timerwr) , .contador(tcount[31:0])); 








//////////////////////////////////////////
//    Interrupt control
// IRQ enable reg (8 bits: bits 7-0 for interrupts 6-0 + trap)

reg [7:1]irqen=0;
always @(posedge cclk or posedge reset) begin
	if (reset) irqen<=0; 
    else
	    if (irqcsen & mwe[0]) irqen<=cdo[7:1];
end

// IRQ vectors
reg [31:2]irqvect[0:7];
assign v2 = {irqvect[2],2'b00};

always @(posedge cclk) 
 if (irqcs & (~ca[5]) & (mwe==4'b1111)) irqvect[ca[4:2]]<=cdo[31:2];

// Enabled IRQs (7 interrupts: timer, uart0_rx, uart0_tx, uart1_rx, uart1_tx, uart2_rx, uart2_tx)
wire [7:1]irqpen={irqen[7]&thre2,  // UART2 TX
                  irqen[6]&dv2,    // UART2 RX
                  irqen[5]&thre1,  // UART1 TX
				  irqen[4]&dv1,    // UART1 RX
                  irqen[3]&thre0,  // UART0 TX
				  irqen[2]&dv0,    // UART0 RX
                  irqen[1]&timer_irq}; // Timer

// Priority encoder (8 vectors: trap=0, timer=1, uart0_rx=2, uart0_tx=3, uart1_rx=4, uart1_tx=5, uart2_rx=6, uart2_tx=7)
wire [2:0]vecn = trap       ? 3'b000 : (
				 irqpen[1]  ? 3'b001 : ( // Timer 
				 irqpen[2]  ? 3'b010 : ( // UART0 RX
				 irqpen[3]  ? 3'b011 : ( // UART0 TX
				 irqpen[4]  ? 3'b100 : ( // UART1 RX
				 irqpen[5]  ? 3'b101 : ( // UART1 TX
				 irqpen[6]  ? 3'b110 : ( // UART2 RX
				 irqpen[7]  ? 3'b111 : ( // UART2 TX
				 			  3'bxxx ))))))));
				 			  
assign ivector = irqvect[vecn];
assign irq = (irqpen!=0)|trap;

endmodule	// System
//////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------
//-- 32-bit RAM Memory with independent byte-write lanes
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////
module ram32
(
    input                         clk,
    input                         re,
    input       [3:0]             wrlanes,
    input       [L2N_RAM_SIZE-1:0] addr,
    output      [31:0]            data_read,
    input       [31:0]            data_write
);

    parameter   RAM_SIZE = 4096;
    localparam  L2N_RAM_SIZE = $clog2(RAM_SIZE);
    reg [31:0] ram_array [0:RAM_SIZE-1];
    reg [31:0] data_out;
    assign data_read = data_out;
    // Escritura por bytes
    always @(posedge clk) begin
        if (wrlanes[0])
            ram_array[addr][ 7: 0] <= data_write[ 7: 0];
        if (wrlanes[1])
            ram_array[addr][15: 8] <= data_write[15: 8];
        if (wrlanes[2])
            ram_array[addr][23:16] <= data_write[23:16];
        if (wrlanes[3])
            ram_array[addr][31:24] <= data_write[31:24];
    end
    // Lectura síncrona
    always @(posedge clk) begin
        if (re)
            data_out <= ram_array[addr];
    end
    // Inicialización de memoria
    initial begin
        `ifdef SIMULATION
            $readmemh("rom.hex", ram_array);
        `else
            $readmemh("rand.hex", ram_array);
        `endif
    end

endmodule

