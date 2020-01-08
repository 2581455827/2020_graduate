/**************************************************************************************************
  Filename:       zcl_sampleswota.c
  Revised:        $Date: 2010-10-24 18:29:41 -0700 (Sun, 24 Oct 2010) $
  Revision:       $Revision: 24191 $

  Description:    Zigbee Cluster Library - sample device application.


  Copyright 2006-2009 Texas Instruments Incorporated. All rights reserved.

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
  This device will be like an On/Off Switch device. This application
  is not intended to be a On/Off Switch device, but will use the device
  description to implement this sample code.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_sampleswota.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

#include "zcl_ota.h"
#include "hal_ota.h"

/*********************************************************************
 * MACROS
 */
#define APP_DISC_ENDPOINT                   0x08

/*********************************************************************
 * CONSTANTS
 */
/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint16 addr;
  uint8 endpoint;
} zclSampleSwOta_Server_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
#pragma location="PREAMBLE"
const CODE preamble_t OTA_Preamble =
{
  0xFFFFFFFF,           // Program Length
  OTA_MANUFACTURER_ID,  // Manufacturer ID
  OTA_TYPE_ID,          // Image Type
  0x00000001,            // Image Version
};
#pragma required=OTA_Preamble

#pragma location="CRC"
const CODE otaCrc_t OTA_CRC =
{
  0xFFFF,        // CRC
  0xFFFF,        // CRC Shadow
};
#pragma required=OTA_CRC

byte zclSampleSwOta_TaskID;

static endPointDesc_t appDiscoveryEp =
{
  APP_DISC_ENDPOINT,
  &zclSampleSwOta_TaskID,
  (SimpleDescriptionFormat_t *)&zclSampleSwOta_SimpleDesc,
  (afNetworkLatencyReq_t) 0
};

zclSampleSwOta_Server_t zclSampleSwOtaServerList[SAMPLESWOTA_OTA_MAX_SERVERS];

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static afAddrType_t zclSampleSwOta_DstAddr;

#define ZCLSAMPLESWOTA_BINDINGLIST       1
static cId_t bindingOutClusters[ZCLSAMPLESWOTA_BINDINGLIST] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF
};

// Test Endpoint to allow SYS_APP_MSGs
static endPointDesc_t sampleSw_TestEp =
{
  20,                                 // Test endpoint
  &zclSampleSwOta_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

uint8 zclSampleSwOta_TransID;  // This is the unique message ID (counter)

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclSampleSwOta_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static void zclSampleSwOta_HandleKeys( byte shift, byte keys );
static void zclSampleSwOta_BasicResetCB( void );
static void zclSampleSwOta_IdentifyCB( zclIdentify_t *pCmd );
static void zclSampleSwOta_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp );
static void zclSampleSwOta_ProcessIdentifyTimeChange( void );

static void zclSampleSwOta_ProcessOTAMsgs( zclOTA_CallbackMsg_t* pMsg );

// Functions to process ZCL Foundation incoming Command/Response messages
static void zclSampleSwOta_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclSampleSwOta_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclSampleSwOta_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclSampleSwOta_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclSampleSwOta_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclSampleSwOta_CmdCallbacks =
{
  zclSampleSwOta_BasicResetCB,     // Basic Cluster Reset command
  zclSampleSwOta_IdentifyCB,       // Identify command
  zclSampleSwOta_IdentifyQueryRspCB, // Identify Query Response command
  NULL,                         // On / Off cluster command - not needed.
  NULL,                         // Level Control Move to Level command
  NULL,                         // Level Control Move command
  NULL,                         // Level Control Step command
  NULL,                         // Group Response commands
  NULL,                         // Scene Store Request command
  NULL,                         // Scene Recall Request command
  NULL,                         // Scene Response commands
  NULL,                         // Alarm (Response) commands
  NULL,                         // RSSI Location commands
  NULL,                         // RSSI Location Response commands
};


/*********************************************************************
 * @fn          zclSampleSwOta_Init
 *
 * @brief       Initialization function for the zclGeneral layer.
 *
 * @param       none
 *
 * @return      none
 */
