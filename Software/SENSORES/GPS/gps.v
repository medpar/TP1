#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Estas inclusiones asumen que has configurado las rutas de búsqueda (Paso 3 en Code::Blocks)
#include <nmea.h>
#include <nmea/gpgll.h>

int main(void)
{
	// Trama de ejemplo a ser decodificada:
    // GPGLL (Geographic Position, Lat/Lon)
    // 4916.45, N, 12311.12, W, 225444, A, *1D
    // Latitud (49° 16.45' N), Longitud (123° 11.12' W), Hora (22:54:44), Estado (A=Válido)
	char sentence[] = "$GPGLL,4916.45,N,12311.12,W,225444,A,*1D\r\n";

	printf("Iniciando análisis de la trama NMEA: %s", sentence);

	// Puntero a la estructura que contendrá los datos decodificados.
	nmea_s *data;

	// 1. Decodificar la trama. El tercer argumento (0) suele ser para opciones/errores.
	data = nmea_parse(sentence, strlen(sentence), 0);


	if(NULL == data) {
		printf("Error: Falló el análisis de la trama! (Verifique el checksum o el formato)\n");
		return -1;
	}

	// 2. Comprobar que el tipo de trama decodificada sea GPGLL
	if (NMEA_GPGLL == data->type) {
		// Castear el puntero genérico 'data' al tipo específico para GPGLL
		nmea_gpgll_s *gpgll = (nmea_gpgll_s *) data;

		printf("\n--- Sentencia GPGLL Decodificada ---\n");

		// 3. Imprimir Latitud y Longitud
		printf("Latitud:\n");
		printf("  Grados: %d\n", gpgll->latitude.degrees);
		printf("  Minutos: %f\n", gpgll->latitude.minutes);
		printf("  Cardinal: %c\n", (char) gpgll->latitude.cardinal);

		printf("Longitud:\n");
		printf("  Grados: %d\n", gpgll->longitude.degrees);
		printf("  Minutos: %f\n", gpgll->longitude.minutes);
		printf("  Cardinal: %c\n", (char) gpgll->longitude.cardinal);

		// Datos adicionales
		printf("Hora UTC: %02d:%02d:%02d\n",
		       gpgll->time.hour, gpgll->time.min, gpgll->time.sec);
		printf("Estado: %c (%s)\n", (char) gpgll->status,
		       ((char)gpgll->status == 'A') ? "Válido" : "Advertencia/Inválido");
	}

	// 4. Liberar la memoria asignada por la función de parsing
	nmea_free(data);

	return 0;
}
