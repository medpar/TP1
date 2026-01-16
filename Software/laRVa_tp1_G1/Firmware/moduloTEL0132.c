uint8_t tramaGPS[1000];

static int uart1_getch_timeout(uint32_t timeout_ms, char *out)
{
	uint32_t start = TIMER;
	uint32_t ticks = (CCLK / 1000u) * timeout_ms;

	while ((UART1STA & 1) == 0) {
		if ((uint32_t)(TIMER - start) > ticks) {
			return 0;
		}
	}
	*out = (char)UART1DAT;
	return 1;
}

uint8_t _getch1()
{
	while((UART1STA&1)==0);
	return UART1DAT;
}

void readGPS(){	
	char cadenaUart[80];
	char buffer;
	char idAux[7];
	int i=0;
	int j, a; 
	char idSearch[] = "$GNGGA"; // Entrada buscada
	uint32_t start = TIMER;
	uint32_t timeout_ticks = (CCLK / 1000u) * 2000u;
		 
	while(1){
		if (!uart1_getch_timeout(2000u, &buffer)) {
			_printf("\nGPS sin datos");
			return;
		}
		// Recojo char de la UART1	
		if(buffer == '$'){ //Indica inicio trama
		
			cadenaUart[0] ='$';
			idAux[0] = '$';
			for (i = 1; i < 80; i++){
				if (!uart1_getch_timeout(2000u, &cadenaUart[i])) {
					_printf("\nGPS sin datos");
					return;
				}
				if(cadenaUart[i] == '\n'){
					cadenaUart[i] = '\0';
					break; //Llegamos al final de una trama

				}
				
			}
			idAux[1] = cadenaUart[1];
			idAux[2] = cadenaUart[2];
			idAux[3] = cadenaUart[3];
			idAux[4] = cadenaUart[4];
			idAux[5] = cadenaUart[5];
			idAux[6] = '\0';
			
			if(myStrcmp(idAux, idSearch) == 0){
				break;				
			}
			_printf("\nNo se ha encontrado la trama buscada");
			
		}	
		if ((uint32_t)(TIMER - start) > timeout_ticks) {
			_printf("\nGPS sin datos");
			return;
		}
	} 
	_printf("\nLa trama total recibida es: %s", cadenaUart);
	procesaCadena(cadenaUart, ',' ); 
		
}

int myStrcmp(char *s1, char *s2) {

    while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    if (*s1 == *s2) {
        return 0;  /* Son cadenas iguales*/
    }
     else {
        return 1;  /*Son cadenas diferentes*/
    }
}

void procesaCadena(const char *cadena, const char delimitador ) {
    char token[50]; 
    int a = 0;
    while (*cadena) {
        int i = 0;
        while (*cadena != delimitador && *cadena != '\0') {
            token[i++] = *cadena++;
        }
        token[i] = '\0';
        switch(a) { /*Identificamos cada componente de la trama y enviamos*/
            case 0:
                _printf("Identificador: %s\n", token);
                break;
			case 1:
                _printf("UTC: %c%c:%c%c:%c%c\n", token[0], token[1], token[2], token[3], token[4], token[5]);
                break;
            case 2:
                 _printf("Latitud: %c%c.%c%c%c%c%c%c%c\n", token[0], token[1], token[2], token[3], token[5], token[6], token[7], token[8], token[9]);
                break;
            case 3:
                _printf("Dir. Latitud: %s\n", token);
                break;
            case 4:
				//_printf("Entero. Longitud: %s\n", token);
                _printf("Longitud: %c%c.%c%c%c%c%c%c%c \n", token[1], token[2], token[3], token[4], token[6], token[7], token[8], token[9], token[10]);
                break;
            case 5: 
                _printf("Dir. Longitud: %s\n", token);
                break;
            case 6:
                _printf("Status: %s\n", token);
                break;
            case 7:
                _printf("Satellites Used: %s\n", token);
                break;
			/* case 8:
                _printf("HDOP : %s\n", token);
                break; */
			case 9:
                _printf("Altitud : %s\n", token);
                break;
			/* case 10:
                _printf("Altura Elipsoide : %s\n", token);
                break;
			case 11:
                _printf("Tiempo Diferencial : %s\n", token);
                break;	
            default:
                _printf("Opcion no valida.\n"); */
			default:
				break;
        }
        a++;
        if (*cadena != '\0') {
            cadena++;  /* Saltar el delimitador*/
        }
    }
}
