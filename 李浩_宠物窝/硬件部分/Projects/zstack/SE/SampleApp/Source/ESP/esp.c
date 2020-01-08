/**************************************************************************************************
  Filename:       esp.c
  Revised:        $Date: 2012-04-02 17:02:19 -0700 (Mon, 02 Apr 2012) $
  Revision:       $Revision: 29996 $

  Description:    This module implements the ESP functionality and contains the
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
  exploits the following clusters for an ESP configuration:

  General Basic
  General Alarms
  General Time
  General Key Establishment
  SE     Price
  SE     Demand Response and Load Control
  SE     Simple Metering
  SE     Message

  Key control:
    SW1:  Send out Cooling Load Control Event to PCT
    SW2:  Send out Load Control Event to Load Control Device
    SW3:  Send out Message to In Premise Display
    SW4:  Not used
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_Nv.h"
#include "MT.h"
#include "MT_APP.h"
#include "ZDObject.h"
#include "AddrMgr.h"

#include "se.h"
#include "esp.h"
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

#define ESP_MIN_REPORTING_INTERVAL       5

#if defined ( SE_UK_EXT ) && defined ( SE_MIRROR )
// There can be up to ESP_MAX_MIRRORS mirror endpoints starting at the ESP_MIRROR_EP_BASE and
// going up to ( ESP_MIRROR_EP_BASE + ESP_MAX_MIRRORS )
#define ESP_MIRROR_EP_BASE                        24

// The max number of mirrors the ESP can have.
// Note: This value is limited by the number of bits in the mirrorMask mask
#define ESP_MAX_MIRRORS                           16
#define ESP_MIRROR_FULL_MASK                      0xFFFF

#define ESP_MIRROR_NOTIFY_ATTR_COUNT              6
#define ESP_MIRROR_USER_ATTRIBUTES_POSITION       ESP_MIRROR_NOTIFY_ATTR_COUNT
#define ESP_MIRROR_MAX_USER_ATTRIBUTES            8
#define ESP_MIRROR_MAX_ATTRIBUTES                 (ESP_MIRROR_NOTIFY_ATTR_COUNT + ESP_MIRROR_MAX_USER_ATTRIBUTES)
#define ESP_MIRROR_INVALID_ENDPOINT               0xFF
#endif  // SE_UK_EXT && SE_MIRROR

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

static uint8 espTaskID;                              // esp osal task id
static afAddrType_t ipdAddr;                         // destination address of in premise display
static afAddrType_t pctAddr;                         // destination address of PCT
static afAddrType_t loadControlAddr;                 // destination address of load control device
static zAddrType_t simpleDescReqAddr;                // destination addresses for simple desc request
static zclCCLoadControlEvent_t loadControlCmd;       // command structure for load control command
static uint16 espFastPollModeDuration;               // number of fast poll events

#if defined ( INTER_PAN )
// define endpoint structure to register with STUB APS for INTER-PAN support
static endPointDesc_t espEp =
{
  ESP_ENDPOINT,
  &espTaskID,
  (SimpleDescriptionFormat_t *)&espSimpleDesc,
  (afNetworkLatencyReq_t)0
};
#endif

#if defined ( SE_UK_EXT ) && defined ( SE_MIRROR )
typedef struct
{
  uint16 srcAddr;
  uint8 srcEndpoint;
  zclAttrRec_t *pAttr;
  uint8 notificationControl;
  zclCCReqMirrorReportAttrRsp_t notificationSet;
} espMirrorInfo_t;

typedef struct
{
  uint16 mirrorMask;
  espMirrorInfo_t mirrorInfo[ESP_MAX_MIRRORS];
} espMirrorControl_t;

#define MIRROR_DEVICE_VERSION       0
#define MIRROR_FLAGS                0

#define MIRROR_MAX_INCLUSTERS       2
CONST cId_t mirrorInClusterList[MIRROR_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_SE_SIMPLE_METERING
};

#define MIRROR_MAX_OUTCLUSTERS       2
CONST cId_t mirrorOutClusterList[MIRROR_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_SE_SIMPLE_METERING
};
#endif  // SE_UK_EXT && SE_MIRROR

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void esp_HandleKeys( uint8 shift, uint8 keys );
static void esp_ProcessAppMsg( uint8 *msg );

static void esp_ProcessIdentifyTimeChange( void );

/*************************************************************************/
/*** Application Callback Functions                                    ***/
/*************************************************************************/

// Foundation Callback functions
static uint8 esp_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );

// General Cluster Callback functions
static void esp_BasicResetCB( void );
static void esp_IdentifyCB( zclIdentify_t *pCmd );
static void esp_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void esp_AlarmCB( zclAlarm_t *pAlarm );
#ifdef SE_UK_EXT
static void esp_GetEventLogCB( uint8 srcEP, afAddrType_t *srcAddr,
                               zclGetEventLog_t *pEventLog, uint8 seqNum );
static void esp_PublishEventLogCB( afAddrType_t *srcAddr, zclPublishEventLog_t *pEventLog );
#endif // SE_UK_EXT

