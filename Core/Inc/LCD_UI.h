#pragma once

#include "fan_control.h"

#include <stdint.h>

// Petit etat pour les animations texte.
typedef struct {
    uint8_t tick;
    uint8_t blink;
} UI_Anim_t;

// Conversions enum -> texte.
const char* UI_FanStateToStr(FanState_t state);
const char* UI_FanActionToStr(FanAction_t action);
const char* UI_FanAlertToStr(FanAlert_t alert);

// Variantes animees pour rendre le LCD plus vivant.
const char* UI_GetAnimatedAction(FanAction_t action);
const char* UI_GetAnimatedAlert(FanAlert_t alert);

// Helpers de dessin.
void UI_AnimUpdate(void);
void UI_DrawFanLine(uint8_t row, uint8_t fan_index, const FanStatus_t *fan);
void UI_DrawFanLineAnimated(uint8_t row, uint8_t fan_index, const FanStatus_t *fan);
void UI_DrawAllFans(const FanStatus_t fans[4]);
