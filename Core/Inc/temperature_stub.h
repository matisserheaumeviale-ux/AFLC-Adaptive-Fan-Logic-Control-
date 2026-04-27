#pragma once

#include <stdint.h>

// Pour le moment on simule 4 temperatures.
#define TEMPERATURE_CHANNEL_COUNT 4U

// Snapshot simple des temperatures en degres C.
typedef struct {
    uint8_t values_c[TEMPERATURE_CHANNEL_COUNT];
} TemperatureSnapshot_t;

void TemperatureStub_Init(void);

// Fait varier les temperatures pour tester la regulation.
void TemperatureStub_Task(uint32_t now_ms);

// Retourne la derniere valeur.
const TemperatureSnapshot_t *TemperatureStub_GetSnapshot(void);
