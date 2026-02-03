//////////////////////////////////////////////////////////////////
//	TP1 - Sistemas electrónicos									//
//	Grupo 1:												  	//
//  Clara Ruiz de las Heras, Mario Medrano Paredes,				//
//  Miguel Barrigón Gómez, Víctor Sánchez Valencia			    //
//////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stddef.h>

extern int _printf(const char *format, ...);

// Copia segura de cadenas con terminador nulo
static void copia_strings(char *dst, int dst_len, const char *src)
{
	if (dst == NULL || dst_len <= 0 || src == NULL) return;
	int i;
	for (i = 0; i < dst_len - 1 && src[i]; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

// Espera un byte en UART1 con timeout
static int uart1_getch_timeout(uint32_t timeout_ms, char *out)
{
	uint32_t start = TIMER;
	uint32_t ticks = (CCLK / 1000u) * timeout_ms;

	// Espera un byte en UART1 con timeout
	while ((UART1STA & 1) == 0) {
		if ((uint32_t)(TIMER - start) > ticks) return 0;
	}
	*out = (char)UART1DAT;
	return 1;
}

// Verifica si una cadena comienza con un prefijo
static int prefijo_strings(const char *s, const char *prefix)
{
	while (*prefix && *s && *s == *prefix) {
		++s;
		++prefix;
	}
	return *prefix == '\0';
}

// Extrae el siguiente token de una cadena separado por comas
static char *siguiente_token(char **ctx)
{
	char *p = *ctx;
	if (p == NULL) return NULL;
	char *start = p;
	while (*p && *p != ',') p++;
	if (*p == ',') {
		*p = '\0';
		*ctx = p + 1;
	} else {
		*ctx = NULL;
	}
	return start;
}

// Analiza una trama GGA y extrae lat/lon
static int procesa_gga(char *line, char *lat, int lat_len, char *lon, int lon_len, char *lat_dir, char *lon_dir)
{
	// Acepta tramas GGA y extrae lat/lon en formato NMEA
	if (!(prefijo_strings(line, "$GNGGA") || prefijo_strings(line, "$GPGGA"))) {
		return 0;
	}

	char *ctx = line;
	(void)siguiente_token(&ctx);           // $GNGGA
	char *utc   = siguiente_token(&ctx);   // hhmmss
	(void)utc;
	char *lat_f   = siguiente_token(&ctx);   // latitud
	char *lat_d   = siguiente_token(&ctx);   // N/S
	char *lon_f   = siguiente_token(&ctx);   // longitud
	char *lon_d   = siguiente_token(&ctx);   // E/W

	if (!lat_f || !lat_d || !lon_f || !lon_d) {
		return 0;
	}

	if (lat && lat_len > 0) copia_strings(lat, lat_len, lat_f);
	if (lon && lon_len > 0) copia_strings(lon, lon_len, lon_f);
	if (lat_dir) *lat_dir = lat_d[0];
	if (lon_dir) *lon_dir = lon_d[0];
	return 1;
}

// Lee una linea NMEA desde UART1
static int lee_nmea(char *line, int line_len)
{
	char c;
	// Sincroniza con '$' y lee hasta fin de linea o timeout
	if (!uart1_getch_timeout(200u, &c)) return 0;
	if (c != '$') return 0;

	line[0] = '$';
	int idx = 1;
	while (idx < line_len - 1) {
		if (!uart1_getch_timeout(200u, &c)) break;
		line[idx++] = c;
		if (c == '\n' || c == '\r') break;
	}
	line[idx] = '\0';
	return 1;
}

// Convierte ddmm.mmmm a microgrados (1e-6)
static int nmea_a_udeg(const char *nmea, char dir, int *out_micro)
{
	if (!nmea || !out_micro || nmea[0] == '\0') return 0;

	int int_part = 0;
	int frac_part = 0;
	int frac_mul = 1;
	const char *p = nmea;
	while (*p && *p != '.') {
		if (*p < '0' || *p > '9') return 0;
		int_part = int_part * 10 + (*p - '0');
		p++;
	}
	if (*p == '.') {
		p++;
		while (*p && frac_mul < 10000) { // hasta 4 decimales
			if (*p < '0' || *p > '9') break;
			frac_part = frac_part * 10 + (*p - '0');
			frac_mul *= 10;
			p++;
		}
		while (frac_mul < 10000) { frac_part *= 10; frac_mul *= 10; }
	}

	int deg = int_part / 100;
	int minutes = int_part % 100;
	int minutes_milli = minutes * 10000 + frac_part; // mm.mmmm escalado

	long micro = (long)deg * 1000000L;
	micro += (long)minutes_milli * 5L / 3L; // (1e6/600000) = 5/3

	int sign = (dir == 'S' || dir == 's' || dir == 'W' || dir == 'w') ? -1 : 1;
	*out_micro = (int)(sign * micro);
	return 1;
}


// Lee la latitud y longitud desde UART1
int GPS_lee_latlon(char *lat, int lat_len, char *lon, int lon_len)
{
	char line[128];
	uint32_t start_all = TIMER;
	uint32_t timeout_all = (CCLK / 1000u) * 1500u; // 1.5s

	// Busca una GGA valida dentro del tiempo limite
	while ((uint32_t)(TIMER - start_all) < timeout_all) {
		if (!lee_nmea(line, (int)sizeof(line))) continue;
		char work[128];
		copia_strings(work, (int)sizeof(work), line);
		if (procesa_gga(work, lat, lat_len, lon, lon_len, NULL, NULL)) {
			return 1;
		}
	}
	return 0;
}


// Lee la latitud y longitud desde UART1 en microgrados
int GPS_lee_latlon_micro(int *lat_micro, int *lon_micro)
{
	char line[128];
	char lat[24] = {0};
	char lon[24] = {0};
	char lat_dir = 'N';
	char lon_dir = 'E';
	uint32_t start_all = TIMER;
	uint32_t timeout_all = (CCLK / 1000u) * 1500u; // 1.5s

	// Igual que GPS_lee_latlon pero devuelve en microgrados con signo
	while ((uint32_t)(TIMER - start_all) < timeout_all) {
		if (!lee_nmea(line, (int)sizeof(line))) continue;
		char work[128];
		copia_strings(work, (int)sizeof(work), line);
		if (procesa_gga(work, lat, (int)sizeof(lat), lon, (int)sizeof(lon), &lat_dir, &lon_dir)) {
			if (nmea_a_udeg(lat, lat_dir, lat_micro) &&
			    nmea_a_udeg(lon, lon_dir, lon_micro)) {
				return 1;
			}
		}
	}
	return 0;
}

// Lee la latitud y longitud desde UART1 y la imprime
void lee_GPS(void)
{
	char line[128];
	char lat[24] = {0};
	char lon[24] = {0};
	char lat_dir = '?';
	char lon_dir = '?';
	int lat_micro = 0;
	int lon_micro = 0;

	uint32_t start_all = TIMER;
	uint32_t timeout_all = (CCLK / 1000u) * 3000u; // 3s

	// Imprime tramas y, si hay GGA valida, muestra lat/lon
	while ((uint32_t)(TIMER - start_all) < timeout_all) {
		if (!lee_nmea(line, (int)sizeof(line))) continue;

		// Imprime la trama en su propia linea (evita solapes)
		_printf("%s\n", line);
		// Analiza lat/lon si es una GGA
		char work[128];
		copia_strings(work, (int)sizeof(work), line);
		procesa_gga(work, lat, (int)sizeof(lat), lon, (int)sizeof(lon), &lat_dir, &lon_dir);
	}

	if (lat[0] && lon[0]) {
		_printf("Lat: %s %c\n", lat, lat_dir);
		_printf("Lon: %s %c\n", lon, lon_dir);
		if (nmea_a_udeg(lat, lat_dir, &lat_micro) &&
		    nmea_a_udeg(lon, lon_dir, &lon_micro)) {
			int lat_abs = (lat_micro < 0) ? -lat_micro : lat_micro;
			int lon_abs = (lon_micro < 0) ? -lon_micro : lon_micro;
			_printf("Lat dec: %d.%06d\n", lat_micro/1000000, lat_abs%1000000);
			_printf("Lon dec: %d.%06d\n", lon_micro/1000000, lon_abs%1000000);
			//_printf("Coordenadas decimales: %d, %d\n", lat_micro/1000000, lon_micro/1000000);
		}
	} else {
		_printf("GPS: No hay datos GGA validos\n");
	}
}
