/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "AFLCalcul.h"
#include "Profil.h"
#include "button.h"
#include "fan_control.h"
#include "led_status.h"
#include "tachometer.h"
#include "temperature_stub.h"
#include "uart_cmd.h"
#include "ui_lcd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  AFLC_APP_BOOT = 0,
  AFLC_APP_PROFILES_CALCULATING,
  AFLC_APP_PROFILES_READY,
  AFLC_APP_WAIT_CONFIRM,
  AFLC_APP_STARTUP_TEST_INIT,
  AFLC_APP_STARTUP_TEST_RUN,
  AFLC_APP_STARTUP_TEST_EVAL,
  AFLC_APP_RUNTIME_CONTROL,
  AFLC_APP_SAFE_STATE
} AflcAppState_t;

typedef struct {
  AflcAppState_t state;
  uint32_t state_entered_ms;
  const Profil_Result_t *profiles;
  TachometerSnapshot_t tach_snapshot;
  AFLCalcul_Output_t control_output;
  const TemperatureSnapshot_t *temperatures;
  uint8_t startup_passed_mask;
  uint8_t startup_fail_mask;
} AflcAppContext_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK_BUTTON_PERIOD_MS         20UL
#define TASK_TACH_PERIOD_MS           50UL
#define TASK_FAN_PERIOD_MS            100UL
#define TASK_CONTROL_PERIOD_MS        250UL
#define TASK_LED_PERIOD_MS            50UL
#define TASK_TEMPERATURE_PERIOD_MS    250UL
#define PROFILES_READY_SHOW_MS        1500UL
#define STARTUP_TEST_DUTY_PERMILLE    250U
#define STARTUP_TEST_TIMEOUT_MS       3000UL
#define STARTUP_TEST_PASS_RPM         200U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t g_next_button_task_ms;
static uint32_t g_next_tach_task_ms;
static uint32_t g_next_fan_task_ms;
static uint32_t g_next_control_task_ms;
static uint32_t g_next_led_task_ms;
static uint32_t g_next_temperature_task_ms;
static AflcAppContext_t g_app;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void MX_AFLC_StartPeripherals(void);
static void AFLC_AppTask(uint32_t now_ms);
static void AFLC_AppEnterState(AflcAppState_t next_state, uint32_t now_ms);
static void AFLC_RuntimeStep(void);
static void AFLC_UpdateStartupPassMask(void);
static void AFLC_BuildMaxRpmArray(uint16_t max_rpm[PROFIL_FAN_COUNT]);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  MX_AFLC_StartPeripherals();
  Tachometer_Init();
  FanControl_Init(&htim3);
  LedStatus_Init();
  Button_Init();
  TemperatureStub_Init();
  Profil_Init();
  UI_LCD_Init();
  UART_Cmd_Init();

  g_app.temperatures = TemperatureStub_GetSnapshot();
  g_app.profiles = NULL;
  g_app.startup_passed_mask = 0U;
  g_app.startup_fail_mask = 0U;

  g_next_button_task_ms = HAL_GetTick();
  g_next_tach_task_ms = HAL_GetTick();
  g_next_fan_task_ms = HAL_GetTick();
  g_next_control_task_ms = HAL_GetTick();
  g_next_led_task_ms = HAL_GetTick();
  g_next_temperature_task_ms = HAL_GetTick();

  AFLC_AppEnterState(AFLC_APP_BOOT, HAL_GetTick());
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now_ms = HAL_GetTick();

    UART_Cmd_Task();

    if (now_ms >= g_next_button_task_ms)
    {
      g_next_button_task_ms = now_ms + TASK_BUTTON_PERIOD_MS;
      Button_Task();
    }

    if (now_ms >= g_next_tach_task_ms)
    {
      g_next_tach_task_ms = now_ms + TASK_TACH_PERIOD_MS;
      Tachometer_Task(now_ms);
    }

    if (now_ms >= g_next_fan_task_ms)
    {
      g_next_fan_task_ms = now_ms + TASK_FAN_PERIOD_MS;
      FanControl_Task(now_ms);
    }

    if (now_ms >= g_next_temperature_task_ms)
    {
      g_next_temperature_task_ms = now_ms + TASK_TEMPERATURE_PERIOD_MS;
      TemperatureStub_Task(now_ms);
      g_app.temperatures = TemperatureStub_GetSnapshot();
    }

    if (now_ms >= g_next_control_task_ms)
    {
      g_next_control_task_ms = now_ms + TASK_CONTROL_PERIOD_MS;
      AFLC_RuntimeStep();
    }

    AFLC_AppTask(now_ms);
    UI_LCD_Task(now_ms);

    if (now_ms >= g_next_led_task_ms)
    {
      g_next_led_task_ms = now_ms + TASK_LED_PERIOD_MS;
      LedStatus_Task(now_ms);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  Tachometer_OnCapture(htim);
}

static void MX_AFLC_StartPeripherals(void)
{
  if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
}

