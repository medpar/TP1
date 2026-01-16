#include <stdint.h>
#include <stddef.h>

extern int _printf(const char *format, ...);

static void copy_field(char *dst, int dst_len, const char *src)
{
	if (dst == NULL || dst_len <= 0 || src == NULL) return;
	int i;
	for (i = 0; i < dst_len - 1 && src[i]; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

static int uart1_getch_timeout(uint32_t timeout_ms, char *out)
{
	uint32_t start = TIMER;
	uint32_t ticks = (CCLK / 1000u) * timeout_ms;

	while ((UART1STA & 1) == 0) {
		if ((uint32_t)(TIMER - start) > ticks) return 0;
	}
	*out = (char)UART1DAT;
	return 1;
}

static int str_starts_with(const char *s, const char *prefix)
{
	while (*prefix && *s && *s == *prefix) {
		++s;
		++prefix;
	}
	return *prefix == '\0';
}

static char *next_token(char **ctx)
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

static int parse_gga(char *line, char *lat, int lat_len, char *lon, int lon_len, char *lat_dir, char *lon_dir)
{
	if (!(str_starts_with(line, "$GNGGA") || str_starts_with(line, "$GPGGA"))) {
		return 0;
	}

	char *ctx = line;
	(void)next_token(&ctx);           // $GNGGA
	char *utc   = next_token(&ctx);   // hhmmss (unused here)
	(void)utc;
	char *lat_f   = next_token(&ctx);   // latitude
	char *lat_d   = next_token(&ctx);   // N/S
	char *lon_f   = next_token(&ctx);   // longitude
	char *lon_d   = next_token(&ctx);   // E/W

	if (!lat_f || !lat_d || !lon_f || !lon_d) {
		return 0;
	}

	if (lat && lat_len > 0) copy_field(lat, lat_len, lat_f);
	if (lon && lon_len > 0) copy_field(lon, lon_len, lon_f);
	if (lat_dir) *lat_dir = lat_d[0];
	if (lon_dir) *lon_dir = lon_d[0];
	return 1;
}

static int read_nmea_line(char *line, int line_len)
{
	char c;
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

static int nmea_to_microdeg(const char *nmea, char dir, int *out_micro)
{
	(void)nmea; (void)dir; (void)out_micro;
	return 0;
}

int GPS_ReadLatLon(char *lat, int lat_len, char *lon, int lon_len)
{
	char line[128];
	uint32_t start_all = TIMER;
	uint32_t timeout_all = (CCLK / 1000u) * 1500u; // 1.5s

	while ((uint32_t)(TIMER - start_all) < timeout_all) {
		if (!read_nmea_line(line, (int)sizeof(line))) continue;
		char work[128];
		copy_field(work, (int)sizeof(work), line);
		if (parse_gga(work, lat, lat_len, lon, lon_len, NULL, NULL)) {
			return 1;
		}
	}
	return 0;
}

void readGPS(void)
{
	char line[128];
	char lat[24] = {0};
	char lon[24] = {0};
	char lat_dir = '?';
	char lon_dir = '?';

	uint32_t start_all = TIMER;
	uint32_t timeout_all = (CCLK / 1000u) * 3000u; // 3s

	while ((uint32_t)(TIMER - start_all) < timeout_all) {
		if (!read_nmea_line(line, (int)sizeof(line))) continue;

		// Imprime la trama en su propia linea (evita solapes)
		_printf("%s\n", line);
		// Analiza lat/lon si es una GGA
		char work[128];
		copy_field(work, (int)sizeof(work), line);
		parse_gga(work, lat, (int)sizeof(lat), lon, (int)sizeof(lon), &lat_dir, &lon_dir);
	}

	if (lat[0] && lon[0]) {
		_printf("Lat: %s %c\n", lat, lat_dir);
		_printf("Lon: %s %c\n", lon, lon_dir);
	} else {
		_printf("GPS: sin datos GGA validos\n");
	}
}
