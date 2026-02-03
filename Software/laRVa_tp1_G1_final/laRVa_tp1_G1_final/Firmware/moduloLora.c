typedef unsigned short int byte;
//#include <cstdint>
//#include "Arduino.h"

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



/*
// Mapa de registros del datasheet LoRa
#define LORA_HOPP_ENABLE (0x0385)
#define LORA_PACK_LENGTH (0x0386)
#define LORA_HOPP_ENABLE (0x0387)
#define LORA_PACK_LENGTH (0x0388)
#define LORA_HOPP_ENABLE (0x0020)
#define LORA_PACK_LENGTH (0x0020)
#define LORA_HOPP_ENABLE (0x0020)
#define LORA_PACK_LENGTH (0x0020)
*/



void _memcpy(unsigned char *dest, unsigned char * src, unsigned int n)
{
 int i;
 for(i=0;i<n;i++) dest[i]=src[i]; 
}


unsigned long millis(){ return (TIMER)/(CCLK/1000);}   


//Función que se encarga de transmitir
unsigned char SPI_transfer(unsigned char *lista, int n){
   int i;
   	SPI1SS = 0b1110; //Habil1itamos el chip select
   for(i=0;i<n;i++){
    SPI1DAT = lista[i]; // Escribimos el dato en el registro SPIDAT (spi_data)
	_printf("%02x ", lista[i]);
	//_printf("%c\n", SPI1DAT);
	
    while(SPI1STA & 1); // Leemos en el registro SPISTA (spi_status) para comprobar que haya funcionado bien.
    lista[i] = SPI1DAT;
    //_printf("Leo (%02x)  \n", lista[i]);
  }
  SPI1SS = 0b1111;
  _printf("\n");
  for(i=0;i<n;i++) _printf("%02x ", lista[i]);
  _printf("\n\n");
  
  
  return SPI1DAT;
}

/*
//Función que se encarga de transmitir
unsigned char SPI_transfer(unsigned char dato){

    SPIDAT = dato; // Escribimos el dato en el registro SPIDAT (spi_data)
    while(SPISTA & 1); // Leemos en el registro SPISTA (spi_status) para comprobar que haya funcionado bien.
    return SPIDAT;
}
*/

//Función que emula el comportamiento de digitalWrite 
void digitalWrite(unsigned int REG, unsigned int data){

    if(data){
        REG = REG | (1<<data); // Mete dato (1) en la posición 1 del registro 
    }else{ 
	    while(GPIN & (1<<0)); // Espero a que LORA BUSY este en cero(libre)
		REG = REG & (~(1<<data)); // Mete dato (0) en la posición 1 del registro 
    }
}
// Poner un bit de un registro a 1
// REG = REG | (1<<bit)
// Poner un bit de un registro a 0
// REG = REG & (~(1<<bit))
// Leer un bit
// ((REG  & (1<<bit))>>bit)


//Función que emula el comportamiento de digitalRead
unsigned int digitalRead(unsigned int REG){
    //Con devolver el valor del reg gpin (proposito general vale)
    return GPIN;
}




unsigned int LoraSx1262_begin(){ //Cambiamos miembro de clase por función (changed, he cambiado bool por int)

    //(CHANGED)SPI_begin(); //inicializado en el system.v linea 290
    /*Los pinModes no se ponen input o output, ya se conectarán los cables dode toque
        pinMode(SX1262_NSS, OUTPUT);
        pinMode(SX1262_RESET, OUTPUT);
        pinMode(SX1262_DIO1, INPUT);
    */
	//_printf("Primer punto\n");
    digitalWrite(SX1262_NSS, 1);
	_delay_ms(10);
	GPOUT |= (1<<2); // LORA_RESET = 1
	_delay_ms(10);


	//LO FORZAMOS SOLO A TRANSMITIR
	GPOUT |=  (1<<1); // L_TX = 1
	GPOUT &= ~(1<<0); // L_RX = 0

	//////////////////////////////
	
	//_printf("Segundo punto\n");
    unsigned int success = LoraSx1262_sanityCheck();
	//_printf("Tercer punto\n");
	
    if(!success){
		//_printf("Ha cascado\n");
        return 0;
		
    }
	//_printf("No Ha cascado\n");
    LoraSx1262_configureRadioEssentials();
	//_printf("no no Ha cascado\n");
    return 1;
}



