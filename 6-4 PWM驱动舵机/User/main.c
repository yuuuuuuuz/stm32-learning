#include "stm32f10x.h"    // Device header
#include "Delay.h"
#include "OLED.H"
#include "Servo.H"
#include "Key.h"

uint8_t KeyNum;
float Angle;

int main()
{
	OLED_Init();
	PWM_Init();
	Key_Init();
	
	Servo_SetAngle(90);
	
	while(1)
	{
		
	}
}
