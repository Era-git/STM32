#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include <Delay.h>

int main(void)
{
	// 1. 开启GPIOC的时钟（PC13属于GPIOC）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 2. 初始化GPIO结构体
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;   // 开漏输出
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;


    // 3. 调用初始化函数
    GPIO_Init(GPIOA, &GPIO_InitStruct);
	
 // GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET); //写1
//	GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET); //写0
	
	
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	while(1)
	{
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == Bit_RESET)
		{
			GPIO_WriteBit(GPIOA, GPIO_Pin_0,Bit_SET);//写1
		}
		else{
			GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);//写0
		}
////		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET); //写0
////		Delay(100);
////		GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET); //写1
////		Delay(100);
		
	}
}
