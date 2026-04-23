#include "fan_control.h"

#include <stddef.h>

#define FAN_PWM_PERIOD             319UL
#define FAN_DUTY_MAX_PERMILLE      1000U
#define FAN_RUNNING_MIN_RPM        200U
#define FAN_STARTUP_TIMEOUT_MS     1500UL
#define FAN_RAMP_STEP_PERMILLE     50U
#define FAN_RAMP_STEP_MS           250UL

typedef struct {
    TIM_HandleTypeDef *htim_pwm;
    uint32_t pwm_channel;
    uint16_t duty_permille;
    uint16_t last_rpm;
    uint32_t command_ms;
    bool ever_detected;
    FanStatus_t status;
} FanChannel_t;

typedef struct {
    bool active;
    uint16_t current_permille;
    uint32_t next_step_ms;
} RampControl_t;

static FanChannel_t g_fans[FAN_CONTROL_CHANNEL_COUNT];
static RampControl_t g_ramp;

static void FanControl_ApplyDuty(FanChannel_t *fan);
static void FanControl_UpdateFan(FanChannel_t *fan, uint8_t index, uint32_t now_ms);
static uint8_t FanControl_DutyToPercent(uint16_t duty_permille);

void FanControl_Init(TIM_HandleTypeDef *htim_pwm)
{
    static const uint32_t pwm_channels[FAN_CONTROL_CHANNEL_COUNT] = {
        TIM_CHANNEL_1,
        TIM_CHANNEL_2,
        TIM_CHANNEL_3,
        TIM_CHANNEL_4
    };
    uint8_t index;

    for (index = 0U; index < FAN_CONTROL_CHANNEL_COUNT; index++) {
        g_fans[index].htim_pwm = htim_pwm;
        g_fans[index].pwm_channel = pwm_channels[index];
        g_fans[index].duty_permille = 0U;
        g_fans[index].last_rpm = 0U;
        g_fans[index].command_ms = HAL_GetTick();
        g_fans[index].ever_detected = false;
        g_fans[index].status.state = FAN_STATE_OFF;
        g_fans[index].status.action = FAN_ACTION_NONE;
        g_fans[index].status.alert = FAN_ALERT_NONE;
        g_fans[index].status.rpm = 0U;
        g_fans[index].status.target_rpm = 0U;
        g_fans[index].status.percent = 0U;
        FanControl_ApplyDuty(&g_fans[index]);
    }

    g_ramp.active = false;
    g_ramp.current_permille = 0U;
    g_ramp.next_step_ms = 0UL;
    FanControl_SetDutyPermilleAll(0U);
}

void FanControl_Task(uint32_t now_ms)
{
    uint8_t index;

    if (g_ramp.active && (now_ms >= g_ramp.next_step_ms)) {
        FanControl_SetDutyPermilleAll(g_ramp.current_permille);
        if (g_ramp.current_permille >= FAN_DUTY_MAX_PERMILLE) {
            g_ramp.active = false;
        } else {
            g_ramp.current_permille = (uint16_t)(g_ramp.current_permille + FAN_RAMP_STEP_PERMILLE);
            g_ramp.next_step_ms = now_ms + FAN_RAMP_STEP_MS;
        }
    }

    for (index = 0U; index < FAN_CONTROL_CHANNEL_COUNT; index++) {
        FanControl_UpdateFan(&g_fans[index], index, now_ms);
    }
}

void FanControl_SetDutyPermille(uint8_t index, uint16_t duty_permille)
{
    if (index >= FAN_CONTROL_CHANNEL_COUNT) {
        return;
    }

    if (duty_permille > FAN_DUTY_MAX_PERMILLE) {
        duty_permille = FAN_DUTY_MAX_PERMILLE;
    }

    g_fans[index].duty_permille = duty_permille;
    g_fans[index].command_ms = HAL_GetTick();
    if (duty_permille == 0U) {
        g_fans[index].ever_detected = false;
        g_fans[index].status.target_rpm = 0U;
    }
    FanControl_ApplyDuty(&g_fans[index]);
}

void FanControl_SetDutyPermilleAll(uint16_t duty_permille)
{
    uint8_t index;

    if (duty_permille > FAN_DUTY_MAX_PERMILLE) {
        duty_permille = FAN_DUTY_MAX_PERMILLE;
    }

    if (g_ramp.active && (duty_permille != g_ramp.current_permille)) {
        g_ramp.active = false;
    }

    for (index = 0U; index < FAN_CONTROL_CHANNEL_COUNT; index++) {
        FanControl_SetDutyPermille(index, duty_permille);
    }
}

void FanControl_SetTargetRpm(uint8_t index, uint16_t target_rpm, uint16_t max_rpm)
{
    uint32_t duty_permille;

    if (index >= FAN_CONTROL_CHANNEL_COUNT) {
        return;
    }

    g_fans[index].status.target_rpm = target_rpm;

    if ((target_rpm == 0U) || (max_rpm == 0U)) {
        FanControl_SetDutyPermille(index, 0U);
        return;
    }

    duty_permille = ((uint32_t)target_rpm * 1000UL) / max_rpm;
    if (duty_permille < 150UL) {
        duty_permille = 150UL;
    }
    if (duty_permille > 1000UL) {
        duty_permille = 1000UL;
    }

    FanControl_SetDutyPermille(index, (uint16_t)duty_permille);
}

