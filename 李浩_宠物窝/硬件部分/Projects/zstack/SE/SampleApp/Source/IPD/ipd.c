/**************************************************************************************************
  Filename:       ipd.c
  Revised:        $Date: 2012-04-02 17:02:19 -0700 (Mon, 02 Apr 2012) $
  Revision:       $Revision: 29996 $

  Description:    This module implements the IPD functionality and contains the
                  init and event loop functions


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

/*********************************************************************
  This application is designed for the test purpose of the SE profile which
  exploits the following clusters for an IPD configuration:

  General Basic
  General Alarms
  General Time
  General Key Establishment
  SE     Price
  SE     Message


  Key control:
    SW1:  Join Network
    SW2:  Send INTER-PAN Get Current Price Message
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
#include "ipd.h"
#include "zcl_general.h"
#include "zcl_se.h"
#include "zcl_key_establish.h"

#if defined( INTER_PAN )
  #include "stub_aps.h"
#endif

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

#define ipdNwkState  devState

// Request Fast Poll Mode parameters
#define IPD_FAST_POLL_UPDATE_PERIOD   2   // in seconds
#define IPD_FAST_POLL_DURATION        5   // in minutes

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

static uint8 ipdTaskID;            // osal task id for IPD
static uint8 ipdTransID;           // transaction id
static afAddrType_t ESPAddr;       // ESP destination address
#if SECURE
static uint8 linkKeyStatus;        // status variable returned from get link key function
#endif
static uint8 option;               // tx options field

#if defined (INTER_PAN)
static uint8 rxOnIdle;             // receiver on when idle flag

// define endpoint structure to register with STUB APS for INTER-PAN support
static endPointDesc_t ipdEp =
{
  IPD_ENDPOINT,
  &ipdTaskID,
  (SimpleDescriptionFormat_t *)&ipdSimpleDesc,
  (afNetworkLatencyReq_t)0
};
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void ipd_HandleKeys( uint8 shift, uint8 keys );

#if SECURE
static uint8 ipd_KeyEstablish_ReturnLinkKey( uint16 shortAddr );
#endif

static void ipd_ProcessIdentifyTimeChange( void );

/*************************************************************************/
/*** Application Callback Functions                                    ***/
/*************************************************************************/

// Foundation Callback functions
static uint8 ipd_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );

// General Cluster Callback functions
static void ipd_BasicResetCB( void );
static void ipd_IdentifyCB( zclIdentify_t *pCmd );
static void ipd_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void ipd_AlarmCB( zclAlarm_t *pAlarm );
#ifdef SE_UK_EXT
static void ipd_GetEventLogCB( uint8 srcEP, afAddrType_t *srcAddr,
                                   zclGetEventLog_t *pEventLog, uint8 seqNum );
static void ipd_PublishEventLogCB( afAddrType_t *srcAddr,
                                             zclPublishEventLog_t *pEventLog );
#endif // SE_UK_EXT

// Function to process ZDO callback messages
static void ipd_ProcessZDOMsgs( zdoIncomingMsg_t *pMsg );

