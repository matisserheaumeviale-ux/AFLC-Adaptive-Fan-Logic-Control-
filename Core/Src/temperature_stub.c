#include "temperature_stub.h"

// Stub temporaire.
// Plus tard, ce module sera remplace par de vraies sondes.
static TemperatureSnapshot_t g_temperatures;

void TemperatureStub_Init(void)
{
    // Valeurs de depart.
    g_temperatures.values_c[0] = 28U;
    g_temperatures.values_c[1] = 31U;
    g_temperatures.values_c[2] = 34U;
    g_temperatures.values_c[3] = 37U;
}

void TemperatureStub_Task(uint32_t now_ms)
{
    uint8_t phase = (uint8_t)((now_ms / 2000UL) % 4UL);

    // Petite variation cyclique pour simuler une charge thermique.
    g_temperatures.values_c[0] = (uint8_t)(28U + phase);
    g_temperatures.values_c[1] = (uint8_t)(31U + (phase / 2U));
    g_temperatures.values_c[2] = (uint8_t)(34U + phase);
    g_temperatures.values_c[3] = (uint8_t)(37U + (phase / 2U));
}

const TemperatureSnapshot_t *TemperatureStub_GetSnapshot(void)
{
    return &g_temperatures;
}
