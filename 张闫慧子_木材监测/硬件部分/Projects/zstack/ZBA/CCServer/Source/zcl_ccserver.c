/**************************************************************************************************
  Filename:       zcl_ccserver.c
  Revised:        $Date: 2011-06-03 13:17:26 -0700 (Fri, 03 Jun 2011) $
  Revision:       $Revision: 26202 $

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
  This device will be like an On/Off Switch device. This application
  is not intended to be a On/Off Switch device, but will use the device
  description to implement this sample code.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "ZDApp.h"

#include "zcl.h"
#include "zba.h"
#include "zcl_cc.h"

#include "zcl_ccserver.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"


/*********************************************************************
 * MACROS
 */
#define nullExtendedPANID( EPID )      osal_isbufset( (EPID), 0x00, Z_EXTADDR_LEN )
#define nullKey( key )                 osal_isbufset( (key), 0x00, SEC_KEY_LEN )

#define NvIdFromAttrId( attrId )       ( (attrId) == ATTRID_CC_TRUST_CENTER_MASTER_KEY ? \
                                         ZCD_NV_SAS_CURR_TC_MASTER_KEY : \
                                         (attrId) == ATTRID_CC_NETWORK_KEY ? \
                                         ZCD_NV_SAS_CURR_NWK_KEY : \
                                         ZCD_NV_SAS_CURR_PRECFG_LINK_KEY )

/*********************************************************************
 * CONSTANTS
 */
#define TO_JOIN_OPERATIONAL_NWK        0x01
#define TO_JOIN_COMMISSIONING_NWK      0x02

#define CCSERVER_REJOIN_TIMEOUT        2500 // 2.5 sec

/*********************************************************************
 * TYPEDEFS
 */
typedef struct zgItem
{
  uint16 id;
  uint16 len;
  void *buf;
} nvItem_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 zclCCServer_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8 leaveInitiated;
static zclCCRestartDevice_t restartDevice;

/*********************************************************************
 * ZCL CC Item Table
 */
static CONST nvItem_t nvItemTable[] =
{
  {
    ZCD_NV_SAS_SHORT_ADDR, sizeof( zclCCServer_ShortAddress ), &zclCCServer_ShortAddress
  },
  {
    ZCD_NV_SAS_EXT_PANID, Z_EXTADDR_LEN, zclCCServer_ExtendedPanId
  },
  {
    ZCD_NV_SAS_PANID, sizeof( zclCCServer_PanId ), &zclCCServer_PanId
  },
  {
    ZCD_NV_SAS_CHANNEL_MASK,  sizeof( zclCCServer_ChannelMask ), &zclCCServer_ChannelMask
  },
  {
    ZCD_NV_SAS_PROTOCOL_VER, sizeof( zclCCServer_ProtocolVersion ), &zclCCServer_ProtocolVersion
  },
  {
    ZCD_NV_SAS_STACK_PROFILE, sizeof( zclCCServer_StackProfile ), &zclCCServer_StackProfile
  },
  {
    ZCD_NV_SAS_STARTUP_CTRL, sizeof( zclCCServer_StartUpControl ), &zclCCServer_StartUpControl
  },
  {
    ZCD_NV_SAS_TC_ADDR, Z_EXTADDR_LEN, zclCCServer_TrustCenterAddr
  },
  {
    ZCD_NV_SAS_USE_INSEC_JOIN, sizeof( zclCCServer_UseInsecureJoin ), &zclCCServer_UseInsecureJoin
  },
  {
    ZCD_NV_SAS_NWK_KEY_SEQ_NUM, sizeof(zclCCServer_NetworkKeySeqNum ), &zclCCServer_NetworkKeySeqNum
  },
  {
    ZCD_NV_SAS_NWK_KEY_TYPE, sizeof( zclCCServer_NetworkKeyType ), &zclCCServer_NetworkKeyType
  },
  {
    ZCD_NV_SAS_NWK_MGR_ADDR, sizeof( zclCCServer_NwkManagerAddr ), &zclCCServer_NwkManagerAddr
  },

  // Last item -- DO NOT MOVE IT!
  {
    0x00, 0, NULL
  }
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void zclCCServer_HandleKeys( uint8 shift, uint8 keys );
static void zclCCServer_InitStartupParameters( uint8 initNv );
static void zclCCServer_UseStartupParameters( void );

// CC Cluster Callback functions
static void zclCCServer_Restart_DeviceCB( zclCCRestartDevice_t *pCmd, 
                                          afAddrType_t *srcAddr, uint8 seqNum );
static void zclCCServer_Save_StartupParametersCB( zclCCStartupParams_t *pCmd,
                                                  afAddrType_t *srcAddr, uint8 seqNum );
static void zclCCServer_Restore_StartupParametersCB( zclCCStartupParams_t *pCmd, 
                                                     afAddrType_t *srcAddr, uint8 seqNum );
static void zclCCServer_Reset_StartupParametersCB( zclCCStartupParams_t *pCmd, 
                                                   afAddrType_t *srcAddr, uint8 seqNum );
static void zclCCServer_Restart_DeviceRspCB( zclCCServerParamsRsp_t *pRsp, 
                                             afAddrType_t *srcAddr, uint8 seqNum );
static void zclCCServer_Save_StartupParametersRspCB( zclCCServerParamsRsp_t *pRsp, 
                                                     afAddrType_t *srcAddr, uint8 seqNum );
static void zclCCServer_Restore_StartupParametersRspCB( zclCCServerParamsRsp_t *pRsp, 
                                                        afAddrType_t *srcAddr, uint8 seqNum );
static void zclCCServer_Reset_StartupParametersRspCB( zclCCServerParamsRsp_t *pRsp, 
                                                      afAddrType_t *srcAddr, uint8 seqNum );

static ZStatus_t zclCCServer_SendLeaveReq( void );
static void zclCCServer_SavePreconfigLinkKey( void );
static uint8 zclCCServer_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo );
static ZStatus_t zclCCServer_ReadWriteCB( uint16 clusterId, uint16 attrId, 
                                          uint8 oper, uint8 *pValue, uint16 *pLen );
