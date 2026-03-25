/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for the Sokoban game.
  *
  * Initialises all MCU peripherals (SPI1, DMA, GPIO), starts the ST7789 display
  * driver and the DWT cycle-counter timer, then runs the main game loop.
  * The loop dispatches input events (set by GPIO EXTI callbacks) and drives the
  * game state machine: Title → Playing → Level Won / Game Won → Title.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "st7789.h"

#include "levels.h"
#include "renderer.h"
#include "box.h"



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
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Initialises the DWT cycle counter for microsecond timing.
 *
 * Enables the trace macrocell (TRCENA), resets the cycle count register, and
 * starts the cycle counter.  Must be called once before GetDeltaTime().
 */
void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; /* Enable trace */
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            /* Start counter */
}

/**
 * @brief Returns the elapsed time in microseconds since the last call.
 *
 * Uses the DWT cycle counter to measure elapsed CPU cycles and converts them
 * to microseconds using @c SystemCoreClock.  Sub-microsecond cycles are
 * accumulated in a static remainder to avoid drift over time.
 *
 * @return Elapsed time in microseconds since the previous call, or 0 on the
 *         first call (used to prime the lastCycle baseline).
 */
uint32_t GetDeltaTime(void) {
    static uint32_t lastCycle = 0;
    static uint32_t remainder = 0;
    uint32_t now = DWT->CYCCNT;
    uint32_t elapsed = now - lastCycle;
    lastCycle = now;

    /* Convert cycles to microseconds, preserving the sub-µs remainder */
    uint32_t totalCycles = elapsed + remainder;
    uint32_t cyclesPerUs = SystemCoreClock / 1000000U;
    if (cyclesPerUs == 0) return 0;

    uint32_t us = totalCycles / cyclesPerUs;
    remainder = totalCycles - (us * cyclesPerUs);
    return us;
}

#include "player.h"
#include <stdbool.h>

/** @brief Pool of box entities shared across all levels. */
static Box    boxes[BOX_MAX];

/** @brief The player entity. */
static Player player;

/**
 * @brief Top-level game state machine states.
 */
typedef enum {
    STATE_TITLE,      /**< Title screen is displayed; waiting for the OK button. */
    STATE_PLAYING,    /**< A level is active and the player can move. */
    STATE_LEVEL_WON,  /**< Current level is complete; waiting to advance. */
    STATE_GAME_WON    /**< All levels finished; game-complete screen is shown. */
} GameState;

/** @brief Current state of the game state machine. */
static GameState gameState = STATE_TITLE;

/** @brief Zero-based index of the level currently loaded or to be loaded next. */
static uint8_t currentLevelIdx = 0;

/**
 * @brief Set to @c true by the OK-button EXTI callback to request a start/advance action.
 *
 * Consumed and cleared at the top of the main loop to avoid missed events.
 */
volatile bool requestStart = false;

/**
 * @brief Set to @c true by the Back-button EXTI callback to return to the title screen.
 *
 * Takes priority over @c requestStart when both are set simultaneously.
 */
