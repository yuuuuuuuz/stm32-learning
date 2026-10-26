#include "stm32f10x.h"    // Device header
#include "Delay.h"
#include "OLED.H"
#include "MOTOR.H"
#include "Key.h"

uint8_t KeyNum;

int main()
{
	OLED_Init();
	Motor_Init();
	
	Motor_SetSpeed(100);

	while(1)
	{
		
	}
}
