#include "uart_cmd.h"

#include "button.h"
#include "fan_control.h"
#include "led_status.h"
#include "printf-scanf.h"
#include "tachometer.h"
#include "usart.h"

#include <stdio.h>

static void UART_Cmd_PrintHelp(void);
static void UART_Cmd_PrintSystemStatus(void);
static void UART_Cmd_PrintTachStatus(void);

void UART_Cmd_Init(void)
{
    // Au boot, on montre tout de suite les commandes.
    UART_Cmd_PrintHelp();
}

void UART_Cmd_Task(void)
{
    uint8_t rx;

    // Lecture non bloquante.
    if (HAL_UART_Receive(&huart1, &rx, 1U, 0U) != HAL_OK) {
        return;
    }

    LedStatus_NotifyCommActivity(HAL_GetTick());

    switch (rx) {
        case 'h':
        case 'H':
            UART_Cmd_PrintHelp();
            break;

        case 's':
        case 'S':
            UART_Cmd_PrintSystemStatus();
            break;

        case 't':
        case 'T':
            UART_Cmd_PrintTachStatus();
            break;

        case 'c':
        case 'C':
            Button_NotifyConfirmCommand();
            printf("Confirm event latched\r\n");
            break;

        case 'z':
        case 'Z':
            FanControl_StartRampTest(HAL_GetTick());
            printf("Ramp test started: 0%% to 100%% over 5s\r\n");
            break;

        case 'a':
        case 'A':
            FanControl_SetDutyPermilleAll(1000U);
            printf("Duty set to 100%% on all fans\r\n");
            break;

        case '0':
            FanControl_SetDutyPermilleAll(0U);
            printf("Duty set to 0%% on all fans\r\n");
            break;

        default:
            if ((rx >= '1') && (rx <= '9')) {
                // Exemple :
                // '5' -> 500 pour-mille -> 50 %
                uint16_t duty_permille = (uint16_t)(rx - '0') * 100U;
                FanControl_SetDutyPermilleAll(duty_permille);
                printf("Duty set to %u%% on all fans\r\n", (unsigned int)(rx - '0') * 10U);
            } else {
                printf("Unknown command '%c' - send 'h' for help\r\n", rx);
            }
            break;
    }
}

int __io_putchar(int ch)
{
    uint8_t byte = (uint8_t)ch;

    // Redirige printf vers USART1.
    (void)HAL_UART_Transmit(&huart1, &byte, 1U, HAL_MAX_DELAY);
    return ch;
}

int __io_getchar(void)
{
    uint8_t byte;

    if (HAL_UART_Receive(&huart1, &byte, 1U, 0U) == HAL_OK) {
        return (int)byte;
    }

    return -1;
}

static void UART_Cmd_PrintHelp(void)
{
    // Menu minimal.
    printf("\r\nCommands:\r\n");
    printf("  h : help\r\n");
    printf("  s : system status\r\n");
    printf("  t : tach/rpm status\r\n");
    printf("  c : confirm startup\r\n");
    printf("  0..9 : set all fans to 0%%..90%%\r\n");
    printf("  a : set all fans to 100%%\r\n");
    printf("  z : non-blocking ramp test\r\n");
}

static void UART_Cmd_PrintSystemStatus(void)
{
    const FanStatus_t *fans = FanControl_GetStatuses();
    uint16_t duty_permille = FanControl_GetDutyPermille();
    uint8_t index;

    // Premiere ligne = vue globale.
    printf("System status | Duty=%u.%u%% | Fault=%s | Ramp=%s\r\n",
           (unsigned int)(duty_permille / 10U),
           (unsigned int)(duty_permille % 10U),
           FanControl_HasAnyFault() ? "YES" : "NO",
           FanControl_IsRampActive() ? "ON" : "OFF");

    for (index = 0U; index < FAN_CONTROL_CHANNEL_COUNT; index++) {
        printf("F%u state=%u rpm=%u duty=%u%% alert=%u\r\n",
               (unsigned int)(index + 1U),
               (unsigned int)fans[index].state,
               (unsigned int)fans[index].rpm,
               (unsigned int)fans[index].percent,
               (unsigned int)fans[index].alert);
    }
}

static void UART_Cmd_PrintTachStatus(void)
{
    uint8_t index;

    // Une ligne par tach.
    for (index = 0U; index < TACHOMETER_CHANNEL_COUNT; index++) {
        const TachometerReading_t *tach = Tachometer_GetReading(index);

        if (tach == NULL) {
            continue;
        }

        printf("T%u inst=%u rpm filt=%u rpm signal=%u timeout=%u\r\n",
               (unsigned int)(index + 1U),
               (unsigned int)tach->rpm_instant,
               (unsigned int)tach->rpm_filtered,
               tach->signal_present ? 1U : 0U,
               tach->timeout ? 1U : 0U);
    }
}
