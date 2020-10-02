#include "rc522_config.h"
#include "rc522_function.h"
#include "bsp_delay.h"
#include "bsp_spi.h"
#include "stdio.h"

//写RC522寄存器
void rc522_WriteData(unsigned char reg, unsigned char value)
{
    unsigned char reg_a;
    reg_a = (reg << 1) & 0x7E;

    RC522_CSN(0);
    spich0_readwrite_byte(ECSPI3, reg_a);
    spich0_readwrite_byte(ECSPI3, value);
    RC522_CSN(1);
}

//读RC522寄存器
unsigned char rc522_ReadData(unsigned char reg)
{
    unsigned char reg_val = 0;
    unsigned char reg_a;
    reg_a = ((reg << 1) & 0x7E) | 0x80;

    RC522_CSN(0);
    spich0_readwrite_byte(ECSPI3, reg_a);
    reg_val = spich0_readwrite_byte(ECSPI3, NULL);
    RC522_CSN(1);

    return reg_val;
}

//对RC522寄存器置位
//ucReg:    地址
//ucMask:   置位值
void SetBitMask(unsigned char ucReg, unsigned char ucMask)
{
    unsigned char reg_val;

    reg_val = rc522_ReadData(ucReg);
    rc522_WriteData(ucReg, reg_val | ucMask);
}

//对RC522寄存器清零
//ucReg:    地址
//ucMask:   清位值
void ClearBitMask(unsigned char ucReg, unsigned char ucMask)
{
    unsigned char reg_val;

    reg_val = rc522_ReadData(ucReg);
    rc522_WriteData(ucReg, reg_val & (~ucMask));
}

//开启天线
void PcdAntennaOn(void)
{
    unsigned char uc;

    uc = rc522_ReadData(TxControlReg);
    if(! (uc & 0x03))
        SetBitMask(TxControlReg, 0x03);
}

//关闭天线
void PcdAntennaOff(void)
{
    ClearBitMask(TxControlReg, 0x03);
}

//复位RC522
void PcdReset(void)
{
    RC522_RST(1);
    delayus(1);

    RC522_RST(0);
    delayus(20);

    RC522_RST(1);
    delayus(10);

    rc522_WriteData(CommandReg, 0x0f);
    while(rc522_ReadData(CommandReg) & 0x10);

    delayus(1);

    //定义发送和接收常用模式和Mifare卡通讯，CRC初始值0x6363
    rc522_WriteData(ModeReg, 0x3D);

    rc522_WriteData(TReloadRegL, 30);   //16位定时器低位
    rc522_WriteData(TReloadRegH, 0);    //16位定时器高位

    rc522_WriteData(TModeReg, 0x8D);    //定义内部定时器设置

    rc522_WriteData(TPrescalerReg, 0x3E);   //设置定时器分频系数

    rc522_WriteData(TxAutoReg, 0x40);   //调制发送信号为100%ASK
}

//设置RC522工作方式
//ucType: 工作方式
void M500PcdConfigISOType(unsigned char ucType)
{
    if(ucType == 'A')
    {
        ClearBitMask(Status2Reg, 0x08);
        rc522_WriteData(ModeReg, 0x3D);
        rc522_WriteData(RxSelReg, 0x86);
        rc522_WriteData(RFCfgReg, 0x7F);
        rc522_WriteData(TReloadRegL, 30);
        rc522_WriteData(TReloadRegH, 0);
        rc522_WriteData(TModeReg, 0x8D); 
        rc522_WriteData(TPrescalerReg, 0x3E);

        delayus(2);

        PcdAntennaOn(); //开天线
    }
}

/**
  * @brief  通过RC522和ISO14443卡通讯
  * @param  ucCommand，RC522命令字
  * @param  pInData，通过RC522发送到卡片的数据
  * @param  ucInLenByte，发送数据的字节长度
  * @param  pOutData，接收到的卡片返回数据
  * @param  pOutLenBit，返回数据的位长度
  * @retval 状态值= MI_OK，成功
  */
