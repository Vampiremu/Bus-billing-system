#include "bsp_clk.h"
#include "bsp_delay.h"
#include "bsp_led.h"
#include "bsp_beep.h"
#include "bsp_key.h"
#include "bsp_int.h"
#include "bsp_uart.h"
#include "stdio.h"
#include "bsp_lcd.h"
#include "bsp_lcdapi.h"
#include "bsp_spi.h"
#include "rc522_config.h"
#include "rc522_function.h"

#define ADDR 0x05

char IC_test ( void );



/* 背景颜色索引 */
unsigned int backcolor[10] = {
	LCD_BLUE, 		LCD_GREEN, 		LCD_RED, 	LCD_CYAN, 	LCD_YELLOW, 
	LCD_LIGHTBLUE, 	LCD_DARKBLUE, 	LCD_WHITE, 	LCD_BLACK, 	LCD_ORANGE

}; 
	

void title_show(void)
{
	tftlcd_dev.forecolor = LCD_RED;	  
	lcd_show_string(10,10,400,32,32,(char*)"Bus Billing System V0.1");  /* 显示字符串 */
	tftlcd_dev.forecolor = LCD_BLACK;
	lcd_drawline(10,60,790,60);
}

void homepage_show(void)
{
	tftlcd_dev.forecolor = LCD_BLACK;
	lcd_show_string(20,100,780,32,32,(char*)"Select mode(Send numbers using serial port):");
	lcd_show_string(20,150,400,32,32,(char*)"1:Payment mode");
	lcd_show_string(20,200,400,32,32,(char*)"2:Recharge mode");
	lcd_show_string(20,250,400,32,32,(char*)"3:Add card mode");
}


/*
 * @description	: main函数
 * @param 		: 无
 * @return 		: 无
 */
int main(void)
{
	char cs;
	unsigned char index = 0;
	unsigned char state = OFF;
	unsigned char ucArray_ID [ 4 ];
	unsigned char Key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	unsigned char Data[16] = {0};
	int i;
	// unsigned int command = 0;

	int_init(); 				/* 初始化中断(一定要最先调用！) */
	imx6u_clkinit();			/* 初始化系统时钟 			*/
	delay_init();				/* 初始化延时 			*/
	clk_enable();				/* 使能所有的时钟 			*/
	led_init();					/* 初始化led 			*/
	beep_init();				/* 初始化beep	 		*/
	uart_init();				/* 初始化串口，波特率115200 */
	lcd_init();					/* 初始化LCD 			*/
	RC522_init();
	
	title_show();
	homepage_show();


	while(1)				
	{	
		
		Data[15] = 100;
		PcdReset();
		cs = IC_CMT_r(ucArray_ID, Key, 0, ADDR, Data);
		cs = IC_CMT_r(ucArray_ID, Key, 1, ADDR, Data);
		if (cs == MI_ERR)
		{
			continue;
		}
		
		for(i = 0; i < 16; i++)
		{
			printf ("%d",Data[i]);
			Data[i] = 0;
		}
		printf ( "\r\n");
		// if ( IC_test() == MI_OK){
		// 	led_switch(LED0,1);
		// 	beep_switch(1);
		// 	delayms(500);
		// 	led_switch(LED0,0);
		// 	beep_switch(0);
		// }
		// else {
		// 	beep_switch(1);
		// 	delayms(300);
		// 	beep_switch(0);
		// 	delayms(300);
		// 	beep_switch(1);
		// 	delayms(300);
		// 	beep_switch(0);
		// }
		state = !state;
		// led_switch(LED0,state);
		delayms(500);	/* 延时一秒	*/
	}
	return 0;
}

/**
  * @brief  测试函数
  * @param  无
  * @retval 无
  */
char IC_test ( void )
{
	char cStatus;
	int i;
	char cStr [ 30 ];
  	unsigned char ucArray_ID [ 4 ];   //先后存放IC卡的类型和UID(IC卡序列号)
	unsigned char ucStatusReturn;     //返回状态
  	static unsigned char ucLineCount = 0; 
	unsigned char ID_type [ 2 ];
	unsigned char Key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	unsigned char Data[16] = {0};

	// for(i = 0; i < 16; i++)
	// {
	// 	Data[i] = i;
	// }
	
  	while ( 1 )
  	{ 
		if ( ( ucStatusReturn = PcdRequest ( PICC_REQALL, ucArray_ID ) ) != MI_OK ) //寻卡
      	{	
			//printf ("start fine card around\r\n"); 
			//若失败再次寻卡
			ucStatusReturn = PcdRequest ( PICC_REQALL, ucArray_ID );		                                                 
		}
		if ( ucStatusReturn == MI_OK  )
		{


			ID_type[ 0 ] = ucArray_ID[ 0 ];
			ID_type[ 1 ] = ucArray_ID[ 1 ];
      		//防冲撞（当有多张卡进入读写器操作范围时，防冲突机制会从其中选择一张进行操作）
			if ( PcdAnticoll ( ucArray_ID ) == MI_OK )                                                                   
			{
				sprintf ( cStr, "TYPE:%02X%02X",ID_type[ 0 ],ID_type[ 1 ]);
				printf("%s\r\n",cStr);


				sprintf ( cStr, ":::%02X%02X%02X%02X:::", 
                  ucArray_ID [ 0 ], ucArray_ID [ 1 ], 
                  ucArray_ID [ 2 ], ucArray_ID [ 3 ] );
							
				printf ( "%s\r\n",cStr ); 

				// cStatus = IC_CMT(ucArray_ID, Key, 0, 0x10, Data);
				// if (cStatus == MI_OK)
				// {
				// 	printf ( "write ok\r\n");
				// }
				// else
				// {
				// 	printf ( "write error\r\n");
				// }

				cStatus = IC_CMT(ucArray_ID, Key, 1, 0x10, Data);
				if (cStatus == MI_OK)
				{
					printf ( "read ok\r\n");
				}
				else
				{
					printf ( "read error\r\n");
				}
				
				for(i = 0; i < 16; i++)
				{
					printf ("%d",Data[i]);
				}
				printf ( "\r\n" );
				
					
				tftlcd_dev.forecolor = LCD_BLACK;
				lcd_clear(LCD_WHITE);
				lcd_show_string(50,100,400,32,32,(char*)cStr);
				if((ucArray_ID[0] == 154) && (ucArray_ID[1] == 5) && (ucArray_ID[2] == 144) && (ucArray_ID[3] == 102))
				{
					lcd_show_string(50,150,400,32,32,(char*)"Is Your Phone !");
					// count[0] += 1;
					// sprintf( cStr, "Swipe %d times",count[0]);
					printf ( "%s\r\n",(char*)"Is Your Phone !" ); 
				}
				return MI_OK;						
			}			
		}		
  }		
}
