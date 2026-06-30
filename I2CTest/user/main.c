#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"

void My_I2C_Init(void);
int My_I2C_SendBytes(I2C_TypeDef *I2Cx, uint8_t Addr, uint8_t *pDaata, uint16_t Size);

int main(void)
{
	My_I2C_Init();
	
	uint8_t commands[] = {0x00, 0x8d, 0x14, 0xaf, 0xa5,};		
	
	My_I2C_SendBytes(I2C1, 0X78, commands, 5);
	
	while(1)
	{
	}
}

void My_I2C_Init(void)
{
	//#1.IO引脚初始化
	
	//对I2C进行重映射AFIO
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
	
	//对PB8,PB9进行初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	//#2.初始化I2C1模块
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);//开启I2C复位时钟
	
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);//施加复位信号
	
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);//释放复位信号
	
	I2C_InitTypeDef I2C_InitStruct;
	
	I2C_InitStruct.I2C_ClockSpeed = 400000;
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
	
	I2C_Init(I2C1, &I2C_InitStruct);
	
	I2C_Cmd(I2C1, ENABLE);//闭合总开关
	
}

int My_I2C_SendBytes(I2C_TypeDef *I2Cx, uint8_t Addr, uint8_t *pData, uint16_t Size)
{
	//#1.等待总线空闲
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) == SET);
	
	//#2.发送起始位
	I2C_GenerateSTART(I2Cx, ENABLE);//写1
	
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_SB) == RESET);
	
	//#3.寻址阶段
	I2C_ClearFlag(I2Cx, I2C_FLAG_AF);//清除AF
	
	//发送地址+RW#
	I2C_SendData(I2Cx, Addr & 0xfe);
	
	while(1){
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_ADDR) == SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
		{
			I2C_GenerateSTOP(I2Cx, ENABLE);
			return -1;//寻址失败
		}
		
	}
	//清除ADDR
	I2C_ReadRegister(I2Cx, I2C_Register_SR1);
	I2C_ReadRegister(I2Cx, I2C_Register_SR2);
	
	//#4.发送数据
	
	for(uint16_t i = 0;i < Size;i++)
	{
		
		// 等待TXE（发送缓冲区空）
        while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_TXE) == RESET);
        
		
        // 检查AF（从机是否NACK）
        if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
        {
            I2C_GenerateSTOP(I2Cx, ENABLE);
			
            return -2;  // 数据被拒收，NACK
        }
        
        // 发送数据
        I2C_SendData(I2Cx, pData[i]);
	}
	
	// 等待最后一个字节发送完成（BTF = 1）
    while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BTF) == RESET);
    
    //#5. 发送停止位
    I2C_GenerateSTOP(I2Cx, ENABLE);
    
    return 0;  // 发送成功
	
}

