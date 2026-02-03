//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									                //
//	Grupo 1:													                          //
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				      //
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			      	//
//////////////////////////////////////////////////////////////////

// Implementacion del modulo LoRa Sx1262 obtenida a partir del codigo
// de ejemplo disponible en el Campus Virtual de la asignatura.

typedef unsigned short int byte;

#define false  0
#define true  1

uint32_t LoraSx1262_pllFrequency;
uint8_t LoraSx1262_bandwidth;
uint8_t LoraSx1262_codingRate;
uint8_t LoraSx1262_spreadingFactor;
uint8_t LoraSx1262_lowDataRateOptimize;
uint32_t LoraSx1262_transmitTimeout;
uint8_t spiBuff[32];
unsigned int inReceiveMode;

#define SX1262_NSS   (*(volatile uint32_t*)0xE0000038)
#define SX1262_DIO1  (1<<1)

#define SPI1DAT	 (*(volatile uint32_t*)0xE0000030)
#define SPI1CTRL (*(volatile uint32_t*)0xE0000034)
#define SPI1STA	 (*(volatile uint32_t*)0xE0000034)
#define SPI1SS	 (*(volatile uint32_t*)0xE0000038)


void _memcpy(unsigned char *dest, unsigned char * src, unsigned int n)
{
 int i;
 for(i=0;i<n;i++) dest[i]=src[i]; 
}


unsigned long millis(){ return (TIMER)/(CCLK/1000);}   

// Transferencia de datos por SPI
unsigned char SPI_transfer(unsigned char *lista, int n){
   int i;
  SPI1SS = 0b1110;      // Chip select
  for(i=0;i<n;i++){
    SPI1DAT = lista[i]; 
	_printf("%02x ", lista[i]);
	
    while(SPI1STA & 1);
    lista[i] = SPI1DAT;
  }
  SPI1SS = 0b1111;
  _printf("\n");
  for(i=0;i<n;i++) _printf("%02x ", lista[i]);
  _printf("\n\n");
  
  
  return SPI1DAT;
}


void digitalWrite(unsigned int REG, unsigned int data){

    if(data){
        REG = REG | (1<<data);
    }else{ 
	    while(GPIN & (1<<0)); // Espero a que LORA BUSY este libre
		REG = REG & (~(1<<data));
    }
}


unsigned int digitalRead(unsigned int REG){
    // Devuelve el valor de los pines de entrada
    return GPIN;
}

// Inicializa el modulo LoRa
unsigned int LoraSx1262_begin(){ 

  digitalWrite(SX1262_NSS, 1);
	_delay_ms(10);
	GPOUT |= (1<<2); // LORA_RESET = 1
	_delay_ms(10);

  // Modo transmisor
	GPOUT |=  (1<<1); // L_TX = 1
	GPOUT &= ~(1<<0); // L_RX = 0

  unsigned int success = LoraSx1262_sanityCheck();
	
  if(!success){
      return 0;
  }
  // Configura los parametros basicos de radio
  LoraSx1262_configureRadioEssentials();
  return 1;
}

// Comprobacion de funcionamiento correcto
unsigned int LoraSx1262_sanityCheck(){
  digitalWrite(SX1262_NSS, 0);
	_delay_ms(10);
  spiBuff[0] = 0x1D;
  spiBuff[1] = 0x07;
  spiBuff[2] = 0x40;
  spiBuff[3] = 0x00;
	spiBuff[4] = 0x00;
	
  SPI_transfer(spiBuff, 5);

  digitalWrite(SX1262_NSS, 1);
	
	if(spiBuff[4] == 0x14){
		_printf("Comprobado funcionamiento correcto\n");
		return 1;  
	}else{	
		return 0; 
	}

}


// Configuracion basica de radio
void LoraSx1262_configureRadioEssentials() {
  digitalWrite(SX1262_NSS, 0); 
  spiBuff[0] = 0x9D;
  spiBuff[1] = 0x01;
  SPI_transfer(spiBuff, 2);
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);

  // Fija la frecuencia inicial de radio
  LoraSx1262_configSetFrequency(868000000);

  // Pone el modem en modo LoRa
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x8A;
  spiBuff[1] = 0x01;
  SPI_transfer(spiBuff, 2);
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);

  // Ajuste del tiempo de espera de recepcion
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x9F;
  spiBuff[1] = 0x00;
  SPI_transfer(spiBuff, 2); 
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);

  // Parametros de modulacion por defecto
  LoraSx1262_configSetPreset(PRESET_LONGRANGE);

  // Configuracion del amplificador de potencia
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x95;
  spiBuff[1] = 0x04;
  spiBuff[2] = 0x07;
  spiBuff[3] = 0x00;
  spiBuff[4] = 0x01;
  SPI_transfer(spiBuff, 5); 
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);

  // Parametros de transmision
  digitalWrite(SX1262_NSS,0);  
  spiBuff[0] = 0x8E;
  spiBuff[1] = 22;
  spiBuff[2] = 0x02;
  SPI_transfer(spiBuff, 3);
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);

  // tiempo de espera por numero de simbolos LoRa
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0xA0;
  spiBuff[1] = 0x00;
  SPI_transfer(spiBuff, 2);
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);

  // Habilita interrupciones de radio
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x08;
  spiBuff[1] = 0x00;
  spiBuff[2] = 0x02;
  spiBuff[3] = 0xFF;
  spiBuff[4] = 0xFF;
  spiBuff[5] = 0x00;
  spiBuff[6] = 0x00;
  spiBuff[7] = 0x00;
  spiBuff[8] = 0x00;
  SPI_transfer(spiBuff, 9);
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);
  _printf("configureRadioEssentials comprobado\n");
}

