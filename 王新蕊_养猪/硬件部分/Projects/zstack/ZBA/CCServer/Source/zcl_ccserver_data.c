/**************************************************************************************************
  Filename:       zcl_ccserver_data.c
  Revised:        $Date: 2011-04-13 10:12:34 -0700 (Wed, 13 Apr 2011) $
  Revision:       $Revision: 25678 $

  Description:    Zigbee Cluster Library - Commissioning Cluster Server Application.


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
#include "AF.h"

#include "zba.h"
#include "zcl_cc.h"

#include "zcl_ccserver.h"

/*********************************************************************
 * CONSTANTS
 */

#define CCSERVER_DEVICE_VERSION        0
#define CCSERVER_FLAGS                 0

#define CCSERVER_HWVERSION             1
#define CCSERVER_ZCLVERSION            1

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Commissioning cluster Start Up Parameter Attributes
uint16 zclCCServer_ShortAddress;
uint8 zclCCServer_ExtendedPanId[Z_EXTADDR_LEN];
uint16 zclCCServer_PanId;
uint32 zclCCServer_ChannelMask;
uint8 zclCCServer_ProtocolVersion;
uint8 zclCCServer_StackProfile;
uint8 zclCCServer_StartUpControl;
uint8 zclCCServer_TrustCenterAddr[Z_EXTADDR_LEN];
uint8 zclCCServer_UseInsecureJoin;
uint8 zclCCServer_NetworkKeySeqNum;
uint8 zclCCServer_NetworkKeyType;
uint16 zclCCServer_NwkManagerAddr;


/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t zclCCServer_Attrs[CCSERVER_MAX_ATTRIBUTES] =
{
  // CC Attributes
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_SHORT_ADDRESS,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_ShortAddress
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_EXTENDED_PANID,
      ZCL_DATATYPE_IEEE_ADDR, 
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)zclCCServer_ExtendedPanId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_PANID,
      ZCL_DATATYPE_UINT16, 
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE, 
      (void *)&zclCCServer_PanId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_CHANNEL_MASK,
      ZCL_DATATYPE_BITMAP32, 
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_ChannelMask
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_PROTOCOL_VERSION,
      ZCL_DATATYPE_UINT8, 
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_ProtocolVersion
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_STACK_PROFILE,
      ZCL_DATATYPE_UINT8, 
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_StackProfile
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_STARTUP_CONTROL,
      ZCL_DATATYPE_ENUM8, 
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_StartUpControl
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_TRUST_CENTER_ADDRESS,
      ZCL_DATATYPE_IEEE_ADDR,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)zclCCServer_TrustCenterAddr
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_TRUST_CENTER_MASTER_KEY,
      ZCL_DATATYPE_128_BIT_SEC_KEY,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)NULL // Attribute value is maintained in the NV
    }
  },
  
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_NETWORK_KEY,
      ZCL_DATATYPE_128_BIT_SEC_KEY,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)NULL // Attribute value is maintained in the NV
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_USE_INSECURE_JOIN,
      ZCL_DATATYPE_BOOLEAN,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_UseInsecureJoin
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_PRECONFIGURED_LINK_KEY,
      ZCL_DATATYPE_128_BIT_SEC_KEY,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)NULL // Attribute value is maintained in the NV
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_NETWORK_KEY_SEQ_NUM,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_NetworkKeySeqNum
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_NETWORK_KEY_TYPE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_NetworkKeyType
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_COMMISSIONING,
    { // Attribute record
      ATTRID_CC_NETWORK_MANAGER_ADDRESS,
      ZCL_DATATYPE_UINT16,
      ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_READ | ACCESS_CONTROL_AUTH_WRITE,
      (void *)&zclCCServer_NwkManagerAddr
    }
  },
};

/*********************************************************************
 * SIMPLE DESCRIPTOR
 */
// This is the Cluster ID List and should be filled with Application
// specific cluster IDs.
#define CCSERVER_MAX_INCLUSTERS       1
const cId_t zclCCServer_InClusterList[CCSERVER_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_COMMISSIONING
};

#define CCSERVER_MAX_OUTCLUSTERS       1
const cId_t zclCCServer_OutClusterList[CCSERVER_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF
};

SimpleDescriptionFormat_t zclCCServer_SimpleDesc =
{
  CCSERVER_ENDPOINT,                  //  int    Endpoint;
  ZBA_PROFILE_ID,                     //  uint16 AppProfId[2];
  ZBA_DEVICEID_ON_OFF_SWITCH,         //  uint16 AppDeviceId[2];
  CCSERVER_DEVICE_VERSION,            //  int    AppDevVer:4;
  CCSERVER_FLAGS,                     //  int    AppFlags:4;
  CCSERVER_MAX_INCLUSTERS,            //  byte   AppNumInClusters;
  (cId_t *)zclCCServer_InClusterList, //  byte  *pAppInClusterList;
  CCSERVER_MAX_OUTCLUSTERS,           //  byte   AppNumInClusters;
  (cId_t *)zclCCServer_OutClusterList //  byte  *pAppInClusterList;
};

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/****************************************************************************
****************************************************************************/
