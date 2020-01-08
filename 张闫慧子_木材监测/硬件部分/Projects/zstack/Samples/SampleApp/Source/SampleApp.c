/**************************************************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Sample Application (no Profile).


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends it's messages either as broadcast or
  broadcast filtered group messages.  The other (more normal)
  message addressing is unicast.  Most of the other sample
  applications are written to support the unicast message model.

  Key control:
    SW1:  Sends a flash command to all devices in Group 1.
    SW2:  Adds/Removes (toggles) this device in and out
          of Group 1.  This will enable and disable the
          reception of the flash command.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"

#include <stdio.h>
#include <string.h>

//MY DEVICE
#include "DHT11.h"
#include "IC.h"

/*********************************************************************
 * MACROS
 */
 
#define GAS P0_6
#define FIRE P1_1
#define RAIN P1_3
/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 AppTitle[] = "ALD2530 Example"; //应用程序名称 
uint16 ReadGasData( void );
 
void myDelay(){
  for(int j =0;j<100;j++)
    for(int i = 0;i<0xffff;i++)
    {}
}
// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_PERIODIC_CLUSTERID,
  SAMPLEAPP_FLASH_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */


uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;

aps_Group_t SampleApp_Group;

uint8 SampleAppPeriodicCounter = 0;
uint8 SampleAppFlashCounter = 0;

 
/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys );
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendFlashMessage( uint16 flashTime );
void SampleApp_Send_P2P_Message(void);
/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
 
/*********************************************************************
 * @fn      SampleApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */

void SampleApp_Init( uint8 task_id )
{ 
  
 
  SampleApp_TaskID = task_id;   //osal分配的任务ID随着用户添加任务的增多而改变
  SampleApp_NwkState = DEV_INIT;//设备状态设定为ZDO层中定义的初始化状态
  SampleApp_TransID = 0;        //消息发送ID（多消息时有顺序之分）
  //模块初始化
  P0SEL&=~0x40;
  P0DIR&=~0x40;//初始化气体
  P1SEL&=~0x2;
  P1DIR&=~0x2;//初始化火焰 p1_1
  P1SEL&=~0x08;
  P1DIR&=~0x08;//初始化雨滴p1_3
  IC_Init();//RFID
  //串口配置
  MT_UartInit();
  MT_UartRegisterTaskID(task_id);  
  HalUARTWrite(0,"UartInit OK\n", sizeof("UartInit OK\n")); 
  

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

#if defined ( BUILD_ALL_DEVICES )
  // The "Demo" target is setup to have BUILD_ALL_DEVICES and HOLD_AUTO_START
  // We are looking at a jumper (defined in SampleAppHw.c) to be jumpered
  // together - if they are - we will start up a coordinator. Otherwise,
  // the device will start as a router.
  if ( readCoordinatorJumper() )
    zgDeviceLogicalType = ZG_DEVICETYPE_COORDINATOR;
  else
    zgDeviceLogicalType = ZG_DEVICETYPE_ROUTER;
#endif // BUILD_ALL_DEVICES

//该段的意思是，如果设置了HOLD_AUTO_START宏定义，将会在启动芯片的时候会暂停启动
//流程，只有外部触发以后才会启动芯片。其实就是需要一个按钮触发它的启动流程。  
#if defined ( HOLD_AUTO_START )
  // HOLD_AUTO_START is a compile option that will surpress ZDApp
  //  from starting the device and wait for the application to
  //  start the device.
  ZDOInitDevice(0);
#endif

  // Setup for the periodic message's destination address 设置发送数据的方式和目的地址寻址模式
  // Broadcast to everyone 发送模式:广播发送
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;//广播
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT; //指定端点号
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;//指定目的网络地址为广播地址

  // Setup for the flash command's destination address - Group 1 组播发送
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup; //组寻址
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT; //指定端点号
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;//组号0x0001

  // Fill out the endpoint description. 定义本设备用来通信的APS层端点描述符
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT; //指定端点号
  SampleApp_epDesc.task_id = &SampleApp_TaskID;   //SampleApp 描述符的任务ID
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;//SampleApp简单描述符
  SampleApp_epDesc.latencyReq = noLatencyReqs;    //延时策略

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );    //向AF层登记描述符

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SampleApp_TaskID ); // 登记所有的按键事件

  // By default, all devices start out in Group 1
  SampleApp_Group.ID = 0x0001;//组号
  osal_memcpy( SampleApp_Group.name, "Group 1", 7  );//设定组名
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );//把该组登记添加到APS中

