/**************************************************************************************************
  Filename:       sb_main.c
  Revised:        $Date: 2012-03-29 12:09:02 -0700 (Thu, 29 Mar 2012) $
  Revision:       $Revision: 29943 $

  Description:    This module contains the main functionality of a Boot Loader for CC2530.
                  It is a minimal subset of functionality from ZMain.c, OnBoard.c and various
                  _hal_X.c modules for the CC2530ZNP target.


  Copyright 2009-2012 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_board_cfg.h"
#include "hal_adc.h"
#include "hal_dma.h"
#include "hal_flash.h"
#include "hal_types.h"
#include "sb_exec.h"
#include "sb_main.h"

/* ------------------------------------------------------------------------------------------------
 *                                          Constants
 * ------------------------------------------------------------------------------------------------
 */

/* Delay jump to valid RC code, waiting for a force boot or force run indication via the
 * physical transport or button press indication. Set to zero to jump immediately, this
 * necessitates the RC to invalidate checksum/shadow to force boot mode.
 */
#if !defined SB_UART_DELAY
#define SB_UART_DELAY  0x260000  // About 1 minute.
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

#if !defined ResetWasWatchDog
#define ResetWasWatchDog ((SLEEPSTA & 0x18) == 0x10)
#endif

/* ------------------------------------------------------------------------------------------------
 *                                       Global Variables
 * ------------------------------------------------------------------------------------------------
 */

halDMADesc_t dmaCh0;

/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */

static uint8 znpCfg1;
static uint8 spiPoll;

/* ------------------------------------------------------------------------------------------------
 *                                       Local Functions
 * ------------------------------------------------------------------------------------------------
 */

static void sblExec(void);
static void sblInit(void);
static void sblJump(void);
static void sblWait(void);
static void vddWait(uint8 vdd);

#include "_hal_uart_isr.c"
#include "_hal_uart_spi.c"

/**************************************************************************************************
 * @fn          main
 *
 * @brief       C-code main functionality.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void main(void)
{
  vddWait(VDD_MIN_RUN);
  HAL_BOARD_INIT();

  if (sbImgValid())
  {
    if ((SB_UART_DELAY == 0) || ResetWasWatchDog)
    {
      sblJump();
    }

    sblInit();
    sblWait();
  }
  else
  {
    sblInit();
  }

  vddWait(VDD_MIN_NV);
  sblExec();
  HAL_SYSTEM_RESET();
}

/**************************************************************************************************
 * @fn          sblExec
 *
 * @brief       Infinite SBL execute loop that jumps upon receiving a code enable.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void sblExec(void)
{
  uint32 dlyCnt = 0;

  while (1)
  {
    if (znpCfg1 == ZNP_CFG1_UART)
    {
      HalUARTPollISR();
    }

    if (sbExec() && sbImgValid())
    {
      // Delay to allow the SB_ENABLE_CMD response to be flushed.
      if (znpCfg1 == ZNP_CFG1_UART)
      {
        for (dlyCnt = 0; dlyCnt < 0x40000; dlyCnt++)
        {
            HalUARTPollISR();
        }
      }

      sblJump();
    }
  }
}

/**************************************************************************************************
 * @fn          sblInit
 *
 * @brief       SBL initialization.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void sblInit(void)
{
#if defined CC2530_MK
  znpCfg1 = ZNP_CFG1_SPI;
#else
  znpCfg1 = P2_0;
#endif

  /* This is in place of calling HalDmaInit() which would require init of the other 4 DMA
   * descriptors in addition to just Channel 0.
   */
  HAL_DMA_SET_ADDR_DESC0(&dmaCh0);


  if (znpCfg1 == ZNP_CFG1_SPI)
  {
    SRDY_CLR();

    // Select general purpose on I/O pins.
    P0SEL &= ~(NP_RDYIn_BIT);      // P0.3 MRDY - GPIO
    P0SEL &= ~(NP_RDYOut_BIT);     // P0.4 SRDY - GPIO

    // Select GPIO direction.
    P0DIR &= ~NP_RDYIn_BIT;        // P0.3 MRDY - IN
    P0DIR |= NP_RDYOut_BIT;        // P0.4 SRDY - OUT

    P0INP &= ~NP_RDYIn_BIT;        // Pullup/down enable of MRDY input.
    P2INP &= ~BV(5);               // Pullup all P0 inputs.

    HalUARTInitSPI();
  }
  else
  {
    halUARTCfg_t uartConfig;

    HalUARTInitISR();
    uartConfig.configured           = TRUE;
    uartConfig.baudRate             = HAL_UART_BR_115200;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 0;  // CC2530 by #define - see hal_board_cfg.h
    uartConfig.rx.maxBufSize        = 0;  // CC2530 by #define - see hal_board_cfg.h
    uartConfig.tx.maxBufSize        = 0;  // CC2530 by #define - see hal_board_cfg.h
    uartConfig.idleTimeout          = 0;  // CC2530 by #define - see hal_board_cfg.h
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = NULL;
    HalUARTOpenISR(&uartConfig);
  }
}

