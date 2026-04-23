#pragma once

#include "Profil.h"
#include "tachometer.h"
#include "temperature_stub.h"

#include <stdint.h>

typedef struct {
    uint16_t target_rpm[PROFIL_FAN_COUNT];
} AFLCalcul_Output_t;

void AFLCalcul_Compute(const Profil_Result_t *profiles,
                       const TemperatureSnapshot_t *temps,
                       const TachometerSnapshot_t *tach,
                       AFLCalcul_Output_t *out);