void zclSampleSwOta_Init( byte task_id )
{
  OTA_ImageHeader_t header;
  preamble_t preamble;
  zclSampleSwOta_TaskID = task_id;
  char lcdBuf[20];

#if HAL_OTA_XNV_IS_SPI
  XNV_SPI_INIT();
#endif
  
  // Set destination address to indirect
  zclSampleSwOta_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zclSampleSwOta_DstAddr.endPoint = 0;
  zclSampleSwOta_DstAddr.addr.shortAddr = 0;

  // Read the OTA File Header
  HalOTARead(0, (uint8 *)&header, sizeof(OTA_ImageHeader_t), HAL_OTA_DL);
  
  if (header.magicNumber == OTA_HDR_MAGIC_NUMBER)
  {
    zclOTA_DownloadedFileVersion = header.fileId.version;
    zclOTA_DownloadedZigBeeStackVersion = header.stackVersion;
  }
  
  // Load the OTA Attributes from the constant values in NV
  HalOTARead(PREAMBLE_OFFSET, (uint8 *)&preamble, sizeof(preamble_t), HAL_OTA_RC);

  zclOTA_ManufacturerId = preamble.manufacturerId;
  zclOTA_ImageType = preamble.imageType;
  zclOTA_CurrentFileVersion = preamble.imageVersion;
  
  // This app is part of the Home Automation Profile
  zclHA_Init( &zclSampleSwOta_SimpleDesc );

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( SAMPLESWOTA_ENDPOINT, &zclSampleSwOta_CmdCallbacks );

  // Register the application's attribute list
  zcl_registerAttrList( SAMPLESWOTA_ENDPOINT, SAMPLESWOTA_MAX_ATTRIBUTES, zclSampleSwOta_Attrs );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( zclSampleSwOta_TaskID );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( zclSampleSwOta_TaskID );

  // Register for callback events from the ZCL OTA
  zclOTA_Register(zclSampleSwOta_TaskID);
  
  // Register for a test endpoint
  afRegister( &sampleSw_TestEp );

  // Register for a test endpoint
  afRegister( &appDiscoveryEp );
  
  // Indicate teh current image version on LCD
  osal_memset(lcdBuf, ' ', sizeof(lcdBuf));
  lcdBuf[19] = '\0';
  osal_memcpy(lcdBuf, "Ver: 0x", 7);
  _ltoa(zclOTA_CurrentFileVersion, (uint8*)&lcdBuf[7], 16);
  HalLcdWriteString(lcdBuf, HAL_LCD_LINE_3);

  ZDO_RegisterForZDOMsg( zclSampleSwOta_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( zclSampleSwOta_TaskID, Match_Desc_rsp );
}

/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for zclGeneral.
 *
 * @param       none
 *
 * @return      none
 */
uint16 zclSampleSwOta_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclSampleSwOta_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_OTA_CALLBACK_IND:
          zclSampleSwOta_ProcessOTAMsgs( (zclOTA_CallbackMsg_t*)MSGpkt  );
          break;
          
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          zclSampleSwOta_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case ZDO_CB_MSG:
          zclSampleSwOta_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          zclSampleSwOta_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & SAMPLESWOTA_IDENTIFY_TIMEOUT_EVT )
  {
    zclSampleSwOta_IdentifyTime = 10;
    zclSampleSwOta_ProcessIdentifyTimeChange();

    return ( events ^ SAMPLESWOTA_IDENTIFY_TIMEOUT_EVT );
  }

  // Discard unknown events
  return 0;
}


/*********************************************************************
 * @fn      zclSampleSwOta_ProcessZDOMsgs()
 *
 * @brief   Process response messages
 *
 * @param   none
 *
 * @return  none
 */
void zclSampleSwOta_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )
{
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        // Light LED
        HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
      }
#if defined(BLINK_LEDS)
      else
      {
        // Flash LED to show failure
        HalLedSet ( HAL_LED_4, HAL_LED_MODE_FLASH );
      }
#endif
      break;

    case Match_Desc_rsp:
      {
        ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );
        if ( pRsp )
        {
          if ( pRsp->status == ZSuccess && pRsp->cnt )
          {
            zclSampleSwOta_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
            zclSampleSwOta_DstAddr.addr.shortAddr = pRsp->nwkAddr;
            // Take the first endpoint, Can be changed to search through endpoints
            zclSampleSwOta_DstAddr.endPoint = pRsp->epList[0];

            // Light LED
            HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
          }
          osal_mem_free( pRsp );
        }
      }
      break;
  }
}

/*********************************************************************
 * @fn      zclSampleSwOta_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_3
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void zclSampleSwOta_HandleKeys( byte shift, byte keys )
{
  zAddrType_t dstAddr;
  (void)shift;  // Intentionally unreferenced parameter

  if ( keys & HAL_KEY_SW_1 )
  {
    // Using this as the "Light Switch"
#ifdef ZCL_ON_OFF
    zclGeneral_SendOnOff_CmdToggle( SAMPLESWOTA_ENDPOINT, &zclSampleSwOta_DstAddr, false, 0 );
#endif
  }

  if ( keys & HAL_KEY_SW_2 )
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

    // Initiate an End Device Bind Request, this bind request will
    // only use a cluster list that is important to binding.
    dstAddr.addrMode = afAddr16Bit;
    dstAddr.addr.shortAddr = 0;   // Coordinator makes the match
    ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(),
                           SAMPLESWOTA_ENDPOINT,
                           ZCL_HA_PROFILE_ID,
                           0, NULL,   // No incoming clusters to bind
                           ZCLSAMPLESWOTA_BINDINGLIST, bindingOutClusters,
                           TRUE );
  }

  if ( keys & HAL_KEY_SW_3 )
  {
  }

  if ( keys & HAL_KEY_SW_4 )
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

    // Initiate a Match Description Request (Service Discovery)
    dstAddr.addrMode = AddrBroadcast;
    dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
    ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                       ZCL_HA_PROFILE_ID,
                       ZCLSAMPLESWOTA_BINDINGLIST, bindingOutClusters,
                       0, NULL,   // No incoming clusters to bind
                       FALSE );
  }
}

/*********************************************************************
 * @fn      zclSampleSwOta_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   none
 *
 * @return  none
 */
