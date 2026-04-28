#pragma once

#include <stdint.h>

/*
 * Module temperature_stub
 * -----------------------
 * Ce module simule des temperatures.
 *
 * Il permet de tester la logique AFLC meme si les vrais capteurs
 * de temperature ne sont pas encore branches.
 *
 * Plus tard, ce module pourra etre remplace par un vrai driver capteur
 * sans devoir reecrire le reste de la regulation.
 */

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
// Le pointeur retourne reste la propriete du module.
const TemperatureSnapshot_t *TemperatureStub_GetSnapshot(void);
