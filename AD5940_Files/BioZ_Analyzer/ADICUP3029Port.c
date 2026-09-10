/*!
 *****************************************************************************
 @file:    ADICUP3029Port.c
 @author:  Neo Xu
 @brief:   The port for ADI's ADICUP3029 board.
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/

#include "ADICUP3029Port.h"
#define SYSTICK_MAXCOUNT ((1L<<24)-1) /* we use Systick to complete function Delay10uS(). This value only applies to ADICUP3029 board. */
#define SYSTICK_CLKFREQ   26000000L   /* Systick clock frequency in Hz. This only appies to ADICUP3029 board */
volatile static uint32_t ucInterrupted = 0;       /* Flag to indicate interrupt occurred */
/**
	@brief Using SPI to transmit N bytes and return the received bytes. This function targets to 
         provide a more efficient way to transmit/receive data.
	@param pSendBuffer :{0 - 0xFFFFFFFF}
      - Pointer to the data to be sent.
	@param pRecvBuff :{0 - 0xFFFFFFFF}
      - Pointer to the buffer used to store received data.
	@param length :{0 - 0xFFFFFFFF}
      - Data length in SendBuffer.
	@return None.
**/
void AD5940_ReadWriteNBytes(unsigned char *pSendBuffer,unsigned char *pRecvBuff,unsigned long length)
{
  uint32_t tx_count=0, rx_count=0;
  pADI_SPI0->CNT = length;
  while(1){
    uint32_t fifo_sta = pADI_SPI0->FIFO_STAT;
    if(rx_count < length){
      if(fifo_sta&0xf00){//there is data in RX FIFO.
        *pRecvBuff++ = pADI_SPI0->RX;
        rx_count ++;
      }
    }
    if(tx_count < length){
      if((fifo_sta&0xf) < 8){// there is space in TX FIFO.
        pADI_SPI0->TX = *pSendBuffer++;
        tx_count ++;
      }
    }
    if(rx_count == length && tx_count==length)
      break;  //done
  }
  while((pADI_SPI0->STAT&BITM_SPI_STAT_XFRDONE) == 0);//wait for transfer done.
}

void AD5940_CsClr(void)
{
   pADI_GPIO1->CLR = (1<<10);
}

void AD5940_CsSet(void)
{
   pADI_GPIO1->SET = (1<<10);
}

void AD5940_RstSet(void)
{
   pADI_GPIO2->SET = 1<<6; //p2.6-ADC3-A3
}

void AD5940_RstClr(void)
{
   pADI_GPIO2->CLR = 1<<6; //p2.6-ADC3-A3
}

void AD5940_Delay10us(uint32_t time)
{
  if(time==0)return;
  if(time*10<SYSTICK_MAXCOUNT/(SYSTICK_CLKFREQ/1000000)){
    SysTick->LOAD = time*10*(SYSTICK_CLKFREQ/1000000);
    SysTick->CTRL = (1 << 2) | (1<<0);    /* Enable SysTick Timer, using core clock */
    while(!((SysTick->CTRL)&(1<<16)));    /* Wait until count to zero */
    SysTick->CTRL = 0;                    /* Disable SysTick Timer */
  }
  else {
    AD5940_Delay10us(time/2);
    AD5940_Delay10us(time/2 + (time&1));
  }
}

uint32_t AD5940_GetMCUIntFlag(void)
{
   return ucInterrupted;
}

uint32_t AD5940_ClrMCUIntFlag(void)
{
   pADI_XINT0->CLR = BITM_XINT_CLR_IRQ0;
   ucInterrupted = 0;
   return 1;
}

/* Functions that used to initialize MCU platform */

