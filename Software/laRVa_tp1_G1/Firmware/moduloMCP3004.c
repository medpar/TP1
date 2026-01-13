int readMCP3004(unsigned char channel){
	int aux1, aux2; 			// Variables auxiliares para guardar la información
	uint32_t aux=0;

	// Revisar si funciona
    SPISS = ADC_CS;
    spixfer(MCP3004_START); 	// Byte de inicio (0b00000001)
	aux1 = spixfer(channel); 	// Byte de seleccion de canal
	aux2 = spixfer(0x00);			// Dummy byte para seguir recibiendo información
	SPISS = 0b11;
	
	aux = aux1 << 8;
	aux|= aux2;
	
	return (aux&=0b0000001111111111);

	//return ((aux1 & 0b111)<<8) | aux2;
}
