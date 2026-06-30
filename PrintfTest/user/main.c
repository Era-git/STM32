#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include <stdio.h>
#include <delay.h>

void My_USART_SendBytes(USART_TypeDef *USARTx, uint8_t *pData, uint16_t Size);

void My_USART1_Init(void);

int main(void)
{
	Delay_Init();
	My_USART1_Init();

//	uint8_t bytesToSend[] = {1,2,3,4,5};
//	
//	My_USART_SendBytes(USART1, bytesToSend, 5);
//	printf("Hello World!\r\n");
	
	while(1)
	{
		uint32_t currentTick = GetTick();//获取当前时间
		uint32_t miliseconds = currentTick % 1000;
		currentTick = currentTick / 1000;
		uint32_t seconds = currentTick % 60;
		currentTick = currentTick /60;
		uint32_t minutes = currentTick % 60;
		currentTick = currentTick /60;
		uint32_t hour = currentTick;
		
		printf("%02u:%02u:%02u.%03u\r\n",hour, minutes, seconds, miliseconds);
		
		Delay(100);
	}
}

//
// @简介：通过串口发送多个字节
// @参数 USARTx:填写串口的名称
// @参数 pData:要发送的数据
// @参数 Size：要发送数据的数量，单位是字节
//
void My_USART_SendBytes(USART_TypeDef *USARTx, uint8_t *pData, uint16_t Size)
{
	for(uint32_t i = 0;i < Size;i++)
	{
		//1.等待发送数据寄存器为空
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
		
		//2.写入发送数据寄存器中
		USART_SendData(USART1, pData[i]);
	}
	//3.等待发送完成
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
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

int fputc(int ch, FILE *f){
	
	//1.等待发送数据寄存器为空
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
		
	//2.写入发送数据寄存器中
	USART_SendData(USART1, (uint8_t)ch);
	
	return ch;
}
