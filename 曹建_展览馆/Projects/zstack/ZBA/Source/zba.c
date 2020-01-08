/**************************************************************************************************
  Filename:       zba.c
  Revised:        $Date: 2011-05-31 10:03:43 -0700 (Tue, 31 May 2011) $
  Revision:       $Revision: 26156 $

  Description:    This file contains the ZigBee Building Automation (ZBA)
                  Profile implementation.


  Copyright 2011 Texas Instruments Incorporated. All rights reserved.

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

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "ZDApp.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_cc.h"
#include "zcl_key_establish.h"

#include "zba.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// This is the ZBA Global Commissioning EPID reserved by the ZigBee Alliance
const uint8 zbaGlobalCommissioningEPID[Z_EXTADDR_LEN] = { 0x00, 0x00, 0x00, 0x10, 0x77, 0xC2, 0x50, 0x00 };

/*********************************************************************
 * Cluster Conversion List - table used to convert between real and
 * logical cluster IDs.
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      zba_Init
 *
 * @brief   Register the Simple descriptor with the ZBA profile.
 *          This function also registers the profile's cluster
 *          conversion table.
 *
 * @param   simpleDesc
 *
 * @return  none
 */
void zba_Init( SimpleDescriptionFormat_t *simpleDesc )
{
  endPointDesc_t *epDesc;

  // Register the application's endpoint descriptor
  //  - This memory is allocated and never freed.
  epDesc = osal_mem_alloc( sizeof ( endPointDesc_t ) );
  if ( epDesc )
  {
    // Fill out the endpoint description.
    epDesc->endPoint = simpleDesc->EndPoint;
    epDesc->task_id = &zcl_TaskID;   // all messages get sent to ZCL first
    epDesc->simpleDesc = simpleDesc;
    epDesc->latencyReq = noLatencyReqs;

    // Register the endpoint description with the AF
    if (afRegister(epDesc) == afStatus_SUCCESS)
    {
      // Set the ZBA Profile-specific APS Fragmentation configuration.
      afAPSF_Config_t cfg = { ZBA_APSF_FRAME_DELAY, ZBA_APSF_WINDOW_SIZE };
      (void)afAPSF_ConfigSet(epDesc->endPoint, &cfg);
    }
  }
}

/*********************************************************************
*********************************************************************/