uint32_t AD5940_MCUResourceInit(void *pCfg)
{
  /* Step1, initialize SPI peripheral and its GPIOs for CS/RST */
  pADI_GPIO0->PE = 0xFFFF;
  pADI_GPIO1->PE = 0xFFFF;
  pADI_GPIO2->PE = 0xFFFF;
  pADI_GPIO2->OEN |= (1<<6); //P2.6-ADC3-A3-AD5940_Reset
  pADI_GPIO2->SET = 1<<6; //Pull high this pin.

  /*Setup Pins P0.0-->SCLK P0.1-->MOSI P0.2-->MISO P1.10-->CS*/
  pADI_GPIO0->CFG = (1<<0)|(1<<2)|(1<<4)|(pADI_GPIO0->CFG&(~((3<<0)|(3<<2)|(3<<4))));
  pADI_GPIO1->CFG &=~(3<<14); /* Configure P1.10 to GPIO function */
  pADI_GPIO1->OEN |= (1<<10); /* P1.10 Output Enable */
  /*Set SPI Baudrate = PCLK/2x(iCLKDiv+1).*/
  pADI_SPI0->DIV = 0;/*Baudrae is 13MHz*/
  pADI_SPI0->CTL = BITM_SPI_CTL_CSRST|        // Configure SPI to reset after a bit shift error is detected
      BITM_SPI_CTL_MASEN|                   // Enable master mode
      /*BITM_SPI_CTL_CON|*/                     // Enable continous transfer mode
         BITM_SPI_CTL_OEN|                     // Select MISO pin to operate as normal -
            BITM_SPI_CTL_RXOF|                    // overwrite data in Rx FIFO during overflow states
               /*BITM_SPI_CTL_ZEN|*/                     // transmit 00 when no valid data in Tx FIFO
                  BITM_SPI_CTL_TIM|                     // initiate trasnfer with a write to SPITX
                     BITM_SPI_CTL_SPIEN;                  // Enable SPI. SCLK idles low/ data clocked on SCLK falling edge
  pADI_SPI0->CNT = 1;// Setup to transfer 1 bytes to slave
  /* Step2: initialize GPIO interrupt that connects to AD5940's interrupt output pin(Gp0, Gp3, Gp4, Gp6 or Gp7 ) */
  pADI_GPIO0->IEN |= 1<<15;// Configure P0.15 as an input

  pADI_XINT0->CFG0 = (0x1<<0)|(1<<3);//External IRQ0 enabled. Falling edge
  pADI_XINT0->CLR = BITM_XINT_CLR_IRQ0;
  NVIC_EnableIRQ(XINT_EVT0_IRQn);		  //Enable External Interrupt 0 source.
  
  AD5940_CsSet();
  AD5940_RstSet();
  return 0;
}

/* MCU related external line interrupt service routine */
void Ext_Int0_Handler()
{
   pADI_XINT0->CLR = BITM_XINT_CLR_IRQ0;
   ucInterrupted = 1;
  /* This example just set the flag and deal with interrupt in AD5940Main function. It's your choice to choose how to process interrupt. */
}


/* Platform Clock & Watchdog Initialization */
uint32_t MCUPlatformInit(void *pCfg)
{
    int UrtCfg(int iBaud);

    /* Stop watchdog timer */
    pADI_WDT0->CTL = 0xC9;

    /* Configure System Clock */
    pADI_CLKG0_OSC->KEY = 0xCB14;
    pADI_CLKG0_OSC->CTL = BITM_CLKG_OSC_CTL_HFOSCEN | BITM_CLKG_OSC_CTL_HFXTALEN;

    /* Timeout safeguard on XTAL stabilization */
    volatile uint32_t timeout = 100000;
    while (((pADI_CLKG0_OSC->CTL & BITM_CLKG_OSC_CTL_HFXTALOK) == 0) && (--timeout > 0))
        ;

    pADI_CLKG0_OSC->KEY = 0xCB14;
    pADI_CLKG0_CLK->CTL0 = 0x201; /* Select XTAL as system clock */
    pADI_CLKG0_CLK->CTL1 = 0;     /* Clocks divided by 1 */
    pADI_CLKG0_CLK->CTL5 = 0x00;  /* Enable peripheral clocks */

    UrtCfg(230400); /* UART Baudrate = 230400 */
    return 1;
}