unsigned int LoraSx1262_sanityCheck(){
	//_printf("Entrando\n");
    digitalWrite(SX1262_NSS, 0); //ERROR
	_delay_ms(10);
    spiBuff[0] = 0x1D;
    spiBuff[1] = 0x07;
    spiBuff[2] = 0x40;
    spiBuff[3] = 0x00;
	spiBuff[4] = 0x00;
	
    SPI_transfer(spiBuff, 5);
  
    digitalWrite(SX1262_NSS, 1);  //CS High = Disabled
	
	if(spiBuff[4] == 0x14){
		_printf("SanityCheck comprobado\n");
		return 1; // Valido 
	}else{	
		return 0; // Error
	}

}


void LoraSx1262_configureRadioEssentials() {
  _printf("Entrando en configureRadioEssentials\n");
  //Tell DIO2 to control the RF switch so we don't have to do it manually
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x9D;  //Opcode for "SetDIO2AsRfSwitchCtrl"
  spiBuff[1] = 0x01;  //Enable
  SPI_transfer(spiBuff, 2);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100); //EQUIVALENTE A delay(100); //Give time for the radio to proces command

  //Just a single SPI command to set the frequency, but it's broken out
  //into its own function so we can call it on-the-fly when the config changes
  LoraSx1262_configSetFrequency(868000000);  //Set default frequency to 868mhz

  //Set modem to LoRa (described in datasheet section 13.4.2)
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x8A;          //Opcode for "SetPacketType"
  spiBuff[1] = 0x01;          //Packet Type: 0x00=GFSK, 0x01=LoRa
  SPI_transfer(spiBuff, 2);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100); //EQUIVALENTE A delay(100);                  //Give time for radio to process the command

  //Set Rx Timeout to reset on SyncWord or Header detection
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x9F;          //Opcode for "StopTimerOnPreamble"
  spiBuff[1] = 0x00;          //Stop timer on:  0x00=SyncWord or header detection, 0x01=preamble detection  SPI.transfer(spiBuff,2); //DUDA, ES 0X00 O 0X01 POR EL PREAMBULO
  SPI_transfer(spiBuff, 2); 
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100); //EQUIVALENTE A delay(100);                  //Give time for radio to process the command

  //Set modulation parameters is just one more SPI command, but since it
  //is often called frequently when changing the radio config, it's broken up into its own function
  LoraSx1262_configSetPreset(PRESET_LONGRANGE);  //Sets default modulation parameters (changed from DEFAULT TO LONGRANGE)

  // Set PA Config
  // See datasheet 13.1.4 for descriptions and optimal settings recommendations
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x95;          //Opcode for "SetPaConfig"
  spiBuff[1] = 0x04;          //paDutyCycle. See datasheet, set in conjuntion with hpMax
  spiBuff[2] = 0x07;          //hpMax.  Basically Tx power.  0x00-0x07 where 0x07 is max power (changed) (EY)
  spiBuff[3] = 0x00;          //device select: 0x00 = SX1262, 0x01 = SX1261
  spiBuff[4] = 0x01;          //paLut (reserved, always set to 1)
  SPI_transfer(spiBuff, 5); 
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100); //EQUIVALENTE A delay(100);                  //Give time for radio to process the command

  // Set TX Params
  // See datasheet 13.4.4 for details
  digitalWrite(SX1262_NSS,0); //Enable radio chip-select
  spiBuff[0] = 0x8E;          //Opcode for SetTxParams
  spiBuff[1] = 22;            //Power.  Can be -17(0xEF) to +14(0x0E) in Low Pow mode.  -9(0xF7) to 22(0x16) in high power mode (CHANGED) (EY)
  spiBuff[2] = 0x02;          //Ramp time. Lookup table.  See table 13-41. 0x02="40uS"
  SPI_transfer(spiBuff, 3);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100); //EQUIVALENTE A delay(100);                  //Give time for radio to process the command

  //Set LoRa Symbol Number timeout
  //How many symbols are needed for a good receive.
  //Symbols are preamble symbols
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0xA0;          //Opcode for "SetLoRaSymbNumTimeout"
  spiBuff[1] = 0x00;          //Number of symbols.  Ping-pong example from Semtech uses 5
  SPI_transfer(spiBuff, 2);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100); //EQUIVALENTE A delay(100);                  //Give time for radio to process the command

  //Enable interrupts
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x08;        //0x08 is the opcode for "SetDioIrqParams"
  spiBuff[1] = 0x00;        //IRQMask MSB.  IRQMask is "what interrupts are enabled"
  spiBuff[2] = 0x02;        //IRQMask LSB         See datasheet table 13-29 for details
  spiBuff[3] = 0xFF;        //DIO1 mask MSB.  Of the interrupts detected, which should be triggered on DIO1 pin
  spiBuff[4] = 0xFF;        //DIO1 Mask LSB
  spiBuff[5] = 0x00;        //DIO2 Mask MSB
  spiBuff[6] = 0x00;        //DIO2 Mask LSB
  spiBuff[7] = 0x00;        //DIO3 Mask MSB
  spiBuff[8] = 0x00;        //DIO3 Mask LSB
  SPI_transfer(spiBuff, 9);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100); //EQUIVALENTE A delay(100);                  //Give time for radio to process the command
  _printf("configureRadioEssentials comprobado\n");
}


