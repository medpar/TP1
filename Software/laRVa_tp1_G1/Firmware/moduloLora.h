//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									                //
//	Grupo 1:													                          //
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				      //
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			      	//
//////////////////////////////////////////////////////////////////

// Definicion del modulo LoRa Sx1262 obtenida a partir del codigo
// de ejemplo disponible en el Campus Virtual de la asignatura.

#ifndef __LORA1262__
#define __LORA1262__


/* Wiring requirements (for default shield pinout)
# +-----------------+-------------+------------+
# | Description     | Arduino Pin | Sx1262 Pin |
# +-----------------+-------------+------------+
# | Power (3v3)     | 3v3         | 3v3        |
# | GND             | GND         | GND        |
# | Radio Reset     | A0          | SX_NRESET  |
# | Busy (optional) | D3          | BUSY       |
# | Radio Interrupt | D5          | DIO1       |
# | SPI SCK         | 13          | SCK        |
# | SPI MOSI        | D11         | MOSI       |
# | SPI MISO        | D12         | MISO       |
# | SPI CS          | D7          | NSS        |
# +-----------------+----------+------------+
*/

//Presets
#define PRESET_DEFAULT    0
#define PRESET_LONGRANGE  1
#define PRESET_FAST       2


typedef struct{
  int rssi;
  int snr;
  unsigned int inReceiveMode;
  int signalRssi;
} LoraSx1262;


unsigned char SPI_transfer(unsigned char *lista, int n);
unsigned int LoraSx1262_begin();
unsigned int LoraSx1262_sanityCheck();
void LoraSx1262_transmit(uint8_t *data, int dataLen);
int LoraSx1262_lora_receive_async(uint8_t* buff, int buffMaxLen);
int LoraSx1262_lora_receive_blocking(uint8_t* buff, int buffMaxLen, uint32_t timeout);
unsigned int LoraSx1262_configSetPreset(int preset);
unsigned int LoraSx1262_configSetFrequency(long frequencyInHz);
unsigned int LoraSx1262_configSetBandwidth(int bandwidth);
unsigned int LoraSx1262_configSetCodingRate(int codingRate);
unsigned int LoraSx1262_configSetSpreadingFactor(int spreadingFactor);
uint32_t LoraSx1262_frequencyToPLL(long freqInHz);


static void LoraSx1262_setModeReceive();
static void LoraSx1262_configureRadioEssentials();
static unsigned int LoraSx1262_waitForRadioCommandCompletion(uint32_t timeout);
static void LoraSx1262_updateRadioFrequency();
static void LoraSx1262_updateModulationParameters();

#endif