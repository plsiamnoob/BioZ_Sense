/*
Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.
*/

#include <stdio.h>
#include <stdint.h>
#include "ADuCM3029.h"
#include "ad5940.h"
#include "BodyImpedance.h"

/* External result printer provided by AD5940Main.c */
extern int32_t BIAShowResult(uint32_t *pData, uint32_t DataCount);

/* Function Prototypes */
uint32_t MCUPlatformInit(void *pCfg);
uint32_t AD5940_MCUResourceInit(void *pCfg);
uint32_t AD5940_GetMCUIntFlag(void);
uint32_t AD5940_ClrMCUIntFlag(void);
void AD5940_Main_Routine(void);
static void LocalAD5940PlatformCfg(void);
static void LocalAD5940BIAStructInit(void);

static void LocalAD5940PlatformCfg(void)
{
  CLKCfg_Type clk_cfg;
  FIFOCfg_Type fifo_cfg;
  AGPIOCfg_Type gpio_cfg;

  /* Disable all AFE control blocks initially */
  AD5940_AFECtrlS(AFECTRL_ALL, bFALSE);

  /* Configure AFE Clocks */
  clk_cfg.ADCClkDiv = ADCCLKDIV_1;
  clk_cfg.ADCCLkSrc = ADCCLKSRC_HFOSC;
  clk_cfg.SysClkDiv = SYSCLKDIV_1;
  clk_cfg.SysClkSrc = SYSCLKSRC_HFOSC;
  clk_cfg.HfOSC32MHzMode = bFALSE;
  clk_cfg.HFOSCEn = bTRUE;
  clk_cfg.HFXTALEn = bFALSE;
  clk_cfg.LFOSCEn = bTRUE;
  AD5940_CLKCfg(&clk_cfg);

  /* Configure Data FIFO */
  fifo_cfg.FIFOEn = bFALSE;
  fifo_cfg.FIFOMode = FIFOMODE_FIFO;
  fifo_cfg.FIFOSize = FIFOSIZE_4KB;
  fifo_cfg.FIFOSrc = FIFOSRC_DFT;
  fifo_cfg.FIFOThresh = 4;
  AD5940_FIFOCfg(&fifo_cfg);
  fifo_cfg.FIFOEn = bTRUE;
  AD5940_FIFOCfg(&fifo_cfg);

  /* Configure Analog GPIO Pins */
  gpio_cfg.FuncSet = GP0_INT;
  gpio_cfg.InputEnSet = 0;
  gpio_cfg.OutputEnSet = 0;
  gpio_cfg.OutVal = 0;
  gpio_cfg.PullEnSet = 0;
  AD5940_AGPIOCfg(&gpio_cfg);

  /* Configure Interrupt Controller */
  /* Enable ALL interrupts on INTC1 so internal polling loops (like ENDSEQ in AppBIAInit) work */
  AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);
  /* Route FIFO threshold interrupt to INTC0 (drives hardware GP0 pin to MCU) */
  AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);
  AD5940_INTCClrFlag(AFEINTSRC_ALLINT);
}

static void LocalAD5940BIAStructInit(void)
{
  AppBIACfg_Type *pBIACfg;
  AppBIAGetCfg(&pBIACfg);

  pBIACfg->SeqStartAddr = 0;
  pBIACfg->MaxSeqLen = 512;
  pBIACfg->RcalVal = 10000.0f; /* 10kOhm RCAL */
  pBIACfg->SinFreq = 50000.0f; /* 50kHz excitation frequency */
  pBIACfg->FifoThresh = 4;
  pBIACfg->BiaODR = 20.0f;
  pBIACfg->NumOfData = -1;
  pBIACfg->DftNum = DFTNUM_8192;
  pBIACfg->DftSrc = DFTSRC_SINC3;
  pBIACfg->HanWinEn = bTRUE;
  pBIACfg->SweepCfg.SweepEn = bTRUE;
}

