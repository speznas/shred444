/**************************************************************************************************
  Filename:       keyfobdemo.h
  Revised:        $Date: 2011-05-05 16:06:18 -0700 (Thu, 05 May 2011) $
  Revision:       $Revision: 25882 $

  Description:    This file contains Key Fob Demo Application header file.

**************************************************************************************************/

#ifndef KEYFOBDEMO_H
#define KEYFOBDEMO_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */


// Key Fob Task Events
#define KFD_START_DEVICE_EVT                              0x0001
#define KFD_BATTERY_CHECK_EVT                             0x0002
#define KFD_ACCEL_READ_EVT                                0x0004
#define KFD_TOGGLE_BUZZER_EVT                             0x0008
#define KFD_ADV_IN_CONNECTION_EVT                         0x0010

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the BLE Application
 */
extern void KeyFobApp_Init( uint8 task_id );

/*
 * Task Event Processor for the BLE Application
 */
extern uint16 KeyFobApp_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* KEYFOBDEMO_H */
