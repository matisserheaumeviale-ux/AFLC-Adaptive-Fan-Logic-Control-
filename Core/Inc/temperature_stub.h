#pragma once

#include <stdint.h>

#define TEMPERATURE_CHANNEL_COUNT 4U

typedef struct {
    uint8_t values_c[TEMPERATURE_CHANNEL_COUNT];
} TemperatureSnapshot_t;

void TemperatureStub_Init(void);
void TemperatureStub_Task(uint32_t now_ms);
const TemperatureSnapshot_t *TemperatureStub_GetSnapshot(void);
