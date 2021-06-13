/*!
    \file  gd32f1x0_pmu.c
    \brief PMU driver

    \version 2014-12-26, V1.0.0, platform GD32F1x0(x=3,5)
    \version 2016-01-15, V2.0.0, platform GD32F1x0(x=3,5,7,9)
    \version 2016-04-30, V3.0.0, firmware update for GD32F1x0(x=3,5,7,9)
    \version 2017-06-19, V3.1.0, firmware update for GD32F1x0(x=3,5,7,9)
    \version 2019-11-20, V3.2.0, firmware update for GD32F1x0(x=3,5,7,9)
*/

/*
    Copyright (c) 2019, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f1x0_pmu.h"

/*!
    \brief      reset PMU register
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_deinit(void)
{
    /* reset PMU */
    rcu_periph_reset_enable(RCU_PMURST);
    rcu_periph_reset_disable(RCU_PMURST);
}

/*!
    \brief      select low voltage detector threshold
    \param[in]  lvdt_n:
                only one parameter can be selected which is shown as below:
      \arg        PMU_LVDT_0: voltage threshold is 2.2V (GD32F130_150) or 2.4V (GD32F170_190)
      \arg        PMU_LVDT_1: voltage threshold is 2.3V (GD32F130_150) or 2.7V (GD32F170_190)
      \arg        PMU_LVDT_2: voltage threshold is 2.4V (GD32F130_150) or 3.0V (GD32F170_190)
      \arg        PMU_LVDT_3: voltage threshold is 2.5V (GD32F130_150) or 3.3V (GD32F170_190)
      \arg        PMU_LVDT_4: voltage threshold is 2.6V (GD32F130_150) or 3.6V (GD32F170_190)
      \arg        PMU_LVDT_5: voltage threshold is 2.7V (GD32F130_150) or 3.9V (GD32F170_190)
      \arg        PMU_LVDT_6: voltage threshold is 2.8V (GD32F130_150) or 4.2V (GD32F170_190)
      \arg        PMU_LVDT_7: voltage threshold is 2.9V (GD32F130_150) or 4.5V (GD32F170_190)
    \param[out] none
    \retval     none
*/
void pmu_lvd_select(uint32_t lvdt_n)
{
    /* disable LVD */
    PMU_CTL &= ~PMU_CTL_LVDEN;
    /* clear LVDT bits */
    PMU_CTL &= ~PMU_CTL_LVDT;
    /* set LVDT bits according to lvdt_n */
    PMU_CTL |= lvdt_n;
    /* enable LVD */
    PMU_CTL |= PMU_CTL_LVDEN;

}

/*!
    \brief      disable PMU lvd
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_lvd_disable(void)
{
    /* disable LVD */
    PMU_CTL &= ~PMU_CTL_LVDEN;
}

/*!
    \brief      PMU work at sleep mode
    \param[in]  sleepmodecmd:
                only one parameter can be selected which is shown as below:
      \arg        WFI_CMD:  use WFI command
      \arg        WFE_CMD:  use WFE command
    \param[out] none
    \retval     none
*/
void pmu_to_sleepmode(uint8_t sleepmodecmd)
{
    /* clear sleepdeep bit of Cortex-M3 system control register */
    SCB->SCR &= ~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);
    
    /* select WFI or WFE command to enter sleep mode */
    if(WFI_CMD == sleepmodecmd){
        __WFI();
    }else{
        __WFE();
    }
}