static ZStatus_t zclCCServer_AuthorizeCB( afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper );
static void *zclCCServer_ZdoLeaveCnfCB( void *pParam );

static void zclCCServer_InitStartupParametersInNV( void );
static void zclCCServer_ResetStartupParametersInNV( void );
static void zclCCServer_SaveStartupParametersInNV(void);
static void zclCCServer_RestoreStartupParametersInNV( void );

/*********************************************************************
 * ZCL CC Clusters Callback table
 */
static zclCC_AppCallbacks_t zclCCServer_CmdCallbacks =			
{
  zclCCServer_Restart_DeviceCB,
  zclCCServer_Save_StartupParametersCB,
  zclCCServer_Restore_StartupParametersCB,
  zclCCServer_Reset_StartupParametersCB,
  zclCCServer_Restart_DeviceRspCB,
  zclCCServer_Save_StartupParametersRspCB,
  zclCCServer_Restore_StartupParametersRspCB,
  zclCCServer_Reset_StartupParametersRspCB,
};

/*********************************************************************
 * @fn          zclCCServer_Init
 *
 * @brief       Initialization function for the ZCL Commissioing Cluster
 *              Server Application.
 *
 * @param       task_id - task id
 *
 * @return      none
 */
void zclCCServer_Init( uint8 task_id )
{
  zclCCServer_TaskID = task_id;

  leaveInitiated = FALSE;

  // This app is part of the Home Automation Profile
  zba_Init( &zclCCServer_SimpleDesc );

  // Register the ZCL Commissioning Cluster Library callback functions
  zclCC_RegisterCmdCallbacks( CCSERVER_ENDPOINT, &zclCCServer_CmdCallbacks );

  // Register the application's attribute list
  zcl_registerAttrList( CCSERVER_ENDPOINT, CCSERVER_MAX_ATTRIBUTES, zclCCServer_Attrs );

  // Register the application's attribute data validation callback function
  zcl_registerValidateAttrData( zclCCServer_ValidateAttrDataCB );

  // Register the application's callback function to read/write attribute data
  zcl_registerReadWriteCB( CCSERVER_ENDPOINT, zclCCServer_ReadWriteCB, zclCCServer_AuthorizeCB );

  // Register for Initiator to receive Leave Confirm
  ZDO_RegisterForZdoCB( ZDO_LEAVE_CNF_CBID, zclCCServer_ZdoLeaveCnfCB );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( zclCCServer_TaskID );

  // Initialize ZBA Startup Attributes Set (SAS)
  zclCCServer_InitStartupParameters( TRUE );

  // See if the device is factory new
  if ( !ZDApp_DeviceConfigured() )
  {
    osal_nv_item_init( ZCD_NV_NWKMGR_ADDR, sizeof( zclCCServer_NwkManagerAddr ),
                       (void *)&zclCCServer_NwkManagerAddr );

    // On startup, attempt to join the network specified by the startup SAS
    // on all channels at least once

    // ZBA Default Settings with the default ZBA Key Material and ZBA EPID
    if ( nullExtendedPANID( zgApsUseExtendedPANID ) )
    {
      osal_cpyExtAddr( zgApsUseExtendedPANID, zbaGlobalCommissioningEPID );
    }

    // Default Network Key and Pre-configured Link Key should already be set
  }
}

/*********************************************************************
 * @fn          zclServerApp_event_loop
 *
 * @brief       Event Loop Processor for the ZCL Commissioing Cluster
 *              Server Application.
 *
 * @param       task_id - task id
 * @param       events - event bitmap
 *
 * @return      unprocessed events
 */
