#pragma once

#include <stdbool.h>
#include <stdint.h>

// Modes d'affichage LED selon l'etat applicatif.
typedef enum {
    LED_STATUS_MODE_BOOT = 0,
    LED_STATUS_MODE_PROFILES_READY,
    LED_STATUS_MODE_WAIT_CONFIRM,
    LED_STATUS_MODE_STARTUP_TEST,
    LED_STATUS_MODE_RUNTIME,
    LED_STATUS_MODE_FAULT
} LedStatus_Mode_t;

void LedStatus_Init(void);
void LedStatus_SetMode(LedStatus_Mode_t mode);

// Fait clignoter / maintenir les LEDs selon le mode.
void LedStatus_Task(uint32_t now_ms);

// Signale une activite de communication UART.
void LedStatus_NotifyCommActivity(uint32_t now_ms);