int main(void)
{
  MCUPlatformInit(0);

  AD5940_MCUResourceInit(0);
  AD5940_Main_Routine();

  while (1)
    ;
}

void AD5940_Main_Routine(void)
{
  static uint32_t AppBuff[512];
  uint32_t temp = 512;

  AD5940_HWReset();
  AD5940_Initialize();

  LocalAD5940PlatformCfg();
  LocalAD5940BIAStructInit();

  AppBIAInit(AppBuff, 512);
  AppBIACtrl(BIACTRL_START, 0);

  volatile uint32_t heartbeat = 0;
  while (1)
  {
    /* Check if the AD5940 pulled the GP0 interrupt pin HIGH */
    if (AD5940_GetMCUIntFlag())
    {
      AD5940_ClrMCUIntFlag();
      temp = 512;
      AppBIAISR(AppBuff, &temp);
      if (temp > 0)
      {
        BIAShowResult(AppBuff, temp);
      }
      heartbeat = 0;
    }
    else
    {
      if (++heartbeat > 3000000)
      {
        heartbeat = 0;
      }
    }
  }
}

/* Platform Initialization Routine */
uint32_t MCUPlatformInit(void *pCfg)
{
  int UrtCfg(int iBaud);

  /* Stop watchdog timer */
  pADI_WDT0->CTL = 0xC9;

  /* Clock Configure */
  pADI_CLKG0_OSC->KEY = 0xCB14;
  pADI_CLKG0_OSC->CTL = BITM_CLKG_OSC_CTL_HFOSCEN | BITM_CLKG_OSC_CTL_HFXTALEN;

  /* Safeguard: Timeout on crystal stabilization */
  volatile uint32_t timeout = 100000;
  while (((pADI_CLKG0_OSC->CTL & BITM_CLKG_OSC_CTL_HFXTALOK) == 0) && (--timeout > 0))
    ;

  pADI_CLKG0_OSC->KEY = 0xCB14;
  pADI_CLKG0_CLK->CTL0 = 0x201; /* Select XTAL as system clock */
  pADI_CLKG0_CLK->CTL1 = 0;     /* ACLK, PCLK, HCLK divided by 1 */
  pADI_CLKG0_CLK->CTL5 = 0x00;  /* Enable clock to all peripherals */

  UrtCfg(230400); /* Set baud rate to 230400 */
  return 1;
}

int UrtCfg(int iBaud)
{
  int iBits = 3;
  int iFormat = 0;
  int i1;
  int iDiv;
  int iRtC;
  int iOSR;
  int iPllMulValue;
  unsigned long long ullRtClk = 16000000;

  /* Setup P0[11:10] as UART pins */
  pADI_GPIO0->CFG = (1 << 22) | (1 << 20) | (pADI_GPIO0->CFG & (~((3 << 22) | (3 << 20))));

  iDiv = (pADI_CLKG0_CLK->CTL1 & BITM_CLKG_CLK_CTL1_PCLKDIVCNT) >> 8;
  if (iDiv == 0)
    iDiv = 1;
  iRtC = (pADI_CLKG0_CLK->CTL0 & BITM_CLKG_CLK_CTL0_CLKMUX);

  switch (iRtC)
  {
  case 0:
    ullRtClk = 26000000;
    break;
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
  case 3:
    ullRtClk = 26000000;
    break;
  default:
    break;
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

int _close(int file)
{
  (void)file;
  return -1;
}
int _fstat(int file, void *st)
{
  (void)file;
  (void)st;
  return 0;
}
int _isatty(int file)
{
  (void)file;
  return 1;
}
int _lseek(int file, int ptr, int dir)
{
  (void)file;
  (void)ptr;
  (void)dir;
  return 0;
}
int _read(int file, char *ptr, int len)
{
  (void)file;
  (void)ptr;
  (void)len;
  return 0;
}
#endif