uint16 zclCCServer_event_loop( uint8 task_id, uint16 events )
{
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    afIncomingMSGPacket_t *MSGpkt;

    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclCCServer_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          break;

        case ZDO_STATE_CHANGE:
          // Process State Change messages
          break;

        case KEY_CHANGE:
          zclCCServer_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
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
  
  if ( events & CCSERVER_LEAVE_TIMER_EVT )
  {
    if ( zcl_CCStartupMode( restartDevice.options ) == CC_STARTUP_MODE_REPLACE_RESTART )
    {
      // Perform a Leave on our old network
      if ( zclCCServer_SendLeaveReq() == ZSuccess )
      {
        // Wait for Leave confirmation before joining the new network
        leaveInitiated = TO_JOIN_OPERATIONAL_NWK;
      }
      else
      {
        // Notify our task to restart the network
        osal_set_event( zclCCServer_TaskID, CCSERVER_RESTART_TIMER_EVT );
      }
    }
    else
    {
      // Notify our task to restart the network
      osal_set_event( zclCCServer_TaskID, CCSERVER_RESTART_TIMER_EVT );
    }

    return ( events ^ CCSERVER_LEAVE_TIMER_EVT );
  }

  if ( events & CCSERVER_RESTART_TIMER_EVT )
  {
    if ( zcl_CCStartupMode( restartDevice.options ) == CC_STARTUP_MODE_REPLACE_RESTART )
    {
      // Set the NV startup option to force a "new" join.
      zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );         
    }
    else
    {
      // Reset the NV startup option to resume from NV by clearing
      // the "new" join option.
      zgWriteStartupOptions( ZG_STARTUP_CLEAR, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );
    }

    SystemResetSoft();

    return ( events ^ CCSERVER_RESTART_TIMER_EVT );
  }

  if ( events & CCSERVER_RESET_TIMER_EVT )
  {
    // Set the NV startup option to default the Config state values
    zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_CONFIG_STATE );

    // Set the NV startup option to force a "new" join.
    zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );

    SystemResetSoft();

    return ( events ^ CCSERVER_RESET_TIMER_EVT );
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zclCCServer_HandleKeys
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
static void zclCCServer_HandleKeys( uint8 shift, uint8 keys )
{
  (void)shift;  // Intentionally unreferenced parameter

  if ( keys & HAL_KEY_SW_1 )
  {
  }

  if ( keys & HAL_KEY_SW_2 )
  {
    zclCCServer_ResetToZBADefault();
  }

  if ( keys & HAL_KEY_SW_3 )
  {
  }

  if ( keys & HAL_KEY_SW_4 )
  {
  }
}

/*********************************************************************
 * @fn      zclCCServer_ResetToZBADefault
 *
 * @brief   Reset the local device to ZBA Default.
 *
 *          Note: This function will decommission the device to return
 *                to the ZBA Default Settings with the default ZBA Key
 *                Material and ZBA Extended PAN ID.
 *
 * @param   none
 *
 * @return  none
 */
void zclCCServer_ResetToZBADefault( void )
{
  // Set the NV startup option to force a "new" join.
  zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );

  // Perform a Leave on our old network
  if ( zclCCServer_SendLeaveReq() == ZSuccess )
  {
    // Wait for Leave confirmation before resetting
    leaveInitiated = TO_JOIN_COMMISSIONING_NWK;
  }
  else
  {
    // Notify our task to reset the device
    osal_set_event( zclCCServer_TaskID, CCSERVER_RESET_TIMER_EVT );
  }
}

/*********************************************************************
 * @fn      zclCCServer_SendLeaveReq
 *
 * @brief   Send out a Leave Request command.
 *
 * @param   void
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclCCServer_SendLeaveReq( void )
{
  NLME_LeaveReq_t leaveReq;

  // Set every field to 0
  osal_memset( &leaveReq, 0, sizeof( NLME_LeaveReq_t ) );

  // Send out our leave
  return ( NLME_LeaveReq( &leaveReq ) );
}

/*********************************************************************
 * @fn      zclCCServer_SavePreconfigLinkKey
 *
 * @brief   Save the Pre-Configured Link Key.
 *
 * @param   void
 *
 * @return  ZStatus_t
 */
static void zclCCServer_SavePreconfigLinkKey( void )
{
  APSME_TCLinkKey_t *pKeyData;

  pKeyData = (APSME_TCLinkKey_t *)osal_mem_alloc( sizeof( APSME_TCLinkKey_t ) );
  if (pKeyData != NULL)
  {
    // Making sure data is cleared for every key all the time
    osal_memset( pKeyData, 0x00, sizeof( APSME_TCLinkKey_t ) );

    osal_memset( pKeyData->extAddr, 0xFF, Z_EXTADDR_LEN );
    osal_nv_read( ZCD_NV_SAS_CURR_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, pKeyData->key );

    // Save the Pre-Configured Link Key as the default TC Link Key the in NV
    osal_nv_write( ZCD_NV_TCLK_TABLE_START, 0, sizeof( APSME_TCLinkKey_t ), pKeyData );

    // Clear copy of key in RAM
    osal_memset( pKeyData, 0x00, sizeof( APSME_TCLinkKey_t ) );

    osal_mem_free( pKeyData );
  }
}

/*********************************************************************
 * @fn      zclCCServer_InitStartupParameters
 *
 * @brief   Initialize the current Startup Parameters Set (SAS) to 
 *          their default values.
 *
 * @param   initNv - whether to initialize keys in NV
 *
 * @return  none
 */
