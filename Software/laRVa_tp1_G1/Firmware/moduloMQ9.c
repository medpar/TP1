
void readGas(){

	estado=0;	//Reset
	ENABLE_5V;	//habilitamos 5V
	IRQEN = 0b00000100;	//habilitamos la interrupción del timer
	TIMER = 1 * 1000000; //60s 
	
}

void readCO(){
	
	int gas_CO=1;
		
	gas_CO = readMCP3004(MCP3004_CH0); //leemos de ADC CH0	- AQUI ESTA EL ERROR
	//_printf("gas_CO: %d\n", gas_CO);	//LECTURA ERRONEA no permite leer
	DISABLE_5V_1V4;	//deshabilitamos 5V
	ENABLE_1V4; //habilitamos 1V4
	
	IRQEN = 0b00000100;	//habilitamos la interrupción del timer
	TIMER = 1 * 1000000; //90s 
	
	estado = 1; //Actualizamos estado
	
	lectura_CO = (10*(330*gas_CO)) / (1024*7);
	_printf("Medicion de C0 en curso, por favor espere 60 seg.\n"); 
	_printf("Lectura CO: %d ppm \n", lectura_CO);

	_printf("Medicion de CH4 en curso, por favor espere 90 seg.\n");
	
}

void readCH4(){
	
	int gas_CH4=1;
	
	gas_CH4 = readMCP3004(MCP3004_CH0); //leemos de ADC
	//_printf("gas_CH4: %d ppm\n", gas_CH4);
	
	DISABLE_5V_1V4;	//deshabilitamos 5V
	
	estado = 2; //Actualizamos estado
	
	lectura_CH4 = 10*(330*gas_CH4) / (1024);	
	//_printf("Estado: %d\n", estado);
	_printf("Lectura CH4: %d ppm \n", lectura_CH4); 

}