unsigned char PcdComMF522(unsigned char ucCommand,
                          unsigned char * pInData, 
                          unsigned char ucInLenByte, 
                          unsigned char * pOutData,
                          unsigned int  * pOutLenBit)
{
    char cStatus = MI_ERR;
    unsigned char ucIrqEn = 0x00;
    unsigned char ucWaitFor = 0x00;
    unsigned char ucLastBits;
    unsigned char ucN;
    unsigned int  ul;



    switch (ucCommand)
    {
        case PCD_AUTHENT:       //Mifare认证
            ucIrqEn   = 0x12;   //允许错误中断请求ErrIEn  允许空闲中断IdleIEn
            ucWaitFor = 0x10;   //认证寻卡等待时候 查询空闲中断标志位
            break;

        case PCD_TRANSCEIVE:    //接收发送 发送接收
            ucIrqEn =   0x77;   //允许TxIEn RxIEn IdleIEn LoAlertIEn ErrIEn TimerIEn
            ucWaitFor = 0x30;   //寻卡等待时候 查询接收中断标志位与 空闲中断标志位
            break;
        
        default:
            break;
    }
    //IRqInv置位管脚IRQ与Status1Reg的IRq位的值相反
    rc522_WriteData(ComIEnReg, ucIrqEn | 0x80);
    //Set1该位清零时，CommIRqReg的屏蔽位清零
    ClearBitMask(ComIrqReg, 0x80);
    //写空闲命令
    rc522_WriteData(CommandReg, PCD_IDLE);
    //置位FlushBuffer清除内部FIFO的读和写指针以及ErrReg的BufferOvfl标志位被清除
    SetBitMask (FIFOLevelReg, 0x80);

    for(ul = 0; ul < ucInLenByte; ul++)
    {
        rc522_WriteData(FIFODataReg, pInData[ul]);  //写数据进FIFODataReg
    }
    rc522_WriteData(CommandReg, ucCommand);         //写命令
    //StartSend置位启动数据发送 该位与收发命令使用时才有效
    if(ucCommand == PCD_TRANSCEIVE) SetBitMask(BitFramingReg, 0x80);
    //根据时钟频率调整，操作M1卡最大等待时间25ms
    ul = 1000;

    do
    {
        ucN = rc522_ReadData(ComIrqReg);    //查询事件中断
        ul --;
    } while (( ul != 0 ) && ( ! ( ucN & 0x01 ) ) && ( ! ( ucN & ucWaitFor ) ));//退出条件i=0,定时器中断，与写空闲命令
    
    ClearBitMask(BitFramingReg, 0x80);      //清理允许StartSend位

    if( ul != 0)
    {   

        //读错误标志寄存器BufferOfI CollErr ParityErr ProtocolErr
        if(!(rc522_ReadData(ErrorReg) & 0x1b))
        {
            cStatus = MI_OK;
            if(ucN & ucIrqEn & 0x01)    //是否发生定时器中断
            {
                cStatus = MI_NOTAGERR;
            }
            if(ucCommand == PCD_TRANSCEIVE)
            {
                //读FIFO中保存的字节数
                ucN = rc522_ReadData(FIFOLevelReg);
                //最后接收到的字节的有效位数
                ucLastBits = rc522_ReadData(ControlReg) & 0x07;

                if(ucLastBits)
                    * pOutLenBit = (ucN - 1) * 8 + ucLastBits;  //N个字节数减去1（最后一个字节）+最后一位的位数 读取到的数据总位数
                else
                    * pOutLenBit = ucN * 8;                     //最后接收到的字节整个字节有效
                
                if(ucN == 0)
                    ucN = 1;
                if(ucN > MAXRLEN)
                    ucN = MAXRLEN;
                for(ul = 0; ul < ucN; ul ++) pOutData[ul] = rc522_ReadData(FIFODataReg);
            }
        }
        else
            cStatus = MI_OK;
    }

    SetBitMask(ControlReg, 0x80);   //stop timer now
    rc522_WriteData(CommandReg, PCD_IDLE);
    


    return cStatus;
}

/**
  * @brief 寻卡
  * @param  ucReq_code，寻卡方式 = 0x52，寻感应区内所有符合14443A标准的卡；
            寻卡方式= 0x26，寻未进入休眠状态的卡
  * @param  pTagType，卡片类型代码
             = 0x4400，Mifare_UltraLight
             = 0x0400，Mifare_One(S50)
             = 0x0200，Mifare_One(S70)
             = 0x0800，Mifare_Pro(X))
             = 0x4403，Mifare_DESFire
  * @retval 状态值= MI_OK，成功
  */
