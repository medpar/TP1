#include <stdint.h>
#include <stddef.h>

extern int _printf(const char *format, ...);

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

void readGPS(void)
{
	char line[120];
	uint32_t start_all = TIMER;
	uint32_t timeout_all = (CCLK / 1000u) * 3000u; // 3s

	while ((uint32_t)(TIMER - start_all) < timeout_all) {
		char c;
		if (!uart1_getch_timeout(200u, &c)) {
			continue;
		}
		if (c != '$') continue;

		line[0] = '$';
		int idx = 1;
		while (idx < (int)(sizeof(line) - 1)) {
			if (!uart1_getch_timeout(200u, &c)) break;
			line[idx++] = c;
			if (c == '\n' || c == '\r') break;
		}
		line[idx] = '\0';

		if (!(str_starts_with(line, "$GNGGA") || str_starts_with(line, "$GPGGA"))) {
			continue;
		}

		char *ctx = line;
		(void)next_token(&ctx);           // $GNGGA
		char *utc   = next_token(&ctx);   // hhmmss
		char *lat   = next_token(&ctx);   // latitude
		char *lat_d = next_token(&ctx);   // N/S
		char *lon   = next_token(&ctx);   // longitude
		char *lon_d = next_token(&ctx);   // E/W

		if (!lat || !lat_d || !lon || !lon_d) {
			_printf("GPS: trama GGA incompleta\n");
			return;
		}

		_printf("GPS UTC %c%c:%c%c:%c%c\n",
		        utc && utc[0] ? utc[0] : '0', utc && utc[1] ? utc[1] : '0',
		        utc && utc[2] ? utc[2] : '0', utc && utc[3] ? utc[3] : '0',
		        utc && utc[4] ? utc[4] : '0', utc && utc[5] ? utc[5] : '0');
		_printf("Lat: %s %s\n", lat, lat_d);
		_printf("Lon: %s %s\n", lon, lon_d);
		return;
	}

	_printf("GPS: sin datos GGA\n");
}
