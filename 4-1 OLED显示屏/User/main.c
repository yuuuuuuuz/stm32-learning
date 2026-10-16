#include "stm32f10x.h"    // Device header
#include "Delay.h"
#include "OLED.H"

int main()
{
	OLED_Init();
	
	OLED_ShowChar(1,1,'A');
	OLED_ShowString(1,3,"HelloWord!");
	OLED_ShowNum(2,1,12345,5);
	OLED_ShowSignedNum(2,7,12345,5);

	while(1)
	{
		
	}
}
