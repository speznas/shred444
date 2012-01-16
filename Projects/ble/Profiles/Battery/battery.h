/**************************************************************************************************
  Filename:       battery.h
  Revised:        $Date: 2010-11-16 15:40:36 -0800 (Tue, 16 Nov 2010) $
  Revision:       $Revision: 24426 $

  Description:    This file contains Battery Profile header file.

**************************************************************************************************/

#ifndef BATTERY_H
#define BATTERY_H

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

// Profile Parameters
#define BATTERY_ATTR_LEVEL            1  // RW uint8 - Profile Attribute value
#define BATTERY_ATTR_STATE            2  // RW uint8 - Profile Attribute value
  
// Profile UUIDs
#define BATTERY_LEVEL_UUID            0xFFB1
#define BATTERY_STATE_UUID            0xFFB2
  
// Battery Service UUID
#define BATTERY_SERVICE_UUID          0xFFB0
  
// Battery Level Max
#define BATTERY_LEVEL_MAX             100  
  
// Battery States  
#define BATTERY_STATE_NOT_PRESENT             0
#define BATTERY_STATE_NOT_USED                1
#define BATTERY_STATE_CHARGING                2
#define BATTERY_STATE_RECHARGE_COMPLETE       3
#define BATTERY_STATE_DISCHARGING             4
#define BATTERY_STATE_CRITICAL_REPLACE_NOW    5
#define BATTERY_STATE_CRITICAL_RECHARGE_NOW   6
#define BATTERY_STATE_RESERVED                7
  
// Proximity Profile Services bit fields
#define BATTERY_SERVICE                       0x00000001

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

/*********************************************************************
 * API FUNCTIONS 
 */

/*
 *      Initializes the Batter service by registering GATT attributes 
 *          with the GATT server. Only call this function once.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */
bStatus_t Battery_AddService( uint32 services );

/*
 * Battery_SetParameter - Set a Battery Profile parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
extern bStatus_t Battery_SetParameter( uint8 param, uint8 len, void *value );
  
/*
 * Battery_GetParameter - Get a Battery Profile parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
extern bStatus_t Battery_GetParameter( uint8 param, void *value );





/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_H */
