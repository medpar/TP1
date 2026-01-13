#define ENABLE_5V	(GPOUT = 0b00001000)
#define ENABLE_1V4	(GPOUT = 0b00010000)
#define DISABLE_5V_1V4	(GPOUT = 0b00000000)

void readGas();
void readCO();
void readCH4();