int UrtCfg(int iBaud)
{
    int iBits = 3;
    int iFormat = 0;
    int i1, iDiv, iRtC, iOSR, iPllMulValue;
    unsigned long long ullRtClk = 16000000;

    /* Setup P0[11:10] as UART pins */
    pADI_GPIO0->CFG = (1 << 22) | (1 << 20) | (pADI_GPIO0->CFG & (~((3 << 22) | (3 << 20))));

    iDiv = (pADI_CLKG0_CLK->CTL1 & BITM_CLKG_CLK_CTL1_PCLKDIVCNT) >> 8;
    if (iDiv == 0) iDiv = 1;
    iRtC = (pADI_CLKG0_CLK->CTL0 & BITM_CLKG_CLK_CTL0_CLKMUX);

    switch (iRtC)
    {
    case 0: ullRtClk = 26000000; break;
    case 1:
        if ((pADI_CLKG0_CLK->CTL0 & 0x200) == 0x200)
            ullRtClk = 26000000;
        else
            ullRtClk = 16000000;
        break;
    case 2:
        iPllMulValue = (pADI_CLKG0_CLK->CTL3 & BITM_CLKG_CLK_CTL3_SPLLNSEL);
        ullRtClk = (iPllMulValue * 1000000);
        break;
    case 3: ullRtClk = 26000000; break;
    default: break;
    }

    pADI_UART0->COMLCR2 = 0x3;
    iOSR = 32;
    i1 = (ullRtClk / (iOSR * iDiv)) / iBaud - 1;
    pADI_UART0->COMDIV = i1;

    pADI_UART0->COMFBR = 0x8800 | (((((2048 / (iOSR * iDiv)) * ullRtClk) / i1) / iBaud) - 2048);
    pADI_UART0->COMIEN = 0;
    pADI_UART0->COMLCR = (iFormat & 0x3c) | (iBits & 3);

    pADI_UART0->COMFCR = (BITM_UART_COMFCR_RFTRIG & 0) | BITM_UART_COMFCR_FIFOEN;
    pADI_UART0->COMFCR |= BITM_UART_COMFCR_RFCLR | BITM_UART_COMFCR_TFCLR;
    pADI_UART0->COMFCR &= ~(BITM_UART_COMFCR_RFCLR | BITM_UART_COMFCR_TFCLR);

    NVIC_EnableIRQ(UART_EVT_IRQn);
    pADI_UART0->COMIEN = BITM_UART_COMIEN_ERBFI | BITM_UART_COMIEN_ELSI;
    return pADI_UART0->COMLSR;
}

int UART0_GetChar_Direct(void)
{
    if (pADI_UART0->COMLSR & (1 << 0))
        return (int)(pADI_UART0->COMRX);
    return -1;
}

void ReadUART(char *buffer, int len)
{
    int i = 0;
    pADI_UART0->COMIEN &= ~(1 << 0);
    while (i < len - 1)
    {
        int tempchar = UART0_GetChar_Direct();
        if (tempchar == -1)
            continue;
        if (tempchar == '\r' || tempchar == '\n')
        {
            if (i > 0)
                break;
            continue;
        }
        buffer[i++] = (char)tempchar;
        fflush(stdout);
    }
    buffer[i] = '\0';
    fflush(stdout);
}
bool ReadUART_NonBlocking(char *buffer, int len)
{
    static int idx = 0;
    int tempchar = UART0_GetChar_Direct();

    if (tempchar == -1) {
        return false;
    }

    if (tempchar == '\r' || tempchar == '\n') {
        if (idx > 0) {
            buffer[idx] = '\0';
            idx = 0;
            return true;
        }
        return false;
    }

    if (idx < len - 1) {
        buffer[idx++] = (char)tempchar;
    }
    
    return false;
}
#if defined(__GNUC__)
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (ptr[i] == '\n')
        {
            pADI_UART0->COMTX = '\r';
            while ((pADI_UART0->COMLSR & 0x20) == 0)
                ;
        }
        pADI_UART0->COMTX = ptr[i];
        while ((pADI_UART0->COMLSR & 0x20) == 0)
            ;
    }
    return len;
}

int _close(int file) { (void)file; return -1; }
int _fstat(int file, void *st) { (void)file; (void)st; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _read(int file, char *ptr, int len) { (void)file; (void)ptr; (void)len; return 0; }
#endif