#pragma once

#include "tachometer.h"
#include "tim.h"

#include <stdbool.h>
#include <stdint.h>

#define FAN_CONTROL_CHANNEL_COUNT 4U

typedef enum {
    FAN_STATE_OFF = 0,
    FAN_STATE_STARTING,
    FAN_STATE_RUNNING,
    FAN_STATE_ERROR
} FanState_t;

typedef enum {
    FAN_ACTION_NONE = 0,
    FAN_ACTION_ACCEL,
    FAN_ACTION_DETECT,
    FAN_ACTION_DECEL,
    FAN_ACTION_IDLE,
    FAN_ACTION_RAMP
} FanAction_t;

typedef enum {
    FAN_ALERT_NONE = 0,
    FAN_ALERT_WARN,
    FAN_ALERT_FAULT
} FanAlert_t;

typedef struct {
    FanState_t state;
    FanAction_t action;
    FanAlert_t alert;
    uint16_t rpm;
    uint16_t target_rpm;
    uint8_t percent;
} FanStatus_t;

void FanControl_Init(TIM_HandleTypeDef *htim_pwm);
void FanControl_Task(uint32_t now_ms);
void FanControl_SetDutyPermille(uint8_t index, uint16_t duty_permille);
void FanControl_SetDutyPermilleAll(uint16_t duty_permille);
void FanControl_SetTargetRpm(uint8_t index, uint16_t target_rpm, uint16_t max_rpm);
void FanControl_SetTargetRpmArray(const uint16_t target_rpm[FAN_CONTROL_CHANNEL_COUNT],
                                  const uint16_t max_rpm[FAN_CONTROL_CHANNEL_COUNT]);
void FanControl_StartRampTest(uint32_t now_ms);
void FanControl_EnterSafeState(void);
bool FanControl_IsRampActive(void);
bool FanControl_HasAnyFault(void);
const FanStatus_t *FanControl_GetStatuses(void);
uint16_t FanControl_GetDutyPermille(void);
uint16_t FanControl_GetDutyPermilleForFan(uint8_t index);
