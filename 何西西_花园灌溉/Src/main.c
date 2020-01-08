/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dht11.h"
#include "flash.h"
#include "GY-30.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t min_humi,max_humi;//土壤湿度的上、下限值
uint8_t rxBufLen = 0;
uint8_t rxByte;
uint8_t aRxBuffer2[64]={0};//串口2数组
uint8_t pump = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void ReadDataFromFlash(){
		uint32_t addr = 0x08008000;
		unsigned char buf[12]={0};
		for(int i = 0;i<4;i++){
				buf[i] = printFlashTest(addr);
				addr+=4;
		}
		min_humi = (buf[0]-'0')*10+buf[1]-'0';
		max_humi = (buf[2]-'0')*10+buf[3]-'0';
	 
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */





/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	DHT11_Data_TypeDef  DHT11_Data;
	uint8_t ADC_value = 0;
	uint8_t i;
	uint32_t ad1,ad2;
	uint8_t isAutoMode = 1;	//自动/手动模式标志位
	uint32_t time = 0;//计时器
	__IO uint16_t ADC_ConvertedValue[12];
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM7_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	DHT11_Init();//温湿度模块初始化
	//writeFlashTest(0x08008000,"7585",4);    //可以取消此行注释来向FLASH写入土壤湿度范围数据

	Init_BH1750();//光照初始化
	//关闭两个LED
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_SET);
	//开启ADC
	if(HAL_ADC_Start_DMA(&hadc1,(uint32_t*)&ADC_ConvertedValue[0],12)!=HAL_OK){
		 Error_Handler();
	}
	HAL_UART_Receive_IT(&huart2,&rxByte,1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
		
		//writeFlashTest(0x08008000,"7583",4);    //可以取消此行注释来向FLASH写入土壤湿度范围数据
	 

		if(!HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_2)){ 							//  按下KEY2,开启/关闭手动模式
				isAutoMode ^=1;
		}
		/*
		if(!HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_3)){
			if(pump){
				pump^=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,0);//关水泵
			}
			else{
				pump^=1;
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,1);//开水泵
			}
		}
		*/
	 	float light = Value_GY30();
		HAL_Delay(200);
		time+=200;
		if(time>=1000){
			time = 0;
			//获取光照
		
			ReadDataFromFlash();//读取flash土壤湿度范围
				//获取温湿度
				unsigned char buf[20]={0};
				int res = DHT11_Read_TempAndHumidity(&DHT11_Data);
				if(res==2){ 
					 
				}
				else
					sprintf(buf,"0000\n");
			//获取土壤湿度
				unsigned int i = 0;
				float	ADC_Value = 0;
				for(i = 0; i < 12; ++i)
				{
					ADC_Value += ADC_ConvertedValue[i];
				}
				ADC_Value = ADC_Value / 12;
				ADC_Value = (ADC_Value-300);
				//如果土壤湿度大于max——humi，蜂鸣器报警，开启水泵
			if(ADC_Value>max_humi&&isAutoMode){
				HAL_GPIO_WritePin(GPIOF,GPIO_PIN_8,GPIO_PIN_SET);  //蜂鸣器
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);//开水泵
				 HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_RESET);//红灯亮
				HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_SET);//绿灯灭
			}
			else if(ADC_Value<=max_humi&&isAutoMode){
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET); //关水泵
				HAL_GPIO_WritePin(GPIOF,GPIO_PIN_8,GPIO_PIN_RESET);//蜂鸣器闭嘴
				HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_RESET);//绿灯亮
				HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_SET);//红灯灭
			}
			else if(!isAutoMode){
				HAL_GPIO_WritePin(GPIOF,GPIO_PIN_9,GPIO_PIN_SET);//红灯灭
				HAL_GPIO_WritePin(GPIOF,GPIO_PIN_10,GPIO_PIN_SET);//绿灯灭
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);//开水泵
				HAL_GPIO_WritePin(GPIOF,GPIO_PIN_8,GPIO_PIN_RESET);//蜂鸣器闭嘴                                                                     
			}
			
			if(light<400){
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,1);//LED亮
			}
			else{
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,0);//LED灭
			}
			sprintf(buf,"%2d%2d%3.1f%d%4d%2d%2d\n",(int)DHT11_Data.humidity,(int)DHT11_Data.temperature,ADC_Value,isAutoMode,(int)light,min_humi,max_humi);
			HAL_UART_Transmit(&huart1,buf,sizeof(buf),10);
			
		}               
		
			
	 
			 
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /**Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  /**Enables the Clock Security System 
  */
  HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function Should not be modified, when the callback is needed,
           the HAL_UART_TxCpltCallback could be implemented in the user file
   */
	if(huart->Instance == USART2)
	{
		if(rxBufLen>64)
		{
			rxBufLen=0;
			HAL_UART_Transmit(&huart1,(uint8_t *)"Too Long!\r\n",11,1000);
			memset(aRxBuffer2,0,64);
		}
		else 
		{
			aRxBuffer2[rxBufLen++]=rxByte;
			if(aRxBuffer2[rxBufLen-1]==0x0A&&aRxBuffer2[rxBufLen-2]==0x0D)
			{
				//HAL_UART_Transmit(&huart1,aRxBuffer2,rxBufLen,1000);
				if(strstr(aRxBuffer2,"CONNECT")){
					uint8_t str[]="蓝牙连接成功!\n";
					HAL_UART_Transmit(&huart1,str,sizeof(str),1000);
				}
				else if(strstr(aRxBuffer2,"DISC")){
					uint8_t str[]="蓝牙断开连接!\n";
					HAL_UART_Transmit(&huart1,str,sizeof(str),1000);
				}
			
				rxBufLen=0;
				memset(aRxBuffer2,0,64);
			}
			else if(aRxBuffer2[0]=='A'&&aRxBuffer2[1]=='T'&&aRxBuffer2[6]=='\n'){
					uint8_t temp[4]={0};
					uint8_t res[4]={0};//用于判断是否写入
					for(int i = 0;i<4;i++){
						temp[i] = aRxBuffer2[i+2];
					}
					writeFlashTest(0x08008000,temp,4); 
					uint8_t str[]="设置温湿度阈值成功!\n";
					HAL_UART_Transmit(&huart1,str,sizeof(str),1000);
					rxBufLen=0;
					memset(aRxBuffer2,0,64);
				
			}
		}
		HAL_UART_Receive_IT(&huart2,&rxByte,1);
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	HAL_UART_Transmit(&huart1,"adc start failed\n",sizeof("adc start failed\n"),10);
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
