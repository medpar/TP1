#define CS_BME680 0b10

#define STATUS_BME 0x73
#define RESET_BME 0x60
#define ID_BME 0x50

#define Config_BME 0x75
#define CTRL_MEAS 0x74
#define CTRL_HUM 0x72
#define CTRL_GAS_1 0x71
#define CTRL_GAS_0 0x70

#define gas_wait0 0x6D
#define gas_wait1 0x6C
#define gas_wait2 0x6B
#define gas_wait3 0x6A
#define gas_wait4 0x69
#define gas_wait5 0x68
#define gas_wait6 0x67
#define gas_wait7 0x66
#define gas_wait8 0x65
#define gas_wait9 0x64

#define res_heat0 0x63
#define res_heat1 0x62
#define res_heat2 0x61
#define res_heat3 0x60
#define res_heat4 0x5F
#define res_heat5 0x5E
#define res_heat6 0x5D
#define res_heat7 0x5C
#define res_heat8 0x5B
#define res_heat9 0x5A

void startBME680();
void writeBME680(char data,char dir);
char readBME680(char dir);
int returnTemp();
int returnPressure();
int returnHUMIDITY(int temp_comp);