void FanControl_SetTargetRpmArray(const uint16_t target_rpm[FAN_CONTROL_CHANNEL_COUNT],
                                  const uint16_t max_rpm[FAN_CONTROL_CHANNEL_COUNT])
{
    uint8_t index;

    if ((target_rpm == NULL) || (max_rpm == NULL)) {
        return;
    }

    for (index = 0U; index < FAN_CONTROL_CHANNEL_COUNT; index++) {
        FanControl_SetTargetRpm(index, target_rpm[index], max_rpm[index]);
    }
}

void FanControl_StartRampTest(uint32_t now_ms)
{
    g_ramp.active = true;
    g_ramp.current_permille = 0U;
    g_ramp.next_step_ms = now_ms;
}

bool FanControl_IsRampActive(void)
{
    return g_ramp.active;
}

void FanControl_EnterSafeState(void)
{
    uint8_t index;

    g_ramp.active = false;
    for (index = 0U; index < FAN_CONTROL_CHANNEL_COUNT; index++) {
        g_fans[index].status.target_rpm = 0U;
    }
    FanControl_SetDutyPermilleAll(0U);
}

bool FanControl_HasAnyFault(void)
{
    uint8_t index;

    for (index = 0U; index < FAN_CONTROL_CHANNEL_COUNT; index++) {
        if (g_fans[index].status.state == FAN_STATE_ERROR) {
            return true;
        }
    }

    return false;
}

const FanStatus_t *FanControl_GetStatuses(void)
{
    return &g_fans[0].status;
}

uint16_t FanControl_GetDutyPermille(void)
{
    return g_fans[0].duty_permille;
}

uint16_t FanControl_GetDutyPermilleForFan(uint8_t index)
{
    if (index >= FAN_CONTROL_CHANNEL_COUNT) {
        return 0U;
    }

    return g_fans[index].duty_permille;
}

static void FanControl_ApplyDuty(FanChannel_t *fan)
{
    uint32_t compare;

    if ((fan == NULL) || (fan->htim_pwm == NULL)) {
        return;
    }

    compare = ((FAN_PWM_PERIOD + 1UL) * fan->duty_permille) / FAN_DUTY_MAX_PERMILLE;
    if (compare > FAN_PWM_PERIOD) {
        compare = FAN_PWM_PERIOD;
    }

    __HAL_TIM_SET_COMPARE(fan->htim_pwm, fan->pwm_channel, compare);
}

static void FanControl_UpdateFan(FanChannel_t *fan, uint8_t index, uint32_t now_ms)
{
    const TachometerReading_t *tach;

    tach = Tachometer_GetReading(index);
    if ((fan == NULL) || (tach == NULL)) {
        return;
    }

    fan->status.percent = FanControl_DutyToPercent(fan->duty_permille);
    fan->status.rpm = tach->rpm_filtered;
    fan->status.alert = FAN_ALERT_NONE;
    fan->status.action = FAN_ACTION_IDLE;

    if (fan->duty_permille == 0U) {
        fan->status.state = FAN_STATE_OFF;
        fan->status.action = FAN_ACTION_NONE;
        fan->status.rpm = 0U;
        fan->status.target_rpm = 0U;
        fan->last_rpm = 0U;
        return;
    }

    if (tach->signal_present && (tach->rpm_filtered >= FAN_RUNNING_MIN_RPM)) {
        fan->status.state = FAN_STATE_RUNNING;
        fan->ever_detected = true;
        if (tach->rpm_filtered > (uint16_t)(fan->last_rpm + 50U)) {
            fan->status.action = g_ramp.active ? FAN_ACTION_RAMP : FAN_ACTION_ACCEL;
        } else if ((fan->last_rpm > 50U) && (tach->rpm_filtered + 50U < fan->last_rpm)) {
            fan->status.action = FAN_ACTION_DECEL;
        } else {
            fan->status.action = FAN_ACTION_IDLE;
        }
    } else if ((now_ms - fan->command_ms) <= FAN_STARTUP_TIMEOUT_MS) {
        fan->status.state = FAN_STATE_STARTING;
        fan->status.action = FAN_ACTION_DETECT;
    } else {
        fan->status.state = FAN_STATE_ERROR;
        fan->status.action = FAN_ACTION_DETECT;
        fan->status.alert = fan->ever_detected ? FAN_ALERT_FAULT : FAN_ALERT_WARN;
    }

    fan->last_rpm = tach->rpm_filtered;
}

static uint8_t FanControl_DutyToPercent(uint16_t duty_permille)
{
    uint32_t percent;

    percent = (duty_permille + 5U) / 10U;
    if (percent > 100U) {
        percent = 100U;
    }

    return (uint8_t)percent;
}