static void zclCCServer_InitStartupParameters( uint8 initNv )
{
  uint8 zbaDefaultKey[SEC_KEY_LEN] = DEFAULT_TC_LINK_KEY;
  
  zclCCServer_ShortAddress = CC_DEFAULT_SHORT_ADDR;

  osal_cpyExtAddr( zclCCServer_ExtendedPanId, zbaGlobalCommissioningEPID );

  zclCCServer_PanId = CC_DEFAULT_PANID;
  zclCCServer_ChannelMask = DEFAULT_CHANLIST;
  zclCCServer_ProtocolVersion = ZB_PROT_VERS;
  zclCCServer_StackProfile = STACK_PROFILE_ID;

  if ( ZG_BUILD_COORDINATOR_TYPE && ZG_DEVICE_COORDINATOR_TYPE )
  {
    // Form the Commissioning Network
    zclCCServer_StartUpControl = CC_STARTUP_CONTROL_OPTION_1;
  }
  else
  {
    // Join the Commissioning Network
    zclCCServer_StartUpControl = CC_STARTUP_CONTROL_OPTION_3;
  }

  osal_memcpy( zclCCServer_TrustCenterAddr, 0x00, Z_EXTADDR_LEN);

  if ( initNv )
  {
    // If the item doesn't exist in NV memory, create and initialize it
    if ( osal_nv_item_init( ZCD_NV_SAS_CURR_PRECFG_LINK_KEY, 
                            SEC_KEY_LEN, zbaDefaultKey ) == SUCCESS )
    {
      // Write the default value back to NV
      osal_nv_write( ZCD_NV_SAS_CURR_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );
    }

    osal_memset( zbaDefaultKey, 0x00, SEC_KEY_LEN );
  
    if ( osal_nv_item_init( ZCD_NV_SAS_CURR_TC_MASTER_KEY, 
                            SEC_KEY_LEN, zbaDefaultKey ) == SUCCESS )
    {
      // Write the default value back to NV
      osal_nv_write( ZCD_NV_SAS_CURR_TC_MASTER_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );
    }

    if ( osal_nv_item_init( ZCD_NV_SAS_CURR_NWK_KEY,
                            SEC_KEY_LEN, zbaDefaultKey ) == SUCCESS )
    {
      // Write the default value back to NV
      osal_nv_write( ZCD_NV_SAS_CURR_NWK_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );
    }
  }
  else
  {
    // Save the default keys directly in the NV
    osal_nv_write( ZCD_NV_SAS_CURR_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );
  
    osal_memset( zbaDefaultKey, 0x00, SEC_KEY_LEN );
  
    osal_nv_write( ZCD_NV_SAS_CURR_TC_MASTER_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );
    osal_nv_write( ZCD_NV_SAS_CURR_NWK_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );
  }

  zclCCServer_UseInsecureJoin = TRUE;

  zclCCServer_NetworkKeySeqNum = CC_DEFAULT_NETWORK_KEY_SEQ_NUM;
  zclCCServer_NetworkKeyType = KEY_TYPE_NWK;
  zclCCServer_NwkManagerAddr = CC_DEFAULT_NETWORK_MANAGER_ADDR;
}

/*********************************************************************
 * @fn      zclCCServer_UseStartupParameters
 *
 * @brief   Set the network parameters to the current Startup Parameters.
 *
 * @param   none
 *
 * @return  none
 */
static void zclCCServer_UseStartupParameters( void )
{
  if ( zclCCServer_StartUpControl == CC_STARTUP_CONTROL_OPTION_1 )
  {
    uint8 networkKey[SEC_KEY_LEN];

    // Form the Commissioning network and become the Coordinator for that network

    // Required attributes: Extended PAN ID
    osal_nv_write( ZCD_NV_EXTENDED_PAN_ID, 0, Z_EXTADDR_LEN, 
                   zclCCServer_ExtendedPanId );
    osal_nv_write( ZCD_NV_APS_USE_EXT_PANID, 0, Z_EXTADDR_LEN, 
                   zclCCServer_ExtendedPanId );

    // Optional attributes: PAN ID, Channel Mask, Network Manager Address,
    // Network Key, Network Key Type (only KEY_TYPE_NWK is currently supported),
    // and Trust Center Address (which is used with Preconfigured Link Key).
    osal_nv_write( ZCD_NV_PANID, 0, sizeof( zclCCServer_PanId ), 
                   &zclCCServer_PanId );

    osal_nv_write( ZCD_NV_CHANLIST, 0, sizeof( zclCCServer_ChannelMask ), 
                   &zclCCServer_ChannelMask );

    osal_nv_write( ZCD_NV_NWKMGR_ADDR, 0, sizeof( zclCCServer_NwkManagerAddr ),
                   &zclCCServer_NwkManagerAddr );

    osal_nv_read( ZCD_NV_SAS_CURR_NWK_KEY, 0, SEC_KEY_LEN, networkKey );
    if ( !nullKey( networkKey ) )
    {
      // Save the Network Key as the Pre-Configured Key in NV
      osal_nv_write( ZCD_NV_PRECFGKEY, 0, SEC_KEY_LEN, networkKey );

      // Clear copy of key in RAM
      osal_memset( networkKey, 0x00, SEC_KEY_LEN );
    }
  }
  else if ( zclCCServer_StartUpControl == CC_STARTUP_CONTROL_OPTION_3 )
  {
     // Join the Commissioning network

    // Required attributes: none

    // Optional attributes: Extended PAN ID, Channel Mask, and 
    // Preconfigured Link Key
    osal_nv_write( ZCD_NV_EXTENDED_PAN_ID, 0, Z_EXTADDR_LEN, 
                   zclCCServer_ExtendedPanId );

    osal_nv_write( ZCD_NV_CHANLIST, 0, sizeof( zclCCServer_ChannelMask ), 
                   &zclCCServer_ChannelMask );

    zclCCServer_SavePreconfigLinkKey();
  }
}

/*********************************************************************
 * @fn      zclCCServer_Restart_DeviceCB
 *
 * @brief   Callback from the ZCL commissioning Cluster Library when
 *          it received a Restart Device for this application.
 *
 * @param   pCmd - restart command
 * @param   srcAddr - source address
 * @param   seqNum - message sequence number
 *
 * @return  none
 */
