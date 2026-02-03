//--------------------------------------------------------------------
// RISC-V things
// by Jesús Arias
//--------------------------------------------------------------------
`include "system.v"
`include "pll.v"

// Top module. (signals assigned to actual pins in file "pines.pcf")
module main(
 
    input  CLKIN,      // Input clock from crystal oscillator 
    // UART   Consola 
    input  RXD,        // P8 
    output TXD,        // P9 
    // UART 1 GPS    
    input  RXD1,       // P49 GPS_TX 
    output TXD1,       // P52 GPS_RX 
    // UART 2 ESP32-C3 
    input  RXD2,       // P122 ESP32-C3_TX 
    output TXD2,       // P121 ESP32-C3_RX 
    // ICE SPI 
    output ICE_SCK,   
    output ICE_MOSI,  
    input  ICE_MISO,  
     
    output BME680_CS,  // BME680      Chip Select (active low) 
    output ADC_CS,     // MCP3004 ADC Chip Select (active low) 
    // LORA SPI 
    output LORA_SCK,  // P115  
    output LORA_MOSI, // P110 
    input  LORA_MISO, // P113 
     
    output LORA_CS,   // LORA (P117)  Chip Select (active low) 
    
    input  LORA_BUSY, //  
    input  LORA_DIO1, // LORA DIO1 interrupt SX1262 
    output LORA_RESET,// LORA_RESET (P119) 
    output L_RX,      // LORA RX activa entrada SX1262 
    output L_TX,      // LORA TX activa salida SX1262 
 

    // LEDS GPOUT 
    output ICE_LED1, 
    output ICE_LED2, 
    output ICE_LED3, 
    output ICE_LED4, 
    // CONTROL SENSORES GPOUT 
    output EN_5V_UP, 
    output EN_5V_M4, 
    output EN_1V4_M4,  
    output M2_ON_OFF, 

	// JTAG interface 
	input  TCK, 
	input  TMS, 
	input  TDI, 
	output TDO 

);

//-- PLL: generates a 25MHz master clock from a 16MHz input
wire clk,pll_lock;

pll
  pll1(
	.clock_in(CLKIN),
	.clock_out(clk),
	.locked(pll_lock)
	);

wire [31:0] gpout;
wire [31:0] gpin;

assign gpin = {30'b0, LORA_DIO1, LORA_BUSY};

// Instance of the system
SYSTEM sys1 (
    .clk(clk),
    .reset(reset),
    .rxd_0(RXD),    
    .txd_0(TXD),    
    .rxd_1(RXD1),   
    .txd_1(TXD1),       
    .rxd_2(RXD2),   
    .txd_2(TXD2),
    .sck(ICE_SCK),
    .mosi(ICE_MOSI),
    .miso(ICE_MISO),
    .sck1(LORA_SCK),
    .miso1(LORA_MISO),
    .mosi1(LORA_MOSI),
    .gpout(gpout),
    .gpin(gpin),
    .ADC_CS(ADC_CS),
    .BME_CS(BME680_CS),
    .LORA_CS(LORA_CS)  

);

assign LORA_RESET = gpout[10];
assign L_RX       = gpout[9];
assign L_TX       = gpout[8];
assign EN_5V_UP   = gpout[7];
assign EN_5V_M4   = gpout[6];
assign EN_1V4_M4  = gpout[5];
assign M2_ON_OFF  = gpout[4];
assign ICE_LED4   = gpout[3];
assign ICE_LED3   = gpout[2];
assign ICE_LED2   = gpout[1];
assign ICE_LED1   = gpout[0];
assign TDO        = 1'b0;

// Automatic RESET pulse: Reset is held active for 255 cycles after PLL lock

reg [21:0]cnt=22'h3fffff;
wire reset=(cnt!=0);

always @(posedge clk) cnt<= reset ? cnt-1: cnt;


endmodule


