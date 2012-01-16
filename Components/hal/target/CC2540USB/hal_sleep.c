/**************************************************************************************************
  Filename:       hal_sleep.c
  Revised:        $Date: 2011-07-06 15:23:44 -0700 (Wed, 06 Jul 2011) $
  Revision:       $Revision: 26622 $

  Description:    This module contains the HAL power management procedures for the CC2540.


  Copyright 2006-2010 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "hal_types.h"
#include "hal_mcu.h"
#include "hal_board.h"
#include "hal_sleep.h"
#include "hal_led.h"
#include "hal_key.h"
#include "OSAL.h"
#include "OSAL_Timers.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"
#include "hal_drivers.h"
#include "hal_assert.h"
#include "ll_sleep.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/* POWER CONSERVATION DEFINITIONS
 * Sleep mode H/W definitions (enabled with POWER_SAVING compile option)
 */
#define CC2540_PM0            0  /* PM0, Clock oscillators on, voltage regulator on */
#define CC2540_PM1            1  /* PM1, 32.768 kHz oscillators on, voltage regulator on */
#define CC2540_PM2            2  /* PM2, 32.768 kHz oscillators on, voltage regulator off */
#define CC2540_PM3            3  /* PM3, All clock oscillators off, voltage regulator off */

/* HAL power management mode is set according to the power management state. The default
 * setting is HAL_SLEEP_OFF. The actual value is tailored to different HW platform. Both
 * HAL_SLEEP_TIMER and HAL_SLEEP_DEEP selections will:
 *   1. turn off the system clock, and
 *   2. halt the MCU.
 * HAL_SLEEP_TIMER can be woken up by sleep timer interrupt, I/O interrupt and reset.
 * HAL_SLEEP_DEEP can be woken up by I/O interrupt and reset.
 */
#define HAL_SLEEP_OFF         CC2540_PM0
#define HAL_SLEEP_TIMER       CC2540_PM2
#define HAL_SLEEP_DEEP        CC2540_PM3

/* MAX_SLEEP_TIME calculation:
 *   Sleep timer maximum duration = 0xFFFF7F / 32768 Hz = 511.996 seconds
 *   Round it to 510 seconds or 510000 ms
 */
#define MAX_SLEEP_TIME                    16711680            /* max time to sleep allowed by ST, in 32kHz ticks */


/* minimum time to sleep (4ms), this macro is to:
 * 1. avoid thrashing in-and-out of sleep with short OSAL timer (~2ms)
 * 2. define minimum safe sleep period
 */
#if !defined (PM_MIN_SLEEP_TIME)
#define PM_MIN_SLEEP_TIME                 131                 /* default to min safe sleep time, in 32kHz ticks */
#endif

/* The PCON instruction must be 4-byte aligned. The following code may cause excessive power
 * consumption if not aligned. See linker file ".xcl" for actual placement.
 */
#pragma location = "SLEEP_CODE"
void halSetSleepMode(void);

/* This value is used to adjust the sleep timer compare value such that the sleep timer
 * compare takes into account the amount of processing time spent in function halSleep().
 * The first value is determined by measuring the number of sleep timer ticks from
 * the beginning of the function to entering sleep mode.  The second value is determined
 * by measuring the number of sleep timer ticks from exit of sleep mode to the call to
 * osal_adjust_timers().
 */
#define HAL_SLEEP_ADJ_TICKS                 35                /* ~1ms, in 32kHz ticks */

#if defined(DEVICE_TYPE) && (DEVICE_TYPE & LL_DEV_TYPE_MASTER)
#define LL_WAKE_ADJUSTMENT                  1
#else // LL_DEV_TYPE_SLAVE
#define LL_WAKE_ADJUSTMENT                  10
#endif // LL_DEV_TYPE_MASTER

#ifndef HAL_SLEEP_DEBUG_POWER_MODE
/* set CC2540 power mode; always use PM2 */
#define HAL_SLEEP_SET_POWER_MODE(mode)       st( SLEEPCMD &= ~PMODE; /* clear mode bits */    \
                                                 SLEEPCMD |= mode;   /* set mode bits   */    \
                                                 while (!(STLOAD & LDRDY));                   \
                                                 {                                            \
                                                   halSetSleepMode();                         \
                                                 }                                            \
                                               )
#else
/* Debug: don't set power mode, just block until sleep timer interrupt */
#define HAL_SLEEP_SET_POWER_MODE(mode)      st( while(halSleepInt == FALSE); \
                                                halSleepInt = FALSE; )
#endif