static void zclCCServer_Restart_DeviceCB( zclCCRestartDevice_t *pCmd, 
                                          afAddrType_t *srcAddr, uint8 seqNum )
{
  zclCCServerParamsRsp_t rsp;

  // Make sure the device is not in the Operational Network 
  if ( osal_ExtAddrEqual( _NIB.extendedPANID, zbaGlobalCommissioningEPID ) )
  {
    // If form network or associate from scratch, set the default network state.
    // Only Coordiunator can form (Option 1) and Router or EndDevices can 
    // associate (Option 3).
    if ( ( ( zclCCServer_StartUpControl == CC_STARTUP_CONTROL_OPTION_1 ) && // (form &&
           ( zgDeviceLogicalType == ZG_DEVICETYPE_COORDINATOR ) )        || //  Coord) ||
         ( ( zclCCServer_StartUpControl == CC_STARTUP_CONTROL_OPTION_3 ) && // (associate && 
           ( ( zgDeviceLogicalType == ZG_DEVICETYPE_ROUTER )             || //  (RTR ||         
             ( zgDeviceLogicalType == ZG_DEVICETYPE_ENDDEVICE ) ) ) )       //   ED))
    {
       // Form or Join a new network
      rsp.status = SUCCESS;
    }
    else
    {
      // Startup state is currently not supported
      rsp.status = ZCL_STATUS_INCONSISTENT_STARTUP_STATE;
    }
  }
  else
  {
    // No access to Commissioning cluster in the Operational Network
    rsp.status = ZCL_STATUS_NOT_AUTHORIZED;
  }

  zclCC_Send_RestartDeviceRsp( CCSERVER_ENDPOINT, srcAddr, &rsp, TRUE, seqNum );

  if ( rsp.status == SUCCESS )
  {
    uint16 delay;

    // If restart immediately is specified the should restart at a convienent 
    // time when all messages have been sent. Give 1s for response to be sent
    // and then restart. Else, calculate the delay time.
    if ( zcl_CCImmediate( pCmd->options ) )
    {
      delay = 1000;
    }
    else
    {
      delay = ( pCmd->delay * 1000 ) + ( osal_rand() % ( pCmd->jitter * 80 ) );
    }

    if ( zcl_CCStartupMode( pCmd->options ) == CC_STARTUP_MODE_REPLACE_RESTART )
    {
      zclCCServer_UseStartupParameters();
    }

    // Save the received command for later
    restartDevice = *pCmd;

    osal_start_timerEx( zclCCServer_TaskID, CCSERVER_LEAVE_TIMER_EVT, delay );
  }
}

/*********************************************************************
 * @fn      zclCCServer_Save_StartupParametersCB
 *
 * @brief   Callback from the ZCL Commissioning Cluster Library when
 *          it received a Save startup parameters for this application.
 *
 * @param   pCmd - save command
 * @param   srcAddr - source address
 * @param   seqNum - message sequence number
 *
 * @return  none
 */
static void zclCCServer_Save_StartupParametersCB( zclCCStartupParams_t *pCmd,
                                                  afAddrType_t *srcAddr, uint8 seqNum )
{
  static uint8 nvInitFlag = FALSE;
  zclCCServerParamsRsp_t rsp;
  
  if( pCmd->index < CCSERVER_MAX_NWK_STARTUP_PARAMS )
  {
    if ( !nvInitFlag )
    {
      nvInitFlag = TRUE;
      
      zclCCServer_InitStartupParametersInNV();
    }
    else
    {
      zclCCServer_SaveStartupParametersInNV();
    }
    
    rsp.status = SUCCESS;
  }
  else
  {
    rsp.status = ZCL_STATUS_INSUFFICIENT_SPACE;
  }
  
  zclCC_Send_SaveStartupParamsRsp( CCSERVER_ENDPOINT, srcAddr, &rsp, TRUE, seqNum );
}

/*********************************************************************
 * @fn      zclCCServer_Restore_StartupParametersCB
 *
 * @brief   Callback from the ZCL commissioning Cluster Library when
 *          it received a Restore startup  parameters for this application.
 *
 * @param   pCmd - restore command
 * @param   srcAddr - source address
 * @param   seqNum - message sequence number
 *
 * @return  none
 */
static void zclCCServer_Restore_StartupParametersCB( zclCCStartupParams_t *pCmd,
                                                     afAddrType_t *srcAddr, uint8 seqNum )
{ 
  zclCCServerParamsRsp_t rsp;
  
  if ( pCmd->index < CCSERVER_MAX_NWK_STARTUP_PARAMS )
  {
    zclCCServer_RestoreStartupParametersInNV();
    
    rsp.status = SUCCESS;
  }
  else
  {
    rsp.status = ZCL_STATUS_INVALID_FIELD;
  }
  
  zclCC_Send_RestoreStartupParamsRsp( CCSERVER_ENDPOINT, srcAddr, &rsp, TRUE, seqNum );
}

/*********************************************************************
 * @fn      zclCCServer_Reset_StartupParametersCB
 *
 * @brief   Callback from the ZCL commissioning Cluster Library when
 *          it received a Reset startup  parameters for this application.
 *
 * @param   pCmd - reset command
 * @param   srcAddr - source address
 * @param   seqNum - message sequence number
 *
 * @return  none
 */
