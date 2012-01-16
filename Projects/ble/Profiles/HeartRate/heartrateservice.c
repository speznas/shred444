/**************************************************************************************************
  Filename:       heartrateservice.c
  Revised:        $Date $
  Revision:       $Revision $

  Description:    This file contains the Heart Rate sample service 
                  for use with the Heart Rate sample application.

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

#include "heartrateservice.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Position of heart rate measurement value in attribute array
#define HEARTRATE_MEAS_VALUE_POS            2

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Heart rate service
CONST uint8 heartRateServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HEARTRATE_SERV_UUID), HI_UINT16(HEARTRATE_SERV_UUID)
};

// Heart rate measurement characteristic
CONST uint8 heartRateMeasUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HEARTRATE_MEAS_UUID), HI_UINT16(HEARTRATE_MEAS_UUID)
};

// Sensor location characteristic
CONST uint8 heartRateSensLocUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HEARTRATE_SENS_LOC_UUID), HI_UINT16(HEARTRATE_SENS_LOC_UUID)
};

// Command characteristic
CONST uint8 heartRateCommandUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HEARTRATE_COMMAND_UUID), HI_UINT16(HEARTRATE_COMMAND_UUID)
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

static heartRateServiceCB_t heartRateServiceCB;

/*********************************************************************
 * Profile Attributes - variables
 */

// Heart Rate Service attribute
static CONST gattAttrType_t heartRateService = { ATT_BT_UUID_SIZE, heartRateServUUID };

// Heart Rate Measurement Characteristic
// Note characteristic value is not stored here
static uint8 heartRateMeasProps = GATT_PROP_NOTIFY;
static uint8 heartRateMeas = 0;
static gattCharCfg_t heartRateMeasClientCharCfg = { INVALID_CONNHANDLE, GATT_CFG_NO_OPERATION };

// Sensor Location Characteristic
static uint8 heartRateSensLocProps = GATT_PROP_READ;
static uint8 heartRateSensLoc = 0;

// Command Characteristic
static uint8 heartRateCommandProps = GATT_PROP_WRITE;
static uint8 heartRateCommand = 0;

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t heartRateAttrTbl[] = 
{
  // Heart Rate Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&heartRateService                /* pValue */
  },

    // Heart Rate Measurement Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &heartRateMeasProps 
    },

      // Heart Rate Measurement Value
      { 
        { ATT_BT_UUID_SIZE, heartRateMeasUUID },
        0, 
        0, 
        &heartRateMeas 
      },

      // Heart Rate Measurement Client Characteristic Configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *) &heartRateMeasClientCharCfg 
      },      

    // Sensor Location Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &heartRateSensLocProps 
    },

      // Sensor Location Value
      { 
        { ATT_BT_UUID_SIZE, heartRateSensLocUUID },
        GATT_PERMIT_READ, 
        0, 
        &heartRateSensLoc 
      },

    // Command Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &heartRateCommandProps 
    },

      // Command Value
      { 
        { ATT_BT_UUID_SIZE, heartRateCommandUUID },
        GATT_PERMIT_WRITE, 
        0, 
        &heartRateCommand 
      }
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 heartRate_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t heartRate_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      HeartRate_AddService
 *
 * @brief   Initializes the Heart Rate service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t HeartRate_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  if ( services & HEARTRATE_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( heartRateAttrTbl, GATT_NUM_ATTRS( heartRateAttrTbl ),
                                          heartRate_ReadAttrCB, heartRate_WriteAttrCB, NULL );
  }

  return ( status );
}

/*********************************************************************
 * @fn      HeartRate_Register
 *
 * @brief   Register a callback function with the Heart Rate Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
extern void HeartRate_Register( heartRateServiceCB_t pfnServiceCB )
{
  heartRateServiceCB = pfnServiceCB;
}

/*********************************************************************
 * @fn      HeartRate_SetParameter
 *
 * @brief   Set a Heart Rate parameter.
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
bStatus_t HeartRate_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
     case HEARTRATE_MEAS_CHAR_CFG:
      heartRateMeasClientCharCfg.value = *((uint16*)value);
      break;      

    case HEARTRATE_SENS_LOC:
      heartRateSensLoc = *((uint8*)value);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      HeartRate_GetParameter
 *
 * @brief   Get a Heart Rate parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t HeartRate_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case HEARTRATE_MEAS_CHAR_CFG:
      *((uint16*)value) = heartRateMeasClientCharCfg.value;
      break;      

    case HEARTRATE_SENS_LOC:
      *((uint8*)value) = heartRateSensLoc;
      break;
      
    case HEARTRATE_COMMAND:
      *((uint8*)value) = heartRateCommand;
      break;  

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          HeartRate_MeasNotify
 *
 * @brief       Send a notification containing a heart rate
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t HeartRate_MeasNotify( uint16 connHandle, attHandleValueNoti_t *pNoti )
{
  // If notifications enabled
  if (heartRateMeasClientCharCfg.value == GATT_CLIENT_CFG_NOTIFY)
  {
    // Set the handle
    pNoti->handle = heartRateAttrTbl[HEARTRATE_MEAS_VALUE_POS].handle;
  
    // Send the notification
    return GATT_Notification( connHandle, pNoti, FALSE );
  }
  else
  {
    return bleIncorrectMode;
  }
}
                               
/*********************************************************************
 * @fn          heartRate_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
static uint8 heartRate_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);

  if (uuid == HEARTRATE_SENS_LOC_UUID)
  {
    *pLen = 1;
    pValue[0] = *pAttr->pValue;
  }
  else if ( uuid == GATT_CLIENT_CHAR_CFG_UUID )
  {
    *pLen = 2;
    pValue[0] = LO_UINT16( heartRateMeasClientCharCfg.value );
    pValue[1] = HI_UINT16( heartRateMeasClientCharCfg.value );
  }

  return ( status );
}

/*********************************************************************
 * @fn      heartRate_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   complete - whether this is the last packet
 * @param   oper - whether to validate and/or write attribute value  
 *
 * @return  Success or Failure
 */
static bStatus_t heartRate_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);

  if (uuid == HEARTRATE_COMMAND_UUID)
  {
    if (len != 1)
    {
      status = ATT_ERR_INVALID_VALUE_SIZE;
    }
    else if (*pValue != HEARTRATE_COMMAND_ENERGY_EXP)
    {
      status = ATT_ERR_INVALID_VALUE;
    }
    else
    {
      *(pAttr->pValue) = pValue[0];
      
      (*heartRateServiceCB)(HEARTRATE_COMMAND_SET);
      
    }
  }
  else if (uuid == GATT_CLIENT_CHAR_CFG_UUID)
  {
    if ( len == 2 )
    {
      uint16 charCfg = BUILD_UINT16( pValue[0], pValue[1] );
      
      // Validate characteristic configuration bit field
      if ( charCfg == GATT_CLIENT_CFG_NOTIFY ||
           charCfg == GATT_CFG_NO_OPERATION )
      {
        heartRateMeasClientCharCfg.connHandle = connHandle;
        heartRateMeasClientCharCfg.value = charCfg;
        
        (*heartRateServiceCB)((charCfg == 0) ? HEARTRATE_MEAS_NOTI_DISABLED :
                                               HEARTRATE_MEAS_NOTI_ENABLED);
      }
      else
      {
        status = ATT_ERR_INVALID_VALUE;
      }
    }
    else
    {
      status = ATT_ERR_INVALID_VALUE_SIZE;
    }
  }
  else
  {
    status = ATT_ERR_ATTR_NOT_FOUND;
  }

  return ( status );
}

/*********************************************************************
*********************************************************************/