/*!
    \brief      PMU work at deepsleep mode
    \param[in]  ldo:
                only one parameter can be selected which is shown as below:
      \arg        PMU_LDO_NORMAL: LDO operates normally when pmu enter deepsleep mode
      \arg        PMU_LDO_LOWPOWER: LDO work at low power mode when pmu enter deepsleep mode
    \param[in]  deepsleepmodecmd:
                only one parameter can be selected which is shown as below:
      \arg        WFI_CMD: use WFI command
      \arg        WFE_CMD: use WFE command
    \param[out] none
    \retval     none
*/
void pmu_to_deepsleepmode(uint32_t ldo,uint8_t deepsleepmodecmd)
{
    static uint32_t reg_snap[ 4 ];
    /* clear stbmod and ldolp bits */
    PMU_CTL &= ~((uint32_t)(PMU_CTL_STBMOD | PMU_CTL_LDOLP));
    
    /* set ldolp bit according to pmu_ldo */
    PMU_CTL |= ldo;
    
    /* set sleepdeep bit of Cortex-M3 system control register */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    
    reg_snap[ 0 ] = REG32( 0xE000E010U );
    reg_snap[ 1 ] = REG32( 0xE000E100U );
    reg_snap[ 2 ] = REG32( 0xE000E104U );
    reg_snap[ 3 ] = REG32( 0xE000E108U );
    
    REG32( 0xE000E010U ) &= 0x00010004U;
    REG32( 0xE000E180U )  = 0XB7FFEF19U;
    REG32( 0xE000E184U )  = 0XFFFFFBFFU;
    REG32( 0xE000E188U )  = 0xFFFFFFFFU;
    
     /* select WFI or WFE command to enter deepsleep mode */
    if(WFI_CMD == deepsleepmodecmd){
        __WFI();
    }else{
        __SEV();
        __WFE();
        __WFE();
    }
    
    REG32( 0xE000E010U ) = reg_snap[ 0 ] ; 
    REG32( 0xE000E100U ) = reg_snap[ 1 ] ;
    REG32( 0xE000E104U ) = reg_snap[ 2 ] ;
    REG32( 0xE000E108U ) = reg_snap[ 3 ] ;   
    
    /* reset sleepdeep bit of Cortex-M3 system control register */
    SCB->SCR &= ~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);
}

/*!
    \brief      pmu work at standby mode
    \param[in]  standbymodecmd:
                only one parameter can be selected which is shown as below:
      \arg        WFI_CMD: use WFI command
      \arg        WFE_CMD: use WFE command
    \param[out] none
    \retval     none
*/
void pmu_to_standbymode(uint8_t standbymodecmd)
{
    /* set sleepdeep bit of Cortex-M3 system control register */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* set stbmod bit */
    PMU_CTL |= PMU_CTL_STBMOD;
        
    /* reset wakeup flag */
    PMU_CTL |= PMU_CTL_WURST;
    
    /* select WFI or WFE command to enter standby mode */
    if(WFI_CMD == standbymodecmd){
        __WFI();
    }else{
        __WFE();
    }
}

/*!
    \brief      enable wakeup pin
    \param[in]  wakeup_pin:
                one or more parameters can be selected which are shown as below:
      \arg        PMU_WAKEUP_PIN0: wakeup pin 0
      \arg        PMU_WAKEUP_PIN1: wakeup pin 1
    \param[out] none
    \retval     none
*/
void pmu_wakeup_pin_enable(uint32_t wakeup_pin )
{
    PMU_CS |= wakeup_pin;
}

/*!
    \brief      disable wakeup pin
    \param[in]  wakeup_pin:
                one or more parameters can be selected which are shown as below:
      \arg        PMU_WAKEUP_PIN0: wakeup pin 0
      \arg        PMU_WAKEUP_PIN1: wakeup pin 1
    \param[out] none
    \retval     none
*/
void pmu_wakeup_pin_disable(uint32_t wakeup_pin )
{
    PMU_CS &= ~(wakeup_pin);

}

/*!
    \brief      enable backup domain write
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_backup_write_enable(void)
{
    PMU_CTL |= PMU_CTL_BKPWEN;
}

/*!
    \brief      disable backup domain write
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_backup_write_disable(void)
{
    PMU_CTL &= ~PMU_CTL_BKPWEN;
}

/*!
    \brief      clear flag bit
    \param[in]  flag_clear:
                one or more parameters can be selected which are shown as below:
      \arg        PMU_FLAG_RESET_WAKEUP: reset wakeup flag
      \arg        PMU_FLAG_RESET_STANDBY: reset standby flag
    \param[out] none
    \retval     none
*/
void pmu_flag_clear(uint32_t flag_clear)
{
    switch(flag_clear){
    case PMU_FLAG_RESET_WAKEUP:
        /* reset wakeup flag */
        PMU_CTL |= PMU_CTL_WURST;
        break;
    case PMU_FLAG_RESET_STANDBY:
        /* reset standby flag */
        PMU_CTL |= PMU_CTL_STBRST;
        break;
    default :
        break;
    }
}

/*!
    \brief      get flag state
    \param[in]  flag:         
                only one parameter can be selected which is shown as below:
      \arg        PMU_FLAG_WAKEUP: wakeup flag 
      \arg        PMU_FLAG_STANDBY: standby flag 
      \arg        PMU_FLAG_LVD: lvd flag 
    \param[out] none
    \retval     FlagStatus SET or RESET
*/
FlagStatus pmu_flag_get(uint32_t flag )
{
    if(PMU_CS & flag){
        return  SET;
    }else{
        return  RESET;
    }
}