static void zclCCServer_Reset_StartupParametersCB( zclCCStartupParams_t *pCmd,
                                                   afAddrType_t *srcAddr, uint8 seqNum ) 
{
  zclCCServerParamsRsp_t rsp;
  
  // Reset current SAS
  if ( pCmd->options & CC_RESET_CURRENT )
  {
    zclCCServer_InitStartupParameters( FALSE );
    
    rsp.status = SUCCESS;
  }
  
  // Only one set of Startup parameters is stored in NV. So index based 
  // Startup parameter reset is valid only for index 0
  if ( ( pCmd->options & CC_RESET_ALL ) || ( pCmd->index < CCSERVER_MAX_NWK_STARTUP_PARAMS ) )
  {
    zclCCServer_ResetStartupParametersInNV();
    
    rsp.status = SUCCESS;
  }

  // Freeing the NV storage associated with an index is not supported. So Erase Index
  // is denied
  if ( pCmd->options & CC_ERASE_INDEX )
  {
    rsp.status = ZCL_STATUS_ACTION_DENIED;
  }
  
  zclCC_Send_ResetStartupParamsRsp( CCSERVER_ENDPOINT, srcAddr, &rsp, TRUE, seqNum );
}

/*********************************************************************
 * @fn      zclCCServer_Restart_DeviceRspCB
 *
 * @brief   Callback from the ZCL commissioning Cluster Library when
 *          it received a Restart device Response for this application.
 *
 * @param   pCmd - restart response
 * @param   srcAddr - source address
 * @param   seqNum - message sequence number
 *
 * @return  none
 */
static void zclCCServer_Restart_DeviceRspCB( zclCCServerParamsRsp_t *pCmd, 
                                             afAddrType_t *srcAddr, uint8 seqNum )
{
 // Add application code here
}

/*********************************************************************
 * @fn      zclCCServer_Save_StartupParametersRspCB
 *
 * @brief   Callback from the ZCL commissioning Cluster Library when
 *          it received a save startup parameter Response for
 *          this application.
 *
 * @param   pCmd - save response
 * @param   srcAddr - source address
 * @param   seqNum - message sequence number
 *
 * @return  none
 */
static void zclCCServer_Save_StartupParametersRspCB( zclCCServerParamsRsp_t *pRsp,
                                                     afAddrType_t *srcAddr, uint8 seqNum )
{
  // Add application code here
}

/*********************************************************************
 * @fn      zclCCServer_Restore_StartupParametersRspCB
 *
 * @brief   Callback from the ZCL commissioning Cluster Library when
 *          it received a Restore startup parameter Response for
 *          this application.
 *
 * @param   pCmd - restore response
 * @param   srcAddr - source address
 * @param   seqNum - message sequence number
 *
 * @return  none
 */  
static void zclCCServer_Restore_StartupParametersRspCB( zclCCServerParamsRsp_t *pRsp,
                                                        afAddrType_t *srcAddr, uint8 seqNum )
{
  // Add application code here
}

/*********************************************************************
 * @fn      zclCCServer_Reset_StartupParametersRspCB
 *
 * @brief   Callback from the ZCL commissioning Cluster Library when
 *          it received a Reset startup parameter Response for
 *          this application.
 *
 * @param   pCmd - reset response
 * @param   srcAddr - source address
 * @param   seqNum - message sequence number
 *
 * @return  none
 */  
static void zclCCServer_Reset_StartupParametersRspCB( zclCCServerParamsRsp_t *pRsp,
                                                      afAddrType_t *srcAddr, uint8 seqNum ) 
{
  // Add application code here
}

/*********************************************************************
 * @fn      zclCCServer_ValidateAttrDataCB
 *
 * @brief   Check to see if the supplied value for the attribute data
 *          is within the specified range of the attribute.
 *
 * @param   pAttr - pointer to attribute
 * @param   pAttrInfo - pointer to attribute info
 *
 * @return  TRUE if data valid. FALSE otherwise.
 */
static uint8 zclCCServer_ValidateAttrDataCB( zclAttrRec_t *pAttr, zclWriteRec_t *pAttrInfo )
{
  uint8 valid = TRUE;

  if ( pAttr->attr.dataPtr == &zclCCServer_StartUpControl )
  {
    if ( ( *(pAttrInfo->attrData) != CC_STARTUP_CONTROL_OPTION_1 ) &&
         ( *(pAttrInfo->attrData) != CC_STARTUP_CONTROL_OPTION_3 ) )
    {
      valid = FALSE;
    }
  }
  else if ( pAttr->attr.dataPtr == &zclCCServer_NetworkKeyType )
  {
    if ( *(pAttrInfo->attrData) != KEY_TYPE_NWK )
    {
      valid = FALSE;
    }
  }

  return ( valid );
}