static void AFLC_AppTask(uint32_t now_ms)
{
  switch (g_app.state)
  {
    case AFLC_APP_BOOT:
      AFLC_AppEnterState(AFLC_APP_PROFILES_CALCULATING, now_ms);
      break;

    case AFLC_APP_PROFILES_CALCULATING:
      Profil_CalculateAll();
      g_app.profiles = Profil_GetResult();
      if ((g_app.profiles != NULL) && g_app.profiles->ready && g_app.profiles->valid)
      {
        AFLC_AppEnterState(AFLC_APP_PROFILES_READY, now_ms);
      }
      else
      {
        g_app.startup_fail_mask = 0x0FU;
        AFLC_AppEnterState(AFLC_APP_SAFE_STATE, now_ms);
      }
      break;

    case AFLC_APP_PROFILES_READY:
      if ((now_ms - g_app.state_entered_ms) >= PROFILES_READY_SHOW_MS)
      {
        AFLC_AppEnterState(AFLC_APP_WAIT_CONFIRM, now_ms);
      }
      break;

    case AFLC_APP_WAIT_CONFIRM:
      if (Button_IsConfirmPressed())
      {
        Button_ClearConfirm();
        AFLC_AppEnterState(AFLC_APP_STARTUP_TEST_INIT, now_ms);
      }
      break;

    case AFLC_APP_STARTUP_TEST_INIT:
      g_app.startup_passed_mask = 0U;
      g_app.startup_fail_mask = 0U;
      FanControl_SetDutyPermilleAll(STARTUP_TEST_DUTY_PERMILLE);
      UI_LCD_ShowStartupTest(g_app.startup_passed_mask, g_app.startup_fail_mask, 0U);
      AFLC_AppEnterState(AFLC_APP_STARTUP_TEST_RUN, now_ms);
      break;

    case AFLC_APP_STARTUP_TEST_RUN:
      AFLC_UpdateStartupPassMask();
      UI_LCD_ShowStartupTest(g_app.startup_passed_mask, 0U, 0U);

      if (g_app.startup_passed_mask == 0x0FU)
      {
        AFLC_AppEnterState(AFLC_APP_STARTUP_TEST_EVAL, now_ms);
      }
      else if ((now_ms - g_app.state_entered_ms) >= STARTUP_TEST_TIMEOUT_MS)
      {
        g_app.startup_fail_mask = (uint8_t)(0x0FU & (uint8_t)(~g_app.startup_passed_mask));
        AFLC_AppEnterState(AFLC_APP_STARTUP_TEST_EVAL, now_ms);
      }
      break;

    case AFLC_APP_STARTUP_TEST_EVAL:
      UI_LCD_ShowStartupTest(g_app.startup_passed_mask, g_app.startup_fail_mask, 1U);
      if (g_app.startup_fail_mask == 0U)
      {
        AFLC_AppEnterState(AFLC_APP_RUNTIME_CONTROL, now_ms);
      }
      else
      {
        AFLC_AppEnterState(AFLC_APP_SAFE_STATE, now_ms);
      }
      break;

    case AFLC_APP_RUNTIME_CONTROL:
      UI_LCD_ShowRuntime(g_app.profiles,
                         g_app.temperatures,
                         &g_app.control_output,
                         FanControl_GetStatuses());
      break;

    case AFLC_APP_SAFE_STATE:
      FanControl_EnterSafeState();
      UI_LCD_ShowSafeState(g_app.startup_fail_mask);
      break;

    default:
      break;
  }
}

static void AFLC_AppEnterState(AflcAppState_t next_state, uint32_t now_ms)
{
  g_app.state = next_state;
  g_app.state_entered_ms = now_ms;

  switch (next_state)
  {
    case AFLC_APP_BOOT:
      LedStatus_SetMode(LED_STATUS_MODE_BOOT);
      break;

    case AFLC_APP_PROFILES_CALCULATING:
      LedStatus_SetMode(LED_STATUS_MODE_BOOT);
      break;

    case AFLC_APP_PROFILES_READY:
      LedStatus_SetMode(LED_STATUS_MODE_PROFILES_READY);
      UI_LCD_ShowProfilesReady(g_app.profiles);
      break;

    case AFLC_APP_WAIT_CONFIRM:
      LedStatus_SetMode(LED_STATUS_MODE_WAIT_CONFIRM);
      UI_LCD_ShowWaitConfirm(g_app.profiles);
      break;

    case AFLC_APP_STARTUP_TEST_INIT:
    case AFLC_APP_STARTUP_TEST_RUN:
    case AFLC_APP_STARTUP_TEST_EVAL:
      LedStatus_SetMode(LED_STATUS_MODE_STARTUP_TEST);
      break;

    case AFLC_APP_RUNTIME_CONTROL:
      LedStatus_SetMode(LED_STATUS_MODE_RUNTIME);
      break;

    case AFLC_APP_SAFE_STATE:
      LedStatus_SetMode(LED_STATUS_MODE_FAULT);
      break;

    default:
      break;
  }
}

static void AFLC_RuntimeStep(void)
{
  uint16_t max_rpm[PROFIL_FAN_COUNT];

  if ((g_app.state != AFLC_APP_RUNTIME_CONTROL) || (g_app.profiles == NULL) || (g_app.temperatures == NULL))
  {
    return;
  }

  Tachometer_GetSnapshot(&g_app.tach_snapshot);
  AFLCalcul_Compute(g_app.profiles, g_app.temperatures, &g_app.tach_snapshot, &g_app.control_output);
  AFLC_BuildMaxRpmArray(max_rpm);
  FanControl_SetTargetRpmArray(g_app.control_output.target_rpm, max_rpm);
}

static void AFLC_UpdateStartupPassMask(void)
{
  uint8_t index;

  Tachometer_GetSnapshot(&g_app.tach_snapshot);

  for (index = 0U; index < PROFIL_FAN_COUNT; index++)
  {
    if (g_app.tach_snapshot.fans[index].rpm_filtered >= STARTUP_TEST_PASS_RPM)
    {
      g_app.startup_passed_mask |= (uint8_t)(1U << index);
    }
  }
}

static void AFLC_BuildMaxRpmArray(uint16_t max_rpm[PROFIL_FAN_COUNT])
{
  uint8_t index;

  if ((max_rpm == NULL) || (g_app.profiles == NULL))
  {
    return;
  }

  for (index = 0U; index < PROFIL_FAN_COUNT; index++)
  {
    max_rpm[index] = g_app.profiles->fans[index].max_rpm;
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