// Transmision
void LoraSx1262_transmit(uint8_t *data, int dataLen) {
  // Limite del tamano de carga util permitido por LoRa
  if (dataLen > 255) { dataLen = 255;}

  GPOUT |=  (1<<1); // L_TX = 1
  GPOUT &= ~(1<<0); // L_RX = 0
	
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x8C;
  spiBuff[1] = 0x00;
  spiBuff[2] = 0x08;
  spiBuff[3] = 0x00;
  spiBuff[4] = dataLen;
  spiBuff[5] = 0x00;
  spiBuff[6] = 0x00;
  SPI_transfer(spiBuff, 7);
  digitalWrite(SX1262_NSS, 1);  
  LoraSx1262_waitForRadioCommandCompletion(100);

  // Escribe la carga util en el buffer de radio
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x0E,
  spiBuff[1] = 0x00;
  SPI_transfer(spiBuff, 2);    // Envia cabecera

  uint8_t size = sizeof(spiBuff);
  for (uint16_t i = 0; i < dataLen; i += size) {
    if (i + size > dataLen) { size = dataLen - i; }
    _memcpy(spiBuff,&(data[i]),size);
    SPI_transfer(spiBuff, size); 
  }
 
  digitalWrite(SX1262_NSS, 1);  
  LoraSx1262_waitForRadioCommandCompletion(1000);

  // Inicia la transmision con tiempo de espera
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x83;
  spiBuff[1] = 0xFF;
  spiBuff[2] = 0xFF;
  spiBuff[3] = 0xFF;
  SPI_transfer(spiBuff, 4);
  digitalWrite(SX1262_NSS, 1);  
  LoraSx1262_waitForRadioCommandCompletion(LoraSx1262_transmitTimeout);

  // Quedamos en modo TX hasta que se cambie a RX
  unsigned int inReceiveMode = false;
}

// Espera a que se termine el comando
unsigned int LoraSx1262_waitForRadioCommandCompletion(uint32_t timeout) {
  uint32_t startTime = millis();
  unsigned int dataTransmitted = false;

  while (!dataTransmitted) {
    // Pausa para evitar consultas demasiado rapidas
    _delay_ms(5);

    // Consulta estado de radio
    digitalWrite(SX1262_NSS, 0);  
    spiBuff[0] = 0xC0;
    spiBuff[1] = 0x00;
    SPI_transfer(spiBuff, 2);
    digitalWrite(SX1262_NSS, 1);  

    // Decodifica el estado del chip y del comando
    uint8_t chipMode = (spiBuff[1] >> 4) & 0x7;
    uint8_t commandStatus = (spiBuff[1] >> 1) & 0x7;

    // Estado 0,1,2 indica ocupado; el resto indica fin del comando
    if (commandStatus != 0 && commandStatus != 1 && commandStatus != 2) {
      dataTransmitted = true;
    }

    // Si entra en espera, damos por completado
    if (chipMode == 0x03 || chipMode == 0x02) {
      dataTransmitted = true;
    }

    // Evita bucles infinitos con tiempo de espera
    if (millis() - startTime >= timeout) {
      return false;
    }
  }
  
  return true;
}


