#pragma once

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef enum {
    FAN_STATE_OFF = 0,
    FAN_STATE_ON,
    FAN_STATE_CONTROL,
    FAN_STATE_ERROR,
    FAN_STATE_NC
} FanState_t;

typedef enum {
    FAN_ACTION_NONE = 0,
    FAN_ACTION_ACCEL,
    FAN_ACTION_DETECT,
    FAN_ACTION_DECEL,
    FAN_ACTION_IDLE,
    FAN_ACTION_DISABLED
} FanAction_t;

typedef enum {
    FAN_ALERT_NONE = 0,
    FAN_ALERT_WARN,
    FAN_ALERT_SHUTDOWN
} FanAlert_t;

typedef struct {
    FanState_t state;
    FanAction_t action;
    FanAlert_t alert;
    uint16_t rpm;
    uint8_t percent;
} FanStatus_t;

typedef struct {
    uint8_t tick;
    uint8_t blink;
} UI_Anim_t;

const char* UI_FanStateToStr(FanState_t state);
const char* UI_FanActionToStr(FanAction_t action);
const char* UI_FanAlertToStr(FanAlert_t alert);

const char* UI_GetAnimatedAction(FanAction_t action);
const char* UI_GetAnimatedAlert(FanAlert_t alert);

void UI_AnimUpdate(void);
void UI_DrawFanLine(uint8_t row, uint8_t fan_index, const FanStatus_t *fan);
void UI_DrawFanLineAnimated(uint8_t row, uint8_t fan_index, const FanStatus_t *fan);
void UI_DrawAllFans(const FanStatus_t fans[4]);
