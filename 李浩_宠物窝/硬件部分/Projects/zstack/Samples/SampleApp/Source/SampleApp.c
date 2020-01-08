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
  PROVIDED �AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
#include "hal_timer.h"
#include "MT_UART.h"
#include "MT_APP.h"
#include "MT.h"

#include <stdio.h>
#include <string.h>

//MY DEVICE
#include "DHT11.h"
#include "gy30.h"
#include "HX711.h"
/*********************************************************************
 * MACROS
 */


#define GapValue 430
#define MotorData P1  
#define MAN P0_1
#define A P1_1
#define B P1_2
#define C P1_3
#define D P1_4

long HX711_Buffer = 0;
long Weight_Maopi = 0,Weight_Shiwu = 0;
int current_round = 0;
byte make_up = 0;
uchar phasecw[4] ={0x10,0x08,0x04,0x02};//��ת �����ͨ���� D-C-B-A
uchar phaseccw[4]={0x02,0x04,0x08,0x10};//��ת �����ͨ���� A-B-C-D
//��ȡëƤ
void Get_Maopi()
{
	Weight_Maopi = HX711_Read();		
}
long Get_Weight()
{
	HX711_Buffer = HX711_Read();
	Weight_Shiwu = HX711_Buffer;
	Weight_Shiwu = Weight_Shiwu - Weight_Maopi;				//获取实物的AD采样数值�?	Weight_Shiwu = (long)((float)Weight_Shiwu/GapValue); 	
	return Weight_Shiwu;
}
void MotorCW()
{
 uchar i;
 A=1;B=C=D=0;Delay_ms(4);
 B=1;A=C=D=0;Delay_ms(4);
 C=1;A=B=D=0;Delay_ms(4);
 D=1;A=B=C=0;Delay_ms(4);
}
//��ʱ��ת��
void MotorCCW(void)
{
 uchar i;
  D=1;A=B=C=0;Delay_ms(4);
  C=1;A=B=D=0;Delay_ms(4);
  B=1;A=C=D=0;Delay_ms(4);
  A=1;B=C=D=0;Delay_ms(4);
  
 /*
 for(i=0;i<4;i++)
  {
   MotorData=phaseccw[i];
   Delay_ms(4);//ת�ٵ���
  }
 */
}
//ֹͣת��
void MotorStop(void)
{
 MotorData=0x00;
}
/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 AppTitle[] = "ALD2530 Example"; //Ӧ�ó������� 
 

 

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
  SampleApp_TaskID = task_id;   //osal���������ID�����û���������������ı�
  SampleApp_NwkState = DEV_INIT;//�豸״̬�趨ΪZDO���ж���ĳ�ʼ��״̬
  SampleApp_TransID = 0;        //��Ϣ����ID������Ϣʱ��˳��֮�֣�
#ifndef ZDO_COORDINATOR
   LIGHT_INIT();//��ʼ��  ���� 
   P0DIR |=0x20;   //P05  SCK
    
    P0SEL &=~0x41; //P06  DT  P01 ����
    P0DIR &=~0X41;
    
    P2SEL &=~0X02;
    P2DIR &=~0X02;//P21 ���
 
    
    P1DIR |=0xFE;//LED P15 16 17 ���
    //P1DIR |=0X1E;//���
    
    //HalTimerConfig(HAL_TIMER_1,HAL_TIMER_MODE_NORMAL,HAL_TIMER_CHANNEL_SINGLE,HAL_TIMER_CH_MODE_OVERFLOW,1,MotorCW);
    
    Get_Maopi();  

#endif
  //��������
  MT_UartInit();
  MT_UartRegisterTaskID(task_id); 
 
  HalUARTWrite(0,"UartInit OK\n", sizeof("UartInit OK\n")); 


  //HalUARTWrite(0,"get", sizeof("get")); 
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

//�öε���˼�ǣ����������HOLD_AUTO_START�궨�壬����������оƬ��ʱ�����ͣ����
//���̣�ֻ���ⲿ�����Ժ�Ż�����оƬ����ʵ������Ҫһ����ť���������������̡�  
#if defined ( HOLD_AUTO_START )
  // HOLD_AUTO_START is a compile option that will surpress ZDApp
  //  from starting the device and wait for the application to
  //  start the device.
  ZDOInitDevice(0);
