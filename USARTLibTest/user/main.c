#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "usart.h"


void My_USART1_Init(void);

int main(void)
{
	My_USART1_Init();
	
//	My_USART_SendByte(USART1, 0x5a);
	
//	uint8_t byteArray[] = {1,2,3,4,5};
//	My_USART_SendBytes(USART1, byteArray, 5);
	
//	My_USART_SendChar(USART1, 'A');
	
//	My_USART_SendString(USART1, "HELLO WORLD");
	
//	const char *strName = "Era";
//	My_USART_Printf(USART1, "Hi, %s! Nice to meet you! \r\n", strName);
	
	
//#初始化GPIOC PC13引脚
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	
	GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	
	while(1)
	{
//		char c = My_USART_ReceiveByte(USART1);
//		
//		if(c == '0')//字符0，不是数字0，换成2，3，4都一样可以
//		{
//			GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);//Bit_SET（输出 1 / 高阻，开漏模式下） → PC13 不导通 → LED 无电流 → 熄灭
//		}
//		else if(c == '1')
//		{
//			GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);//Bit_RESET（输出 0） → PC13 被拉低到 GND → LED 导通 → 点亮
//		}
		char buffer[100];
		
		if(My_USART_ReceiveLine(USART1, buffer, 100, LINE_SEPERATOR_CRLF, -1) == 0)
		{
			My_USART_SendString(USART1, buffer);
		}
		
	}
}
//
// @简介：对USART1进行初始化
//        PB6 - Tx , PB7 - Rx
//     115200, 8, 1, None, 双向 
// 
void My_USART1_Init(void)
{
	// #1.初始化PB6和PB7
	GPIO_InitTypeDef GPIO_InitStruct;
	
//	//Tx PA9 复用输出推挽
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//	
//	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStruct);
//	
//	//Rx PA10 输入上拉
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
//	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
//	GPIO_Init(GPIOA, &GPIO_InitStruct);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);//使能AFIO模块的时钟
	
	GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
	
	//Tx PB6 输出推挽
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	//Rx PB7 输入上拉
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	

	// #2.初始化串口USART1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	USART_InitTypeDef USART_InitStruct;
	
	USART_InitStruct.USART_BaudRate = 115200; //波特率
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;//双向
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;//8位数据位
	USART_InitStruct.USART_Parity = USART_Parity_No;//无校验
	USART_InitStruct.USART_StopBits = USART_StopBits_1;//1位停止位
	
	USART_Init(USART1, &USART_InitStruct);
	
	USART_Cmd(USART1, ENABLE);//使能USART1
}