#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "SampleApp", HAL_LCD_LINE_1 ); //如果支持LCD，显示提示信息
#endif
}

/*********************************************************************
 * @fn      SampleApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
//用户应用任务的事件处理函数
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG ) //接收系统消息再进行判断
  {
    //接收属于本应用任务SampleApp的消息，以SampleApp_TaskID标记
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // Received when a key is pressed
        case KEY_CHANGE://按键事件
          SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        // Received when a messages is received (OTA) for this endpoint
      case AF_INCOMING_MSG_CMD://接收数据事件,调用函数AF_DataRequest()接收数据
          SampleApp_MessageMSGCB( MSGpkt );//调用回调函数对收到的数据进行处理
          break;

        // Received whenever the device changes state in the network
        case ZDO_STATE_CHANGE:
          //只要网络状态发生改变，就通过ZDO_STATE_CHANGE事件通知所有的任务。
          //同时完成对协调器，路由器，终端的设置
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          //if ( (SampleApp_NwkState == DEV_ZB_COORD)//实验中协调器只接收数据所以取消发送事件
          if ( (SampleApp_NwkState == DEV_ROUTER) || (SampleApp_NwkState == DEV_END_DEVICE) )
          {
            // Start sending the periodic message in a regular interval.
            //这个定时器只是为发送周期信息开启的，设备启动初始化后从这里开始
            //触发第一个周期信息的发送，然后周而复始下去
            osal_start_timerEx( SampleApp_TaskID,
                              SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                              SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
           
          }
          else
          {
              
          }
          break;

        default:
          break;
      }

      // Release the memory 事件处理完了，释放消息占用的内存
      osal_msg_deallocate( (uint8 *)MSGpkt );
   
      // Next - if one is available 指针指向下一个放在缓冲区的待处理的事件，
      //返回while ( MSGpkt )重新处理事件，直到缓冲区没有等待处理事件为止
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }

    // return unprocessed events 返回未处理的事件
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out - This event is generated by a timer
  //  (setup in SampleApp_Init()).
  if ( events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT )
  {
    // Send the periodic message 处理周期性事件，
    //利用SampleApp_SendPeriodicMessage()处理完当前的周期性事件，然后启动定时器
    //开启下一个周期性事情，这样一种循环下去，也即是上面说的周期性事件了，
    //可以做为传感器定时采集、上传任务
    SampleApp_SendPeriodicMessage();
    
    // Setup to send message again in normal period (+ a little jitter)
    osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
        (SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT + (osal_rand() & 0x00FF)) );

    // return unprocessed events 返回未处理的事件
    return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * @fn      SampleApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
void SampleApp_HandleKeys( uint8 shift, uint8 keys )  
{
  (void)shift;  // Intentionally unreferenced parameter
  
  if ( keys & HAL_KEY_SW_1 )
  {
    /* This key sends the Flash Command is sent to Group 1.
     * This device will not receive the Flash Command from this
     * device (even if it belongs to group 1).
     */
   SampleApp_SendFlashMessage( SAMPLEAPP_FLASH_DURATION );
   
    
  
  }

  if ( keys & HAL_KEY_SW_6 )
  {
    /* The Flashr Command is sent to Group 1.
     * This key toggles this device in and out of group 1.
     * If this device doesn't belong to group 1, this application
     * will not receive the Flash command sent to group 1.
     */
  
   /*
    aps_Group_t *grp;
    grp = aps_FindGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
    if ( grp )
    {
      // Remove from the group
      aps_RemoveGroup( SAMPLEAPP_ENDPOINT, SAMPLEAPP_FLASH_GROUP );
    }
    else
    {
      // Add to the flash group
      aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );
    }
   */
  }
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
//接收数据，参数为接收到的数据
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  uint16 flashTime;
  unsigned short len = 6+2+1+1+1+9;
  byte buf[6+2+1+1+1+9]={0}; 
  
  switch ( pkt->clusterId ) //判断簇ID
  {
    case SAMPLEAPP_PERIODIC_CLUSTERID: //收到广播数据
      osal_memset(buf, 0 , len);
      osal_memcpy(buf, pkt->cmd.Data, len);
      //HalUARTWrite(0,buf, len);
      memset(ucTagType,0,4);
      uint8 i;
      uint8 Card_Id[9]={0}; //存放32位卡号
      uint8 asc_16[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
      if(IC_Test()==1 ){   
          //16进制转ASC码
          for(i=0;i<4;i++)
          {
            Card_Id[i*2]=asc_16[ucTagType[i]/16];
            Card_Id[i*2+1]=asc_16[ucTagType[i]%16];        
          }   
          Card_Id[8] = 0;
          sprintf(buf,"%s%s\n",buf,Card_Id);
         
       }
         HalUARTWrite(0,buf, len);
      
      break;

    case SAMPLEAPP_FLASH_CLUSTERID: //收到组播数据  
      flashTime = BUILD_UINT16(pkt->cmd.Data[1], pkt->cmd.Data[2] );
      HalLedBlink( HAL_LED_4, 4, 50, (flashTime / 4) );
      break;
  case SAMPLEAPP_P2P_CLUSTERID:
 
    break;
  }
}

