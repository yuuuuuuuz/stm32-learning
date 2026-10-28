#include "stm32f10x.h"                  // Device header

void MyDMA_Init(uint32_t AddrA,uint32_t AddrB,uint16_t Size)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	
	DMA_InitTypeDef DMA_Initstructure;
	DMA_Initstructure.DMA_PeripheralBaseAddr=AddrA;
	DMA_Initstructure.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	DMA_Initstructure.DMA_PeripheralInc=DMA_PeripheralInc_Enable;
	DMA_Initstructure.DMA_MemoryBaseAddr=AddrB;
	DMA_Initstructure.DMA_MemoryDataSize=DMA_PeripheralDataSize_Byte;
	DMA_Initstructure.DMA_MemoryInc=DMA_PeripheralInc_Enable;
	DMA_Initstructure.DMA_DIR=DMA_DIR_PeripheralSRC;
	DMA_Initstructure.DMA_BufferSize=Size;
	DMA_Initstructure.DMA_Mode=DMA_Mode_Normal;
	DMA_Initstructure.DMA_M2M=DMA_M2M_Enable;
	DMA_Initstructure.DMA_Priority=DMA_Priority_Medium;
	DMA_Init(DMA1_Channel1,& DMA_Initstructure);
	
	DMA_Cmd(DMA1_Channel1,ENABLE);
}