// Recepcion
void LoraSx1262_setModeReceive() {
  if (inReceiveMode) { return; }
  // Forzamos solo a recepcion
  GPOUT &= ~(1<<1); // L_TX = 0
  GPOUT |=  (1<<0); // L_RX = 1
  
  // Parametros de paquete para recepcion
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x8C;
  spiBuff[1] = 0x00;
  spiBuff[2] = 0x08;
  spiBuff[3] = 0x00;
  spiBuff[4] = 0xFF;
  spiBuff[5] = 0x00;
  spiBuff[6] = 0x00;
  SPI_transfer(spiBuff, 7);
  digitalWrite(SX1262_NSS, 1);  
  LoraSx1262_waitForRadioCommandCompletion(100);

  // Deja el chip a la espera de un paquete
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x82;
  spiBuff[1] = 0xFF;
  spiBuff[2] = 0xFF;
  spiBuff[3] = 0xFF;
  SPI_transfer(spiBuff, 4);
  digitalWrite(SX1262_NSS, 1);  
  LoraSx1262_waitForRadioCommandCompletion(100);

  // Marca que estamos en modo recepcion
  unsigned int  inReceiveMode = true;
}

// Recepcion asincrona
int LoraSx1262_lora_receive_async(uint8_t* buff, int buffMaxLen) {
  LoraSx1262_setModeReceive(); // Pasa a modo recepcion si hace falta
  _printf("\n\n\n");
  _printf("Comenzando recepcion");
  // Si no hay interrupcion, no hay paquete listo
  if ((GPIN & (1<<1)) == false) { return -1; }

  // Limpia las interrupciones de radio
  while ((GPIN & (1<<1))) {
    digitalWrite(SX1262_NSS, 0);  
    spiBuff[0] = 0x02;
    spiBuff[1] = 0xFF;
    spiBuff[2] = 0xFF;
    SPI_transfer(spiBuff, 3);
    digitalWrite(SX1262_NSS, 1);  
  }

  // Lee estado del paquete (RSSI, SNR, etc)
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x14;
  spiBuff[1] = 0xFF;
  spiBuff[2] = 0xFF;
  spiBuff[3] = 0xFF;
  spiBuff[4] = 0xFF;
  SPI_transfer(spiBuff, 5);
  digitalWrite(SX1262_NSS, 1);  

  // Convierte las metricas reportadas por el chip
  unsigned int rssi       = -((int)spiBuff[2]) / 2;
  unsigned int snr        =  ((int8_t)spiBuff[3]) / 4;
  unsigned int signalRssi = -((int)spiBuff[4]) / 2;
    
   _printf("RSSI: %d\n", rssi);
   _printf("SNR: %d\n", snr);
   _printf("Signal_RSSI: %d\n", signalRssi);
   
  // Consulta longitud y direccion de inicio del paquete
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x13;
  spiBuff[1] = 0xFF;
  spiBuff[2] = 0xFF;
  spiBuff[3] = 0xFF;
  SPI_transfer(spiBuff, 4);
  digitalWrite(SX1262_NSS, 1);  

  uint8_t payloadLen = spiBuff[2];
  uint8_t startAddress = spiBuff[3];

  // Ajusta al tamano del buffer del usuario
  if (buffMaxLen < payloadLen) {payloadLen = buffMaxLen;}

  // Lee el paquete desde la memoria del SX1262
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x1E;
  spiBuff[1] = startAddress;
  spiBuff[2] = 0x00;
  SPI_transfer(spiBuff, 3);    // Inicia la lectura
  SPI_transfer(buff,payloadLen);  // Copia datos al buffer del usuario
  digitalWrite(SX1262_NSS, 1);  
  _printf("Paquete: ");
  _printf("%d\n",buff);

  return payloadLen;
}

// Recepcion con bloqueo
int LoraSx1262_lora_receive_blocking(uint8_t *buff, int buffMaxLen, uint32_t timeout) {
  LoraSx1262_setModeReceive(); // Pasa a modo recepcion si hace falta
 
  uint32_t startTime = millis();
  uint32_t elapsed = startTime;

  // Espera a la interrupcion o al tiempo de espera
  while (digitalRead(SX1262_DIO1) == false) {
    if (timeout > 0) {
      elapsed = millis() - startTime;
      if (elapsed >= timeout) {
        return -1;
      }
    }
  }
  
  // Si hay paquete, lo leemos
  return LoraSx1262_lora_receive_async(buff,buffMaxLen);
}

// Actualiza la frecuencia del PLL
void LoraSx1262_updateRadioFrequency() {
  digitalWrite(SX1262_NSS, 0);  
  spiBuff[0] = 0x86;
  spiBuff[1] = (LoraSx1262_pllFrequency >> 24) & 0xFF;
  spiBuff[2] = (LoraSx1262_pllFrequency >> 16) & 0xFF;
  spiBuff[3] = (LoraSx1262_pllFrequency >>  8) & 0xFF;
  spiBuff[4] = (LoraSx1262_pllFrequency >>  0) & 0xFF;
  SPI_transfer(spiBuff, 5);
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);
}

