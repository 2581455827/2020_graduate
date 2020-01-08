/******************************************************************************
  Filename:       SampleAppHw.c
  Revised:        $Date: 2010-11-15 18:17:40 -0800 (Mon, 15 Nov 2010) $
  Revision:       $Revision: 24415 $

  Description:    Hardware setup for SampleApp


  Copyright 2008-2010 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
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
******************************************************************************/

/******************************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "hal_mcu.h"
#include "hal_defs.h"

#include "SampleAppHw.h"

/******************************************************************************
 * CONSTANTS
 */

/* NOTE:  A jumper on SmartRF05 header P18 is used to select device start-up as
 *        a ZigBee Coordinator. The jumper connects GPIO pins P0.2 and P0.3:
 *        On SmartRF05 Rev 1.3 or earlier:  P0.2=P18 pin 7, P0.3=P18 pin 9
 *        On SmartRF05 Rev 1.7 or later:    P0.2=P18 pin 9, P0.3=P18 pin 11
 */

#define JUMPERIN_BIT  BV(2)  //P0.2
#define JUMPERIN_SEL  P0SEL
#define JUMPERIN_DIR  P0DIR
#define JUMPERIN      P0

#define JUMPEROUT_BIT BV(3)  //P0.3
#define JUMPEROUT_SEL P0SEL
#define JUMPEROUT_DIR P0DIR
#define JUMPEROUT     P0

/******************************************************************************
 * @fn      readCoordinatorJumper
 *
 * @brief   Checks for a jumper between 2 GPIO pins to determine if the device
 *          should start-up as a ZigBee Coordinator
 *
 * @return  TRUE if the jumper is there, FALSE if not
 */
uint8 readCoordinatorJumper( void )
{
  uint8 i,j;
  uint8 result;

  JUMPERIN_SEL  &= ~(JUMPERIN_BIT);
  JUMPERIN_DIR  &= ~(JUMPERIN_BIT);

  JUMPEROUT_SEL &= ~(JUMPEROUT_BIT);
  JUMPEROUT_DIR |= JUMPEROUT_BIT;

  // Start with output bit OFF
  JUMPEROUT &= ~(JUMPEROUT_BIT);

  for ( i = 0; i < 8; i++ )
  {
    for ( j = 0; j < 8; j++ )
    {
      // Burn time for input to see change
      result = JUMPERIN & JUMPERIN_BIT;
    }

    j = i & 0x01;
    if (((j == 0) && (result != 0)) ||
        ((j != 0) && (result != JUMPERIN_BIT)))
    {
      return ( FALSE );
    }

    // Toggle the output bit
    JUMPEROUT ^= JUMPEROUT_BIT;
  }

  return ( TRUE );
}

/******************************************************************************
 */