/* sleep and external interrupt port masks */
#define STIE_BV                             BV(5)
#define P0IE_BV                             BV(5)
#define P1IE_BV                             BV(4)
#define P2IE_BV                             BV(1)

/* sleep timer interrupt control */
#define HAL_SLEEP_TIMER_ENABLE_INT()        st(IEN0 |= STIE_BV;)     /* enable sleep timer interrupt */
#define HAL_SLEEP_TIMER_DISABLE_INT()       st(IEN0 &= ~STIE_BV;)    /* disable sleep timer interrupt */
#define HAL_SLEEP_TIMER_CLEAR_INT()         st(IRCON &= ~0x80;)      /* clear sleep interrupt flag */

/* backup interrupt enable registers before sleep */
#define HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2) st(ien0  = IEN0;    /* backup IEN0 register */ \
                                                             ien1  = IEN1;    /* backup IEN1 register */ \
                                                             ien2  = IEN2;    /* backup IEN2 register */ \
                                                             IEN0 &= STIE_BV; /* disable IEN0 except STIE */ \
                                                             IEN1 &= P0IE_BV; /* disable IEN1 except P0IE */ \
                                                             IEN2 &= (P1IE_BV|P2IE_BV);) /* disable IEN2 except P1IE, P2IE */

/* restore interrupt enable registers before sleep */
#define HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2) st(IEN0 = ien0;   /* restore IEN0 register */ \
                                                  IEN1 = ien1;   /* restore IEN1 register */ \
                                                  IEN2 = ien2;)  /* restore IEN2 register */

/* convert msec to 625 usec units with round */
#define HAL_SLEEP_MS_TO_625US(ms)           (((((uint32) (ms)) * 8) + 4) / 5)

/* convert msec to 32kHz units without round : The ratio of 32 kHz ticks to
 * msec ticks is 32768/1000 = 32.768 or 4096/125. */
#define HAL_SLEEP_MS_TO_32KHZ(ms)           ((((uint32) (ms)) * 4096) / 125)

/* convert 625us units to 32kHz units without round: The ratio of 32 kHz ticks
 * to 625 usec ticks is 32768/1600 = 20.48 or 512/25. */
#define HAL_SLEEP_625US_TO_32KHZ(us)        ((((uint32) (us)) * 512) / 25)

/* for optimized indexing of uint32's */
#if HAL_MCU_LITTLE_ENDIAN()
#define UINT32_NDX0   0
#define UINT32_NDX1   1
#define UINT32_NDX2   2
#define UINT32_NDX3   3
#else
#define UINT32_NDX0   3
#define UINT32_NDX1   2
#define UINT32_NDX2   1
#define UINT32_NDX3   0
#endif

#define NOP()  asm("NOP")

/* ------------------------------------------------------------------------------------------------
 *                                        Local Variables
 * ------------------------------------------------------------------------------------------------
 */

/* HAL power management mode is set according to the power management state.
 */
static uint8 halPwrMgtMode = HAL_SLEEP_OFF;

/* stores the sleep timer count upon entering sleep */
//static uint32 halSleepTimerStart;

#ifdef HAL_SLEEP_DEBUG_POWER_MODE
static bool halSleepInt = FALSE;
#endif

/* ------------------------------------------------------------------------------------------------
 *                                      Function Prototypes
 * ------------------------------------------------------------------------------------------------
 */

void halSleepSetTimer(uint32 timeout);
uint32 HalTimerElapsed( void );
uint32 TimerElapsed( void );

