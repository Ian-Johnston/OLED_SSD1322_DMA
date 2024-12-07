/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  * VS2022 / VisualGDB:
  * To upload HEX from VS2022 = BUILD then PROGRAM AND START WITHOUT DEBUGGING
  *
  *
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SSD1322_OLED_lib/SSD1322_HW_Driver.h"
#include "SSD1322_OLED_lib/SSD1322_API.h"
#include "SSD1322_OLED_lib/SSD1322_GFX.h"
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
//******************************************************************************

// SPI receive buffer for packets data
volatile uint8_t rx_buffer[PACKET_WIDTH*PACKET_COUNT];

// Array with character bitmaps
uint8_t chars[CHAR_COUNT][CHAR_HEIGHT];

// Array with annunciators flags (boolean)
uint8_t flags[CHAR_COUNT];

// When scanning the display, the order of the characters output is not sequential,
// due to optimization of the VFD PCB layout. The Reorder[] array is used as a lookup
// table to determine the correct position of characters.
const uint8_t Reorder[PACKET_COUNT] = {	8,7,6,5,4,3,2,1,0,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,17,16,15,14,13,12,11,10,9,46,45,44,43,42,41,40,39,38,37,36};
// IJ TEST const uint8_t Reorder[PACKET_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 29, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46 };

// Used for line-by-line recoding and scaling of the characters of the main display line.
// All 32 possible variants of a 5-pixel wide character bitmap string are converted into
// the corresponding 10 OLED display pixels with 16 color gradations and packed into 5 bytes.
const uint8_t Upscale1[32][5] = {{0x00,0x00,0x00,0x00,0},{0x00,0x00,0x00,0x00,0xFF},{0x00,0x00,0x00,0xFF,0},{0x00,0x00,0x00,0xFF,0xFF},
						   	     {0x00,0x00,0xFF,0x00,0},{0x00,0x00,0xFF,0x00,0xFF},{0x00,0x00,0xFF,0xFF,0},{0x00,0x00,0xFF,0xFF,0xFF},
                                 {0x00,0xFF,0x00,0x00,0},{0x00,0xFF,0x00,0x00,0xFF},{0x00,0xFF,0x00,0xFF,0},{0x00,0xFF,0x00,0xFF,0xFF},
                                 {0x00,0xFF,0xFF,0x00,0},{0x00,0xFF,0xFF,0x00,0xFF},{0x00,0xFF,0xFF,0xFF,0},{0x00,0xFF,0xFF,0xFF,0xFF},
                                 {0xFF,0x00,0x00,0x00,0},{0xFF,0x00,0x00,0x00,0xFF},{0xFF,0x00,0x00,0xFF,0},{0xFF,0x00,0x00,0xFF,0xFF},
                                 {0xFF,0x00,0xFF,0x00,0},{0xFF,0x00,0xFF,0x00,0xFF},{0xFF,0x00,0xFF,0xFF,0},{0xFF,0x00,0xFF,0xFF,0xFF},
                                 {0xFF,0xFF,0x00,0x00,0},{0xFF,0xFF,0x00,0x00,0xFF},{0xFF,0xFF,0x00,0xFF,0},{0xFF,0xFF,0x00,0xFF,0xFF},
                                 {0xFF,0xFF,0xFF,0x00,0},{0xFF,0xFF,0xFF,0x00,0xFF},{0xFF,0xFF,0xFF,0xFF,0},{0xFF,0xFF,0xFF,0xFF,0xFF}};

// Used for line-by-line recoding and scaling of the characters of the auxiliary display line.
// All 32 possible variants of a 5-pixel wide character bitmap string are converted into
// the corresponding 5 OLED display pixels with 16 color gradations and packed into 3 bytes.

const uint8_t Upscale2[32][3] = {{0x00,0x00,0x00},{0x00,0x00,0xF0},{0x00,0x0F,0x00},{0x00,0x0F,0xF0},{0x00,0xF0,0x00},{0x00,0xF0,0xF0},
                                 {0x00,0xFF,0x00},{0x00,0xFF,0xF0},{0x0F,0x00,0x00},{0x0F,0x00,0xF0},{0x0F,0x0F,0x00},{0x0F,0x0F,0xF0},
                                 {0x0F,0xF0,0x00},{0x0F,0xF0,0xF0},{0x0F,0xFF,0x00},{0x0F,0xFF,0xF0},{0xF0,0x00,0x00},{0xF0,0x00,0xF0},
                                 {0xF0,0x0F,0x00},{0xF0,0x0F,0xF0},{0xF0,0xF0,0x00},{0xF0,0xF0,0xF0},{0xF0,0xFF,0x00},{0xF0,0xFF,0xF0},
                                 {0xFF,0x00,0x00},{0xFF,0x00,0xF0},{0xFF,0x0F,0x00},{0xFF,0x0F,0xF0},{0xFF,0xF0,0x00},{0xFF,0xF0,0xF0},
                                 {0xFF,0xFF,0x00},{0xFF,0xFF,0xF0}};

