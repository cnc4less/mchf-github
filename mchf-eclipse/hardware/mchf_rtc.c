/*  -*-  mode: c; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; coding: utf-8  -*-  */
/************************************************************************************
**                                                                                 **
**                               mcHF QRP Transceiver                              **
**                             K Atanassov - M0NKA 2014                            **
**                                                                                 **
**---------------------------------------------------------------------------------**
**                                                                                 **
**  Licence:		GNU GPLv3                                                      **
************************************************************************************/
#include "mchf_board.h"
#include "mchf_rtc.h"

#ifdef USE_RTC_LSE
/* Private macros */
/* Internal status registers for RTC */
#define RTC_PRESENCE_REG                   RTC_BKP_DR0
#define RTC_PRESENCE_INIT_VAL              0x0001       // if we find this value after power on, we assume battery is available
#define RTC_PRESENCE_OK_VAL                0x0002       // then we set this value to rembember a clock is present
//#define RTC_PRESENCE_ACK_VAL               0x0003       // if we find this value after power on, we assume user enabled RTC
//#define RTC_PRESENCE_NACK_VAL              0x0004       // if we find this value after power on, we assume user decided against using RTC

static void RTC_LSE_Config() {
    RCC_LSEConfig(RCC_LSE_ON);

    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WriteProtectionCmd(DISABLE);
    RTC_WaitForSynchro();
    RTC_WriteProtectionCmd(ENABLE);
    RTC_WriteBackupRegister(RTC_PRESENCE_REG, RTC_PRESENCE_OK_VAL);
}

void MchfRtc_FullReset() {
    RCC_BackupResetCmd(ENABLE);
}



#if 0
static void RTC_LSI_Config() {
    RCC_LSICmd(ENABLE);

    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    RCC_RTCCLKCmd(ENABLE);
    RTC_WriteProtectionCmd(DISABLE);
    RTC_WaitForSynchro();
    RTC_WriteProtectionCmd(ENABLE);
    RTC_WriteBackupRegister(RTC_STATUS_REG, RTC_STATUS_INIT_OK);
}
#endif

#endif

void MchfRtc_Start()
{
    // ok, there is a battery, so let us now start the oscillator
        RTC_LSE_Config();

        // very first start of rtc
        RTC_InitTypeDef rtc_init;

        rtc_init.RTC_HourFormat = RTC_HourFormat_24;
        // internal clock, 32kHz, see www.st.com/resource/en/application_note/dm00025071.pdf
        // rtc_init.RTC_AsynchPrediv = 123;   // 117; // 111; // 47; // 54; // 128 -1 = 127;
        // rtc_init.RTC_SynchPrediv =  234;   // 246; // 606; // 547; // 250 -1 = 249;
        // internal clock, 32kHz, see www.st.com/resource/en/application_note/dm00025071.pdf
         rtc_init.RTC_AsynchPrediv = 127; // 32768 = 128 * 256
         rtc_init.RTC_SynchPrediv =  255;

        RTC_Init(&rtc_init);
}

bool MchfRtc_enabled()
{
    bool retval = false;
#ifdef USE_RTC_LSE

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    PWR_BackupAccessCmd(ENABLE);

    uint32_t status = RTC_ReadBackupRegister(RTC_PRESENCE_REG);

    if (status == RTC_PRESENCE_OK_VAL) {
        RTC_ClearITPendingBit(RTC_IT_WUT);
        EXTI->PR = 0x00400000;
        retval = true;
        ts.vbat_present = true;
    } else if (status == 0) {
        // if we find the RTC_PRESENCE_INIT_VAL in the backup register next time we boot
        // we know there is a battery present.
        RTC_WriteBackupRegister(RTC_PRESENCE_REG,RTC_PRESENCE_INIT_VAL);
    } else {
        ts.vbat_present = true;
    }
#endif
    return retval;
}

bool MchfRtc_SetPpm(int16_t ppm)
{
    bool retval = false;
    if (ppm >= RTC_CALIB_PPM_MIN && ppm <= RTC_CALIB_PPM_MAX)
    {
        uint32_t calm;
        uint32_t calp;
        float64_t ppm2pulses = rint((float64_t)ppm * 1.048576); //  = (32 * 32768) / 1000.0000
        if (ppm2pulses <= 0.0) // important, we must make sure to not set calp if 0ppm
        {
            calm = - ppm2pulses;
            calp = 0;
        }
        else
        {
            calm = 512 - ppm2pulses;
            if (calm > 512)
            {
                calm = 0;
            }
            calp = 1;
        }
        RTC_SmoothCalibConfig(RTC_SmoothCalibPeriod_32sec,calp,calm);
        retval = true;
    }
    return retval;
}
