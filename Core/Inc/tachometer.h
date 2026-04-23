#pragma once

#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#define TACHOMETER_CHANNEL_COUNT 4U

typedef struct {
    uint16_t rpm_instant;
    uint16_t rpm_filtered;
    bool signal_present;
    bool timeout;
} TachometerReading_t;

typedef struct {
    TachometerReading_t fans[TACHOMETER_CHANNEL_COUNT];
} TachometerSnapshot_t;

void Tachometer_Init(void);
void Tachometer_OnCapture(TIM_HandleTypeDef *htim);
void Tachometer_Task(uint32_t now_ms);

const TachometerReading_t *Tachometer_GetReading(uint8_t index);
void Tachometer_GetSnapshot(TachometerSnapshot_t *snapshot);