// SE Callback functions
static void esp_GetProfileCmdCB( zclCCGetProfileCmd_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetProfileRspCB( zclCCGetProfileRsp_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ReqMirrorCmdCB( afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ReqMirrorRspCB( zclCCReqMirrorRsp_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_MirrorRemCmdCB( afAddrType_t *srcAddr, uint8 seqNum );
static void esp_MirrorRemRspCB( zclCCMirrorRemRsp_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ReqFastPollModeCmdCB( zclCCReqFastPollModeCmd_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ReqFastPollModeRspCB( zclCCReqFastPollModeRsp_t *pRsp,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetCurrentPriceCB( zclCCGetCurrentPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetScheduledPriceCB( zclCCGetScheduledPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PriceAcknowledgementCB( zclCCPriceAcknowledgement_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetBlockPeriodCB( zclCCGetBlockPeriod_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishPriceCB( zclCCPublishPrice_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishBlockPeriodCB( zclCCPublishBlockPeriod_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_DisplayMessageCB( zclCCDisplayMessage_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_CancelMessageCB( zclCCCancelMessage_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetLastMessageCB( afAddrType_t *srcAddr, uint8 seqNum );
static void esp_MessageConfirmationCB( zclCCMessageConfirmation_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_LoadControlEventCB( zclCCLoadControlEvent_t *pCmd,
                          afAddrType_t *srcAddr, uint8 status, uint8 seqNum);
static void esp_CancelLoadControlEventCB( zclCCCancelLoadControlEvent_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_CancelAllLoadControlEventsCB( zclCCCancelAllLoadControlEvents_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ReportEventStatusCB( zclCCReportEventStatus_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetScheduledEventCB( zclCCGetScheduledEvent_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_SelAvailEmergencyCreditCmdCB( zclCCSelAvailEmergencyCredit_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ChangeSupplyCmdCB( zclCCChangeSupply_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_SupplyStatusRspCB( zclCCSupplyStatusResponse_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
#if defined ( SE_UK_EXT )
static void esp_GetSnapshotRspCB( zclCCReqGetSnapshotRsp_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishTariffInformationCB( zclCCPublishTariffInformation_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishPriceMatrixCB( zclCCPublishPriceMatrix_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishBlockThresholdsCB( zclCCPublishBlockThresholds_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishConversionFactorCB( zclCCPublishConversionFactor_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishCalorificValueCB( zclCCPublishCalorificValue_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishCO2ValueCB( zclCCPublishCO2Value_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishCPPEventCB( zclCCPublishCPPEvent_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishBillingPeriodCB( zclCCPublishBillingPeriod_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishConsolidatedBillCB( zclCCPublishConsolidatedBill_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishCreditPaymentInfoCB( zclCCPublishCreditPaymentInfo_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetTariffInformationCB( zclCCGetTariffInformation_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetPriceMatrixCB( uint32 issuerTariffId,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetBlockThresholdsCB( uint32 issuerTariffId,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetConversionFactorCB( zclCCGetConversionFactor_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetCalorificValueCB( zclCCGetCalorificValue_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetCO2ValueCB( zclCCGetCO2Value_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetBillingPeriodCB( zclCCGetBillingPeriod_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetConsolidatedBillCB( zclCCGetConsolidatedBill_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_CPPEventResponseCB( zclCCCPPEventResponse_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ChangeDebtCB( zclCCChangeDebt_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_EmergencyCreditSetupCB( zclCCEmergencyCreditSetup_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ConsumerTopupCB( zclCCConsumerTopup_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_CreditAdjustmentCB( zclCCCreditAdjustment_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ChangePaymentModeCB( zclCCChangePaymentMode_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetPrepaySnapshotCB( zclCCGetPrepaySnapshot_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetTopupLogCB( uint8 numEvents,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_SetLowCreditWarningLevelCB( uint8 numEvents,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetDebtRepaymentLogCB( zclCCGetDebtRepaymentLog_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetPrepaySnapshotResponseCB( zclCCGetPrepaySnapshotResponse_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ChangePaymentModeResponseCB( zclCCChangePaymentModeResponse_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_ConsumerTopupResponseCB( zclCCConsumerTopupResponse_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_GetCommandsCB( uint8 prepayNotificationFlags,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishTopupLogCB( zclCCPublishTopupLog_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
static void esp_PublishDebtLogCB( zclCCPublishDebtLog_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum );
#endif  // SE_UK_EXT

/************************************************************************/
/***               Functions to process ZCL Foundation                ***/
/***               incoming Command/Response messages                 ***/
/************************************************************************/
static void esp_ProcessZCLMsg( zclIncomingMsg_t *msg );
#if defined ( ZCL_READ )
static uint8 esp_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_READ
#if defined ( ZCL_WRITE )
static uint8 esp_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_WRITE
#if defined ( ZCL_REPORT )
static uint8 esp_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg );
static uint8 esp_ProcessInConfigReportRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 esp_ProcessInReadReportCfgCmd( zclIncomingMsg_t *pInMsg );
static uint8 esp_ProcessInReadReportCfgRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 esp_ProcessInReportCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_REPORT
static uint8 esp_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#if defined ( ZCL_DISCOVER )
static uint8 esp_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif // ZCL_DISCOVER

// Functions to handle ZDO messages
static void esp_ProcessZDOMsg( zdoIncomingMsg_t *inMsg );

#if defined ( SE_UK_EXT ) && defined ( SE_MIRROR )
static void esp_MirrorInit( void );
static uint8 esp_GetMirrorEndpoint( afAddrType_t *srcAddr );
static uint8 esp_AllocMirror( afAddrType_t *srcAddr );
static void esp_FreeMirror( uint8 endPoint );
static uint8 esp_IsMirrorEndpoint( uint8 endpoint );
static void esp_MirrorProcessZCLMsg( zclIncomingMsg_t *pInMsg );
static espMirrorInfo_t *esp_GetMirrorInfo( uint8 endpoint );
static uint8 esp_MirrorUpdateAttribute( uint8 endpoint, uint16 cluster,
                                        zclReport_t *pReport );
static void esp_MirrorInitAttributeSet( uint8 endpoint );

static espMirrorControl_t esp_MirrorControl;
#endif  // SE_UK_EXT && SE_MIRROR

/*********************************************************************
 * ZCL General Clusters Callback table
 */
static zclGeneral_AppCallbacks_t esp_GenCmdCallbacks =
{
  esp_BasicResetCB,              // Basic Cluster Reset command
  esp_IdentifyCB,                // Identify command
  esp_IdentifyQueryRspCB,        // Identify Query Response command
  NULL,                          // On/Off cluster commands
  NULL,                          // Level Control Move to Level command
  NULL,                          // Level Control Move command
  NULL,                          // Level Control Step command
  NULL,                          // Level Control Stop command
  NULL,                          // Group Response commands
  NULL,                          // Scene Store Request command
  NULL,                          // Scene Recall Request command
  NULL,                          // Scene Response command
  esp_AlarmCB,                   // Alarm (Response) command
#ifdef SE_UK_EXT
  esp_GetEventLogCB,             // Get Event Log command
  esp_PublishEventLogCB,         // Publish Event Log command
#endif
  NULL,                          // RSSI Location command
  NULL                           // RSSI Location Response command
};

/*********************************************************************
 * ZCL SE Clusters Callback table
 */
static zclSE_AppCallbacks_t esp_SECmdCallbacks =
{
  esp_PublishPriceCB,                      // Publish Price
  esp_PublishBlockPeriodCB,                // Publish Block Period
#if defined ( SE_UK_EXT )
  esp_PublishTariffInformationCB,          // Publish Tariff Information
  esp_PublishPriceMatrixCB,                // Publish Price Matrix
  esp_PublishBlockThresholdsCB,            // Publish Block Thresholds
  esp_PublishConversionFactorCB,           // Publish Conversion Factor
  esp_PublishCalorificValueCB,             // Publish Calorific Value
  esp_PublishCO2ValueCB,                   // Publish CO2 Value
  esp_PublishCPPEventCB,                   // Publish CPP Event
  esp_PublishBillingPeriodCB,              // Publish Billing Period
  esp_PublishConsolidatedBillCB,           // Publish Consolidated Bill
  esp_PublishCreditPaymentInfoCB,          // Publish Credit Payment Info
#endif  // SE_UK_EXT
  esp_GetCurrentPriceCB,                   // Get Current Price
  esp_GetScheduledPriceCB,                 // Get Scheduled Price
  esp_PriceAcknowledgementCB,              // Price Acknowledgement
  esp_GetBlockPeriodCB,                    // Get Block Period
#if defined ( SE_UK_EXT )
  esp_GetTariffInformationCB,              // Get Tariff Information
  esp_GetPriceMatrixCB,                    // Get Price Matrix
  esp_GetBlockThresholdsCB,                // Get Block Thresholds
  esp_GetConversionFactorCB,               // Get Conversion Factor
  esp_GetCalorificValueCB,                 // Get Calorific Value
  esp_GetCO2ValueCB,                       // Get CO2 Value
  esp_GetBillingPeriodCB,                  // Get Billing Period
  esp_GetConsolidatedBillCB,               // Get Consolidated Bill
  esp_CPPEventResponseCB,                  // CPP Event Response
#endif  // SE_UK_EXT
  esp_LoadControlEventCB,                  // Load Control Event
  esp_CancelLoadControlEventCB,            // Cancel Load Control Event
  esp_CancelAllLoadControlEventsCB,        // Cancel All Load Control Events
  esp_ReportEventStatusCB,                 // Report Event Status
  esp_GetScheduledEventCB,                 // Get Scheduled Event
  esp_GetProfileRspCB,                     // Get Profile Response
  esp_ReqMirrorCmdCB,                      // Request Mirror Command
  esp_MirrorRemCmdCB,                      // Mirror Remove Command
  esp_ReqFastPollModeRspCB,                // Request Fast Poll Mode Response
#if defined ( SE_UK_EXT )
  esp_GetSnapshotRspCB,                    // Get Snapshot Response
#endif  // SE_UK_EXT
  esp_GetProfileCmdCB,                     // Get Profile Command
  esp_ReqMirrorRspCB,                      // Request Mirror Response
  esp_MirrorRemRspCB,                      // Mirror Remove Response
  esp_ReqFastPollModeCmdCB,                // Request Fast Poll Mode Command
#if defined ( SE_UK_EXT )
  NULL,                                    // Get Snapshot Command
  NULL,                                    // Take Snapshot Command
  NULL,                                    // Mirror Report Attribute Response
#endif  // SE_UK_EXT
  esp_DisplayMessageCB,                    // Display Message Command
  esp_CancelMessageCB,                     // Cancel Message Command
  esp_GetLastMessageCB,                    // Get Last Message Command
  esp_MessageConfirmationCB,               // Message Confirmation
  NULL,                                    // Request Tunnel Response
  NULL,                                    // Transfer Data
  NULL,                                    // Transfer Data Error
  NULL,                                    // Ack Transfer Data
  NULL,                                    // Ready Data
#if defined ( SE_UK_EXT )
  NULL,                                    // Supported Tunnel Protocols Response
  NULL,                                    // Tunnel Closure Notification
#endif  // SE_UK_EXT
  NULL,                                    // Request Tunnel
  NULL,                                    // Close Tunnel
#if defined ( SE_UK_EXT )
  NULL,                                    // Get Supported Tunnel Protocols
#endif  // SE_UK_EXT
  esp_SupplyStatusRspCB,                   // Supply Status Response
#if defined ( SE_UK_EXT )
  esp_GetPrepaySnapshotResponseCB,         // Get Prepay Snapshot Response
  esp_ChangePaymentModeResponseCB,         // Change Payment Mode Response
  esp_ConsumerTopupResponseCB,             // Consumer Topup Response
  esp_GetCommandsCB,                       // Get Commands
  esp_PublishTopupLogCB,                   // Publish Topup Log
  esp_PublishDebtLogCB,                    // Publish Debt Log
#endif  // SE_UK_EXT
  esp_SelAvailEmergencyCreditCmdCB,        // Select Available Emergency Credit Command
  esp_ChangeSupplyCmdCB,                   // Change Supply Command
#if defined ( SE_UK_EXT )
  esp_ChangeDebtCB,                        // Change Debt
  esp_EmergencyCreditSetupCB,              // Emergency Credit Setup
  esp_ConsumerTopupCB,                     // Consumer Topup
  esp_CreditAdjustmentCB,                  // Credit Adjustment
  esp_ChangePaymentModeCB,                 // Change PaymentMode
  esp_GetPrepaySnapshotCB,                 // Get Prepay Snapshot
  esp_GetTopupLogCB,                       // Get Topup Log
  esp_SetLowCreditWarningLevelCB,          // Set Low Credit Warning Level
  esp_GetDebtRepaymentLogCB,               // Get Debt Repayment Log
  NULL,                                    // Publish Calendar
  NULL,                                    // Publish Day Profile
  NULL,                                    // Publish Week Profile
  NULL,                                    // Publish Seasons
  NULL,                                    // Publish Special Days
  NULL,                                    // Get Calendar
  NULL,                                    // Get Day Profiles
  NULL,                                    // Get Week Profiles
  NULL,                                    // Get Seasons
  NULL,                                    // Get Special Days
  NULL,                                    // Publish Change Tenancy
  NULL,                                    // Publish Change Supplier
  NULL,                                    // Change Supply
  NULL,                                    // Change Password
  NULL,                                    // Local Change Supply
  NULL,                                    // Get Change Tenancy
  NULL,                                    // Get Change Supplier
  NULL,                                    // Get Change Supply
  NULL,                                    // Supply Status Response
  NULL,                                    // Get Password
#endif  // SE_UK_EXT
};

/*********************************************************************
 * @fn          esp_Init
 *
 * @brief       Initialization function for the ZCL App Application.
 *
 * @param       uint8 task_id - esp task id
 *
 * @return      none
 */
void esp_Init( uint8 task_id )
{
  espTaskID = task_id;

  // Register for an SE endpoint
  zclSE_Init( &espSimpleDesc );

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( ESP_ENDPOINT, &esp_GenCmdCallbacks );

  // Register the ZCL SE Cluster Library callback functions
  zclSE_RegisterCmdCallbacks( ESP_ENDPOINT, &esp_SECmdCallbacks );

  // Register the application's attribute list
  zcl_registerAttrList( ESP_ENDPOINT, ESP_MAX_ATTRIBUTES, espAttrs );

  // Register the application's cluster option list
  zcl_registerClusterOptionList( ESP_ENDPOINT, ESP_MAX_OPTIONS, espOptions );

  // Register the application's attribute data validation callback function
  zcl_registerValidateAttrData( esp_ValidateAttrDataCB );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( espTaskID );

  // register for end device annce and simple descriptor responses
  ZDO_RegisterForZDOMsg( espTaskID, Device_annce );
  ZDO_RegisterForZDOMsg( espTaskID, Simple_Desc_rsp );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( espTaskID );

#if defined ( INTER_PAN )
  // Register with Stub APS
  StubAPS_RegisterApp( &espEp );
#endif

  // Start the timer to sync esp timer with the osal timer
  osal_start_timerEx( espTaskID, ESP_UPDATE_TIME_EVT, ESP_UPDATE_TIME_PERIOD );

  // setup address mode and destination endpoint fields for PCT
  pctAddr.addrMode = (afAddrMode_t)Addr16Bit;
  pctAddr.endPoint = ESP_ENDPOINT;

  // setup address mode and destination endpoint fields for load control device
  loadControlAddr.addrMode = (afAddrMode_t)Addr16Bit;
  loadControlAddr.endPoint = ESP_ENDPOINT;

  //setup load control command structure
  loadControlCmd.issuerEvent = 0x12345678;            // arbitrary id
  loadControlCmd.deviceGroupClass = 0x000000;         // addresses all groups
  loadControlCmd.startTime = 0x00000000;              // start time = NOW
  loadControlCmd.durationInMinutes = 0x0001;          // duration of one minute
  loadControlCmd.criticalityLevel = 0x01;             // green level
  loadControlCmd.coolingTemperatureSetPoint = 0x076C; // 19 degrees C, 66.2 degress fahrenheit
  loadControlCmd.eventControl = 0x00;                 // no randomized start or end applied

  // Initialize variable used to control number of fast poll events
  espFastPollModeDuration = 0;

  // detect and remove stored deprecated end device children after power up
  uint8 cleanupChildTable = TRUE;
  zgSetItem( ZCD_NV_ROUTER_OFF_ASSOC_CLEANUP, sizeof(cleanupChildTable), &cleanupChildTable );

#if defined ( SE_UK_EXT ) && defined ( SE_MIRROR )
  esp_MirrorInit();
#endif  // SE_UK_EXT && SE_MIRROR

}

/*********************************************************************
 * @fn          esp_event_loop
 *
 * @brief       Event Loop Processor for esp.
 *
 * @param       uint8 task_id - esp task id
 * @param       uint16 events - event bitmask
 *
 * @return      none
 */
uint16 esp_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( espTaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case MT_SYS_APP_MSG:
          // Message received from MT (serial port)
          esp_ProcessAppMsg( ((mtSysAppMsg_t *)MSGpkt)->appData );
          break;

        case ZCL_INCOMING_MSG:
          // Incoming ZCL foundation command/response messages
          esp_ProcessZCLMsg( (zclIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          esp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case ZDO_CB_MSG:
          // ZDO sends the message that we registered for
          esp_ProcessZDOMsg( (zdoIncomingMsg_t *)MSGpkt );
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

  // handle processing of identify timeout event triggered by an identify command
  if ( events & ESP_IDENTIFY_TIMEOUT_EVT )
  {
    if ( espIdentifyTime > 0 )
    {
      espIdentifyTime--;
    }
    esp_ProcessIdentifyTimeChange();

    return ( events ^ ESP_IDENTIFY_TIMEOUT_EVT );
  }

  // event to get current time
  if ( events & ESP_UPDATE_TIME_EVT )
  {
    espTime = osal_getClock();
    osal_start_timerEx( espTaskID, ESP_UPDATE_TIME_EVT, ESP_UPDATE_TIME_PERIOD );

    return ( events ^ ESP_UPDATE_TIME_EVT );
  }


  // event to get simple descriptor of the newly joined device
  if ( events & SIMPLE_DESC_QUERY_EVT )
  {
      ZDP_SimpleDescReq( &simpleDescReqAddr, simpleDescReqAddr.addr.shortAddr,
                        ESP_ENDPOINT, 0);

      return ( events ^ SIMPLE_DESC_QUERY_EVT );
  }

  // handle processing of timeout event triggered by request fast polling command
  if ( events & ESP_FAST_POLL_MODE_EVT )
  {
    if (espFastPollModeDuration)
    {
      espFastPollModeDuration--;
      // Start the timer for the fast poll period
      osal_start_timerEx( espTaskID, ESP_FAST_POLL_MODE_EVT, ESP_FAST_POLL_TIMER_PERIOD );
    }

    return ( events ^ ESP_FAST_POLL_MODE_EVT );
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      esp_ProcessAppMsg
 *
 * @brief   Process MT SYS APP MSG
 *
 * @param   msg - pointer to message
 *
 * @return  none
 */
static void esp_ProcessAppMsg( uint8 *msg )
{
  // user should include code to handle MT SYS APP MSG here
}

/*********************************************************************
 * @fn      esp_ProcessIdentifyTimeChange
 *
 * @brief   Called to blink led for specified IdentifyTime attribute value
 *
 * @param   none
 *
 * @return  none
 */
static void esp_ProcessIdentifyTimeChange( void )
{
  if ( espIdentifyTime > 0 )
  {
    osal_start_timerEx( espTaskID, ESP_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( espTaskID, ESP_IDENTIFY_TIMEOUT_EVT );
  }
}


/*********************************************************************
 * @fn      esp_HandleKeys
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
static void esp_HandleKeys( uint8 shift, uint8 keys )
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
      // send out cooling event to PCT
      loadControlCmd.deviceGroupClass = HVAC_DEVICE_CLASS; // HVAC compressor or furnace - bit 0 is set
      zclSE_LoadControl_Send_LoadControlEvent( ESP_ENDPOINT, &pctAddr, &loadControlCmd, TRUE, 0 );
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      // send out load control event to load control device
      loadControlCmd.deviceGroupClass = ONOFF_LOAD_DEVICE_CLASS; // simple misc residential on/off loads - bit 7 is set
      zclSE_LoadControl_Send_LoadControlEvent( ESP_ENDPOINT, &loadControlAddr, &loadControlCmd, TRUE, 0 );
    }

    if ( keys & HAL_KEY_SW_3 )
    {
      zclCCDisplayMessage_t displayCmd;             // command structure for message being sent to in premise display

      // Define to zero to send the TI IPD message, non-zero to send a string of abc's.
#if   !defined IPD_MSG_SZ
      #define  IPD_MSG_SZ  0
#endif
#if   (IPD_MSG_SZ == 0)
      uint8 msgBuf[]="TI IPD Test Msg!";
      const uint8 msgLen = sizeof(msgBuf);
#else
      uint8 *msgBuf = osal_mem_alloc(IPD_MSG_SZ);
      const uint8 msgLen = IPD_MSG_SZ;
      uint8 idx;

      if (!msgBuf)  return;

      for (idx = 0; idx < msgLen; idx ++)
      {
        msgBuf[idx] = 'a' + idx % 26;
      }
#endif

      displayCmd.msgString.strLen = msgLen;
      displayCmd.msgString.pStr = msgBuf;

      zclSE_Message_Send_DisplayMessage( ESP_ENDPOINT, &ipdAddr, &displayCmd, TRUE, 0 );

#if   (IPD_MSG_SZ != 0)
      osal_mem_free(msgBuf);
#endif
    }

    if ( keys & HAL_KEY_SW_4 )
    {

    }
  }
}

/*********************************************************************
 * @fn      esp_ValidateAttrDataCB
 *
 * @brief   Check to see if the supplied value for the attribute data
 *          is within the specified range of the attribute.
 *
 *
 * @param   pAttr - pointer to attribute
 * @param   pAttrInfo - pointer to attribute info
 *
 * @return  TRUE if data valid. FALSE otherwise.
 */
static uint8 esp_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
{
  uint8 valid = TRUE;

  switch ( pAttrInfo->dataType )
  {
    case ZCL_DATATYPE_BOOLEAN:
      if ( ( *(pAttrInfo->attrData) != 0 ) && ( *(pAttrInfo->attrData) != 1 ) )
      {
        valid = FALSE;
      }
      break;

    default:
      break;
  }

  return ( valid );
}

/*********************************************************************
 * @fn      esp_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library to set all
 *          the attributes of all the clusters to their factory defaults
 *
 * @param   none
 *
 * @return  none
 */
static void esp_BasicResetCB( void )
{
  // user should handle setting attributes to factory defaults here
}

/*********************************************************************
 * @fn      esp_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identify Command for this application.
 *
 * @param   pCmd - pointer to structure for Identify command
 *
 * @return  none
 */
static void esp_IdentifyCB( zclIdentify_t *pCmd )
{
  espIdentifyTime = pCmd->identifyTime;
  esp_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      esp_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   pRsp - pointer to structure for Identity Query Response command
 *
 * @return  none
 */
static void esp_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_AlarmCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Alam request or response command for
 *          this application.
 *
 * @param   pAlarm - pointer to structure for Alarm command
 *
 * @return  none
 */
static void esp_AlarmCB( zclAlarm_t *pAlarm )
{
  // add user code here
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      esp_GetEventLogCB
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
static void esp_GetEventLogCB( uint8 srcEP, afAddrType_t *srcAddr,
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
 * @fn      esp_PublishEventLogCB
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
static void esp_PublishEventLogCB( afAddrType_t *srcAddr, zclPublishEventLog_t *pEventLog )
{
  // add user code here
}
#endif // SE_UK_EXT

/*********************************************************************
 * @fn      esp_GetProfileCmdCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Get Profile Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Get Profile command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetProfileCmdCB( zclCCGetProfileCmd_t *pCmd,
                                 afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_SIMPLE_METERING )
  // Upon receipt of the Get Profile Command, the metering device shall send
  // Get Profile Response back.

  // Variables in the following are initialized to arbitrary value for test purpose
  // In real application, user shall look up the interval data captured during
  // the period specified in the pCmd->endTime and return corresponding data.

  uint32 endTime;
  uint8  status = zclSE_SimpleMeter_GetProfileRsp_Status_Success;
  uint8  profileIntervalPeriod = PROFILE_INTERVAL_PERIOD_60MIN;
  uint8  numberOfPeriodDelivered = 5;
  uint24 intervals[] = {0xa00001, 0xa00002, 0xa00003, 0xa00004, 0xa00005};

  // endTime: 32 bit value (in UTC) representing the end time of the most
  // chronologically recent interval being requested.
  // Example: Data collected from 2:00 PM to 3:00 PM would be specified as a
  // 3:00 PM interval (end time).

  // The Intervals block returned shall be the most recent block with
  // its EndTime equal or older to the one in the request (pCmd->endTime).
  // Requested End Time with value 0xFFFFFFFF indicats the most recent
  // Intervals block is requested.

  // Sample Code - assuming the end time of the requested block is the same as
  // it in the request.
  endTime = pCmd->endTime;

  // Send Get Profile Response Command back

  zclSE_SimpleMetering_Send_GetProfileRsp( ESP_ENDPOINT, srcAddr, endTime,
                                           status,
                                           profileIntervalPeriod,
                                           numberOfPeriodDelivered, intervals,
                                           FALSE, seqNum );
#endif // ZCL_SIMPLE_METERING
}

/*********************************************************************
 * @fn      esp_GetProfileRspCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Get Profile Response for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Get Profile Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetProfileRspCB( zclCCGetProfileRsp_t *pCmd,
                                 afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_ReqMirrorCmdCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Request Mirror Command for
 *          this application.
 *
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_ReqMirrorCmdCB( afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_SIMPLE_METERING )
#if defined ( SE_UK_EXT ) && defined ( SE_MIRROR )
  uint8 endpoint;

  // See if a mirror exists for the given source address and endpoint
  endpoint = esp_GetMirrorEndpoint( srcAddr );

  if ( endpoint == ESP_MIRROR_INVALID_ENDPOINT )
  {
    // No endpoint exists.  Try allocating one.
    endpoint = esp_AllocMirror(srcAddr);

    if (endpoint != ESP_MIRROR_INVALID_ENDPOINT)
    {
      // Setup endpoint
      espMirrorInfo_t *pInfo = esp_GetMirrorInfo(endpoint);

      if ( pInfo != NULL )
      {
        // Create a simple descriptor for the mirror
        SimpleDescriptionFormat_t *pMirrorSimpleDesc = osal_mem_alloc( sizeof( SimpleDescriptionFormat_t ) );

        if ( pMirrorSimpleDesc != NULL )
        {
          zclAttrRec_t *pAttr = pInfo->pAttr;

          pMirrorSimpleDesc->EndPoint = endpoint;
          pMirrorSimpleDesc->AppProfId = ZCL_SE_PROFILE_ID;
          pMirrorSimpleDesc->AppDeviceId = ZCL_SE_DEVICEID_METER;
          pMirrorSimpleDesc->AppDevVer = MIRROR_DEVICE_VERSION;
          pMirrorSimpleDesc->Reserved = MIRROR_FLAGS;
          pMirrorSimpleDesc->AppNumInClusters = MIRROR_MAX_INCLUSTERS;
          pMirrorSimpleDesc->pAppInClusterList = (cId_t *) mirrorInClusterList;
          pMirrorSimpleDesc->AppNumOutClusters = MIRROR_MAX_OUTCLUSTERS;
          pMirrorSimpleDesc->pAppOutClusterList = (cId_t *) mirrorOutClusterList;

          zclSE_Init( pMirrorSimpleDesc );

          // Register the attribute list
          zcl_registerAttrList( endpoint, ESP_MIRROR_MAX_ATTRIBUTES, (CONST zclAttrRec_t *) pAttr );
        }
        else
        {
          esp_FreeMirror( endpoint );
          endpoint = ESP_MIRROR_INVALID_ENDPOINT;
        }
      }
    }
  }

  if ( endpoint != ESP_MIRROR_INVALID_ENDPOINT )
  {
    // Send response to the peer with the mirror endpoint ID
    zclSE_SimpleMetering_Send_ReqMirrorRsp( ESP_ENDPOINT, srcAddr, endpoint, TRUE, seqNum );
  }
  else
  {
    // Send response indicating we cannot create the mirror
    zclSE_SimpleMetering_Send_ReqMirrorRsp(ESP_ENDPOINT, srcAddr, 0xFFFF, TRUE, seqNum);
  }
#endif  // SE_UK_EXT && SE_MIRROR
#endif  // ZCL_SIMPLE_METERING
}
/*********************************************************************
 * @fn      esp_ReqMirrorRspCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Request Mirror Response for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Request Mirror Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_ReqMirrorRspCB( zclCCReqMirrorRsp_t *pCmd,
                                afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_MirrorRemCmdCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Mirror Remove Command for
 *          this application.
 *
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_MirrorRemCmdCB( afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_SIMPLE_METERING )
#if defined ( SE_UK_EXT ) && defined ( SE_MIRROR )
  // Check if this is a valid endpoint for a mirror
  uint8 endpoint = esp_GetMirrorEndpoint( srcAddr );

  if ( endpoint != ESP_MIRROR_INVALID_ENDPOINT )
  {
    // Set the PhysicalEnvironment attribute indicating a mirror slot is available
    espPhysicalEnvironment |= PHY_MIRROR_CAPACITY_ENV;

    endPointDesc_t *epDesc = afFindEndPointDesc( endpoint );

    // Free memory allocated for SimpleDescriptor in esp_ReqMirrorCmdCB()
    if ( epDesc->simpleDesc != NULL )
    {
      osal_mem_free( epDesc->simpleDesc );
    }

    // Delete endpoint completely
    afDelete( endpoint );

    // Free the mirror in the mirror control block
    esp_FreeMirror( endpoint );

    // Send response to peer
    zclSE_SimpleMetering_Send_RemMirrorRsp( ESP_ENDPOINT, srcAddr, endpoint, TRUE, seqNum );
  }
  else
  {
    // The specification does not state how to deal with the case where no mirror exists,
    // For now, send 0xFFFF in the endpoint.
    zclSE_SimpleMetering_Send_RemMirrorRsp( ESP_ENDPOINT, srcAddr, 0xFFFF, TRUE, seqNum );
  }
#endif  // SE_UK_EXT && SE_MIRROR
#endif  // ZCL_SIMPLE_METERING
}

/*********************************************************************
 * @fn      esp_MirrorRemRspCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Mirror Remove Response for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Mirror Remove Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_MirrorRemRspCB( zclCCMirrorRemRsp_t *pCmd,
                                afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_ReqFastPollModeCmdCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Request Fast Poll Mode Command for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Request Fast Poll Mode command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_ReqFastPollModeCmdCB( zclCCReqFastPollModeCmd_t *pCmd, afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_SIMPLE_METERING )
  if ( pCmd != NULL )
  {
    zclCCReqFastPollModeRsp_t fastPollRsp;
    UTCTime utcSecs;

    if (pCmd->fastPollUpdatePeriod < espFastPollUpdatePeriod)
    {
      // handles client requests for a fast poll rate that is less than the
      // value of the its FastPollUpdateRate attribute
      fastPollRsp.appliedUpdatePeriod = espFastPollUpdatePeriod;
    }
    else
    {
      fastPollRsp.appliedUpdatePeriod = pCmd->fastPollUpdatePeriod;
    }

    if ((espFastPollModeDuration == 0) && (pCmd->duration > 0))
    {
      if (pCmd->duration > MAX_DURATION_IN_MINUTES_FAST_POLL_MODE)
      {
        // handles client requests for duration that is greater than the
        // maximum allowable 15 minutes.
        espFastPollModeDuration = MAX_DURATION_IN_MINUTES_FAST_POLL_MODE;
      }
      else
      {
        espFastPollModeDuration = pCmd->duration;
      }

      // This controls the counter for ZCLTESTAPP_FAST_POLL_MODE_EVT based on a 1 second timer
      espFastPollModeDuration *= 60;  // Duration in seconds

      // Start the timer for the fast poll period
      osal_start_timerEx( espTaskID, ESP_FAST_POLL_MODE_EVT, ESP_FAST_POLL_TIMER_PERIOD );
    }

    // get UTC time and update with requested duration in seconds
    utcSecs = osal_getClock();
    fastPollRsp.fastPollModeEndTime = utcSecs + espFastPollModeDuration;

    zclSE_SimpleMetering_Send_ReqFastPollModeRsp( ESP_ENDPOINT, srcAddr,
                                                  &fastPollRsp,
                                                  TRUE, seqNum );

#if defined ( LCD_SUPPORTED )
    HalLcdWriteString("Fast Polling", HAL_LCD_LINE_1);
    HalLcdWriteStringValue("Cur 0x", utcSecs, 16, HAL_LCD_LINE_2 );
    HalLcdWriteStringValue("End 0x", fastPollRsp.fastPollModeEndTime, 16, HAL_LCD_LINE_3 );
#endif
  }
#endif // ZCL_SIMPLE_METERING
}

/*********************************************************************
 * @fn      esp_ReqFastPollModeRspCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Request Fast Poll Mode Response for
 *          this application.
 *
 * @param   pRsp - pointer to structure for Request Fast Poll Mode Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_ReqFastPollModeRspCB( zclCCReqFastPollModeRsp_t *pRsp,
                                      afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetCurrentPriceCB
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
static void esp_GetCurrentPriceCB( zclCCGetCurrentPrice_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_PRICING )
  // On receipt of Get Current Price command, the device shall send a
  // Publish Price command with the information for the current time.
  zclCCPublishPrice_t cmd;
  uint8 rateLabelLen = 4; // adjust this value if different label is set, test label "BASE"

  osal_memset( &cmd, 0, sizeof( zclCCPublishPrice_t ) );

  // Set Pricing information
  cmd.providerId = 0xbabeface;
  cmd.rateLabel.pStr = (uint8 *)osal_mem_alloc(rateLabelLen);
  if (cmd.rateLabel.pStr != NULL)
  {
    cmd.rateLabel.strLen = rateLabelLen;
    osal_memcpy(cmd.rateLabel.pStr, "BASE", rateLabelLen);
  }
  cmd.issuerEventId = 0x00000000;
  cmd.currentTime = osal_getClock();
  cmd.unitOfMeasure = 0x00;
  cmd.currency = 0x0348;
  cmd.priceTrailingDigit = 0x11;
  cmd.numberOfPriceTiers = 0x21;
  cmd.startTime = 0x00000000;
  cmd.durationInMinutes = 0x003C;
  cmd.price = 0x00000018;
  cmd.priceRatio = SE_OPTIONAL_FIELD_UINT8;
  cmd.generationPrice = SE_OPTIONAL_FIELD_UINT32;
  cmd.generationPriceRatio = SE_OPTIONAL_FIELD_UINT8;
  cmd.alternateCostDelivered = SE_OPTIONAL_FIELD_UINT32;
  cmd.alternateCostUnit = SE_OPTIONAL_FIELD_UINT8;
  cmd.alternateCostTrailingDigit = SE_OPTIONAL_FIELD_UINT8;
  cmd.numberOfBlockThresholds = SE_OPTIONAL_FIELD_UINT8;
  cmd.priceControl = SE_PROFILE_PRICEACK_REQUIRED_MASK;

  // copy source address of display device that requested current pricing info so
  // that esp can send messages to it using destination address of IPDAddr
  osal_memcpy( &ipdAddr, srcAddr, sizeof ( afAddrType_t ) );

  zclSE_Pricing_Send_PublishPrice( ESP_ENDPOINT, srcAddr, &cmd, FALSE, seqNum );

  if (cmd.rateLabel.pStr != NULL)
  {
    osal_mem_free(cmd.rateLabel.pStr);
  }
#endif // ZCL_PRICING
}

/*********************************************************************
 * @fn      esp_GetScheduledPriceCB
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
static void esp_GetScheduledPriceCB( zclCCGetScheduledPrice_t *pCmd,
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

  zclSE_Pricing_Send_PublishPrice( ESP_ENDPOINT, srcAddr, &cmd, FALSE, seqNum );

#endif // ZCL_PRICING
}

/*********************************************************************
 * @fn      esp_PriceAcknowledgementCB
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
static void esp_PriceAcknowledgementCB( zclCCPriceAcknowledgement_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetBlockPeriodCB
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
static void esp_GetBlockPeriodCB( zclCCGetBlockPeriod_t *pCmd,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishPriceCB
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
static void esp_PublishPriceCB( zclCCPublishPrice_t *pCmd,
                                      afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishBlockPeriodCB
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
static void esp_PublishBlockPeriodCB( zclCCPublishBlockPeriod_t *pCmd,
                                      afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_DisplayMessageCB
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
static void esp_DisplayMessageCB( zclCCDisplayMessage_t *pCmd,
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
    HalLcdWriteString( (char*)pCmd->msgString.pStr, HAL_LCD_LINE_1 );
#endif // LCD_SUPPORTED
}

/*********************************************************************
 * @fn      esp_CancelMessageCB
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
static void esp_CancelMessageCB( zclCCCancelMessage_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetLastMessageCB
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
static void esp_GetLastMessageCB( afAddrType_t *srcAddr, uint8 seqNum )
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

  zclSE_Message_Send_DisplayMessage( ESP_ENDPOINT, srcAddr, &cmd,
                                     FALSE, seqNum );
#endif // ZCL_MESSAGe
}

/*********************************************************************
 * @fn      esp_MessageConfirmationCB
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
static void esp_MessageConfirmationCB( zclCCMessageConfirmation_t *pCmd,
                                             afAddrType_t *srcAddr, uint8 seqNum)
{
  // add user code here
}

#if defined (ZCL_LOAD_CONTROL)
/*********************************************************************
 * @fn      esp_SendReportEventStatus
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
static void esp_SendReportEventStatus( afAddrType_t *srcAddr, uint8 seqNum,
                                              uint32 eventID, uint32 startTime,
                                              uint8 eventStatus, uint8 criticalityLevel,
                                              uint8 eventControl )
{
  zclCCReportEventStatus_t *pRsp;

  pRsp = (zclCCReportEventStatus_t *)osal_mem_alloc( sizeof( zclCCReportEventStatus_t ) );

  if ( pRsp != NULL)
  {
    // Mandatory fields - use the incoming data
    pRsp->issuerEventID = eventID;
    pRsp->eventStartTime = startTime;
    pRsp->criticalityLevelApplied = criticalityLevel;
    pRsp->eventControl = eventControl;
    pRsp->eventStatus = eventStatus;
    pRsp->signatureType = SE_PROFILE_SIGNATURE_TYPE_ECDSA;

    // esp_Signature is a static array.
    // value can be changed in esp_data.c
    osal_memcpy( pRsp->signature, espSignature, 16 );

    // Optional fields - fill in with non-used value by default
    pRsp->coolingTemperatureSetPointApplied = SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT;
    pRsp->heatingTemperatureSetPointApplied = SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT;
    pRsp->averageLoadAdjustment = SE_OPTIONAL_FIELD_INT8;
    pRsp->dutyCycleApplied = SE_OPTIONAL_FIELD_UINT8;

    // Send response back
    // DisableDefaultResponse is set to FALSE - it is recommended to turn on
    // default response since Report Event Status Command does not have
    // a response.
    zclSE_LoadControl_Send_ReportEventStatus( ESP_ENDPOINT, srcAddr,
                                            pRsp, FALSE, seqNum );
    osal_mem_free( pRsp );
  }
}
#endif // ZCL_LOAD_CONTROL

/*********************************************************************
 * @fn      esp_LoadControlEventCB
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
static void esp_LoadControlEventCB( zclCCLoadControlEvent_t *pCmd,
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
  esp_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEvent,
                                   pCmd->startTime, eventStatus,
                                   pCmd->criticalityLevel, pCmd->eventControl);

  if ( status != ZCL_STATUS_INVALID_FIELD )
  {
    // add user load control event handler here
  }
#endif // ZCL_LOAD_CONTROL
}

/*********************************************************************
 * @fn      esp_CancelLoadControlEventCB
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
static void esp_CancelLoadControlEventCB( zclCCCancelLoadControlEvent_t *pCmd,
                                                afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_LOAD_CONTROL )
  if ( 0 )  // User shall replace the if condition with "if the event exist"
  {
    // If the event exist, stop the event, and respond with status: cancelled

    // Cancel the event here

    // Use the following sample code to send response back.
    /*
    esp_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEventID,
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
    esp_SendReportEventStatus( srcAddr, seqNum, pCmd->issuerEventID,
                                     SE_OPTIONAL_FIELD_UINT32,                  // startTime
                                     EVENT_STATUS_LOAD_CONTROL_EVENT_RECEIVED,  // eventStatus
                                     SE_OPTIONAL_FIELD_UINT8,                   // Criticality level
                                     SE_OPTIONAL_FIELD_UINT8 );                 // eventControl
  }

#endif // ZCL_LOAD_CONTROL
}

/*********************************************************************
 * @fn      esp_CancelAllLoadControlEventsCB
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
static void esp_CancelAllLoadControlEventsCB( zclCCCancelAllLoadControlEvents_t *pCmd,
                                                    afAddrType_t *srcAddr, uint8 seqNum )
{
  // Upon receipt of Cancel All Load Control Event Command,
  // the receiving device shall look up the table for all events
  // and send a seperate response for each event

}

/*********************************************************************
 * @fn      esp_ReportEventStatusCB
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
static void esp_ReportEventStatusCB( zclCCReportEventStatus_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum)
{
  // add user code here
}
/*********************************************************************
 * @fn      esp_GetScheduledEventCB
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
static void esp_GetScheduledEventCB( zclCCGetScheduledEvent_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_SelAvailEmergencyCreditCmdCB
 *
 * @brief   Callback from the ZCL SE Prepayment Cluster Library when it recieved
 *          Select Available Emergency Credit command in the application
 *
 * @param   pCmd - pointer to structure for Select Available Emergency Credit command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - Sequence no of the message
 *
 * @return  none
 */
static void esp_SelAvailEmergencyCreditCmdCB( zclCCSelAvailEmergencyCredit_t *pCmd,
                                              afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_PREPAYMENT )
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString("Emergency Credit", HAL_LCD_LINE_1);

  if ((pCmd->siteId.strLen > 0) &&
      (pCmd->siteId.strLen <= HAL_LCD_MAX_CHARS) &&
      (pCmd->siteId.pStr != NULL))
  {
    HalLcdWriteString((char*)pCmd->siteId.pStr, HAL_LCD_LINE_2);
  }

  if ((pCmd->meterSerialNumber.strLen > 0) &&
      (pCmd->meterSerialNumber.strLen <= HAL_LCD_MAX_CHARS) &&
      (pCmd->meterSerialNumber.pStr != NULL))
  {
    HalLcdWriteString((char*)pCmd->meterSerialNumber.pStr, HAL_LCD_LINE_3);
  }
#endif
#endif  // ZCL_PREPAYMENT
}

/*********************************************************************
 * @fn      esp_ChangeSupplyCmdCB
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
static void esp_ChangeSupplyCmdCB( zclCCChangeSupply_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum )
{
#if defined ( ZCL_PREPAYMENT )
#if !defined ( SE_UK_EXT )
  zclCCSupplyStatusResponse_t SupplyStatus_cmd;

  osal_memset( &SupplyStatus_cmd, 0, sizeof( zclCCSupplyStatusResponse_t ) );

  SupplyStatus_cmd.providerId = pCmd->providerId;
  SupplyStatus_cmd.implementationDateTime = osal_getClock();
  SupplyStatus_cmd.supplyStatus = pCmd->proposedSupplyStatus;

  zclSE_Prepayment_Send_SupplyStatusResponse( ESP_ENDPOINT, srcAddr, &SupplyStatus_cmd,
                                              FALSE, seqNum );
#endif  // SE_UK_EXT
#endif  // ZCL_PREPAYMENT
}

/*********************************************************************
 * @fn      esp_SupplyStatusRspCB
 *
 * @brief    Callback from the ZCL SE Prepayment Cluster Library when it recieved
 *           Supply Status Response command in the application
 *
 * @param   pCmd - pointer to structure for Supply Status Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - Sequence no of the message
 *
 * @return  none
 */
static void esp_SupplyStatusRspCB( zclCCSupplyStatusResponse_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

#if defined ( SE_UK_EXT )
/*********************************************************************
 * @fn      esp_GetSnapshotRspCB
 *
 * @brief   Callback from the ZCL SE Profile Simple Metering Cluster Library when
 *          it received a Get Snapshot Response for
 *          this application.
 *
 * @param   pCmd - pointer to structure for Get Snapshot Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetSnapshotRspCB( zclCCReqGetSnapshotRsp_t *pCmd,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishTariffInformationCB
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
static void esp_PublishTariffInformationCB( zclCCPublishTariffInformation_t *pCmd,
                                            afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishPriceMatrixCB
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
static void esp_PublishPriceMatrixCB( zclCCPublishPriceMatrix_t *pCmd,
                                      afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishBlockThresholdsCB
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
static void esp_PublishBlockThresholdsCB( zclCCPublishBlockThresholds_t *pCmd,
                                          afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishConversionFactorCB
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
static void esp_PublishConversionFactorCB( zclCCPublishConversionFactor_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishCalorificValueCB
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
static void esp_PublishCalorificValueCB( zclCCPublishCalorificValue_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishCO2ValueCB
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
static void esp_PublishCO2ValueCB( zclCCPublishCO2Value_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishCPPEventCB
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
static void esp_PublishCPPEventCB( zclCCPublishCPPEvent_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishBillingPeriodCB
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
static void esp_PublishBillingPeriodCB( zclCCPublishBillingPeriod_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishConsolidatedBillCB
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
static void esp_PublishConsolidatedBillCB( zclCCPublishConsolidatedBill_t *pCmd,
                                           afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishCreditPaymentInfoCB
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
static void esp_PublishCreditPaymentInfoCB( zclCCPublishCreditPaymentInfo_t *pCmd,
                                            afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetTariffInformationCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Get Tariff Information for this application.
 *
 * @param   pCmd - pointer to structure for Get Tariff Information command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetTariffInformationCB( zclCCGetTariffInformation_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetPriceMatrixCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Get Price Matrix for this application.
 *
 * @param   issuerTariffId - issuer tariff Id
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetPriceMatrixCB( uint32 issuerTariffId,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetBlockThresholdsCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Get Block Thresholds for this application.
 *
 * @param   issuerTariffId - issuer tariff Id
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetBlockThresholdsCB( uint32 issuerTariffId,
                                      afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetConversionFactorCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Get Conversion Factor for this application.
 *
 * @param   pCmd - pointer to structure for Get Conversion Factor command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetConversionFactorCB( zclCCGetConversionFactor_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetCalorificValueCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Get Calorific Value for this application.
 *
 * @param   pCmd - pointer to structure for Get Calorific Value command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetCalorificValueCB( zclCCGetCalorificValue_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetCO2ValueCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Get CO2 Value for this application.
 *
 * @param   pCmd - pointer to structure for Get CO2 Value command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetCO2ValueCB( zclCCGetCO2Value_t *pCmd,
                               afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetBillingPeriodCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Get Billing Period for this application.
 *
 * @param   pCmd - pointer to structure for Get Billing Period command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetBillingPeriodCB( zclCCGetBillingPeriod_t *pCmd,
                                    afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetConsolidatedBillCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a Get Consolidated Bill for this application.
 *
 * @param   pCmd - pointer to structure for Get Consolidated Bill command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetConsolidatedBillCB( zclCCGetConsolidatedBill_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_CPPEventResponseCB
 *
 * @brief   Callback from the ZCL SE Profile Price Cluster Library when
 *          it received a CPP Event Response for this application.
 *
 * @param   pCmd - pointer to structure for CPP Event Response command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_CPPEventResponseCB( zclCCCPPEventResponse_t *pCmd,
                                    afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_ChangeDebtCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Change Debt for this application.
 *
 * @param   pCmd - pointer to structure for Change Debt command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_ChangeDebtCB( zclCCChangeDebt_t *pCmd,
                              afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_EmergencyCreditSetupCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Emergency Credit Setup for this application.
 *
 * @param   pCmd - pointer to structure for Emergency Credit Setup command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_EmergencyCreditSetupCB( zclCCEmergencyCreditSetup_t *pCmd,
                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_ConsumerTopupCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Consumer Topup for this application.
 *
 * @param   pCmd - pointer to structure for Consumer Topup command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_ConsumerTopupCB( zclCCConsumerTopup_t *pCmd,
                                 afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_CreditAdjustmentCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Credit Adjustment for this application.
 *
 * @param   pCmd - pointer to structure for Credit Adjustment command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_CreditAdjustmentCB( zclCCCreditAdjustment_t *pCmd,
                                    afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_ChangePaymentModeCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Change Payment Mode for this application.
 *
 * @param   pCmd - pointer to structure for Change Payment Mode command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_ChangePaymentModeCB( zclCCChangePaymentMode_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetPrepaySnapshotCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Get Prepay Snapshot for this application.
 *
 * @param   pCmd - pointer to structure for Get Prepay Snapshot command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetPrepaySnapshotCB( zclCCGetPrepaySnapshot_t *pCmd,
                                     afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetTopupLogCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Get Topup Log for this application.
 *
 * @param   numEvents - number of events
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetTopupLogCB( uint8 numEvents,
                               afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_SetLowCreditWarningLevelCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Set Low Credit Warning Level for this application.
 *
 * @param   numEvents - number of events
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_SetLowCreditWarningLevelCB( uint8 numEvents,
                                            afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetDebtRepaymentLogCB
 *
 * @brief   Callback from the ZCL SE Profile Prepayment Cluster Library when
 *          it received a Get Debt Repayment Log for this application.
 *
 * @param   pCmd - pointer to structure for Get Debt Repayment Log command
 * @param   srcAddr - pointer to source address
 * @param   seqNum - sequence number of this command
 *
 * @return  none
 */
static void esp_GetDebtRepaymentLogCB( zclCCGetDebtRepaymentLog_t *pCmd,
                                       afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetPrepaySnapshotResponseCB
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
static void esp_GetPrepaySnapshotResponseCB( zclCCGetPrepaySnapshotResponse_t *pCmd,
                                             afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_ChangePaymentModeResponseCB
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
static void esp_ChangePaymentModeResponseCB( zclCCChangePaymentModeResponse_t *pCmd,
                                             afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_ConsumerTopupResponseCB
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
static void esp_ConsumerTopupResponseCB( zclCCConsumerTopupResponse_t *pCmd,
                                         afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_GetCommandsCB
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
static void esp_GetCommandsCB( uint8 prepayNotificationFlags,
                               afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishTopupLogCB
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
static void esp_PublishTopupLogCB( zclCCPublishTopupLog_t *pCmd,
                                   afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}

/*********************************************************************
 * @fn      esp_PublishDebtLogCB
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
static void esp_PublishDebtLogCB( zclCCPublishDebtLog_t *pCmd,
                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  // add user code here
}
#endif // SE_UK_EXT

/******************************************************************************
 *
 *  Functions for processing ZDO incoming messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      esp_ProcessZDOMsg
 *
 * @brief   Process the incoming ZDO messages.
 *
 * @param   inMsg - message to process
 *
 * @return  none
 */
static void esp_ProcessZDOMsg( zdoIncomingMsg_t *inMsg )
{
  ZDO_DeviceAnnce_t devAnnce;

  switch ( inMsg->clusterID )
  {
    case Device_annce:
      {
        ZDO_ParseDeviceAnnce( inMsg, &devAnnce );
        simpleDescReqAddr.addrMode = (afAddrMode_t)Addr16Bit;
        simpleDescReqAddr.addr.shortAddr = devAnnce.nwkAddr;

        // set simple descriptor query event
        osal_set_event( espTaskID, SIMPLE_DESC_QUERY_EVT );
      }
      break;

    case Simple_Desc_rsp:
      {
        ZDO_SimpleDescRsp_t *pSimpleDescRsp;   // pointer to received simple desc response
        pSimpleDescRsp = (ZDO_SimpleDescRsp_t *)osal_mem_alloc( sizeof( ZDO_SimpleDescRsp_t ) );

        if(pSimpleDescRsp)
        {
          pSimpleDescRsp->simpleDesc.pAppInClusterList = NULL;
          pSimpleDescRsp->simpleDesc.pAppOutClusterList = NULL;

          ZDO_ParseSimpleDescRsp( inMsg, pSimpleDescRsp );
          if( pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_SE_DEVICEID_PCT ) // this is a PCT
          {
            pctAddr.addr.shortAddr = pSimpleDescRsp->nwkAddr;
          }
          else if ( pSimpleDescRsp->simpleDesc.AppDeviceId == ZCL_SE_DEVICEID_LOAD_CTRL_EXTENSION ) // this is a load control device
          {
            loadControlAddr.addr.shortAddr = pSimpleDescRsp->nwkAddr;
          }

          // free memory for InClusterList
          if (pSimpleDescRsp->simpleDesc.pAppInClusterList)
          {
            osal_mem_free(pSimpleDescRsp->simpleDesc.pAppInClusterList);
          }

          // free memory for OutClusterList
          if (pSimpleDescRsp->simpleDesc.pAppOutClusterList)
          {
            osal_mem_free(pSimpleDescRsp->simpleDesc.pAppOutClusterList);
          }

          osal_mem_free( pSimpleDescRsp );
        }
      }
      break;
  }
}


/******************************************************************************
 *
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      esp_ProcessZCLMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - message to process
 *
 * @return  none
 */
static void esp_ProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{

#if defined ( SE_UK_EXT ) && defined ( SE_MIRROR )
  if ( esp_IsMirrorEndpoint( pInMsg->endPoint ) )
  {
    esp_MirrorProcessZCLMsg( pInMsg );
  }
  else
#endif  // SE_UK_EXT && SE_MIRROR
  {
    switch ( pInMsg->zclHdr.commandID )
    {
#if defined ( ZCL_READ )
      case ZCL_CMD_READ_RSP:
        esp_ProcessInReadRspCmd( pInMsg );
        break;
#endif // ZCL_READ
#if defined ( ZCL_WRITE )
      case ZCL_CMD_WRITE_RSP:
        esp_ProcessInWriteRspCmd( pInMsg );
        break;
#endif // ZCL_WRITE
#if defined ( ZCL_REPORT )
      case ZCL_CMD_CONFIG_REPORT:
        esp_ProcessInConfigReportCmd( pInMsg );
        break;

      case ZCL_CMD_CONFIG_REPORT_RSP:
        esp_ProcessInConfigReportRspCmd( pInMsg );
        break;

      case ZCL_CMD_READ_REPORT_CFG:
        esp_ProcessInReadReportCfgCmd( pInMsg );
        break;

      case ZCL_CMD_READ_REPORT_CFG_RSP:
        esp_ProcessInReadReportCfgRspCmd( pInMsg );
        break;

      case ZCL_CMD_REPORT:
        esp_ProcessInReportCmd( pInMsg );
        break;
#endif // ZCL_REPORT
      case ZCL_CMD_DEFAULT_RSP:
        esp_ProcessInDefaultRspCmd( pInMsg );
        break;
#if defined ( ZCL_DISCOVER )
      case ZCL_CMD_DISCOVER_RSP:
        esp_ProcessInDiscRspCmd( pInMsg );
        break;
#endif // ZCL_DISCOVER
      default:
        break;
    }
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
 * @fn      esp_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 esp_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
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
 * @fn      esp_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 esp_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
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

#if defined ( ZCL_REPORT )
/*********************************************************************
 * @fn      esp_ProcessInConfigReportCmd
 *
 * @brief   Process the "Profile" Configure Reporting Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  TRUE if attribute was found in the Attribute list,
 *          FALSE if not
 */
static uint8 esp_ProcessInConfigReportCmd( zclIncomingMsg_t *pInMsg )
{
  zclCfgReportCmd_t *cfgReportCmd;
  zclCfgReportRec_t *reportRec;
  zclCfgReportRspCmd_t *cfgReportRspCmd;
  zclAttrRec_t attrRec;
  uint8 status;
  uint8 i, j = 0;

  cfgReportCmd = (zclCfgReportCmd_t *)pInMsg->attrCmd;

  // Allocate space for the response command
  cfgReportRspCmd = (zclCfgReportRspCmd_t *)osal_mem_alloc( sizeof ( zclCfgReportRspCmd_t ) +
                                        sizeof ( zclCfgReportStatus_t) * cfgReportCmd->numAttr );
  if ( cfgReportRspCmd == NULL )
  {
    return FALSE; // EMBEDDED RETURN
  }

  // Process each Attribute Reporting Configuration record
  for ( i = 0; i < cfgReportCmd->numAttr; i++ )
  {
    reportRec = &(cfgReportCmd->attrList[i]);

    status = ZCL_STATUS_SUCCESS;

    if ( zclFindAttrRec( ESP_ENDPOINT, pInMsg->clusterId, reportRec->attrID, &attrRec ) )
    {
      if ( reportRec->direction == ZCL_SEND_ATTR_REPORTS )
      {
        if ( reportRec->dataType == attrRec.attr.dataType )
        {
          // This the attribute that is to be reported
          if ( zcl_MandatoryReportableAttribute( &attrRec ) == TRUE )
          {
            if ( reportRec->minReportInt < ESP_MIN_REPORTING_INTERVAL ||
                 ( reportRec->maxReportInt != 0 &&
                   reportRec->maxReportInt < reportRec->minReportInt ) )
            {
              // Invalid fields
              status = ZCL_STATUS_INVALID_VALUE;
            }
            else
            {
              // Set the Min and Max Reporting Intervals and Reportable Change
              //status = zclSetAttrReportInterval( pAttr, cfgReportCmd );
              status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE; // for now
            }
          }
          else
          {
            // Attribute cannot be reported
            status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE;
          }
        }
        else
        {
          // Attribute data type is incorrect
          status = ZCL_STATUS_INVALID_DATA_TYPE;
        }
      }
      else
      {
        // We shall expect reports of values of this attribute
        if ( zcl_MandatoryReportableAttribute( &attrRec ) == TRUE )
        {
          // Set the Timeout Period
          //status = zclSetAttrTimeoutPeriod( pAttr, cfgReportCmd );
          status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE; // for now
        }
        else
        {
          // Reports of attribute cannot be received
          status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
        }
      }
    }
    else
    {
      // Attribute is not supported
      status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    // If not successful then record the status
    if ( status != ZCL_STATUS_SUCCESS )
    {
      cfgReportRspCmd->attrList[j].status = status;
      cfgReportRspCmd->attrList[j++].attrID = reportRec->attrID;
    }
  } // for loop

  if ( j == 0 )
  {
    // Since all attributes were configured successfully, include a single
    // attribute status record in the response command with the status field
    // set to SUCCESS and the attribute ID field omitted.
    cfgReportRspCmd->attrList[0].status = ZCL_STATUS_SUCCESS;
    cfgReportRspCmd->numAttr = 1;
  }
  else
  {
    cfgReportRspCmd->numAttr = j;
  }

  // Send the response back
  zcl_SendConfigReportRspCmd( ESP_ENDPOINT, &(pInMsg->srcAddr),
                              pInMsg->clusterId, cfgReportRspCmd, ZCL_FRAME_SERVER_CLIENT_DIR,
                              TRUE, pInMsg->zclHdr.transSeqNum );
  osal_mem_free( cfgReportRspCmd );

  return TRUE ;
}

/*********************************************************************
 * @fn      esp_ProcessInConfigReportRspCmd
 *
 * @brief   Process the "Profile" Configure Reporting Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 esp_ProcessInConfigReportRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclCfgReportRspCmd_t *cfgReportRspCmd;
  zclAttrRec_t attrRec;
  uint8 i;

  cfgReportRspCmd = (zclCfgReportRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < cfgReportRspCmd->numAttr; i++)
  {
    if ( zclFindAttrRec( ESP_ENDPOINT, pInMsg->clusterId,
                         cfgReportRspCmd->attrList[i].attrID, &attrRec ) )
    {
      // Notify the device of success (or otherwise) of the its original configure
      // reporting command, for each attribute.
    }
  }

  return TRUE;
}

/*********************************************************************
 * @fn      esp_ProcessInReadReportCfgCmd
 *
 * @brief   Process the "Profile" Read Reporting Configuration Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 esp_ProcessInReadReportCfgCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadReportCfgCmd_t *readReportCfgCmd;
  zclReadReportCfgRspCmd_t *readReportCfgRspCmd;
  zclReportCfgRspRec_t *reportRspRec;
  zclAttrRec_t attrRec;
  uint8 reportChangeLen;
  uint8 *dataPtr;
  uint8 hdrLen;
  uint8 dataLen = 0;
  uint8 status;
  uint8 i;

  readReportCfgCmd = (zclReadReportCfgCmd_t *)pInMsg->attrCmd;

  // Find out the response length (Reportable Change field is of variable length)
  for ( i = 0; i < readReportCfgCmd->numAttr; i++ )
  {
    // For supported attributes with 'analog' data type, find out the length of
    // the Reportable Change field
    if ( zclFindAttrRec( ESP_ENDPOINT, pInMsg->clusterId,
                         readReportCfgCmd->attrList[i].attrID, &attrRec ) )
    {
      if ( zclAnalogDataType( attrRec.attr.dataType ) )
      {
         reportChangeLen = zclGetDataTypeLength( attrRec.attr.dataType );

         // add padding if neede
         if ( PADDING_NEEDED( reportChangeLen ) )
         {
           reportChangeLen++;
         }
         dataLen += reportChangeLen;
      }
    }
  }

  hdrLen = sizeof( zclReadReportCfgRspCmd_t ) + ( readReportCfgCmd->numAttr * sizeof( zclReportCfgRspRec_t ) );

  // Allocate space for the response command
  readReportCfgRspCmd = (zclReadReportCfgRspCmd_t *)osal_mem_alloc( hdrLen + dataLen );
  if ( readReportCfgRspCmd == NULL )
  {
    return FALSE; // EMBEDDED RETURN
  }

  dataPtr = (uint8 *)( (uint8 *)readReportCfgRspCmd + hdrLen );
  readReportCfgRspCmd->numAttr = readReportCfgCmd->numAttr;
  for (i = 0; i < readReportCfgCmd->numAttr; i++)
  {
    reportRspRec = &(readReportCfgRspCmd->attrList[i]);

    if ( zclFindAttrRec( ESP_ENDPOINT, pInMsg->clusterId,
                         readReportCfgCmd->attrList[i].attrID, &attrRec ) )
    {
      if ( zcl_MandatoryReportableAttribute( &attrRec ) == TRUE )
      {
        // Get the Reporting Configuration
        // status = zclReadReportCfg( readReportCfgCmd->attrID[i], reportRspRec );
        status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE; // for now
        if ( status == ZCL_STATUS_SUCCESS && zclAnalogDataType( attrRec.attr.dataType ) )
        {
          reportChangeLen = zclGetDataTypeLength( attrRec.attr.dataType );
          //osal_memcpy( dataPtr, pBuf, reportChangeLen );
          reportRspRec->reportableChange = dataPtr;

          // add padding if needed
          if ( PADDING_NEEDED( reportChangeLen ) )
          {
            reportChangeLen++;
          }
          dataPtr += reportChangeLen;
        }
      }
      else
      {
        // Attribute not in the Mandatory Reportable Attribute list
        status = ZCL_STATUS_UNREPORTABLE_ATTRIBUTE;
      }
    }
    else
    {
      // Attribute not found
      status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
    }

    reportRspRec->status = status;
    reportRspRec->attrID = readReportCfgCmd->attrList[i].attrID;
  }

  // Send the response back
  zcl_SendReadReportCfgRspCmd( ESP_ENDPOINT, &(pInMsg->srcAddr),
                               pInMsg->clusterId, readReportCfgRspCmd, ZCL_FRAME_SERVER_CLIENT_DIR,
                               TRUE, pInMsg->zclHdr.transSeqNum );
  osal_mem_free( readReportCfgRspCmd );

  return TRUE;
}

/*********************************************************************
 * @fn      esp_ProcessInReadReportCfgRspCmd
 *
 * @brief   Process the "Profile" Read Reporting Configuration Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 esp_ProcessInReadReportCfgRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadReportCfgRspCmd_t *readReportCfgRspCmd;
  zclReportCfgRspRec_t *reportRspRec;
  uint8 i;

  readReportCfgRspCmd = (zclReadReportCfgRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < readReportCfgRspCmd->numAttr; i++ )
  {
    reportRspRec = &(readReportCfgRspCmd->attrList[i]);

    // Notify the device of the results of the its original read reporting
    // configuration command.

    if ( reportRspRec->status == ZCL_STATUS_SUCCESS )
    {
      if ( reportRspRec->direction == ZCL_SEND_ATTR_REPORTS )
      {
        // add user code here
      }
      else
      {
        // expecting attribute reports
      }
    }
  }

  return TRUE;
}

/*********************************************************************
 * @fn      esp_ProcessInReportCmd
 *
 * @brief   Process the "Profile" Report Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 esp_ProcessInReportCmd( zclIncomingMsg_t *pInMsg )
{
  zclReportCmd_t *reportCmd;
  zclReport_t *reportRec;
  uint8 i;
  uint8 *meterData;
  char lcdBuf[13];

  reportCmd = (zclReportCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < reportCmd->numAttr; i++)
  {
    // Device is notified of the latest values of the attribute of another device.
    reportRec = &(reportCmd->attrList[i]);

    if ( reportRec->attrID == ATTRID_SE_CURRENT_SUMMATION_DELIVERED )
    {
      // process simple metering current summation delivered attribute
      meterData = reportRec->attrData;

      // process to convert hex to ascii
      for(i=0; i<6; i++)
      {
        if(meterData[5-i] == 0)
        {
          lcdBuf[i*2] = '0';
          lcdBuf[i*2+1] = '0';
        }
        else if(meterData[5-i] <= 0x0A)
        {
          lcdBuf[i*2] = '0';
          _ltoa(meterData[5-i],(uint8*)&lcdBuf[i*2+1],16);
        }
        else
        {
          _ltoa(meterData[5-i],(uint8*)&lcdBuf[i*2],16);
        }
      }

      // print out value of current summation delivered in hex
      HalLcdWriteString("Zigbee Coord esp", HAL_LCD_LINE_1);
      HalLcdWriteString("Curr Summ Dlvd", HAL_LCD_LINE_2);
      HalLcdWriteString(lcdBuf, HAL_LCD_LINE_3);
    }
  }
  return TRUE;
}
#endif // ZCL_REPORT

/*********************************************************************
 * @fn      esp_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 esp_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;

  // Device is notified of the Default Response command.

  return TRUE;
}

#if defined ( ZCL_DISCOVER )
/*********************************************************************
 * @fn      esp_ProcessInDiscRspCmd
 *
 * @brief   Process the "Profile" Discover Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 esp_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
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

#if defined ( SE_UK_EXT ) && defined ( SE_MIRROR )
/*********************************************************************
 * @fn      esp_MirrorInit
 *
 * @brief   Initialize the ESP Mirror Subsystem.
 *
 * @param   none
 *
 * @return  none
 */
static void esp_MirrorInit( void )
{
  osal_memset( &esp_MirrorControl, 0, sizeof( esp_MirrorControl ) );

  // set the attribute
  espPhysicalEnvironment |= PHY_MIRROR_CAPACITY_ENV;
}

/*********************************************************************
 * @fn      esp_GetMirrorEndpoint
 *
 * @brief   Get the endpoint of a mirror using the Source Address and
 *          source endpoint.
 *
 * @param   srcAddr - source address to lookup
 *
 * @return  Endpoint of the mirror or
 *          ESP_MIRROR_INVALID_ENDPOINT if there is no space
 */
static uint8 esp_GetMirrorEndpoint( afAddrType_t *srcAddr )
{
  uint8 i;

  for ( i = 0; i < ESP_MAX_MIRRORS; i++ )
  {
    if ( ( esp_MirrorControl.mirrorMask & (1 << i) ) &&
         ( esp_MirrorControl.mirrorInfo[i].srcAddr == srcAddr->addr.shortAddr ) &&
         ( esp_MirrorControl.mirrorInfo[i].srcEndpoint == srcAddr->endPoint ) )
    {
      return ( i + ESP_MIRROR_EP_BASE );
    }
  }

  return ESP_MIRROR_INVALID_ENDPOINT;
}

/*********************************************************************
 * @fn      esp_GetMirrorInfo
 *
 * @brief   Get the control information for a mirror endpoint.
 *
 * @param   endpoint - to lookup
 *
 * @return  Pointer to Mirror Information or
 *          NULL if no Mirror found for the Endpoint
 */
static espMirrorInfo_t *esp_GetMirrorInfo( uint8 endpoint )
{
  if ( esp_IsMirrorEndpoint( endpoint ) )
  {
    // Get the index to the mirror
    uint8 index = endpoint - ESP_MIRROR_EP_BASE;

    // Return a pointer to the attributes
    return &esp_MirrorControl.mirrorInfo[index];
  }

  return NULL;
}

/*********************************************************************
 * @fn      esp_MirrorInitAttributeSet
 *
 * @brief   Adds notification attributes to the mirror endpoint
 *
 * @param   endpoint - Endpoint of the mirror
 *
 * @return  none
 */
static void esp_MirrorInitAttributeSet( uint8 endpoint )
{
  espMirrorInfo_t *pInfo = esp_GetMirrorInfo( endpoint );

  if ( pInfo != NULL )
  {
    zclAttrRec_t *pAttributes = pInfo->pAttr;

    if ( pAttributes != NULL )
    {
      // Note: Attributes 0 through ESP_MIRROR_USER_ATTRIBUTES_POSITION-1 are used for
      // the mirror notify attribute set.  The rest of the attributes are reserved for
      // the meter.  The meter creates attributes with a report attribute command.
      pAttributes[0].clusterID = ZCL_CLUSTER_ID_SE_SIMPLE_METERING;
      pAttributes[0].attr.attrId = ATTRID_SE_NOTIFICATION_CONTROL_FLAGS;
      pAttributes[0].attr.dataType = ZCL_DATATYPE_BITMAP8;
      pAttributes[0].attr.accessControl = ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE;
      pAttributes[0].attr.dataPtr = &pInfo->notificationControl;

      pAttributes[1].clusterID = ZCL_CLUSTER_ID_SE_SIMPLE_METERING;
      pAttributes[1].attr.attrId = ATTRID_SE_NOTIFICATION_FLAGS;
      pAttributes[1].attr.dataType = ZCL_DATATYPE_BITMAP8;
      pAttributes[1].attr.accessControl = ACCESS_CONTROL_READ;
      pAttributes[1].attr.dataPtr = &pInfo->notificationSet.NotificationFlags;

      pAttributes[2].clusterID = ZCL_CLUSTER_ID_SE_SIMPLE_METERING;
      pAttributes[2].attr.attrId = ATTRID_SE_PRICE_NOTIFICATION_FLAGS;
      pAttributes[2].attr.dataType = ZCL_DATATYPE_BITMAP16;
      pAttributes[2].attr.accessControl = ACCESS_CONTROL_READ;
      pAttributes[2].attr.dataPtr = &pInfo->notificationSet.PriceNotificationFlags;

      pAttributes[3].clusterID = ZCL_CLUSTER_ID_SE_SIMPLE_METERING;
      pAttributes[3].attr.attrId = ATTRID_SE_CALENDAR_NOTIFICATION_FLAGS;
      pAttributes[3].attr.dataType = ZCL_DATATYPE_BITMAP8;
      pAttributes[3].attr.accessControl = ACCESS_CONTROL_READ;
      pAttributes[3].attr.dataPtr = &pInfo->notificationSet.CalendarNotificationFlags;

      pAttributes[4].clusterID = ZCL_CLUSTER_ID_SE_SIMPLE_METERING;
      pAttributes[4].attr.attrId = ATTRID_SE_PRE_PAY_NOTIFICATION_FLAGS;
      pAttributes[4].attr.dataType = ZCL_DATATYPE_BITMAP16;
      pAttributes[4].attr.accessControl = ACCESS_CONTROL_READ;
      pAttributes[4].attr.dataPtr = &pInfo->notificationSet.PrePayNotificationFlags;

      pAttributes[5].clusterID = ZCL_CLUSTER_ID_SE_SIMPLE_METERING;
      pAttributes[5].attr.attrId = ATTRID_SE_DEVICE_MANAGEMENT_FLAGS;
      pAttributes[5].attr.dataType = ZCL_DATATYPE_BITMAP8;
      pAttributes[5].attr.accessControl = ACCESS_CONTROL_READ;
      pAttributes[5].attr.dataPtr = &pInfo->notificationSet.DeviceMgmtNotificationFlags;
    }
  }
}

/*********************************************************************
 * @fn      esp_AllocMirror
 *
 * @brief   Allocate a mirror endpoint in the mirror control structure.
 *
 * @param   srcAddr - of the device requesting a Mirror
 *
 * @return  Endpoint of the mirror or
 *          ESP_MIRROR_INVALID_ENDPOINT if there is no space
 */
static uint8 esp_AllocMirror( afAddrType_t *srcAddr )
{
  uint8 i;
  zclAttrRec_t *pAttr;
  uint8 endpoint;

  // Verify space is available for another mirror endpoint
  if ( esp_MirrorControl.mirrorMask != ESP_MIRROR_FULL_MASK )
  {
    // Allocate memory for the attribute table
    pAttr = osal_mem_alloc( sizeof( zclAttrRec_t ) * ESP_MIRROR_MAX_ATTRIBUTES );

    if ( pAttr != NULL )
    {
      // Initialize the attribute
      osal_memset( pAttr, 0, sizeof( zclAttrRec_t ) * ESP_MIRROR_MAX_ATTRIBUTES );

      // Set the attribute clusters to 0xFFFF indicating they are not in use
      for ( i = 0; i < ESP_MIRROR_MAX_ATTRIBUTES; i++ )
      {
        pAttr[i].clusterID = 0xFFFF;
      }

      // Find the next free endpoint slot
      for ( i = 0; i < ESP_MAX_MIRRORS; i++ )
      {
        if ( esp_MirrorControl.mirrorMask & (1 << i) )
        {
          continue;
        }

        endpoint = i + ESP_MIRROR_EP_BASE;

        // Zero out the info memory
        osal_memset( &esp_MirrorControl.mirrorInfo[i], 0, sizeof( espMirrorInfo_t ) );

        // Setup mirror information
        esp_MirrorControl.mirrorMask |= 1 << i;
        esp_MirrorControl.mirrorInfo[i].srcAddr = srcAddr->addr.shortAddr;
        esp_MirrorControl.mirrorInfo[i].srcEndpoint = srcAddr->endPoint;
        esp_MirrorControl.mirrorInfo[i].pAttr = pAttr;

        // Create attributes for the notification set
        esp_MirrorInitAttributeSet( endpoint );

        // If all endpoints are in use, set the PhysicalEnvironment attribute
        // indicating all mirror slot are used
        if ( esp_MirrorControl.mirrorMask == ESP_MIRROR_FULL_MASK )
        {
          espPhysicalEnvironment &= ~PHY_MIRROR_CAPACITY_ENV;
        }

        return endpoint;
      }

      // Free the attribute memory if we could not allocate the endpoint
      osal_mem_free( pAttr );
    }
  }

  return ESP_MIRROR_INVALID_ENDPOINT;
}

/*********************************************************************
 * @fn      esp_FreeMirror
 *
 * @brief   Free a mirror endpoint in the mirror control structure.
 *
 * @param   endPoint - Endpoint of the mirror to free
 *
 * @return  none
 */
void esp_FreeMirror( uint8 endPoint )
{
  if ( esp_IsMirrorEndpoint( endPoint ) )
  {
    // Get the index to the mirror
    uint8 index = endPoint - ESP_MIRROR_EP_BASE;
    uint8 i;

    // Clear the endpoint bit in the mask of allocated mirrors
    esp_MirrorControl.mirrorMask &= ~(1 << index);

    if ( esp_MirrorControl.mirrorInfo[index].pAttr )
    {
      // Free the user attribute data
      for ( i = ESP_MIRROR_USER_ATTRIBUTES_POSITION; i < ESP_MIRROR_MAX_ATTRIBUTES; i++ )
      {
        if ( esp_MirrorControl.mirrorInfo[index].pAttr[i].attr.dataPtr )
        {
          osal_mem_free( esp_MirrorControl.mirrorInfo[index].pAttr[i].attr.dataPtr );
          esp_MirrorControl.mirrorInfo[index].pAttr[i].attr.dataPtr = NULL;
        }
      }

      // Free the attribute table
      osal_mem_free( esp_MirrorControl.mirrorInfo[index].pAttr );
      esp_MirrorControl.mirrorInfo[index].pAttr = NULL;
    }
  }
}

/*********************************************************************
 * @fn      esp_IsMirrorEndpoint
 *
 * @brief   Check if the endpoint is in the mirror block of endpoints.
 *
 * @param   endpoint - Endpoint of the mirror to free
 *
 * @return  TRUE - if Endpoint is in the Mirror Block
 *          FALSE - Otherwise
 */
static uint8 esp_IsMirrorEndpoint( uint8 endpoint )
{
  if ( (endpoint >= ESP_MIRROR_EP_BASE ) &&
       (endpoint <= ESP_MIRROR_EP_BASE + ESP_MAX_MIRRORS ) )
  {
    return TRUE;
  }

  return FALSE;
}

/*********************************************************************
 * @fn      esp_MirrorUpdateAttribute
 *
 * @brief   Update Attributes in Mirror
 *
 * @param   endpoint - Endpoint of the mirror to free
 *          cluster - Cluster ID
 *          pReport - Pointer to reported attributes
 *
 * @return  TRUE - if Data updated
 *          FALSE - Otherwise
 */
static uint8 esp_MirrorUpdateAttribute( uint8 endpoint, uint16 cluster, zclReport_t *pReport )
{
  espMirrorInfo_t *pInfo = esp_GetMirrorInfo( endpoint );

  if ( pInfo != NULL )
  {
    zclAttrRec_t *pAttributes = pInfo->pAttr;
    uint8 i;

    if ( pAttributes != NULL )
    {
      // Check if the attribute already exists
      for ( i = ESP_MIRROR_USER_ATTRIBUTES_POSITION; i < ESP_MIRROR_MAX_ATTRIBUTES; i++ )
      {
        if ( pAttributes[i].clusterID == cluster )
        {
          if ( pAttributes[i].attr.attrId == pReport->attrID )
          {
            if ( pAttributes[i].attr.dataPtr )
            {
              // Update the attribute data
              zclSerializeData( pReport->dataType, pReport->attrData, pAttributes[i].attr.dataPtr );
              return TRUE;
            }
          }
        }
      }

      // Look for a free attribute slot and add the attribute
      for ( i = ESP_MIRROR_USER_ATTRIBUTES_POSITION; i < ESP_MIRROR_MAX_ATTRIBUTES; i++ )
      {
        if ( pAttributes[i].attr.dataPtr == NULL )
        {
          uint8 dataLength = zclGetDataTypeLength( pReport->dataType );

          if ( dataLength > 0 )
          {
            pAttributes[i].clusterID = cluster;
            pAttributes[i].attr.attrId = pReport->attrID;
            pAttributes[i].attr.dataType = pReport->dataType;
            pAttributes[i].attr.accessControl = ACCESS_CONTROL_READ;
            pAttributes[i].attr.dataPtr = osal_mem_alloc( dataLength );

            if ( pAttributes[i].attr.dataPtr != NULL )
            {
              zclSerializeData( pReport->dataType, pReport->attrData, pAttributes[i].attr.dataPtr );
              return TRUE;
            }
          }
        }
      }
    }
  }

  return FALSE;
}

/*********************************************************************
 * @fn      esp_MirrorProcessZCLMsg
 *
 * @brief   Process ZCL messages for mirror endpoints.
 *
 * @param   pInMsg - ZCL Message
 *
 * @return  none
 */
static void esp_MirrorProcessZCLMsg( zclIncomingMsg_t *pInMsg )
{
  uint8 i;

  if ( pInMsg->zclHdr.commandID == ZCL_CMD_REPORT )
  {
    zclReportCmd_t *reportCmd = (zclReportCmd_t *)pInMsg->attrCmd;

    if ( reportCmd != NULL)
    {
      for (i = 0; i < reportCmd->numAttr; i++)
      {
        // Update the attribute
        esp_MirrorUpdateAttribute( pInMsg->endPoint, pInMsg->clusterId, &reportCmd->attrList[i] );
      }
    }

    // Build a response
    espMirrorInfo_t *pInfo = esp_GetMirrorInfo( pInMsg->srcAddr.endPoint );

    if ( pInfo != NULL )
    {
      if ( pInfo->notificationControl & SE_NOTIFICATION_REPORT_ATTR_RSP_BIT )
      {
        // Send a mirror report attr rsp using the notification set from the mirror info
        zclSE_SimpleMetering_Send_MirrorReportAttrRsp( pInMsg->endPoint, &pInMsg->srcAddr,
                                                       &pInfo->notificationSet, TRUE,
                                                       pInMsg->zclHdr.transSeqNum);
      }
    }
  }
}
#endif  // SE_UK_EXT && SE_MIRROR
/****************************************************************************
****************************************************************************/
