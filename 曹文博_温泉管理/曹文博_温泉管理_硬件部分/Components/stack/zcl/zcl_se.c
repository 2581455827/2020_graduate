/**************************************************************************************************
  Filename:       zcl_se.c
  Revised:        $Date: 2012-03-11 00:25:21 -0800 (Sun, 11 Mar 2012) $
  Revision:       $Revision: 29708 $

  Description:    Zigbee Cluster Library - SE (Smart Energy) Profile.


  Copyright 2007-2012 Texas Instruments Incorporated. All rights reserved.

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
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_se.h"
#include "DebugTrace.h"

#if defined ( INTER_PAN )
  #include "stub_aps.h"
#endif


#include "zcl_key_establish.h"


/*********************************************************************
 * MACROS
 */
// Clusters that are supported thru Inter-PAN communication
#define INTER_PAN_CLUSTER( id )  ( (id) == ZCL_CLUSTER_ID_SE_PRICING || \
                                   (id) == ZCL_CLUSTER_ID_SE_MESSAGE )

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */
typedef struct zclSECBRec
{
  struct zclSECBRec          *next;
  uint8                       endpoint; // Used to link it into the endpoint descriptor
  zclSE_AppCallbacks_t       *CBs;     // Pointer to Callback function
} zclSECBRec_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static zclSECBRec_t *zclSECBs = (zclSECBRec_t *)NULL;
static uint8 zclSEPluginRegisted = FALSE;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static ZStatus_t zclSE_HdlIncoming(  zclIncoming_t *pInMsg );
static ZStatus_t zclSE_HdlInSpecificCommands( zclIncoming_t *pInMsg );