volatile bool requestTitle = false;

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
  /* USER CODE BEGIN 2 */

  DWT_Init();
  ST7789_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  Renderer_DrawTitleScreen();
  gameState = STATE_TITLE;

  GetDeltaTime();
  static uint32_t acc_us = 0;
  static bool levelWon = false;

  while (1) {
    /* USER CODE END WHILE */
    uint32_t dt = GetDeltaTime();
    acc_us += dt;

    if (requestTitle) {
        requestTitle = false;
        requestStart = false; // prioritize title
        gameState = STATE_TITLE;
        Renderer_DrawTitleScreen();
    }

    if (requestStart) {
        requestStart = false;
        if (gameState == STATE_TITLE) {
            Level_Load(currentLevelIdx, &player, boxes, BOX_MAX);
            gameState = STATE_PLAYING;
            levelWon = false;
        } else if (gameState == STATE_LEVEL_WON) {
            currentLevelIdx++;
            Level_Load(currentLevelIdx, &player, boxes, BOX_MAX);
            gameState = STATE_PLAYING;
            levelWon = false;
        } else if (gameState == STATE_GAME_WON) {
            currentLevelIdx = 0;
            gameState = STATE_TITLE;
            Renderer_DrawTitleScreen();
            levelWon = false;
        }
    }

    if (gameState == STATE_PLAYING) {
        Player_Update(&player, dt);

        /* Update all boxes */
        for (uint8_t i = 0; i < BOX_MAX; i++)
            Box_Update(&boxes[i], dt);

        /* Check win condition */
        if (!levelWon && Level_IsComplete(boxes, BOX_MAX)) {
            levelWon = true;

            if (currentLevelIdx < Level_GetCount() - 1) {
                gameState = STATE_LEVEL_WON;
                Renderer_DrawLevelCompleteScreen();
            } else {
                gameState = STATE_GAME_WON;
                Renderer_DrawGameCompleteScreen();
            }
        } else if (levelWon && !Level_IsComplete(boxes, BOX_MAX)) {
            levelWon = false; // Reset if somehow un-won
        }

        /* --- Redraw --- */
        bool needRedraw = player.frameChanged || player.posChanged;
        for (uint8_t i = 0; i < BOX_MAX; i++)
            if (boxes[i].posChanged) needRedraw = true;

        if (needRedraw && !levelWon) {
            const LevelDesc* desc = Level_GetDesc(currentLevelIdx);
            if (desc) {
                if (player.state >= ANIM_WALK_DOWN && player.pushedBoxIdx >= 0) {
                    Renderer_DrawPushStrip(desc->tiles, &player,
                                           &boxes[player.pushedBoxIdx],
                                           boxes, BOX_MAX);
                } else {
                    Renderer_DrawPlayerStrip(desc->tiles, &player, boxes, BOX_MAX);

                    for (uint8_t i = 0; i < BOX_MAX; i++) {
                        if (boxes[i].posChanged)
                            Renderer_DrawBoxes(desc->tiles, &boxes[i], 1);
                    }
                }
            }
        }
    }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA8 PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA10 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief GPIO EXTI interrupt callback — handles all button presses.
 *
 * Called by the HAL EXTI driver after any of the configured falling-edge
 * interrupts fires.  The button-to-pin mapping is:
 *
 * | Pin      | Button | Action                                  |
 * |----------|--------|-----------------------------------------|
 * | PB3      | Back   | Return to title from any in-game state  |
 * | PA10     | OK     | Start game / advance level / restart    |
 * | PA15     | Up     | Face up; then walk up on second press   |
 * | PB7      | Right  | Face right; then walk right             |
 * | PB8      | Down   | Face down; then walk down               |
 * | PB9      | Left   | Face left; then walk left               |
 *
 * Direction buttons use a two-press scheme: the first press rotates the
 * player's facing direction (idle state), and the second press (while already
 * facing that way) initiates movement.
 *
 * @param GPIO_Pin Bitmask of the pin that triggered the interrupt.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == GPIO_PIN_3) { /* BACK */
      if (gameState == STATE_PLAYING || gameState == STATE_LEVEL_WON || gameState == STATE_GAME_WON) {
          requestTitle = true;
      }
  } else if (GPIO_Pin == GPIO_PIN_10) { /* OK */
      if (gameState == STATE_TITLE || gameState == STATE_LEVEL_WON || gameState == STATE_GAME_WON) {
          requestStart = true;
      }
  } else if (gameState == STATE_PLAYING) {
      if (GPIO_Pin == GPIO_PIN_15) {        /* UP */
          if (player.state <= ANIM_IDLE_LEFT && player.state != ANIM_IDLE_UP) {
              player.state = ANIM_IDLE_UP;
              player.frame = 0;
              player.frameChanged = true;
          } else if (player.state == ANIM_IDLE_UP) {
              const LevelDesc* desc = Level_GetDesc(currentLevelIdx);
              if (desc) Player_StartWalk(&player, ANIM_WALK_UP, desc->tiles, boxes, BOX_MAX);
          }
      } else if (GPIO_Pin == GPIO_PIN_7) {  /* RIGHT */
          if (player.state <= ANIM_IDLE_LEFT && player.state != ANIM_IDLE_RIGHT) {
              player.state = ANIM_IDLE_RIGHT;
              player.frame = 0;
              player.frameChanged = true;
          } else if (player.state == ANIM_IDLE_RIGHT) {
              const LevelDesc* desc = Level_GetDesc(currentLevelIdx);
              if (desc) Player_StartWalk(&player, ANIM_WALK_RIGHT, desc->tiles, boxes, BOX_MAX);
          }
      } else if (GPIO_Pin == GPIO_PIN_8) {  /* DOWN */
          if (player.state <= ANIM_IDLE_LEFT && player.state != ANIM_IDLE_DOWN) {
              player.state = ANIM_IDLE_DOWN;
              player.frame = 0;
              player.frameChanged = true;
          } else if (player.state == ANIM_IDLE_DOWN) {
              const LevelDesc* desc = Level_GetDesc(currentLevelIdx);
              if (desc) Player_StartWalk(&player, ANIM_WALK_DOWN, desc->tiles, boxes, BOX_MAX);
          }
      } else if (GPIO_Pin == GPIO_PIN_9) {  /* LEFT */
          if (player.state <= ANIM_IDLE_LEFT && player.state != ANIM_IDLE_LEFT) {
              player.state = ANIM_IDLE_LEFT;
              player.frame = 0;
              player.frameChanged = true;
          } else if (player.state == ANIM_IDLE_LEFT) {
              const LevelDesc* desc = Level_GetDesc(currentLevelIdx);
              if (desc) Player_StartWalk(&player, ANIM_WALK_LEFT, desc->tiles, boxes, BOX_MAX);
          }
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
#ifdef USE_FULL_ASSERT
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