/*********************************************************************
 * @fn      SampleApp_SendPeriodicMessage
 *
 * @brief   Send the periodic message.
 *
 * @param   none
 *
 * @return  none
 */
//分析发送周期信息
void SampleApp_SendPeriodicMessage( void )
{ 
#ifndef ZDO_COORDINATOR 
  //获得的数据通过串口输出到电脑显示  
  char str[6+2+1+1+1]={0};
  //获取温湿度
  unsigned char temp_humi[6]={0};
  strcpy(temp_humi,DHT11());
  //获取气体
  uint16 gas = ReadGasData();
  //获取火焰
  byte fire = 0;
  if(FIRE){
    fire = 0;
  }
  else{
    fire = 1;
  }
  //获取雨滴
  byte rain = RAIN==0?1:0;
  
 
  sprintf(str,"%s%2d%d%d",temp_humi,gas,fire,rain);
  HalUARTWrite(0,str,sizeof(str));

   // 调用AF_DataRequest将数据无线广播出去
  if( AF_DataRequest( &SampleApp_Periodic_DstAddr,//发送目的地址＋端点地址和传送模式
                       &SampleApp_epDesc,//源(答复或确认)终端的描述（比如操作系统中任务ID等）源EP
                       SAMPLEAPP_PERIODIC_CLUSTERID, //被Profile指定的有效的集群号
                       sizeof(str),       //发送数据长度
                       (uchar*)str,// 发送数据缓冲区
                       &SampleApp_TransID,     // 任务ID号
                       AF_DISCV_ROUTE,      // 有效位掩码的发送选项
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )  //传送跳数，通常设置为AF_DEFAULT_RADIUS
  {
  }
  else
  {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
    // Error occurred in request to send.
  }
#elif defined ZDO_COORDINATOR
  //协调器发送
  
    
  

#endif 
  
 
}

/*********************************************************************
 * @fn      SampleApp_SendFlashMessage
 *
 * @brief   Send the flash message to group 1.
 *
 * @param   flashTime - in milliseconds
 *
 * @return  none
 */
void SampleApp_SendFlashMessage( uint16 flashTime )  
{
  uint8 buffer[3];
  buffer[0] = (uint8)(SampleAppFlashCounter++);
  buffer[1] = LO_UINT16( flashTime );
  buffer[2] = HI_UINT16( flashTime );

  if ( AF_DataRequest( &SampleApp_Flash_DstAddr, &SampleApp_epDesc,
                       SAMPLEAPP_FLASH_CLUSTERID,
                       3,
                       buffer,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
  }
  else
  {
    // Error occurred in request to send.
  }
}
void SampleApp_Send_P2P_Message(void){
   
}
/*********************************************************************
*********************************************************************/
 uint16 ReadGasData( void )
{
  uint16 reading = 0;
  
  /* Enable channel */
  ADCCFG |= 0x40;
  
  /* writing to this register starts the extra conversion */
  ADCCON3 = 0x86;// AVDD5 引脚  00： 64 抽取率(7 位ENOB)  0110： AIN6
  
  /* Wait for the conversion to be done */
  while (!(ADCCON1 & 0x80));
  
  /* Disable channel after done conversion */
  ADCCFG &= (0x40 ^ 0xFF); //按位异或。如1010^1111=0101（二进制）
  
  /* Read the result */
  reading = ADCL;
  reading |= (int16) (ADCH << 8); 
  
  reading >>= 8;
  
  return (reading);
} 