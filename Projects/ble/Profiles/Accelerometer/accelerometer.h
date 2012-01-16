/**************************************************************************************************
  Filename:       accelerometer.h
  Revised:        $Date: 2010-11-16 15:40:36 -0800 (Tue, 16 Nov 2010) $
  Revision:       $Revision: 24426 $

  Description:    This file contains Accelerometer Profile header file.

**************************************************************************************************/

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

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
#define ACCEL_ENABLER                 0  // RW uint8 - Profile Attribute value
#define ACCEL_X_ATTR                  1  // RW int16 - Profile Attribute value
#define ACCEL_Y_ATTR                  2  // RW int16 - Profile Attribute value
#define ACCEL_Z_ATTR                  3  // RW int16 - Profile Attribute value
#define ACCEL_RANGE                   4  // RW uint16 - Profile Attribute value
  
// Profile UUIDs
#define ACCEL_ENABLER_UUID            0xFFA1
#define ACCEL_RANGE_UUID              0xFFA2
#define ACCEL_X_UUID                  0xFFA3
#define ACCEL_Y_UUID                  0xFFA4
#define ACCEL_Z_UUID                  0xFFA5
  
// Accelerometer Service UUID
#define ACCEL_SERVICE_UUID            0xFFA0
  
// Profile Range Values
#define ACCEL_RANGE_2G                20
#define ACCEL_RANGE_8G                80

// Accelerometer Profile Services bit fields
#define ACCEL_SERVICE                 0x00000001

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */
// Callback when the device has been started.  Callback event to 
// the ask for a battery check.
typedef NULL_OK void (*accelEnabler_t)( void );

typedef struct
{
  accelEnabler_t        pfnAccelEnabler;  // Called when Enabler attribute changes
} accelCBs_t;

/*********************************************************************
 * API FUNCTIONS 
 */

/*
 * Accel_InitService- Initializes the Accelerometer service by registering 
 *          GATT attributes with the GATT server. Only call this function once.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */
extern bStatus_t Accel_InitService( uint32 services );

/*
 * Accel_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
extern bStatus_t Accel_RegisterAppCBs( accelCBs_t *appCallbacks );


/*
 * Accel_SetParameter - Set an Accelerometer Profile parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
extern bStatus_t Accel_SetParameter( uint8 param, uint8 len, void *value );
  
/*
 * Accel_GetParameter - Get an Accelerometer Profile parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
extern bStatus_t Accel_GetParameter( uint8 param, void *value );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ACCELEROMETER_H */