#ifdef ZCL_SIMPLE_METERING
static ZStatus_t zclSE_ProcessInSimpleMeteringCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_GetProfileCmd( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_GetProfileRsp( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_ReqMirrorCmd( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_ReqMirrorRsp( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_MirrorRemCmd( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_MirrorRemRsp( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_ReqFastPollModeCmd( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_ReqFastPollModeRsp( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#ifdef SE_UK_EXT
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_GetSnapshotCmd( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_GetSnapshotRsp( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_TakeSnapshotCmd( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_MirrorReportAttrRsp( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#endif  // SE_UK_EXT
#endif  // ZCL_SIMPLE_METERING

#ifdef ZCL_PRICING
static ZStatus_t zclSE_ProcessInPricingCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetCurrentPrice( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetScheduledPrice( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishPrice( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PriceAcknowledgement( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetBlockPeriod( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishBlockPeriod( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#ifdef SE_UK_EXT
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishTariffInformation( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishPriceMatrix( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishBlockThreshold( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishConversionFactor( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishCalorificValue( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishCO2Value( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishCPPEvent( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishBillingPeriod( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishConsolidatedBill( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishCreditPaymentInfo( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetTariffInformation( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetPriceMatrix( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetBlockThresholds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetConversionFactor( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetCalorificValue( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetCO2Value( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetBillingPeriod( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetConsolidatedBill( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Pricing_CPPEventResponse( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#endif  // SE_UK_EXT
#endif  // ZCL_PRICING

#ifdef ZCL_MESSAGE
static ZStatus_t zclSE_ProcessInMessageCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Message_DisplayMessage( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Message_CancelMessage( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Message_GetLastMessage( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Message_MessageConfirmation( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#endif  // ZCL_MESSAGE

#ifdef ZCL_LOAD_CONTROL
static ZStatus_t zclSE_ProcessInLoadControlCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_LoadControl_LoadControlEvent( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_LoadControl_CancelLoadControlEvent( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_LoadControl_CancelAllLoadControlEvents( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_LoadControl_ReportEventStatus( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_LoadControl_GetScheduledEvents( zclIncoming_t *pInMsg,
                                                                    zclSE_AppCallbacks_t *pCBs );
#endif  // ZCL_LOAD_CONTROL

#ifdef ZCL_TUNNELING
static ZStatus_t zclSE_ProcessInTunnelingCmds(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_RequestTunnel(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_ReqTunnelRsp(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_CloseTunnel(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_TransferData(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_TransferDataError(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_AckTransferData(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_ReadyData(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
#ifdef SE_UK_EXT
static ZStatus_t zclSE_ProcessInCmd_Tunneling_GetSuppTunnelProt(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_SuppTunnelProtRsp(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
static ZStatus_t zclSE_ProcessInCmd_Tunneling_TunnelClosureNotification(zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
#endif  // SE_UK_EXT
#endif  // ZCL_TUNNELING

#ifdef ZCL_PREPAYMENT
static ZStatus_t zclSE_ProcessInPrepaymentCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_SelAvailEmergencyCredit( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs);
#ifndef SE_UK_EXT
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ChangeSupply( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_SupplyStatusResponse( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#else // SE_UK_EXT
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ChangeDebt( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_EmergencyCreditSetup( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ConsumerTopup( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_CreditAdjustment( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ChangePaymentMode( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetPrepaySnapshot( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetTopupLog( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_SetLowCreditWarningLevel( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetDebtRepaymentLog( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetPrepaySnapshotResponse( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ChangePaymentModeResponse( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ConsumerTopupResponse( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetCommands( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_PublishTopupLog( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Prepayment_PublishDebtLog( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#endif  // SE_UK_EXT
#endif  // ZCL_PREPAYMENT

#ifdef ZCL_TOU
#ifdef SE_UK_EXT
static ZStatus_t zclSE_ProcessInTouCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishCalendar( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishDayProfile( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishWeekProfile( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishSeasons( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishSpecialDays( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_GetCalendar( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_GetDayProfiles( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_GetWeekProfiles( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_GetSeasons( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_Tou_GetSpecialDays( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#endif  // SE_UK_EXT
#endif  // ZCL_TOU

#ifdef ZCL_DEVICE_MGMT
#ifdef SE_UK_EXT
static ZStatus_t zclSE_ProcessInDeviceMgmtCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_GetChangeTenancy( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_GetChangeSupplier( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_GetChangeSupply( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_SupplyStatusResponse( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_GetPassword( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_PublishChangeTenancy( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_PublishChangeSupplier( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_ChangeSupply( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_ChangePassword( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_LocalChangeSupply( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs );
#endif  // SE_UK_EXT
#endif  // ZCL_DEVICE_MGMT

#ifdef SE_UK_EXT
static uint8 zclSE_Parse_UTF8String( uint8 *pBuf, UTF8String_t *pString, uint8 maxLen );
#endif  // SE_UK_EXT

#ifdef ZCL_SIMPLE_METERING
/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_GetProfileCmd
 *
 * @brief   Call to send out a Get Profile Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   channel - returned inteval (delivered @ 0; received @ 1)
 * @param   endTime - UTC time for the starting time of requested interval
 * @param   numOfPeriods - number of periods requested
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_GetProfileCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                      uint8 channel, uint32 endTime, uint8 numOfPeriods,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[6];

  buf[0] = channel;
  osal_buffer_uint32( &buf[1], endTime );
  buf[5] = numOfPeriods;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                          COMMAND_SE_GET_PROFILE_CMD, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum, 6, buf );
}

/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_GetProfileRsp
 *
 * @brief   Call to send out a Get Profile Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   endTime - UTC time for the starting time of requested interval
 * @param   intervals - data buffer holding an array of interval data captured
 *          using the period
 *          specified by the ProfileIntervalPeriod attribute. Data is organized
 *          in a reverse chronological order, the most recent interval is
 *          transmitted first and the oldest interval is transmitted last.
 *          Invalid intervals intervals should be marked as 0xFFFFFF
 * @param   len - length of the intervals buffer
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_GetProfileRsp( uint8 srcEP, afAddrType_t *dstAddr,
                                                   uint32 endTime, uint8 rspStatus, uint8 profileIntervalPeriod,
                                                   uint8 numOfPeriodDelivered, uint24 *intervals,
                                                   uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 i;
  uint8 len;
  ZStatus_t status;

  // endTime + status + profileIntervalPeriod + numOfEntry + array
  len = 4 + 1 + 1 + 1 + (3 * numOfPeriodDelivered);
  buf = osal_mem_alloc( len );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  osal_buffer_uint32( buf, endTime );
  buf[4] = rspStatus;
  buf[5] = profileIntervalPeriod;

  // Starting of the array of uint24
  buf[6] = numOfPeriodDelivered;   // Number of entries in the array
  pBuf = &buf[7];
  for ( i = 0; i < numOfPeriodDelivered; i++ )
  {
    pBuf = osal_buffer_uint24( pBuf, *intervals++ );
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                            COMMAND_SE_GET_PROFILE_RSP, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0, seqNum, len, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_ReqMirrorRsp
 *
 * @brief   Call to send out a Request Mirror Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_ReqMirrorRsp( uint8 srcEP, afAddrType_t *dstAddr,
                                                      uint16 endpointId,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  ZStatus_t status;
  uint8 buf[2];

  buf[0] = (uint8) endpointId ;
  buf[1] = (uint8)( endpointId >> 8 );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                            COMMAND_SE_REQ_MIRROR_RSP, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum, 2, buf );

  return status ;
}

/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_RemMirrorRsp
 *
 * @brief   Call to send out a Remove Mirror Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_RemMirrorRsp( uint8 srcEP, afAddrType_t *dstAddr,
                                                      uint16 endpointId,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  ZStatus_t status;
  uint8 buf[2];

  buf[0] = (uint8) endpointId ;
  buf[1] = (uint8)( endpointId >> 8 );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                            COMMAND_SE_MIRROR_REM_RSP, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum, 2, buf );

  return status ;
}


/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_ReqFastPollModeCmd
 *
 * @brief   Call to send out a Request Fast Poll Mode Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - zclCCReqFastPollModeCmd_t command
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_ReqFastPollModeCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                        zclCCReqFastPollModeCmd_t *pCmd,
                                                        uint8 disableDefaultRsp, uint8 seqNum )
{
  ZStatus_t status;
  uint8 buf[PACKET_LEN_SE_METERING_FAST_POLLING_REQ];

  buf[0] = pCmd->fastPollUpdatePeriod;
  buf[1] = pCmd->duration;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                            COMMAND_SE_REQ_FAST_POLL_MODE_CMD, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum,
                            PACKET_LEN_SE_METERING_FAST_POLLING_REQ, buf );

  return status ;
}

/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_ReqFastPollModeRsp
 *
 * @brief   Call to send out a Request Fast Poll Mode Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - zclCCReqFastPollModeRsp_t command
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_ReqFastPollModeRsp( uint8 srcEP, afAddrType_t *dstAddr,
                                                        zclCCReqFastPollModeRsp_t *pCmd,
                                                        uint8 disableDefaultRsp, uint8 seqNum )
{
  ZStatus_t status;
  uint8 buf[PACKET_LEN_SE_METERING_FAST_POLLING_RSP];

  buf[0] = pCmd->appliedUpdatePeriod;
  osal_buffer_uint32( &buf[1], pCmd->fastPollModeEndTime );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                            COMMAND_SE_REQ_FAST_POLL_MODE_RSP, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0, seqNum,
                            PACKET_LEN_SE_METERING_FAST_POLLING_RSP, buf );

  return status ;
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_GetSnapshotCmd
 *
 * @brief   Call to send out a Get Snapshot Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - zclCCReqGetSnapshotCmd_t command
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_GetSnapshotCmd( uint8 srcEP, afAddrType_t *dstAddr,
                                                    zclCCReqGetSnapshotCmd_t *pCmd,
                                                    uint8 disableDefaultRsp, uint8 seqNum )
{
  ZStatus_t status;
  uint8 buf[PACKET_LEN_SE_GET_SNAPSHOT_CMD];

  osal_buffer_uint32( &buf[0], pCmd->StartTime );
  buf[4] = pCmd->NumberOfSnapshots;
  buf[5] = LO_UINT16( pCmd->SnapshotCause );
  buf[6] = HI_UINT16( pCmd->SnapshotCause );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                            COMMAND_SE_GET_SNAPSHOT_CMD, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum,
                            PACKET_LEN_SE_GET_SNAPSHOT_CMD, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_GetSnapshotRsp
 *
 * @brief   Call to send out a Get Snapshot Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - zclCCReqGetSnapshotRsp_t command
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_GetSnapshotRsp( uint8 srcEP, afAddrType_t *dstAddr,
                                                    zclCCReqGetSnapshotRsp_t *pCmd,
                                                    uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;
  uint16 payloadLen;

  if ( pCmd->pSnapshotPayload )
  {
    switch( pCmd->SnapshotPayloadType )
    {
      case SE_SNAPSHOT_TYPE_CSD_AND_RCV_REGISTER:
        payloadLen = SE_SNAPSHOT_CSD_AND_RCV_REGISTER_PAYLOAD_LEN;
        break;

      case SE_SNAPSHOT_TYPE_TOU_INFO_RECEIVED:
      case SE_SNAPSHOT_TYPE_TOU_INFO_DELIVERED:
        // Len in Bytes = (uint8) + (NumberOfTiersInUse * (48BitIntegers))
        payloadLen = 1 + (pCmd->pSnapshotPayload[0] * 6);
        break;

      case SE_SNAPSHOT_TYPE_BLOCK_INFO_RECEIVED:
      case SE_SNAPSHOT_TYPE_BLOCK_INFO_DELIVERED:
        // Len in Bytes = (uint8) + (NumberOfTiersAndBlockThresholdsInUse * (32BitIntegers))
        payloadLen = 1 + (pCmd->pSnapshotPayload[0] * 4);
        break;

      default:
        return ZInvalidParameter;
    }

    bufLen = PACKET_LEN_SE_GET_SNAPSHOT_RSP + payloadLen;

    buf = osal_mem_alloc( bufLen );
    if ( buf == NULL )
    {
      return ( ZMemError );
    }

    pBuf = osal_buffer_uint32( buf, pCmd->IssuerEventID );
    pBuf = osal_buffer_uint32( pBuf, pCmd->SnapshotTime );
    *pBuf++ = pCmd->CommandIndex;
    *pBuf++ = LO_UINT16( pCmd->SnapshotCause );
    *pBuf++ = HI_UINT16( pCmd->SnapshotCause );
    *pBuf++ = pCmd->SnapshotPayloadType;

    osal_memcpy(pBuf, pCmd->pSnapshotPayload, payloadLen);

    status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                              COMMAND_SE_GET_SNAPSHOT_RSP, TRUE,
                              ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0, seqNum,
                              bufLen, buf );

    osal_mem_free( buf );

    return status;
  }
  else
  {
    return ZInvalidParameter;
  }
}

/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_TakeSnapshot
 *
 * @brief   Call to send out a Take Snapshot
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_TakeSnapshot( uint8 srcEP, afAddrType_t *dstAddr,
                                                  uint8 disableDefaultRsp, uint8 seqNum )
{
  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                          COMMAND_SE_TAKE_SNAPSHOT_CMD, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum,
                          0, NULL );
}

/*********************************************************************
 * @fn      zclSE_SimpleMetering_Send_MirrorReportAttrRsp
 *
 * @brief   Call to send out a Mirror Report Attribute Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - zclCCReqMirrorReportAttrRsp_t command
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_SimpleMetering_Send_MirrorReportAttrRsp( uint8 srcEP, afAddrType_t *dstAddr,
                                                         zclCCReqMirrorReportAttrRsp_t *pCmd,
                                                         uint8 disableDefaultRsp, uint8 seqNum )
{
  ZStatus_t status;
  uint8 buf[PACKET_LEN_SE_MIRROR_REPORT_ATTR_RSP];

  buf[0] = pCmd->NotificationFlags;
  buf[1] = LO_UINT16( pCmd->PriceNotificationFlags );
  buf[2] = HI_UINT16( pCmd->PriceNotificationFlags );
  buf[3] = pCmd->CalendarNotificationFlags;
  buf[4] = LO_UINT16( pCmd->PrePayNotificationFlags );
  buf[5] = HI_UINT16( pCmd->PrePayNotificationFlags );
  buf[6] = pCmd->DeviceMgmtNotificationFlags;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SIMPLE_METERING,
                            COMMAND_SE_MIRROR_REPORT_ATTR_RSP, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum,
                            PACKET_LEN_SE_MIRROR_REPORT_ATTR_RSP, buf );

  return status;
}
#endif  // SE_UK_EXT
#endif  // ZCL_SIMPLE_METERING

#ifdef ZCL_PRICING
/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetScheduledPrice
 *
 * @brief   Call to send out a Get Scheduled Price Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetScheduledPrice( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCGetScheduledPrice_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[5];
  ZStatus_t status;

  osal_buffer_uint32( buf, pCmd->startTime );
  buf[4] = pCmd->numEvents;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_GET_SCHEDULED_PRICE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, 5, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishPrice
 *
 * @brief   Call to send out a Publish Price Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishPrice( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCPublishPrice_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_PUBLISH_PRICE + pCmd->rateLabel.strLen;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->providerId );
  *pBuf++ = pCmd->rateLabel.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->rateLabel.pStr, pCmd->rateLabel.strLen );
  pBuf = osal_buffer_uint32( pBuf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->currentTime );
  *pBuf++ = pCmd->unitOfMeasure;
  *pBuf++ = LO_UINT16( pCmd->currency );
  *pBuf++ = HI_UINT16( pCmd->currency );
  *pBuf++ = pCmd->priceTrailingDigit;
  *pBuf++ = pCmd->numberOfPriceTiers;
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  *pBuf++ = LO_UINT16( pCmd->durationInMinutes );
  *pBuf++ = HI_UINT16( pCmd->durationInMinutes );
  pBuf = osal_buffer_uint32( pBuf, pCmd->price );
  *pBuf++ = pCmd->priceRatio;
  pBuf = osal_buffer_uint32( pBuf, pCmd->generationPrice );
  *pBuf++ = pCmd->generationPriceRatio;
  pBuf = osal_buffer_uint32( pBuf, pCmd->alternateCostDelivered );
  *pBuf++ = pCmd->alternateCostUnit;
  *pBuf++ = pCmd->alternateCostTrailingDigit;
  *pBuf++ = pCmd->numberOfBlockThresholds;
  *pBuf = pCmd->priceControl;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_PUBLISH_PRICE, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PriceAcknowledgement
 *
 * @brief   Call to send out a Price Acknowledgement
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PriceAcknowledgement( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCPriceAcknowledgement_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;

  buf = osal_mem_alloc( PACKET_LEN_SE_PRICE_ACKNOWLEDGEMENT );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->providerId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->priceAckTime );
  *pBuf = pCmd->control;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_PRICE_ACKNOWLEDGEMENT, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_PRICE_ACKNOWLEDGEMENT, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetBlockPeriod
 *
 * @brief   Call to send out a Get Block Period
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetBlockPeriod( uint8 srcEP, afAddrType_t *dstAddr,
                                             zclCCGetBlockPeriod_t *pCmd,
                                             uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[5];
  ZStatus_t status;

  osal_buffer_uint32( buf, pCmd->startTime );
  buf[4] = pCmd->numEvents;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_GET_BLOCK_PERIOD, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, 5, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishBlockPeriod
 *
 * @brief   Call to send out a Publish Block Period
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishBlockPeriod( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCPublishBlockPeriod_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_PUBLISH_BLOCK_PERIOD;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->providerId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->blockPeriodStartTime );
  pBuf = osal_buffer_uint24( pBuf, pCmd->blockPeriodDurInMins );
  *pBuf++ = pCmd->numPriceTiersAndBlock;
#ifdef SE_UK_EXT
  *pBuf++ = pCmd->tariffType;
#endif
  *pBuf = pCmd->blockPeriodControl;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_PUBLISH_BLOCK_PERIOD, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishTariffInformation
 *
 * @brief   Call to send out a Publish Tariff Information
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishTariffInformation( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCPublishTariffInformation_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_MIN_PUBLISH_TARIFF_INFORMATION + pCmd->tarifLabel.strLen;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->supplierId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->issuerTariffId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  *pBuf++ = pCmd->tariffType;
  *pBuf++ = pCmd->tarifLabel.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->tarifLabel.pStr, pCmd->tarifLabel.strLen );
  *pBuf++ = pCmd->numPriceTiersInUse;
  *pBuf++ = pCmd->numBlockThresholdsInUse;
  *pBuf++ = pCmd->unitOfMeasure;
  *pBuf++ = LO_UINT16( pCmd->currency );
  *pBuf++ = HI_UINT16( pCmd->currency );
  *pBuf++ = pCmd->priceTrailingDigit;
  pBuf = osal_buffer_uint32( pBuf, pCmd->standingCharge );
  *pBuf++ = pCmd->tierBlockMode;
  *pBuf++ = LO_UINT16( pCmd->blockThresholdMask );
  *pBuf++ = HI_UINT16( pCmd->blockThresholdMask );
  pBuf = osal_buffer_uint24( pBuf, pCmd->BlockThresholdMultiplier );
  pBuf = osal_buffer_uint24( pBuf, pCmd->BlockThresholdDivisor );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_PUBLISH_TARIFF_INFO, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishPriceMatrix
 *
 * @brief   Call to send out a Publish Price Matrix
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishPriceMatrix( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishPriceMatrix_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;
  uint8 i;

  bufLen = PACKET_LEN_SE_MIN_PUBLISH_PRICE_MATRIX + (pCmd->numElements * sizeof(uint32));
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerTariffId );
  *pBuf++ = pCmd->commandIndex;

  for ( i = 0; i < pCmd->numElements; i++ )
  {
    pBuf = osal_buffer_uint32( pBuf, pCmd->pTierBlockPrice[i] );
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_PUBLISH_PRICE_MATRIX, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishBlockThresholds
 *
 * @brief   Call to send out a Publish Block Thresholds
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishBlockThresholds( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishBlockThresholds_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;
  uint8 i;

  bufLen = PACKET_LEN_SE_MIN_PUBLISH_BLOCK_THRESHOLD + (pCmd->numElements * 6);
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerTariffId );
  *pBuf++ = pCmd->commandIndex;

  for ( i = 0; i < pCmd->numElements; i++ )
  {
    pBuf = osal_memcpy( pBuf, pCmd->pTierBlockThreshold[i], 6 );
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_PUBLISH_BLOCK_THRESHOLD, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishConversionFactor
 *
 * @brief   Call to send out a Publish Conversion Factor
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishConversionFactor( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishConversionFactor_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_PUBLISH_CONVERSION_FACTOR];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  pBuf = osal_buffer_uint32( pBuf, pCmd->conversionFactor );
  *pBuf = pCmd->trailingDigit;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_PUBLISH_CONVERSION_FACTOR, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_PUBLISH_CONVERSION_FACTOR, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishCalorificValue
 *
 * @brief   Call to send out a Publish Calorific Value
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishCalorificValue( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishCalorificValue_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_PUBLISH_CALORIFIC_VALUE];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  pBuf = osal_buffer_uint32( pBuf, pCmd->calorificValue );
  *pBuf++ = pCmd->calorificValueUnit;
  *pBuf = pCmd->trailingDigit;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_PUBLISH_CALORIFIC_VALUE, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_PUBLISH_CALORIFIC_VALUE, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishCO2Value
 *
 * @brief   Call to send out a Publish CO2 Value
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishCO2Value( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishCO2Value_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_PUBLISH_CO2_VALUE];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  *pBuf++ = pCmd->tariffType;
  pBuf = osal_buffer_uint32( pBuf, pCmd->CO2Value );
  *pBuf++ = pCmd->CO2ValueUnit;
  *pBuf = pCmd->trailingDigit;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_PUBLISH_CO2_VALUE, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_PUBLISH_CO2_VALUE, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishCPPEvent
 *
 * @brief   Call to send out a Publish CPP Event
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishCPPEvent( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishCPPEvent_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_PUBLISH_CPP_EVENT];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  *pBuf++ = LO_UINT16( pCmd->durationInMinutes );
  *pBuf++ = HI_UINT16( pCmd->durationInMinutes );
  *pBuf++ = pCmd->tariffType;
  *pBuf++ = pCmd->CPPPriceTier;
  *pBuf = pCmd->CPPAuth;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_PUBLISH_CPP_EVENT, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_PUBLISH_CPP_EVENT, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishBillingPeriod
 *
 * @brief   Call to send out a Publish Billing Period
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishBillingPeriod( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishBillingPeriod_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_PUBLISH_BILLING_PERIOD];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  pBuf = osal_buffer_uint24( pBuf, pCmd->duration );
  *pBuf = pCmd->tariffType;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_PUBLISH_BILLING_PERIOD, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_PUBLISH_BILLING_PERIOD, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishConsolidatedBill
 *
 * @brief   Call to send out a Publish Consolidated Bill
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishConsolidatedBill( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishConsolidatedBill_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_PUBLISH_CONSOLIDATED_BILL];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  pBuf = osal_buffer_uint24( pBuf, pCmd->duration );
  *pBuf++ = pCmd->tariffType;
  pBuf = osal_buffer_uint32( pBuf, pCmd->consolidatedBill );
  *pBuf++ = LO_UINT16( pCmd->currency );
  *pBuf++ = HI_UINT16( pCmd->currency );
  *pBuf = pCmd->trailingDigit;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_PUBLISH_CONSOLIDATED_BILL, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_PUBLISH_CONSOLIDATED_BILL, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_PublishCreditPaymentInfo
 *
 * @brief   Call to send out a Publish Credit Payment Info
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_PublishCreditPaymentInfo( uint8 srcEP, afAddrType_t *dstAddr,
                                                 zclCCPublishCreditPaymentInfo_t *pCmd,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_MIN_PUBLISH_CREDIT_PAYMENT_INFO + pCmd->creditPaymentRef.strLen;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->paymentDueDate );
  pBuf = osal_buffer_uint32( pBuf, pCmd->creditPaymentOverdueAmt );
  *pBuf++ = pCmd->creditPaymentStatus;
  pBuf = osal_buffer_uint32( pBuf, pCmd->creditPayment );
  pBuf = osal_buffer_uint32( pBuf, pCmd->creditPaymentDate );
  *pBuf++ = pCmd->creditPaymentRef.strLen;
  osal_memcpy( pBuf, pCmd->creditPaymentRef.pStr, pCmd->creditPaymentRef.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                            COMMAND_SE_PUBLISH_CREDIT_PAYMENT_INFO, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetTariffInformation
 *
 * @brief   Call to send out a Get Tariff Information
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetTariffInformation( uint8 srcEP, afAddrType_t *dstAddr,
                                                   zclCCGetTariffInformation_t *pCmd,
                                                   uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_TARIFF_INFO];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->startTime );
  *pBuf++ = pCmd->numEvents;
  *pBuf = pCmd->tariffType;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_GET_TARIFF_INFO, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_TARIFF_INFO, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetPriceMatrix
 *
 * @brief   Call to send out a Get Price Matrix
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   issuerId - Issuer ID
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetPriceMatrix( uint8 srcEP, afAddrType_t *dstAddr,
                                             uint32 issuerId,
                                             uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_PRICE_MATRIX];

  osal_buffer_uint32( buf, issuerId );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_GET_PRICE_MATRIX, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_PRICE_MATRIX, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetBlockThresholds
 *
 * @brief   Call to send out a Get Block Thresholds
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   issuerId - Issuer ID
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetBlockThresholds( uint8 srcEP, afAddrType_t *dstAddr,
                                                 uint32 issuerId,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_BLOCK_THRESHOLD];

  osal_buffer_uint32( buf, issuerId );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_GET_BLOCK_THRESHOLD, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_BLOCK_THRESHOLD, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetConversionFactor
 *
 * @brief   Call to send out a Get Conversion Factor
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetConversionFactor( uint8 srcEP, afAddrType_t *dstAddr,
                                                  zclCCGetConversionFactor_t *pCmd,
                                                  uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_CONVERSION_FACTOR];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->startTime );
  *pBuf = pCmd->numEvents;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_GET_CONVERSION_FACTOR, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_CONVERSION_FACTOR, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetCalorificValue
 *
 * @brief   Call to send out a Get Calorific Value
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetCalorificValue( uint8 srcEP, afAddrType_t *dstAddr,
                                                zclCCGetCalorificValue_t *pCmd,
                                                uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_CALORIFIC_VALUE];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->startTime );
  *pBuf = pCmd->numEvents;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_GET_CALORIFIC_VALUE, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_CALORIFIC_VALUE, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetCO2Value
 *
 * @brief   Call to send out a Get CO2 Value
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetCO2Value( uint8 srcEP, afAddrType_t *dstAddr,
                                                zclCCGetCO2Value_t *pCmd,
                                                uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_CO2_VALUE];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->startTime );
  *pBuf++ = pCmd->numEvents;
  *pBuf = pCmd->tariffType;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_GET_CO2_VALUE, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_CO2_VALUE, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetBillingPeriod
 *
 * @brief   Call to send out a Get Billing Period
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetBillingPeriod( uint8 srcEP, afAddrType_t *dstAddr,
                                               zclCCGetBillingPeriod_t *pCmd,
                                               uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_BILLING_PERIOD];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->startTime );
  *pBuf++ = pCmd->numEvents;
  *pBuf = pCmd->tariffType;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_GET_BILLING_PERIOD, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_BILLING_PERIOD, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_GetConsolidatedBill
 *
 * @brief   Call to send out a Get Consolidated Bill
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_GetConsolidatedBill( uint8 srcEP, afAddrType_t *dstAddr,
                                                zclCCGetConsolidatedBill_t *pCmd,
                                                uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_CONSOLIDATED_BILL];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->startTime );
  *pBuf++ = pCmd->numEvents;
  *pBuf = pCmd->tariffType;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_GET_CONSOLIDATED_BILL, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_CONSOLIDATED_BILL, buf );
}

/*********************************************************************
 * @fn      zclSE_Pricing_Send_CPPEventResponse
 *
 * @brief   Call to send out a CPP Event Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Pricing_Send_CPPEventResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                               zclCCCPPEventResponse_t *pCmd,
                                               uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_CPP_EVENT_RESPONSE];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  *pBuf = pCmd->CPPAuth;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PRICING,
                          COMMAND_SE_CPP_EVENT_RESPONSE, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_CPP_EVENT_RESPONSE, buf );
}
#endif  // SE_UK_EXT
#endif  // ZCL_PRICING


#ifdef ZCL_MESSAGE
/*********************************************************************
 * @fn      zclSE_Message_Send_DisplayMessage
 *
 * @brief   Call to send out a Display Message Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Message_Send_DisplayMessage( uint8 srcEP, afAddrType_t *dstAddr,
                                             zclCCDisplayMessage_t *pCmd,
                                             uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  // msgId + msgCtrl + start time + duration + msgLen + msg
  bufLen = 4 + 1 + 4 + 2 + 1 + pCmd->msgString.strLen;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->messageId );  // Streamline the uint32 data
  *pBuf++ = pCmd->messageCtrl.transmissionMode |
           (pCmd->messageCtrl.importance << SE_PROFILE_MSGCTRL_IMPORTANCE) |
#if defined ( SE_UK_EXT )
           (pCmd->messageCtrl.pinRequired << SE_PROFILE_MSGCTRL_PINREQUIRED ) |
           (pCmd->messageCtrl.acceptanceRequired << SE_PROFILE_MSGCTRL_ACCEPTREQUIRED ) |
#endif
           (pCmd->messageCtrl.confirmationRequired << SE_PROFILE_MSGCTRL_CONFREQUIRED);
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  *pBuf++ = LO_UINT16( pCmd->durationInMinutes );
  *pBuf++ = HI_UINT16( pCmd->durationInMinutes );
  *pBuf++ = pCmd->msgString.strLen;

  osal_memcpy( pBuf, pCmd->msgString.pStr, pCmd->msgString.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_MESSAGE,
                            COMMAND_SE_DISPLAY_MESSAGE, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp,
                            0, seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Message_Send_CancelMessage
 *
 * @brief   Call to send out a Cancel Message Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Message_Send_CancelMessage( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCCancelMessage_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[5];

  osal_buffer_uint32( buf, pCmd->messageId );  // Streamline the uint32 data
  buf[4] = pCmd->messageCtrl.transmissionMode |
           (pCmd->messageCtrl.importance << SE_PROFILE_MSGCTRL_IMPORTANCE) |
#if defined ( SE_UK_EXT )
           (pCmd->messageCtrl.pinRequired << SE_PROFILE_MSGCTRL_PINREQUIRED ) |
           (pCmd->messageCtrl.acceptanceRequired << SE_PROFILE_MSGCTRL_ACCEPTREQUIRED ) |
#endif
           (pCmd->messageCtrl.confirmationRequired << SE_PROFILE_MSGCTRL_CONFREQUIRED);

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_MESSAGE,
                          COMMAND_SE_CANCEL_MESSAGE, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp,
                          0, seqNum, 5, buf );
}

/*********************************************************************
 * @fn      zclSE_Message_Send_MessageConfirmation
 *
 * @brief   Call to send out a Message Confirmation
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Message_Send_MessageConfirmation( uint8 srcEP, afAddrType_t *dstAddr,
                                                  zclCCMessageConfirmation_t *pCmd,
                                                  uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen = 4 + 4; // msgId + confirm time
  ZStatus_t status;

#if defined ( SE_UK_EXT )
  // Message Response length must be 0 - 20 octets
  if ( pCmd->msgString.strLen > SE_PROFILE_MESSAGE_RESPONSE_LENGTH )
  {
    return (ZInvalidParameter);
  }

  // msgLen + msg
  bufLen += 1 + pCmd->msgString.strLen;
#endif

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->messageId );  // Streamline the uint32 data
  pBuf = osal_buffer_uint32( pBuf, pCmd->confirmTime );

#if defined ( SE_UK_EXT )
  *pBuf++ = pCmd->msgString.strLen;
  osal_memcpy( pBuf, pCmd->msgString.pStr, pCmd->msgString.strLen );
#endif

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_MESSAGE,
                            COMMAND_SE_MESSAGE_CONFIRMATION, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp,
                            0, seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}
#endif  // ZCL_MESSAGE

#ifdef ZCL_LOAD_CONTROL
/*********************************************************************
 * @fn      zclSE_LoadControl_Send_LoadControlEvent
 *
 * @brief   Call to send out a Load Control Event
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_LoadControl_Send_LoadControlEvent( uint8 srcEP, afAddrType_t *dstAddr,
                                                      zclCCLoadControlEvent_t *pCmd,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;

  buf = osal_mem_alloc( PACKET_LEN_SE_LOAD_CONTROL_EVENT );

  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEvent );
  pBuf = osal_buffer_uint24( pBuf, pCmd->deviceGroupClass );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  *pBuf++ = LO_UINT16( pCmd->durationInMinutes );
  *pBuf++ = HI_UINT16( pCmd->durationInMinutes );
  *pBuf++ = pCmd->criticalityLevel;
  *pBuf++ = pCmd->coolingTemperatureOffset;
  *pBuf++ = pCmd->heatingTemperatureOffset;
  *pBuf++ = LO_UINT16( pCmd->coolingTemperatureSetPoint );
  *pBuf++ = HI_UINT16( pCmd->coolingTemperatureSetPoint );
  *pBuf++ = LO_UINT16( pCmd->heatingTemperatureSetPoint );
  *pBuf++ = HI_UINT16( pCmd->heatingTemperatureSetPoint );
  *pBuf++ = pCmd->averageLoadAdjustmentPercentage;
  *pBuf++ = pCmd->dutyCycle;
  *pBuf = pCmd->eventControl;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
                            COMMAND_SE_LOAD_CONTROL_EVENT, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0, seqNum,
                            PACKET_LEN_SE_LOAD_CONTROL_EVENT, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_LoadControl_Send_CancelLoadControlEvent
 *
 * @brief   Call to send out a Cancel Load Control Event
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_LoadControl_Send_CancelLoadControlEvent( uint8 srcEP, afAddrType_t *dstAddr,
                                                      zclCCCancelLoadControlEvent_t *pCmd,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_CANCEL_LOAD_CONTROL_EVENT];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventID );
  pBuf = osal_buffer_uint24( pBuf, pCmd->deviceGroupClass );
  *pBuf++ = pCmd->cancelControl;
  pBuf = osal_buffer_uint32( pBuf, pCmd->effectiveTime );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
                          COMMAND_SE_CANCEL_LOAD_CONTROL_EVENT, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0, seqNum,
                          PACKET_LEN_SE_CANCEL_LOAD_CONTROL_EVENT, buf );
}

/*********************************************************************
 * @fn      zclSE_LoadControl_Send_ReportEventStatus
 *
 * @brief   Call to send out a Report Event Status
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_LoadControl_Send_ReportEventStatus( uint8 srcEP, afAddrType_t *dstAddr,
                                                      zclCCReportEventStatus_t *pCmd,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;

  buf = osal_mem_alloc( PACKET_LEN_SE_REPORT_EVENT_STATUS );

  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventID );
  *pBuf++ = pCmd->eventStatus;
  pBuf = osal_buffer_uint32( pBuf, pCmd->eventStartTime );
  *pBuf++ = pCmd->criticalityLevelApplied;
  *pBuf++ = LO_UINT16( pCmd->coolingTemperatureSetPointApplied );
  *pBuf++ = HI_UINT16( pCmd->coolingTemperatureSetPointApplied );
  *pBuf++ = LO_UINT16( pCmd->heatingTemperatureSetPointApplied );
  *pBuf++ = HI_UINT16( pCmd->heatingTemperatureSetPointApplied );
  *pBuf++ = pCmd->averageLoadAdjustment;
  *pBuf++ = pCmd->dutyCycleApplied;
  *pBuf++ = pCmd->eventControl;
  *pBuf++ = pCmd->signatureType;

  zclGeneral_KeyEstablishment_ECDSASign( buf, PACKET_LEN_SE_REPORT_EVENT_STATUS_ONLY, pBuf);

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
                            COMMAND_SE_REPORT_EVENT_STATUS, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum,
                            PACKET_LEN_SE_REPORT_EVENT_STATUS, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_LoadControl_Send_GetScheduledEvent
 *
 * @brief   Call to send out a Get Scheduled Event
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_LoadControl_Send_GetScheduledEvent( uint8 srcEP, afAddrType_t *dstAddr,
                                                      zclCCGetScheduledEvent_t *pCmd,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_SCHEDULED_EVENT];

  osal_buffer_uint32( buf, pCmd->startTime );
  buf[4] = pCmd->numEvents;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_LOAD_CONTROL,
                          COMMAND_SE_GET_SCHEDULED_EVENT, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0, seqNum,
                          PACKET_LEN_SE_GET_SCHEDULED_EVENT, buf );
}
#endif  // ZCL_LOAD_CONTROL

#ifdef ZCL_PREPAYMENT
/*********************************************************************
 * @fn      zclSE_Prepayment_Send_SelAvailEmergencyCredit
 *
 * @brief   Call to send out a Select Available Emergency Credit command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_SelAvailEmergencyCredit( uint8 srcEP,
                                                         afAddrType_t *dstAddr,
                                                         zclCCSelAvailEmergencyCredit_t *pCmd,
                                                         uint8 disableDefaultRsp,
                                                         uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 bufLen;
  ZStatus_t status;

  // include in length all variable length fields
  bufLen = PACKET_LEN_SE_SEL_AVAIL_EMERGENCY_CREDIT +
           pCmd->siteId.strLen +
           pCmd->meterSerialNumber.strLen;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->commandDateTime );
  *pBuf++ = pCmd->originatingDevice;
  *pBuf++ = pCmd->siteId.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->siteId.pStr, pCmd->siteId.strLen );
  *pBuf++ = pCmd->meterSerialNumber.strLen;
  osal_memcpy( pBuf, pCmd->meterSerialNumber.pStr, pCmd->meterSerialNumber.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_SEL_AVAIL_EMERGENCY_CREDIT, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

#ifndef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_Prepayment_Send_ChangeSupply
 *
 * @brief   Call to send out a Change Supply command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_ChangeSupply( uint8 srcEP, afAddrType_t *dstAddr,
                                              zclCCChangeSupply_t *pCmd,
                                              uint8 disableDefaultRsp,
                                              uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 bufLen;
  ZStatus_t status;

  // include in length all variable length fields
  bufLen = PACKET_LEN_SE_CHANGE_SUPPLY +
           pCmd->siteId.strLen +
           pCmd->meterSerialNumber.strLen;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->providerId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->requestDateTime );
  *pBuf++ = pCmd->siteId.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->siteId.pStr, pCmd->siteId.strLen );
  *pBuf++ = pCmd->meterSerialNumber.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->meterSerialNumber.pStr, pCmd->meterSerialNumber.strLen );
  pBuf = osal_buffer_uint32( pBuf, pCmd->implementationDateTime );
  *pBuf++ = pCmd->proposedSupplyStatus;
  *pBuf = pCmd->origIdSupplyControlBits;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_CHANGE_SUPPLY, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_SupplyStatusResponse
 *
 * @brief   Call to send out a Supply Status Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_SupplyStatusResponse( uint8 srcEP,
                                                      afAddrType_t *dstAddr,
                                                      zclCCSupplyStatusResponse_t *pCmd,
                                                      uint8 disableDefaultRsp,
                                                      uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;

  buf = osal_mem_alloc( PACKET_LEN_SE_SUPPLY_STATUS_RESPONSE );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->providerId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->implementationDateTime );
  *pBuf = pCmd->supplyStatus;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_SUPPLY_STATUS_RESPONSE, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_SUPPLY_STATUS_RESPONSE, buf );

  osal_mem_free( buf );

  return status;
}

#else // SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_Prepayment_Send_ChangeDebt
 *
 * @brief   Call to send out a Change Debt
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_ChangeDebt( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCChangeDebt_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;
  uint8 bufLen = PACKET_LEN_SE_MIN_CHANGE_DEBT + pCmd->debtLabel.strLen + pCmd->signature.strLen;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->cmdIssueTime );
  *pBuf++ = pCmd->debtLabel.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->debtLabel.pStr, pCmd->debtLabel.strLen );
  pBuf = osal_buffer_uint32( pBuf, pCmd->debtAmount );
  *pBuf++ = pCmd->debtRecoveryMethod;
  *pBuf++ = pCmd->debtType;
  pBuf = osal_buffer_uint32( pBuf, pCmd->recoveryStartTime );
  *pBuf++ = LO_UINT16( pCmd->debtRecoveryCollectionTime );
  *pBuf++ = HI_UINT16( pCmd->debtRecoveryCollectionTime );
  pBuf = osal_buffer_uint32( pBuf, pCmd->debtRecoveryFrequency );
  pBuf = osal_buffer_uint32( pBuf, pCmd->debtRecoveryAmt );
  *pBuf++ = LO_UINT16( pCmd->debtRecoveryBalancePct );
  *pBuf++ = HI_UINT16( pCmd->debtRecoveryBalancePct );
  *pBuf++ = pCmd->debtRecoveryMaxMissed;
  *pBuf++ = pCmd->signature.strLen;
  (void)osal_memcpy( pBuf, pCmd->signature.pStr, pCmd->signature.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_CHANGE_DEBT, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_EmergencyCreditSetup
 *
 * @brief   Call to send out a Emergency Credit Setup
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_EmergencyCreditSetup( uint8 srcEP, afAddrType_t *dstAddr,
                                                      zclCCEmergencyCreditSetup_t *pCmd,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_EMERGENCY_CREDIT_SETUP];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->cmdIssueTime );
  pBuf = osal_buffer_uint32( pBuf, pCmd->emergencyCreditLimit );
  pBuf = osal_buffer_uint32( pBuf, pCmd->emergencyCreditThreshold );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_EMERGENCY_CREDIT_SETUP, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_EMERGENCY_CREDIT_SETUP, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_ConsumerTopup
 *
 * @brief   Call to send out a Consumer Topup
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_ConsumerTopup( uint8 srcEP, afAddrType_t *dstAddr,
                                               zclCCConsumerTopup_t *pCmd,
                                               uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;
  uint8 bufLen = PACKET_LEN_SE_MIN_CONSUMER_TOPUP + pCmd->topupCode.strLen;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = buf;

  *pBuf++ = pCmd->originatingDevice;
  *pBuf++ = pCmd->topupCode.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->topupCode.pStr, pCmd->topupCode.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_CONSUMER_TOPUP, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_CreditAdjustment
 *
 * @brief   Call to send out a Credit Adjustment
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_CreditAdjustment( uint8 srcEP, afAddrType_t *dstAddr,
                                                  zclCCCreditAdjustment_t *pCmd,
                                                  uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;
  uint8 bufLen = PACKET_LEN_SE_MIN_CREDIT_ADJUSTMENT +  pCmd->signature.strLen;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->cmdIssueTime );
  *pBuf++ = pCmd->creditAdjustmentType;
  pBuf = osal_memcpy( pBuf, pCmd->creditAdjustmentValue, 6 );
  *pBuf++ = pCmd->signature.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->signature.pStr, pCmd->signature.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_CREDIT_ADJUSTMENT, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_ChangePaymentMode
 *
 * @brief   Call to send out a Change Payment Mode
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_ChangePaymentMode( uint8 srcEP, afAddrType_t *dstAddr,
                                                   zclCCChangePaymentMode_t *pCmd,
                                                   uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;
  uint8 bufLen = PACKET_LEN_SE_MIN_CHANGE_PAYMENT_MODE +  pCmd->signature.strLen;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->supplierId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->modeEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->implementationDate );
  *pBuf++ = pCmd->proposedPaymentControl;
  pBuf = osal_buffer_uint32( pBuf, pCmd->cutOffValue );
  *pBuf++ = pCmd->signature.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->signature.pStr, pCmd->signature.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_CHANGE_PAYMENT_MODE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_GetPrepaySnapshot
 *
 * @brief   Call to send out a Get Prepay Snapshot
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_GetPrepaySnapshot( uint8 srcEP, afAddrType_t *dstAddr,
                                                   zclCCGetPrepaySnapshot_t *pCmd,
                                                   uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_PREPAY_SNAPSHOT];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->startTime );
  *pBuf++ = pCmd->numberOfSnapshots;
  *pBuf++ = LO_UINT16( pCmd->snapshotCause );
  *pBuf   = HI_UINT16( pCmd->snapshotCause );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_GET_PREPAY_SNAPSHOT, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_PREPAY_SNAPSHOT, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_GetTopupLog
 *
 * @brief   Call to send out a Get Topup Log
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   numEvents - number of events
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_GetTopupLog( uint8 srcEP, afAddrType_t *dstAddr,
                                             uint8 numEvents, uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_TOPUP_LOG];

  buf[0] = numEvents;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_GET_TOPUP_LOG, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_TOPUP_LOG, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_SetLowCreditWarningLevel
 *
 * @brief   Call to send out a Set Low Credit Warning Level
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   warningLevel - warning level
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_SetLowCreditWarningLevel( uint8 srcEP, afAddrType_t *dstAddr,
                                                          uint8 warningLevel,
                                                          uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_SET_LOW_CREDIT_WARNING_LEVEL];

  buf[0] = warningLevel;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_SET_LOW_CREDIT_WARNING_LEVEL, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_SET_LOW_CREDIT_WARNING_LEVEL, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_GetDebtRepaymentLog
 *
 * @brief   Call to send out a Get Debt Repayment Log
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_GetDebtRepaymentLog( uint8 srcEP, afAddrType_t *dstAddr,
                                                     zclCCGetDebtRepaymentLog_t *pCmd,
                                                     uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_DEBT_REPAYMENT_LOG];

  buf[0] = pCmd->numberOfDebt;
  buf[1] = pCmd->debtType;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_GET_DEBT_REPAYMENT_LOG, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_DEBT_REPAYMENT_LOG, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_GetPrepaySnapshotResponse
 *
 * @brief   Call to send out a Get Prepay Snapshot Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_GetPrepaySnapshotResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                                           zclCCGetPrepaySnapshotResponse_t *pCmd,
                                                           uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_PREPAY_SNAPSHOT_RESPONSE];
  uint8 *pBuf;

  pBuf = osal_buffer_uint32( buf, pCmd->eventIssuerId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->snapshotTime );
  *pBuf++ = pCmd->commandIndex;
  *pBuf++ = LO_UINT16( pCmd->snapshotCause );
  *pBuf++ = HI_UINT16( pCmd->snapshotCause );
  *pBuf++ = pCmd->snapshotPayloadType;

  pBuf = osal_buffer_uint32( pBuf, pCmd->payload.type1DebtRemaining );
  pBuf = osal_buffer_uint32( pBuf, pCmd->payload.type2DebtRemaining );
  pBuf = osal_buffer_uint32( pBuf, pCmd->payload.type3DebtRemaining );
  pBuf = osal_buffer_uint32( pBuf, pCmd->payload.emergencyCreditRemaining );
  pBuf = osal_buffer_uint32( pBuf, pCmd->payload.creditRemaining );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_GET_PREPAY_SNAPSHOT_RESPONSE, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_PREPAY_SNAPSHOT_RESPONSE, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_ChangePaymentModeResponse
 *
 * @brief   Call to send out a Change Payment Mode Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_ChangePaymentModeResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                                           zclCCChangePaymentModeResponse_t *pCmd,
                                                           uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_CHANGE_PAYMENT_MODE_RESPONSE];
  uint8 *pBuf = buf;

  *pBuf++ = pCmd->friendlyCredit;
  pBuf = osal_buffer_uint32( pBuf, pCmd->friendlyCreditCalendar );
  pBuf = osal_buffer_uint32( pBuf, pCmd->emergencyCreditLimit );
  pBuf = osal_buffer_uint32( pBuf, pCmd->cmergencyCreditThreshold );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_CHANGE_PAYMENT_MODE_RESPONSE, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_CHANGE_PAYMENT_MODE_RESPONSE, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_ConsumerTopupResponse
 *
 * @brief   Call to send out a Consumer Topup Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_ConsumerTopupResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                                       zclCCConsumerTopupResponse_t *pCmd,
                                                       uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_SE_CONSUMER_TOPUP_RESPONSE];
  uint8 *pBuf = buf;

  *pBuf++ = pCmd->resultType;
  pBuf = osal_buffer_uint32( pBuf, pCmd->topupValue );
  *pBuf++ = pCmd->sourceofTopup;
  pBuf = osal_buffer_uint32( pBuf, pCmd->creditRemaining );

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_CONSUMER_TOPUP_RESPONSE, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_SE_CONSUMER_TOPUP_RESPONSE, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_GetCommands
 *
 * @brief   Call to send out a Get Commands
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   notificationFlags - notification flags
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_GetCommands( uint8 srcEP, afAddrType_t *dstAddr,
                                             uint8 notificationFlags, uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_COMMANDS];

  buf[0] = notificationFlags;

  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                          COMMAND_SE_GET_COMMANDS, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_COMMANDS, buf );
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_PublishTopupLog
 *
 * @brief   Call to send out a Publish Topup Log
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_PublishTopupLog( uint8 srcEP, afAddrType_t *dstAddr,
                                                zclCCPublishTopupLog_t *pCmd,
                                                uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;
  uint8 bufLen = PACKET_LEN_SE_MIN_PUBLISH_TOPUP_LOG + (pCmd->numCodes * SE_TOPUP_CODE_LEN);
  uint8 i;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = buf;

  *pBuf++ = pCmd->cmdIndex;
  *pBuf++ = pCmd->totalCmds;

  bufLen = 2;

  for ( i = 0; i < pCmd->numCodes; i++ )
  {
    *pBuf++ = pCmd->pPayload[i].strLen;
    pBuf = osal_memcpy( pBuf, pCmd->pPayload[i].pStr, pCmd->pPayload[i].strLen );
    bufLen += 1 + pCmd->pPayload[i].strLen;
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_PUBLISH_TOPUP_LOG, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Prepayment_Send_PublishDebtLog
 *
 * @brief   Call to send out a Publish Debt Log
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Prepayment_Send_PublishDebtLog( uint8 srcEP, afAddrType_t *dstAddr,
                                                zclCCPublishDebtLog_t *pCmd,
                                                uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;
  uint8 bufLen = PACKET_LEN_SE_MIN_PUBLISH_DEBT_LOG + (pCmd->numDebts * sizeof(zclCCDebtPayload_t));
  uint8 i;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = buf;

  *pBuf++ = pCmd->cmdIndex;
  *pBuf++ = pCmd->totalCmds;

  for ( i = 0; i < pCmd->numDebts; i++ )
  {
    pBuf = osal_buffer_uint32( pBuf, pCmd->pPayload[i].collectionTime );
    pBuf = osal_buffer_uint32( pBuf, pCmd->pPayload[i].amountCollected );
    *pBuf++ = pCmd->pPayload[i].debtType;
    pBuf = osal_buffer_uint32( pBuf, pCmd->pPayload[i].outstandingDebt );
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_PREPAYMENT,
                            COMMAND_SE_PUBLISH_DEBT_LOG, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}
#endif  //SE_UK_EXT
#endif  // ZCL_PREPAYMENT

#ifdef ZCL_TUNNELING
/*********************************************************************
 * @fn      zclSE_Tunneling_Send_RequestTunnel
 *
 * @brief   Call to send out a Request Tunnel
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_RequestTunnel( uint8 srcEP, afAddrType_t *dstAddr,
                                              zclCCRequestTunnel_t  *pCmd,
                                              uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;

  buf = osal_mem_alloc( PACKET_LEN_SE_TUNNELING_REQUEST );
  if ( buf == NULL )
  {
    return ZMemError;
  }

  pBuf = buf;
  *pBuf++ = pCmd->protocolId;
  *pBuf++ = LO_UINT16( pCmd->manufacturerCode );
  *pBuf++ = HI_UINT16( pCmd->manufacturerCode );
  *pBuf++ = pCmd->flowControlSupport;
  *pBuf++ = LO_UINT16( pCmd->maxInTransferSize );
  *pBuf   = HI_UINT16( pCmd->maxInTransferSize );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            COMMAND_SE_REQUEST_TUNNEL, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_TUNNELING_REQUEST, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tunneling_Send_ReqTunnelRsp
 *
 * @brief   Call to send out a Request Tunnel Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
extern ZStatus_t zclSE_Tunneling_Send_ReqTunnelRsp( uint8 srcEP, afAddrType_t *dstAddr,
                                                    zclCCReqTunnelRsp_t *pCmd,
                                                    uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;

  buf = osal_mem_alloc( PACKET_LEN_SE_TUNNELING_RESPONSE );
  if ( buf == NULL )
  {
    return ZMemError;
  }

  pBuf = buf;
  *pBuf++ = LO_UINT16( pCmd->tunnelId ) ;
  *pBuf++ = HI_UINT16( pCmd->tunnelId );
  *pBuf++ = pCmd->tunnelStatus;
  *pBuf++ = LO_UINT16( pCmd->maxInTransferSize ) ;
  *pBuf   = HI_UINT16( pCmd->maxInTransferSize );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            COMMAND_SE_REQUEST_TUNNEL_RESPONSE, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_TUNNELING_RESPONSE, buf );

  osal_mem_free( buf );

  return status;

}

/*********************************************************************
 * @fn      zclSE_Tunneling_Send_CloseTunnel
 *
 * @brief   Call to send out a Close Tunnel
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_CloseTunnel( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCCloseTunnel_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_TUNNELING_CLOSE];
  ZStatus_t status;

  buf[0] = LO_UINT16( pCmd->tunnelId );
  buf[1] = HI_UINT16( pCmd->tunnelId );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            COMMAND_SE_CLOSE_TUNNEL, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_TUNNELING_CLOSE, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tunneling_Send_TransferData
 *
 * @brief   Call to send out a Transfer Data
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   cmdId - command identifier
 * @param   dataLen - length of transported data
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_TransferData( uint8 srcEP, afAddrType_t *dstAddr,
                                             zclCCTransferData_t *pCmd, uint8 cmdId,
                                             uint16 dataLen, uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 bufLen;
  ZStatus_t status;
  uint8 direction;

  bufLen = PACKET_LEN_SE_TUNNELING_TRANSFER_DATA + dataLen;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ZMemError;
  }

  pBuf = buf;
  *pBuf++ = LO_UINT16( pCmd->tunnelId );
  *pBuf++ = HI_UINT16( pCmd->tunnelId );

  if ( cmdId == COMMAND_SE_DATA_CLIENT_SERVER_DIR )
  {
    direction = ZCL_FRAME_CLIENT_SERVER_DIR;
  }
  else
  {
    direction = ZCL_FRAME_SERVER_CLIENT_DIR;
  }

  osal_memcpy( pBuf, pCmd->data, dataLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            cmdId, TRUE, direction, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tunneling_Send_TransferDataError
 *
 * @brief   Call to send out a Transfer Data Error
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   cmdId - command identifier
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_TransferDataError( uint8 srcEP, afAddrType_t *dstAddr,
                                                  zclCCTransferDataError_t *pCmd, uint8 cmdId,
                                                  uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_TUNNELING_DATA_ERROR];
  ZStatus_t status;
  uint8 direction;

  buf[0] = LO_UINT16( pCmd->tunnelId );
  buf[1] = HI_UINT16( pCmd->tunnelId );
  buf[2] = pCmd->transferDataStatus;

  if ( cmdId == COMMAND_SE_DATA_ERROR_CLIENT_SERVER_DIR)
  {
    direction = ZCL_FRAME_CLIENT_SERVER_DIR;
  }
  else
  {
    direction = ZCL_FRAME_SERVER_CLIENT_DIR;
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            cmdId, TRUE, direction, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_TUNNELING_DATA_ERROR, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tunneling_Send_AckTransferData
 *
 * @brief   Call to send out an Acknowledgment Transfer Data
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   cmdId - command identifier
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_AckTransferData( uint8 srcEP, afAddrType_t *dstAddr,
                                                zclCCAckTransferData_t *pCmd, uint8 cmdId,
                                                uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_TUNNELING_DATA_ACK];
  ZStatus_t status;
  uint8 direction;

  buf[0] = LO_UINT16( pCmd->tunnelId );
  buf[1] = HI_UINT16( pCmd->tunnelId );
  buf[2] = LO_UINT16( pCmd->numberOfBytesLeft );
  buf[3] = HI_UINT16( pCmd->numberOfBytesLeft );

  if ( cmdId == COMMAND_SE_ACK_SERVER_CLIENT_DIR )
  {
    direction = ZCL_FRAME_SERVER_CLIENT_DIR;
  }
  else
  {
    direction = ZCL_FRAME_CLIENT_SERVER_DIR;
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            cmdId, TRUE, direction, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_TUNNELING_DATA_ACK, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tunneling_Send_ReadyData
 *
 * @brief   Call to send out a Ready Data
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   cmdId - command identifier
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_ReadyData( uint8 srcEP, afAddrType_t *dstAddr,
                                          zclCCReadyData_t *pCmd, uint8 cmdId,
                                          uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_TUNNELING_READY_DATA];
  ZStatus_t status;
  uint8 direction;

  buf[0] = LO_UINT16( pCmd->tunnelId );
  buf[1] = HI_UINT16( pCmd->tunnelId );
  buf[2] = LO_UINT16( pCmd->numberOfOctetsLeft );
  buf[3] = HI_UINT16( pCmd->numberOfOctetsLeft );

  if ( cmdId == COMMAND_SE_READY_DATA_CLIENT_SERVER_DIR )
  {
    direction = ZCL_FRAME_CLIENT_SERVER_DIR;
  }
  else
  {
    direction = ZCL_FRAME_SERVER_CLIENT_DIR;
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            cmdId, TRUE, direction, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_TUNNELING_READY_DATA, buf );

  return status;
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_Tunneling_Send_GetSuppTunnelProt
 *
 * @brief   Call to send out a Get Supported Tunnel Protocols
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_GetSuppTunnelProt( uint8 srcEP, afAddrType_t *dstAddr,
                                                  zclCCGetSuppTunnProt_t *pCmd,
                                                  uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_TUNNELING_GET_SUPP_PROT];
  ZStatus_t status;

  buf[0] = pCmd->protocolOffset;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            COMMAND_SE_GET_SUPP_TUNNEL_PROTOCOLS, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_TUNNELING_GET_SUPP_PROT, buf );
  return status;
}

/*********************************************************************
 * @fn      zclSE_Tunneling_Send_SuppTunnelProtRsp
 *
 * @brief   Call to send out a Supported Tunnel Protocols Response
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_SuppTunnelProtRsp( uint8 srcEP, afAddrType_t *dstAddr,
                                                  zclCCSuppTunnProtRsp_t *pCmd,
                                                  uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint8 bufLen;
  uint8 i;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_TUNNELING_SUPP_PROT_RSP +
           (pCmd->protocolCount * PACKET_LEN_SE_TUNNELING_PROTOCOL_PAYLOAD);

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ZMemError;
  }

  pBuf = buf;
  *pBuf++ = pCmd->protocolListComp;
  *pBuf++ = pCmd->protocolCount;

  for ( i = 0; i < pCmd->protocolCount; i++ )
  {
    *pBuf++ = LO_UINT16( pCmd->protocol[i].manufacturerCode );
    *pBuf++ = HI_UINT16( pCmd->protocol[i].manufacturerCode );
    *pBuf++ = pCmd->protocol[i].protocolId;
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            COMMAND_SE_SUPP_TUNNEL_PROTOCOLS_RSP, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tunneling_Send_TunnelClosureNotification
 *
 * @brief   Call to send out a Tunnel Closure Notification
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tunneling_Send_TunnelClosureNotification( uint8 srcEP, afAddrType_t *dstAddr,
                                                   zclCCTunnelClosureNotification_t *pCmd,
                                                   uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_TUNNELING_TUNNEL_CLOSURE];
  ZStatus_t status;

  buf[0] = LO_UINT16( pCmd->tunnelId );
  buf[1] = HI_UINT16( pCmd->tunnelId );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_SE_TUNNELING,
                            COMMAND_SE_TUNNEL_CLOSURE_NOTIFICATION, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_TUNNELING_TUNNEL_CLOSURE, buf );

  return status;
}
#endif  //SE_UK_EXT
#endif  // ZCL_TUNNELING

#ifdef ZCL_TOU
#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_Tou_Send_PublishCalendar
 *
 * @brief   Call to send out a Publish Calendar Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_PublishCalendar( uint8 srcEP, afAddrType_t *dstAddr,
                                          zclCCPublishCalendar_t *pCmd,
                                          uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_PUBLISH_CALENDAR + pCmd->calendarName.strLen;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerCalendarId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  *pBuf++ = pCmd->calendarType;
  *pBuf++ = pCmd->calendarTimeRef;
  *pBuf++ = pCmd->calendarName.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->calendarName.pStr, pCmd->calendarName.strLen );
  *pBuf++ = pCmd->numOfSeasons;
  *pBuf++ = pCmd->numOfWeekProfiles;
  *pBuf = pCmd->numOfDayProfiles;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_PUBLISH_CALENDAR, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_PublishDayProfile
 *
 * @brief   Call to send out a Publish Day Profile Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_PublishDayProfile( uint8 srcEP, afAddrType_t *dstAddr,
                                            zclCCPublishDayProfile_t *pCmd,
                                            uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;
  uint8 i;

  bufLen = PACKET_LEN_SE_PUBLISH_DAY_PROFILE + ( pCmd->numTransferEntries * SE_DAY_SCHEDULE_ENTRY_LEN );
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerCalendarId );
  *pBuf++ = pCmd->dayId;
  *pBuf++ = pCmd->totalNumSchedEnt;
  *pBuf++ = pCmd->commandIndex;

  for ( i = 0; i < pCmd->numTransferEntries; i++ )
  {
    if ( pCmd->issuerCalendarId <= SE_CALENDAR_TYPE_IMPORT_EXPORT_CALENDAR )
    {
      zclCCRateEntry_t *pRateEntry = (zclCCRateEntry_t *)pCmd->pScheduleEntries;
      pRateEntry += i;

      *pBuf++ = LO_UINT16( pRateEntry->startTime );
      *pBuf++ = HI_UINT16( pRateEntry->startTime );
      *pBuf++ = pRateEntry->activePriceTier;

    }
    else
    {
      zclCCFriendlyCreditEntry_t *pFriendlyEntry =  (zclCCFriendlyCreditEntry_t *)pCmd->pScheduleEntries;
      pFriendlyEntry += i;

      *pBuf++ = LO_UINT16( pFriendlyEntry->startTime );
      *pBuf++ = HI_UINT16( pFriendlyEntry->startTime );
      *pBuf++ = pFriendlyEntry->friendCreditEnable;
    }
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_PUBLISH_DAY_PROFILE, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_PublishWeekProfile
 *
 * @brief   Call to send out a Publish Week Profile Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_PublishWeekProfile( uint8 srcEP, afAddrType_t *dstAddr,
                                             zclCCPublishWeekProfile_t *pCmd,
                                             uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;

  buf = osal_mem_alloc( PACKET_LEN_SE_PUBLISH_WEEK_PROFILE );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerCalendarId );
  *pBuf++ = pCmd->weekId;
  *pBuf++ = pCmd->dayIdRefMonday;
  *pBuf++ = pCmd->dayIdRefTuestday;
  *pBuf++ = pCmd->dayIdRefWednesday;
  *pBuf++ = pCmd->dayIdRefThursday;
  *pBuf++ = pCmd->dayIdRefFriday;
  *pBuf++ = pCmd->dayIdRefSaturday;
  *pBuf   = pCmd->dayIdRefSunday;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_PUBLISH_WEEK_PROFILE, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_PUBLISH_WEEK_PROFILE, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_PublishSeasons
 *
 * @brief   Call to send out a Publish Seasons Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_PublishSeasons( uint8 srcEP, afAddrType_t *dstAddr,
                                         zclCCPublishSeasons_t *pCmd,
                                         uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;
  uint8 i;

  bufLen = PACKET_LEN_SE_PUBLISH_SEASONS + ( pCmd->numTransferEntries * SE_SEASON_ENTRY_LEN );
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerCalendarId );
  *pBuf++ = pCmd->commandIndex;


  for ( i = 0; i < pCmd->numTransferEntries; i++ )
  {
    zclCCSeasonEntry_t *pEntry = &( pCmd->pSeasonEntry[i] );

    pBuf = osal_buffer_uint32( pBuf, pEntry->seasonStartDate );
    *pBuf++ = pEntry->weekIdRef;
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_PUBLISH_SEASONS, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_PublishSpecialDays
 *
 * @brief   Call to send out a Publish Special Days Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_PublishSpecialDays( uint8 srcEP, afAddrType_t *dstAddr,
                                             zclCCPublishSpecialDays_t *pCmd,
                                             uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;
  uint8 i;

  bufLen = PACKET_LEN_SE_PUBLISH_SPECIAL_DAYS + ( pCmd->numTransferEntries * SE_SPECIAL_DAY_ENTRY_LEN );
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->startTime );
  *pBuf++ = pCmd->calendarType;
  *pBuf++ = pCmd->totalNumSpecialDays;
  *pBuf++ = pCmd->commandIndex;

  for ( i = 0; i < pCmd->numTransferEntries; i++ )
  {
    zclCCSpecialDayEntry_t *pEntry = &( pCmd->pSpecialDayEntry[i] );

    pBuf = osal_buffer_uint32( pBuf, pEntry->specialDayDate );
    *pBuf++ = pEntry->dayIdRef;
  }

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_PUBLISH_SPECIAL_DAYS, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_GetCalendar
 *
 * @brief   Call to send out a Get Calendar Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_GetCalendar( uint8 srcEP, afAddrType_t *dstAddr,
                                      zclCCGetCalendar_t *pCmd,
                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_CALENDAR];
  ZStatus_t status;

  osal_buffer_uint32( buf, pCmd->startTime );
  buf[4] = pCmd->numOfCalendars;
  buf[5] = pCmd->calendarType;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_GET_CALENDAR, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_GET_CALENDAR, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_GetDayProfiles
 *
 * @brief   Call to send out a Get Day Profiles Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_GetDayProfiles( uint8 srcEP, afAddrType_t *dstAddr,
                                         zclCCGetDayProfiles_t *pCmd,
                                         uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_DAY_PROFILE];
  ZStatus_t status;

  osal_buffer_uint32( buf, pCmd->issuerCalendarId );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_GET_DAY_PROFILES, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_GET_DAY_PROFILE, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_GetWeekProfiles
 *
 * @brief   Call to send out a Get Week Profiles Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_GetWeekProfiles( uint8 srcEP, afAddrType_t *dstAddr,
                                          zclCCGetWeekProfiles_t *pCmd,
                                          uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_WEEK_PROFILE];
  ZStatus_t status;

  osal_buffer_uint32( buf, pCmd->issuerCalendarId );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_GET_WEEK_PROFILES, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_GET_WEEK_PROFILE, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_GetSeasons
 *
 * @brief   Call to send out a Get Seasons Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_GetSeasons( uint8 srcEP, afAddrType_t *dstAddr,
                                     zclCCGetSeasons_t *pCmd,
                                     uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_SEASONS];
  ZStatus_t status;

  osal_buffer_uint32( buf, pCmd->issuerCalendarId );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_GET_SEASONS, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_GET_SEASONS, buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_Tou_Send_GetSpecialDays
 *
 * @brief   Call to send out a Get Special Days Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_Tou_Send_GetSpecialDays( uint8 srcEP, afAddrType_t *dstAddr,
                                         zclCCGetSpecialDays_t *pCmd,
                                         uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 buf[PACKET_LEN_SE_GET_SPECIAL_DAYS];
  ZStatus_t status;

  osal_buffer_uint32( buf, pCmd->startTime );
  buf[4] = pCmd->numOfEvents;
  buf[5] = pCmd->calendarType;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_TOU_CALENDAR,
                            COMMAND_SE_GET_SPECIAL_DAYS, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_GET_SPECIAL_DAYS, buf );

  return status;
}
#endif  // SE_UK_EXT
#endif  // ZCL_TOU

#ifdef ZCL_DEVICE_MGMT
#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_GetChangeTenancy
 *
 * @brief   Call to send out a Get Change of Tenancy Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_GetChangeTenancy( uint8 srcEP, afAddrType_t *dstAddr,
                                                  uint8 disableDefaultRsp, uint8 seqNum )
{
  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                          COMMAND_SE_GET_CHANGE_OF_TENANCY, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, 0, NULL );
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_GetChangeSupplier
 *
 * @brief   Call to send out a Get Change of Supplier Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_GetChangeSupplier( uint8 srcEP, afAddrType_t *dstAddr,
                                                   uint8 disableDefaultRsp, uint8 seqNum )
{
  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                          COMMAND_SE_GET_CHANGE_OF_SUPPLIER, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, 0, NULL );
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_GetChangeSupply
 *
 * @brief   Call to send out a Get Change Supply Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_GetChangeSupply( uint8 srcEP, afAddrType_t *dstAddr,
                                                 uint8 disableDefaultRsp, uint8 seqNum )
{
  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                          COMMAND_SE_GET_CHANGE_SUPPLY, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, 0, NULL );
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_SupplyStatusResponse
 *
 * @brief   Call to send out a Supply Status Response Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_SupplyStatusResponse( uint8 srcEP, afAddrType_t *dstAddr,
                                                      zclCCSupplyStatusResponse_t *pCmd,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  ZStatus_t status;

  buf = osal_mem_alloc( PACKET_LEN_SE_SUPPLY_STATUS_RESPONSE );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->supplierId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->issuerEventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->implementationDateTime );
  *pBuf = pCmd->supplyStatus;

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                            COMMAND_SE_SUPPLY_STATUS_RESPONSE, TRUE,
                            ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                            seqNum, PACKET_LEN_SE_SUPPLY_STATUS_RESPONSE, buf );

  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_GetPassword
 *
 * @brief   Call to send out a Get Password Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_GetPassword( uint8 srcEP, afAddrType_t *dstAddr,
                                             zclCCGetPassword_t *pCmd,
                                             uint8 disableDefaultRsp, uint8 seqNum )
{
  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                          COMMAND_SE_GET_PASSWORD, TRUE,
                          ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_GET_PASSWORD,
                          &(pCmd->passwordLevel) );
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_PublishChangeTenancy
 *
 * @brief   Call to send out a Publish Change of Tenancy Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_PublishChangeTenancy( uint8 srcEP, afAddrType_t *dstAddr,
                                                      zclCCPublishChangeTenancy_t *pCmd,
                                                      uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_PUBLISH_CHANGE_OF_TENANCY + pCmd->signature.strLen;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->supplierId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->eventId );
  *pBuf++ = pCmd->tariffType;
  pBuf = osal_buffer_uint32( pBuf, pCmd->implementationDateTime );
  pBuf = osal_buffer_uint32( pBuf, pCmd->propTenencyChangeCtrl );
  *pBuf++ = pCmd->signature.strLen;
  (void) osal_memcpy( pBuf, pCmd->signature.pStr, pCmd->signature.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                            COMMAND_SE_PUBLISH_CHANGE_OF_TENANCY, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_PublishChangeSupplier
 *
 * @brief   Call to send out a Publish Change of Supplier Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_PublishChangeSupplier( uint8 srcEP, afAddrType_t *dstAddr,
                                                       zclCCPublishChangeSupplier_t *pCmd,
                                                       uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  // include in length all variable length fields
  bufLen = PACKET_LEN_SE_PUBLISH_CHANGE_OF_SUPPLIER +
           pCmd->supplierIdName.strLen +
           pCmd->signature.strLen;

  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->supplierId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->eventId );
  *pBuf++ = pCmd->tariffType;
  pBuf = osal_buffer_uint32( pBuf, pCmd->propSupplierId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->suppIdImplemDateTime );
  pBuf = osal_buffer_uint32( pBuf, pCmd->supplierChangeCtrl );
  *pBuf++ = pCmd->supplierIdName.strLen;
  pBuf = osal_memcpy( pBuf, pCmd->supplierIdName.pStr, pCmd->supplierIdName.strLen );
  *pBuf++ = pCmd->signature.strLen;
  (void) osal_memcpy( pBuf, pCmd->signature.pStr, pCmd->signature.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                            COMMAND_SE_PUBLISH_CHANGE_OF_SUPPLIER, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_ChangeSupply
 *
 * @brief   Call to send out a Change Supply Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_ChangeSupply( uint8 srcEP, afAddrType_t *dstAddr,
                                              zclCCChangeSupply_t *pCmd,
                                              uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint8 *pBuf;
  uint16 bufLen;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_CHANGE_SUPPLY + pCmd->signature.strLen;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  pBuf = osal_buffer_uint32( buf, pCmd->supplierId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->eventId );
  pBuf = osal_buffer_uint32( pBuf, pCmd->requestDateTime );
  pBuf = osal_buffer_uint32( pBuf, pCmd->implementationDateTime );
  *pBuf++ = pCmd->proposedSupplyStatus;
  *pBuf++ = pCmd->origIdSupplyControlBits;
  *pBuf++ = pCmd->signature.strLen;
  (void) osal_memcpy( pBuf, pCmd->signature.pStr, pCmd->signature.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                            COMMAND_SE_CHANGE_SUPPLY, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_ChangePassword
 *
 * @brief   Call to send out a Change Password Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_ChangePassword( uint8 srcEP, afAddrType_t *dstAddr,
                                                zclCCChangePassword_t *pCmd,
                                                uint8 disableDefaultRsp, uint8 seqNum )
{
  uint8 *buf;
  uint16 bufLen;
  ZStatus_t status;

  bufLen = PACKET_LEN_SE_CHANGE_PASSWORD + pCmd->password.strLen;
  buf = osal_mem_alloc( bufLen );
  if ( buf == NULL )
  {
    return ( ZMemError );
  }

  buf[0] = pCmd->passwordLevel;
  buf[1] = pCmd->password.strLen;
  (void) osal_memcpy( &(buf[2]), pCmd->password.pStr, pCmd->password.strLen );

  status = zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                            COMMAND_SE_CHANGE_PASSWORD, TRUE,
                            ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                            seqNum, bufLen, buf );
  osal_mem_free( buf );

  return status;
}

/*********************************************************************
 * @fn      zclSE_DeviceMgmt_Send_LocalChangeSupply
 *
 * @brief   Call to send out a Local Change Supply Command
 *
 * @param   srcEP - Sending application's endpoint
 * @param   dstAddr - where you want the message to go
 * @param   pCmd - command payload
 * @param   disableDefaultRsp - disable default response
 * @param   seqNum - ZCL sequence number
 *
 * @return  ZStatus_t
 */
ZStatus_t zclSE_DeviceMgmt_Send_LocalChangeSupply( uint8 srcEP, afAddrType_t *dstAddr,
                                                   zclCCLocalChangeSupply_t *pCmd,
                                                   uint8 disableDefaultRsp, uint8 seqNum )
{
  return zcl_SendCommand( srcEP, dstAddr, ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                          COMMAND_SE_LOCAL_CHANGE_SUPPLY, TRUE,
                          ZCL_FRAME_SERVER_CLIENT_DIR, disableDefaultRsp, 0,
                          seqNum, PACKET_LEN_SE_LOCAL_CHANGE_SUPPLY,
                          &(pCmd->propSupplyStatus) );
}
#endif  // SE_UK_EXT
#endif  // ZCL_DEVICE_MGMT

/*********************************************************************
 * @fn      zclSE_RegisterCmdCallbacks
 *
 * @brief   Register an applications command callbacks
 *
 * @param   endpoint - application's endpoint
 * @param   callbacks - pointer to the callback record.
 *
 * @return  ZMemError if not able to allocate
 */
ZStatus_t zclSE_RegisterCmdCallbacks( uint8 endpoint, zclSE_AppCallbacks_t *callbacks )
{
  zclSECBRec_t *pNewItem;
  zclSECBRec_t *pLoop;

  // Register as a ZCL Plugin
  if ( !zclSEPluginRegisted )
  {
#ifndef SE_UK_EXT // SE 1.1
    zcl_registerPlugin( ZCL_CLUSTER_ID_SE_PRICING,
                        ZCL_CLUSTER_ID_SE_PREPAYMENT,
                        zclSE_HdlIncoming );
#else
    zcl_registerPlugin( ZCL_CLUSTER_ID_SE_PRICING,
                        ZCL_CLUSTER_ID_SE_DEVICE_MGMT,
                        zclSE_HdlIncoming );
#endif
    zclSEPluginRegisted = TRUE;
  }

  // Fill in the new profile list
  pNewItem = osal_mem_alloc( sizeof( zclSECBRec_t ) );
  if ( pNewItem == NULL )
  {
    return ( ZMemError );
  }

  pNewItem->next = (zclSECBRec_t *)NULL;
  pNewItem->endpoint = endpoint;
  pNewItem->CBs = callbacks;

  // Find spot in list
  if ( zclSECBs == NULL )
  {
    zclSECBs = pNewItem;
  }
  else
  {
    // Look for end of list
    pLoop = zclSECBs;
    while ( pLoop->next != NULL )
    {
      pLoop = pLoop->next;
    }

    // Put new item at end of list
    pLoop->next = pNewItem;
  }

  return ( ZSuccess );
}

#if defined( ZCL_LOAD_CONTROL ) || defined( ZCL_SIMPLE_METERING ) || \
    defined( ZCL_PRICING ) || defined( ZCL_MESSAGE ) || \
    defined( ZCL_PREPAYMENT ) || defined( ZCL_TUNNELING ) || \
    defined( ZCL_TOU ) || defined( ZCL_DEVICE_MGMT )
/*********************************************************************
 * @fn      zclSE_FindCallbacks
 *
 * @brief   Find the callbacks for an endpoint
 *
 * @param   endpoint
 *
 * @return  pointer to the callbacks
 */
static zclSE_AppCallbacks_t *zclSE_FindCallbacks( uint8 endpoint )
{
  zclSECBRec_t *pCBs;

  pCBs = zclSECBs;
  while ( pCBs )
  {
    if ( pCBs->endpoint == endpoint )
    {
      return ( pCBs->CBs );
    }
    pCBs = pCBs->next;
  }
  return ( (zclSE_AppCallbacks_t *)NULL );
}
#endif

/*********************************************************************
 * @fn      zclSE_HdlIncoming
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library or Profile commands for attributes
 *          that aren't in the attribute list
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_HdlIncoming(  zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZSuccess;

#if defined ( INTER_PAN )
  if ( StubAPS_InterPan( pInMsg->msg->srcAddr.panId, pInMsg->msg->srcAddr.endPoint ) &&
       !INTER_PAN_CLUSTER(pInMsg->msg->clusterId) )
  {
    return ( stat ); // Cluster not supported thru Inter-PAN
  }
#endif
  if ( zcl_ClusterCmd( pInMsg->hdr.fc.type ) )
  {
    // Is this a manufacturer specific command?
    if ( pInMsg->hdr.fc.manuSpecific == 0 )
    {
      stat = zclSE_HdlInSpecificCommands( pInMsg );
    }
    else
    {
      // We don't support any manufacturer specific command.
      stat = ZFailure;
    }
  }
  else
  {
    // Handle all the normal (Read, Write...) commands -- should never get here
    stat = ZFailure;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_HdlInSpecificCommands
 *
 * @brief   Function to process incoming Commands specific
 *          to this cluster library

 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_HdlInSpecificCommands( zclIncoming_t *pInMsg )
{
  ZStatus_t stat;
  zclSE_AppCallbacks_t *pCBs;

  // make sure endpoint exists

#if defined( ZCL_LOAD_CONTROL ) || defined( ZCL_SIMPLE_METERING ) || \
    defined( ZCL_PRICING ) || defined( ZCL_MESSAGE ) || \
    defined( ZCL_PREPAYMENT ) || defined( ZCL_TUNNELING ) || \
    defined( ZCL_TOU ) || defined( ZCL_DEVICE_MGMT )

  pCBs = zclSE_FindCallbacks( pInMsg->msg->endPoint );
  if ( pCBs == NULL )
  {
    return ( ZFailure );
  }

#endif
  switch ( pInMsg->msg->clusterId )			
  {
#ifdef ZCL_SIMPLE_METERING
    case ZCL_CLUSTER_ID_SE_SIMPLE_METERING:
      stat = zclSE_ProcessInSimpleMeteringCmds( pInMsg, pCBs );
      break;
#endif

#ifdef ZCL_PRICING
    case ZCL_CLUSTER_ID_SE_PRICING:
      stat = zclSE_ProcessInPricingCmds( pInMsg, pCBs );
      break;
#endif

#ifdef ZCL_MESSAGE
    case ZCL_CLUSTER_ID_SE_MESSAGE:
      stat = zclSE_ProcessInMessageCmds( pInMsg, pCBs );
      break;
#endif

#ifdef ZCL_LOAD_CONTROL
    case ZCL_CLUSTER_ID_SE_LOAD_CONTROL:
      stat = zclSE_ProcessInLoadControlCmds( pInMsg, pCBs );
      break;
#endif

#ifdef ZCL_TUNNELING
    case ZCL_CLUSTER_ID_SE_SE_TUNNELING:
      stat = zclSE_ProcessInTunnelingCmds( pInMsg, pCBs );
      break;
#endif // ZCL_TUNNELING

#ifdef ZCL_PREPAYMENT
    case ZCL_CLUSTER_ID_SE_PREPAYMENT:
      stat = zclSE_ProcessInPrepaymentCmds( pInMsg, pCBs );
      break;
#endif // ZCL_PREPAYMENT

#ifdef SE_UK_EXT
#ifdef ZCL_TOU
    case ZCL_CLUSTER_ID_SE_TOU_CALENDAR:
      stat = zclSE_ProcessInTouCmds( pInMsg, pCBs );
      break;
#endif  // ZCL_TOU

#ifdef ZCL_DEVICE_MGMT
    case ZCL_CLUSTER_ID_SE_DEVICE_MGMT:
      stat = zclSE_ProcessInDeviceMgmtCmds( pInMsg, pCBs );
      break;
#endif  // ZCL_DEVICE_MGMT
#endif  // SE_UK_EXT

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

#ifdef ZCL_SIMPLE_METERING
/*********************************************************************
 * @fn      zclSE_ProcessInSimpleMeteringCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_ProcessInSimpleMeteringCmds( zclIncoming_t *pInMsg,
                                                     zclSE_AppCallbacks_t *pCBs )
{
  ZStatus_t stat;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Process Client commands, received by server
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_GET_PROFILE_CMD:
        stat = zclSE_ProcessInCmd_SimpleMeter_GetProfileCmd( pInMsg, pCBs );
        break;

      case COMMAND_SE_REQ_MIRROR_RSP:
        stat = zclSE_ProcessInCmd_SimpleMeter_ReqMirrorRsp( pInMsg, pCBs );
        break;

      case COMMAND_SE_MIRROR_REM_RSP:
        stat = zclSE_ProcessInCmd_SimpleMeter_MirrorRemRsp( pInMsg, pCBs );
        break;

      case COMMAND_SE_REQ_FAST_POLL_MODE_CMD:
        stat = zclSE_ProcessInCmd_SimpleMeter_ReqFastPollModeCmd( pInMsg, pCBs );
        break;

#ifdef SE_UK_EXT
      case COMMAND_SE_GET_SNAPSHOT_CMD:
        stat = zclSE_ProcessInCmd_SimpleMeter_GetSnapshotCmd( pInMsg, pCBs );
        break;

      case COMMAND_SE_TAKE_SNAPSHOT_CMD:
        stat = zclSE_ProcessInCmd_SimpleMeter_TakeSnapshotCmd( pInMsg, pCBs );
        break;

      case COMMAND_SE_MIRROR_REPORT_ATTR_RSP:
        stat = zclSE_ProcessInCmd_SimpleMeter_MirrorReportAttrRsp( pInMsg, pCBs );
        break;
#endif  // SE_UK_EXT

      default:
        stat = ZFailure;
        break;
    }
  }
  else
  {
    // Process Server commands, received by client
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_GET_PROFILE_RSP:
        stat = zclSE_ProcessInCmd_SimpleMeter_GetProfileRsp( pInMsg, pCBs );
        break;

      case COMMAND_SE_REQ_MIRROR_CMD:
        stat = zclSE_ProcessInCmd_SimpleMeter_ReqMirrorCmd( pInMsg, pCBs );
        break;

      case COMMAND_SE_MIRROR_REM_CMD:
        stat = zclSE_ProcessInCmd_SimpleMeter_MirrorRemCmd( pInMsg, pCBs );
        break;

      case COMMAND_SE_REQ_FAST_POLL_MODE_RSP:
        stat = zclSE_ProcessInCmd_SimpleMeter_ReqFastPollModeRsp( pInMsg, pCBs );
        break;

#ifdef SE_UK_EXT
      case COMMAND_SE_GET_SNAPSHOT_RSP:
        stat = zclSE_ProcessInCmd_SimpleMeter_GetSnapshotRsp( pInMsg, pCBs );
        break;
#endif  // SE_UK_EXT

      default:
        stat = ZFailure;
        break;
    }
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_GetProfileCmd
 *
 * @brief   Process in the received Get Profile Command.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 *                      ZCL_STATUS_INVALID_FIELD @ Range checking
 *                                           failure
 *
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_GetProfileCmd( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_GetProfileCmd )
  {
    zclCCGetProfileCmd_t cmd;

    cmd.channel = pInMsg->pData[0];
    cmd.endTime = osal_build_uint32( &pInMsg->pData[1], 4 );
    cmd.numOfPeriods = pInMsg->pData[5];

    // Range checking
    if ( cmd.channel > MAX_INTERVAL_CHANNEL_SE_SIMPLE_METERING )
    {
      return ZCL_STATUS_INVALID_FIELD;
    }
    pCBs->pfnSimpleMeter_GetProfileCmd( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum  );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_GetProfileRsp
 *
 * @brief   Process in the received Get Profile Response.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 *                      ZCL_STATUS_INVALID_FIELD @ Range checking
 *                                           failure
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_GetProfileRsp( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_GetProfileRsp )
  {
    uint24 *pBuf24;
    uint8  *pBuf8;
    uint8  i;
    zclCCGetProfileRsp_t cmd;

    cmd.endTime = osal_build_uint32( &pInMsg->pData[0], 4 );
    cmd.status = pInMsg->pData[4];
    cmd.profileIntervalPeriod = pInMsg->pData[5];
    cmd.numOfPeriodDelivered = pInMsg->pData[6];

    // Range Checking
    if ( cmd.profileIntervalPeriod > MAX_PROFILE_INTERVAL_PERIOD_SE_SIMPLE_METERING )
    {
       return ZCL_STATUS_INVALID_FIELD;
    }

    // Convert the byte stream to array of uint24
    pBuf8 = &pInMsg->pData[7];  // Pointer to the start of the array of bytes

    // Pointer to the start of the array of uint24
    pBuf24 = (uint24*)osal_mem_alloc( cmd.numOfPeriodDelivered *
                                     sizeof(uint24) );
    if ( pBuf24 == NULL )
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;  // Memory allocation error
    }

    cmd.intervals = pBuf24;
    for ( i = 0; i < cmd.numOfPeriodDelivered; i++ )
    {
      *(pBuf24++) = osal_build_uint32( pBuf8, 3 );
      pBuf8 += 3;
    }

    pCBs->pfnSimpleMeter_GetProfileRsp( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );

    osal_mem_free( cmd.intervals );

    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_ReqMirrorCmd
 *
 * @brief   Process in the received Request Mirror Command.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_ReqMirrorCmd( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_ReqMirrorCmd )
  {
    pCBs->pfnSimpleMeter_ReqMirrorCmd( &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_ReqMirrorRsp
 *
 * @brief   Process in the received Request Mirror Response.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_ReqMirrorRsp( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_ReqMirrorRsp )
  {
    zclCCReqMirrorRsp_t cmd;

    cmd.endpointId = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );

    pCBs->pfnSimpleMeter_ReqMirrorRsp( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZSuccess ;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_MirrorRemCmd
 *
 * @brief   Process in the received Mirror Removed Command.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_MirrorRemCmd( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_MirrorRemCmd )
  {
    pCBs->pfnSimpleMeter_MirrorRemCmd( &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_MirrorRemRsp
 *
 * @brief   Process in the received Mirror Removed Response.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_MirrorRemRsp( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_MirrorRemRsp )
  {
    zclCCMirrorRemRsp_t cmd;

    cmd.endpointId = pInMsg->pData[0] | ( (uint16)pInMsg->pData[1] << 8 );

    pCBs->pfnSimpleMeter_MirrorRemRsp( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}


/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_ReqFastPollModeCmd
 *
 * @brief   Process in the received Request Fast Poll Mode Command
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_ReqFastPollModeCmd( zclIncoming_t *pInMsg,
                                                                   zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_ReqFastPollModeCmd )
  {
    zclCCReqFastPollModeCmd_t cmd;
    zclAttrRec_t attrRec;
    uint8 fastPollUpdatePeriodAttr = 0;

    // Retrieve Fast Poll Update Period Attribute Record and save value to local variable
    if ( zclFindAttrRec( pInMsg->msg->endPoint, pInMsg->msg->clusterId,
                         ATTRID_SE_FAST_POLL_UPDATE_PERIOD, &attrRec ) )
    {
      zclReadAttrData( (uint8 *)&fastPollUpdatePeriodAttr, &attrRec, NULL );
    }

    // Value has been set by application
    if (( fastPollUpdatePeriodAttr > 0 ) && (pInMsg->pData[0] < fastPollUpdatePeriodAttr))
    {
      // the minimum acceptable value is defined by the attribute value
      cmd.fastPollUpdatePeriod = fastPollUpdatePeriodAttr;
    }
    else
    {
      // use received update period
      cmd.fastPollUpdatePeriod = pInMsg->pData[0];
    }

    // As per SE 1.1 spec: maximum duration value will be used if received exceeds it
    cmd.duration = MIN(pInMsg->pData[1], MAX_DURATION_IN_MINUTES_FAST_POLL_MODE);

    pCBs->pfnSimpleMeter_ReqFastPollModeCmd(&cmd, &(pInMsg->msg->srcAddr),
                                            pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_ReqFastPollModeRsp
 *
 * @brief   Process in the received Request Fast Poll Mode Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_ReqFastPollModeRsp( zclIncoming_t *pInMsg,
                                                                   zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_ReqFastPollModeRsp )
  {
    zclCCReqFastPollModeRsp_t cmd;

    cmd.appliedUpdatePeriod = pInMsg->pData[0];

    cmd.fastPollModeEndTime = osal_build_uint32( &pInMsg->pData[1], 4 );

    pCBs->pfnSimpleMeter_ReqFastPollModeRsp(&cmd, &(pInMsg->msg->srcAddr),
                                            pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_GetSnapshotCmd
 *
 * @brief   Process in the received Get Snapshot Command
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_GetSnapshotCmd( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_GetSnapshotCmd )
  {
    zclCCReqGetSnapshotCmd_t cmd;

    cmd.StartTime = osal_build_uint32( &pInMsg->pData[0], 4 );
    cmd.NumberOfSnapshots = pInMsg->pData[4];
    cmd.SnapshotCause = BUILD_UINT16( pInMsg->pData[5], pInMsg->pData[6] );

    pCBs->pfnSimpleMeter_GetSnapshotCmd(&cmd, &(pInMsg->msg->srcAddr),
                                        pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_GetSnapshotRsp
 *
 * @brief   Process in the received Get Snapshot Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_GetSnapshotRsp( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_GetSnapshotRsp )
  {
    zclCCReqGetSnapshotRsp_t cmd;

    cmd.IssuerEventID = osal_build_uint32( &pInMsg->pData[0], 4 );
    cmd.SnapshotTime = osal_build_uint32( &pInMsg->pData[4], 4 );
    cmd.CommandIndex = pInMsg->pData[8];
    cmd.SnapshotCause = BUILD_UINT16( pInMsg->pData[9], pInMsg->pData[10] );
    cmd.SnapshotPayloadType = pInMsg->pData[11];

    cmd.pSnapshotPayload = pInMsg->pData + 12;

    pCBs->pfnSimpleMeter_GetSnapshotRsp(&cmd, &(pInMsg->msg->srcAddr),
                                        pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_TakeSnapshotCmd
 *
 * @brief   Process in the received Take Snapshot Command
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_TakeSnapshotCmd( zclIncoming_t *pInMsg,
                                                                 zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_TakeSnapshotCmd )
  {
    pCBs->pfnSimpleMeter_TakeSnapshotCmd( &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_SimpleMeter_MirrorReportAttrRsp
 *
 * @brief   Process in the received Mirror Report Attribute Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_SimpleMeter_MirrorReportAttrRsp( zclIncoming_t *pInMsg,
                                                                     zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnSimpleMeter_MirrorReportAttrRsp )
  {
    zclCCReqMirrorReportAttrRsp_t cmd;

    cmd.NotificationFlags = pInMsg->pData[0];
    cmd.PriceNotificationFlags = BUILD_UINT16( pInMsg->pData[1], pInMsg->pData[2] );
    cmd.CalendarNotificationFlags = pInMsg->pData[3];
    cmd.PrePayNotificationFlags = BUILD_UINT16( pInMsg->pData[4], pInMsg->pData[5] );
    cmd.DeviceMgmtNotificationFlags = pInMsg->pData[6];

    pCBs->pfnSimpleMeter_MirrorReportAttrRsp(&cmd, &(pInMsg->msg->srcAddr),
                                            pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}
#endif  // SE_UK_EXT
#endif  // ZCL_SIMPLE_METERING


#ifdef ZCL_PRICING
/*********************************************************************
 * @fn      zclSE_ProcessInPricingCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_ProcessInPricingCmds( zclIncoming_t *pInMsg,
                                              zclSE_AppCallbacks_t *pCBs )
{
  ZStatus_t stat;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Process Client commands, received by server
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_GET_CURRENT_PRICE:
        stat = zclSE_ProcessInCmd_Pricing_GetCurrentPrice( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_SCHEDULED_PRICE:
        stat = zclSE_ProcessInCmd_Pricing_GetScheduledPrice( pInMsg, pCBs );
        break;

      case COMMAND_SE_PRICE_ACKNOWLEDGEMENT:
        stat = zclSE_ProcessInCmd_Pricing_PriceAcknowledgement ( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_BLOCK_PERIOD:
        stat = zclSE_ProcessInCmd_Pricing_GetBlockPeriod ( pInMsg, pCBs );
        break;

#ifdef SE_UK_EXT
      case COMMAND_SE_GET_TARIFF_INFO:
        stat = zclSE_ProcessInCmd_Pricing_GetTariffInformation ( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_PRICE_MATRIX:
        stat = zclSE_ProcessInCmd_Pricing_GetPriceMatrix ( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_BLOCK_THRESHOLD:
        stat = zclSE_ProcessInCmd_Pricing_GetBlockThresholds ( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_CONVERSION_FACTOR:
        stat = zclSE_ProcessInCmd_Pricing_GetConversionFactor ( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_CALORIFIC_VALUE:
        stat = zclSE_ProcessInCmd_Pricing_GetCalorificValue ( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_CO2_VALUE:
        stat = zclSE_ProcessInCmd_Pricing_GetCO2Value ( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_BILLING_PERIOD:
        stat = zclSE_ProcessInCmd_Pricing_GetBillingPeriod ( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_CONSOLIDATED_BILL:
        stat = zclSE_ProcessInCmd_Pricing_GetConsolidatedBill ( pInMsg, pCBs );
        break;

      case COMMAND_SE_CPP_EVENT_RESPONSE:
        stat = zclSE_ProcessInCmd_Pricing_CPPEventResponse ( pInMsg, pCBs );
        break;
#endif  // SE_UK_EXT

      default:
        stat = ZFailure;
        break;
    }
  }
  else
  {
    // Process Server commands, received by client
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_PUBLISH_PRICE:
        stat = zclSE_ProcessInCmd_Pricing_PublishPrice( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_BLOCK_PERIOD:
        stat = zclSE_ProcessInCmd_Pricing_PublishBlockPeriod( pInMsg, pCBs );
        break;

#ifdef SE_UK_EXT
      case COMMAND_SE_PUBLISH_TARIFF_INFO:
        stat = zclSE_ProcessInCmd_Pricing_PublishTariffInformation( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_PRICE_MATRIX:
        stat = zclSE_ProcessInCmd_Pricing_PublishPriceMatrix( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_BLOCK_THRESHOLD:
        stat = zclSE_ProcessInCmd_Pricing_PublishBlockThreshold( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_CONVERSION_FACTOR:
        stat = zclSE_ProcessInCmd_Pricing_PublishConversionFactor( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_CALORIFIC_VALUE:
        stat = zclSE_ProcessInCmd_Pricing_PublishCalorificValue( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_CO2_VALUE:
        stat = zclSE_ProcessInCmd_Pricing_PublishCO2Value( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_CPP_EVENT:
        stat = zclSE_ProcessInCmd_Pricing_PublishCPPEvent( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_BILLING_PERIOD:
        stat = zclSE_ProcessInCmd_Pricing_PublishBillingPeriod( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_CONSOLIDATED_BILL:
        stat = zclSE_ProcessInCmd_Pricing_PublishConsolidatedBill( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_CREDIT_PAYMENT_INFO:
        stat = zclSE_ProcessInCmd_Pricing_PublishCreditPaymentInfo( pInMsg, pCBs );
        break;
#endif  // SE_UK_EXT

      default:
        stat = ZFailure;
        break;
    }
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetCurrentPrice
 *
 * @brief   Process in the received Get Current Price.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetCurrentPrice( zclIncoming_t *pInMsg,
                                                              zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetCurrentPrice )
  {
    zclCCGetCurrentPrice_t cmd;

    cmd.option = pInMsg->pData[0];

    pCBs->pfnPricing_GetCurrentPrice( &cmd,  &(pInMsg->msg->srcAddr),
                                     pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetScheduledPrice
 *
 * @brief   Process in the received Get Scheduled Price.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetScheduledPrice( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetScheduledPrice )
  {
    zclCCGetScheduledPrice_t cmd;

    cmd.startTime = osal_build_uint32( pInMsg->pData, 4 );
    cmd.numEvents = pInMsg->pData[4];

    pCBs->pfnPricing_GetScheduledPrice( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishPrice
 *
 * @brief   Process in the received Publish Price.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishPrice( zclIncoming_t *pInMsg,
                                                           zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishPrice )
  {
    zclCCPublishPrice_t cmd;

    // Parse the command and do range check
    if ( zclSE_ParseInCmd_PublishPrice( &cmd, &(pInMsg->pData[0]),
                                        pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnPricing_PublishPrice( &cmd, &(pInMsg->msg->srcAddr),
                                    pInMsg->hdr.transSeqNum );

      // Free the memory allocated in zclSE_ParseInCmd_PublishPrice()
      if ( cmd.rateLabel.pStr != NULL )
      {
        osal_mem_free( cmd.rateLabel.pStr );
      }

      // SE 1.1
      if ((pInMsg->pDataLen - cmd.rateLabel.strLen) > PACKET_LEN_SE_PUBLISH_PRICE_SE_1_0)
      {
        return ZCL_STATUS_CMD_HAS_RSP;
      }
      else
      {
        // SE 1.0 backwards compatibility
        return ZSuccess;
      }
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PriceAcknowledgement
 *
 * @brief   Process in the received Price Acknowledgement
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PriceAcknowledgement( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PriceAcknowledgement )
  {
    zclCCPriceAcknowledgement_t cmd;

    cmd.providerId = osal_build_uint32( pInMsg->pData, 4 );
    cmd.issuerEventId = osal_build_uint32( &pInMsg->pData[4], 4 );
    cmd.priceAckTime = osal_build_uint32( &pInMsg->pData[8], 4 );
    cmd.control = pInMsg->pData[12];

    pCBs->pfnPricing_PriceAcknowledgement( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetBlockPeriod
 *
 * @brief   Process in the received Get Block Period.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetBlockPeriod( zclIncoming_t *pInMsg,
                                                            zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetBlockPeriod )
  {
    zclCCGetBlockPeriod_t cmd;

    cmd.startTime = osal_build_uint32( pInMsg->pData, 4 );
    cmd.numEvents = pInMsg->pData[4];

    pCBs->pfnPricing_GetBlockPeriod( &cmd, &(pInMsg->msg->srcAddr),
                                     pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishBlockPeriod
 *
 * @brief   Process in the received Publish Block Period.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishBlockPeriod( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishBlockPeriod )
  {
    zclCCPublishBlockPeriod_t cmd;

    // Parse the command and do range check
    zclSE_ParseInCmd_PublishBlockPeriod( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnPricing_PublishBlockPeriod( &cmd, &(pInMsg->msg->srcAddr),
                                         pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishTariffInformation
 *
 * @brief   Process in the received Publish Tarif Information.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishTariffInformation( zclIncoming_t *pInMsg,
                                                                      zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishTariffInformation )
  {
    zclCCPublishTariffInformation_t cmd;

    zclSE_ParseInCmd_PublishTariffInformation( &cmd, &(pInMsg->pData[0]),
                                               pInMsg->pDataLen );

    pCBs->pfnPricing_PublishTariffInformation( &cmd, &(pInMsg->msg->srcAddr),
                                               pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishPriceMatrix
 *
 * @brief   Process in the received Publish Price Matrix.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishPriceMatrix( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishPriceMatrix )
  {
    zclCCPublishPriceMatrix_t cmd;

    if ( zclSE_ParseInCmd_PublishPriceMatrix( &cmd, &(pInMsg->pData[0]),
                                             pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnPricing_PublishPriceMatrix( &cmd, &(pInMsg->msg->srcAddr),
                                                 pInMsg->hdr.transSeqNum );

      if ( cmd.pTierBlockPrice != NULL )
      {
        osal_mem_free( cmd.pTierBlockPrice );
      }

      return ZCL_STATUS_CMD_HAS_RSP;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishBlockThreshold
 *
 * @brief   Process in the received Publish Block Threshold.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishBlockThreshold( zclIncoming_t *pInMsg,
                                                                   zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishBlockThresholds )
  {
    zclCCPublishBlockThresholds_t cmd;

    if ( zclSE_ParseInCmd_PublishBlockThresholds( &cmd, &(pInMsg->pData[0]),
                                             pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnPricing_PublishBlockThresholds( &cmd, &(pInMsg->msg->srcAddr),
                                               pInMsg->hdr.transSeqNum );

      if ( cmd.pTierBlockThreshold != NULL )
      {
        osal_mem_free( cmd.pTierBlockThreshold );
      }

      return ZCL_STATUS_CMD_HAS_RSP;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishConversionFactor
 *
 * @brief   Process in the received Publish Conversion Factor.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishConversionFactor( zclIncoming_t *pInMsg,
                                                                     zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishConversionFactor )
  {
    zclCCPublishConversionFactor_t cmd;

    zclSE_ParseInCmd_PublishConversionFactor( &cmd, &(pInMsg->pData[0]),
                                              pInMsg->pDataLen );

    pCBs->pfnPricing_PublishConversionFactor( &cmd, &(pInMsg->msg->srcAddr),
                                              pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishCalorificValue
 *
 * @brief   Process in the received Publish Calorific Value.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishCalorificValue( zclIncoming_t *pInMsg,
                                                                   zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishCalorificValue )
  {
    zclCCPublishCalorificValue_t cmd;

    zclSE_ParseInCmd_PublishCalorificValue( &cmd, &(pInMsg->pData[0]),
                                            pInMsg->pDataLen );

    pCBs->pfnPricing_PublishCalorificValue( &cmd, &(pInMsg->msg->srcAddr),
                                            pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishCO2Value
 *
 * @brief   Process in the received Publish CO2 Value.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishCO2Value( zclIncoming_t *pInMsg,
                                                             zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishCO2Value )
  {
    zclCCPublishCO2Value_t cmd;

    zclSE_ParseInCmd_PublishCO2Value( &cmd, &(pInMsg->pData[0]),
                                      pInMsg->pDataLen );

    pCBs->pfnPricing_PublishCO2Value( &cmd, &(pInMsg->msg->srcAddr),
                                      pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishCPPEvent
 *
 * @brief   Process in the received Publish CPP Event.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishCPPEvent( zclIncoming_t *pInMsg,
                                                             zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishCPPEvent )
  {
    zclCCPublishCPPEvent_t cmd;

    zclSE_ParseInCmd_PublishCPPEvent( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnPricing_PublishCPPEvent( &cmd, &(pInMsg->msg->srcAddr),
                                      pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishBillingPeriod
 *
 * @brief   Process in the received Publish Billing Period.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishBillingPeriod( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishBillingPeriod )
  {
    zclCCPublishBillingPeriod_t cmd;

    zclSE_ParseInCmd_PublishBillingPeriod( &cmd, &(pInMsg->pData[0]),
                                          pInMsg->pDataLen );

    pCBs->pfnPricing_PublishBillingPeriod( &cmd, &(pInMsg->msg->srcAddr),
                                                 pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishConsolidatedBill
 *
 * @brief   Process in the received Publish Consolidated Bill.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishConsolidatedBill( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishConsolidatedBill )
  {
    zclCCPublishConsolidatedBill_t cmd;

    zclSE_ParseInCmd_PublishConsolidatedBill( &cmd, &(pInMsg->pData[0]),
                                              pInMsg->pDataLen );

    pCBs->pfnPricing_PublishConsolidatedBill( &cmd, &(pInMsg->msg->srcAddr),
                                              pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_PublishCreditPaymentInfo
 *
 * @brief   Process in the received Publish Credit Payment Information.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_PublishCreditPaymentInfo( zclIncoming_t *pInMsg,
                                                                      zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_PublishCreditPaymentInfo )
  {
    zclCCPublishCreditPaymentInfo_t cmd;

    zclSE_ParseInCmd_PublishCreditPaymentInfo( &cmd, &(pInMsg->pData[0]),
                                               pInMsg->pDataLen );

    pCBs->pfnPricing_PublishCreditPaymentInfo( &cmd, &(pInMsg->msg->srcAddr),
                                               pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetTariffInformation
 *
 * @brief   Process in the received Get Tariff Information.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetTariffInformation( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetTariffInformation )
  {
    zclCCGetTariffInformation_t cmd;

    zclSE_ParseInCmd_GetTariffInformation( &cmd, &(pInMsg->pData[0]),
                                          pInMsg->pDataLen );

    pCBs->pfnPricing_GetTariffInformation( &cmd, &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetPriceMatrix
 *
 * @brief   Process in the received Get Price Matrix.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetPriceMatrix( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetPriceMatrix )
  {
    uint32 issuerTariffId = osal_build_uint32( pInMsg->pData, 4 );

    pCBs->pfnPricing_GetPriceMatrix( issuerTariffId, &(pInMsg->msg->srcAddr),
                                               pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetBlockThresholds
 *
 * @brief   Process in the received Get Block Thresholds.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetBlockThresholds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetBlockThresholds )
  {
    uint32 issuerTariffId = osal_build_uint32( pInMsg->pData, 4 );

    pCBs->pfnPricing_GetBlockThresholds( issuerTariffId, &(pInMsg->msg->srcAddr),
                                                 pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetConversionFactor
 *
 * @brief   Process in the received Get Conversion Factor.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetConversionFactor( zclIncoming_t *pInMsg,
                                                                 zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetConversionFactor )
  {
    zclCCGetConversionFactor_t cmd;

    zclSE_ParseInCmd_GetConversionFactor( &cmd, &(pInMsg->pData[0]),
                                          pInMsg->pDataLen );

    pCBs->pfnPricing_GetConversionFactor( &cmd, &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetCalorificValue
 *
 * @brief   Process in the received Get Calorific Value.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetCalorificValue( zclIncoming_t *pInMsg,
                                                               zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetCalorificValue )
  {
    zclCCGetCalorificValue_t cmd;

    zclSE_ParseInCmd_GetCalorificValue( &cmd, &(pInMsg->pData[0]),
                                        pInMsg->pDataLen );

    pCBs->pfnPricing_GetCalorificValue( &cmd, &(pInMsg->msg->srcAddr),
                                        pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetCO2Value
 *
 * @brief   Process in the received Get CO2 Value.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetCO2Value( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetCO2Value )
  {
    zclCCGetCO2Value_t cmd;

    zclSE_ParseInCmd_GetCO2Value( &cmd, &(pInMsg->pData[0]),
                                  pInMsg->pDataLen );

    pCBs->pfnPricing_GetCO2Value( &cmd, &(pInMsg->msg->srcAddr),
                                  pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetBillingPeriod
 *
 * @brief   Process in the received Get Billing Period.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetBillingPeriod( zclIncoming_t *pInMsg,
                                                              zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetBillingPeriod )
  {
    zclCCGetBillingPeriod_t cmd;

    zclSE_ParseInCmd_GetBillingPeriod( &cmd, &(pInMsg->pData[0]),
                                       pInMsg->pDataLen );

    pCBs->pfnPricing_GetBillingPeriod( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_GetConsolidatedBill
 *
 * @brief   Process in the received Get Consolidated Bill.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_GetConsolidatedBill( zclIncoming_t *pInMsg,
                                                                 zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_GetConsolidatedBill )
  {
    zclCCGetConsolidatedBill_t cmd;

    zclSE_ParseInCmd_GetConsolidatedBill( &cmd, &(pInMsg->pData[0]),
                                          pInMsg->pDataLen );

    pCBs->pfnPricing_GetConsolidatedBill( &cmd, &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Pricing_CPPEventResponse
 *
 * @brief   Process in the received a CPP Event Response.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Pricing_CPPEventResponse( zclIncoming_t *pInMsg,
                                                              zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPricing_CPPEventResponse )
  {
    zclCCCPPEventResponse_t cmd;

    zclSE_ParseInCmd_CPPEventResponse( &cmd, &(pInMsg->pData[0]),
                                       pInMsg->pDataLen );

    pCBs->pfnPricing_CPPEventResponse( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}
#endif  // SE_UK_EXT
#endif  // ZCL_PRICING


#ifdef ZCL_MESSAGE
/*********************************************************************
 * @fn      zclSE_ProcessInMessageCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_ProcessInMessageCmds( zclIncoming_t *pInMsg,
                                              zclSE_AppCallbacks_t *pCBs )
{
  ZStatus_t stat;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Process Client commands, received by server
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_GET_LAST_MESSAGE:
        stat = zclSE_ProcessInCmd_Message_GetLastMessage( pInMsg, pCBs );
        break;

      case COMMAND_SE_MESSAGE_CONFIRMATION:
        stat = zclSE_ProcessInCmd_Message_MessageConfirmation( pInMsg, pCBs );
        break;

      default:
        stat = ZFailure;
        break;
    }
  }
  else
  {
    // Process Server commands, received by client
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_DISPLAY_MESSAGE:
        stat = zclSE_ProcessInCmd_Message_DisplayMessage( pInMsg, pCBs );
        break;

      case COMMAND_SE_CANCEL_MESSAGE:
        stat = zclSE_ProcessInCmd_Message_CancelMessage( pInMsg, pCBs );
        break;

      default:
        stat = ZFailure;
        break;
    }
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Message_DisplayMessage
 *
 * @brief   Process in the received Display Message Command.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Message_DisplayMessage( zclIncoming_t *pInMsg,
                                                             zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnMessage_DisplayMessage )
  {
    zclCCDisplayMessage_t cmd;

    if ( zclSE_ParseInCmd_DisplayMessage( &cmd,  &(pInMsg->pData[0]), pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnMessage_DisplayMessage( &cmd, &(pInMsg->msg->srcAddr),
                                    pInMsg->hdr.transSeqNum );

      // Free memory allocated in zclSE_ParseInCmd_DiplayMessage()
      if ( cmd.msgString.pStr != NULL )
      {
        osal_mem_free( cmd.msgString.pStr );
      }

      return ZSuccess;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Message_CancelMessage
 *
 * @brief   Process in the received Cancel Message Command.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Message_CancelMessage( zclIncoming_t *pInMsg,
                                                            zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnMessage_CancelMessage )
  {
    zclCCCancelMessage_t cmd;

    zclSE_ParseInCmd_CancelMessage( &cmd,  &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnMessage_CancelMessage( &cmd, &(pInMsg->msg->srcAddr),
                                   pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Message_GetLastMessage
 *
 * @brief   Process in the received Get Last Message Command.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Message_GetLastMessage( zclIncoming_t *pInMsg,
                                                             zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnMessage_GetLastMessage )
  {
    pCBs->pfnMessage_GetLastMessage( &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Message_MessageConfirmation
 *
 * @brief   Process in the received Message Confirmation.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Message_MessageConfirmation( zclIncoming_t *pInMsg,
                                                                 zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnMessage_MessageConfirmation )
  {
    zclCCMessageConfirmation_t cmd;

    zclSE_ParseInCmd_MessageConfirmation( &cmd, &(pInMsg->pData[0]),
                                          pInMsg->pDataLen );
    pCBs->pfnMessage_MessageConfirmation( &cmd, &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}
#endif  // ZCL_MESSAGE


#ifdef ZCL_LOAD_CONTROL
/*********************************************************************
 * @fn      zclSE_ProcessInLoadControlCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_ProcessInLoadControlCmds( zclIncoming_t *pInMsg,
                                                  zclSE_AppCallbacks_t *pCBs )
{
  ZStatus_t stat;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Process Client commands, received by server
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_REPORT_EVENT_STATUS:
        stat = zclSE_ProcessInCmd_LoadControl_ReportEventStatus( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_SCHEDULED_EVENT:
        stat = zclSE_ProcessInCmd_LoadControl_GetScheduledEvents( pInMsg, pCBs );
        break;

      default:
        stat = ZFailure;
        break;
    }
  }
  else
  {
    // Process Server commands, received by client
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_LOAD_CONTROL_EVENT:
        stat = zclSE_ProcessInCmd_LoadControl_LoadControlEvent( pInMsg, pCBs );
        break;

      case COMMAND_SE_CANCEL_LOAD_CONTROL_EVENT:
        stat = zclSE_ProcessInCmd_LoadControl_CancelLoadControlEvent( pInMsg, pCBs );
        break;

      case COMMAND_SE_CANCEL_ALL_LOAD_CONTROL_EVENT:
        stat = zclSE_ProcessInCmd_LoadControl_CancelAllLoadControlEvents( pInMsg, pCBs );
        break;

      default:
        stat = ZFailure;
        break;
    }
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_LoadControl_LoadControlEvent
 *
 * @brief   Process in the received Load Control Event.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 *                      ZCL_STATUS_INVALID_FIELD @ Invalid field value
 */
static ZStatus_t zclSE_ProcessInCmd_LoadControl_LoadControlEvent( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  uint8 status = ZSuccess;

  if ( pCBs->pfnLoadControl_LoadControlEvent )
  {
    zclCCLoadControlEvent_t cmd;

    zclSE_ParseInCmd_LoadControlEvent( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    // Range checking
    if ( cmd.durationInMinutes > MAX_DURATION_IN_MINUTES_SE_LOAD_CONTROL )
    {
      status = ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd.criticalityLevel > MAX_CRITICAL_LEVEL_SE_LOAD_CONTROL )
    {
      status = ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd. coolingTemperatureSetPoint != SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT &&
         cmd. coolingTemperatureSetPoint > MAX_TEMPERATURE_SETPOINT_SE_LOAD_CONTROL )
    {
      status = ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd. heatingTemperatureSetPoint != SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT &&
         cmd. heatingTemperatureSetPoint > MAX_TEMPERATURE_SETPOINT_SE_LOAD_CONTROL )
    {
      status = ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd.averageLoadAdjustmentPercentage != SE_OPTIONAL_FIELD_INT8 &&
        (cmd.averageLoadAdjustmentPercentage < MIN_AVERAGE_LOAD_ADJUSTMENT_PERCENTAGE_SE ||
         cmd.averageLoadAdjustmentPercentage > MAX_AVERAGE_LOAD_ADJUSTMENT_PERCENTAGE_SE ) )
    {
      status = ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd. dutyCycle != SE_OPTIONAL_FIELD_UINT8 &&
         cmd. dutyCycle > MAX_DUTY_CYCLE_SE_LOAD_CONTROL )
    {
      status = ZCL_STATUS_INVALID_FIELD;
    }

    // If any of the four fields is optional, set them all to optional
    if ( cmd.coolingTemperatureOffset == SE_OPTIONAL_FIELD_UINT8 ||
         cmd.heatingTemperatureOffset == SE_OPTIONAL_FIELD_UINT8 ||
         cmd.coolingTemperatureSetPoint == SE_OPTIONAL_FIELD_UINT16 ||
         cmd.heatingTemperatureSetPoint == SE_OPTIONAL_FIELD_UINT16 )
    {
      cmd.coolingTemperatureOffset = SE_OPTIONAL_FIELD_UINT8;
      cmd.heatingTemperatureOffset = SE_OPTIONAL_FIELD_UINT8;
      cmd.coolingTemperatureSetPoint = SE_OPTIONAL_FIELD_UINT16;
      cmd.heatingTemperatureSetPoint = SE_OPTIONAL_FIELD_UINT16;
    }

    pCBs->pfnLoadControl_LoadControlEvent( &cmd, &(pInMsg->msg->srcAddr), status, pInMsg->hdr.transSeqNum );

    // The Load Control Event command has response, therefore,
    // inform zclto not to send default response.
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;  // Not supported
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_LoadControl_CancelLoadControlEvent
 *
 * @brief   Process in the received Cancel Load Control Event.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_LoadControl_CancelLoadControlEvent( zclIncoming_t *pInMsg,
                                                                        zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnLoadControl_CancelLoadControlEvent )
  {
    zclCCCancelLoadControlEvent_t cmd;

    zclSE_ParseInCmd_CancelLoadControlEvent( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnLoadControl_CancelLoadControlEvent( &cmd, &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_LoadControl_CancelAllLoadControlEvent
 *
 * @brief   Process in the received Cancel All Load Control Event.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_LoadControl_CancelAllLoadControlEvents( zclIncoming_t *pInMsg,
                                                                             zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnLoadControl_CancelAllLoadControlEvents )
  {
    zclCCCancelAllLoadControlEvents_t cmd;

    cmd.cancelControl = pInMsg->pData[0];

    pCBs->pfnLoadControl_CancelAllLoadControlEvents( &cmd, &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_LoadControl_ReportEventStatus
 *
 * @brief   Process in the received Load Control Event.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 *                      ZCL_STATUS_INVALID_FIELD @ Range checking
 *                                           failure
 */
static ZStatus_t zclSE_ProcessInCmd_LoadControl_ReportEventStatus( zclIncoming_t *pInMsg,
                                                                   zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnLoadControl_ReportEventStatus )
  {

    zclCCReportEventStatus_t cmd;

    zclSE_ParseInCmd_ReportEventStatus( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    // Range Checking
    if ( cmd.eventStatus != EVENT_STATUS_LOAD_CONTROL_EVENT_REJECTED &&
        (cmd.eventStatus == 0 ||
         cmd.eventStatus > EVENT_STATUS_LOAD_CONTROL_EVENT_SUPERSEDED ) )
    {
      return ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd.criticalityLevelApplied > MAX_CRITICAL_LEVEL_SE_LOAD_CONTROL )
    {
      return ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd.coolingTemperatureSetPointApplied != SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT &&
         cmd.coolingTemperatureSetPointApplied > MAX_TEMPERATURE_SETPOINT_SE_LOAD_CONTROL )
    {
      return ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd.heatingTemperatureSetPointApplied != SE_OPTIONAL_FIELD_TEMPERATURE_SET_POINT &&
         cmd.heatingTemperatureSetPointApplied > MAX_TEMPERATURE_SETPOINT_SE_LOAD_CONTROL )
    {
      return ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd.averageLoadAdjustment != SE_OPTIONAL_FIELD_INT8 &&
        (cmd.averageLoadAdjustment < MIN_AVERAGE_LOAD_ADJUSTMENT_PERCENTAGE_SE ||
         cmd.averageLoadAdjustment > MAX_AVERAGE_LOAD_ADJUSTMENT_PERCENTAGE_SE ) )
    {
      return ZCL_STATUS_INVALID_FIELD;
    }

    if ( cmd.dutyCycleApplied != SE_OPTIONAL_FIELD_UINT8 &&
         cmd.dutyCycleApplied > MAX_DUTY_CYCLE_SE_LOAD_CONTROL )
    {
      return ZCL_STATUS_INVALID_FIELD;
    }

    pCBs->pfnLoadControl_ReportEventStatus( &cmd, &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_LoadControl_GetScheduledEvents
 *
 * @brief   Process in the received Get Scheduled Event.
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_LoadControl_GetScheduledEvents( zclIncoming_t *pInMsg,
                                                                    zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnLoadControl_GetScheduledEvents )
  {
    zclCCGetScheduledEvent_t cmd;

    cmd.startTime = osal_build_uint32( pInMsg->pData, 4);
    cmd.numEvents = pInMsg->pData[4];

    pCBs->pfnLoadControl_GetScheduledEvents( &cmd, &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

#endif  // ZCL_LOAD_CONTROL

#ifdef ZCL_PREPAYMENT
/*********************************************************************
 * @fn      zclSE_ProcessInPrepaymentCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_ProcessInPrepaymentCmds( zclIncoming_t *pInMsg,
                                                zclSE_AppCallbacks_t *pCBs )
{
  ZStatus_t stat;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Process Client commands, received by server
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_SEL_AVAIL_EMERGENCY_CREDIT:
        stat = zclSE_ProcessInCmd_Prepayment_SelAvailEmergencyCredit( pInMsg, pCBs );
        break;

#ifndef SE_UK_EXT   // this is SE 1.1 command definition
      case COMMAND_SE_CHANGE_SUPPLY:
        stat = zclSE_ProcessInCmd_Prepayment_ChangeSupply( pInMsg, pCBs );
        break;
#else
      case COMMAND_SE_CHANGE_DEBT:
        stat = zclSE_ProcessInCmd_Prepayment_ChangeDebt( pInMsg, pCBs );
        break;

      case COMMAND_SE_EMERGENCY_CREDIT_SETUP:
        stat = zclSE_ProcessInCmd_Prepayment_EmergencyCreditSetup( pInMsg, pCBs );
        break;

      case COMMAND_SE_CONSUMER_TOPUP:
        stat = zclSE_ProcessInCmd_Prepayment_ConsumerTopup( pInMsg, pCBs );
        break;

      case COMMAND_SE_CREDIT_ADJUSTMENT:
        stat = zclSE_ProcessInCmd_Prepayment_CreditAdjustment( pInMsg, pCBs );
        break;

      case COMMAND_SE_CHANGE_PAYMENT_MODE:
        stat = zclSE_ProcessInCmd_Prepayment_ChangePaymentMode( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_PREPAY_SNAPSHOT:
        stat = zclSE_ProcessInCmd_Prepayment_GetPrepaySnapshot( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_TOPUP_LOG:
        stat = zclSE_ProcessInCmd_Prepayment_GetTopupLog( pInMsg, pCBs );
        break;

      case COMMAND_SE_SET_LOW_CREDIT_WARNING_LEVEL:
        stat = zclSE_ProcessInCmd_Prepayment_SetLowCreditWarningLevel( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_DEBT_REPAYMENT_LOG:
        stat = zclSE_ProcessInCmd_Prepayment_GetDebtRepaymentLog( pInMsg, pCBs );
        break;
#endif  // SE_UK_EXT

      default:
        stat = ZFailure;
        break;
    }
  }
  else
  {
    // Process Server commands, received by client
    switch ( pInMsg->hdr.commandID )
    {
#ifndef SE_UK_EXT   // this is SE 1.1 command definition
      case COMMAND_SE_SUPPLY_STATUS_RESPONSE:
        stat = zclSE_ProcessInCmd_Prepayment_SupplyStatusResponse( pInMsg, pCBs );
        break;

#else // SE_UK_EXT
    case COMMAND_SE_GET_PREPAY_SNAPSHOT_RESPONSE:
        stat = zclSE_ProcessInCmd_Prepayment_GetPrepaySnapshotResponse( pInMsg, pCBs );
        break;

    case COMMAND_SE_CHANGE_PAYMENT_MODE_RESPONSE:
        stat = zclSE_ProcessInCmd_Prepayment_ChangePaymentModeResponse( pInMsg, pCBs );
        break;

    case COMMAND_SE_CONSUMER_TOPUP_RESPONSE:
        stat = zclSE_ProcessInCmd_Prepayment_ConsumerTopupResponse( pInMsg, pCBs );
        break;

    case COMMAND_SE_GET_COMMANDS:
        stat = zclSE_ProcessInCmd_Prepayment_GetCommands( pInMsg, pCBs );
        break;

    case COMMAND_SE_PUBLISH_TOPUP_LOG:
        stat = zclSE_ProcessInCmd_Prepayment_PublishTopupLog( pInMsg, pCBs );
        break;

    case COMMAND_SE_PUBLISH_DEBT_LOG:
        stat = zclSE_ProcessInCmd_Prepayment_PublishDebtLog( pInMsg, pCBs );
        break;
#endif  // SE_UK_EXT

      default:
        stat = ZFailure;
        break;
    }
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_SelAvailEmergencyCredit
 *
 * @brief   Process in the received Select Available Emergency Credit
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_SelAvailEmergencyCredit( zclIncoming_t *pInMsg,
                                                                    zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_SelAvailEmergencyCredit )
  {
    zclCCSelAvailEmergencyCredit_t cmd;

    zclSE_ParseInCmd_SelAvailEmergencyCredit( &cmd, &(pInMsg->pData[0]),
                                              pInMsg->pDataLen );

    // Callback to process message
    pCBs->pfnPrepayment_SelAvailEmergencyCredit( &cmd, &(pInMsg->msg->srcAddr),
                                                 pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

#ifndef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_ChangeSupply
 *
 * @brief   Process in the received Change Supply
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ChangeSupply( zclIncoming_t *pInMsg,
                                                             zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_ChangeSupply )
  {
    zclCCChangeSupply_t cmd;

    zclSE_ParseInCmd_ChangeSupply( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    // Callback to process message
    pCBs->pfnPrepayment_ChangeSupply( &cmd, &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_SupplyStatusResponse
 *
 * @brief   Process in the received Supply Status Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_SupplyStatusResponse( zclIncoming_t *pInMsg,
                                                                    zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_SupplyStatusResponse )
  {
    zclCCSupplyStatusResponse_t cmd;

    cmd.providerId = osal_build_uint32( pInMsg->pData, 4);
    cmd.implementationDateTime = osal_build_uint32( &pInMsg->pData[4], 4);
    cmd.supplyStatus = pInMsg->pData[8];

    pCBs->pfnPrepayment_SupplyStatusResponse( &cmd, &(pInMsg->msg->srcAddr),
                                              pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}

#else // SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_ChangeDebt
 *
 * @brief   Process in the received Change Debt
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ChangeDebt( zclIncoming_t *pInMsg,
                                                           zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_ChangeDebt )
  {
    zclCCChangeDebt_t cmd;

    zclSE_ParseInCmd_ChangeDebt( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnPrepayment_ChangeDebt( &cmd, &(pInMsg->msg->srcAddr),
                                    pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_EmergencyCreditSetup
 *
 * @brief   Process in the received Emergency Credit Setup
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_EmergencyCreditSetup( zclIncoming_t *pInMsg,
                                                                     zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_EmergencyCreditSetup )
  {
    zclCCEmergencyCreditSetup_t cmd;

    zclSE_ParseInCmd_EmergencyCreditSetup( &cmd, &(pInMsg->pData[0]),
                                           pInMsg->pDataLen );

    pCBs->pfnPrepayment_EmergencyCreditSetup( &cmd, &(pInMsg->msg->srcAddr),
                                              pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_ConsumerTopup
 *
 * @brief   Process in the received Consumer Topup
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ConsumerTopup( zclIncoming_t *pInMsg,
                                                              zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_ConsumerTopup )
  {
    zclCCConsumerTopup_t cmd;

    zclSE_ParseInCmd_ConsumerTopup( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnPrepayment_ConsumerTopup( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_CreditAdjustment
 *
 * @brief   Process in the received Credit Adjustment
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_CreditAdjustment( zclIncoming_t *pInMsg,
                                                                 zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_CreditAdjustment )
  {
    zclCCCreditAdjustment_t cmd;

    zclSE_ParseInCmd_CreditAdjustment( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnPrepayment_CreditAdjustment( &cmd, &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_ChangePaymentMode
 *
 * @brief   Process in the received Change Payment Mode
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ChangePaymentMode( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_ChangePaymentMode )
  {
    zclCCChangePaymentMode_t cmd;

    zclSE_ParseInCmd_ChangePaymentMode( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnPrepayment_ChangePaymentMode( &cmd, &(pInMsg->msg->srcAddr),
                                           pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_GetPrepaySnapshot
 *
 * @brief   Process in the received Get Prepay Snapshot
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetPrepaySnapshot( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_GetPrepaySnapshot )
  {
    zclCCGetPrepaySnapshot_t cmd;

    zclSE_ParseInCmd_GetPrepaySnapshot( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnPrepayment_GetPrepaySnapshot( &cmd, &(pInMsg->msg->srcAddr),
                                           pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_GetTopupLog
 *
 * @brief   Process in the received Get Topup Log
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetTopupLog( zclIncoming_t *pInMsg,
                                                            zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_GetTopupLog )
  {
    pCBs->pfnPrepayment_GetTopupLog( pInMsg->pData[0], &(pInMsg->msg->srcAddr),
                                     pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_SetLowCreditWarningLevel
 *
 * @brief   Process in the received Set Low Credit Warning Level
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_SetLowCreditWarningLevel( zclIncoming_t *pInMsg,
                                                                         zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_SetLowCreditWarningLevel )
  {
    pCBs->pfnPrepayment_SetLowCreditWarningLevel( pInMsg->pData[0],
                                                  &(pInMsg->msg->srcAddr),
                                                  pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_GetDebtRepaymentLog
 *
 * @brief   Process in the received Get Debt Repayment Log
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetDebtRepaymentLog( zclIncoming_t *pInMsg,
                                                                    zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_GetDebtRepaymentLog )
  {
    zclCCGetDebtRepaymentLog_t cmd;

    zclSE_ParseInCmd_GetDebtRepaymentLog( &cmd, &(pInMsg->pData[0]),
                                          pInMsg->pDataLen );

    pCBs->pfnPrepayment_GetDebtRepaymentLog( &cmd, &(pInMsg->msg->srcAddr),
                                             pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_GetPrepaySnapshotResponse
 *
 * @brief   Process in the received Get Prepay Snapshot Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetPrepaySnapshotResponse( zclIncoming_t *pInMsg,
                                                                          zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_GetPrepaySnapshotResponse )
  {
    zclCCGetPrepaySnapshotResponse_t cmd;

    zclSE_ParseInCmd_GetPrepaySnapshotResponse( &cmd, &(pInMsg->pData[0]),
                                                pInMsg->pDataLen );

    pCBs->pfnPrepayment_GetPrepaySnapshotResponse( &cmd, &(pInMsg->msg->srcAddr),
                                                   pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_ChangePaymentModeResponse
 *
 * @brief   Process in the received Change Payment Mode Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ChangePaymentModeResponse( zclIncoming_t *pInMsg,
                                                                          zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_ChangePaymentModeResponse )
  {
    zclCCChangePaymentModeResponse_t cmd;

    zclSE_ParseInCmd_ChangePaymentModeResponse( &cmd, &(pInMsg->pData[0]),
                                               pInMsg->pDataLen );

    pCBs->pfnPrepayment_ChangePaymentModeResponse( &cmd, &(pInMsg->msg->srcAddr),
                                                   pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_ConsumerTopupResponse
 *
 * @brief   Process in the received Consumer Topup Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_ConsumerTopupResponse( zclIncoming_t *pInMsg,
                                                                      zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_ConsumerTopupResponse )
  {
    zclCCConsumerTopupResponse_t cmd;

    zclSE_ParseInCmd_ConsumerTopupResponse( &cmd, &(pInMsg->pData[0]),
                                            pInMsg->pDataLen );

    pCBs->pfnPrepayment_ConsumerTopupResponse( &cmd, &(pInMsg->msg->srcAddr),
                                               pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_GetCommands
 *
 * @brief   Process in the received Get Commands
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_GetCommands( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_GetCommands )
  {
    pCBs->pfnPrepayment_GetCommands( pInMsg->pData[0], &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_PublishTopupLog
 *
 * @brief   Process in the received Publish Topup Log
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_PublishTopupLog( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_PublishTopupLog )
  {
    zclCCPublishTopupLog_t cmd;

    if ( zclSE_ParseInCmd_PublishTopupLog( &cmd, &(pInMsg->pData[0]),
                                                pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnPrepayment_PublishTopupLog( &cmd, &(pInMsg->msg->srcAddr),
                                           pInMsg->hdr.transSeqNum );

      // Free memory
      if ( cmd.pPayload != NULL )
      {
        osal_mem_free( cmd.pPayload );
      }

      return ZSuccess;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Prepayment_PublishDebtLog
 *
 * @brief   Process in the received Publish Debt Log
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Prepayment_PublishDebtLog( zclIncoming_t *pInMsg,
                                                               zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnPrepayment_PublishDebtLog )
  {
    zclCCPublishDebtLog_t cmd;

    if ( zclSE_ParseInCmd_PublishDebtLog( &cmd, &(pInMsg->pData[0]),
                                                pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnPrepayment_PublishDebtLog( &cmd, &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );

      // Free memory
      if ( cmd.pPayload != NULL )
      {
        osal_mem_free( cmd.pPayload );
      }

      return ZSuccess;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}
#endif  // SE_UK_EXT
#endif  // ZCL_PREPAYMENT

#ifdef ZCL_TUNNELING
/*********************************************************************
 * @fn      zclSE_ProcessInTunnelingCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_ProcessInTunnelingCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  ZStatus_t stat;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Process Client commands, received by Server
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_REQUEST_TUNNEL:
        stat = zclSE_ProcessInCmd_Tunneling_RequestTunnel( pInMsg, pCBs );
        break;

      case COMMAND_SE_CLOSE_TUNNEL:
        stat = zclSE_ProcessInCmd_Tunneling_CloseTunnel( pInMsg, pCBs );
        break;

      case COMMAND_SE_DATA_CLIENT_SERVER_DIR:
        stat = zclSE_ProcessInCmd_Tunneling_TransferData( pInMsg, pCBs );
        break;

      case COMMAND_SE_DATA_ERROR_CLIENT_SERVER_DIR:
        stat = zclSE_ProcessInCmd_Tunneling_TransferDataError( pInMsg, pCBs );
        break;

      case COMMAND_SE_ACK_CLIENT_SERVER_DIR:
        stat = zclSE_ProcessInCmd_Tunneling_AckTransferData( pInMsg, pCBs );
        break;

      case COMMAND_SE_READY_DATA_CLIENT_SERVER_DIR:
        stat = zclSE_ProcessInCmd_Tunneling_ReadyData( pInMsg, pCBs );
        break;

#ifdef SE_UK_EXT
      case COMMAND_SE_GET_SUPP_TUNNEL_PROTOCOLS:
        stat = zclSE_ProcessInCmd_Tunneling_GetSuppTunnelProt( pInMsg, pCBs );
        break;
#endif  //SE_UK_EXT

      default:
        stat = ZFailure;
        break;
    }
  }
  else
  {
    // Process Server commands, received by Client
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_REQUEST_TUNNEL_RESPONSE:
        stat = zclSE_ProcessInCmd_Tunneling_ReqTunnelRsp( pInMsg, pCBs );
        break;

      case COMMAND_SE_DATA_SERVER_CLIENT_DIR:
        stat = zclSE_ProcessInCmd_Tunneling_TransferData( pInMsg, pCBs );
        break;

      case COMMAND_SE_DATA_ERROR_SERVER_CLIENT_DIR:
        stat = zclSE_ProcessInCmd_Tunneling_TransferDataError( pInMsg, pCBs );
        break;

      case COMMAND_SE_ACK_SERVER_CLIENT_DIR:
        stat = zclSE_ProcessInCmd_Tunneling_AckTransferData( pInMsg, pCBs );
        break;

      case COMMAND_SE_READY_DATA_SERVER_CLIENT_DIR:
        stat = zclSE_ProcessInCmd_Tunneling_ReadyData( pInMsg, pCBs );
        break;

#ifdef SE_UK_EXT
      case COMMAND_SE_SUPP_TUNNEL_PROTOCOLS_RSP:
        stat = zclSE_ProcessInCmd_Tunneling_SuppTunnelProtRsp( pInMsg, pCBs );
        break;

      case COMMAND_SE_TUNNEL_CLOSURE_NOTIFICATION:
        stat = zclSE_ProcessInCmd_Tunneling_TunnelClosureNotification( pInMsg, pCBs );
        break;
#endif  // SE_UK_EXT
      default:
        stat = ZFailure;
        break;
    }
  }


  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_RequestTunnel
 *
 * @brief   process in the received tunnel request
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_RequestTunnel( zclIncoming_t *pInMsg,
                                                             zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_RequestTunnel )
  {
    zclCCRequestTunnel_t  cmd;

    cmd.protocolId = pInMsg->pData[0];
    cmd.manufacturerCode = BUILD_UINT16( pInMsg->pData[1], pInMsg->pData[2] );
    cmd.flowControlSupport = pInMsg->pData[3];
    cmd.maxInTransferSize = BUILD_UINT16( pInMsg->pData[4], pInMsg->pData[5] );

    pCBs->pfnTunneling_RequestTunnel( &cmd, &(pInMsg->msg->srcAddr),
                                      pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_ReqTunnelRsp
 *
 * @brief  process in the received tunnel response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_ReqTunnelRsp( zclIncoming_t *pInMsg,
                                                            zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_ReqTunnelRsp )
  {

    zclCCReqTunnelRsp_t cmd;

    cmd.tunnelId = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );
    cmd.tunnelStatus = pInMsg->pData[2];
    cmd.maxInTransferSize = BUILD_UINT16( pInMsg->pData[3], pInMsg->pData[4] );

    pCBs->pfnTunneling_ReqTunnelRsp( &cmd, &(pInMsg->msg->srcAddr),
                                     pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_CloseTunnel
 *
 * @brief   process in the received tunnel close
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_CloseTunnel( zclIncoming_t *pInMsg,
                                                           zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_CloseTunnel )
  {
    zclCCCloseTunnel_t cmd;

    cmd.tunnelId = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );

    pCBs->pfnTunneling_CloseTunnel( &cmd, &(pInMsg->msg->srcAddr),
                                    pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_TransferData
 *
 * @brief   process in the received tunnel transfer data
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_TransferData( zclIncoming_t *pInMsg,
                                                            zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_TransferData )
  {
    zclCCTransferData_t cmd;
    uint16 dataLen = pInMsg->pDataLen - PACKET_LEN_SE_TUNNELING_TRANSFER_DATA;

    zclSE_ParseInCmd_TransferData( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnTunneling_TransferData( &cmd, &(pInMsg->msg->srcAddr),
                                     pInMsg->hdr.commandID, dataLen,
                                     pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_TransferDataError
 *
 * @brief   process in the transfer data error
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_TransferDataError( zclIncoming_t *pInMsg,
                                                                 zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_TransferDataError )
  {
    zclCCTransferDataError_t cmd;

    cmd.tunnelId = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );
    cmd.transferDataStatus = pInMsg->pData[2];

    pCBs->pfnTunneling_TransferDataError( &cmd, &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_AckTransferData
 *
 * @brief   Process in the received Ack Transfer Data
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_AckTransferData( zclIncoming_t *pInMsg,
                                                               zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_AckTransferData )
  {
    zclCCAckTransferData_t cmd;

    cmd.tunnelId = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );
    cmd.numberOfBytesLeft = BUILD_UINT16( pInMsg->pData[2], pInMsg->pData[3] );

    pCBs->pfnTunneling_AckTransferData( &cmd, &(pInMsg->msg->srcAddr),
                                        pInMsg->hdr.commandID,
                                        pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_ReadyData
 *
 * @brief   Process in the received Ready Data
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_ReadyData( zclIncoming_t *pInMsg,
                                                         zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_ReadyData )
  {
    zclCCReadyData_t cmd;

    cmd.tunnelId = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );
    cmd.numberOfOctetsLeft = BUILD_UINT16( pInMsg->pData[2], pInMsg->pData[3] );

    pCBs->pfnTunneling_ReadyData( &cmd, &(pInMsg->msg->srcAddr),
                                  pInMsg->hdr.commandID,
                                  pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_GetSuppTunnelProt
 *
 * @brief   Process in the received Get Supported Tunnel Protocols
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_GetSuppTunnelProt( zclIncoming_t *pInMsg,
                                                                 zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_GetSuppTunnelProt )
  {
    zclCCGetSuppTunnProt_t cmd;

    cmd.protocolOffset = pInMsg->pData[0];

    pCBs->pfnTunneling_GetSuppTunnelProt( &cmd, &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_SuppTunnelProtRsp
 *
 * @brief   Process in the received Supported Tunnel Protocols Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_SuppTunnelProtRsp( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_SuppTunnelProtRsp )
  {
    zclCCSuppTunnProtRsp_t *pCmd;
    uint8 i;
    uint8 *buf;

    pCmd = ( zclCCSuppTunnProtRsp_t * )osal_mem_alloc( sizeof( zclCCSuppTunnProtRsp_t ) +
                                                    (sizeof( zclCCProtocolPayload_t ) * pInMsg->pData[1]) );
    if ( pCmd != NULL )
    {
      buf = &(pInMsg->pData[0]);

      pCmd->protocolListComp = *buf++;
      pCmd->protocolCount = *buf++;

      for ( i = 0; i < pCmd->protocolCount; i++ )
      {
        pCmd->protocol[i].manufacturerCode = BUILD_UINT16( buf[0], buf[1] );
        buf += 2;
        pCmd->protocol[i].protocolId = *buf++;
      }

      pCBs->pfnTunneling_SuppTunnelProtRsp( pCmd, &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );

      osal_mem_free( pCmd );

      return ZCL_STATUS_CMD_HAS_RSP;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tunneling_TunnelClosureNotification
 *
 * @brief   Process in the received Tunnel Closure Notification
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tunneling_TunnelClosureNotification( zclIncoming_t *pInMsg,
                                                                         zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTunneling_TunnelClosureNotification )
  {
    zclCCTunnelClosureNotification_t cmd;

    cmd.tunnelId = BUILD_UINT16( pInMsg->pData[0], pInMsg->pData[1] );

    pCBs->pfnTunneling_TunnelClosureNotification( &cmd, &(pInMsg->msg->srcAddr),
                                                  pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}
#endif  // SE_UK_EXT
#endif  // ZCL_TUNNELING

#ifdef ZCL_TOU
#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ProcessInTouCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_ProcessInTouCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  ZStatus_t stat;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Process Client commands, received by Server
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_GET_CALENDAR:
        stat = zclSE_ProcessInCmd_Tou_GetCalendar( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_DAY_PROFILES:
        stat = zclSE_ProcessInCmd_Tou_GetDayProfiles( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_WEEK_PROFILES:
        stat = zclSE_ProcessInCmd_Tou_GetWeekProfiles( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_SEASONS:
        stat = zclSE_ProcessInCmd_Tou_GetSeasons( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_SPECIAL_DAYS:
        stat = zclSE_ProcessInCmd_Tou_GetSpecialDays( pInMsg, pCBs );
        break;

      default:
        stat = ZFailure;
        break;
    }
  }
  else
  {
    // Process Server commands, received by Client
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_PUBLISH_CALENDAR:
        stat = zclSE_ProcessInCmd_Tou_PublishCalendar( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_DAY_PROFILE:
        stat = zclSE_ProcessInCmd_Tou_PublishDayProfile( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_WEEK_PROFILE:
        stat = zclSE_ProcessInCmd_Tou_PublishWeekProfile( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_SEASONS:
        stat = zclSE_ProcessInCmd_Tou_PublishSeasons( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_SPECIAL_DAYS:
        stat = zclSE_ProcessInCmd_Tou_PublishSpecialDays( pInMsg, pCBs );
        break;

      default:
        stat = ZFailure;
        break;
    }
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_PublishCalendar
 *
 * @brief   process in the received Publish Calendar
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishCalendar( zclIncoming_t *pInMsg,
                                                         zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_PublishCalendar )
  {
    zclCCPublishCalendar_t cmd;

    zclSE_ParseInCmd_PublishCalendar( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnTou_PublishCalendar( &cmd, &(pInMsg->msg->srcAddr),
                                  pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_PublishDayProfile
 *
 * @brief   process in the received Publish Day Profile
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishDayProfile( zclIncoming_t *pInMsg,
                                                           zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_PublishDayProfile )
  {
    zclCCPublishDayProfile_t cmd;

    if ( zclSE_ParseInCmd_PublishDayProfile( &cmd, &(pInMsg->pData[0]),
                                                pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnTou_PublishDayProfile( &cmd, &(pInMsg->msg->srcAddr),
                                      pInMsg->hdr.transSeqNum );

      // Free the memory allocated in zclSE_ParseInCmd_PublishDayProfile()
      if ( cmd.pScheduleEntries != NULL )
      {
        osal_mem_free( cmd.pScheduleEntries );
      }

      return ZSuccess;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_PublishWeekProfile
 *
 * @brief   process in the received Publish Week Profile
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishWeekProfile( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_PublishWeekProfile )
  {
    zclCCPublishWeekProfile_t cmd;

    // Parse the command buffer
    cmd.issuerCalendarId = osal_build_uint32( pInMsg->pData, 4 );

    cmd.weekId = pInMsg->pData[4];
    cmd.dayIdRefMonday    = pInMsg->pData[5];
    cmd.dayIdRefTuestday  = pInMsg->pData[6];
    cmd.dayIdRefWednesday = pInMsg->pData[7];
    cmd.dayIdRefThursday  = pInMsg->pData[8];
    cmd.dayIdRefFriday    = pInMsg->pData[9];
    cmd.dayIdRefSaturday  = pInMsg->pData[10];
    cmd.dayIdRefSunday    = pInMsg->pData[11];

    pCBs->pfnTou_PublishWeekProfile( &cmd, &(pInMsg->msg->srcAddr),
                                     pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_PublishSeasons
 *
 * @brief   process in the received Publish Seasons
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishSeasons( zclIncoming_t *pInMsg,
                                                        zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_PublishSeasons )
  {
    zclCCPublishSeasons_t cmd;

    // Parse the command
    if ( zclSE_ParseInCmd_PublishSeasons( &cmd, &(pInMsg->pData[0]),
                                                pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnTou_PublishSeasons( &cmd, &(pInMsg->msg->srcAddr),
                                   pInMsg->hdr.transSeqNum );

      // Free the memory allocated in zclSE_ParseInCmd_PublishSeasons()
      if ( cmd.pSeasonEntry != NULL )
      {
        osal_mem_free( cmd.pSeasonEntry );
      }

      return ZSuccess;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_PublishSpecialDays
 *
 * @brief   process in the received Publish Special Days
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 *                      ZCL_STATUS_SOFTWARE_FAILURE @ ZStack memory allocation failure
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_PublishSpecialDays( zclIncoming_t *pInMsg,
                                                            zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_PublishSpecialDays )
  {
    zclCCPublishSpecialDays_t cmd;

    // Parse the command
    if ( zclSE_ParseInCmd_PublishSpecialDays( &cmd, &(pInMsg->pData[0]),
                                                pInMsg->pDataLen ) == ZSuccess )
    {
      pCBs->pfnTou_PublishSpecialDays( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );

      // Free the memory allocated in zclSE_ParseInCmd_PublishSpecialDays()
      if ( cmd.pSpecialDayEntry != NULL )
      {
        osal_mem_free( cmd.pSpecialDayEntry );
      }

      return ZSuccess;
    }
    else
    {
      return ZCL_STATUS_SOFTWARE_FAILURE;
    }
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_GetCalendar
 *
 * @brief   process in the received Get Calendar
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_GetCalendar( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_GetCalendar )
  {
    zclCCGetCalendar_t cmd;

    // Parse the command buffer
    cmd.startTime = osal_build_uint32( pInMsg->pData, 4 );

    cmd.numOfCalendars = pInMsg->pData[4];
    cmd.calendarType   = pInMsg->pData[5];

    pCBs->pfnTou_GetCalendar( &cmd, &(pInMsg->msg->srcAddr),
                              pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_GetDayProfiles
 *
 * @brief   process in the received Get Day Profiles
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_GetDayProfiles( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_GetDayProfiles )
  {
    zclCCGetDayProfiles_t cmd;

    // Parse the command buffer
    cmd.issuerCalendarId = osal_build_uint32( pInMsg->pData, 4 );

    pCBs->pfnTou_GetDayProfiles( &cmd, &(pInMsg->msg->srcAddr),
                                 pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_GetWeekProfiles
 *
 * @brief   process in the received Get Week Profiles
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_GetWeekProfiles( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_GetWeekProfiles )
  {
    zclCCGetWeekProfiles_t cmd;

    // Parse the command buffer
    cmd.issuerCalendarId = osal_build_uint32( pInMsg->pData, 4 );

    pCBs->pfnTou_GetWeekProfiles( &cmd, &(pInMsg->msg->srcAddr),
                                  pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_GetSeasons
 *
 * @brief   process in the received Get Seasons
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_GetSeasons( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_GetSeasons )
  {
    zclCCGetSeasons_t cmd;

    // Parse the command buffer
    cmd.issuerCalendarId = osal_build_uint32( pInMsg->pData, 4 );

    pCBs->pfnTou_GetSeasons( &cmd, &(pInMsg->msg->srcAddr),
                             pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_Tou_GetSpecialDays
 *
 * @brief   process in the received Get Special Days
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_Tou_GetSpecialDays( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnTou_GetSpecialDays )
  {
    zclCCGetSpecialDays_t cmd;

    // Parse the command buffer
    cmd.startTime = osal_build_uint32( pInMsg->pData, 4 );

    cmd.numOfEvents  = pInMsg->pData[4];
    cmd.calendarType = pInMsg->pData[5];

    pCBs->pfnTou_GetSpecialDays( &cmd, &(pInMsg->msg->srcAddr),
                                 pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}
#endif  // SE_UK_EXT
#endif  // ZCL_TOU

#ifdef ZCL_DEVICE_MGMT
#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ProcessInDeviceMgmtCmds
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library on a command ID basis
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSE_ProcessInDeviceMgmtCmds( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  ZStatus_t stat;

  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Process Client commands, received by Server
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_GET_CHANGE_OF_TENANCY:
        stat = zclSE_ProcessInCmd_DeviceMgmt_GetChangeTenancy( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_CHANGE_OF_SUPPLIER:
        stat = zclSE_ProcessInCmd_DeviceMgmt_GetChangeSupplier( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_CHANGE_SUPPLY:
        stat = zclSE_ProcessInCmd_DeviceMgmt_GetChangeSupply( pInMsg, pCBs );
        break;

      case COMMAND_SE_SUPPLY_STATUS_RESPONSE:
        stat = zclSE_ProcessInCmd_DeviceMgmt_SupplyStatusResponse( pInMsg, pCBs );
        break;

      case COMMAND_SE_GET_PASSWORD:
        stat = zclSE_ProcessInCmd_DeviceMgmt_GetPassword( pInMsg, pCBs );
        break;

      default:
        stat = ZFailure;
        break;
    }
  }
  else
  {
    // Process Server commands, received by Client
    switch ( pInMsg->hdr.commandID )
    {
      case COMMAND_SE_PUBLISH_CHANGE_OF_TENANCY:
        stat = zclSE_ProcessInCmd_DeviceMgmt_PublishChangeTenancy( pInMsg, pCBs );
        break;

      case COMMAND_SE_PUBLISH_CHANGE_OF_SUPPLIER:
        stat = zclSE_ProcessInCmd_DeviceMgmt_PublishChangeSupplier( pInMsg, pCBs );
        break;

      case COMMAND_SE_CHANGE_SUPPLY:
        stat = zclSE_ProcessInCmd_DeviceMgmt_ChangeSupply( pInMsg, pCBs );
        break;

      case COMMAND_SE_CHANGE_PASSWORD:
        stat = zclSE_ProcessInCmd_DeviceMgmt_ChangePassword( pInMsg, pCBs );
        break;

      case COMMAND_SE_LOCAL_CHANGE_SUPPLY:
        stat = zclSE_ProcessInCmd_DeviceMgmt_LocalChangeSupply( pInMsg, pCBs );
        break;

      default:
        stat = ZFailure;
        break;
    }
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_GetChangeTenancy
 *
 * @brief   Process in the received Get Change Of Tenancy
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_GetChangeTenancy( zclIncoming_t *pInMsg,
                                                                 zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_GetChangeTenancy )
  {
    pCBs->pfnDeviceMgmt_GetChangeTenancy( &(pInMsg->msg->srcAddr),
                                          pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_GetChangeSupplier
 *
 * @brief   Process in the received Get Change Of Supplier
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_GetChangeSupplier( zclIncoming_t *pInMsg,
                                                                  zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_GetChangeSupplier )
  {
    pCBs->pfnDeviceMgmt_GetChangeSupplier( &(pInMsg->msg->srcAddr),
                                           pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_GetChangeSupply
 *
 * @brief   Process in the received Get Change Supply
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_GetChangeSupply( zclIncoming_t *pInMsg,
                                                                zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_GetChangeSupply )
  {
    pCBs->pfnDeviceMgmt_GetChangeSupply( &(pInMsg->msg->srcAddr),
                                           pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_SupplyStatusResponse
 *
 * @brief   Process in the received Supply Status Response
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and send default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_SupplyStatusResponse( zclIncoming_t *pInMsg,
                                                                     zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_SupplyStatusResponse )
  {
    zclCCSupplyStatusResponse_t cmd;

    cmd.supplierId = osal_build_uint32( pInMsg->pData, 4);
    cmd.issuerEventId = osal_build_uint32( &pInMsg->pData[4], 4);
    cmd.implementationDateTime = osal_build_uint32( &pInMsg->pData[8], 4);
    cmd.supplyStatus = pInMsg->pData[12];

    pCBs->pfnDeviceMgmt_SupplyStatusResponse( &cmd, &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_GetPassword
 *
 * @brief   Process in the received Get Password
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_GetPassword( zclIncoming_t *pInMsg,
                                                            zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_GetPassword )
  {
    zclCCGetPassword_t cmd;

    cmd.passwordLevel = pInMsg->pData[0];

    pCBs->pfnDeviceMgmt_GetPassword( &cmd, &(pInMsg->msg->srcAddr), pInMsg->hdr.transSeqNum );

    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_PublishChangeTenancy
 *
 * @brief   Process in the received Publish Change of Tenancy
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_PublishChangeTenancy( zclIncoming_t *pInMsg,
                                                                     zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_PublishChangeTenancy )
  {
    zclCCPublishChangeTenancy_t cmd;

    // Parse the command
    zclSE_ParseInCmd_PublishChangeTenancy( &cmd, &(pInMsg->pData[0]),
                                          pInMsg->pDataLen );

    pCBs->pfnDeviceMgmt_PublishChangeTenancy( &cmd, &(pInMsg->msg->srcAddr),
                                             pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_PublishChangeSupplier
 *
 * @brief   Process in the received Publish Change of Supplier
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_PublishChangeSupplier( zclIncoming_t *pInMsg,
                                                                      zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_PublishChangeSupplier )
  {
    zclCCPublishChangeSupplier_t cmd;

    // Parse the command
    zclSE_ParseInCmd_PublishChangeSupplier( &cmd, &(pInMsg->pData[0]),
                                           pInMsg->pDataLen );

    pCBs->pfnDeviceMgmt_PublishChangeSupplier( &cmd, &(pInMsg->msg->srcAddr),
                                              pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_ChangeSupply
 *
 * @brief   Process in the received Change Supply
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZCL_STATUS_CMD_HAS_RSP @ Supported and do
 *                                           not need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_ChangeSupply( zclIncoming_t *pInMsg,
                                                             zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_ChangeSupply )
  {
    zclCCChangeSupply_t cmd;

    // Parse the command
    zclSE_ParseInCmd_ChangeSupply( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnDeviceMgmt_ChangeSupply( &cmd, &(pInMsg->msg->srcAddr),
                                      pInMsg->hdr.transSeqNum );
    return ZCL_STATUS_CMD_HAS_RSP;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_ChangePassword
 *
 * @brief   Process in the received Change Password
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_ChangePassword( zclIncoming_t *pInMsg,
                                                               zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_ChangePassword )
  {
    zclCCChangePassword_t cmd;

    // Parse the command
    zclSE_ParseInCmd_ChangePassword( &cmd, &(pInMsg->pData[0]), pInMsg->pDataLen );

    pCBs->pfnDeviceMgmt_ChangePassword( &cmd, &(pInMsg->msg->srcAddr),
                                       pInMsg->hdr.transSeqNum );
    return ZSuccess;
  }

  return ZFailure;
}

/*********************************************************************
 * @fn      zclSE_ProcessInCmd_DeviceMgmt_LocalChangeSupply
 *
 * @brief   Process in the received Local Change Supply
 *
 * @param   pInMsg - pointer to the incoming message
 * @param   pCBs - pointer to the application call back function
 *
 * @return  ZStatus_t - ZFailure @ Unsupported
 *                      ZSuccess @ Supported and need default rsp
 */
static ZStatus_t zclSE_ProcessInCmd_DeviceMgmt_LocalChangeSupply( zclIncoming_t *pInMsg, zclSE_AppCallbacks_t *pCBs )
{
  if ( pCBs->pfnDeviceMgmt_LocalChangeSupply )
  {
    zclCCLocalChangeSupply_t cmd;

    cmd.propSupplyStatus = pInMsg->pData[0];

    pCBs->pfnDeviceMgmt_LocalChangeSupply( &cmd, &(pInMsg->msg->srcAddr),
                                           pInMsg->hdr.transSeqNum );

    return ZSuccess;
  }

  return ZFailure;
}
#endif  // SE_UK_EXT
#endif  // ZCL_DEVICE_MGMT

#ifdef ZCL_PRICING
/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishPrice
 *
 * @brief   Parse received Publish Price Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_PublishPrice( zclCCPublishPrice_t *pCmd, uint8 *buf, uint8 len )
{
  uint8 originalLen; // stores octet string original length

  // Parse the command buffer
  pCmd->providerId = osal_build_uint32( buf, 4 );
  buf += 4;

  // Notice that rate label is a variable length UTF-8 string
  pCmd->rateLabel.strLen = *buf++;
  if ( pCmd->rateLabel.strLen == SE_OPTIONAL_FIELD_UINT8 )
  {
    // If character count is 0xFF, set string length to 0
    pCmd->rateLabel.strLen = 0;
  }

  if ( pCmd->rateLabel.strLen != 0 )
  {
    originalLen = pCmd->rateLabel.strLen; //save original length

    // truncate rate label to maximum size
    if ( pCmd->rateLabel.strLen > (SE_RATE_LABEL_LEN-1) )
    {
      pCmd->rateLabel.strLen = (SE_RATE_LABEL_LEN-1);
    }

    pCmd->rateLabel.pStr = osal_mem_alloc( pCmd->rateLabel.strLen );
    if ( pCmd->rateLabel.pStr == NULL )
    {
      return ZMemError;
    }
    osal_memcpy( pCmd->rateLabel.pStr, buf, pCmd->rateLabel.strLen );
    buf += originalLen; // move pointer original length of received string
  }
  else
  {
    pCmd->rateLabel.pStr = NULL;
  }

  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->currentTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->unitOfMeasure = *buf++;
  pCmd->currency = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->priceTrailingDigit = *buf++;
  pCmd->numberOfPriceTiers = *buf++;
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->durationInMinutes = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->price = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->priceRatio = *buf++;
  pCmd->generationPrice = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->generationPriceRatio = *buf++;

  // SE 1.1 fields
  if ((len - pCmd->rateLabel.strLen) > PACKET_LEN_SE_PUBLISH_PRICE_SE_1_0)
  {
    pCmd->alternateCostDelivered = osal_build_uint32( buf, 4 );
    buf += 4;

    pCmd->alternateCostUnit = *buf++;

    pCmd->alternateCostTrailingDigit = *buf++;

    pCmd->numberOfBlockThresholds = *buf++;

    pCmd->priceControl = *buf;
  }
  else
  {
    // for backwards compatibility with SE 1.0
    pCmd->alternateCostDelivered = SE_OPTIONAL_FIELD_UINT32;
    pCmd->alternateCostUnit = SE_OPTIONAL_FIELD_UINT8;
    pCmd->alternateCostTrailingDigit = SE_OPTIONAL_FIELD_UINT8;
    pCmd->numberOfBlockThresholds = SE_OPTIONAL_FIELD_UINT8;
    pCmd->priceControl = SE_OPTIONAL_FIELD_UINT8;
  }

  return ZSuccess;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishBlockPeriod
 *
 * @brief   Parse received Publish Block Period Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishBlockPeriod( zclCCPublishBlockPeriod_t *pCmd,
                                          uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->providerId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->blockPeriodStartTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->blockPeriodDurInMins = osal_build_uint32( buf, 3 );
  buf += 3;

  pCmd->numPriceTiersAndBlock = *buf++;

#ifdef SE_UK_EXT
  pCmd->tariffType = *buf++;
#endif

  pCmd->blockPeriodControl = *buf;
}

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishTariffInfomation
 *
 * @brief   Parse received Publish Tariff Info.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishTariffInformation( zclCCPublishTariffInformation_t *pCmd,
                                                uint8 *buf, uint8 len )
{
  uint8 fieldLen;
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->supplierId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->issuerTariffId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->tariffType = *buf++;

  fieldLen = zclSE_Parse_UTF8String(buf, &pCmd->tarifLabel, SE_TARIFF_LABEL_LEN);
  buf += fieldLen;

  pCmd->numPriceTiersInUse = *buf++;
  pCmd->numBlockThresholdsInUse = *buf++;
  pCmd->unitOfMeasure = *buf++;
  pCmd->currency = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;
  pCmd->priceTrailingDigit = *buf++;
  pCmd->standingCharge = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->tierBlockMode = *buf++;
  pCmd->blockThresholdMask = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;
  pCmd->BlockThresholdMultiplier = osal_build_uint32( buf, 3 );
  buf += 3;
  pCmd->BlockThresholdDivisor = osal_build_uint32( buf, 3 );
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishPriceMatrix
 *
 * @brief   Parse received Publish Price Matrix.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_PublishPriceMatrix( zclCCPublishPriceMatrix_t *pCmd,
                                               uint8 *buf, uint8 len )
{
  // Parse the command buffer
  pCmd->issuerTariffId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->commandIndex = *buf++;
  pCmd->numElements = (len - PACKET_LEN_SE_MIN_PUBLISH_PRICE_MATRIX) / sizeof (uint32);
  pCmd->pTierBlockPrice = NULL;

  if ( pCmd->numElements )
  {
    pCmd->pTierBlockPrice = osal_mem_alloc(sizeof (uint32) * pCmd->numElements);

    if ( pCmd->pTierBlockPrice == NULL )
    {
      return ZMemError;
    }

    uint8 i;

    for ( i = 0; i < pCmd->numElements; i++ )
    {
      pCmd->pTierBlockPrice[i] = osal_build_uint32( buf, 4 );
      buf += 4;
    }
  }

  return ZSuccess;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishBlockThresholds
 *
 * @brief   Parse received Publish Block Thresholds.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_PublishBlockThresholds( zclCCPublishBlockThresholds_t *pCmd,
                                                   uint8 *buf, uint8 len )
{
  // Parse the command buffer
  pCmd->issuerTariffId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->commandIndex = *buf++;
  pCmd->numElements = (len - PACKET_LEN_SE_MIN_PUBLISH_BLOCK_THRESHOLD) / 6;
  pCmd->pTierBlockThreshold = NULL;

  if ( pCmd->numElements )
  {
    pCmd->pTierBlockThreshold = osal_mem_alloc(6 * pCmd->numElements);

    if ( pCmd->pTierBlockThreshold == NULL )
    {
      return ZMemError;
    }

    uint8 i;

    for ( i = 0; i < pCmd->numElements; i++ )
    {
      osal_memcpy( pCmd->pTierBlockThreshold[i], buf, 6 );
      buf += 6;
    }
  }

  return ZSuccess;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishConversionFactor
 *
 * @brief   Parse received Publish Conversion Factor.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishConversionFactor( zclCCPublishConversionFactor_t *pCmd,
                                               uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->conversionFactor = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->trailingDigit = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishCalorificValue
 *
 * @brief   Parse received Publish Calorific Value.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishCalorificValue( zclCCPublishCalorificValue_t *pCmd,
                                             uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->calorificValue = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->calorificValueUnit = *buf++;
  pCmd->trailingDigit = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishCO2Value
 *
 * @brief   Parse received Publish CO2 Value.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishCO2Value( zclCCPublishCO2Value_t *pCmd,
                                       uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->tariffType = *buf++;
  pCmd->CO2Value = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->CO2ValueUnit = *buf++;
  pCmd->trailingDigit = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishCPPEvent
 *
 * @brief   Parse received Publish CPP Event.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishCPPEvent( zclCCPublishCPPEvent_t *pCmd,
                                       uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->durationInMinutes = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;
  pCmd->tariffType = *buf++;
  pCmd->CPPPriceTier = *buf++;
  pCmd->CPPAuth = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishBillingPeriod
 *
 * @brief   Parse received Publish Billing Period.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishBillingPeriod( zclCCPublishBillingPeriod_t *pCmd,
                                            uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->duration = osal_build_uint32( buf, 3 );
  buf += 3;
  pCmd->tariffType = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishConsolidatedBill
 *
 * @brief   Parse received Publish Consolidated Bill.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishConsolidatedBill( zclCCPublishConsolidatedBill_t *pCmd,
                                               uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->duration = osal_build_uint32( buf, 3 );
  buf += 3;
  pCmd->tariffType = *buf++;
  pCmd->consolidatedBill = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->currency = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;
  pCmd->trailingDigit = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishCreditPaymentInfo
 *
 * @brief   Parse received Publish Credit Payment Info.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishCreditPaymentInfo( zclCCPublishCreditPaymentInfo_t *pCmd,
                                                uint8 *buf, uint8 len )
{
  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->paymentDueDate = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->creditPaymentOverdueAmt = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->creditPaymentStatus = *buf++;
  pCmd->creditPayment = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->creditPaymentDate = osal_build_uint32( buf, 4 );
  buf += 4;
  (void)zclSE_Parse_UTF8String(buf, &pCmd->creditPaymentRef, SE_CREDIT_PAYMENT_REF_LEN);
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetTariffInformation
 *
 * @brief   Parse received Get Tariff Information.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetTariffInformation( zclCCGetTariffInformation_t *pCmd,
                                            uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->numEvents = *buf++;
  pCmd->tariffType = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetConversionFactor
 *
 * @brief   Parse received Get Conversion Factor.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetConversionFactor( zclCCGetConversionFactor_t *pCmd,
                                           uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->numEvents = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetCalorificValue
 *
 * @brief   Parse received Get Calorific Value.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetCalorificValue( zclCCGetCalorificValue_t *pCmd,
                                         uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->numEvents = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetCO2Value
 *
 * @brief   Parse received Get CO2 Value.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetCO2Value( zclCCGetCO2Value_t *pCmd, uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->numEvents = *buf++;
  pCmd->tariffType = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetBillingPeriod
 *
 * @brief   Parse received Get Billing Period.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetBillingPeriod( zclCCGetBillingPeriod_t *pCmd,
                                        uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->numEvents = *buf++;
  pCmd->tariffType = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetConsolidatedBill
 *
 * @brief   Parse received Get Consolidated Bill.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetConsolidatedBill( zclCCGetConsolidatedBill_t *pCmd,
                                           uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->numEvents = *buf++;
  pCmd->tariffType = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_CPPEventResponse
 *
 * @brief   Parse received CPP Event Response.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_CPPEventResponse( zclCCCPPEventResponse_t *pCmd,
                                        uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->CPPAuth = *buf;
}
#endif  // SE_UK_EXT
#endif  // ZCL_PRICING

#ifdef ZCL_MESSAGE
/*********************************************************************
 * @fn      zclSE_ParseInCmd_DisplayMessage
 *
 * @brief   Parse received Display Message Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_DisplayMessage( zclCCDisplayMessage_t *pCmd, uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->messageId = osal_build_uint32( buf, 4 );

  // Message control bitmap
  osal_memset ( &(pCmd->messageCtrl), 0, sizeof( zclMessageCtrl_t ) );

  pCmd->messageCtrl.transmissionMode = buf[4] & 0x03;    // bit 0&1
  pCmd->messageCtrl.importance = ( buf[4] >> SE_PROFILE_MSGCTRL_IMPORTANCE ) & 0x03; // bit 2&3
#if defined ( SE_UK_EXT )
  pCmd->messageCtrl.pinRequired = ( buf[4] >> SE_PROFILE_MSGCTRL_PINREQUIRED ) & 0x01;  // bit 4
  pCmd->messageCtrl.acceptanceRequired = ( buf[4] >> SE_PROFILE_MSGCTRL_ACCEPTREQUIRED ) & 0x01;  // bit 5
#endif
  pCmd->messageCtrl.confirmationRequired = ( buf[4] >> SE_PROFILE_MSGCTRL_CONFREQUIRED ) & 0x01;  // bit 7

  pCmd->startTime = osal_build_uint32( &(buf[5]), 4 );
  pCmd->durationInMinutes = BUILD_UINT16( buf[9], buf[10] );
  pCmd->msgString.strLen = buf[11];

  // Copy the message string
  if ( pCmd->msgString.strLen != 0 )
  {
    pCmd->msgString.pStr = osal_mem_alloc( pCmd->msgString.strLen );
    if ( pCmd->msgString.pStr == NULL )
    {
      return ZMemError;
    }
    osal_memcpy( pCmd->msgString.pStr, &(buf[12]), pCmd->msgString.strLen );
  }
  else
  {
    pCmd->msgString.pStr = NULL;
  }

  return ZSuccess;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_CancelMessage
 *
 * @brief   Parse received Cancel Message Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_CancelMessage( zclCCCancelMessage_t *pCmd, uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->messageId = osal_build_uint32( buf, 4 );

  // Message control bitmap
  osal_memset ( &(pCmd->messageCtrl), 0, sizeof( zclMessageCtrl_t ) );

  pCmd->messageCtrl.transmissionMode = buf[4] & 0x03;    // bit 0&1
  pCmd->messageCtrl.importance = ( buf[4] >> SE_PROFILE_MSGCTRL_IMPORTANCE ) & 0x03; // bit 2&3
#if defined ( SE_UK_EXT )
  pCmd->messageCtrl.pinRequired = ( buf[4] >> SE_PROFILE_MSGCTRL_PINREQUIRED ) & 0x01;  // bit 4
  pCmd->messageCtrl.acceptanceRequired = ( buf[4] >> SE_PROFILE_MSGCTRL_ACCEPTREQUIRED ) & 0x01;  // bit 5
#endif
  pCmd->messageCtrl.confirmationRequired = ( buf[4] >> SE_PROFILE_MSGCTRL_CONFREQUIRED ) & 0x01;  // bit 7
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_MessageConfirmation
 *
 * @brief   Parse received Message Confirmation Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_MessageConfirmation( zclCCMessageConfirmation_t *pCmd,
                                           uint8 *buf, uint8 len )
{
  pCmd->messageId = osal_build_uint32( buf, 4 );
  pCmd->confirmTime = osal_build_uint32( &(buf[4]), 4 );

#if defined ( SE_UK_EXT )
  pCmd->msgString.strLen = buf[8];
#else
  pCmd->msgString.strLen = 0;
#endif

  // Point to the Message Response string
  if ( pCmd->msgString.strLen != 0 )
  {
    pCmd->msgString.pStr = &(buf[9]);
  }
  else
  {
    pCmd->msgString.pStr = NULL;
  }
}
#endif  // ZCL_MESSAGE

#ifdef ZCL_LOAD_CONTROL
/*********************************************************************
 * @fn      zclSE_ParseInCmd_LoadControlEvent
 *
 * @brief   Parse received Load Control Event.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_LoadControlEvent( zclCCLoadControlEvent_t *pCmd,
                                        uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Maybe add checking for buffer length later
  // Skipped right now to leave MT input to guarantee
  // proper buffer length
  pCmd->issuerEvent = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->deviceGroupClass = osal_build_uint32( buf, 3 );
  buf += 3;

  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->durationInMinutes = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->criticalityLevel = *buf++;
  pCmd->coolingTemperatureOffset = *buf++;
  pCmd->heatingTemperatureOffset = *buf++;

  pCmd->coolingTemperatureSetPoint = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->heatingTemperatureSetPoint = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->averageLoadAdjustmentPercentage = *buf++;
  pCmd->dutyCycle = *buf++;
  pCmd->eventControl = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_CancelLoadControlEvent
 *
 * @brief   Parse received Cancel Load Control Event Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_CancelLoadControlEvent( zclCCCancelLoadControlEvent_t *pCmd,
                                              uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Maybe add checking for buffer length later
  // Skipped right now to leave MT input to guarantee
  // proper buffer length
  pCmd->issuerEventID = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->deviceGroupClass = osal_build_uint32( buf, 3 );
  buf += 3;

  pCmd->cancelControl = *buf++;
  pCmd->effectiveTime = osal_build_uint32( buf, 4 );
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_ReportEventStatus
 *
 * @brief   Parse received Report Event Status.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ReportEventStatus( zclCCReportEventStatus_t *pCmd,
                                         uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Maybe add checking for buffer length later
  // Skipped right now to leave MT input to guarantee
  // proper buffer length
  pCmd->issuerEventID = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->eventStatus = *buf++;

  pCmd->eventStartTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->criticalityLevelApplied = *buf++;
  pCmd->coolingTemperatureSetPointApplied = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->heatingTemperatureSetPointApplied = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->averageLoadAdjustment = *buf++;
  pCmd->dutyCycleApplied = *buf++;
  pCmd->eventControl = *buf++;
  pCmd->signatureType = *buf++;

  osal_memcpy( pCmd->signature, buf, SE_PROFILE_SIGNATURE_LENGTH );
}
#endif  // ZCL_LOAD_CONTROL

#ifdef ZCL_PREPAYMENT
/*********************************************************************
 * @fn      zclSE_ParseInCmd_SelAvailEmergencyCredit
 *
 * @brief   Parse received Select Available Emergency Credit Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_SelAvailEmergencyCredit( zclCCSelAvailEmergencyCredit_t *pCmd,
                                               uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter
  uint8 originalLen; // stores octet string original length

  // Parse the command buffer
  pCmd->commandDateTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->originatingDevice = *buf++;

  // Notice that site ID is a variable length UTF-8 string
  pCmd->siteId.strLen = *buf++;
  if ( pCmd->siteId.strLen == SE_OPTIONAL_FIELD_UINT8 )
  {
    // If character count is 0xFF, set string length to 0
    pCmd->siteId.strLen = 0;
  }

  if ( pCmd->siteId.strLen != 0 )
  {
    originalLen = pCmd->siteId.strLen; //save original length

    // truncate Site ID to maximum size
    if ( pCmd->siteId.strLen > (SE_SITE_ID_LEN-1) )
    {
      pCmd->siteId.strLen = (SE_SITE_ID_LEN-1);
    }

    pCmd->siteId.pStr = buf;

    buf += originalLen; // move pointer original length of received string
  }
  else
  {
    pCmd->siteId.pStr = NULL;
  }

  // Notice that meterSerialNumber is a variable length UTF-8 string
  pCmd->meterSerialNumber.strLen = *buf++;
  if ( pCmd->meterSerialNumber.strLen == SE_OPTIONAL_FIELD_UINT8 )
  {
    // If character count is 0xFF, set string length to 0
    pCmd->meterSerialNumber.strLen = 0;
  }

  if ( pCmd->meterSerialNumber.strLen != 0 )
  {
    originalLen = pCmd->meterSerialNumber.strLen; //save original length

    // truncate Meter Serial Number to maximum size
    if ( pCmd->meterSerialNumber.strLen > (SE_METER_SERIAL_NUM_LEN-1) )
    {
      pCmd->meterSerialNumber.strLen = (SE_METER_SERIAL_NUM_LEN-1);
    }

    pCmd->meterSerialNumber.pStr = buf;

    buf += originalLen; // move pointer original length of received string
  }
  else
  {
    pCmd->meterSerialNumber.pStr = NULL;
  }
}

#ifndef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ParseInCmd_ChangeSupply
 *
 * @brief   Parse received Change Supply Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ChangeSupply( zclCCChangeSupply_t *pCmd, uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter
  uint8 originalLen; // stores octet string original length

  // Parse the command buffer
  pCmd->providerId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->requestDateTime = osal_build_uint32( buf, 4 );
  buf += 4;

  // Notice that site ID is a variable length UTF-8 string
  pCmd->siteId.strLen = *buf++;
  if ( pCmd->siteId.strLen == SE_OPTIONAL_FIELD_UINT8 )
  {
    // If character count is 0xFF, set string length to 0
    pCmd->siteId.strLen = 0;
  }

  if ( pCmd->siteId.strLen != 0 )
  {
    originalLen = pCmd->siteId.strLen; //save original length

    // truncate Site ID to maximum size
    if ( pCmd->siteId.strLen > (SE_SITE_ID_LEN-1) )
    {
      pCmd->siteId.strLen = (SE_SITE_ID_LEN-1);
    }

    pCmd->siteId.pStr = buf;

    buf += originalLen; // move pointer original length of received string
  }
  else
  {
    pCmd->siteId.pStr = NULL;
  }

  // Notice that meterSerialNumber is a variable length UTF-8 string
  pCmd->meterSerialNumber.strLen = *buf++;
  if ( pCmd->meterSerialNumber.strLen == SE_OPTIONAL_FIELD_UINT8 )
  {
    // If character count is 0xFF, set string length to 0
    pCmd->meterSerialNumber.strLen = 0;
  }

  if ( pCmd->meterSerialNumber.strLen != 0 )
  {
    originalLen = pCmd->meterSerialNumber.strLen; //save original length

    // truncate Meter Serial Number to maximum size
    if ( pCmd->meterSerialNumber.strLen > (SE_METER_SERIAL_NUM_LEN-1) )
    {
      pCmd->meterSerialNumber.strLen = (SE_METER_SERIAL_NUM_LEN-1);
    }

    pCmd->meterSerialNumber.pStr = buf;

    buf += originalLen; // move pointer original length of received string
  }
  else
  {
    pCmd->meterSerialNumber.pStr = NULL;
  }

  pCmd->implementationDateTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->proposedSupplyStatus = *buf++;

  pCmd->origIdSupplyControlBits = *buf;
}
#endif  // not defined SE_UK_EXT

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ParseInCmd_ChangeDebt
 *
 * @brief   Parse received Change Debt Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ChangeDebt( zclCCChangeDebt_t *pCmd, uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter
  uint8 fieldLen;

  // Parse the command buffer
  pCmd->cmdIssueTime = osal_build_uint32( buf, 4 );
  buf += 4;

  fieldLen = zclSE_Parse_UTF8String(buf, &pCmd->debtLabel, SE_DEBT_LABEL_LEN);

  buf += fieldLen;

  pCmd->debtAmount = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->debtRecoveryMethod = *buf++;
  pCmd->debtType = *buf++;

  pCmd->recoveryStartTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->debtRecoveryCollectionTime = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->debtRecoveryFrequency = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->debtRecoveryAmt = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->debtRecoveryBalancePct = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->debtRecoveryMaxMissed = *buf++;

  (void)zclSE_Parse_UTF8String(buf, &pCmd->signature, SE_SIGNATURE_LEN);
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_EmergencyCreditSetup
 *
 * @brief   Parse received Emergency Credit Setup Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_EmergencyCreditSetup( zclCCEmergencyCreditSetup_t *pCmd,
                                            uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->cmdIssueTime = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->emergencyCreditLimit = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->emergencyCreditThreshold = osal_build_uint32( buf, 4 );
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_ConsumerTopup
 *
 * @brief   Parse received Consumer Topup Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ConsumerTopup( zclCCConsumerTopup_t *pCmd, uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->originatingDevice = *buf++;

  (void)zclSE_Parse_UTF8String(buf, &pCmd->topupCode, SE_TOPUP_CODE_LEN);
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_CreditAdjustment
 *
 * @brief   Parse received Credit Adjustment Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_CreditAdjustment( zclCCCreditAdjustment_t *pCmd,
                                        uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->cmdIssueTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->creditAdjustmentType = *buf++;

  osal_memcpy( pCmd->creditAdjustmentValue, buf, 6 );
  buf += 6;

  (void)zclSE_Parse_UTF8String(buf, &pCmd->signature, SE_SIGNATURE_LEN);
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_ChangePaymentMode
 *
 * @brief   Parse received Change Payment Mode Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ChangePaymentMode( zclCCChangePaymentMode_t *pCmd,
                                         uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->supplierId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->modeEventId = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->implementationDate = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->proposedPaymentControl = *buf++;

  pCmd->cutOffValue = osal_build_uint32( buf, 4 );
  buf += 4;

  (void)zclSE_Parse_UTF8String(buf, &pCmd->signature, SE_SIGNATURE_LEN);
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetPrepaySnapshot
 *
 * @brief   Parse received Get Prepay Snapshot Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetPrepaySnapshot( zclCCGetPrepaySnapshot_t *pCmd,
                                         uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->startTime = osal_build_uint32( buf, 4 );
  pCmd->numberOfSnapshots = buf[4];
  pCmd->snapshotCause = BUILD_UINT16( buf[5], buf[6] );
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetDebtRepaymentLog
 *
 * @brief   Parse received Get Debt Repayment Log Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetDebtRepaymentLog( zclCCGetDebtRepaymentLog_t *pCmd,
                                           uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->numberOfDebt = buf[0];
  pCmd->debtType = buf[1];
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_GetPrepaySnapshotResponse
 *
 * @brief   Parse received Get Prepay Snapshot Response Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_GetPrepaySnapshotResponse( zclCCGetPrepaySnapshotResponse_t *pCmd,
                                                 uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->eventIssuerId = osal_build_uint32( buf, 4 );
  pCmd->snapshotTime = osal_build_uint32( buf+4, 4 );
  pCmd->commandIndex = buf[8];
  pCmd->snapshotCause = BUILD_UINT16( buf[9], buf[10] );
  pCmd->snapshotPayloadType = buf[11];
  buf += 12;

  if ( pCmd->snapshotPayloadType == SE_SNAPSHOT_TYPE_DEBIT_CREDIT_ADDITION )
  {
    pCmd->payload.type1DebtRemaining = osal_build_uint32( buf, 4 );
    pCmd->payload.type2DebtRemaining = osal_build_uint32( buf+4, 4 );
    pCmd->payload.type3DebtRemaining = osal_build_uint32( buf+8, 4 );
    pCmd->payload.emergencyCreditRemaining = osal_build_uint32( buf+12, 4 );
    pCmd->payload.creditRemaining = osal_build_uint32( buf+16, 4 );
  }
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_ChangePaymentModeResponse
 *
 * @brief   Parse received Change Payment Mode Response Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ChangePaymentModeResponse( zclCCChangePaymentModeResponse_t *pCmd,
                                                 uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->friendlyCredit = *buf++;
  pCmd->friendlyCreditCalendar = osal_build_uint32( buf, 4 );
  pCmd->emergencyCreditLimit = osal_build_uint32( buf+4, 4 );
  pCmd->cmergencyCreditThreshold = osal_build_uint32( buf+8, 4 );
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_ConsumerTopupResponse
 *
 * @brief   Parse received ConsumerTopupResponse Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ConsumerTopupResponse( zclCCConsumerTopupResponse_t *pCmd,
                                                 uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  pCmd->resultType = *buf++;
  pCmd->topupValue = osal_build_uint32( buf, 4 );
  buf += 4;
  pCmd->sourceofTopup = *buf++;
  pCmd->creditRemaining = osal_build_uint32( buf, 4 );
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishTopupLog
 *
 * @brief   Parse received Publish Topup Log Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_PublishTopupLog( zclCCPublishTopupLog_t *pCmd,
                                            uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter
  uint8 i, pos, numCodes = 0;

  // Count the number of strings in the message
  pos = 2;
  while ( pos < len )
  {
    if (buf[pos] == 0)
    {
      break;
    }

    pos += buf[pos] + 1;
    numCodes++;
  }

  pCmd->cmdIndex = *buf++;
  pCmd->totalCmds = *buf++;
  pCmd->numCodes = numCodes;

  if ( numCodes )
  {
    pCmd->pPayload = osal_mem_alloc( sizeof(UTF8String_t) * numCodes );

    if ( pCmd->pPayload == NULL )
    {
      return ZMemError;
    }

    for ( i = 0; i < numCodes; i++ )
    {
      uint8 fieldLen = zclSE_Parse_UTF8String(buf, &pCmd->pPayload[i], SE_TOPUP_CODE_LEN);

      buf += fieldLen;
    }
  }

  return ZSuccess;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishDebtLog
 *
 * @brief   Parse received Publish Debt Log Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_PublishDebtLog( zclCCPublishDebtLog_t *pCmd,
                                           uint8 *buf, uint8 len )
{
  uint8 i;
  uint8 numDebts = (len - 2) / 13;

  pCmd->cmdIndex = *buf++;
  pCmd->totalCmds = *buf++;
  pCmd->numDebts = numDebts;

  if ( numDebts )
  {
    pCmd->pPayload = osal_mem_alloc( sizeof(zclCCDebtPayload_t) * numDebts );

    if ( pCmd->pPayload == NULL )
    {
      return ZMemError;
    }

    for ( i = 0; i < numDebts; i++ )
    {
      pCmd->pPayload[i].collectionTime = osal_build_uint32( buf, 4 );
      pCmd->pPayload[i].amountCollected = osal_build_uint32( buf+4, 4 );
      pCmd->pPayload[i].debtType = buf[8];
      pCmd->pPayload[i].outstandingDebt = osal_build_uint32( buf+9, 4 );
      buf += 13;
    }
  }

  return ZSuccess;
}
#endif  // SE_UK_EXT
#endif  // ZCL_PREPAYMENT

#ifdef ZCL_TUNNELING
/*********************************************************************
 * @fn      zclSE_ParseInCmd_TransferData
 *
 * @brief   Parse received Transfer Data Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_TransferData( zclCCTransferData_t *pCmd, uint8 *buf, uint8 len )
{
  pCmd->tunnelId = BUILD_UINT16( buf[0], buf[1] );
  buf += 2;

  pCmd->data = buf;
}
#endif //  ZCL_TUNNELING

#ifdef ZCL_TOU
#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishCalendar
 *
 * @brief   Parse received Publish Calendar Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishCalendar( zclCCPublishCalendar_t *pCmd,
                                       uint8 *buf, uint8 len )
{
  uint8 originalLen; // stores octet string original length

  // Parse the command buffer
  pCmd->issuerCalendarId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->calendarType = *buf++;
  pCmd->calendarTimeRef = *buf++;

  // Notice that calendarName is a variable length UTF-8 string
  pCmd->calendarName.strLen = *buf++;
  if ( pCmd->calendarName.strLen == SE_OPTIONAL_FIELD_UINT8 )
  {
    // If character count is 0xFF, set string length to 0
    pCmd->calendarName.strLen = 0;
  }

  if ( pCmd->calendarName.strLen != 0 )
  {
    originalLen = pCmd->calendarName.strLen; //save original length

    // truncate rate label to maximum size
    if ( pCmd->calendarName.strLen > (SE_CALENDAR_NAME_LEN-1) )
    {
      pCmd->calendarName.strLen = (SE_CALENDAR_NAME_LEN-1);
    }

    pCmd->calendarName.pStr = buf;

    buf += originalLen; // move pointer original length of received string
  }
  else
  {
    pCmd->calendarName.pStr = NULL;
  }

  pCmd->numOfSeasons = *buf++;
  pCmd->numOfWeekProfiles = *buf++;
  pCmd->numOfDayProfiles = *buf;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishDayProfile
 *
 * @brief   Parse received Publish Day Profile Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_PublishDayProfile( zclCCPublishDayProfile_t *pCmd,
                                              uint8 *buf, uint8 len )
{
  // Parse the command buffer
  pCmd->issuerCalendarId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->dayId = *buf++;
  pCmd->totalNumSchedEnt = *buf++;
  pCmd->commandIndex = *buf++;

  pCmd->numTransferEntries = (len - PACKET_LEN_SE_PUBLISH_DAY_PROFILE) / SE_DAY_SCHEDULE_ENTRY_LEN;
  if (pCmd->numTransferEntries)
  {
    if ( pCmd->issuerCalendarId <= SE_CALENDAR_TYPE_IMPORT_EXPORT_CALENDAR )
    {
      pCmd->pScheduleEntries = osal_mem_alloc( pCmd->numTransferEntries * sizeof( zclCCRateEntry_t ) );

      if ( pCmd->pScheduleEntries != NULL )
      {
        zclCCRateEntry_t *pRateEntry = (zclCCRateEntry_t *)pCmd->pScheduleEntries;
        uint8 i;

        for ( i = 0; i < pCmd->numTransferEntries; i++ )
        {
          pRateEntry->startTime = BUILD_UINT16( buf[0], buf[1] );
          buf += 2;

          pRateEntry->activePriceTier = *buf++;

          pRateEntry++;
        }
      }
      else
      {
        return ZMemError;
      }
    }
    else
    {
      pCmd->pScheduleEntries = osal_mem_alloc( pCmd->numTransferEntries * sizeof( zclCCFriendlyCreditEntry_t ) );

      if ( pCmd->pScheduleEntries != NULL )
      {
        zclCCFriendlyCreditEntry_t *pFriendlyEntry = (zclCCFriendlyCreditEntry_t *)pCmd->pScheduleEntries;
        uint8 i;

        for ( i = 0; i < pCmd->numTransferEntries; i++ )
        {
          pFriendlyEntry->startTime = BUILD_UINT16( buf[0], buf[1] );
          buf += 2;

          pFriendlyEntry->friendCreditEnable = *buf++;

          pFriendlyEntry++;
        }
      }
      else
      {
        return ZMemError;
      }
    }
  }

  return ZSuccess;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishSeasons
 *
 * @brief   Parse received Publish Seasons Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_PublishSeasons( zclCCPublishSeasons_t *pCmd, uint8 *buf, uint8 len )
{
  // Parse the command buffer
  pCmd->issuerCalendarId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->commandIndex = *buf++;

  pCmd->numTransferEntries = ( len - PACKET_LEN_SE_PUBLISH_SEASONS ) / SE_SEASON_ENTRY_LEN;

  if ( pCmd->numTransferEntries )
  {
    pCmd->pSeasonEntry = (zclCCSeasonEntry_t *)osal_mem_alloc( pCmd->numTransferEntries *
                                                               sizeof( zclCCSeasonEntry_t ) );

    if ( pCmd->pSeasonEntry != NULL )
    {
      uint8 i;

      for ( i = 0; i < pCmd->numTransferEntries; i++ )
      {
        pCmd->pSeasonEntry[i].seasonStartDate = osal_build_uint32( buf, 4 );
        buf += 4;

        pCmd->pSeasonEntry[i].weekIdRef = *buf++;
      }
    }
    else
    {
      return ZMemError;
    }
  }

  return ZSuccess;
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishSpecialDays
 *
 * @brief   Parse received Publish Special Days Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  ZStatus_t - ZSuccess @ Parse successful
 *                      ZMemError @ Memory allocation failure
 */
ZStatus_t zclSE_ParseInCmd_PublishSpecialDays( zclCCPublishSpecialDays_t *pCmd,
                                               uint8 *buf, uint8 len )
{
  // Parse the command buffer
  pCmd->issuerEventId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->startTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->calendarType = *buf++;
  pCmd->totalNumSpecialDays = *buf++;
  pCmd->commandIndex = *buf++;

  pCmd->numTransferEntries = ( len - PACKET_LEN_SE_PUBLISH_SPECIAL_DAYS ) / SE_SPECIAL_DAY_ENTRY_LEN;

  if ( pCmd->numTransferEntries )
  {
    pCmd->pSpecialDayEntry = (zclCCSpecialDayEntry_t *)osal_mem_alloc( pCmd->numTransferEntries *
                                                                       sizeof( zclCCSpecialDayEntry_t ) );

    if ( pCmd->pSpecialDayEntry != NULL )
    {
      uint8 i;

      for ( i = 0; i < pCmd->numTransferEntries; i++ )
      {
        pCmd->pSpecialDayEntry[i].specialDayDate = osal_build_uint32( buf, 4 );
        buf += 4;

        pCmd->pSpecialDayEntry[i].dayIdRef = *buf++;
      }
    }
    else
    {
      return ZMemError;
    }
  }

  return ZSuccess;
}
#endif  // SE_UK_EXT
#endif  // ZCL_TOU

#ifdef ZCL_DEVICE_MGMT
#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishChangeTenancy
 *
 * @brief   Parse received Publish Change of Tenancy Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishChangeTenancy( zclCCPublishChangeTenancy_t *pCmd,
                                            uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->supplierId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->eventId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->tariffType = *buf++;

  pCmd->implementationDateTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->propTenencyChangeCtrl = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->signature.strLen = *buf++;;

  // Point to the Signature string
  if ( pCmd->signature.strLen != 0 )
  {
    pCmd->signature.pStr = buf;
  }
  else
  {
    pCmd->signature.pStr = NULL;
  }
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_PublishChangeSupplier
 *
 * @brief   Parse received Publish Change of Supplier Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_PublishChangeSupplier( zclCCPublishChangeSupplier_t *pCmd,
                                             uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->supplierId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->eventId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->tariffType = *buf++;

  pCmd->propSupplierId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->suppIdImplemDateTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->supplierChangeCtrl = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->supplierIdName.strLen = *buf++;;

  // Point to the Supplier ID Name string
  if ( pCmd->supplierIdName.strLen != 0 )
  {
    uint8 originalLen; // stores octet string original length

    originalLen = pCmd->supplierIdName.strLen; //save original length

    // truncate SupplierIdName to maximum size
    if ( pCmd->supplierIdName.strLen > (SE_SUPPLIER_ID_NAME_LEN-1) )
    {
      pCmd->supplierIdName.strLen = (SE_SUPPLIER_ID_NAME_LEN-1);
    }

    pCmd->supplierIdName.pStr = buf;
    buf += originalLen; // move pointer original length of received string
  }
  else
  {
    pCmd->supplierIdName.pStr = NULL;
  }

  pCmd->signature.strLen = *buf++;;

  // Point to the Signature string
  if ( pCmd->signature.strLen != 0 )
  {
    pCmd->signature.pStr = buf;
  }
  else
  {
    pCmd->signature.pStr = NULL;
  }
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_ChangeSupply
 *
 * @brief   Parse received Change Supply Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ChangeSupply( zclCCChangeSupply_t *pCmd, uint8 *buf, uint8 len )
{
  (void)len;  // Intentionally unreferenced parameter

  // Parse the command buffer
  pCmd->supplierId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->eventId = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->requestDateTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->implementationDateTime = osal_build_uint32( buf, 4 );
  buf += 4;

  pCmd->proposedSupplyStatus = *buf++;

  pCmd->origIdSupplyControlBits = *buf++;

  pCmd->signature.strLen = *buf++;;

  // Point to the Signature string
  if ( pCmd->signature.strLen != 0 )
  {
    pCmd->signature.pStr = buf;
  }
  else
  {
    pCmd->signature.pStr = NULL;
  }
}

/*********************************************************************
 * @fn      zclSE_ParseInCmd_ChangePassword
 *
 * @brief   Parse received Change Password Command.
 *
 * @param   pCmd - pointer to the output data struct
 * @param   buf - pointer to the input data buffer
 * @param   len - length of the input buffer
 *
 * @return  none
 */
void zclSE_ParseInCmd_ChangePassword( zclCCChangePassword_t *pCmd, uint8 *buf, uint8 len )
{
  pCmd->passwordLevel = *buf++;

  pCmd->password.strLen = *buf++;;

  // Point to the Signature string
  if ( pCmd->password.strLen != 0 )
  {
    // truncate password to maximum size
    if ( pCmd->password.strLen > (SE_PASSWORD_LEN-1) )
    {
      pCmd->password.strLen = (SE_PASSWORD_LEN-1);
    }

    pCmd->password.pStr = buf;
  }
  else
  {
    pCmd->password.pStr = NULL;
  }
}
#endif  // SE_UK_EXT
#endif  // ZCL_DEVICE_MGMT

#ifdef SE_UK_EXT
/*********************************************************************
 * @fn      zclSE_Parse_UTF8String
 *
 * @brief   Called to parse a UTF8String from a message
 *
 * @param   pBuf - pointer to the incoming message
 * @param   pString - pointer to the UTF8String_t
 * @param   maxLen - max length of the string field in pBuf
 *
 * @return  uint8 - number of bytes parsed from pBuf
 */
static uint8 zclSE_Parse_UTF8String( uint8 *pBuf, UTF8String_t *pString, uint8 maxLen )
{
  uint8 originalLen = 0;

  pString->strLen = *pBuf++;
  if ( pString->strLen == SE_OPTIONAL_FIELD_UINT8 )
  {
    // If character count is 0xFF, set string length to 0
    pString->strLen = 0;
  }

  if ( pString->strLen != 0 )
  {
    originalLen = pString->strLen; //save original length

    // truncate to maximum size
    if ( pString->strLen > (maxLen-1) )
    {
      pString->strLen = (maxLen-1);
    }

    pString->pStr = pBuf;
  }
  else
  {
    pString->pStr = NULL;
  }

  return originalLen + 1;
}
#endif  // SE_UK_EXT
/********************************************************************************************
*********************************************************************************************/
