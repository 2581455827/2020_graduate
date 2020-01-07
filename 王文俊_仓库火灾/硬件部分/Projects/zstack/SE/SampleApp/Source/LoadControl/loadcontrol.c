/**************************************************************************************************
  Filename:       loadcontrol.c
  Revised:        $Date: 2012-04-02 17:02:19 -0700 (Mon, 02 Apr 2012) $
  Revision:       $Revision: 29996 $

  Description:    This module implements the Load Control Device functionality
                  and contains the init and event loop functions


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
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
  This application is designed for the test purpose of the SE profile which
  exploits the following clusters for a Load Control configuration:

  General Basic
  General Alarms
  General Time
  General Key Establishment
  SE     Demand Response and Load Control

  Key control:
    SW1:  Join Network
    SW2:  N/A
    SW3:  N/A
    SW4:  N/A

*********************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "OSAL.h"
#include "OSAL_Clock.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "AddrMgr.h"

#include "se.h"
#include "loadcontrol.h"
#include "zcl_general.h"
#include "zcl_se.h"
#include "zcl_key_establish.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"


/*********************************************************************
 * MACROS
 */

// There is no attribute in the Mandatory Reportable Attribute list for now
#define zcl_MandatoryReportableAttribute( a ) ( a == NULL )

/*********************************************************************
 * CONSTANTS
 */

#define loadControlNwkState  devState

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static uint8 loadControlTaskID;                // osal task id for load control device
static uint8 loadControlTransID;               // transaction id
static afAddrType_t ESPAddr;                   // ESP destination address
#if SECURE
static uint8 linkKeyStatus;                    // status variable from get link key function
#endif
static zclCCReportEventStatus_t rsp;           // structure for report event status

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void loadcontrol_HandleKeys( uint8 shift, uint8 keys );

#if SECURE
static uint8 loadcontrol_KeyEstablish_ReturnLinkKey( uint16 shortAddr );
#endif

static void loadcontrol_ProcessIdentifyTimeChange( void );

/*************************************************************************/
/*** Application Callback Functions                                    ***/
/*************************************************************************/

// Foundation Callback functions
static uint8 loadcontrol_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );

// General Cluster Callback functions
static void loadcontrol_BasicResetCB( void );
static void loadcontrol_IdentifyCB( zclIdentify_t *pCmd );
static void loadcontrol_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void loadcontrol_AlarmCB( zclAlarm_t *pAlarm );
#ifdef SE_UK_EXT
static void loadcontrol_GetEventLogCB( uint8 srcEP, afAddrType_t *srcAddr,
                                   zclGetEventLog_t *pEventLog, uint8 seqNum );
static void loadcontrol_PublishEventLogCB( afAddrType_t *srcAddr,
                                             zclPublishEventLog_t *pEventLog );
#endif // SE_UK_EXT

// Function to process ZDO callback messages
static void loadcontrol_ProcessZDOMsgs( zdoIncomingMsg_t *pMsg );

// SE Callback functions

static void loadcontrol_LoadControlEventCB( zclCCLoadControlEvent_t *pCmd,
                           afAddrType_t *srcAddr, uint8 status, uint8 seqNum );
