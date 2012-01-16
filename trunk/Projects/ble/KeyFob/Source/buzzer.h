/**************************************************************************************************
  Filename:       buzzer.h
  Revised:        $Date: 2010-09-29 11:12:58 -0700 (Wed, 29 Sep 2010) $
  Revision:       $Revision: 23938 $

  Description:    Header file for control of the buzzer of the keyfob board in 
                  the CC2540DK-mini kit.

**************************************************************************************************/

#ifndef BUZZER_H
#define BUZZER_H

#include "hal_types.h"

// Function prototypes
extern void buzzerInit(void);
extern uint8 buzzerStart(uint16 frequency);
extern void buzzerStop(void);
#endif