// A 256x5 pixel sprite (128x5 bytes) is used to draw the annunciators by copying rectangular areas into the OLED buffer.
const uint8_t Sprites[5][128] =	{
	      { 0x00,0x00,0x00,0xF0,0xF0,0xF0,0x00,0x00,0x00,0xF0,0xFF,0xF0,0x0F,0x00,0x00,0x0F,0x00,0xF0,0xF0,0xFF,0xF0,0xF0,0x00,0xF0,0x00,0xF0,0x0F,0xFF,0x00,0xF0,0x0F,0x0F,
	        0x0F,0x0F,0x00,0xF0,0x00,0x0F,0xFF,0x00,0xFF,0xF0,0x00,0xF0,0x00,0xF0,0x0F,0x00,0xFF,0xF0,0x00,0x0F,0xF0,0x00,0xFF,0xFF,0x00,0x0F,0xFF,0x0F,0xF0,0x0F,0xF0,0x00,
	        0x0F,0x0F,0x00,0xF0,0xFF,0xF0,0xF0,0x00,0xFF,0x0F,0xF0,0x00,0xF0,0x00,0xFF,0x00,0xFF,0x00,0xF0,0x0F,0xF0,0x0F,0xFF,0x0F,0x00,0xF0,0xFF,0xF0,0x0F,0x00,0xF0,0x00,
	        0x00,0xFF,0xF0,0x0F,0xF0,0x0F,0xFF,0x0F,0x00,0x0F,0x00,0xFF,0xF0,0xF0,0x0F,0x0F,0x00,0x00,0xF0,0xFF,0xF0,0xF0,0x0F,0x00,0x0F,0xFF,0x0F,0xF0,0x0F,0xFF,0x00,0x00 },
	      { 0x00,0x00,0x00,0x0F,0xFF,0x00,0x00,0x00,0x00,0xF0,0xF0,0x0F,0x0F,0x00,0x00,0xF0,0xF0,0xF0,0xF0,0x0F,0x0F,0x0F,0x00,0xF0,0x0F,0x0F,0x0F,0x0F,0x00,0xFF,0x0F,0x0F,
	        0x0F,0x0F,0x00,0xF0,0x00,0x0F,0x00,0xF0,0xF0,0x00,0x00,0xFF,0x0F,0xF0,0xF0,0xF0,0x0F,0x00,0x00,0xF0,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x0F,0x0F,0x0F,0x00,
	        0x0F,0x0F,0xF0,0xF0,0xF0,0x0F,0x0F,0x00,0xF0,0x0F,0x0F,0x0F,0x0F,0x00,0xF0,0xF0,0xF0,0x0F,0x0F,0x0F,0x0F,0x0F,0x00,0x0F,0x0F,0x0F,0x0F,0x00,0x0F,0x0F,0x0F,0x00,
	        0x0F,0x00,0x00,0x0F,0x0F,0x0F,0x00,0x0F,0xF0,0xFF,0x00,0x0F,0x00,0xF0,0x0F,0x0F,0x00,0x00,0xF0,0x0F,0x00,0xFF,0x0F,0x00,0x0F,0x00,0x0F,0x0F,0x0F,0x0F,0x00,0x00 },
	      { 0x00,0x00,0x0F,0xFF,0x0F,0xFF,0x00,0x00,0x00,0xF0,0xF0,0x0F,0x0F,0x00,0x00,0xFF,0xF0,0xF0,0xF0,0x0F,0x0F,0x0F,0x00,0xF0,0x0F,0x0F,0x0F,0xFF,0x00,0xF0,0xFF,0x0F,
	        0x0F,0x0F,0x00,0xF0,0x00,0x0F,0x00,0xF0,0xFF,0x00,0x00,0xF0,0xF0,0xF0,0xFF,0xF0,0x0F,0x00,0x00,0xFF,0xFF,0x00,0x00,0xF0,0x00,0x0F,0xF0,0x0F,0x0F,0x0F,0x0F,0x00,
	        0x0F,0x0F,0x0F,0xF0,0xFF,0x0F,0x0F,0x00,0xFF,0x0F,0x0F,0x0F,0x0F,0x00,0xF0,0xF0,0xFF,0x0F,0xFF,0x0F,0x0F,0x0F,0xFF,0x0F,0x0F,0x0F,0x0F,0x00,0x0F,0x0F,0x0F,0x0F,
	        0x0F,0x0F,0xF0,0x0F,0x0F,0x0F,0xF0,0x0F,0x0F,0x0F,0x00,0x0F,0x00,0xF0,0x0F,0xF0,0x00,0x00,0xF0,0x0F,0x00,0xF0,0xFF,0x00,0x0F,0xFF,0x0F,0x0F,0x0F,0x0F,0x00,0x00 },
	      { 0x00,0x00,0x00,0x0F,0xFF,0x00,0x00,0x00,0x00,0xF0,0xF0,0x0F,0x0F,0x00,0x00,0xF0,0xF0,0xF0,0xF0,0x0F,0x0F,0x0F,0x00,0xF0,0x0F,0x0F,0x0F,0x00,0x00,0xF0,0x0F,0x0F,
	        0x0F,0x0F,0x00,0xF0,0x00,0x0F,0x00,0xF0,0xF0,0x00,0x00,0xF0,0x00,0xF0,0xF0,0xF0,0x0F,0x00,0x00,0xF0,0x0F,0x00,0x0F,0x00,0x00,0x0F,0x00,0x0F,0xF0,0x0F,0xF0,0x00,
	        0x0F,0x0F,0x00,0xF0,0xF0,0x0F,0x0F,0x00,0xF0,0x0F,0xF0,0x0F,0x0F,0x00,0xFF,0x00,0xF0,0x0F,0x0F,0x0F,0xF0,0x00,0x0F,0x0F,0x0F,0x0F,0x0F,0x00,0x0F,0x0F,0x0F,0x00,
	        0x0F,0x00,0xF0,0x0F,0xF0,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0xF0,0x0F,0x0F,0x00,0x00,0xF0,0x0F,0x00,0xF0,0x0F,0x00,0x00,0x0F,0x0F,0xF0,0x0F,0x0F,0x00,0x00 },
	      { 0x00,0x00,0x00,0xF0,0xF0,0xF0,0x00,0x00,0x00,0xF0,0xFF,0xF0,0x0F,0xFF,0x00,0xF0,0xF0,0xFF,0xF0,0x0F,0x00,0xF0,0x00,0xFF,0xF0,0xF0,0x0F,0x00,0x00,0xF0,0x0F,0x0F,
	        0xFF,0x0F,0xF0,0xFF,0x00,0x0F,0xFF,0x00,0xF0,0x00,0x00,0xF0,0x00,0xF0,0xF0,0xF0,0x0F,0x00,0x00,0xF0,0x0F,0x00,0xFF,0xFF,0x00,0x0F,0xFF,0x0F,0x0F,0x0F,0x0F,0x00,
	        0x0F,0x0F,0x00,0xF0,0xF0,0x00,0xF0,0x00,0xF0,0x0F,0x0F,0x00,0xF0,0x00,0xF0,0xF0,0xFF,0x0F,0x0F,0x0F,0x0F,0x0F,0xFF,0x0F,0xF0,0xF0,0x0F,0x00,0x0F,0xF0,0xF0,0x00,
	        0x00,0xFF,0xF0,0x0F,0x0F,0x0F,0xFF,0x0F,0x00,0x0F,0x00,0x0F,0x00,0xFF,0x0F,0x0F,0x00,0x00,0xFF,0x0F,0x00,0xF0,0x0F,0x00,0x0F,0xFF,0x0F,0x0F,0x0F,0xF0,0xF0,0x00 } };