void LoraSx1262_transmit(uint8_t *data, int dataLen) {
  _printf("Entrando en transmit\n");
  //Max lora packet size is 255 bytes
  if (dataLen > 255) { dataLen = 255;}

  /*Set packet parameters
  # Tell LoRa what kind of packet we're sending (and how long)
  # Parameters are:
  # - Preamble Length MSB
  # - Preamble Length LSB
  # - Header Type (variable vs fixed len)
  # - Payload Length
  # - CRC on/off
  # - IQ Inversion on/off
  */
  //LO FORZAMOS SOLO A TRANSMITIR
  GPOUT |=  (1<<1); // L_TX = 1
  GPOUT &= ~(1<<0); // L_RX = 0
	
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x8C;          //Opcode for "SetPacketParameters"
  spiBuff[1] = 0x00;          //PacketParam1 = Preamble Len MSB
  spiBuff[2] = 0x08;          //PacketParam2 = Preamble Len LSB
  spiBuff[3] = 0x00;          //PacketParam3 = Header Type. 0x00 = Variable Len, 0x01 = Fixed Length
  spiBuff[4] = dataLen;       //PacketParam4 = Payload Length (Max is 255 bytes)
  spiBuff[5] = 0x00;          //PacketParam5 = CRC Type. 0x00 = Off, 0x01 = on
  spiBuff[6] = 0x00;          //PacketParam6 = Invert IQ.  0x00 = Standard, 0x01 = Inverted
  SPI_transfer(spiBuff, 7);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  LoraSx1262_waitForRadioCommandCompletion(100);  //Give time for radio to process the command
  _printf("Primer punto de control\n");
  //Write the payload to the buffer
  //  Reminder: PayloadLength is defined in setPacketParams
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x0E,          //Opcode for WriteBuffer command
  spiBuff[1] = 0x00;          //Dummy byte before writing payload
  SPI_transfer(spiBuff, 2);    //Send header info
  //_printf("Segundo punto de control\n");
  //SPI.transfer overwrites original buffer.  This could probably be confusing to the user
  //If they tried writing the same buffer twice and got different results
  //Eg "radio.transmit(buff,10); radio.transmit(buff,10);" would transmit two different packets
  //We'll make a performance+memory compromise and write in 32 byte chunks to avoid changing the contents
  //of the original data array
  //Copy contents to SPI buff until it's full, and then write that
  //TEST: I tested this method, which uses about 0.1ms (100 microseconds) more time, but it saves us about 10% of ram.
  //I think this is a fair trade 
//  _printf("Tercer punto de control\n");
  uint8_t size = sizeof(spiBuff);
  _printf("Tercer punto de control\n");
  for (uint16_t i = 0; i < dataLen; i += size) {
    if (i + size > dataLen) { size = dataLen - i; }
    _memcpy(spiBuff,&(data[i]),size);
    _printf("Prueba 1 %s\n", spiBuff);
    SPI_transfer(spiBuff, size); //Write the payload itself
  }
 
  _printf("Cuarto punto de control\n");
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  LoraSx1262_waitForRadioCommandCompletion(1000);   //Give time for radio to process the command

  //Transmit!
  // An interrupt will be triggered if we surpass our timeout
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x83;          //Opcode for SetTx command
  spiBuff[1] = 0xFF;          //Timeout (3-byte number)
  spiBuff[2] = 0xFF;          //Timeout (3-byte number)
  spiBuff[3] = 0xFF;          //Timeout (3-byte number)
  SPI_transfer(spiBuff, 4);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  LoraSx1262_waitForRadioCommandCompletion(LoraSx1262_transmitTimeout); //Wait for tx to complete, with a timeout so we don't wait forever
  _printf("Quinto punto de control\n");
  //Remember that we are in Tx mode.  If we want to receive a packet, we need to switch into receiving mode
  unsigned int inReceiveMode = false;
}