unsigned char PcdRequest(unsigned char ucReq_code, unsigned char * pTagType)
{
    char cStatus;
    unsigned char ucComMF522Buf[MAXRLEN];
    unsigned int ulLen;

    //清理指示MIFARECyptol单元接通以及所有卡的数据通信被加密的情况
    ClearBitMask(Status2Reg, 0x08);
    //发送最后一个字节的7位
    rc522_WriteData(BitFramingReg, 0x07);
    //TX1,TX2管脚的输出信号传递经发送调制的13.56的能量载波信号
    SetBitMask (TxControlReg, 0x03);
    ucComMF522Buf [ 0 ] = ucReq_code;		//存入 卡片命令字

    cStatus = PcdComMF522 ( PCD_TRANSCEIVE,	
                            ucComMF522Buf,
                            1, 
                            ucComMF522Buf,
                            & ulLen );	//寻卡  

    if ( ( cStatus == MI_OK ) && ( ulLen == 0x10 ) )	//寻卡成功返回卡类型 
    {    
        * pTagType = ucComMF522Buf [ 0 ];
        * ( pTagType + 1 ) = ucComMF522Buf [ 1 ];
    }

    else
        cStatus = MI_ERR;

    return cStatus;
}

//防冲撞
//pSnr: 卡片序列号，4字节
//状态值 = MI_OK，成功
unsigned char PcdAnticoll(unsigned char * pSnr)
{
    char cStatus;
    unsigned char uc, ucSnr_check = 0;
    unsigned char ucComMF522Buf [ MAXRLEN ]; 
    unsigned int ulLen;

    //清MFCryptol On位 只有成功执行MFAuthent命令后，该位才能置位
    ClearBitMask (Status2Reg, 0x08);
    //清理寄存器 停止收发
    rc522_WriteData(BitFramingReg, 0x00);
    //清ValuesAfterColl所有接收的位在冲突后被清除
    ClearBitMask (CollReg, 0x80);
    
    ucComMF522Buf [ 0 ] = 0x93;	          //卡片防冲突命令
    ucComMF522Buf [ 1 ] = 0x20;

    cStatus = PcdComMF522 ( PCD_TRANSCEIVE, 
                            ucComMF522Buf,
                            2, 
                            ucComMF522Buf,
                            & ulLen);      //与卡片通信
    if ( cStatus == MI_OK)		            //通信成功
    {
        for ( uc = 0; uc < 4; uc ++ )
        {
            * ( pSnr + uc )  = ucComMF522Buf [ uc ]; //读出UID
            ucSnr_check ^= ucComMF522Buf [ uc ];
        }
    
        if ( ucSnr_check != ucComMF522Buf [ uc ] )
            cStatus = MI_ERR;    				 
    }
  
    SetBitMask (CollReg, 0x80);
      
    return cStatus;
}


/**
  * 用RC522计算CRC16
  * pIndata，计算CRC16的数组
  * ucLen，计算CRC16的数组字节长度
  * pOutData，存放计算结果存放的首地址
  * 无
  */
void CalulateCRC(unsigned char * pIndata, unsigned char ucLen, unsigned char * pOutData)
{
    unsigned char uc, ucN;

    ClearBitMask(DivIrqReg, 0x04);

    rc522_WriteData(CommandReg, PCD_IDLE);

    SetBitMask(FIFOLevelReg, 0x80);

    for(uc = 0; uc < ucLen; uc ++)
    {
        rc522_WriteData(FIFODataReg, * (pIndata + uc));
    }
    rc522_WriteData(CommandReg,PCD_CALCCRC);
    uc = 0xFF;

    do
    {
        ucN = rc522_ReadData(DivIrqReg);
        uc --;
    }while(( uc != 0) && !(ucN & 0x04));

    pOutData[0] = rc522_ReadData(CRCResultRegL);
    pOutData[1] = rc522_ReadData(CRCResultRegM);
}

/**
  * @brief  选定卡片
  * @param  pSnr，卡片序列号，4字节
  * @retval 状态值= MI_OK，成功
  */
unsigned char PcdSelect ( unsigned char * pSnr )
{
    char ucN;
    unsigned char uc;
    unsigned char ucComMF522Buf [ MAXRLEN ]; 
    unsigned int  ulLen;
  
  
    ucComMF522Buf [ 0 ] = PICC_ANTICOLL1;
    ucComMF522Buf [ 1 ] = 0x70;
    ucComMF522Buf [ 6 ] = 0;

    for ( uc = 0; uc < 4; uc ++ )
    {
        ucComMF522Buf [ uc + 2 ] = * ( pSnr + uc );
        ucComMF522Buf [ 6 ] ^= * ( pSnr + uc );
    }
  
    CalulateCRC ( ucComMF522Buf, 7, & ucComMF522Buf [ 7 ] );

    ClearBitMask ( Status2Reg, 0x08 );

    ucN = PcdComMF522 ( PCD_TRANSCEIVE,
                        ucComMF522Buf,
                        9,
                        ucComMF522Buf, 
                        & ulLen );
  
    if ( ( ucN == MI_OK ) && ( ulLen == 0x18 ) )
        ucN = MI_OK;  
    else
        ucN = MI_ERR;    
  
    return ucN;		
}

