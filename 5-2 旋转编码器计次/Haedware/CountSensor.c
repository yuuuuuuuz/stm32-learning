#include "stm32f10x.h"       // Device header

uint16_t CountSensor_Count;

void CountSensor_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStruct; 
	NVIC_InitTypeDef NVIC_InitStruct;  	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);     //开时钟GPIOB
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);      //开时钟AFIO
	
	//GPIO_InitTypeDef GPIO_InitStructure;             
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPU;        //配置GPIO
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource14);    //配置AFIO
	
	//EXTI_InitTypeDef EXTI_InitStruct;              
	EXTI_InitStruct.EXTI_Line=EXTI_Line14;         //配置EXTI：将EXTI的第14个线路配置为中断模式，下降沿触发，开启中断
	EXTI_InitStruct.EXTI_LineCmd=ENABLE;
	EXTI_InitStruct.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger=EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStruct);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);            //配置NVIC
	
	//NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannel=EXTI15_10_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&NVIC_InitStruct);
}

uint16_t CountSensor_Get()
{
	return CountSensor_Count;
}

void EXTI15_10_IRQHandler()
{
	if (EXTI_GetITStatus(EXTI_Line14) == SET )
	{
		CountSensor_Count++;
		EXTI_ClearITPendingBit(EXTI_Line14);
	}
}
