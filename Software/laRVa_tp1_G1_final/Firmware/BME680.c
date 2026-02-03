
void startBME680(){ //Inicializamos modulo BME
	//Hacemos reset de los registros del módulo
	writeBME680(0xB6,RESET_BME);

	writeBME680(0x10,STATUS_BME);
    // 1. Fijar sobremuestreo de humedad a 1x escribiendo 0b001 en osrs_h
	writeBME680(0x01,CTRL_HUM);
    // 2. Fijar sobremuestreo de temperatura a 2x escribiendo 0b010 en osrs_t
    // 3. Fijar sobremuestreo de presión a 16x escribiendo 0b101 en osrs_p
	writeBME680(0x54,CTRL_MEAS);
    // 4. Ajustar gas_wait_0 a 0x59 para seleccionar 100 ms de calentamiento
	writeBME680(0x59,gas_wait0);
    // 5. Establecer el punto de consigna del heater escribiendo la resistencia objetivo en res_heat_0
	writeBME680(0x00,res_heat0);
    // 6. Ajustar nb_conv a 0x0 para usar la configuración de heater definida
    // 7. Poner run_gas_l en 1 para habilitar la medida de gas
	writeBME680(0x10,CTRL_GAS_1);
    // 8. Poner mode en 0b01 para disparar una única medición
	writeBME680(0x55,CTRL_MEAS);
}

void writeBME680(char data,char dir){
     SPISS = CS_BME680; //Seleccionamos el chipSelect
     spixfer(0x00|dir); //Pasamos direccion
     spixfer(data); //Pasamos dato
     SPISS = 0b11; //Devolvemos a 1 chip select
}

char readBME680(char dir){ //Lectura del registro de una direccion de memoria concreta
    unsigned char readDATA;
    SPISS = CS_BME680;//Seleccionamos chip select
	
    spixfer(0x80|dir); //Lectura del SPI
    readDATA = spixfer(0); //Enviamos 0
    SPISS=0b11;
    return readDATA;
}