/**
  * @brief  验证卡片密码
  * @param  ucAuth_mode，密码验证模式= 0x60，验证A密钥，
            密码验证模式= 0x61，验证B密钥
  * @param  unsigned char ucAddr，块地址
  * @param  pKey，密码 
  * @param  pSnr，卡片序列号，4字节
  * @retval 状态值= MI_OK，成功
  */
unsigned char PcdAuthState ( unsigned char ucAuth_mode, unsigned char ucAddr, unsigned char * pKey, unsigned char * pSnr )
{
    char cStatus;
    unsigned char uc, ucComMF522Buf [ MAXRLEN ];
    unsigned int ulLen;
  

    ucComMF522Buf [ 0 ] = ucAuth_mode;
    ucComMF522Buf [ 1 ] = ucAddr;

    for ( uc = 0; uc < 6; uc ++ )
        ucComMF522Buf [ uc + 2 ] = * ( pKey + uc );   

    for ( uc = 0; uc < 6; uc ++ )
        ucComMF522Buf [ uc + 8 ] = * ( pSnr + uc );   

    cStatus = PcdComMF522 (PCD_AUTHENT,
                           ucComMF522Buf, 
                           12,
                           ucComMF522Buf,
                           & ulLen );

    if ( ( cStatus != MI_OK ) || ( ! ( rc522_ReadData ( Status2Reg ) & 0x08 ) ) )
        cStatus = MI_ERR;   
    
  return cStatus;
}

/**
  * @brief  写数据到M1卡一块
  * @param  unsigned char ucAddr，块地址
  * @param  pData，写入的数据，16字节
  * @retval 状态值= MI_OK，成功
  */
unsigned char PcdWrite ( unsigned char ucAddr, unsigned char * pData )
{
    char cStatus;
    unsigned char uc, ucComMF522Buf [ MAXRLEN ];
    unsigned int ulLen;
   
    ucComMF522Buf [ 0 ] = PICC_WRITE;
    ucComMF522Buf [ 1 ] = ucAddr;

    CalulateCRC ( ucComMF522Buf, 2, & ucComMF522Buf [ 2 ] );

    cStatus = PcdComMF522 ( PCD_TRANSCEIVE,
                            ucComMF522Buf,
                            4, 
                            ucComMF522Buf,
                            & ulLen );

    if ( ( cStatus != MI_OK ) || ( ulLen != 4 ) || ( ( ucComMF522Buf [ 0 ] & 0x0F ) != 0x0A ) )
        cStatus = MI_ERR;   
      
    if ( cStatus == MI_OK )
    {
        //memcpy(ucComMF522Buf, pData, 16);
        for ( uc = 0; uc < 16; uc ++ )
            ucComMF522Buf [ uc ] = * ( pData + uc );  
    
        CalulateCRC ( ucComMF522Buf, 16, & ucComMF522Buf [ 16 ] );

        cStatus = PcdComMF522 ( PCD_TRANSCEIVE,
                           ucComMF522Buf, 
                           18, 
                           ucComMF522Buf,
                           & ulLen );
    
        if ( ( cStatus != MI_OK ) || ( ulLen != 4 ) || ( ( ucComMF522Buf [ 0 ] & 0x0F ) != 0x0A ) )
            cStatus = MI_ERR;   
    
    } 	
    return cStatus;		
}

/**
  * @brief  读取M1卡一块数据
  * @param  ucAddr，块地址
  * @param  pData，读出的数据，16字节
  * @retval 状态值= MI_OK，成功
  */
