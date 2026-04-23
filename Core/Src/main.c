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
#include "_DeclarationGenerale.h"
#include "printf-scanf.h"
#include "PORT_1-2.h"
#include "stdbool.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define FAN_PPR               2u
#define TIM2_TICK_HZ          125000u   // PSC=63 @ 8MHz => 8us/tick
#define PWM_ARR_TIM3          319u      // 25kHz @ 8MHz, PSC=0
#define DUTY_MIN_PERM         200u
#define DUTY_MAX_PERM         1000u
#define STATUS_PRINT_MS       500u
#define TACH_TIMEOUT_MS       1200u
#define SW2_MASQUE   0x04   // P2_2 ? diminue
#define SW3_MASQUE   0x08   // P2_3 ? 50% / stop
#define SW4_MASQUE   0x10   // P2_4 ? augmente

volatile uint32_t gulLastCap      = 0;
volatile uint32_t gulPeriod       = 0;
volatile uint8_t  gucNewPer       = 0;
volatile uint8_t  gucFirstCap     = 1;
volatile uint32_t gulLastEdgeMs   = 0;
volatile uint32_t gulRPM_inst     = 0;
volatile uint32_t gulRPM_filt     = 0;

uint16_t gusDuty_perm = 300;

uint32_t gulLastStatusPrintMs = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */


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

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    vTraiterCommandeUART();
    vCalculRPM();
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
  if ((htim->Instance == TIM2) && (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3))
  {
    uint32_t ulNow;
    uint32_t ulPer;

    ulNow = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);

    if (gucFirstCap != 0u)
    {
      gulLastCap = ulNow;
      gulLastEdgeMs = HAL_GetTick();
      gucFirstCap = 0u;
      return;
    }

    if (ulNow >= gulLastCap)
      ulPer = ulNow - gulLastCap;
    else
      ulPer = (0xFFFFu - gulLastCap) + ulNow + 1u;

    gulLastCap = ulNow;
    gulLastEdgeMs = HAL_GetTick();

    if (ulPer > 0u)
    {
      gulPeriod = ulPer;
      gucNewPer = 1u;
    }
  }
}
//********************************vSetFanDutyPerm*******************************
//    Nom de la fonction : vSetFanDutyPerm
//    Auteur : Matisse Rh�aume Viale		
//    Date de cr�ation : 2026-04-02
//    Date de la derni�re modification : 
//    Description : 	Routine pour ajuster le duty cycle du PWM du ventilateur.
//                    La valeur est donn�e en pour mille (0 � 1000).
//							
//    Fonctions appel�es : __HAL_TIM_SET_COMPARE()		
//    Param�tres d'entr�e : usDutyPerm (uint16_t, 0 � 1000)		
//    Param�tres de sortie : Aucun		
//    Variables utilis�es : ulCompare, gusDuty_perm
//    Equate : PWM_ARR_TIM3, DUTY_MAX_PERM	
//    #Define : Aucun	
// 						
//******************************************************************************
static void vSetFanDutyPerm(uint16_t usDutyPerm)
{
  uint32_t ulCompare;

  if (usDutyPerm > DUTY_MAX_PERM)
    usDutyPerm = DUTY_MAX_PERM;

  gusDuty_perm = usDutyPerm;

  ulCompare = ((uint32_t)(PWM_ARR_TIM3 + 1u) * (uint32_t)gusDuty_perm) / 1000u;

  if (ulCompare > PWM_ARR_TIM3)
    ulCompare = PWM_ARR_TIM3;

  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ulCompare);
}
//*******************************vTraiterCommandeUART***************************
//    Nom de la fonction : vTraiterCommandeUART
//    Auteur : Matisse Rh�aume Viale		
//    Date de cr�ation : 2026-04-02
//    Date de la derni�re modification : 
//    Description : 	Routine pour lire une commande via UART et ajuster le
//                    duty cycle du ventilateur en cons�quence.
//                    Commandes disponibles :
//                    - '0' ou 'x' : arr�t
//                    - '1' � '9' : 10% � 90%
//                    - 'a' : 100%
//                    - 'z' : rampe 0% � 100% en 5 secondes
//							
//    Fonctions appel�es : HAL_UART_Receive(), vSetFanDutyPerm(),
//                         vRampe0a100_5s(), printf()		
//    Param�tres d'entr�e : Aucun		
//    Param�tres de sortie : Aucun		
//    Variables utilis�es : ucRx
//    Equate : Aucun	
//    #Define : Aucun	
// 						
//******************************************************************************
static void vTraiterCommandeUART(void)
{
  uint8_t ucRx;

  if (HAL_UART_Receive(&huart1, &ucRx, 1, 0) == HAL_OK)
  {
    if ((ucRx == 'x') || (ucRx == 'X') || (ucRx == '0'))
    {
      vSetFanDutyPerm(0u);
      printf("Arret\r\n");
    }
    else if ((ucRx >= '1') && (ucRx <= '9'))
    {
      vSetFanDutyPerm((uint16_t)(ucRx - '0') * 100u);
      printf("Duty = %d0%%\r\n", ucRx - '0');
    }
    else if ((ucRx == 'a') || (ucRx == 'A'))
    {
      vSetFanDutyPerm(1000u);
      printf("Duty = 100%%\r\n");
    }
    else if ((ucRx == 'z') || (ucRx == 'Z'))
    {
      printf("Rampe 0 a 100%% en 5 s\r\n");
      vRampe0a100_5s();
    }
    else if (ucRx == 't' || ucRx == 'T')
{
  vCalculRPM();
  vPrintRPM();
}
  }
}

//********************************vCalculRPM************************************
static void vCalculRPM(void)
{
  if ((HAL_GetTick() - gulLastEdgeMs) > TACH_TIMEOUT_MS)
  {
    gulRPM_inst = 0;
    gulRPM_filt = 0;
    return;
  }

  if (gucNewPer != 0u)
  {
    gucNewPer = 0u;

    if (gulPeriod > 0u)
    {
      gulRPM_inst = ((TIM2_TICK_HZ * 60u) / gulPeriod) / FAN_PPR;
      gulRPM_filt = gulRPM_inst;
    }
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
  /* User can add his own implementation to report the HAL error return state */
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