unsigned int LoraSx1262_waitForRadioCommandCompletion(uint32_t timeout) {
  uint32_t startTime = millis();
  _printf("Tiempo 1: %d\n", startTime);
  unsigned int dataTransmitted = false;

  //Keep checking radio status until it has completed
  while (!dataTransmitted) {
    //Wait some time between spamming SPI status commands, asking if the chip is ready yet
    //Some commands take a bit before the radio even changes into a busy state,
    //so if we check too fast we might pre-maturely think we're done processing the command
    //3ms delay gives inconsistent results.  4ms seems stable.  Using 5ms to be safe
    _delay_ms(5);

    //Ask the radio for a status update
    digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
    spiBuff[0] = 0xC0;          //Opcode for "getStatus" command
    spiBuff[1] = 0x00;          //Dummy byte, status will overwrite this byte
    SPI_transfer(spiBuff, 2);
    digitalWrite(SX1262_NSS, 1); //Disable radio chip-select

    //Parse out the status (see datasheet for what each bit means)
    uint8_t chipMode = (spiBuff[1] >> 4) & 0x7;     //Chip mode is bits [6:4] (3-bits)
    uint8_t commandStatus = (spiBuff[1] >> 1) & 0x7;//Command status is bits [3:1] (3-bits)

    //Status 0, 1, 2 mean we're still busy.  Anything else means we're done.
    //Commands 3-6 = command timeout, command processing error, failure to execute command, and Tx Done (respoectively)
    if (commandStatus != 0 && commandStatus != 1 && commandStatus != 2) {
	  _printf("Transmitido1\n");
      dataTransmitted = true;
    }

    //If we're in standby mode, we don't need to wait at all
    //0x03 = STBY_XOSC, 0x02= STBY_RC
    if (chipMode == 0x03 || chipMode == 0x02) {
	  _printf("Transmitido2\n");
      dataTransmitted = true;
    }

    //Avoid infinite loop by implementing a timeout
    if (millis() - startTime >= timeout) {
		_printf("Tiemo 2 %d\n", millis());
      return false;
    }
  }
  
  _printf("WE did it\n");
  return true;
}


