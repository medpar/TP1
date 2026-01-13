#define ADC_CS 	0b01

#define MCP3004_START 0b00000001	
#define MCP3004_CH0   0b10000000	
#define MCP3004_CH1   0b10010000	
#define MCP3004_CH2   0b10100000
#define MCP3004_CH3   0b10110000

int readMCP3004(unsigned char channel);