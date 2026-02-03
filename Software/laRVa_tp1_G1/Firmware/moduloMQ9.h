
#define EN_5V_UP   (1U << 7)
#define EN_5V_M4   (1U << 6)
#define EN_1V4_M4  (1U << 5)

#define ENABLE_5V       (GPOUT = (GPOUT & ~EN_1V4_M4) | EN_5V_UP | EN_5V_M4)
#define ENABLE_1V4      (GPOUT = (GPOUT & ~(EN_5V_UP | EN_5V_M4)) | EN_1V4_M4)
#define DISABLE_5V_1V4  (GPOUT &= ~(EN_5V_UP | EN_5V_M4 | EN_1V4_M4))

int MQ9_ReadCOppm(uint8_t channel);
int MQ9_ReadCH4ppm(uint8_t channel);
void readGas();
void readCO();
void readCH4();
