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
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "sensors_i2c.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define THR_COUNT 5u
#define KEY_DEBOUNCE_MS 25u
#define KEY3_SHORT_MS 450u
#define KEY3_LONG_MS 650u
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;

static uint8_t ui_page; /* 0 info, 1 thresholds */
static uint8_t thr_idx;

static uint16_t thr_adc;
static int16_t thr_tmax_c10;
static int16_t thr_rhmin10;
static uint16_t thr_lux_min;
static uint16_t thr_pmin_hpa;

static uint8_t key3_armed;
static uint32_t key3_down_tick;

static void fmt_t10(char *buf, int16_t t10)
{
  unsigned neg = 0;
  int v = t10;
  if (v < 0)
  {
    neg = 1;
    v = -v;
  }
  sprintf(buf, "%s%d.%d", neg ? "-" : "", v / 10, v % 10);
}

static uint16_t read_adc_avg(void)
{
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++)
  {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 50) == HAL_OK)
      sum += (uint32_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
  }
  return (uint16_t)(sum / 8u);
}

static void thr_clamp_all(void)
{
  if (thr_adc > 4095u)
    thr_adc = 4095u;
  if (thr_tmax_c10 < -200)
    thr_tmax_c10 = -200;
  if (thr_tmax_c10 > 600)
    thr_tmax_c10 = 600;
  if (thr_rhmin10 < 0)
    thr_rhmin10 = 0;
  if (thr_rhmin10 > 1000)
    thr_rhmin10 = 1000;
  if (thr_lux_min > 60000u)
    thr_lux_min = 60000u;
  if (thr_pmin_hpa < 300u)
    thr_pmin_hpa = 300u;
  if (thr_pmin_hpa > 1100u)
    thr_pmin_hpa = 1100u;
}

static void thr_delta(int dir)
{
  switch (thr_idx)
  {
  case 0:
    if (dir > 0)
      thr_adc = (uint16_t)((thr_adc + 32u > 4095u) ? 4095u : thr_adc + 32u);
    else
      thr_adc = (thr_adc > 32u) ? (uint16_t)(thr_adc - 32u) : 0u;
    break;
  case 1:
    thr_tmax_c10 = (int16_t)(thr_tmax_c10 + dir * 5);
    break;
  case 2:
    thr_rhmin10 = (int16_t)(thr_rhmin10 + dir * 10);
    break;
  case 3:
    if (dir > 0)
      thr_lux_min = (uint16_t)((thr_lux_min + 50u > 60000u) ? 60000u : thr_lux_min + 50u);
    else
      thr_lux_min = (thr_lux_min > 50u) ? (uint16_t)(thr_lux_min - 50u) : 0u;
    break;
  case 4:
    if (dir > 0)
      thr_pmin_hpa = (uint16_t)((thr_pmin_hpa + 1u > 1100u) ? 1100u : thr_pmin_hpa + 1u);
    else
      thr_pmin_hpa = (thr_pmin_hpa > 300u) ? (uint16_t)(thr_pmin_hpa - 1u) : 300u;
    break;
  default:
    break;
  }
  thr_clamp_all();
}

