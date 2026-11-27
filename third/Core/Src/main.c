/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "stdio.h"
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
#define RUN 1
#define STOP 0
#define DIR_FORWARD 0
#define DIR_BACKWARD 1

typedef struct
{
  uint8_t run_flag;
  uint8_t dir_flag;
  uint16_t step_ms;
}SysStateTypeDef;

SysStateTypeDef sys = {STOP, DIR_FORWARD, 500};
SysStateTypeDef sys_last;  

uint8_t uart_buf[5];
uint8_t buf_idx = 0;
uint8_t ch;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t UART_Parse(uint8_t *buf, SysStateTypeDef *sys)
{
  uint8_t cmd = buf[1];
  uint8_t STEP_L = buf[2];
  uint8_t STEP_H = buf[3];
  uint16_t step = (STEP_H << 8) | STEP_L;

  if(step < 100 || step > 2000)
  {
    return 0;
  }

  switch(cmd)
  {
    case 0x01:
      sys->run_flag = !sys->run_flag;
      break;
    case 0x02:
      sys->dir_flag = !sys->dir_flag;
      break;
    case 0x03:
      sys->step_ms = step;
      break;
    default:
      return 0;
  }
  return 1;
}

void Key_Scan(SysStateTypeDef *sys)
{
  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET)
  {
    HAL_Delay(20);
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET)
    {
      while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET);
      sys->run_flag = !sys->run_flag;
    }
  }
}

void OLED_Refresh(SysStateTypeDef *sys, SysStateTypeDef *last)
{
  if(sys->run_flag == last->run_flag && sys->dir_flag == last->dir_flag && sys->step_ms == last->step_ms)
  {
    return;
  }
  *last = *sys;

  char buf[20];
  OLED_Clear();

  if(sys->run_flag == RUN)
  {
    OLED_ShowString(1,1,"RUN");
  }
  else
  {
    OLED_ShowString(1,1,"STOP");
  }

  if(sys->dir_flag == DIR_FORWARD)
  {
    OLED_ShowString(2,1,"FORWARD");
  }
  else
  {
    OLED_ShowString(2,1,"BACKWARD");
  }

  sprintf(buf,"T:%d ms",sys->step_ms);
  OLED_ShowString(4,1,buf);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance == USART1)
  {
    uart_buf[buf_idx] = ch;
    buf_idx++;

    if(buf_idx >= 5)
    {
      if(uart_buf[0] == 0xAA && uart_buf[4] == 0xBB)
      {
        UART_Parse(uart_buf, &sys);
      }
      buf_idx = 0;
    }
    HAL_UART_Receive_IT(&huart1, &ch, 1);
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

 
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
  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_Clear();
  HAL_UART_Receive_IT(&huart1, &ch, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint8_t pos = 0;
  while (1)
  {
    Key_Scan(&sys);
    OLED_Refresh(&sys, &sys_last);

    if(sys.run_flag == STOP)
    {
      continue;
    }

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

    if(sys.dir_flag == DIR_FORWARD)
    {
      switch(pos)
      {
        case 0: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); break;
        case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); break;
        case 2: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET); break;
      }
      pos++;
      if(pos >= 3) pos = 0;
    }
    else
    {
      switch(pos)
      {
        case 0: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET); break;
        case 1: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); break;
        case 2: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); break;
      }
      pos++;
      if(pos >= 3) pos = 0;
    }
    HAL_Delay(sys.step_ms);
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

 
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }


  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */


void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  
  /* USER CODE END 6 */
}
#endif
