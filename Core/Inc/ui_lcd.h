#pragma once

#include "AFLCalcul.h"
#include "Profil.h"
#include "fan_control.h"
#include "temperature_stub.h"

#include <stdint.h>

void UI_LCD_Init(void);
void UI_LCD_Task(uint32_t now_ms);
void UI_LCD_ShowProfilesReady(const Profil_Result_t *profiles);
void UI_LCD_ShowWaitConfirm(const Profil_Result_t *profiles);
void UI_LCD_ShowStartupTest(uint8_t passed_mask, uint8_t fail_mask, uint8_t complete);
void UI_LCD_ShowRuntime(const Profil_Result_t *profiles,
                        const TemperatureSnapshot_t *temps,
                        const AFLCalcul_Output_t *targets,
                        const FanStatus_t fans[FAN_CONTROL_CHANNEL_COUNT]);
void UI_LCD_ShowSafeState(uint8_t fail_mask);
