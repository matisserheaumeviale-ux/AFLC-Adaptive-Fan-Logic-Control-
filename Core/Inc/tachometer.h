#pragma once

#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Module tachometer
 * -----------------
 * Ce module mesure la vitesse reelle des ventilateurs.
 *
 * Le signal tach donne des impulsions.
 * Le timer mesure le temps entre deux impulsions.
 * Ensuite ce temps est converti en RPM.
 *
 * Deux niveaux de lecture sont exposes :
 * - instantane
 * - filtre
 */

// 4 entrees tachymetre.
#define TACHOMETER_CHANNEL_COUNT 4U

// Lecture d'un canal.
typedef struct {
    uint16_t rpm_instant;
    uint16_t rpm_filtered;
    bool signal_present;
    bool timeout;
} TachometerReading_t;

// Copie complete des 4 lectures.
typedef struct {
    TachometerReading_t fans[TACHOMETER_CHANNEL_COUNT];
} TachometerSnapshot_t;

void Tachometer_Init(void);

// Callback appele depuis l'interruption input capture.
// Cette fonction doit rester tres courte : elle capture la periode,
// puis le gros du traitement est fait plus tard dans la tache periodique.
void Tachometer_OnCapture(TIM_HandleTypeDef *htim);

// Tache periodique pour convertir les captures en RPM stables.
void Tachometer_Task(uint32_t now_ms);

// Retourne la lecture d'un ventilateur.
const TachometerReading_t *Tachometer_GetReading(uint8_t index);

// Copie l'etat complet des 4 ventilateurs dans une structure fournie.
void Tachometer_GetSnapshot(TachometerSnapshot_t *snapshot);
