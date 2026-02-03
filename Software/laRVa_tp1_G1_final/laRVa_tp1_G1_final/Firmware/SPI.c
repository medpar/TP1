
/*	------ 

    SPI Control:   bits 31-14  bits 13-8  bits 7-0 
                      xxxx        DLEN     DIVIDER 

        DLEN:    Data lenght (8 to 32 bits) 
        DIVIDER: SCK frequency = Fclk / (2*(DIVIDER+1)) 

    SPI Flags:     bits 31-1  bit 0
                      xxxx     BUSY 

        BUSY:  SPI exchanging data when 1 

    SPI Slave Select: bits 31-2  bit 1   bit 0 
                         xxxx     ss1     ss0 

        ss0 : Selects the SPI slave 0 when 0 (active low) 
        ss1 : Selects the SPI slave 1 when 0 (active low) 

    ------ */

#define FREQ_SPI 1000000

#define DIV_SPI 	((CCLK/(2*FREQ_SPI))-1) 			// Calculamos el divisor para que la frecuencia sea 1MHz


// --------------------------------------------------------
// SPI
unsigned char spixfer (unsigned char d)
{
	SPIDAT=d;
	while(SPISTA&1);
	return SPIDAT;
}

// SPI1
unsigned char spixfer1 (unsigned char d)
{
	SPI1DAT=d;
	while(SPI1STA&1);
	return SPI1DAT;
}
// --------------------------------------------------------