//Sets the radio into receive mode, allowing it to listen for incoming packets.
//If radio is already in receive mode, this does nothing.
//There's no such thing as "setModeTransmit" because it is set automatically when transmit() is called
void LoraSx1262_setModeReceive() {
  if (inReceiveMode) { return; }  //We're already in receive mode, this would do nothing
  //LO FORZAMOS SOLO A TRANSMITIR
  GPOUT &= ~(1<<1); // L_TX = 0
  GPOUT |=  (1<<0); // L_RX = 1
  
  //Set packet parameters
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x8C;          //Opcode for "SetPacketParameters"
  spiBuff[1] = 0x00;          //PacketParam1 = Preamble Len MSB
  spiBuff[2] = 0x08;          //PacketParam2 = Preamble Len LSB
  spiBuff[3] = 0x00;          //PacketParam3 = Header Type. 0x00 = Variable Len, 0x01 = Fixed Length
  spiBuff[4] = 0xFF;          //PacketParam4 = Payload Length (Max is 255 bytes)
  spiBuff[5] = 0x00;          //PacketParam5 = CRC Type. 0x00 = Off, 0x01 = on
  spiBuff[6] = 0x00;          //PacketParam6 = Invert IQ.  0x00 = Standard, 0x01 = Inverted
  SPI_transfer(spiBuff, 7);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  LoraSx1262_waitForRadioCommandCompletion(100);

  // Tell the chip to wait for it to receive a packet.
  // Based on our previous config, this should throw an interrupt when we get a packet
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x82;          //0x82 is the opcode for "SetRX"
  spiBuff[1] = 0xFF;          //24-bit timeout, 0xFFFFFF means no timeout
  spiBuff[2] = 0xFF;          // ^^
  spiBuff[3] = 0xFF;          // ^^
  SPI_transfer(spiBuff, 4);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  LoraSx1262_waitForRadioCommandCompletion(100);

  //Remember that we're in receive mode so we don't need to run this code again unnecessarily
  unsigned int  inReceiveMode = true;
}


int LoraSx1262_lora_receive_async(uint8_t* buff, int buffMaxLen) {
  LoraSx1262_setModeReceive(); //Sets the mode to receive (if not already in receive mode)
  _printf("\n\n\n");
  _printf("INICIANDO RECEPCION");
  //Radio pin DIO1 (interrupt) goes high when we have a packet ready.  If it's low, there's no packet yet
  if ((GPIN & (1<<1)) == false) { return -1; } //Return -1, meanining no packet ready
  _printf("INICIANDO RECEPCION");
  //Tell the radio to clear the interrupt, and set the pin back inactive.
  while ((GPIN & (1<<1))) {
    //Clear all interrupt flags.  This should result in the interrupt pin going low
    digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
    spiBuff[0] = 0x02;          //Opcode for ClearIRQStatus command
    spiBuff[1] = 0xFF;          //IRQ bits to clear (MSB) (0xFFFF means clear all interrupts)
    spiBuff[2] = 0xFF;          //IRQ bits to clear (LSB)
    SPI_transfer(spiBuff, 3);
    digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  }
  _printf("\n LIMPIAMOS INTERRUPCIONES");

  // (Optional) Read the packet status info from the radio.
  // This is things like radio strength, noise, etc.
  // See datasheet 13.5.3 for more info
  // This provides debug info about the packet we received
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x14;          //Opcode for get packet status
  spiBuff[1] = 0xFF;          //Dummy byte. Returns status
  spiBuff[2] = 0xFF;          //Dummy byte. Returns rssi
  spiBuff[3] = 0xFF;          //Dummy byte. Returns snd
  spiBuff[4] = 0xFF;          //Dummy byte. Returns signal RSSI
  SPI_transfer(spiBuff, 5);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select

  //Store these values as class variables so they can be accessed if needed
  //Documentation for what these variables mean can be found in the .h file
  unsigned int rssi       = -((int)spiBuff[2]) / 2;  //"Average over last packet received of RSSI. Actual signal power is –RssiPkt/2 (dBm)"
  unsigned int snr        =  ((int8_t)spiBuff[3]) / 4;   //SNR is returned as a SIGNED byte, so we need to do some conversion first
  unsigned int signalRssi = -((int)spiBuff[4]) / 2;
    
   _printf("RSSI: %d\n", rssi);
   _printf("SNR: %d\n", snr);
   _printf("Signal_RSSI: %d\n", signalRssi);
   
  //We're almost ready to read the packet from the radio
  //But first we have to know how big the packet is, and where in the radio memory it is stored
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x13;          //Opcode for GetRxBufferStatus command
  spiBuff[1] = 0xFF;          //Dummy.  Returns radio status
  spiBuff[2] = 0xFF;          //Dummy.  Returns loraPacketLength
  spiBuff[3] = 0xFF;          //Dummy.  Returns memory offset (address)
  SPI_transfer(spiBuff, 4);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select

  uint8_t payloadLen = spiBuff[2];    //How long the lora packet is
  uint8_t startAddress = spiBuff[3];  //Where in 1262 memory is the packet stored

  //Make sure we don't overflow the buffer if the packet is larger than our buffer
  if (buffMaxLen < payloadLen) {payloadLen = buffMaxLen;}

  //Read the radio buffer from the SX1262 into the user-supplied buffer
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x1E;          //Opcode for ReadBuffer command
  spiBuff[1] = startAddress;  //SX1262 memory location to start reading from
  spiBuff[2] = 0x00;          //Dummy byte
  SPI_transfer(spiBuff, 3);    //Send commands to get read started
  SPI_transfer(buff,payloadLen);  //Get the contents from the radio and store it into the user provided buffer
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _printf("Paquete: ");
  _printf("%d\n",buff);

  return payloadLen;  //Return how many bytes we actually read
}


