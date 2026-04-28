#pragma once

#include "tachometer.h"
#include "tim.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Module fan_control
 * ------------------
 * Ce module fait le lien entre la logique applicative et les sorties PWM.
 *
 * Il sait :
 * - commander les 4 ventilateurs
 * - convertir une cible RPM en duty PWM
 * - deduire un etat logique a partir du tachymetre
 * - signaler les warnings / faults
 *
 * C'est donc a la fois :
 * - un module de commande
 * - un module de diagnostic
 */

// 4 sorties PWM.
#define FAN_CONTROL_CHANNEL_COUNT 4U

// Etat global du fan vu par l'application.
typedef enum {
    FAN_STATE_OFF = 0,
    FAN_STATE_STARTING,
    FAN_STATE_RUNNING,
    FAN_STATE_ERROR
} FanState_t;

// Mouvement observe sur la vitesse.
typedef enum {
    FAN_ACTION_NONE = 0,
    FAN_ACTION_ACCEL,
    FAN_ACTION_DETECT,
    FAN_ACTION_DECEL,
    FAN_ACTION_IDLE,
    FAN_ACTION_RAMP
} FanAction_t;

// Niveau d'alerte.
typedef enum {
    FAN_ALERT_NONE = 0,
    FAN_ALERT_WARN,
    FAN_ALERT_FAULT
} FanAlert_t;

// Resume utile pour le LCD et le debug UART.
typedef struct {
    FanState_t state;
    FanAction_t action;
    FanAlert_t alert;
    uint16_t rpm;
    uint16_t target_rpm;
    uint8_t percent;
} FanStatus_t;

// Lie le module au timer PWM.
// Le timer passe ici est TIM3 dans ce projet.
void FanControl_Init(TIM_HandleTypeDef *htim_pwm);

// Tache periodique.
// Met a jour l'etat logique de chaque ventilateur.
void FanControl_Task(uint32_t now_ms);

// Commande brute en pour mille.
// 0    = 0 %
// 1000 = 100 %
void FanControl_SetDutyPermille(uint8_t index, uint16_t duty_permille);
void FanControl_SetDutyPermilleAll(uint16_t duty_permille);

// Commande plus haut niveau en RPM.
// max_rpm sert de reference pour convertir RPM -> PWM.
void FanControl_SetTargetRpm(uint8_t index, uint16_t target_rpm, uint16_t max_rpm);
void FanControl_SetTargetRpmArray(const uint16_t target_rpm[FAN_CONTROL_CHANNEL_COUNT],
                                  const uint16_t max_rpm[FAN_CONTROL_CHANNEL_COUNT]);

// Test de rampe non bloquant.
// Pratique pour valider les ventilateurs sans bloquer la boucle principale.
void FanControl_StartRampTest(uint32_t now_ms);

// Coupe tout si le demarrage a echoue.
void FanControl_EnterSafeState(void);
bool FanControl_IsRampActive(void);
bool FanControl_HasAnyFault(void);

// Retourne le tableau de statuts internes.
// Le pointeur retourne pointe vers les donnees du module.
const FanStatus_t *FanControl_GetStatuses(void);

// Helpers de debug.
uint16_t FanControl_GetDutyPermille(void);
uint16_t FanControl_GetDutyPermilleForFan(uint8_t index);
