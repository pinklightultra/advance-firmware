/* USER CODE BEGIN Header */
/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2025
 * STMicroelectronics.
 * Copyright (c) 2025 xiphonics, inc.
 *
 * This file is
 * part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_it.h"
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Adapters/adv/system/assets/error_bitmaps.h"
#include "Adapters/adv/system/critical_error_message.h"
#include "tusb.h"
#include <System/Console/nanoprintf.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

typedef struct {
  uint32_t R0;
  uint32_t R1;
  uint32_t R2;
  uint32_t R3;
  uint32_t R12;
  uint32_t LR;
  uint32_t PC;
  uint32_t xPSR;
} HardFaultStackFrame;

typedef enum {
  FAULT_TYPE_HARD = 0,
  FAULT_TYPE_MEMMANAGE,
  FAULT_TYPE_BUS,
  FAULT_TYPE_USAGE,
} FaultType;

static void format_fault_hex_or_placeholder(char *buffer, size_t bufferSize,
                                            bool valid, uint32_t value) {
  if (valid) {
    npf_snprintf(buffer, bufferSize, "0x%08lX", (unsigned long)value);
  } else {
    npf_snprintf(buffer, bufferSize, "0x--------");
  }
}

__attribute__((used, noreturn)) static void
fault_c_handler(HardFaultStackFrame *stackFrame, FaultType faultType,
                uint32_t stackPointer) {
  char detailLine[33];
  char detailLine2[33];
  char bfarText[11];
  char mmfarText[11];
  const char *detailLines[] = {detailLine, detailLine2};

  (void)stackPointer;

  format_fault_hex_or_placeholder(bfarText, sizeof(bfarText),
                                  (SCB->CFSR & SCB_CFSR_BFARVALID_Msk) != 0U,
                                  SCB->BFAR);
  format_fault_hex_or_placeholder(mmfarText, sizeof(mmfarText),
                                  (SCB->CFSR & SCB_CFSR_MMARVALID_Msk) != 0U,
                                  SCB->MMFAR);

  npf_snprintf(detailLine, sizeof(detailLine), "PC 0x%08lX LR 0x%08lX",
               (unsigned long)stackFrame->PC, (unsigned long)stackFrame->LR);
  npf_snprintf(detailLine2, sizeof(detailLine2), "CFSR 0x%08lX HFSR 0x%08lX",
               (unsigned long)SCB->CFSR, (unsigned long)SCB->HFSR);

  if (faultType == FAULT_TYPE_BUS) {
    npf_snprintf(detailLine2, sizeof(detailLine2), "CFSR 0x%08lX BFAR %s",
                 (unsigned long)SCB->CFSR, bfarText);
  } else if (faultType == FAULT_TYPE_MEMMANAGE) {
    npf_snprintf(detailLine2, sizeof(detailLine2), "CFSR 0x%08lX MMFAR %s",
                 (unsigned long)SCB->CFSR, mmfarText);
  }

  critical_error_message(&g_deadBitmap, NULL, detailLines,
                         sizeof(detailLines) / sizeof(detailLines[0]),
                         DEFAULT_ERROR_MESSAGE_DELAY_SEC);
}
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA2D_HandleTypeDef hdma2d;
extern DMA_HandleTypeDef hdma_sai1_a;
extern DMA_HandleTypeDef hdma_sai1_b;
extern SD_HandleTypeDef hsd1;
extern TIM_HandleTypeDef htim13;
extern UART_HandleTypeDef huart7;
extern TIM_HandleTypeDef htim8;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1) {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
__attribute__((naked)) void HardFault_Handler(void) {
  __asm volatile("tst lr, #4\n"
                 "ite eq\n"
                 "mrseq r0, msp\n"
                 "mrsne r0, psp\n"
                 "mov r2, r0\n"
                 "movs r1, #0\n"
                 "b fault_c_handler\n");
}

/**
 * @brief This function handles Memory management fault.
 */
__attribute__((naked)) void MemManage_Handler(void) {
  __asm volatile("tst lr, #4\n"
                 "ite eq\n"
                 "mrseq r0, msp\n"
                 "mrsne r0, psp\n"
                 "mov r2, r0\n"
                 "movs r1, #1\n"
                 "b fault_c_handler\n");
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
__attribute__((naked)) void BusFault_Handler(void) {
  __asm volatile("tst lr, #4\n"
                 "ite eq\n"
                 "mrseq r0, msp\n"
                 "mrsne r0, psp\n"
                 "mov r2, r0\n"
                 "movs r1, #2\n"
                 "b fault_c_handler\n");
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
__attribute__((naked)) void UsageFault_Handler(void) {
  __asm volatile("tst lr, #4\n"
                 "ite eq\n"
                 "mrseq r0, msp\n"
                 "mrsne r0, psp\n"
                 "mov r2, r0\n"
                 "movs r1, #3\n"
                 "b fault_c_handler\n");
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void) {
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles DMA1 stream0 global interrupt.
 */
void DMA1_Stream0_IRQHandler(void) {
  /* USER CODE BEGIN DMA1_Stream0_IRQn 0 */

  /* USER CODE END DMA1_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_sai1_a);
  /* USER CODE BEGIN DMA1_Stream0_IRQn 1 */

  /* USER CODE END DMA1_Stream0_IRQn 1 */
}

/**
 * @brief This function handles DMA1 stream1 global interrupt.
 */
void DMA1_Stream1_IRQHandler(void) {
  /* USER CODE BEGIN DMA1_Stream1_IRQn 0 */

  /* USER CODE END DMA1_Stream1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_sai1_b);
  /* USER CODE BEGIN DMA1_Stream1_IRQn 1 */

  /* USER CODE END DMA1_Stream1_IRQn 1 */
}

/**
 * @brief This function handles EXTI line[9:5] interrupts.
 */
void EXTI9_5_IRQHandler(void) {
  /* USER CODE BEGIN EXTI9_5_IRQn 0 */

  /* USER CODE END EXTI9_5_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(CHARGER_INT_Pin);
  /* USER CODE BEGIN EXTI9_5_IRQn 1 */

  /* USER CODE END EXTI9_5_IRQn 1 */
}

/**
 * @brief This function handles EXTI line[15:10] interrupts.
 */
void EXTI15_10_IRQHandler(void) {
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  /* USER CODE END EXTI15_10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(SD_DET_Pin);
  HAL_GPIO_EXTI_IRQHandler(USB_INT_Pin);
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}

/**
 * @brief This function handles TIM8 update interrupt and TIM13 global
 * interrupt.
 */
void TIM8_UP_TIM13_IRQHandler(void) {
  /* USER CODE BEGIN TIM8_UP_TIM13_IRQn 0 */

  /* USER CODE END TIM8_UP_TIM13_IRQn 0 */
  HAL_TIM_IRQHandler(&htim8);
  HAL_TIM_IRQHandler(&htim13);
  /* USER CODE BEGIN TIM8_UP_TIM13_IRQn 1 */

  /* USER CODE END TIM8_UP_TIM13_IRQn 1 */
}

/**
 * @brief This function handles SDMMC1 global interrupt.
 */
void SDMMC1_IRQHandler(void) {
  /* USER CODE BEGIN SDMMC1_IRQn 0 */

  /* USER CODE END SDMMC1_IRQn 0 */
  HAL_SD_IRQHandler(&hsd1);
  /* USER CODE BEGIN SDMMC1_IRQn 1 */

  /* USER CODE END SDMMC1_IRQn 1 */
}

/**
 * @brief This function handles UART7 global interrupt.
 */
void UART7_IRQHandler(void) {
  /* USER CODE BEGIN UART7_IRQn 0 */

  /* USER CODE END UART7_IRQn 0 */
  HAL_UART_IRQHandler(&huart7);
  /* USER CODE BEGIN UART7_IRQn 1 */

  /* USER CODE END UART7_IRQn 1 */
}

/**
 * @brief This function handles DMA2D global interrupt.
 */
void DMA2D_IRQHandler(void) {
  /* USER CODE BEGIN DMA2D_IRQn 0 */

  /* USER CODE END DMA2D_IRQn 0 */
  HAL_DMA2D_IRQHandler(&hdma2d);
  /* USER CODE BEGIN DMA2D_IRQn 1 */

  /* USER CODE END DMA2D_IRQn 1 */
}

/* USER CODE BEGIN 1 */
/**
 * @brief This function handles USB On The Go FS global interrupt.
 */
void OTG_FS_IRQHandler(void) {
  // USB_FS is port 0 in tinyusb
  tusb_int_handler(0, true);
}
/* USER CODE END 1 */
