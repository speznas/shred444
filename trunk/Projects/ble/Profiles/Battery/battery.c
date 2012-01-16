/**************************************************************************************************
  Filename:       battery.c
  Revised:        $Date: 2011-03-15 17:49:12 -0700 (Tue, 15 Mar 2011) $
  Revision:       $Revision: 25439 $

  Description:    Battery Profile

**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "battery.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED        7

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Battery Service UUID
CONST uint8 battServiceUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATTERY_SERVICE_UUID), HI_UINT16(BATTERY_SERVICE_UUID)
};

// Battery Level UUID
CONST uint8 battLevelUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATTERY_LEVEL_UUID), HI_UINT16(BATTERY_LEVEL_UUID)
};

// Battery State UUID
CONST uint8 battStateUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATTERY_STATE_UUID), HI_UINT16(BATTERY_STATE_UUID)
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * Profile Attributes - variables
 */

// Battery Service attribute
static CONST gattAttrType_t battService = { ATT_BT_UUID_SIZE, battServiceUUID };


// Battery Level Characteristic Properties
static uint8 batLevelCharProps = GATT_PROP_READ;

// Battery Level Characteristic Value
static uint8 batLevel = 100;

// Battery Level Characteristic User Description
static uint8 batLevelCharUserDesp[14] = "Battery Level\0";


// Battery State Characteristic Properties
static uint8 batStateCharProps = GATT_PROP_READ;

// Battery State Characteristic
static uint8 batState = BATTERY_STATE_NOT_PRESENT;

// Battery State Characteristic User Description
static uint8 batStateCharUserDesp[14] = "Battery State\0";

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t batteryAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = 
{
  // Battery Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                   /* permissions */
    0,                                  /* handle */
    (uint8 *)&battService                /* pValue */
  },
  
    // Battery Level Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &batLevelCharProps 
    },

      // Battery Level Characteristic Value
      { 
        { ATT_BT_UUID_SIZE, battLevelUUID },
        GATT_PERMIT_READ, 
        0,
        (uint8*)&batLevel 
      },

      // Battery Level Characteristic User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        batLevelCharUserDesp 
      },      

    // Battery State Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &batStateCharProps 
    },

      // Battery State Characteristic Value
      { 
        { ATT_BT_UUID_SIZE, battStateUUID },
        GATT_PERMIT_READ, 
        0,
        (uint8*)&batState 
      },

      // Battery State Characteristic User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        batStateCharUserDesp 
      },   
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 bat_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Battery_AddService
 *
 * @brief   Initializes the Batter service by registering GATT attributes 
 *          with the GATT server. Only call this function once.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  success or failure
 */
bStatus_t Battery_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  if ( services & BATTERY_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App  
    status = GATTServApp_RegisterService( batteryAttrTbl, GATT_NUM_ATTRS( batteryAttrTbl ), 
                                          bat_ReadAttrCB, NULL, NULL );
  }

  return ( status );
}


/*********************************************************************
 * @fn      Battery_SetParameter
 *
 * @brief   Set a Battery Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Battery_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
     case BATTERY_ATTR_LEVEL:
      if ( (len == sizeof ( uint8 )) && ((*((uint8*)value) <= BATTERY_LEVEL_MAX)) ) 
      {
        batLevel = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    case BATTERY_ATTR_STATE:
      if ( (len == sizeof ( uint8 )) && ((*((uint8*)value) < BATTERY_STATE_RESERVED)) ) 
      {
        batState = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      Battery_GetParameter
 *
 * @brief   Get a Battery Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Battery_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case BATTERY_ATTR_LEVEL:
      *((uint8*)value) = batLevel;
      break;
      
    case BATTERY_ATTR_STATE:
      *((uint8*)value) = batState;
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}


            
/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */

/*********************************************************************
 * @fn          bat_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 *
 * @return      Success or Failure
 */
static uint8 bat_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }

  // 16-bit UUID
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);  
  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    switch ( uuid )
    {
      // No need for "GATT_SERVICE_UUID" case;
      // gattserverapp handles those types for reads  
      case BATTERY_LEVEL_UUID:
      case BATTERY_STATE_UUID:
        *pLen = 1;
        pValue[0] = *pAttr->pValue;
        break;
      
      default:
        // Should never get here!
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    //128-bit UUID
    *pLen = 0;
    status = ATT_ERR_INVALID_HANDLE;
  }
  
  return ( status );
}


/*********************************************************************
*********************************************************************/
