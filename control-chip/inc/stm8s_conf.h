/**
  ******************************************************************************
  * @file    stm8s_conf.h
  * @author  MCD Application Team
  * @version V2.2.0
  * @date    30-September-2014
  * @brief   This file is used to configure the Library.
  ******************************************************************************
  */

#ifndef __STM8S_CONF_H
#define __STM8S_CONF_H

/* Includes ------------------------------------------------------------------*/
#include "stm8s.h"

/* Uncomment the line below to enable peripheral header file inclusion */
#define _UART1_H
#define _GPIO_H
#define _CLK_H

/* Comment the line below to disable the specific peripheral inclusion */

/**
 * @brief  In the following line adjust the value of External High Speed oscillator (HSE)
   used in your application
   
   Tip: To avoid modifying this file each time you need to use different HSE, you
        can define the HSE value in your toolchain compiler preprocessor.
  */
#if !defined  HSE_VALUE
#define HSE_VALUE   ((uint32_t)16000000) /*!< Value of the External oscillator in Hz*/
#endif

/**
 * @brief  In the following line adjust the External High Speed oscillator (HSE) Startup 
   Timeout value 
   */
#define HSE_STARTUP_TIMEOUT   ((uint16_t)0x0500) /*!< Time out for HSE start up */

/**
 * @brief  Definition of Device on-board crystal oscillator clock frequency (in Hz)
   */
#define HSI_VALUE   ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Uncomment the line below to expanse the "assert_param" macro in the 
   Standard Peripheral Library drivers code */
/* #define USE_FULL_ASSERT    (1) */ 

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT

/**
  * @brief  The assert_param macro is used for function's parameters check.
  * @param expr: If expr is false, it calls assert_failed function
  *   which reports the name of the source file and the source
  *   line number of the call that failed. 
  *   If expr is true, it returns no value.
  * @retval : None
  */
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */

#endif /* __STM8S_CONF_H */