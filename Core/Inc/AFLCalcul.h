#pragma once

#include "Profil.h"
#include "tachometer.h"
#include "temperature_stub.h"

#include <stdint.h>

// Sortie du calcul principal.
// On produit une consigne RPM par ventilateur.
typedef struct {
    uint16_t target_rpm[PROFIL_FAN_COUNT];
} AFLCalcul_Output_t;

// Calcule la cible RPM a partir :
// - du profil choisi
// - de la temperature lue
// - du vrai retour tachymetre
// C'est le coeur de la regulation.
void AFLCalcul_Compute(const Profil_Result_t *profiles,
                       const TemperatureSnapshot_t *temps,
                       const TachometerSnapshot_t *tach,
                       AFLCalcul_Output_t *out);
