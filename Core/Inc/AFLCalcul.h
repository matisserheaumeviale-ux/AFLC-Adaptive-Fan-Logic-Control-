#pragma once

#include "Profil.h"
#include "tachometer.h"
#include "temperature_stub.h"

#include <stdint.h>

/*
 * Module AFLCalcul
 * ----------------
 * Ce module contient la logique de regulation "metier".
 *
 * Entrees :
 * - les profils prepares par Profil.c
 * - les temperatures courantes
 * - le retour tachymetre reel
 *
 * Sortie :
 * - une cible RPM par ventilateur
 *
 * Important :
 * ce module ne parle ni au hardware, ni au PWM, ni au LCD.
 * Il decide seulement "quelle vitesse on veut".
 */

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
//
// En pratique :
// - temperature basse  -> RPM bas
// - temperature haute  -> RPM haut
// - entre les deux     -> interpolation lineaire
void AFLCalcul_Compute(const Profil_Result_t *profiles,
                       const TemperatureSnapshot_t *temps,
                       const TachometerSnapshot_t *tach,
                       AFLCalcul_Output_t *out);