#endif

  // Setup for the periodic message's destination address ���÷������ݵķ�ʽ��Ŀ�ĵ�ַѰַģʽ
  // Broadcast to everyone ����ģʽ:�㲥����
  SampleApp_Periodic_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;//�㲥
  SampleApp_Periodic_DstAddr.endPoint = SAMPLEAPP_ENDPOINT; //ָ���˵��
  SampleApp_Periodic_DstAddr.addr.shortAddr = 0xFFFF;//ָ��Ŀ�������ַΪ�㲥��ַ

  // Setup for the flash command's destination address - Group 1 �鲥����
  SampleApp_Flash_DstAddr.addrMode = (afAddrMode_t)afAddrGroup; //��Ѱַ
  SampleApp_Flash_DstAddr.endPoint = SAMPLEAPP_ENDPOINT; //ָ���˵��
  SampleApp_Flash_DstAddr.addr.shortAddr = SAMPLEAPP_FLASH_GROUP;//���0x0001

  // Fill out the endpoint description. ���屾�豸����ͨ�ŵ�APS��˵�������
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT; //ָ���˵��
  SampleApp_epDesc.task_id = &SampleApp_TaskID;   //SampleApp ������������ID
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;//SampleApp��������
  SampleApp_epDesc.latencyReq = noLatencyReqs;    //��ʱ����

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );    //��AF��Ǽ�������

  // Register for all key events - This app will handle all key events
  RegisterForKeys( SampleApp_TaskID ); // �Ǽ����еİ����¼�

  // By default, all devices start out in Group 1
  SampleApp_Group.ID = 0x0001;//���
  osal_memcpy( SampleApp_Group.name, "Group 1", 7  );//�趨����
  aps_AddGroup( SAMPLEAPP_ENDPOINT, &SampleApp_Group );//�Ѹ���Ǽ���ӵ�APS��

