#ifndef __RC522_FUNCTION_H
#define __RC522_FUNCTION_H

#include "imx6ul.h"

#define macDummy_Data 0x00

void rc522_WriteData(unsigned char reg, unsigned char value);
unsigned char rc522_ReadData(unsigned char reg);
void SetBitMask(unsigned char ucReg, unsigned char ucMask);
void ClearBirMask(unsigned char ucReg, unsigned char ucMask);
void PcdAntennaOn(void);
void PcdAntennaOff(void);
void PcdReset(void);
unsigned char PcdComMF522(uint8_t ucCommand,
                          uint8_t * pInData, 
                          uint8_t ucInLenByte, 
                          uint8_t * pOutData,
                          uint32_t * pOutLenBit);
unsigned char PcdRequest(uint8_t ucReq_code, uint8_t * pTagType);
unsigned char PcdAnticoll(uint8_t * pSnr);
void CalulateCRC(uint8_t * pIndata, uint8_t ucLen, uint8_t * pOutData);
unsigned char PcdSelect ( uint8_t * pSnr );
unsigned char PcdAuthState ( uint8_t ucAuth_mode, uint8_t ucAddr, uint8_t * pKey, uint8_t * pSnr );
unsigned char PcdWrite ( uint8_t ucAddr, uint8_t * pData );
unsigned char PcdRead ( uint8_t ucAddr, uint8_t * pData );
unsigned char IC_CMT (unsigned char * UID, 
             unsigned char * KEY, 
             unsigned char RW, 
             unsigned char addr, 
             unsigned char * Dat );
unsigned char IC_CMT_r (unsigned char * UID, 
                        unsigned char * KEY, 
                        unsigned char RW, 
                        unsigned char addr, 
                        unsigned char * Dat );
#endif // !__RC522_FUNCTION_H