// Configuracion de parametros de modulacion
void LoraSx1262_updateModulationParameters() {
  digitalWrite(SX1262_NSS, 0);       
  spiBuff[0] = 0x8B;
  spiBuff[1] = LoraSx1262_spreadingFactor;
  spiBuff[2] = LoraSx1262_bandwidth;
  spiBuff[3] = LoraSx1262_codingRate;
  spiBuff[4] = LoraSx1262_lowDataRateOptimize;
  SPI_transfer(spiBuff, 5);
  digitalWrite(SX1262_NSS, 1);  
  _delay_ms(100);

  // Ajusta el tiempo de espera segun el factor de expansion
  switch (LoraSx1262_spreadingFactor) {
    case 12:
      LoraSx1262_transmitTimeout = 252000;   // 126 segundos
      break;
    case 11:
      LoraSx1262_transmitTimeout = 160000;   // 81 segundos
      break;
    case 10:
      LoraSx1262_transmitTimeout = 60000;   // 36 segundos
      break;
    case 9:
      LoraSx1262_transmitTimeout = 40000;   // 20 segundos
      break;
    case 8:
      LoraSx1262_transmitTimeout = 20000;   // 11 segundos
      break;
    case 7:
      LoraSx1262_transmitTimeout = 12000;   // 6.3 segundos  
      break;
    case 6:
      LoraSx1262_transmitTimeout = 7000;   // 3.7 segundos
      break;
    default:  
      LoraSx1262_transmitTimeout = 5000;   // 2.2 segundos 
      break;
  }
}

unsigned int LoraSx1262_configSetPreset(int preset) {
  if (preset == PRESET_DEFAULT) {
    LoraSx1262_bandwidth = 5;                 // 250kHz
    LoraSx1262_codingRate = 1;                // CR_4_5
    LoraSx1262_spreadingFactor = 7;           // SF7
    LoraSx1262_lowDataRateOptimize = 0;        
    LoraSx1262_updateModulationParameters();
    return true;
  }

  if (preset == PRESET_LONGRANGE) {
    LoraSx1262_bandwidth = 4;                 // 125kHz
    LoraSx1262_codingRate = 1;                // CR_4_5
    LoraSx1262_spreadingFactor = 7;           // SF7
    LoraSx1262_lowDataRateOptimize = 0;  
    LoraSx1262_updateModulationParameters();
    return true;
  }

  if (preset == PRESET_FAST) {
    LoraSx1262_bandwidth = 6;                 // 500kHz
    LoraSx1262_codingRate = 1;                // CR_4_5
    LoraSx1262_spreadingFactor = 5;           // SF5
    LoraSx1262_lowDataRateOptimize = 0;
    LoraSx1262_updateModulationParameters();
    return true;
  }

  return false;
}


unsigned int LoraSx1262_configSetFrequency(long frequencyInHz) {
  // Verifica que la frecuencia este dentro de rango
  if (frequencyInHz < 150000000 || frequencyInHz > 960000000) { return false;}

  // Calcula la frecuencia del PLL
  LoraSx1262_pllFrequency = LoraSx1262_frequencyToPLL(frequencyInHz);
  LoraSx1262_updateRadioFrequency();
  _printf("Frecuencia fijada a: %d\n", frequencyInHz);
  _printf("Frecuencia del PLL fijada a: %d\n", LoraSx1262_pllFrequency);
  return true;
}


unsigned int LoraSx1262_configSetBandwidth(int bandwidth) {
  // Ancho de banda valido: 0-10, excepto 7
  if (bandwidth < 0 || bandwidth > 0x0A || bandwidth == 7) { return false; }
  LoraSx1262_bandwidth = bandwidth;
  LoraSx1262_updateModulationParameters();
  return true;
}


unsigned int LoraSx1262_configSetCodingRate(int codingRate) {
  // Tasa de codificacion valida: 1-4
  if (codingRate < 1 || codingRate > 4) { return false; }
  LoraSx1262_codingRate = codingRate;
  LoraSx1262_updateModulationParameters();
  return true;
}


unsigned int LoraSx126_configSetSpreadingFactor(int spreadingFactor) {
  if (spreadingFactor < 5 || spreadingFactor > 12) { return false; }

  // Recomendado activar LowDataRateOptimize para SF11 y SF12
  LoraSx1262_lowDataRateOptimize = (spreadingFactor >= 11) ? 1 : 0;
  LoraSx1262_spreadingFactor = spreadingFactor;
  LoraSx1262_updateModulationParameters();
  return true;
}


uint32_t LoraSx1262_frequencyToPLL(long rfFreq) {
  // Calcula  la frecuencia del PLL evitando overflow en enteros
  uint32_t q = rfFreq / 15625UL;
  uint32_t r = rfFreq % 15625UL;

  // Multiplica por 16384 y ajusta el resto
  q *= 16384UL;
  r *= 16384UL;
  
  return q + (r / 15625UL);
}