#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "SampleApp", HAL_LCD_LINE_1 ); //���֧��LCD����ʾ��ʾ��Ϣ
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
//�û�Ӧ��������¼�������
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG ) //����ϵͳ��Ϣ�ٽ����ж�
  {
    //�������ڱ�Ӧ������SampleApp����Ϣ����SampleApp_TaskID���
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // Received when a key is pressed
        case KEY_CHANGE://�����¼�
          SampleApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        // Received when a messages is received (OTA) for this endpoint
        case AF_INCOMING_MSG_CMD://���������¼�,���ú���AF_DataRequest()��������
          SampleApp_MessageMSGCB( MSGpkt );//���ûص��������յ������ݽ��д���
          break;

        // Received whenever the device changes state in the network
        case ZDO_STATE_CHANGE:
          //ֻҪ����״̬�����ı䣬��ͨ��ZDO_STATE_CHANGE�¼�֪ͨ���е�����
          //ͬʱ��ɶ�Э������·�������ն˵�����
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          //if ( (SampleApp_NwkState == DEV_ZB_COORD)//ʵ����Э����ֻ������������ȡ�������¼�
          if ( (SampleApp_NwkState == DEV_ROUTER) || (SampleApp_NwkState == DEV_END_DEVICE) )
          {
            // Start sending the periodic message in a regular interval.
            //�����ʱ��ֻ��Ϊ����������Ϣ�����ģ��豸������ʼ��������￪ʼ
            //������һ��������Ϣ�ķ��ͣ�Ȼ���ܶ���ʼ��ȥ
            osal_start_timerEx( SampleApp_TaskID,
                              SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                              SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
         /*
            osal_start_timerEx( 0X1,
                              PUMP_EVT,
                              SAMPLEAPP_PUMP_TIMEOUT );
           */
          }
          else
          {
              
          }
          break;

        default:
          break;
      }

      // Release the memory �¼��������ˣ��ͷ���Ϣռ�õ��ڴ�
      osal_msg_deallocate( (uint8 *)MSGpkt );
   
      // Next - if one is available ָ��ָ����һ�����ڻ������Ĵ�������¼���
      //����while ( MSGpkt )���´����¼���ֱ��������û�еȴ������¼�Ϊֹ
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }

    // return unprocessed events ����δ������¼�
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out - This event is generated by a timer
  //  (setup in SampleApp_Init()).
  if ( events & SAMPLEAPP_SEND_PERIODIC_MSG_EVT )
  {
    // Send the periodic message �����������¼���
    //����SampleApp_SendPeriodicMessage()�����굱ǰ���������¼���Ȼ��������ʱ��
    //������һ�����������飬����һ��ѭ����ȥ��Ҳ��������˵���������¼��ˣ�
    //������Ϊ��������ʱ�ɼ����ϴ�����
    SampleApp_SendPeriodicMessage();
    
    // Setup to send message again in normal period (+ a little jitter)
    osal_start_timerEx( SampleApp_TaskID, SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
        (SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT + (osal_rand() & 0x00FF)) );

    // return unprocessed events ����δ������¼�
    return (events ^ SAMPLEAPP_SEND_PERIODIC_MSG_EVT);
  }
  /*
  else if(events & PUMP_EVT){
    MotorCW();
    
    osal_start_timerEx( 0X1, PUMP_EVT,
        (SAMPLEAPP_PUMP_TIMEOUT + (osal_rand() & 0x00FF)) );
    
     return (events ^ PUMP_EVT);
  }
  */
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
//�������ݣ�����Ϊ���յ�������
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  uint16 flashTime;
  unsigned short len = 6+5+6+3;
  byte buf[6+5+6+3]; 
  
  switch ( pkt->clusterId ) //�жϴ�ID
  {
    case SAMPLEAPP_PERIODIC_CLUSTERID: //�յ��㲥����
      osal_memset(buf, 0 , len);
      osal_memcpy(buf, pkt->cmd.Data, len); //�������ݵ���������
      HalUARTWrite(0,buf, len);
   
      break;

    case SAMPLEAPP_FLASH_CLUSTERID: //�յ��鲥����  
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
//��������������Ϣ
void SampleApp_SendPeriodicMessage( void )
{ 
 
  //��õ�����ͨ�����������������ʾ  
  char str[6+5+6+3]={0};
  //��ȡ��ʪ��
  unsigned char temp_humi[6]={0};
  strcpy(temp_humi,DHT11());
  unsigned int lt = Light();//��ȡ��ǿ
  //��ȡ���
  /*
  if(��ˮ)
    ����C
  if(����)
    ����
  if(�¶ȹ���)
    ����
  if(ѹ��)
    ���
  */
  byte man = MAN==1?1:0;
  float w =0;
 
  w = HX711_Read();
  float true_weight = w-Weight_Maopi;
  if(true_weight<0)
    true_weight=0;
  uint8 rain = 0;
  rain = P2_1==1?0:1;
  sprintf(str,"%4.3f%5d%4s%d%d",(float)(true_weight)/1000,lt,temp_humi,man,rain);
  

  HalUARTWrite(0,str,sizeof(str));
   // ����AF_DataRequest���������߹㲥��ȥ
  if( AF_DataRequest( &SampleApp_Periodic_DstAddr,//����Ŀ�ĵ�ַ���˵��ַ�ʹ���ģʽ
                       &SampleApp_epDesc,//Դ(�𸴻�ȷ��)�ն˵��������������ϵͳ������ID�ȣ�ԴEP
                       SAMPLEAPP_PERIODIC_CLUSTERID, //��Profileָ������Ч�ļ�Ⱥ��
                       sizeof(str),       //�������ݳ���
                       (uchar*)str,// �������ݻ�����
                       &SampleApp_TransID,     // ����ID��
                       AF_DISCV_ROUTE,      // ��Чλ����ķ���ѡ��
                       AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )  //����������ͨ������ΪAF_DEFAULT_RADIUS
  {
  }
  else
  {
    HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
    // Error occurred in request to send.
  }
  //ѹ��С��50��С��300
  if(true_weight<50&&make_up==0)
    make_up =1;
  if(make_up&&true_weight<300)
  {
    for(int i = 0;i<10;i++)  //160ms
       MotorCW();
    current_round+=10;
  }
  if(true_weight>300){
    if(current_round>0){
      for(int i = 0;i<10;i++)  //160ms
         MotorCCW();    
      current_round-=10;
    }
    make_up = 0;
  }
 

  //��⵽����,P15��
  
  if(man)
    P1_5 = 1;
  else
    P1_5 = 0;
  //�¶�С��25�ȣ�P17��
  if(temp_humi[0]<'2'||(temp_humi[0]==2&&temp_humi[1]<'5'))
    P1_7=1;
  else
    P1_7 = 0;
  if(rain)//��⵽���
    P1_6 = 1;
  else
    P1_6=0;
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
 