/*********************************************************************
 * @fn      zclCCServer_ReadWriteCB
 *
 * @brief   Read/write attribute data. This callback function should
 *          only be called for the Network Security Key attributes.
 *
 *          Note: This function is only required when the attribute data
 *                format is unknown to ZCL. This function gets called
 *                when the pointer 'dataPtr' to the attribute value is
 *                NULL in the attribute database registered with the ZCL.
 *
 * @param   clusterId - cluster that attribute belongs to
 * @param   attrId - attribute to be read or written
 * @param   oper - ZCL_OPER_LEN, ZCL_OPER_READ, or ZCL_OPER_WRITE
 * @param   pValue - pointer to attribute value
 * @param   pLen - length of attribute value read
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclCCServer_ReadWriteCB( uint16 clusterId, uint16 attrId, 
                                          uint8 oper, uint8 *pValue, uint16 *pLen )
{
  ZStatus_t status = ZCL_STATUS_SUCCESS;

  switch ( oper )
  {
    case ZCL_OPER_LEN:
      *pLen = SEC_KEY_LEN;
      break;

    case ZCL_OPER_READ:
      osal_nv_read( NvIdFromAttrId( attrId ), 0, SEC_KEY_LEN, pValue );

      if ( pLen != NULL )
      {
        *pLen = SEC_KEY_LEN;
      }
      break;

    case ZCL_OPER_WRITE:
      osal_nv_write( NvIdFromAttrId( attrId ), 0, SEC_KEY_LEN, pValue );
      break;

    default:
      status = ZCL_STATUS_SOFTWARE_FAILURE; // Should never get here!
      break;
  }

  return ( status );
}

/*********************************************************************
 * @fn      zclCCServer_AuthorizeCB
 *
 * @brief   Authorize a Read or Write operation on a given attribute. 
 *
 *          Note: The pfnAuthorizeCB callback function is only required
 *                when the Read/Write operation on an attribute requires 
 *                authorization (i.e., attributes with ACCESS_CONTROL_AUTH_READ
 *                or ACCESS_CONTROL_AUTH_WRITE access permissions).
 *
 * @param   srcAddr - source Address
 * @param   pAttr - pointer to attribute
 * @param   oper - ZCL_OPER_READ, or ZCL_OPER_WRITE
 *
 * @return  ZCL_STATUS_SUCCESS: Operation authorized
 *          ZCL_STATUS_NOT_AUTHORIZED: Operation not authorized
 */
static ZStatus_t zclCCServer_AuthorizeCB( afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper )
{
  // Make sure the device is not in the Operational Network 
  if ( osal_ExtAddrEqual( _NIB.extendedPANID, zbaGlobalCommissioningEPID ) )
  {
    return ( ZCL_STATUS_SUCCESS );
  }

  return ( ZCL_STATUS_NOT_AUTHORIZED );
}

/******************************************************************************
 * @fn      zclCCServer_ZdoLeaveCnfCB
 *
 * @brief   This callback is called to process a Leave Confirmation message.
 *
 *          Note: this callback function returns a pointer if it has handled
 *                the confirmation message and no further action should be
 *                taken with it. It returns NULL if it has not handled the
 *                confirmation message and normal processing should take place.
 *
 * @param       pParam - received message
 *
 * @return      Pointer if message processed. NULL, otherwise.
 */
static void *zclCCServer_ZdoLeaveCnfCB( void *pParam )
{
  (void)pParam;

  // Did we initiate the leave?
  if ( leaveInitiated == FALSE )
  {
    return ( NULL );
  }

  if ( leaveInitiated == TO_JOIN_OPERATIONAL_NWK )
  {
    // Notify our task to join the restart network
    osal_set_event( zclCCServer_TaskID, CCSERVER_RESTART_TIMER_EVT );
  }
  else
  {
    // Notify our task to reset the device
    osal_set_event( zclCCServer_TaskID, CCSERVER_RESET_TIMER_EVT );
  }

  return ( (void *)&leaveInitiated );
}

/*********************************************************************
 * @fn      zclCCServer_InitStartupParametersInNV
 *
 * @brief   Initialize the Startup Parameters in NV.
 *
 * @param   none
 *
 * @return  none
 */