int LoraSx1262_lora_receive_blocking(uint8_t *buff, int buffMaxLen, uint32_t timeout) {
  LoraSx1262_setModeReceive(); //Sets the mode to receive (if not already in receive mode)
 
  uint32_t startTime = millis();
  uint32_t elapsed = startTime;

  //Wait for radio interrupt pin to go high, indicating a packet was received, or if we hit our timeout
  while (digitalRead(SX1262_DIO1) == false) {
    //If user specified a timeout, check if we hit it
    if (timeout > 0) {
      elapsed = millis() - startTime;
      if (elapsed >= timeout) {
        return -1;    //Return error, saying that we hit our timeout
      }
    }
  }
  
  //If our pin went high, then we got a packet!  Return it
  return LoraSx1262_lora_receive_async(buff,buffMaxLen);
}


void LoraSx1262_updateRadioFrequency() {
  //Set PLL frequency (this is a complicated math equation.  See datasheet entry for SetRfFrequency)
  digitalWrite(SX1262_NSS, 0); //Enable radio chip-select
  spiBuff[0] = 0x86;  //Opcode for set RF Frequencty
  spiBuff[1] = (LoraSx1262_pllFrequency >> 24) & 0xFF;  //MSB of pll frequency
  spiBuff[2] = (LoraSx1262_pllFrequency >> 16) & 0xFF;  //
  spiBuff[3] = (LoraSx1262_pllFrequency >>  8) & 0xFF;  //
  spiBuff[4] = (LoraSx1262_pllFrequency >>  0) & 0xFF;  //LSB of requency
  SPI_transfer(spiBuff, 5);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100);                  //Give time for the radio to proces command
}