unsigned char PcdRead ( unsigned char ucAddr, unsigned char * pData )
{
    char cStatus;
    unsigned char uc, ucComMF522Buf [ MAXRLEN ]; 
    unsigned int ulLen;
  
    ucComMF522Buf [ 0 ] = PICC_READ;
    ucComMF522Buf [ 1 ] = ucAddr;

    CalulateCRC ( ucComMF522Buf, 2, & ucComMF522Buf [ 2 ] );
 
    cStatus = PcdComMF522 ( PCD_TRANSCEIVE,
                            ucComMF522Buf,
                            4, 
                            ucComMF522Buf,
                            & ulLen );

    if ( ( cStatus == MI_OK ) && ( ulLen == 0x90 ) )
    {
        for ( uc = 0; uc < 16; uc ++ )
            * ( pData + uc ) = ucComMF522Buf [ uc ];   
    }
    else
        cStatus = MI_ERR;   
   
    return cStatus;		
}

/**
  * @brief  命令卡片进入休眠状态
  * @param  无
  * @retval 状态值= MI_OK，成功
  */
char PcdHalt( void )
{
	unsigned char ucComMF522Buf [ MAXRLEN ]; 
	unsigned int  ulLen;
  

    ucComMF522Buf [ 0 ] = PICC_HALT;
    ucComMF522Buf [ 1 ] = 0;
	
    CalulateCRC ( ucComMF522Buf, 2, & ucComMF522Buf [ 2 ] );
 	PcdComMF522 ( PCD_TRANSCEIVE,
                  ucComMF522Buf,
                  4, 
                  ucComMF522Buf, 
                  & ulLen );

    return MI_OK;	
}


unsigned char IC_CMT (unsigned char * UID, unsigned char * KEY, unsigned char RW, unsigned char addr, unsigned char * Dat )
{
    char cStatus;
    char cc;
    unsigned char ucArray_ID [ 4 ] = { 0 }; //先后存放IC卡的类型和UID(IC卡序列号)
  
	
    //PcdRequest ( 0x52, ucArray_ID ); //寻卡

    //PcdAnticoll ( ucArray_ID );      //防冲撞
  
    PcdSelect ( UID );               //选定卡
  
    cc = PcdAuthState ( 0x60, addr, KEY, UID );//校验
    if (cc == MI_OK)
	{
		printf ( "ok\r\n");
	}
	else
	{
		printf ( "error\r\n");
	}

    cc = PcdAuthState ( 0x61, addr, KEY, UID );//校验
    if (cc == MI_OK)
	{
		printf ( "ok\r\n");
	}
	else
	{
		printf ( "error\r\n");
	}
	

	if ( RW )                        //读写选择，1是读，0是写
    {
        cc = PcdRead ( addr, Dat );
        if(cc == MI_OK) cStatus = MI_OK;
        else cStatus = MI_ERR;
    }
    else 
    {
        cc = PcdWrite ( addr, Dat );
        if(cc == MI_OK) cStatus = MI_OK;
        else cStatus = MI_ERR;
    }
        
   	 
    PcdHalt ();	 
    return cStatus;
}

unsigned char IC_CMT_r (unsigned char * UID, unsigned char * KEY, unsigned char RW, unsigned char addr, unsigned char * Dat )
{
    char cStatus;
    char cc;
    int i;
    unsigned char ucArray_ID [ 4 ] = { 0 }; //先后存放IC卡的类型和UID(IC卡序列号)
  
	
    cc = PcdRequest ( 0x52, ucArray_ID ); //寻卡
    if (cc == MI_ERR)
    {
        cStatus = MI_ERR;
        return cStatus;
    }
    

    PcdAnticoll ( ucArray_ID );      //防冲撞

    for(i = 0; i < 4; i ++)
    {
        UID[i] = ucArray_ID[i];
    }
  
    PcdSelect ( UID );               //选定卡
  
    cc = PcdAuthState ( 0x60, addr, KEY, UID );//校验
    if (cc == MI_OK)
	{
		printf ( "ok\r\n");
	}
	else
	{
		printf ( "error\r\n");
	}

    cc = PcdAuthState ( 0x61, addr, KEY, UID );//校验
    if (cc == MI_OK)
	{
		printf ( "ok\r\n");
	}
	else
	{
		printf ( "error\r\n");
	}
	

	if ( RW )                        //读写选择，1是读，0是写
    {
        cc = PcdRead ( addr, Dat );
        if(cc == MI_OK) cStatus = MI_OK;
        else cStatus = MI_ERR;
    }
    else 
    {
        cc = PcdWrite ( addr, Dat );
        if(cc == MI_OK) cStatus = MI_OK;
        else cStatus = MI_ERR;
    }
        
   	 
    PcdHalt ();	 
    return cStatus;
}