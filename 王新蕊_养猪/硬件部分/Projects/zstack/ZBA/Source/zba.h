/**************************************************************************************************
  Filename:       zba.h
  Revised:        $Date: 2011-05-31 10:03:43 -0700 (Tue, 31 May 2011) $
  Revision:       $Revision: 26156 $

  Description:    This file contains the ZigBee Building Automation (ZBA)
                  Profile definitions.


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

#ifndef ZBA_H
#define ZBA_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */
// Zigbee Building Automation Profile Identification
#define ZBA_PROFILE_ID                                     0x0105

// Generic Device IDs
#define ZBA_DEVICEID_ON_OFF_SWITCH                         0x0000
#define ZBA_DEVICEID_LEVEL_CONTROL_SWITCH                  0x0001
#define ZBA_DEVICEID_ON_OFF_OUTPUT                         0x0002
#define ZBA_DEVICEID_LEVEL_CONTROLLABLE_OUTPUT             0x0003
#define ZBA_DEVICEID_SCENE_SELECTOR                        0x0004
#define ZBA_DEVICEID_CONFIGURATION_TOOL                    0x0005
#define ZBA_DEVICEID_REMOTE_CONTROL                        0x0006
#define ZBA_DEVICEID_COMBINED_INTERFACE                    0x0007
#define ZBA_DEVICEID_RANGE_EXTENDER                        0x0008
#define ZBA_DEVICEID_MAINS_POWER_OUTLET                    0x0009
#define ZBA_DEVICEID_CONST_BACNET_DEVICE                   0x000A
#define ZBA_DEVICEID_BACNET_TUNNEL_DEVICE                  0x000B

// Lighting Device IDs
#define ZBA_DEVICEID_ON_OFF_LIGHT                          0x0100
#define ZBA_DEVICEID_DIMMABLE_LIGHT                        0x0101
#define ZBA_DEVICEID_COLORED_DIMMABLE_LIGHT                0x0102
#define ZBA_DEVICEID_ON_OFF_LIGHT_SWITCH                   0x0103
#define ZBA_DEVICEID_DIMMER_SWITCH                         0x0104
#define ZBA_DEVICEID_COLOR_DIMMER_SWITCH                   0x0105
#define ZBA_DEVICEID_LIGHT_SENSOR                          0x0106
#define ZBA_DEVICEID_OCCUPANCY_SENSOR                      0x0107
#define ZBA_DEVICEID_ON_OFF_BALLAST                        0x0108
#define ZBA_DEVICEID_DIMMABLE_BALLAST                      0x0109

// Closures Device IDs
#define ZBA_DEVICEID_SHADE                                 0x0200
#define ZBA_DEVICEID_SHADE_CONTROLLER                      0x0201

// HVAC Device IDs
#define ZBA_DEVICEID_HEATING_COOLING_UNIT                  0x0300
#define ZBA_DEVICEID_THERMOSTAT                            0x0301
#define ZBA_DEVICEID_TEMPERATURE_SENSOR                    0x0302
#define ZBA_DEVICEID_PUMP                                  0x0303
#define ZBA_DEVICEID_PUMP_CONTROLLER                       0x0304
#define ZBA_DEVICEID_PRESSURE_SENSOR                       0x0305
#define ZBA_DEVICEID_FLOW_SENSOR                           0x0306
#define ZBA_DEVICEID_HUMIDITY_SENSOR                       0x0307

// Intruder Alarm Systems (IAS) Device IDs
#define ZBA_DEVICEID_IAS_CONTROL_INDICATING_EQUIPMENT      0x0400
#define ZBA_DEVICEID_IAS_ANCILLARY_CONTROL_EQUIPMENT       0x0401
#define ZBA_DEVICEID_IAS_ZONE                              0x0402
#define ZBA_DEVICEID_IAS_WARNING_DEVICE                    0x0403

// Profile-specific APS Fragmentation configuration.
#define ZBA_APSF_FRAME_DELAY                               12
#define ZBA_APSF_WINDOW_SIZE                               3

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
extern const uint8 zbaGlobalCommissioningEPID[];

/*********************************************************************
 * FUNCTIONS
 */

/*
 *  ZBA Profile initialization function
 */
extern void zba_Init( SimpleDescriptionFormat_t *simpleDesc );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZBA_H */
  