void LoraSx1262_updateModulationParameters() {
  /*Set modulation parameters
  # Modulation parameters are:
  #  - SpreadingFactor
  #  - Bandwidth
  #  - CodingRate
  #  - LowDataRateOptimize
  # None of these actually matter that much.  You can set them to anything, and data will still show up
  # on a radio frequency monitor.
  # You just MUST call "setModulationParameters", otherwise the radio won't work at all*/
  digitalWrite(SX1262_NSS, 0);      //Enable radio chip-select
  spiBuff[0] = 0x8B;                //Opcode for "SetModulationParameters"
  spiBuff[1] = LoraSx1262_spreadingFactor;     //ModParam1 = Spreading Factor.  Can be SF5-SF12, written in hex (0x05-0x0C)
  spiBuff[2] = LoraSx1262_bandwidth;           //ModParam2 = Bandwidth.  See Datasheet 13.4.5.2 for details. 0x00=7.81khz (slowest)
  spiBuff[3] = LoraSx1262_codingRate;          //ModParam3 = CodingRate.  Semtech recommends CR_4_5 (which is 0x01).  Options are 0x01-0x04, which correspond to coding rate 5-8 respectively
  spiBuff[4] = LoraSx1262_lowDataRateOptimize; //LowDataRateOptimize.  0x00 = 0ff, 0x01 = On.  Required to be on for SF11 + SF12
  SPI_transfer(spiBuff, 5);
  digitalWrite(SX1262_NSS, 1); //Disable radio chip-select
  _delay_ms(100);                  //Give time for radio to process the command

  //Come up with a reasonable timeout for transmissions
  //SF12 is painfully slow, so we want a nice long timeout for that,
  //but we really don't want someone using SF5 to have to wait MINUTES for a timeout
  //I came up with these timeouts by measuring how long it actually took to transmit a packet
  //at each spreading factor with a MAX 255-byte payload and 7khz Bandwitdh (the slowest one)
  switch (LoraSx1262_spreadingFactor) {
    case 12:
      LoraSx1262_transmitTimeout = 252000; //Actual tx time 126 seconds
      break;
    case 11:
      LoraSx1262_transmitTimeout = 160000; //Actual tx time 81 seconds
      break;
    case 10:
      LoraSx1262_transmitTimeout = 60000; //Actual tx time 36 seconds
      break;
    case 9:
      LoraSx1262_transmitTimeout = 40000; //Actual tx time 20 seconds
      break;
    case 8:
      LoraSx1262_transmitTimeout = 20000; //Actual tx time 11 seconds
      break;
    case 7:
      LoraSx1262_transmitTimeout = 12000; //Actual tx time 6.3 seconds
      break;
    case 6:
      LoraSx1262_transmitTimeout = 7000; //Actual tx time 3.7s seconds
      break;
    default:  //SF5
      LoraSx1262_transmitTimeout = 5000; //Actual tx time 2.2 seconds
      break;
  }
}


unsigned int LoraSx1262_configSetPreset(int preset) {
  if (preset == PRESET_DEFAULT) {
    LoraSx1262_bandwidth = 5;            //250khz
    LoraSx1262_codingRate = 1;           //CR_4_5
    LoraSx1262_spreadingFactor = 7;      //SF7
    LoraSx1262_lowDataRateOptimize = 0;  //Don't optimize (used for SF12 only)
    LoraSx1262_updateModulationParameters();
    return true;
  }

  if (preset == PRESET_LONGRANGE) {
    LoraSx1262_bandwidth = 4;            //125khz
    LoraSx1262_codingRate = 1;           //CR_4_5
    LoraSx1262_spreadingFactor = 7;      //SF7 (changed)
    LoraSx1262_lowDataRateOptimize = 0;  //Optimize for low data rate (SF12 only)
    LoraSx1262_updateModulationParameters();
    return true;
  }

  if (preset == PRESET_FAST) {
    LoraSx1262_bandwidth = 6;            //500khz
    LoraSx1262_codingRate = 1;           //CR_4_5
    LoraSx1262_spreadingFactor = 5;      //SF5
    LoraSx1262_lowDataRateOptimize = 0;  //Don't optimize (used for SF12 only)
    LoraSx1262_updateModulationParameters();
    return true;
  }

  //Invalid preset specified
  return false;
}