static void zclSampleSwOta_ProcessIdentifyTimeChange( void )
{
  if ( zclSampleSwOta_IdentifyTime > 0 )
  {
    osal_start_timerEx( zclSampleSwOta_TaskID, SAMPLESWOTA_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    if ( zclSampleSwOta_OnOff )
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_ON );
    else
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( zclSampleSwOta_TaskID, SAMPLESWOTA_IDENTIFY_TIMEOUT_EVT );
  }
}

/*********************************************************************
 * @fn      zclSampleSwOta_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to  default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zclSampleSwOta_BasicResetCB( void )
{
}

/*********************************************************************
 * @fn      zclSampleSwOta_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zclSampleSwOta_IdentifyCB( zclIdentify_t *pCmd )
{
  zclSampleSwOta_IdentifyTime = pCmd->identifyTime;
  zclSampleSwOta_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      zclSampleSwOta_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   srcAddr - source address
 * @param   timeout - number of seconds to identify yourself (valid for query response)
 *
 * @return  none
 */
static void zclSampleSwOta_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  // Query Response (with timeout value)
  (void)pRsp;
}

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      zclSampleSwOta_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zclSampleSwOta_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zclSampleSwOta_ProcessInReadRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_WRITE
    case ZCL_CMD_WRITE_RSP:
      zclSampleSwOta_ProcessInWriteRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_REPORT
    // See ZCL Test Applicaiton (zcl_testapp.c) for sample code on Attribute Reporting
    case ZCL_CMD_CONFIG_REPORT:
      //zclSampleSwOta_ProcessInConfigReportCmd( pInMsg );
      break;

    case ZCL_CMD_CONFIG_REPORT_RSP:
      //zclSampleSwOta_ProcessInConfigReportRspCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG:
      //zclSampleSwOta_ProcessInReadReportCfgCmd( pInMsg );
      break;

    case ZCL_CMD_READ_REPORT_CFG_RSP:
      //zclSampleSwOta_ProcessInReadReportCfgRspCmd( pInMsg );
      break;

    case ZCL_CMD_REPORT:
      //zclSampleSwOta_ProcessInReportCmd( pInMsg );
      break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
      zclSampleSwOta_ProcessInDefaultRspCmd( pInMsg );
      break;
#ifdef ZCL_DISCOVER
    case ZCL_CMD_DISCOVER_RSP:
      zclSampleSwOta_ProcessInDiscRspCmd( pInMsg );
      break;
#endif
    default:
      break;
  }

  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      zclSampleSwOta_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclSampleSwOta_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    // Notify the originator of the results of the original read attributes
    // attempt and, for each successfull request, the value of the requested
    // attribute
  }

  return TRUE;
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      zclSampleSwOta_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclSampleSwOta_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }

  return TRUE;
}
#endif // ZCL_WRITE

/*********************************************************************
 * @fn      zclSampleSwOta_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclSampleSwOta_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
  // Device is notified of the Default Response command.
  (void)pInMsg;
  return TRUE;
}

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zclSampleSwOta_ProcessInDiscRspCmd
 *
 * @brief   Process the "Profile" Discover Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclSampleSwOta_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverRspCmd_t *discoverRspCmd;
  uint8 i;

  discoverRspCmd = (zclDiscoverRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }

  return TRUE;
}
#endif // ZCL_DISCOVER

/*********************************************************************
 * @fn      zclSampleSwOta_ProcessOTAMsgs
 *
 * @brief   Called to process callbacks from the ZCL OTA.
 *
 * @param   none
 *
 * @return  none
 */
static void zclSampleSwOta_ProcessOTAMsgs( zclOTA_CallbackMsg_t* pMsg )
{
  uint8 RxOnIdle;

  switch(pMsg->ota_event)
  {
  case ZCL_OTA_START_CALLBACK:
    if (pMsg->hdr.status == ZSuccess)
    {
      // Speed up the poll rate  
      RxOnIdle = TRUE;
      ZMacSetReq( ZMacRxOnIdle, &RxOnIdle );
      NLME_SetPollRate( 2000 );
    }
    break;
    
  case ZCL_OTA_DL_COMPLETE_CALLBACK:
    if (pMsg->hdr.status == ZSuccess)
    {
      // Reset the CRC Shadow and reboot.  The bootloader will see the 
      // CRC shadow has been cleared and switch to the new image
      HalOTAInvRC();
      SystemReset();
    }
    else
    {
      // slow the poll rate back down.
      RxOnIdle = FALSE;
      ZMacSetReq( ZMacRxOnIdle, &RxOnIdle );
      NLME_SetPollRate(DEVICE_POLL_RATE);
    }
    break;
    
  default:
    break;
  }
}

/****************************************************************************
****************************************************************************/


