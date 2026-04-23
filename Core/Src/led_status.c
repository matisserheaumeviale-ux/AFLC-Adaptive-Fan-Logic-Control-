#include "led_status.h"

#include "main.h"

#define LED_HEARTBEAT_PERIOD_MS 500UL
#define LED_ACTIVITY_HOLD_MS    100UL

typedef struct {
    uint32_t heartbeat_ms;
    uint32_t comm_activity_ms;
    bool heartbeat_on;
    LedStatus_Mode_t mode;
} LedStatusRuntime_t;

static LedStatusRuntime_t g_leds;

static void LedStatus_Write(GPIO_TypeDef *port, uint16_t pin, bool is_on);

void LedStatus_Init(void)
{
    g_leds.heartbeat_ms = HAL_GetTick();
    g_leds.comm_activity_ms = 0UL;
    g_leds.heartbeat_on = false;
    g_leds.mode = LED_STATUS_MODE_BOOT;

    LedStatus_Write(LED_STATUS_GPIO_Port, LED_STATUS_Pin, false);
    LedStatus_Write(LED_USB_GPIO_Port, LED_USB_Pin, false);
    LedStatus_Write(LED_ERROR_GPIO_Port, LED_ERROR_Pin, false);
    LedStatus_Write(LED_EXTRA1_GPIO_Port, LED_EXTRA1_Pin, false);
}

void LedStatus_SetMode(LedStatus_Mode_t mode)
{
    g_leds.mode = mode;
}

void LedStatus_Task(uint32_t now_ms)
{
    bool led_status_on = false;
    bool led_error_on = false;
    bool led_extra_on = false;

    if ((now_ms - g_leds.heartbeat_ms) >= LED_HEARTBEAT_PERIOD_MS) {
        g_leds.heartbeat_ms = now_ms;
        g_leds.heartbeat_on = !g_leds.heartbeat_on;
    }

    switch (g_leds.mode) {
        case LED_STATUS_MODE_BOOT:
            led_status_on = true;
            break;
        case LED_STATUS_MODE_PROFILES_READY:
            led_status_on = true;
            led_extra_on = true;
            break;
        case LED_STATUS_MODE_WAIT_CONFIRM:
            led_status_on = g_leds.heartbeat_on;
            break;
        case LED_STATUS_MODE_STARTUP_TEST:
            led_status_on = true;
            led_extra_on = g_leds.heartbeat_on;
            break;
        case LED_STATUS_MODE_RUNTIME:
            led_status_on = g_leds.heartbeat_on;
            break;
        case LED_STATUS_MODE_FAULT:
        default:
            led_error_on = true;
            break;
    }

    LedStatus_Write(LED_USB_GPIO_Port,
                    LED_USB_Pin,
                    (now_ms - g_leds.comm_activity_ms) <= LED_ACTIVITY_HOLD_MS);
    LedStatus_Write(LED_STATUS_GPIO_Port, LED_STATUS_Pin, led_status_on);
    LedStatus_Write(LED_ERROR_GPIO_Port, LED_ERROR_Pin, led_error_on);
    LedStatus_Write(LED_EXTRA1_GPIO_Port, LED_EXTRA1_Pin, led_extra_on);
}

void LedStatus_NotifyCommActivity(uint32_t now_ms)
{
    g_leds.comm_activity_ms = now_ms;
}

static void LedStatus_Write(GPIO_TypeDef *port, uint16_t pin, bool is_on)
{
    HAL_GPIO_WritePin(port, pin, is_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
