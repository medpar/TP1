#pragma once

#include <stdint.h>

#define GP2Y1010_LED_ON_US 320u
#define GP2Y1010_SAMPLE_US 280u

int GP2Y1010_ReadRaw(uint8_t channel);
