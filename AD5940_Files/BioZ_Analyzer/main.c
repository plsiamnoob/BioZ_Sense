#include <stdint.h>
#include <stdbool.h>
#include "ADuCM3029.h"
#include "BodyImpedance.h"
#include "ADICUP3029Port.h"
#include "ad5940.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

/* Preserved from AD5940Main.c: Complete BIA Structure Initialization */
void AD5940BIAStructInit(void)
{
    AppBIACfg_Type *pBIACfg;
    
    AppBIAGetCfg(&pBIACfg);
    
    pBIACfg->SeqStartAddr = 0;
    pBIACfg->MaxSeqLen = 512;
    pBIACfg->RcalVal = 10000.0;
    pBIACfg->DftNum = DFTNUM_8192;
    pBIACfg->NumOfData = -1;
    pBIACfg->ReDoRtiaCal = bTRUE;
    pBIACfg->BiaODR = 20;
    pBIACfg->FifoThresh = 4;
    pBIACfg->ADCSinc3Osr = ADCSINC3OSR_2;
}

/* Merged Platform & AFE Configuration Routine */
static void LocalAD5940PlatformCfg(void)
{
    CLKCfg_Type clk_cfg;
    FIFOCfg_Type fifo_cfg;
    AGPIOCfg_Type gpio_cfg;

    /* Hardware reset & core initialization */
    AD5940_HWReset();
    AD5940_Initialize();

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

    /* Preserved from AD5940Main.c: Full AGPIO matrix configuration */
    gpio_cfg.FuncSet = GP6_SYNC | GP5_SYNC | GP4_SYNC | GP2_TRIG | GP1_SYNC | GP0_INT;
    gpio_cfg.InputEnSet = AGPIO_Pin2;
    gpio_cfg.OutputEnSet = AGPIO_Pin0 | AGPIO_Pin1 | AGPIO_Pin4 | AGPIO_Pin5 | AGPIO_Pin6;
    gpio_cfg.OutVal = 0;
    gpio_cfg.PullEnSet = 0;
    AD5940_AGPIOCfg(&gpio_cfg);

    /* Configure Interrupt Controllers */
    AD5940_INTCCfg(AFEINTC_1, AFEINTSRC_ALLINT, bTRUE);
    AD5940_INTCCfg(AFEINTC_0, AFEINTSRC_DATAFIFOTHRESH, bTRUE);
    AD5940_INTCClrFlag(AFEINTSRC_ALLINT);

    /* Preserved from AD5940Main.c: Unlock sleep key */
    AD5940_SleepKeyCtrlS(SLPKEY_UNLOCK);
}

int main(void)
{
    char input_buffer[64];

    /* Initialize MCU Platform & Peripherals */
    MCUPlatformInit(0);
    AD5940_MCUResourceInit(0);

    /* Initialize AD5940 AFE Hardware & BIA Defaults */
    LocalAD5940PlatformCfg();
    AD5940BIAStructInit(); 

    while (1)
    {
        fflush(stdout);

        /* Read incoming command line from UART */
        ReadUART(input_buffer, sizeof(input_buffer));
        input_buffer[strcspn(input_buffer, "\r\n")] = 0;

        float start = 0, stop = 0, step = 0;
        int pts = 0;

        // 1. Single / Constant Frequency Sweep: "SINGLE,FREQ,PTS"
        if (sscanf(input_buffer, "SINGLE,%f,%d", &start, &pts) == 2) 
        {
            printf("ACK SINGLE\n");
            PerformConstantSweep(start, pts);
        }
        // 2. Linear Sweep: "LIN,START,STOP,STEP,PTS"
        else if (sscanf(input_buffer, "LIN,%f,%f,%f,%d", &start, &stop, &step, &pts) == 4) 
        {
            printf("ACK LIN\n");
            printf("Startin\n");
            PerformLinearSweep(start, stop, pts);
        }
        // 3. Logarithmic Sweep: "LOG,START,STOP,STEP,PTS"
        else if (sscanf(input_buffer, "LOG,%f,%f,%f,%d", &start, &stop, &step, &pts) == 4) 
        {
            printf("ACK LOG\n");
            PerformLogSweep(start, stop, pts);
        }
        // 4. Standalone Abort Signal: "STOPSWEEP"
        else if (strcasecmp(input_buffer, "STOPSWEEP") == 0) 
        {
            AppBIACfg.StopRequired = bTRUE;
            printf("SWEEPSTOPPED\n");
        }
        // 5. Legacy Command support: "MEAS <Freq>"
        else if (sscanf(input_buffer, "MEAS %f", &start) == 1) 
        {
            printf("ACK SINGLE\n");
            PerformConstantSweep(start, 1);
        }
        // 6. Unknown Command Handling
        else if (strlen(input_buffer) > 0) 
        {
            printf("ERR UNKNOWN_COMMAND\n");
        }
    }
}
