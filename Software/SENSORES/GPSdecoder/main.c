/* Definición para evitar problemas con timespec en entornos Windows/MinGW */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "minmea.h"

// Función para procesar y mostrar los datos de las sentencias GLL
void procesar_gll(const char *linea) {
    struct minmea_sentence_gll frame;

    // minmea_parse_gll extrae los datos y verifica el checksum
    if (minmea_parse_gll(&frame, linea)) {
        // El campo 'status' indica si la señal es válida ('A') o no ('V')
        if (frame.status == MINMEA_GLL_STATUS_DATA_VALID) {

            // Convertimos las coordenadas de punto fijo a grados decimales
            float lat = minmea_tocoord(&frame.latitude);
            float lon = minmea_tocoord(&frame.longitude);

            printf("[GLL DETECTADA]\n");
            printf("  Latitud:  %f\n", lat);
            printf("  Longitud: %f\n", lon);
            printf("  Hora UTC: %02d:%02d:%02d\n",
                    frame.time.hours, frame.time.minutes, frame.time.seconds);
        } else {
            printf("[GLL] Senial no valida actualmente (Status V).\n");
        }
    } else {
        printf("[GLL] Error de parseo o Checksum incorrecto.\n");
    }
}

int main() {
    // Array con las líneas de ejemplo que proporcionaste
    const char *rafaga[] = {
    "$GPGLL,5106.94086,N,17.01517,E,123210.00,A,A*5F",
    "$GPGLL,5106.94086,N,17.01517,E,123210.00,A,A*5F",
     "$GPTXT,01,01,02,ANTSTATUS=INIT*26",
    "$GPRMC,,V,,,,,,,,,,N*532",
    "$GPVTG,,,,\xff,,,,,N*30",
    "$$GPGGA,,,,,,0,00,99.99,,,,,,*48",
    "GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*30",
    "$GPGLL,,,,,,V,N",
    "$GPXTE,A,A,0.67,L,N*6e",
    "$GPXTE,A,A,0.67,L,N*6g",
    "$GPTXT,hello\n ",
    "$GPTXT,hello\r*24",
    "$GPTXT,hello\r\n$",
    "$GPGLL,5107.54086,N,17.91517,E,123213.00,A,A*58"

    };

    int num_lineas = sizeof(rafaga) / sizeof(rafaga[0]);

    printf("Iniciando decodificador de sentencias GLL...\n");
    printf("-------------------------------------------\n");
    int i;
    for (i = 0; i < num_lineas; i++) {
        // minmea_sentence_id identifica si la línea es GLL, GGA, RMC, etc.
        if (minmea_sentence_id(rafaga[i], false) == MINMEA_SENTENCE_GLL) {
            procesar_gll(rafaga[i]);
        }
    }

    return 0;
}
