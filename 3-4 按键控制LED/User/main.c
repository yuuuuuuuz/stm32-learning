#include "stm32f10x.h"    // Device header
#include "Delay.h"
#include "LED.h"
#include "Key.h"

uint8_t KeyNum;

int main()
{
	LED_Init();
	Key_Init();

	while(1)
	{
		
		KeyNum=Key_GetNum();
		if(KeyNum == 1)
		{
			LED1_Turn();
		}
		if(KeyNum == 2)
		{
			LED2_Turn();
		}
		
		
		//GPIO_ResetBits(GPIOA, GPIO_Pin_1);   // 输出低
		//Delay_ms(500);
		//GPIO_SetBits(GPIOA, GPIO_Pin_1);     // 输出高
		//Delay_ms(1000);
		
	}
}