static void loadcontrol_CancelLoadControlEventCB( zclCCCancelLoadControlEvent_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void loadcontrol_CancelAllLoadControlEventsCB( zclCCCancelAllLoadControlEvents_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void loadcontrol_ReportEventStatusCB( zclCCReportEventStatus_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void loadcontrol_GetScheduledEventCB( zclCCGetScheduledEvent_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );

/************************************************************************/
/***               Functions to process ZCL Foundation                ***/
/***               incoming Command/Response messages                 ***/
/************************************************************************/
static void loadcontrol_ProcessZCLMsg( zclIncomingMsg_t *msg );
#if defined ( ZCL_READ )
static uint8 loadcontrol_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_READ
#if defined ( ZCL_WRITE )
static uint8 loadcontrol_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_WRITE
static uint8 loadcontrol_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#if defined ( ZCL_DISCOVER )
static uint8 loadcontrol_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_DISCOVER

/*********************************************************************
 * ZCL General Clusters Callback table
 */
static zclGeneral_AppCallbacks_t loadcontrol_GenCmdCallbacks =
{
  loadcontrol_BasicResetCB,              // Basic Cluster Reset command
  loadcontrol_IdentifyCB,                // Identify command
  loadcontrol_IdentifyQueryRspCB,        // Identify Query Response command
  NULL,                                  // On/Off cluster commands
  NULL,                                  // Level Control Move to Level command
  NULL,                                  // Level Control Move command
  NULL,                                  // Level Control Step command
  NULL,                                  // Level Control Stop command
  NULL,                                  // Group Response commands
  NULL,                                  // Scene Store Request command
  NULL,                                  // Scene Recall Request command
  NULL,                                  // Scene Response command
  loadcontrol_AlarmCB,                   // Alarm (Response) command
#ifdef SE_UK_EXT
  loadcontrol_GetEventLogCB,             // Get Event Log command
  loadcontrol_PublishEventLogCB,         // Publish Event Log command
#endif
  NULL,                                  // RSSI Location command
  NULL                                   // RSSI Location Response command
};

/*********************************************************************
 * ZCL SE Clusters Callback table
 */
static zclSE_AppCallbacks_t loadcontrol_SECmdCallbacks =
{
  NULL,                                               // Publish Price
  NULL,                                               // Publish Block Period
#if defined ( SE_UK_EXT )
  NULL,                                               // Publish Tariff Information
  NULL,                                               // Publish Price Matrix
  NULL,                                               // Publish Block Thresholds
  NULL,                                               // Publish Conversion Factor
  NULL,                                               // Publish Calorific Value
  NULL,                                               // Publish CO2 Value
  NULL,                                               // Publish CPP Event
  NULL,                                               // Publish Billing Period
  NULL,                                               // Publish Consolidated Bill
  NULL,                                               // Publish Credit Payment Info
#endif  // SE_UK_EXT
  NULL,                                               // Get Current Price
  NULL,                                               // Get Scheduled Price
  NULL,                                               // Price Acknowledgement
  NULL,                                               // Get Block Period
#if defined ( SE_UK_EXT )
  NULL,                                               // Get Tariff Information
  NULL,                                               // Get Price Matrix
  NULL,                                               // Get Block Thresholds
  NULL,                                               // Get Conversion Factor
  NULL,                                               // Get Calorific Value
  NULL,                                               // Get CO2 Value
  NULL,                                               // Get Billing Period
  NULL,                                               // Get Consolidated Bill
  NULL,                                               // CPP Event Response
#endif  // SE_UK_EXT
  loadcontrol_LoadControlEventCB,                     // Load Control Event
  loadcontrol_CancelLoadControlEventCB,               // Cancel Load Control Event
  loadcontrol_CancelAllLoadControlEventsCB,           // Cancel All Load Control Events
  loadcontrol_ReportEventStatusCB,                    // Report Event Status
  loadcontrol_GetScheduledEventCB,                    // Get Scheduled Event
  NULL,                                               // Get Profile Response
  NULL,                                               // Request Mirror Command
  NULL,                                               // Mirror Remove Command
  NULL,                                               // Request Fast Poll Mode Response
#if defined ( SE_UK_EXT )
  NULL,                                               // Get Snapshot Response
#endif  // SE_UK_EXT
  NULL,                                               // Get Profile Command
  NULL,                                               // Request Mirror Response
  NULL,                                               // Mirror Remove Response
  NULL,                                               // Request Fast Poll Mode Command
#if defined ( SE_UK_EXT )
  NULL,                                               // Get Snapshot Command
  NULL,                                               // Take Snapshot Command
  NULL,                                               // Mirror Report Attribute Response
#endif  // SE_UK_EXT
  NULL,                                               // Display Message Command
  NULL,                                               // Cancel Message Command
  NULL,                                               // Get Last Message Command
  NULL,                                               // Message Confirmation
  NULL,                                               // Request Tunnel Response
  NULL,                                               // Transfer Data
  NULL,                                               // Transfer Data Error
  NULL,                                               // Ack Transfer Data
  NULL,                                               // Ready Data
#if defined ( SE_UK_EXT )
  NULL,                                               // Supported Tunnel Protocols Response
  NULL,                                               // Tunnel Closure Notification
#endif  // SE_UK_EXT
  NULL,                                               // Request Tunnel
  NULL,                                               // Close Tunnel
#if defined ( SE_UK_EXT )
  NULL,                                               // Get Supported Tunnel Protocols
#endif  // SE_UK_EXT
  NULL,                                               // Supply Status Response
#if defined ( SE_UK_EXT )
  NULL,                                               // Get Prepay Snapshot Response
  NULL,                                               // Change Payment Mode Response
  NULL,                                               // Consumer Topup Response
  NULL,                                               // Get Commands
  NULL,                                               // Publish Topup Log
  NULL,                                               // Publish Debt Log
#endif  // SE_UK_EXT
  NULL,                                               // Select Available Emergency Credit Command
  NULL,                                               // Change Supply Command
#if defined ( SE_UK_EXT )
  NULL,                                               // Change Debt
  NULL,                                               // Emergency Credit Setup
  NULL,                                               // Consumer Topup
  NULL,                                               // Credit Adjustment
  NULL,                                               // Change PaymentMode
  NULL,                                               // Get Prepay Snapshot
  NULL,                                               // Get Topup Log
  NULL,                                               // Set Low Credit Warning Level
  NULL,                                               // Get Debt Repayment Log
  NULL,                                               // Publish Calendar
  NULL,                                               // Publish Day Profile
  NULL,                                               // Publish Week Profile
  NULL,                                               // Publish Seasons
  NULL,                                               // Publish Special Days
  NULL,                                               // Get Calendar
  NULL,                                               // Get Day Profiles
  NULL,                                               // Get Week Profiles
  NULL,                                               // Get Seasons
  NULL,                                               // Get Special Days
  NULL,                                               // Publish Change Tenancy
  NULL,                                               // Publish Change Supplier
  NULL,                                               // Change Supply
  NULL,                                               // Change Password
  NULL,                                               // Local Change Supply
  NULL,                                               // Get Change Tenancy
  NULL,                                               // Get Change Supplier
  NULL,                                               // Get Change Supply
  NULL,                                               // Supply Status Response
  NULL,                                               // Get Password
#endif  // SE_UK_EXT
};

/*********************************************************************
 * @fn          loadcontrol_Init
 *
 * @brief       Initialization function for the ZCL App Application.
 *
 * @param       uint8 task_id - load control task id
 *
 * @return      none
 */
void loadcontrol_Init( uint8 task_id )
{
  loadControlTaskID = task_id;

  // setup destination address for ESP
  ESPAddr.addrMode = (afAddrMode_t)Addr16Bit;
  ESPAddr.endPoint = LOADCONTROL_ENDPOINT;
  ESPAddr.addr.shortAddr = 0;

  // register for SE endpoint
  zclSE_Init( &loadControlSimpleDesc );

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( LOADCONTROL_ENDPOINT, &loadcontrol_GenCmdCallbacks );

  // Register the ZCL SE Cluster Library callback functions
  zclSE_RegisterCmdCallbacks( LOADCONTROL_ENDPOINT, &loadcontrol_SECmdCallbacks );

  // Register the application's attribute list
  zcl_registerAttrList( LOADCONTROL_ENDPOINT, LOADCONTROL_MAX_ATTRIBUTES, loadControlAttrs );

  // Register the application's cluster option list
  zcl_registerClusterOptionList( LOADCONTROL_ENDPOINT, LOADCONTROL_MAX_OPTIONS, loadControlOptions );

  // Register the application's attribute data validation callback function
  zcl_registerValidateAttrData( loadcontrol_ValidateAttrDataCB );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( loadControlTaskID );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( loadControlTaskID );

  // Register with the ZDO to receive Match Descriptor Responses
  ZDO_RegisterForZDOMsg(task_id, Match_Desc_rsp);

  // Start the timer to sync LoadControl timer with the osal timer
  osal_start_timerEx( loadControlTaskID, LOADCONTROL_UPDATE_TIME_EVT, LOADCONTROL_UPDATE_TIME_PERIOD );
}

/*********************************************************************
 * @fn          loadcontrol_event_loop
 *
 * @brief       Event Loop Processor for loadcontrol.
 *
 * @param       uint8 task id - load control task id
 * @param       uint16 events - event bitmask
 *
 * @return      none
 */
uint16 loadcontrol_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( loadControlTaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          loadcontrol_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;

        case ZCL_INCOMING_MSG:
          // Incoming ZCL foundation command/response messages
          loadcontrol_ProcessZCLMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          loadcontrol_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          if (DEV_ROUTER == (devStates_t)(MSGpkt->hdr.status))
          {
#if SECURE
            {
              // check to see if link key had already been established
              linkKeyStatus = loadcontrol_KeyEstablish_ReturnLinkKey(ESPAddr.addr.shortAddr);

              if (linkKeyStatus != ZSuccess)
              {
                cId_t cbkeCluster = ZCL_CLUSTER_ID_GEN_KEY_ESTABLISHMENT;
                zAddrType_t dstAddr;

                // Send out a match for the key establishment
                dstAddr.addrMode = AddrBroadcast;
                dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
                ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR, ZCL_SE_PROFILE_ID,
                                  1, &cbkeCluster, 0, NULL, FALSE );
              }
            }
#endif
          }
          break;

#if defined( ZCL_KEY_ESTABLISH )
        case ZCL_KEY_ESTABLISH_IND:
          if ((MSGpkt->hdr.status) == TermKeyStatus_Success)
          {
            ESPAddr.endPoint = LOADCONTROL_ENDPOINT; // set destination endpoint back to application endpoint
          }

          break;
#endif

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // event to intiate key establishment request
  if ( events & LOADCONTROL_KEY_ESTABLISHMENT_REQUEST_EVT )
  {
    zclGeneral_KeyEstablish_InitiateKeyEstablishment(loadControlTaskID, &ESPAddr, loadControlTransID);

    return ( events ^ LOADCONTROL_KEY_ESTABLISHMENT_REQUEST_EVT );
  }

  // handle processing of identify timeout event triggered by an identify command
  if ( events & LOADCONTROL_IDENTIFY_TIMEOUT_EVT )
  {
    if ( loadControlIdentifyTime > 0 )
    {
      loadControlIdentifyTime--;
    }
    loadcontrol_ProcessIdentifyTimeChange();

    return ( events ^ LOADCONTROL_IDENTIFY_TIMEOUT_EVT );
  }

  // event to get current time
  if ( events & LOADCONTROL_UPDATE_TIME_EVT )
  {
    loadControlTime = osal_getClock();
    osal_start_timerEx( loadControlTaskID, LOADCONTROL_UPDATE_TIME_EVT, LOADCONTROL_UPDATE_TIME_PERIOD );

    return ( events ^ LOADCONTROL_UPDATE_TIME_EVT );
  }

  // event to handle load control complete event
  if ( events & LOADCONTROL_LOAD_CTRL_EVT )
  {
    // load control evt completed

    // Send response back
    // DisableDefaultResponse is set to FALSE - it is recommended to turn on
    // default response since Report Event Status Command does not have
    // a response.
    rsp.eventStatus = EVENT_STATUS_LOAD_CONTROL_EVENT_COMPLETED;
    zclSE_LoadControl_Send_ReportEventStatus( LOADCONTROL_ENDPOINT, &ESPAddr,
                                            &rsp, FALSE, loadControlTransID );

    HalLcdWriteString("Load Evt Complete", HAL_LCD_LINE_3);

    HalLedSet(HAL_LED_4, HAL_LED_MODE_OFF);

    return ( events ^ LOADCONTROL_LOAD_CTRL_EVT );

  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      loadcontrol_ProcessZDOMsgs
 *
 * @brief   Called to process callbacks from the ZDO.
 *
 * @param   none
 *
 * @return  none
 */
static void loadcontrol_ProcessZDOMsgs( zdoIncomingMsg_t *pMsg )
{
  if (pMsg->clusterID == Match_Desc_rsp)
  {
    ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( pMsg );

    if (pRsp)
    {
      if (pRsp->cnt)
      {
        // Record the trust center
        ESPAddr.endPoint = pRsp->epList[0];
        ESPAddr.addr.shortAddr = pMsg->srcAddr.addr.shortAddr;

        // send out key establishment request
        osal_set_event( loadControlTaskID, LOADCONTROL_KEY_ESTABLISHMENT_REQUEST_EVT);
      }
      osal_mem_free(pRsp);
    }
  }
}

/*********************************************************************
 * @fn      loadcontrol_ProcessIdentifyTimeChange
 *
 * @brief   Called to blink led for specified IdentifyTime attribute value
 *
 * @param   none
 *
 * @return  none
 */
static void loadcontrol_ProcessIdentifyTimeChange( void )
{
  if ( loadControlIdentifyTime > 0 )
  {
    osal_start_timerEx( loadControlTaskID, LOADCONTROL_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( loadControlTaskID, LOADCONTROL_IDENTIFY_TIMEOUT_EVT );
  }
}

#if SECURE
/*********************************************************************
 * @fn      loadcontrol_KeyEstablish_ReturnLinkKey
 *
 * @brief   This function get the requested link key
 *
 * @param   shortAddr - short address of the partner.
 *
 * @return  none
 */
static uint8 loadcontrol_KeyEstablish_ReturnLinkKey( uint16 shortAddr )
{
  uint8 status = ZFailure;
  AddrMgrEntry_t entry;

  // Look up the long address of the device

  entry.user = ADDRMGR_USER_DEFAULT;
  entry.nwkAddr = shortAddr;

  if ( AddrMgrEntryLookupNwk( &entry ) )
  {
    // check if APS link key has been established
    if ( APSME_IsLinkKeyValid( entry.extAddr ) == TRUE )
    {
      status = ZSuccess;
    }
  }
  else
  {
    // It's an unknown device
    status = ZInvalidParameter;
  }

  return status;
}
#endif

/*********************************************************************
 * @fn      loadcontrol_HandleKeys
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
static void loadcontrol_HandleKeys( uint8 shift, uint8 keys )
{
  // Shift is used to make each button/switch dual purpose.
  if ( shift )
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }
    if ( keys & HAL_KEY_SW_2 )
    {
    }
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {
      ZDOInitDevice(0); // join the network
    }

    if ( keys & HAL_KEY_SW_2 )
    {

    }

    if ( keys & HAL_KEY_SW_3 )
    {

    }

    if ( keys & HAL_KEY_SW_4 )
    {

    }
  }
}

/*********************************************************************
 * @fn      loadcontrol_ValidateAttrDataCB
 *
 * @brief   Check to see if the supplied value for the attribute data
 *          is within the specified range of the attribute.
 *
 * @param   pAttr - pointer to attribute
 * @param   pAttrInfo - pointer to attribute info
 *
 * @return  TRUE if data valid. FALSE otherwise.
 */
static uint8 loadcontrol_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
{
  uint8 valid = TRUE;

  switch ( pAttrInfo->dataType )
  {
    case ZCL_DATATYPE_BOOLEAN:
      if ( ( *(pAttrInfo->attrData) != 0 ) && ( *(pAttrInfo->attrData) != 1 ) )
        valid = FALSE;
      break;

  default:
      break;
  }

  return ( valid );
}

/*********************************************************************
 * @fn      loadcontrol_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library to set all
 *          the attributes of all the clusters to their factory defaults
 *
 * @param   none
 *
 * @return  none
 */
static void loadcontrol_BasicResetCB( void )
{
  // user should handle setting attributes to factory defaults here
}

/*********************************************************************
 * @fn      loadcontrol_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   pCmd - pointer to structure for Identify command
 *
 * @return  none
 */
static void loadcontrol_IdentifyCB( zclIdentify_t *pCmd )
{
  loadControlIdentifyTime = pCmd->identifyTime;
  loadcontrol_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      loadcontrol_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   pRsp - pointer to structure for Identity Query Response command
 *
 * @return  none
 */
static void loadcontrol_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp )
{
  // add user code here
}

/*********************************************************************
 * @fn      loadcontrol_AlarmCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Alarm request or response command for
 *          this application.
 *
 * @param   pAlarm - pointer to structure for Alarm command
 *
 * @return  none
 */
static void loadcontrol_AlarmCB( zclAlarm_t *pAlarm )
{
  // add user code here
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      loadcontrol_GetEventLogCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received a Get Event Log command for this
 *          application.
 *
 * @param   srcEP - source endpoint
 * @param   srcAddr - pointer to source address
 * @param   pEventLog - pointer to structure for Get Event Log command
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void loadcontrol_GetEventLogCB( uint8 srcEP, afAddrType_t *srcAddr,
                                       zclGetEventLog_t *pEventLog, uint8 seqNum )
{
  // add user code here, which could fragment the event log payload if
  // the entire payload doesn't fit into one Publish Event Log Command.
  // Note: the Command Index starts at 0 and is incremented for each
  // fragment belonging to the same command.

  // There's no event log for now! The Metering Device will support
  // logging for all events configured to do so.
}

/*********************************************************************
 * @fn      loadcontrol_PublishEventLogCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received a Publish Event Log command for this
 *          application.
 *
 * @param   srcAddr - pointer to source address
 * @param   pEventLog - pointer to structure for Publish Event Log command
 *
 * @return  none
 */
static void loadcontrol_PublishEventLogCB( afAddrType_t *srcAddr, zclPublishEventLog_t *pEventLog )
{
  // add user code here
}
#endif // SE_UK_EXT

#if defined (ZCL_LOAD_CONTROL)
/*********************************************************************
 * @fn      loadcontrol_SendReportEventStatus
 *
 * @brief   Callback from the ZCL SE Profile Message Cluster Library when
 *          it received a Load Control Event Command for
 *          this application.
 *
 * @param   afAddrType_t *srcAddr - pointer to source address
 * @param   uint8 seqNum - sequence number for this event
 * @param   uint32 eventID - event ID for this event
 * @param   uint32 startTime - start time for this event
 * @param   uint8 eventStatus - status for this event
 * @param   uint8 criticalityLevel - criticality level for this event
 * @param   uint8 eventControl - event control for this event
 *
 * @return  none
 */
static void loadcontrol_SendReportEventStatus( afAddrType_t *srcAddr, uint8 seqNum,
                                              uint32 eventID, uint32 startTime,
                                              uint8 eventStatus, uint8 criticalityLevel,
                                              uint8 eventControl )
{

  // Mandatory fields - use the incoming data
  rsp.issuerEventID = eventID;
  rsp.eventStartTime = startTime;
  rsp.criticalityLevelApplied = criticalityLevel;
  rsp.eventControl = eventControl;
  rsp.eventStatus = eventStatus;
  rsp.signatureType = SE_PROFILE_SIGNATURE_TYPE_ECDSA;

  // loadcontrol_Signature is a static array.
  // value can be changed in loadcontrol_data.c
  osal_memcpy( rsp.signature, loadControlSignature, SE_PROFILE_SIGNATURE_LENGTH );

  // Optional fields - fill in with non-used value by default
  rsp.coolingTemperatureSetPointApplied = SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT;
  rsp.heatingTemperatureSetPointApplied = SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT;
  rsp.averageLoadAdjustment = SE_OPTIONAL_FIELD_INT8;
  rsp.dutyCycleApplied = SE_OPTIONAL_FIELD_UINT8;

  // Send response back
  // DisableDefaultResponse is set to FALSE - it is recommended to turn on
  // default response since Report Event Status Command does not have
  // a response.
  zclSE_LoadControl_Send_ReportEventStatus( LOADCONTROL_ENDPOINT, srcAddr,
                                            &rsp, FALSE, seqNum );
}
#endif // ZCL_LOAD_CONTROL

/*********************************************************************
 * @fn      loadcontrol_LoadControlEventCB
 *
 * @brief   Callback from the ZCL SE Profile Load Control Cluster Library when
 *          it received a Load Control Event Command for
 *          this application.
 *
 * @param   pCmd - pointer to load control event command
 * @param   srcAddr - pointer to source address
 * @param   status - event status
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void loadcontrol_LoadControlEventCB( zclCCLoadControlEvent_t *pCmd,
                                               afAddrType_t *srcAddr, uint8 status,
                                               uint8 seqNum)
{
#if defined ( ZCL_LOAD_CONTROL )
  // According to the Smart Metering Specification, upon receipt
  // of the Load Control Event command, the receiving device shall
  // send Report Event Status command back.
  uint8 eventStatus;

  if ( status == ZCL_STATUS_INVALID_FIELD )
  {
    // If the incoming message has invalid fields in it
    // Send response back with status: rejected
    eventStatus = EVENT_STATUS_LOAD_CONTROL_EVENT_REJECTED;
  }
  else
  { // Send response back with status: received
    eventStatus = EVENT_STATUS_LOAD_CONTROL_EVENT_RECEIVED;
  }

  // Send response back
  loadcontrol_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEvent,
                                   pCmd->startTime, eventStatus,
                                   pCmd->criticalityLevel, pCmd->eventControl);


  if ( status != ZCL_STATUS_INVALID_FIELD )
  {
    // Start the Load Control Event
    if ( pCmd->issuerEvent == LOADCONTROL_EVENT_ID )
    {
      if ( pCmd->startTime == START_TIME_NOW ) // start time = NOW
      {
        // send back status event = load control event started
        eventStatus = EVENT_STATUS_LOAD_CONTROL_EVENT_STARTED;
        loadcontrol_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEvent,
                                   pCmd->startTime, eventStatus,
                                   pCmd->criticalityLevel, pCmd->eventControl);
        if ( pCmd->deviceGroupClass == ONOFF_LOAD_DEVICE_CLASS ) // is this one for residential on/off load?
        {
          HalLcdWriteString("Load Evt Started", HAL_LCD_LINE_3);
        }
        else if ( pCmd->deviceGroupClass == HVAC_DEVICE_CLASS ) // is this one for HVAC compressor/furnace?
        {
          HalLcdWriteString("PCT Evt Started", HAL_LCD_LINE_3);
        }
        HalLedBlink ( HAL_LED_4, 0, 50, 500 );

        osal_start_timerEx( loadControlTaskID, LOADCONTROL_LOAD_CTRL_EVT,
                           (LOADCONTROL_LOAD_CTRL_PERIOD * (pCmd->durationInMinutes)) );
      }
    }
  }
#endif // ZCL_LOAD_CONTROL
}

/*********************************************************************
 * @fn      loadcontrol_CancelLoadControlEventCB
 *
 * @brief   Callback from the ZCL SE Profile Load Control Cluster Library when
 *          it received a Cancel Load Control Event Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Cancel Load Control Event command
 * @param   scrAddr - source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void loadcontrol_CancelLoadControlEventCB( zclCCCancelLoadControlEvent_t *pCmd,
                                                afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_LOAD_CONTROL )
  if ( 0 )  // User shall replace the if condition with "if the event exist"
  {
    // If the event exist, stop the event, and respond with status: cancelled

    // Cancel the event here

    // Use the following sample code to send response back.
    /*
    loadcontrol_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEventID,
                                     // startTime
                                     EVENT_STATUS_LOAD_CONTROL_EVENT_CANCELLED, // eventStatus
                                     // Criticality level
                                     // eventControl };
    */

  }
  else
  {
    // If the event does not exist, respond with status: rejected
    // The rest of the mandatory fields are not available, therefore,
    // set to optional value
    loadcontrol_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEventID,
                                     SE_OPTIONAL_FIELD_UINT32,   // startTime
                                     EVENT_STATUS_LOAD_CONTROL_EVENT_RECEIVED, // eventStatus
                                     SE_OPTIONAL_FIELD_UINT8,    // Criticality level
                                     SE_OPTIONAL_FIELD_UINT8 );   // eventControl
  }

#endif // ZCL_LOAD_CONTROL

}

/*********************************************************************
 * @fn      loadcontrol_CancelAllLoadControlEventsCB
 *
 * @brief   Callback from the ZCL SE Profile Load Control Cluster Library when
 *          it received a Cancel All Load Control Event Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Cancel All Load Control Event command
 * @param   scrAddr - source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void loadcontrol_CancelAllLoadControlEventsCB( zclCCCancelAllLoadControlEvents_t *pCmd,
                                                    afAddrType_t *srcAddr, uint8 seqNum )
{
  // Upon receipt of Cancel All Load Control Event Command,
  // the receiving device shall look up the table for all events
  // and send a seperate response for each event

}

/*********************************************************************
 * @fn      loadcontrol_ReportEventStatusCB
 *
 * @brief   Callback from the ZCL SE Profile Load Control Cluster Library when
 *          it received a Report Event Status Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Report Event Status command
 * @param   scrAddr - source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void loadcontrol_ReportEventStatusCB( zclCCReportEventStatus_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum)
{
  // add user code here
}
/*********************************************************************
 * @fn      loadcontrol_GetScheduledEventCB
 *
 * @brief   Callback from the ZCL SE Profile Load Control Cluster Library when
 *          it received a Get Scheduled Event Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Get Scheduled Event command
 * @param   scrAddr - source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void loadcontrol_GetScheduledEventCB( zclCCGetScheduledEvent_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum )
{
 // add user code here
}

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      loadcontrol_ProcessZCLMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - message to process
 *
 * @return  none
 */
static void loadcontrol_ProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#if defined ( ZCL_READ )
    case ZCL_CMD_READ_RSP:
      loadcontrol_ProcessInReadRspCmd( pInMsg );
      break;
#endif // ZCL_READ
#if defined ( ZCL_WRITE )
    case ZCL_CMD_WRITE_RSP:
      loadcontrol_ProcessInWriteRspCmd( pInMsg );
      break;
#endif // ZCL_WRITE
    case ZCL_CMD_DEFAULT_RSP:
      loadcontrol_ProcessInDefaultRspCmd( pInMsg );
      break;
#if defined ( ZCL_DISCOVER )
    case ZCL_CMD_DISCOVER_RSP:
      loadcontrol_ProcessInDiscRspCmd( pInMsg );
      break;
#endif // ZCL_DISCOVER
    default:
      break;
  }

  if ( pInMsg->attrCmd != NULL )
  {
    // free the parsed command
    osal_mem_free( pInMsg->attrCmd );
    pInMsg->attrCmd = NULL;
  }
}

#if defined ( ZCL_READ )
/*********************************************************************
 * @fn      loadcontrol_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 loadcontrol_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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

#if defined ( ZCL_WRITE )
/*********************************************************************
 * @fn      loadcontrol_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 loadcontrol_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
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
 * @fn      loadcontrol_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 loadcontrol_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.

  return TRUE;
}

#if defined ( ZCL_DISCOVER )
/*********************************************************************
 * @fn      loadcontrol_ProcessInDiscRspCmd
 *
 * @brief   Process the "Profile" Discover Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 loadcontrol_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
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

/****************************************************************************
****************************************************************************/