/**************************************************************************************************
 * @fn          sblJump
 *
 * @brief       Execute a simple long jump from non-banked SBL code to non-banked RC code space.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
static void sblJump(void)
{
  asm("LJMP 0x2000\n");  // Immediate jump to run-code.
  HAL_SYSTEM_RESET();
}

/**************************************************************************************************
 * @fn          sblWait
 *
 * @brief       A timed-out wait loop that exits early upon receiving a force code/sbl byte.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void sblWait(void)
{
  uint32 dlyCnt;

  if (znpCfg1 == ZNP_CFG1_SPI)
  {
    // Slave signals ready for read by setting its ready flag first.
    SRDY_SET();
    // Flag to sbRx() to poll for 1 Rx byte instead of blocking read until MRDY_CLR.
    spiPoll = TRUE;
    dlyCnt = 0x38;  // About 50 msecs.
  }
  else
  {
    dlyCnt = SB_UART_DELAY;
  }

  while (--dlyCnt)
  {
    uint8 ch;

    if (znpCfg1 == ZNP_CFG1_UART)
    {
      HalUARTPollISR();
    }

    if (sbRx(&ch, 1))
    {
      if (ch == SB_FORCE_BOOT)
      {
        break;
      }
      else if (ch == SB_FORCE_RUN)
      {
        dlyCnt = 0;
      }
    }
  }

  if (znpCfg1 == ZNP_CFG1_SPI)
  {
    // Master blocks waiting for slave to clear its ready flag before continuing.
    SRDY_CLR();
    // Flag to sbRx() to now block while reading Rx bytes until MRDY_CLR.
    spiPoll = FALSE;
  }

  if (dlyCnt == 0)
  {
    sblJump();
  }
}

/**************************************************************************************************
 * @fn          sbRx
 *
 * @brief       Serial Boot loader read API that makes the low-level read according to RPC mode.
 *
 * input parameters
 *
 * @param       buf - Pointer to a buffer to fill with up to 'len' bytes.
 * @param       len - Maximum count of bytes to fill into the 'buf'.
 *
 *
 * output parameters
 *
 * None.
 *
 * @return      The count of the number of bytes filled into the 'buf'.
 **************************************************************************************************
 */
uint16 sbRx(uint8 *buf, uint16 len)
{
  if (znpCfg1 == ZNP_CFG1_UART)
  {
    return HalUARTReadISR(buf, len);
  }
  else
  {
    if (spiPoll)
    {
      if (URXxIF)
      {
        *buf = UxDBUF;
        URXxIF = 0;
        return 1;
      }
      else
      {
        return 0;
      }
    }
    else
    {
      return HalUARTReadSPI(buf, len);
    }
  }
}

/**************************************************************************************************
 * @fn          sbTx
 *
 * @brief       Serial Boot loader write API that makes the low-level write according to RPC mode.
 *
 * input parameters
 *
 * @param       buf - Pointer to a buffer of 'len' bytes to write to the serial transport.
 * @param       len - Length in bytes of the 'buf'.
 *
 *
 * output parameters
 *
 * None.
 *
 * @return      The count of the number of bytes written from the 'buf'.
 **************************************************************************************************
 */
uint16 sbTx(uint8 *buf, uint16 len)
{
  if (znpCfg1 == ZNP_CFG1_UART)
  {
    return HalUARTWriteISR(buf, len);
  }
  else
  {
    return HalUARTWriteSPI(buf, len);
  }
}

/**************************************************************************************************
 * @fn          vddWait
 *
 * @brief       Loop waiting for 16 reads of the Vdd over the requested limit.
 *
 * input parameters
 *
 * @param       vdd - Vdd level to wait for.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void vddWait(uint8 vdd)
{
  uint8 cnt = 16;

  do {
    do {
      ADCCON3 = 0x0F;
      while (!(ADCCON1 & 0x80));
    } while (ADCH < vdd);
  } while (--cnt);
}

/**************************************************************************************************
*/