static void key_poll(uint32_t now)
{
  uint8_t k3 = (HAL_GPIO_ReadPin(key3_GPIO_Port, key3_Pin) == GPIO_PIN_RESET) ? 1u : 0u;
  if (k3)
  {
    if (!key3_armed)
    {
      key3_armed = 1u;
      key3_down_tick = now;
    }
  }
  else
  {
    if (key3_armed)
    {
      uint32_t dt = now - key3_down_tick;
      if (dt >= KEY_DEBOUNCE_MS)
      {
        if (dt < KEY3_SHORT_MS)
          ui_page ^= 1u;
        else if (dt >= KEY3_LONG_MS && ui_page == 1u)
          thr_idx = (uint8_t)((thr_idx + 1u) % THR_COUNT);
      }
      key3_armed = 0u;
    }
  }

  static uint8_t k1s = 0u, k2s = 0u;
  static uint32_t lk1 = 0u, lk2 = 0u;
  uint8_t p1 = (HAL_GPIO_ReadPin(key1_GPIO_Port, key1_Pin) == GPIO_PIN_RESET) ? 1u : 0u;
  uint8_t p2 = (HAL_GPIO_ReadPin(key2_GPIO_Port, key2_Pin) == GPIO_PIN_RESET) ? 1u : 0u;

  if (p1 != k1s && (now - lk1) > KEY_DEBOUNCE_MS)
  {
    k1s = p1;
    lk1 = now;
    if (ui_page == 1u && p1)
      thr_delta(+1);
  }
  if (p2 != k2s && (now - lk2) > KEY_DEBOUNCE_MS)
  {
    k2s = p2;
    lk2 = now;
    if (ui_page == 1u && p2)
      thr_delta(-1);
  }
}

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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  thr_adc = 2048u;
  thr_tmax_c10 = 350;
  thr_rhmin10 = 300;
  thr_lux_min = 50u;
  thr_pmin_hpa = 990u;

  Sensors_Init(&hi2c1);
  SSD1306_Init(&hi2c1);

  Sensor_I2C_Data_t sens;
  char line[22];

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_sens = 0;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();
    key_poll(now);

    uint16_t adc_raw = read_adc_avg();
    if (adc_raw > thr_adc)
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

    if ((now - last_sens) >= 400u)
    {
      last_sens = now;
      Sensors_Read(&sens);
    }

    SSD1306_Clear();
    if (ui_page == 0u)
    {
      sprintf(line, "PA0 ADC:%4u", (unsigned)adc_raw);
      SSD1306_DrawStr(0, 0, line);
      sprintf(line, "LED:%s", (adc_raw > thr_adc) ? "ON " : "OFF");
      SSD1306_DrawStr(0, 8, line);

      if (sens.sht_ok)
      {
        char tb[8];
        fmt_t10(tb, sens.sht_temp_c10);
        sprintf(line, "SHT T:%sC", tb);
        SSD1306_DrawStr(0, 16, line);
        sprintf(line, "SHT RH:%d.%d%%", sens.sht_rh10 / 10, sens.sht_rh10 % 10);
        SSD1306_DrawStr(0, 24, line);
      }
      else
      {
        SSD1306_DrawStr(0, 16, "SHT: ---");
      }

      if (sens.bmp_ok)
      {
        char tb[8];
        fmt_t10(tb, sens.bmp_temp_c10);
        sprintf(line, "BMP T:%sC", tb);
        SSD1306_DrawStr(0, 32, line);
        sprintf(line, "BMP P:%ldPa", (long)sens.bmp_press_pa);
        SSD1306_DrawStr(0, 40, line);
      }
      else
      {
        SSD1306_DrawStr(0, 32, "BMP: ---");
      }

      if (sens.bh_ok)
        sprintf(line, "BH1750:%ulu", (unsigned long)sens.bh_lux);
      else
        sprintf(line, "BH1750:---");
      SSD1306_DrawStr(0, 48, line);

      SSD1306_DrawStr(0, 56, "K3:thr L3:sel");
    }
    else
    {
      SSD1306_DrawStr(0, 0, "Thresholds");
      char tb[8];

      sprintf(line, "%sADC:%u", (thr_idx == 0u) ? ">" : " ", (unsigned)thr_adc);
      SSD1306_DrawStr(0, 8, line);
      fmt_t10(tb, thr_tmax_c10);
      sprintf(line, "%sTmax:%sC", (thr_idx == 1u) ? ">" : " ", tb);
      SSD1306_DrawStr(0, 16, line);
      sprintf(line, "%sRHmin:%d.%d%%", (thr_idx == 2u) ? ">" : " ", thr_rhmin10 / 10, thr_rhmin10 % 10);
      SSD1306_DrawStr(0, 24, line);
      sprintf(line, "%sLuxmin:%u", (thr_idx == 3u) ? ">" : " ", (unsigned)thr_lux_min);
      SSD1306_DrawStr(0, 32, line);
      sprintf(line, "%sPmin:%uhPa", (thr_idx == 4u) ? ">" : " ", (unsigned)thr_pmin_hpa);
      SSD1306_DrawStr(0, 40, line);

      SSD1306_DrawStr(0, 48, "K1+ K2-");
      SSD1306_DrawStr(0, 56, "K3:pg L3:next");
    }

    SSD1306_Refresh();
    HAL_Delay(80);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  (void)file;
  (void)line;
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