// Declare bytes array for a OLED frame buffer.
// Dimensions are divided by 2 because one byte contains two 4-bit grayscale pixels
uint8_t tx_buffer[64*256/2];

// Flag indicating finish of SPI transmission to OLED
volatile uint8_t SPI1_TX_completed_flag = 1; 

// Flag indicating finish of SPI start-up initialization
volatile uint8_t Init_Completed_flag = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//******************************************************************************

//SPI transmission finished interrupt callback
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi1)
{
	SPI1_TX_completed_flag = 1;
}
//******************************************************************************
static uint8_t InverseByte(uint8_t a)
{
    a = ((a & 0x55) << 1) | ((a & 0xAA) >> 1);
    a = ((a & 0xCC) >> 2) | ((a & 0x33) << 2);
    return (a >> 4) | (a << 4);
}
//******************************************************************************

// Each character on the display is encoded by a matrix of 40 bits packed
// into 5 consecutive bytes. 5x7=35 bits (S1-S35) define the pixel image of the character,
// 1 bit (S36) is the annunciator, 4 bits are not used. To optimize VFD PCB routing,
// the bit order in packets is shuffled:
//
// S17 S16 S15 S14 S13 S12 S11 S10
// S9  S8  S7  S6  S5  S4  S3  S2
// S1  S36 0   0   0   0   S35 S34
// S33 S32 S31 S30 S29 S28 S27 S26
// S25 S24 S23 S22 S21 S20 S19 S18
//
// The Packets_to_chars function sorts the character bitmap, extracts the annunciator
// flag, and stores the result in separate arrays chars[][] and flags[]
//
// 0 0 0 S1  S2  S3  S4  S5
// 0 0 0 S6  S7  S8  S9  S10
// 0 0 0 S11 S12 S13 S14 S15
// 0 0 0 S16 S17 S18 S19 S20
// 0 0 0 S21 S22 S23 S24 S25
// 0 0 0 S26 S27 S28 S29 S30
// 0 0 0 S31 S32 S33 S34 S35
//
void Packets_to_chars (void)
{
  for (int i=0; i<PACKET_COUNT; i++) {
    uint8_t d0 = rx_buffer[i*PACKET_WIDTH+0];
    uint8_t d1 = rx_buffer[i*PACKET_WIDTH+1];
    uint8_t d2 = rx_buffer[i*PACKET_WIDTH+2];
    uint8_t d3 = rx_buffer[i*PACKET_WIDTH+3];
    uint8_t d4 = rx_buffer[i*PACKET_WIDTH+4];
    chars[Reorder[i]][0] = 0x1F & InverseByte((d1 << 4) | ((d2 & 0x80) >> 4));
    chars[Reorder[i]][1] = 0x1F & InverseByte((d0 << 7) | ((d1 & 0xF0) >> 1));
    chars[Reorder[i]][2] = 0x1F & InverseByte((d0 & 0xFE) <<2);
    chars[Reorder[i]][3] = 0x1F & InverseByte(((d0 & 0xC0) >>3) | (d4 << 5));
    chars[Reorder[i]][4] = 0x1F & InverseByte(d4 & 0xF8);
    chars[Reorder[i]][5] = 0x1F & InverseByte(d3 << 3);
    chars[Reorder[i]][6] = 0x1F & InverseByte((d2 <<6) | ((d3 & 0xE0) >> 2));
    flags[Reorder[i]] = (d2 & 0x40) == 0x40;	  
  };
}