/**************************************************************************************************
 * @fn          halSleep
 *
 * @brief       This function put the CC2540 to sleep. The PCON instruction must be 4-byte aligned.
 *              The following code may cause excessive power consumption if not aligned. See linker
 *              file ".xcl" for actual placement.
 *
 * input parameters
 *
 * @param       None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halSetSleepMode(void)
{
  // WARNING: DO NOT ADD ANY ADDITIONAL CODE; THIS IS A FIXED SIZED SEGMENT!
  PCON = PCON_IDLE;
  HAL_DISABLE_INTERRUPTS();
}

/**************************************************************************************************
 * @fn          halSleep
 *
 * @brief       This function is called from the OSAL task loop using and existing OSAL
 *              interface.  It sets the low power mode of the LL and the CC2540.
 *
 * input parameters
 *
 * @param       osal_timeout - Next OSAL timer timeout, in msec.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halSleep( uint16 osal_timeout )
{
  uint32 timeout;
  uint32 llTimeout = 0;

  /* get next OSAL timer expiration and convert to 32kHz units */
  timeout = HAL_SLEEP_MS_TO_32KHZ(osal_timeout);
  if (timeout == 0)
  {
    /* get next LL event (in 625us ticks) and convert to 32kHz units */
    timeout = HAL_SLEEP_625US_TO_32KHZ(LL_TimeToNextRfEvent());

    /* since LL time is in 625us, its specification of when to start is more
     * coarse than it needs to be which results in wasted power consumption,
     * so to compensate, add some time back (in 32kHz granularity), but only
     * if the LL is active (i.e. timeout != 0). */
    if ( timeout != 0 )
    {
      timeout += LL_WAKE_ADJUSTMENT;
    }
  }
  else
  {
    /* get next LL event (in 625us ticks) and convert to 32kHz units */
    llTimeout = HAL_SLEEP_625US_TO_32KHZ(LL_TimeToNextRfEvent());

    /* get lesser of two timeouts... */
    if ((llTimeout != 0) && (llTimeout < timeout))
    {
      /* ...and use a common variable */
      timeout = llTimeout;

      /* since LL time is in 625us, its specification of when to start is more
      * coarse than it needs to be which results in wasted power consumption,
      * so to compensate, add some time back (in 32kHz granularity), but only
      * if the LL is active (i.e. timeout != 0), which it isn't in this case. */
      timeout += LL_WAKE_ADJUSTMENT;
    }
  }

  /* HAL_SLEEP_PM2 is entered only if the timeout is zero and
   * the device is a stimulated device.
   */
  halPwrMgtMode = (timeout == 0) ? HAL_SLEEP_DEEP : HAL_SLEEP_TIMER;

  /* DEEP sleep can only be entered when zgPollRate == 0.
   * This is to eliminate any possibility of entering PM3 between
   * two network timers.
   */
  if ( (timeout > PM_MIN_SLEEP_TIME) || (timeout == 0) )
  {
    halIntState_t ien0, ien1, ien2;

    HAL_ASSERT(HAL_INTERRUPTS_ARE_ENABLED());
    HAL_DISABLE_INTERRUPTS();

    if (LL_PowerOffReq(halPwrMgtMode) == LL_SLEEP_REQUEST_ALLOWED)
    {
#if ((defined HAL_KEY) && (HAL_KEY == TRUE))
      /* get peripherals ready for sleep */
      HalKeyEnterSleep();
#endif

#ifdef HAL_SLEEP_DEBUG_LED
      HAL_TURN_OFF_LED3();
#else
      /* use this to turn LEDs off during sleep */
      HalLedEnterSleep();
#endif

      /* enable sleep timer interrupt */
      if (timeout != 0)
      {
        if (timeout > MAX_SLEEP_TIME )
        {
          halSleepSetTimer( MAX_SLEEP_TIME );
        }
        else
        {
          /* set sleep timer */
          halSleepSetTimer( timeout );
        }
      }

      /* save interrupt enable registers and disable all interrupts */
      HAL_SLEEP_IE_BACKUP_AND_DISABLE(ien0, ien1, ien2);
      HAL_ENABLE_INTERRUPTS();

      /* set CC2540 power mode, interrupt is disabled after this function */
      // TODO: NEED FLAG TO HANDLE RACE CONDITION IN CASE INTERRUPT OCCURS
      //       BETWEEN PREVIOUS ENABLE_INTERRUPTS AND ENTERING SLEEP MODE HERE?
      HAL_SLEEP_SET_POWER_MODE(halPwrMgtMode);

      /* restore interrupt enable registers */
      HAL_SLEEP_IE_RESTORE(ien0, ien1, ien2);

#ifdef HAL_SLEEP_DEBUG_LED
      HAL_TURN_ON_LED3();
#else
      /* use this to turn LEDs back on after sleep */
      HalLedExitSleep();
#endif

#if ((defined HAL_KEY) && (HAL_KEY == TRUE))
      /* handle peripherals */
      (void)HalKeyExitSleep();
#endif

      /* power on the MAC; blocks until completion */
      // Note: This is done at the end so that the 32MHz can stablize
      //       in parallel. Also note that preceding routines should not
      //       require the 32MHz timer.
      LL_PowerOnReq();
    }
    HAL_ENABLE_INTERRUPTS();
  }

  return;
}