int returnTemp(){

    int par_t1, par_t2, par_t3, temp_adc, var1, var2, var3, t_fine, temp_comp;

	writeBME680(0x00,STATUS_BME); //entramos a la página 0 de los registros

    par_t1 = (int32_t)((readBME680(0xEA) << 8) | readBME680(0xE9));
    par_t2 = (int32_t)((readBME680(0x8B) << 8) | readBME680(0x8A));
    par_t3 = (int32_t)readBME680(0x8C);
	
	writeBME680(0x10,STATUS_BME); //entramos a la página 1 de los registros
    temp_adc = (int32_t)((readBME680(0x22) << 12) | (readBME680(0x23) << 4) | (readBME680(0x24) >> 4));

    var1 = ((int32_t)temp_adc >> 3) - ((int32_t)par_t1 << 1);
    var2 = (var1 * (int32_t)par_t2) >> 11;
    var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12) * ((int32_t)par_t3 << 4)) >> 14;
    t_fine = var2 + var3;
    temp_comp = ((t_fine * 5) + 128) >> 8;
	
    return temp_comp;
}
int returnPressure(){

    int par_p1, par_p2, par_p3, par_p4, par_p5, par_p6, par_p7, par_p8, par_p9, par_p10, press_adc,press_comp;
    int var1, var2, var3, t_fine;
	
	writeBME680(0x00,STATUS_BME); //entramos a la página 0 de los registros

    par_p1 = (int32_t)((readBME680(0x8F) << 8) | readBME680(0x8E));
    par_p2 = (int32_t)((readBME680(0x91) << 8) | readBME680(0x90));
    par_p3 = (int32_t)readBME680(0x92);
    par_p4 = (int32_t)((readBME680(0x95) << 8) | readBME680(0x94));
    par_p5 = (int32_t)((readBME680(0x97) << 8) | readBME680(0x96));
    par_p6 = (int32_t)readBME680(0x99);
    par_p7 = (int32_t)readBME680(0x98);
    par_p8 = (int32_t)((readBME680(0x9D) << 8) | readBME680(0x9C));
    par_p9 = (int32_t)((readBME680(0x9F) << 8) | readBME680(0x9E));
    par_p10 = (int32_t)readBME680(0xA0);
	
	writeBME680(0x10,STATUS_BME); //entramos a la página 1 de los registros
    press_adc = (int32_t)((readBME680(0x1F) << 12) | (readBME680(0x20) << 4) | (readBME680(0x21) >> 4));

    var1 = ((int32_t)t_fine >> 1) - 64000;
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)par_p6) >> 2;
    var2 = var2 + ((var1 * (int32_t)par_p5) << 1);
    var2 = (var2 >> 2) + ((int32_t)par_p4 << 16);
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)par_p3 << 5)) >> 3) + (((int32_t)par_p2 * var1) >> 1);
    var1 = var1 >> 18;
    var1 = ((32768 + var1) * (int32_t)par_p1) >> 15;
    press_comp = 1048576 - press_adc;
    press_comp = (uint32_t)((press_comp - (var2 >> 12)) * ((uint32_t)3125));
    if (press_comp >= (1 << 30)) press_comp = ((press_comp / (uint32_t)var1) << 1);
    else press_comp = ((press_comp << 1) / (uint32_t)var1);
    var1 = ((int32_t)par_p9 * (int32_t)(((press_comp >> 3) * (press_comp >> 3)) >> 13)) >> 12;
    var2 = ((int32_t)(press_comp >> 2) * (int32_t)par_p8) >> 13;
    var3 = ((int32_t)(press_comp >> 8) * (int32_t)(press_comp >> 8) * (int32_t)(press_comp >> 8) * (int32_t)par_p10) >> 17;
    press_comp = (int32_t)(press_comp) + ((var1 + var2 + var3 + ((int32_t)par_p7 << 7)) >> 4);

    return press_comp;
}

int returnHUMIDITY(int temp_comp){
    int par_h1, par_h2, par_h3, par_h4, par_h5, par_h6, par_h7, hum_adc,temp_scaled, hum_comp;
	int var1, var2, var3, var4, var5, var6;
	
	writeBME680(0x00,STATUS_BME); //entramos a la página 0 de los registros
	
    par_h1 = (int32_t)((readBME680(0xE3) << 4) | (readBME680(0xE2) & 0b00001111));
    par_h2 = (int32_t)((readBME680(0xE1) << 4) | (readBME680(0xE2) >> 4));
    par_h3 = (int32_t)readBME680(0xE4);
    par_h4 = (int32_t)readBME680(0xE5);
    par_h5 = (int32_t)readBME680(0xE6);
    par_h3 = (int32_t)readBME680(0xE7);
    par_h3 = (int32_t)readBME680(0xE8);
	
	writeBME680(0x10,STATUS_BME); //entramos a la página 1 de los registros
    hum_adc = (int32_t)((readBME680(0x25) << 8) | readBME680(0x26));

    temp_scaled = (int32_t)temp_comp;
    var1 = (int32_t)hum_adc - (int32_t)((int32_t)par_h1 << 4) - (((temp_scaled * (int32_t)par_h3) / ((int32_t)100)) >> 1);
    var2 = ((int32_t)par_h2 * (((temp_scaled * (int32_t)par_h4) / ((int32_t)100)) + (((temp_scaled * ((temp_scaled * (int32_t)par_h5) / ((int32_t)100))) >> 6) / ((int32_t)100)) + ((int32_t)(1 << 14)))) >> 10;
    var3 = var1 * var2;
    var4 = (((int32_t)par_h6 << 7) + ((temp_scaled * (int32_t)par_h7) / ((int32_t)100))) >> 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;
    hum_comp = (((var3 + var6) >> 10) * ((int32_t) 1000)) >> 12;

    return hum_comp;
}