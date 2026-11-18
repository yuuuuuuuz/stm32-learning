#include "stm32f10x.h"  // Device header
#include "OLED.h"
#include "Serial.h"

int main(void)
{
	OLED_Init();
	Serial_Init();
	
	OLED_ShowString(1,1,"Hello");
	
	while(1)
	{
		
	}	
}