// SE Callback functions
static void ipd_GetCurrentPriceCB( zclCCGetCurrentPrice_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_GetScheduledPriceCB( zclCCGetScheduledPrice_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PriceAcknowledgementCB( zclCCPriceAcknowledgement_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_GetBlockPeriodCB( zclCCGetBlockPeriod_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishPriceCB( zclCCPublishPrice_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishBlockPeriodCB( zclCCPublishBlockPeriod_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_DisplayMessageCB( zclCCDisplayMessage_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_CancelMessageCB( zclCCCancelMessage_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_GetLastMessageCB( afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_MessageConfirmationCB( zclCCMessageConfirmation_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_ReqFastPollModeCmdCB( zclCCReqFastPollModeCmd_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_ReqFastPollModeRspCB( zclCCReqFastPollModeRsp_t *pRsp,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_SelAvailEmergencyCreditCmdCB( zclCCSelAvailEmergencyCredit_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_ChangeSupplyCmdCB( zclCCChangeSupply_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_SupplyStatusRspCB( zclCCSupplyStatusResponse_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
#if defined ( SE_UK_EXT )
static void ipd_PublishTariffInformationCB( zclCCPublishTariffInformation_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishPriceMatrixCB( zclCCPublishPriceMatrix_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishBlockThresholdsCB( zclCCPublishBlockThresholds_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishConversionFactorCB( zclCCPublishConversionFactor_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishCalorificValueCB( zclCCPublishCalorificValue_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishCO2ValueCB( zclCCPublishCO2Value_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishCPPEventCB( zclCCPublishCPPEvent_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishBillingPeriodCB( zclCCPublishBillingPeriod_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishConsolidatedBillCB( zclCCPublishConsolidatedBill_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishCreditPaymentInfoCB( zclCCPublishCreditPaymentInfo_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_GetPrepaySnapshotResponseCB( zclCCGetPrepaySnapshotResponse_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_ChangePaymentModeResponseCB( zclCCChangePaymentModeResponse_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_ConsumerTopupResponseCB( zclCCConsumerTopupResponse_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_GetCommandsCB( uint8 prepayNotificationFlags,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishTopupLogCB( zclCCPublishTopupLog_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
static void ipd_PublishDebtLogCB( zclCCPublishDebtLog_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum );
#endif  // SE_UK_EXT

/************************************************************************/
/***               Functions to process ZCL Foundation                ***/
/***               incoming Command/Response messages                 ***/
/************************************************************************/
static void ipd_ProcessZCLMsg( zclIncomingMsg_t *msg );
#if defined ( ZCL_READ )
static uint8 ipd_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_READ
#if defined ( ZCL_WRITE )
static uint8 ipd_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_WRITE
static uint8 ipd_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#if defined ( ZCL_DISCOVER )
static uint8 ipd_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_DISCOVER

/*********************************************************************
 * ZCL General Clusters Callback table
 */
static zclGeneral_AppCallbacks_t ipd_GenCmdCallbacks =
{
  ipd_BasicResetCB,              // Basic Cluster Reset command
  ipd_IdentifyCB,                // Identify command
  ipd_IdentifyQueryRspCB,        // Identify Query Response command
  NULL,                          // On/Off cluster commands
  NULL,                          // Level Control Move to Level command
  NULL,                          // Level Control Move command
  NULL,                          // Level Control Step command
  NULL,                          // Level Control Stop command
  NULL,                          // Group Response commands
  NULL,                          // Scene Store Request command
  NULL,                          // Scene Recall Request command
  NULL,                          // Scene Response command
  ipd_AlarmCB,                   // Alarm (Response) command
#ifdef SE_UK_EXT
  ipd_GetEventLogCB,             // Get Event Log command
  ipd_PublishEventLogCB,         // Publish Event Log command
#endif
  NULL,                          // RSSI Location command
  NULL                           // RSSI Location Response command
};

/*********************************************************************
 * ZCL SE Clusters Callback table
 */
static zclSE_AppCallbacks_t ipd_SECmdCallbacks =
{
  ipd_PublishPriceCB,               // Publish Price
  ipd_PublishBlockPeriodCB,         // Publish Block Period
#if defined ( SE_UK_EXT )
  ipd_PublishTariffInformationCB,   // Publish Tariff Information
  ipd_PublishPriceMatrixCB,         // Publish Price Matrix
  ipd_PublishBlockThresholdsCB,     // Publish Block Thresholds
  ipd_PublishConversionFactorCB,    // Publish Conversion Factor
  ipd_PublishCalorificValueCB,      // Publish Calorific Value
  ipd_PublishCO2ValueCB,            // Publish CO2 Value
  ipd_PublishCPPEventCB,            // Publish CPP Event
  ipd_PublishBillingPeriodCB,       // Publish Billing Period
  ipd_PublishConsolidatedBillCB,    // Publish Consolidated Bill
  ipd_PublishCreditPaymentInfoCB,   // Publish Credit Payment Info
#endif  // SE_UK_EXT
  ipd_GetCurrentPriceCB,            // Get Current Price
  ipd_GetScheduledPriceCB,          // Get Scheduled Price
  ipd_PriceAcknowledgementCB,       // Price Acknowledgement
  ipd_GetBlockPeriodCB,             // Get Block Period
#if defined ( SE_UK_EXT )
  NULL,                             // Get Tariff Information
  NULL,                             // Get Price Matrix
  NULL,                             // Get Block Thresholds
  NULL,                             // Get Conversion Factor
  NULL,                             // Get Calorific Value
  NULL,                             // Get CO2 Value
  NULL,                             // Get Billing Period
  NULL,                             // Get Consolidated Bill
  NULL,                             // CPP Event Response
#endif  // SE_UK_EXT
  NULL,                             // Load Control Event
  NULL,                             // Cancel Load Control Event
  NULL,                             // Cancel All Load Control Events
  NULL,                             // Report Event Status
  NULL,                             // Get Scheduled Event
  NULL,                             // Get Profile Response
  NULL,                             // Request Mirror Command
  NULL,                             // Mirror Remove Command
  ipd_ReqFastPollModeRspCB,         // Request Fast Poll Mode Response
#if defined ( SE_UK_EXT )
  NULL,                             // Get Snapshot Response
#endif  // SE_UK_EXT
  NULL,                             // Get Profile Command
  NULL,                             // Request Mirror Response
  NULL,                             // Mirror Remove Response
  ipd_ReqFastPollModeCmdCB,         // Request Fast Poll Mode Command
#if defined ( SE_UK_EXT )
  NULL,                             // Get Snapshot Command
  NULL,                             // Take Snapshot Command
  NULL,                             // Mirror Report Attribute Response
#endif  // SE_UK_EXT
  ipd_DisplayMessageCB,             // Display Message Command
  ipd_CancelMessageCB,              // Cancel Message Command
  ipd_GetLastMessageCB,             // Get Last Message Command
  ipd_MessageConfirmationCB,        // Message Confirmation
  NULL,                             // Request Tunnel Response
  NULL,                             // Transfer Data
  NULL,                             // Transfer Data Error
  NULL,                             // Ack Transfer Data
  NULL,                             // Ready Data
#if defined ( SE_UK_EXT )
  NULL,                             // Supported Tunnel Protocols Response
  NULL,                             // Tunnel Closure Notification
#endif  // SE_UK_EXT
  NULL,                             // Request Tunnel
  NULL,                             // Close Tunnel
#if defined ( SE_UK_EXT )
  NULL,                             // Get Supported Tunnel Protocols
#endif  // SE_UK_EXT
  ipd_SupplyStatusRspCB,            // Supply Status Response
#if defined ( SE_UK_EXT )
  ipd_GetPrepaySnapshotResponseCB,  // Get Prepay Snapshot Response
  ipd_ChangePaymentModeResponseCB,  // Change Payment Mode Response
  ipd_ConsumerTopupResponseCB,      // Consumer Topup Response
  ipd_GetCommandsCB,                // Get Commands
  ipd_PublishTopupLogCB,            // Publish Topup Log
  ipd_PublishDebtLogCB,             // Publish Debt Log
#endif  // SE_UK_EXT
  ipd_SelAvailEmergencyCreditCmdCB, // Select Available Emergency Credit Command
  ipd_ChangeSupplyCmdCB,            // Change Supply Command
#if defined ( SE_UK_EXT )
  NULL,                             // Change Debt
  NULL,                             // Emergency Credit Setup
  NULL,                             // Consumer Topup
  NULL,                             // Credit Adjustment
  NULL,                             // Change PaymentMode
  NULL,                             // Get Prepay Snapshot
  NULL,                             // Get Topup Log
  NULL,                             // Set Low Credit Warning Level
  NULL,                             // Get Debt Repayment Log
  NULL,                             // Publish Calendar
  NULL,                             // Publish Day Profile
  NULL,                             // Publish Week Profile
  NULL,                             // Publish Seasons
  NULL,                             // Publish Special Days
  NULL,                             // Get Calendar
  NULL,                             // Get Day Profiles
  NULL,                             // Get Week Profiles
  NULL,                             // Get Seasons
  NULL,                             // Get Special Days
  NULL,                             // Publish Change Tenancy
  NULL,                             // Publish Change Supplier
  NULL,                             // Change Supply
  NULL,                             // Change Password
  NULL,                             // Local Change Supply
  NULL,                             // Get Change Tenancy
  NULL,                             // Get Change Supplier
  NULL,                             // Get Change Supply
  NULL,                             // Supply Status Response
  NULL,                             // Get Password
#endif  // SE_UK_EXT
};

/*********************************************************************
 * @fn          ipd_Init
 *
 * @brief       Initialization function for the ZCL App Application.
 *
 * @param       uint8 task_id - ipd task id
 *
 * @return      none
 */
void ipd_Init( uint8 task_id )
{
  ipdTaskID = task_id;
  ipdTransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  // setup ESP destination address
  ESPAddr.addrMode = (afAddrMode_t)Addr16Bit;
  ESPAddr.endPoint = IPD_ENDPOINT;
  ESPAddr.addr.shortAddr = 0;

  // Register for SE endpoint
  zclSE_Init( &ipdSimpleDesc );

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( IPD_ENDPOINT, &ipd_GenCmdCallbacks );

  // Register the ZCL SE Cluster Library callback functions
  zclSE_RegisterCmdCallbacks( IPD_ENDPOINT, &ipd_SECmdCallbacks );

  // Register the application's attribute list
  zcl_registerAttrList( IPD_ENDPOINT, IPD_MAX_ATTRIBUTES, ipdAttrs );

  // Register the application's cluster option list
  zcl_registerClusterOptionList( IPD_ENDPOINT, IPD_MAX_OPTIONS, ipdOptions );

  // Register the application's attribute data validation callback function
  zcl_registerValidateAttrData( ipd_ValidateAttrDataCB );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( ipdTaskID );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( ipdTaskID );

  // Register with the ZDO to receive Match Descriptor Responses
  ZDO_RegisterForZDOMsg(task_id, Match_Desc_rsp);

#if defined ( INTER_PAN )
  // Register with Stub APS
  StubAPS_RegisterApp( &ipdEp );
#endif

  // Start the timer to sync IPD timer with the osal timer
  osal_start_timerEx( ipdTaskID, IPD_UPDATE_TIME_EVT, IPD_UPDATE_TIME_PERIOD );
}

/*********************************************************************
 * @fn          ipd_event_loop
 *
 * @brief       Event Loop Processor for ipd.
 *
 * @param       uint8 task_id - ipd task id
 * @param       uint16 events - event bitmask
 *
 * @return      none
 */
uint16 ipd_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ipdTaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          ipd_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;

        case ZCL_INCOMING_MSG:
          // Incoming ZCL foundation command/response messages
          ipd_ProcessZCLMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          ipd_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_STATE_CHANGE:
          if (DEV_END_DEVICE == (devStates_t)(MSGpkt->hdr.status))
          {
#if SECURE
            {
              // check to see if link key had already been established
              linkKeyStatus = ipd_KeyEstablish_ReturnLinkKey(ESPAddr.addr.shortAddr);

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
              else
              {
#if !defined ( ZCL_PREPAYMENT ) && !defined ( FAST_POLL_TEST )
                // link key already established, resume sending reports
                osal_start_timerEx( ipdTaskID, IPD_GET_PRICING_INFO_EVT, IPD_GET_PRICING_INFO_PERIOD );
#endif
              }
            }
#else
          {
#if !defined ( ZCL_PREPAYMENT ) && !defined ( FAST_POLL_TEST )
            osal_start_timerEx( ipdTaskID, IPD_GET_PRICING_INFO_EVT, IPD_GET_PRICING_INFO_PERIOD );
#endif
          }
#endif
            // Per smart energy spec end device polling requirement of not to poll < 7.5 seconds.
            NLME_SetPollRate ( SE_DEVICE_POLL_RATE );
          }
          break;

#if defined( ZCL_KEY_ESTABLISH )
        case ZCL_KEY_ESTABLISH_IND:
          if ((MSGpkt->hdr.status) == TermKeyStatus_Success)
          {
            ESPAddr.endPoint = IPD_ENDPOINT; // set destination endpoint back to application endpoint

#if !defined ( ZCL_PREPAYMENT ) && !defined ( FAST_POLL_TEST )
            osal_start_timerEx( ipdTaskID, IPD_GET_PRICING_INFO_EVT, IPD_GET_PRICING_INFO_PERIOD );
#endif
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
  if ( events & IPD_KEY_ESTABLISHMENT_REQUEST_EVT )
  {
    zclGeneral_KeyEstablish_InitiateKeyEstablishment(ipdTaskID, &ESPAddr, ipdTransID);

    return ( events ^ IPD_KEY_ESTABLISHMENT_REQUEST_EVT );
  }

  // event to get current price
  if ( events & IPD_GET_PRICING_INFO_EVT )
  {
#if defined( INTER_PAN )
    uint8 x = TRUE;
    ZMacGetReq( ZMacRxOnIdle, &rxOnIdle );
    ZMacSetReq( ZMacRxOnIdle, &x );
#endif
    zclSE_Pricing_Send_GetCurrentPrice( IPD_ENDPOINT, &ESPAddr, option, TRUE, 0 );

    osal_start_timerEx( ipdTaskID, IPD_GET_PRICING_INFO_EVT, IPD_GET_PRICING_INFO_PERIOD );

    return ( events ^ IPD_GET_PRICING_INFO_EVT );
  }

  // handle processing of identify timeout event triggered by an identify command
  if ( events & IPD_IDENTIFY_TIMEOUT_EVT )
  {
    if ( ipdIdentifyTime > 0 )
    {
      ipdIdentifyTime--;
    }
    ipd_ProcessIdentifyTimeChange();

    return ( events ^ IPD_IDENTIFY_TIMEOUT_EVT );
  }

  // event to get current time
  if ( events & IPD_UPDATE_TIME_EVT )
  {
    ipdTime = osal_getClock();
    osal_start_timerEx( ipdTaskID, IPD_UPDATE_TIME_EVT, IPD_UPDATE_TIME_PERIOD );

    return ( events ^ IPD_UPDATE_TIME_EVT );
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      ipd_ProcessZDOMsgs
 *
 * @brief   Called to process callbacks from the ZDO.
 *
 * @param   none
 *
 * @return  none
 */
static void ipd_ProcessZDOMsgs( zdoIncomingMsg_t *pMsg )
{
  // make sure message comes from TC to initiate Key Establishment
  if ((pMsg->clusterID == Match_Desc_rsp) &&
      (pMsg->srcAddr.addr.shortAddr == zgTrustCenterAddr))
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
        osal_set_event( ipdTaskID, IPD_KEY_ESTABLISHMENT_REQUEST_EVT);
      }
      osal_mem_free(pRsp);
    }
  }
}

/*********************************************************************
 * @fn      ipd_ProcessIdentifyTimeChange
 *
 * @brief   Called to blink led for specified IdentifyTime attribute value
 *
 * @param   none
 *
 * @return  none
 */
static void ipd_ProcessIdentifyTimeChange( void )
{
  if ( ipdIdentifyTime > 0 )
  {
    osal_start_timerEx( ipdTaskID, IPD_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( ipdTaskID, IPD_IDENTIFY_TIMEOUT_EVT );
  }
}

#if SECURE
/*********************************************************************
 * @fn      ipd_KeyEstablish_ReturnLinkKey
 *
 * @brief   This function get the requested link key
 *
 * @param   shortAddr - short address of the partner.
 *
 * @return  none
 */
static uint8 ipd_KeyEstablish_ReturnLinkKey( uint16 shortAddr )
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
 * @fn      ipd_HandleKeys
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
static void ipd_HandleKeys( uint8 shift, uint8 keys )
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
#if defined( INTER_PAN )

      uint8 x = TRUE;
      ZMacGetReq( ZMacRxOnIdle, &rxOnIdle );
      ZMacSetReq( ZMacRxOnIdle, &x );
      afAddrType_t dstAddr;
      uint8 option = 1;

      // Send a request for public pricing information using the INTERP-DATA SAP.
      // The request is sent as a broadcast to all PANs within the discovered
      // channel. Receiving devices that implement the INTRP-DATA SAP will process
      // it and, if any such device is able to respond, it will respond directly
      // to the requestor. After receiving at least one response the requestor may
      // store the PAN ID and device address of one or more responders so that it
      // may query them directly in future.
      dstAddr.addrMode = afAddrBroadcast;
      dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
      dstAddr.endPoint = STUBAPS_INTER_PAN_EP;
      dstAddr.panId = 0xFFFF;

      zclSE_Pricing_Send_GetCurrentPrice( IPD_ENDPOINT, &dstAddr, option, TRUE, 0 );
#endif
    }

    if ( keys & HAL_KEY_SW_3 )
    {
#if defined ( ZCL_PREPAYMENT )
      zclCCSelAvailEmergencyCredit_t cmd;
      uint8 siteIdLen = 4; // adjust this value if different label is set, test label "TEST"
      uint8 meterSerialNumLen = 6; // adjust this value if different label is set, test label "123456"

      osal_memset( &cmd, 0, sizeof( zclCCSelAvailEmergencyCredit_t ) );

      cmd.commandDateTime = osal_getClock();
      cmd.originatingDevice = SE_ORIG_DEV_INPREMISES_DISPLAY_DEV;

      cmd.siteId.pStr = (uint8 *)osal_mem_alloc(siteIdLen);
      if (cmd.siteId.pStr != NULL)
      {
        cmd.siteId.strLen = siteIdLen;
        osal_memcpy(cmd.siteId.pStr, "TEST", siteIdLen);
      }

      cmd.meterSerialNumber.pStr = (uint8 *)osal_mem_alloc(meterSerialNumLen);
      if (cmd.meterSerialNumber.pStr != NULL)
      {
        cmd.meterSerialNumber.strLen = meterSerialNumLen;
        osal_memcpy(cmd.meterSerialNumber.pStr, "123456", meterSerialNumLen);
      }

      zclSE_Prepayment_Send_SelAvailEmergencyCredit(IPD_ENDPOINT,
                                                    &ESPAddr,
                                                    &cmd, FALSE, 1 );
      if (cmd.siteId.pStr != NULL)
      {
        osal_mem_free(cmd.siteId.pStr);
      }

      if (cmd.meterSerialNumber.pStr != NULL)
      {
        osal_mem_free(cmd.meterSerialNumber.pStr);
      }

#else // for regular IPD test

#if  defined ( FAST_POLL_TEST )
      zclCCReqFastPollModeCmd_t fastPollCmd;

      fastPollCmd.fastPollUpdatePeriod = IPD_FAST_POLL_UPDATE_PERIOD;
      fastPollCmd.duration = IPD_FAST_POLL_DURATION;

      // send out fast poll mode request
      zclSE_SimpleMetering_Send_ReqFastPollModeCmd( IPD_ENDPOINT, &ESPAddr,
                                                    &fastPollCmd,
                                                    TRUE, 1);
#endif // FAST_POLL_TEST

#endif  // ZCL_PREPAYMENT
    }

    if ( keys & HAL_KEY_SW_4 )
    {
#if defined ( ZCL_PREPAYMENT )
#if !defined ( SE_UK_EXT )
      zclCCChangeSupply_t changeCmd;
      uint8 siteIdLen = 4; // adjust this value if different label is set, test label "TEST"
      uint8 meterSerialNumLen = 6; // adjust this value if different label is set, test label "123456"

      changeCmd.providerId = 0xbabeface;
      changeCmd.requestDateTime = osal_getClock();

      changeCmd.siteId.pStr = (uint8 *)osal_mem_alloc(siteIdLen);
      if (changeCmd.siteId.pStr != NULL)
      {
        changeCmd.siteId.strLen = siteIdLen;
        osal_memcpy(changeCmd.siteId.pStr, "TEST", siteIdLen);
      }

      changeCmd.meterSerialNumber.pStr = (uint8 *)osal_mem_alloc(meterSerialNumLen);
      if (changeCmd.meterSerialNumber.pStr != NULL)
      {
        changeCmd.meterSerialNumber.strLen = meterSerialNumLen;
        osal_memcpy(changeCmd.meterSerialNumber.pStr, "123456", meterSerialNumLen);
      }

      changeCmd.implementationDateTime = 0x00000000;  // immediately
      changeCmd.proposedSupplyStatus  = SE_SUPPLY_ON;
      changeCmd.origIdSupplyControlBits = ((SE_ORIG_DEV_INPREMISES_DISPLAY_DEV << 4) | SE_SUPPLY_CTRL_ACK_REQUIRED);

      zclSE_Prepayment_Send_ChangeSupply( IPD_ENDPOINT, &ESPAddr,
                                          &changeCmd, TRUE, 1);

      if (changeCmd.siteId.pStr != NULL)
      {
        osal_mem_free(changeCmd.siteId.pStr);
      }

      if (changeCmd.meterSerialNumber.pStr != NULL)
      {
        osal_mem_free(changeCmd.meterSerialNumber.pStr);
      }
#endif  // SE_UK_EXT
#endif  // ZCL_PREPAYMENT
    }
  }
}

/*********************************************************************
 * @fn      ipd_ValidateAttrDataCB
 *
 * @brief   Check to see if the supplied value for the attribute data
 *          is within the specified range of the attribute.
 *
 * @param   pAttr - pointer to attribute
 * @param   pAttrInfo - pointer to attribute info
 *
 * @return  TRUE if data valid. FALSE otherwise.
 */
static uint8 ipd_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
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
 * @fn      ipd_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library to set all
 *          the attributes of all the clusters to their factory defaults
 *
 * @param   none
 *
 * @return  none
 */
static void ipd_BasicResetCB( void )
{
  // user should handle setting attributes to factory defaults here
}

/*********************************************************************
 * @fn      ipd_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   pCmd - pointer to structure for Identify command
 *
 * @return  none
 */
static void ipd_IdentifyCB( zclIdentify_t *pCmd )
{
  ipdIdentifyTime = pCmd->identifyTime;
  ipd_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      ipd_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   pRsp - pointer to structure for Identity Query Response command
 *
 * @return  none
 */
static void ipd_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_AlarmCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Alarm request or response command for
 *          this application.
 *
 * @param   pAlarm - pointer to structure for Alarm command
 *
 * @return  none
 */
static void ipd_AlarmCB( zclAlarm_t *pAlarm )
{
  // add user code here
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      ipd_GetEventLogCB
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
static void ipd_GetEventLogCB( uint8 srcEP, afAddrType_t *srcAddr,
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
 * @fn      ipd_PublishEventLogCB
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
static void ipd_PublishEventLogCB( afAddrType_t *srcAddr, zclPublishEventLog_t *pEventLog )
{
  // add user code here
}
#endif // SE_UK_EXT

/*********************************************************************
 * @fn      ipd_GetCurrentPriceCB
 *
 * @brief   Callback from the ZCL SE Profile Pricing Cluster Library when
 *          it received a Get Current Price for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Get Current Price command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_GetCurrentPriceCB( zclCCGetCurrentPrice_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_PRICING )
  // On receipt of Get Current Price command, the device shall send a
  // Publish Price command with the information for the current time.
  zclCCPublishPrice_t cmd;

  osal_memset( &cmd, 0, sizeof( zclCCPublishPrice_t ) );

  cmd.providerId = 0xbabeface;
  cmd.numberOfPriceTiers = 0xfe;

  zclSE_Pricing_Send_PublishPrice( IPD_ENDPOINT, srcAddr, &cmd, FALSE, seqNum );
#endif // ZCL_PRICING
}

/*********************************************************************
 * @fn      ipd_GetScheduledPriceCB
 *
 * @brief   Callback from the ZCL SE Profile Pricing Cluster Library when
 *          it received a Get Scheduled Price for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Get Scheduled Price command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_GetScheduledPriceCB( zclCCGetScheduledPrice_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum  )
{
  // On receipt of Get Scheduled Price command, the device shall send a
  // Publish Price command for all currently scheduled price events.
  // The sample code as follows only sends one.

#if defined ( ZCL_PRICING )
  zclCCPublishPrice_t cmd;

  osal_memset( &cmd, 0, sizeof( zclCCPublishPrice_t ) );

  cmd.providerId = 0xbabeface;
  cmd.numberOfPriceTiers = 0xfe;

  zclSE_Pricing_Send_PublishPrice( IPD_ENDPOINT, srcAddr, &cmd, FALSE, seqNum );

#endif // ZCL_PRICING
}

/*********************************************************************
 * @fn      ipd_PriceAcknowledgementCB
 *
 * @brief   Callback from the ZCL SE Profile Pricing Cluster Library when
 *          it received a Price Acknowledgement for this application.
 *
 * @param   pCmd - pointer to structure for Price Acknowledgement command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
void ipd_PriceAcknowledgementCB( zclCCPriceAcknowledgement_t *pCmd,
                                 afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_GetBlockPeriodCB
 *
 * @brief   Callback from the ZCL SE Profile Pricing Cluster Library when
 *          it received a Get Block Period for this application.
 *
 * @param   pCmd - pointer to structure for Get Block Period command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_GetBlockPeriodCB( zclCCGetBlockPeriod_t *pCmd,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishPriceCB
 *
 * @brief   Callback from the ZCL SE Profile Pricing Cluster Library when
 *          it received a Publish Price for this application.
 *
 * @param   pCmd - pointer to structure for Publish Price command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_PublishPriceCB( zclCCPublishPrice_t *pCmd,
                                afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_PRICING )
  if ( pCmd )
  {
    // display Provider ID field
    HalLcdWriteString("Provider ID", HAL_LCD_LINE_1);
    HalLcdWriteValue( pCmd->providerId, 10, HAL_LCD_LINE_2 );
  }

  // Verify Price Control Options
  if ((pCmd->priceControl != SE_OPTIONAL_FIELD_UINT8) &&
      (pCmd->priceControl & SE_PROFILE_PRICEACK_REQUIRED_MASK))
  {
    zclCCPriceAcknowledgement_t cmd;

    cmd.providerId = pCmd->providerId;
    cmd.issuerEventId = pCmd->issuerEventId;
    cmd.priceAckTime = osal_getClock();
    cmd.control = pCmd->priceControl;

    zclSE_Pricing_Send_PriceAcknowledgement( IPD_ENDPOINT, srcAddr,
                                            &cmd, TRUE, seqNum );
  }

#if defined ( INTER_PAN )
  ZMacSetReq( ZMacRxOnIdle, &rxOnIdle ); // set receiver on when idle flag to FALSE
                                         // after getting publish price command via INTER-PAN
#endif // INTER_PAN
#endif // ZCL_PRICING
}

/*********************************************************************
 * @fn      ipd_PublishBlockPeriodCB
 *
 * @brief   Callback from the ZCL SE Profile Pricing Cluster Library when
 *          it received a Publish Block Period for this application.
 *
 * @param   pCmd - pointer to structure for Get Block Period command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_PublishBlockPeriodCB( zclCCPublishBlockPeriod_t *pCmd,
                                      afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_DisplayMessageCB
 *
 * @brief   Callback from the ZCL SE Profile Message Cluster Library when
 *          it received a Display Message Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Display Message command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_DisplayMessageCB( zclCCDisplayMessage_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // Upon receipt of the Display Message Command, the device shall
  // display the message. If the Message Confirmation bit indicates
  // the message originator require a confirmation of receipt from
  // a Utility Customer, the device should display the message or
  // alert the user until it is either confirmed via a button or by
  // selecting a confirmation option on the device.  Confirmation is
  // typically used when the Utility is sending down information
  // such as a disconnection notice, or prepaid billing information.
  // Message duration is ignored when confirmation is requested and
  // the message is displayed until confirmed.

#if defined ( LCD_SUPPORTED )
  // Allowing that strings have a non-printing '\0' terminator.
  if (pCmd->msgString.strLen <= HAL_LCD_MAX_CHARS+1)
  {
    HalLcdWriteString((char*)pCmd->msgString.pStr, HAL_LCD_LINE_3);
  }
  else
  {
    // Allow 3 digit message remainder size and space; and the ellipse ("...") plus the plus ("+"):
    const uint8 left = HAL_LCD_MAX_CHARS - 4 - 4;
    uint8 buf[HAL_LCD_MAX_CHARS];

    (void)osal_memcpy(buf, pCmd->msgString.pStr, left);
    (void)osal_memcpy(buf+left, "...+\0", 5);  // Copy the "end-of-string" delimiter.
    HalLcdWriteStringValue((char *)buf, pCmd->msgString.strLen-left, 10, HAL_LCD_LINE_3);
  }
#endif // LCD_SUPPORTED
}

/*********************************************************************
 * @fn      ipd_CancelMessageCB
 *
 * @brief   Callback from the ZCL SE Profile Message Cluster Library when
 *          it received a Cancel Message Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Cancel Message command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_CancelMessageCB( zclCCCancelMessage_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_GetLastMessageCB
 *
 * @brief   Callback from the ZCL SE Profile Message Cluster Library when
 *          it received a Get Last Message Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Get Last Message command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_GetLastMessageCB( afAddrType_t *srcAddr, uint8 seqNum )
{
  // On receipt of Get Last Message command, the device shall send a
  // Display Message command back to the sender

#if defined ( ZCL_MESSAGE )
  zclCCDisplayMessage_t cmd;
  uint8 msg[10] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29 };

  // Fill in the command with information for the last message
  cmd.messageId = 0xaabbccdd;
  cmd.messageCtrl.transmissionMode = 0;
  cmd.messageCtrl.importance = 1;
  cmd.messageCtrl.confirmationRequired = 1;
  cmd.durationInMinutes = 60;

  cmd.msgString.strLen = 10;
  cmd.msgString.pStr = msg;

  zclSE_Message_Send_DisplayMessage( IPD_ENDPOINT, srcAddr, &cmd,
                                     FALSE, seqNum );
#endif // ZCL_MESSAGE
}

/*********************************************************************
 * @fn      ipd_MessageConfirmationCB
 *
 * @brief   Callback from the ZCL SE Profile Message Cluster Library when
 *          it received a Message Confirmation Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Message Confirmation command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number for this command
 *
 * @return  none
 */
static void ipd_MessageConfirmationCB( zclCCMessageConfirmation_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
 // add user code here
}

/*********************************************************************
 * @fn      ipd_ReqFastPollModeCmdCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Request Fast Poll Mode Command for this application.
 *
 * @param   pCmd - pointer to structure for Request Fast Poll Mode command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_ReqFastPollModeCmdCB( zclCCReqFastPollModeCmd_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}


/*********************************************************************
 * @fn      ipd_ReqFastPollModeRspCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Request Fast Poll Mode Response for this application.
 *
 * @param   pRsp - pointer to structure for Request Fast Poll Mode Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_ReqFastPollModeRspCB( zclCCReqFastPollModeRsp_t *pRsp,
                                      afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_SIMPLE_METERING )
#if defined ( LCD_SUPPORTED )

  HalLcdWriteString("Fast Polling", HAL_LCD_LINE_1 );
  HalLcdWriteStringValue("App: ", pRsp->appliedUpdatePeriod, 10, HAL_LCD_LINE_2 );
  HalLcdWriteStringValue("End 0x", pRsp->fastPollModeEndTime, 16, HAL_LCD_LINE_3 );

#endif // LCD_SUPPORTED
#endif // ZCL_SIMPLE_METERING
}

/*********************************************************************
 * @fn      ipd_SelAvailEmergencyCreditCmdCB
 *
 * @brief   Callback from the ZCL SE Prepayment Cluster Library when it recieved
 *          Select Available Emergency Credit command in the application
 *
 *
 * @param   pCmd - pointer to structure for Select Available Emergency Credit command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - Sequence no of the message
 *
 * @return  none
 */
static void ipd_SelAvailEmergencyCreditCmdCB( zclCCSelAvailEmergencyCredit_t *pCmd,
                                              afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_ChangeSupplyCmdCB
 *
 * @brief   Callback from the ZCL SE Prepayment Cluster Library when it recieved
 *          Change Supply command in the application
 *
 * @param   pCmd - pointer to structure for Change Supply command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - Sequence no of the message
 *
 * @return  none
 */
static void ipd_ChangeSupplyCmdCB( zclCCChangeSupply_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_SupplyStatusRspCB
 *
 * @brief   Callback from the ZCL SE Prepayment Cluster Library when it recieved
 *          Supply Status Response command in the application
 *
 * @param   pCmd - pointer to structure for Supply Status Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - Sequence no of the message
 *
 * @return  none
 */
static void ipd_SupplyStatusRspCB( zclCCSupplyStatusResponse_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_PREPAYMENT )
#if ! defined ( SE_UK_EXT )
#if defined ( LCD_SUPPORTED )
  if ( pCmd )
  {
    // display Provider ID field
    HalLcdWriteString("SupplyStatusRsp", HAL_LCD_LINE_1 );
    HalLcdWriteValue( pCmd->providerId, 10, HAL_LCD_LINE_2 );
    HalLcdWriteStringValue("Status 0x", pCmd->supplyStatus, 16, HAL_LCD_LINE_3 );
  }
#endif // LCD_SUPPORTED
#endif // SE_UK_EXT
#endif // ZCL_PREPAYMENT
}

#if defined ( SE_UK_EXT )
/*********************************************************************
 * @fn      ipd_PublishTariffInformationCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish Tariff Information for this application.
 *
 * @param   pCmd - pointer to structure for Publish Tariff Information command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishTariffInformationCB( zclCCPublishTariffInformation_t *pCmd,
                                            afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishPriceMatrixCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish Price Matrix for this application.
 *
 * @param   pCmd - pointer to structure for Publish Price Matrix command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishPriceMatrixCB( zclCCPublishPriceMatrix_t *pCmd,
                                      afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishBlockThresholdsCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish Block Thresholds for this application.
 *
 * @param   pCmd - pointer to structure for Publish Block Thresholds command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishBlockThresholdsCB( zclCCPublishBlockThresholds_t *pCmd,
                                          afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishConversionFactorCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish Conversion Factor for this application.
 *
 * @param   pCmd - pointer to structure for Publish Conversion Factor command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishConversionFactorCB( zclCCPublishConversionFactor_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishCalorificValueCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish Calorific Value for this application.
 *
 * @param   pCmd - pointer to structure for Publish Calorific Value command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishCalorificValueCB( zclCCPublishCalorificValue_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishCO2ValueCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish CO2 Value for this application.
 *
 * @param   pCmd - pointer to structure for Publish CO2 Value command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishCO2ValueCB( zclCCPublishCO2Value_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishCPPEventCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish CPP Event for this application.
 *
 * @param   pCmd - pointer to structure for Publish CPP Event command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishCPPEventCB( zclCCPublishCPPEvent_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishBillingPeriodCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish Billing Period for this application.
 *
 * @param   pCmd - pointer to structure for Publish Billing Period command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishBillingPeriodCB( zclCCPublishBillingPeriod_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishConsolidatedBillCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish Consolidated Bill for this application.
 *
 * @param   pCmd - pointer to structure for Publish Consolidated Bill command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishConsolidatedBillCB( zclCCPublishConsolidatedBill_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishCreditPaymentInfoCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Publish Credit Payment Info for this application.
 *
 * @param   pCmd - pointer to structure for Publish Credit Payment Info command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishCreditPaymentInfoCB( zclCCPublishCreditPaymentInfo_t *pCmd,
                                            afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_GetPrepaySnapshotResponseCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Get Prepay Snapshot Response for this application.
 *
 * @param   pCmd - pointer to structure for Get Prepay Snapshot Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_GetPrepaySnapshotResponseCB( zclCCGetPrepaySnapshotResponse_t *pCmd,
                                             afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_ChangePaymentModeResponseCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Change Payment Mode Response for this application.
 *
 * @param   pCmd - pointer to structure for Change Payment Mode Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_ChangePaymentModeResponseCB( zclCCChangePaymentModeResponse_t *pCmd,
                                             afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_ConsumerTopupResponseCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Consumer Topup Response for this application.
 *
 * @param   pCmd - pointer to structure for Consumer Topup Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_ConsumerTopupResponseCB( zclCCConsumerTopupResponse_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_GetCommandsCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Get Commands for this application.
 *
 * @param   prepayNotificationFlags - prepayment notification flags
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_GetCommandsCB( uint8 prepayNotificationFlags,
                               afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishTopupLogCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Publish Topup Log for this application.
 *
 * @param   pCmd - pointer to structure for Publish Topup Log command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishTopupLogCB( zclCCPublishTopupLog_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      ipd_PublishDebtLogCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Publish Debt Log for this application.
 *
 * @param   pCmd - pointer to structure for Publish Debt Log command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void ipd_PublishDebtLogCB( zclCCPublishDebtLog_t *pCmd,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}
#endif  // SE_UK_EXT

/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      ipd_ProcessZCLMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - message to process
 *
 * @return  none
 */
static void ipd_ProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#if defined ( ZCL_READ )
    case ZCL_CMD_READ_RSP:
      ipd_ProcessInReadRspCmd( pInMsg );
      break;
#endif // ZCL_READ
#if defined ( ZCL_WRITE )
    case ZCL_CMD_WRITE_RSP:
      ipd_ProcessInWriteRspCmd( pInMsg );
      break;
#endif // ZCL_WRITE
    case ZCL_CMD_DEFAULT_RSP:
      ipd_ProcessInDefaultRspCmd( pInMsg );
      break;
#if defined ( ZCL_DISCOVER )
    case ZCL_CMD_DISCOVER_RSP:
      ipd_ProcessInDiscRspCmd( pInMsg );
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
 * @fn      ipd_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 ipd_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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
 * @fn      ipd_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 ipd_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
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
 * @fn      ipd_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 ipd_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.

  return TRUE;
}

#if defined ( ZCL_DISCOVER )
/*********************************************************************
 * @fn      ipd_ProcessInDiscRspCmd
 *
 * @brief   Process the "Profile" Discover Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 ipd_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
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
