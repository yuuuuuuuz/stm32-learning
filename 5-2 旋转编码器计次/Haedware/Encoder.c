#include "stm32f10x.h"                  // Device header

int16_t Encoder_Count;

void Encoder_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct; 
	NVIC_InitTypeDef NVIC_InitStruct;  	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);     //开时钟GPIOB
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);      //开时钟AFIO
	
	//GPIO_InitTypeDef GPIO_InitStructure;             
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;        //配置GPIO
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource0);    //配置AFIO
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource1);
	
	//EXTI_InitTypeDef EXTI_InitStruct;              
	EXTI_InitStruct.EXTI_Line=EXTI_Line0 | EXTI_Line1;         //配置EXTI：将EXTI的第14个线路配置为中断模式，下降沿触发，开启中断
	EXTI_InitStruct.EXTI_LineCmd=ENABLE;
	EXTI_InitStruct.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger=EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStruct);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);            //配置NVIC
	
	//NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannel=EXTI0_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&NVIC_InitStruct);
	
	NVIC_InitStruct.NVIC_IRQChannel=EXTI1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority=2;
	NVIC_Init(&NVIC_InitStruct);
}

int16_t Encoder_Get(void)
{
	int16_t Temp;
	Temp=Encoder_Count;
	Encoder_Count = 0;
	return Temp;
}
 
void  EXTI0_IRQHandler (void)
{
	if(EXTI_GetITStatus(EXTI_Line0) == SET)
	{
		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1) == 0)
		{
			Encoder_Count --;
		}
		EXTI_ClearITPendingBit(EXTI_Line0);
	}
}

void  EXTI1_IRQHandler (void)
{
	if(EXTI_GetITStatus(EXTI_Line1) == SET)
	{
		if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0) ==0 )
		{
			Encoder_Count++;
		}
		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}
