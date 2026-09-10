#ifndef ADICUP3029PORT_H
#define ADICUP3029PORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include the processor header file to declare pADI peripheral pointers */
#include "ADuCM3029.h"
#include "ad5940.h"

void AD5940_ReadWriteNBytes(unsigned char *pSendBuffer, unsigned char *pRecvBuff, unsigned long length);
void AD5940_CsClr(void);
void AD5940_CsSet(void);
void AD5940_RstSet(void);
void AD5940_RstClr(void);
void AD5940_Delay10us(uint32_t time);
uint32_t AD5940_GetMCUIntFlag(void);
uint32_t AD5940_ClrMCUIntFlag(void);
uint32_t AD5940_MCUResourceInit(void *pCfg);
void Ext_Int0_Handler(void);
uint32_t MCUPlatformInit(void *pCfg);
int UrtCfg(int iBaud);
int UART0_GetChar_Direct(void);
void ReadUART(char *buffer, int len);
bool ReadUART_NonBlocking(char *buffer, int len);

#endif /* ADICUP3029PORT_H */