unsigned int LoraSx1262_configSetFrequency(long frequencyInHz) {
  //Make sure the specified frequency is in the valid range.
  if (frequencyInHz < 150000000 || frequencyInHz > 960000000) { return false;}

  //Calculate the PLL frequency (See datasheet section 13.4.1 for calculation)
  //PLL frequency controls the radio's clock multipler to achieve the desired frequency
  LoraSx1262_pllFrequency = LoraSx1262_frequencyToPLL(frequencyInHz);
  LoraSx1262_updateRadioFrequency();
  _printf("La frecuencia ha sido fijada a: %d\n", frequencyInHz);
  _printf("La frecuencia del PLL a: %d\n", LoraSx1262_pllFrequency);
  return true;
}


unsigned int LoraSx1262_configSetBandwidth(int bandwidth) {
  //Bandwidth setting must be 0-10 (excluding 7 for some reason)
  if (bandwidth < 0 || bandwidth > 0x0A || bandwidth == 7) { return false; }
  LoraSx1262_bandwidth = bandwidth;
  LoraSx1262_updateModulationParameters();
  return true;
}


unsigned int LoraSx1262_configSetCodingRate(int codingRate) {
  //Coding rate must be 1-4 (inclusive)
  if (codingRate < 1 || codingRate > 4) { return false; }
  LoraSx1262_codingRate = codingRate;
  LoraSx1262_updateModulationParameters();
  return true;
}


unsigned int LoraSx126_configSetSpreadingFactor(int spreadingFactor) {
  if (spreadingFactor < 5 || spreadingFactor > 12) { return false; }

  //The datasheet highly recommends enabling "LowDataRateOptimize" for SF11 and SF12
  LoraSx1262_lowDataRateOptimize = (spreadingFactor >= 11) ? 1 : 0;  //Turn on for SF11+SF12, turn off for anything else
  LoraSx1262_spreadingFactor = spreadingFactor;
  LoraSx1262_updateModulationParameters();
  return true;
}


uint32_t LoraSx1262_frequencyToPLL(long rfFreq) {
  /* Datasheet Says:
	 *		rfFreq = (pllFreq * xtalFreq) / 2^25
	 * Rewrite to solve for pllFreq
	 *		pllFreq = (2^25 * rfFreq)/xtalFreq
	 *
	 *	In our case, xtalFreq is 32mhz
	 *	pllFreq = (2^25 * rfFreq) / 32000000
	 */

	//Basically, we need to do "return ((1 << 25) * rfFreq) / 32000000L"
  //It's very important to perform this without losing precision or integer overflow.
  //If arduino supported 64-bit varibales (which it doesn't), we could just do this:
  //    uint64_t firstPart = (1 << 25) * (uint64_t)rfFreq;
  //    return (uint32_t)(firstPart / 32000000L);
  //
  //Instead, we need to break this up mathimatically to avoid integer overflow
  //First, we'll simplify the equation by dividing both parts by 2048 (2^11)
  //    ((1 << 25) * rfFreq) / 32000000L      -->      (16384 * rfFreq) / 15625;
  //
  // Now, we'll divide first, then multiply (multiplying first would cause integer overflow)
  // Because we're dividing, we need to keep track of the remainder to avoid losing precision
  uint32_t q = rfFreq / 15625UL;  //Gives us the result (quotient), rounded down to the nearest integer
  uint32_t r = rfFreq % 15625UL;  //Everything that isn't divisible, aka "the part that hasn't been divided yet"

  //Multiply by 16384 to satisfy the equation above
  q *= 16384UL;
  r *= 16384UL; //Don't forget, this part still needs to be divided because it was too small to divide before
  
  return q + (r / 15625UL);  //Finally divide the the remainder part before adding it back in with the quotient
}


/*

uint32_t my_millis(){
  struct timespec t ;
  clock_gettime ( CLOCK_MONOTONIC_RAW , & t ) ; // change CLOCK_MONOTONIC_RAW to CLOCK_MONOTONIC on non linux computers
  return t.tv_sec * 1000 + ( t.tv_nsec + 500000 ) / 1000000 ;
} */