/**************************************************************************************************
 * @fn          halSleepSetTimer
 *
 * @brief       This function sets the CC2540 sleep timer compare value.  It reads the value of
 *              the sleep timer, adds to it the timeout value (given in 32KHz units), minus a
 *              configurable adjustment to take the sleep handler execution time into account,
 *              and sets the sleep compare for the wake interrupt.
 *
 * input parameters
 *
 * @param       timeout - Timeout value in 32kHz units.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halSleepSetTimer(uint32 timeout)
{
  uint32 ticks;

  HAL_SLEEP_TIMER_DISABLE_INT();

  /* read the sleep timer; ST0 must be read first */
  ((uint8 *) &ticks)[UINT32_NDX0] = ST0;
  ((uint8 *) &ticks)[UINT32_NDX1] = ST1;
  ((uint8 *) &ticks)[UINT32_NDX2] = ST2;
  ((uint8 *) &ticks)[UINT32_NDX3] = 0;

  /* Compute sleep timer compare value. */
  ticks += timeout;

  /* subtract the processing time spent in function halSleep() */
  ticks -= HAL_SLEEP_ADJ_TICKS;

  /* set sleep timer compare; ST0 must be written last */
  ST2 = ((uint8 *) &ticks)[UINT32_NDX2];
  ST1 = ((uint8 *) &ticks)[UINT32_NDX1];
  ST0 = ((uint8 *) &ticks)[UINT32_NDX0];

  HAL_SLEEP_TIMER_CLEAR_INT();
  HAL_SLEEP_TIMER_ENABLE_INT();

  return;
}

/**************************************************************************************************
 * @fn          TimerElapsed
 *
 * @brief       Determine the number of OSAL timer ticks elapsed during sleep.
 *
 * input parameters
 *
 * @param       None.
 *
 * output parameters
 *
 * None.
 *
 * @return      Number of timer ticks elapsed during sleep.
 **************************************************************************************************
 */
uint32 TimerElapsed( void )
{
  //return ( HalTimerElapsed() );
  return( 0 );
}

/**************************************************************************************************
 * @fn          HalTimerElapsed
 *
 * @brief       Determine the number of OSAL timer ticks elapsed during sleep, then ticks are
 *              calculated in units of msec.
 *
 * input parameters
 *
 * @param       None.
 *
 * output parameters
 *
 * None.
 *
 * @return      Number of timer ticks elapsed during sleep.
 **************************************************************************************************
 */
#if 0
uint32 HalTimerElapsed( void )
{
  uint32 ticks;

  /* read the sleep timer; ST0 must be read first */
  ((uint8 *) &ticks)[UINT32_NDX0] = ST0;
  ((uint8 *) &ticks)[UINT32_NDX1] = ST1;
  ((uint8 *) &ticks)[UINT32_NDX2] = ST2;

  /* set bit 24 to handle wraparound */
  ((uint8 *) &ticks)[UINT32_NDX3] = 0x01;

  /* calculate elapsed time */
  ticks -= halSleepTimerStart;

  /* add back the processing time spent in function halSleep() */
  ticks += HAL_SLEEP_ADJ_TICKS;

  /* mask off excess if no wraparound */
  ticks &= 0x00FFFFFF;

  /* Convert elapsed time in milliseconds with round.  1000/32768 = 125/4096 */
  return ( ((ticks * 125) + 4095) / 4096 );
}
#endif

/**************************************************************************************************
 * @fn          halRestoreSleepLevel
 *
 * @brief       Restore the deepest timer sleep level.
 *
 * input parameters
 *
 * @param       None
 *
 * output parameters
 *
 *              None.
 *
 * @return      None.
 **************************************************************************************************
 */
void halRestoreSleepLevel( void )
{
  /* Stubs */
#ifdef PM_TEST
  osal_start_timerEx (Hal_TaskID, HAL_SLEEP_TIMER_EVENT, 1000);
#endif // PM_TEST
}

/**************************************************************************************************
 * @fn          halSleepTimerIsr
 *
 * @brief       Sleep timer ISR.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
HAL_ISR_FUNCTION(halSleepTimerIsr, ST_VECTOR)
{
  HAL_SLEEP_TIMER_CLEAR_INT();
  CLEAR_SLEEP_MODE();

  /* CC2540 chip bug workaround. T2 overflow compare 1 interrupt
   * is generate only for "=" but not for ">" condition.
   */

#ifdef HAL_SLEEP_DEBUG_POWER_MODE
  halSleepInt = TRUE;
#endif
}

/**************************************************************************************************
 * @fn          halSleepWait
 *
 * @brief       Perform a blocking wait for the specified number of microseconds.
 *              Use assumptions about number of clock cycles needed for the various instructions.
 *              This function assumes a 32 MHz clock.
 *              NB! This function is highly dependent on architecture and compiler!
 *
 * input parameters
 *
 * @param       duration - Duration of wait in microseconds.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
#pragma optimize=none
void halSleepWait(uint16 duration)
{
  duration >>= 1;
    
  while (duration-- > 0)
  {
    NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
    NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP();
  }
}