static void zclCCServer_InitStartupParametersInNV( void )
{
  uint8 key[SEC_KEY_LEN];

  for ( uint8 i = 0; nvItemTable[i].id != 0x00; i++ )
  {
    // Initialize the item
    osal_nv_item_init( nvItemTable[i].id, nvItemTable[i].len, nvItemTable[i].buf );
  }

  // Use the default key values in the NV
  osal_nv_read( ZCD_NV_SAS_CURR_TC_MASTER_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_item_init( ZCD_NV_SAS_TC_MASTER_KEY, SEC_KEY_LEN, key );

  osal_nv_read( ZCD_NV_SAS_CURR_NWK_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_item_init( ZCD_NV_SAS_NWK_KEY, SEC_KEY_LEN, key );

  osal_nv_read( ZCD_NV_SAS_CURR_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_item_init( ZCD_NV_SAS_PRECFG_LINK_KEY, SEC_KEY_LEN, key );

  // Clear copy of key in RAM
  osal_memset( key, 0x00, SEC_KEY_LEN );
}

/*********************************************************************
 * @fn      zclCCServer_ResetStartupParametersInNV
 *
 * @brief   Reset the Startup Parameters in NV.
 *
 * @param   none
 *
 * @return  none
 */
static void zclCCServer_ResetStartupParametersInNV( void )
{
  uint8 tmpByte;
  uint16 tmpShort;
  uint32 tmpLong;
  uint8 tmpExtAddr[Z_EXTADDR_LEN];
  uint8 zbaDefaultKey[SEC_KEY_LEN] = DEFAULT_TC_LINK_KEY;

  tmpShort = CC_DEFAULT_SHORT_ADDR;
  osal_nv_write( ZCD_NV_SAS_SHORT_ADDR, 0, sizeof( tmpShort ), &tmpShort );

  osal_cpyExtAddr( tmpExtAddr, zbaGlobalCommissioningEPID );
  osal_nv_write( ZCD_NV_SAS_EXT_PANID, 0, Z_EXTADDR_LEN, tmpExtAddr );

  tmpShort = CC_DEFAULT_PANID;
  osal_nv_write( ZCD_NV_SAS_PANID, 0, sizeof( tmpShort ), &tmpShort );

  tmpLong = DEFAULT_CHANLIST;
  osal_nv_write( ZCD_NV_SAS_CHANNEL_MASK, 0, sizeof( tmpLong ), &tmpLong );

  tmpByte = ZB_PROT_VERS;
  osal_nv_write( ZCD_NV_SAS_PROTOCOL_VER, 0, sizeof( tmpShort ), &tmpShort );

  tmpByte = STACK_PROFILE_ID;
  osal_nv_write( ZCD_NV_SAS_STACK_PROFILE, 0, sizeof( tmpShort ), &tmpShort );

  if ( ZG_BUILD_COORDINATOR_TYPE && ZG_DEVICE_COORDINATOR_TYPE )
  {
    // Form the Commissioning Network
    tmpByte = CC_STARTUP_CONTROL_OPTION_1;
  }
  else
  {
    // Join the Commissioning Network
    tmpByte = CC_STARTUP_CONTROL_OPTION_3;
  }

  osal_nv_write( ZCD_NV_SAS_STARTUP_CTRL, 0, sizeof( tmpByte ), &tmpByte );

  osal_memset( tmpExtAddr, 0x00, Z_EXTADDR_LEN );
  osal_nv_write( ZCD_NV_SAS_TC_ADDR, 0, Z_EXTADDR_LEN, tmpExtAddr );

  osal_nv_write( ZCD_NV_SAS_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );
  
  osal_memset( zbaDefaultKey, 0x00, SEC_KEY_LEN );
  osal_nv_write( ZCD_NV_SAS_TC_MASTER_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );
  osal_nv_write( ZCD_NV_SAS_NWK_KEY, 0, SEC_KEY_LEN, zbaDefaultKey );

  tmpByte = TRUE;
  osal_nv_write( ZCD_NV_SAS_USE_INSEC_JOIN, 0, sizeof( tmpByte ), &tmpByte );

  tmpByte = CC_DEFAULT_NETWORK_KEY_SEQ_NUM;
  osal_nv_write( ZCD_NV_SAS_NWK_KEY_SEQ_NUM, 0, sizeof( tmpByte ), &tmpByte );

  tmpByte = KEY_TYPE_NWK;
  osal_nv_write( ZCD_NV_SAS_NWK_KEY_TYPE, 0, sizeof( tmpByte ), &tmpByte );

  tmpShort = CC_DEFAULT_NETWORK_MANAGER_ADDR;
  osal_nv_write( ZCD_NV_SAS_NWK_MGR_ADDR, 0, sizeof( tmpShort ), &tmpShort );
}

/*********************************************************************
 * @fn      zclCCServer_RestoreStartupParametersInNV
 *
 * @brief   Save the Startup Parameters in NV
 *
 * @param   none
 *
 * @return  none
 */
static void zclCCServer_SaveStartupParametersInNV( void )
{
  uint8 key[SEC_KEY_LEN];

  for ( uint8 i = 0; nvItemTable[i].id != 0x00; i++ )
  {
    // Write the item
    osal_nv_write( nvItemTable[i].id, 0, nvItemTable[i].len, nvItemTable[i].buf );
  }

  // Update the keys in the NV
  osal_nv_read( ZCD_NV_SAS_CURR_TC_MASTER_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_write( ZCD_NV_SAS_TC_MASTER_KEY, 0, SEC_KEY_LEN, key );

  osal_nv_read( ZCD_NV_SAS_CURR_NWK_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_write( ZCD_NV_SAS_NWK_KEY, 0, SEC_KEY_LEN, key );

  osal_nv_read( ZCD_NV_SAS_CURR_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_write( ZCD_NV_SAS_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, key );

  // Clear copy of key in RAM
  osal_memset( key, 0x00, SEC_KEY_LEN );
}

/*********************************************************************
 * @fn      zclCCServer_RestoreStartupParametersInNV
 *
 * @brief   Restore the Startup Parameters from NV
 *
 * @param   none
 *
 * @return  none
 */
static void zclCCServer_RestoreStartupParametersInNV( void )
{
  uint8 key[SEC_KEY_LEN];

  for ( uint8 i = 0; nvItemTable[i].id != 0x00; i++ )
  {
    // Read the item
    osal_nv_read( nvItemTable[i].id, 0, nvItemTable[i].len, nvItemTable[i].buf );
  }

  // Update the current keys in the NV
  osal_nv_read( ZCD_NV_SAS_TC_MASTER_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_write( ZCD_NV_SAS_CURR_TC_MASTER_KEY, 0, SEC_KEY_LEN, key );

  osal_nv_read( ZCD_NV_SAS_NWK_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_write( ZCD_NV_SAS_CURR_NWK_KEY, 0, SEC_KEY_LEN, key );

  osal_nv_read( ZCD_NV_SAS_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, key );
  osal_nv_write( ZCD_NV_SAS_CURR_PRECFG_LINK_KEY, 0, SEC_KEY_LEN, key );

  // Clear copy of key in RAM
  osal_memset( key, 0x00, SEC_KEY_LEN );
}


/****************************************************************************
****************************************************************************/