//******************************************************************************
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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  SSD1322_API_init();
  Init_Completed_flag = 1;                      // Now is a safe time to enable the EXTI interrupt handler
  set_buffer_size(256, 64);                     // SSD1322 OLED size

  while (1)
  {
	  Packets_to_chars();
	  fill_buffer(tx_buffer, 0);                // Clearing OLED frame buffer
	  // Drawing (recoding, horizontal and vertical scaling) of the characters of the main display line
	  for (int i=0; i<CHAR_HEIGHT; i++)
		  for (int j=0; j<LINE1_LEN; j++)
			  for (int k = 0; k<5; k++)
				  tx_buffer[2+j*7+k+(LINE1_Y+i*5+0)*128] = tx_buffer[2+j*7+k+(LINE1_Y+i*5+1)*128] =
				  tx_buffer[2+j*7+k+(LINE1_Y+i*5+2)*128] = tx_buffer[2+j*7+k+(LINE1_Y+i*5+3)*128] = Upscale1[chars[j][i]][k];
	  // Drawing (recoding and vertical scaling) of the characters of the auxiliary display line.
	  for (int i=0; i<CHAR_HEIGHT; i++)
		  for (int j=0; j<LINE2_LEN; j++)
			  for (int k = 0; k<3; k++)
				  tx_buffer[7+j*4+k+(LINE2_Y+i*2+0)*128] = tx_buffer[7+j*4+k+(LINE2_Y+i*2+1)*128] = Upscale2[chars[j+LINE1_LEN][i]][k];
	  // Drawing of the annunciators
	  for (int k=0; k<LINE1_LEN; k++)
		  if (flags[k])
			  for (int i=0; i<5; i++)
				  for (int j=0; j<7; j++)
					  tx_buffer[1+j+k*7+i*128] = Sprites[i][1+k*7+j];
	  send_buffer_to_OLED(tx_buffer, 0, 0);     // Send the frame buffer content to OLED
	  HAL_GPIO_TogglePin(GPIOC, TEST_OUT_Pin);	// Test LED toggle
  };
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
