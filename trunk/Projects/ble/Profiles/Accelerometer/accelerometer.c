/**************************************************************************************************
  Filename:       accelerometer.c
  Revised:        $Date: 2010-11-16 15:40:36 -0800 (Tue, 16 Nov 2010) $
  Revision:       $Revision: 24426 $

  Description:    Accelerometer Profile

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

#include "accelerometer.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED        19

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Accelerometer Service UUID
CONST uint8 accServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(ACCEL_SERVICE_UUID), HI_UINT16(ACCEL_SERVICE_UUID)
};

// Accelerometer Enabler UUID
CONST uint8 accEnablerUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(ACCEL_ENABLER_UUID), HI_UINT16(ACCEL_ENABLER_UUID)
};

// Accelerometer Range UUID
CONST uint8 rangeUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(ACCEL_RANGE_UUID), HI_UINT16(ACCEL_RANGE_UUID)
};

// Accelerometer X-Axis Data UUID
CONST uint8 xUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(ACCEL_X_UUID), HI_UINT16(ACCEL_X_UUID)
};

// Accelerometer Y-Axis Data UUID
CONST uint8 yUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(ACCEL_Y_UUID), HI_UINT16(ACCEL_Y_UUID)
};

// Accelerometer Z-Axis Data UUID
CONST uint8 zUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(ACCEL_Z_UUID), HI_UINT16(ACCEL_Z_UUID)
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
static accelCBs_t *accel_AppCBs = NULL;

// Connection the characteristic configuration was received on
uint16 charCfgConnHandle = INVALID_CONNHANDLE;

/*********************************************************************
 * Profile Attributes - variables
 */

// Accelerometer Service attribute
static CONST gattAttrType_t accelService = { ATT_BT_UUID_SIZE, accServUUID };


// Enabler Characteristic Properties
static uint8 accelEnabledCharProps = GATT_PROP_READ | GATT_PROP_WRITE;

// Enabler Characteristic Value
static uint8 accelEnabled = FALSE;

// Enabler Characteristic user description
static uint8 accelEnabledUserDesc[14] = "Accel Enable\0";


// Range Characteristic Properties
static uint8 accelRangeCharProps = GATT_PROP_READ;

// Range Characteristic Value
static uint16 accelRange = ACCEL_RANGE_2G;

// Range Characteristic user description
static uint8 accelRangeUserDesc[13] = "Accel Range\0";


// Accel Coordinate Characteristic Properties
static uint8 accelXCharProps = GATT_PROP_NOTIFY;
static uint8 accelYCharProps = GATT_PROP_NOTIFY;
static uint8 accelZCharProps = GATT_PROP_NOTIFY;

// Accel Coordinate Characteristics
static int8 accelCoordinates[3] = {0, 0, 0};

// Accel Coordinate Characteristic Configs
static uint16 accelConfigCoordinates[3] = {0, 0, 0};

// Accel Coordinate Characteristic user descriptions
static uint8 accelXCharUserDesc[20] = "Accel X-Coordinate\0";
static uint8 accelYCharUserDesc[20] = "Accel Y-Coordinate\0";
static uint8 accelZCharUserDesc[20] = "Accel Z-Coordinate\0";


/*********************************************************************
 * Profile Attributes - Table
 */
static gattAttribute_t accelAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = 
{
  // Accelerometer Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                   /* permissions */
    0,                                  /* handle */
    (uint8 *)&accelService                /* pValue */
  },
  
    // Accel Enabler Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &accelEnabledCharProps 
    },

      // Accelerometer Enable Characteristic Value
      { 
        { ATT_BT_UUID_SIZE, accEnablerUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0,
        &accelEnabled 
      },

      // Accelerometer Enable User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0,
        (uint8*)&accelEnabledUserDesc 
      },

    // Accel Range Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &accelRangeCharProps 
    },

      // Accelerometer Range Char Value
      { 
        { ATT_BT_UUID_SIZE, rangeUUID },
        GATT_PERMIT_READ, 
        0,
        (uint8*)&accelRange 
      },

      // Accelerometer Range User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0,
        accelRangeUserDesc 
      },
      
    // X-Coordinate Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &accelXCharProps 
    },
  
      // X-Coordinate Characteristic Value
      { 
        { ATT_BT_UUID_SIZE, xUUID },
        0, 
        0, 
        (uint8*) &accelCoordinates[0] 
      },
      
      // X-Coordinate Characteristic configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *)&accelConfigCoordinates[0] 
      },

      // X-Coordinate Characteristic User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        accelXCharUserDesc
      },  

   // Y-Coordinate Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &accelYCharProps 
    },
  
      // Y-Coordinate Characteristic Value
      { 
        { ATT_BT_UUID_SIZE, yUUID },
        0, 
        0, 
        (uint8*) &accelCoordinates[1] 
      },
      
      // Y-Coordinate Characteristic configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *)&accelConfigCoordinates[1] 
      },

      // Y-Coordinate Characteristic User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        accelYCharUserDesc
      },

   // Z-Coordinate Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &accelZCharProps 
    },
  
      // Z-Coordinate Characteristic Value
      { 
        { ATT_BT_UUID_SIZE, zUUID },
        0, 
        0, 
        (uint8*) &accelCoordinates[2] 
      },
      
      // Z-Coordinate Characteristic configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *)&accelConfigCoordinates[2] 
      },

      // Z-Coordinate Characteristic User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        accelZCharUserDesc
      },  

};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 accel_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                               uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t accel_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                    uint8 *pValue, uint8 len, uint16 offset );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Accel_InitService
 *
 * @brief   Initializes the Accelerometer service by
 *          registering GATT attributes with the GATT server. Only
 *          call this function once.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t Accel_InitService( uint32 services )
{
  uint8 status = SUCCESS;

  if ( services & ACCEL_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( accelAttrTbl, GATT_NUM_ATTRS( accelAttrTbl ),
                                          accel_ReadAttrCB, accel_WriteAttrCB, NULL );
  }

  return ( status );
}

/*********************************************************************
 * @fn      Accel_RegisterAppCBs
 *
 * @brief   Does the profile initialization.  Only call this function
 *          once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t Accel_RegisterAppCBs( accelCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    accel_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}


/*********************************************************************
 * @fn      Accel_SetParameter
 *
 * @brief   Set an Accelerometer Profile parameter.
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
bStatus_t Accel_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    case ACCEL_ENABLER:
      if ( len == sizeof ( uint8 ) ) 
      {
        accelEnabled = *((uint8*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    case ACCEL_RANGE:
      if ( (len == sizeof ( uint16 )) && ((*((uint8*)value)) <= ACCEL_RANGE_8G) ) 
      {
        accelRange = *((uint16*)value);
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    case ACCEL_X_ATTR:
    case ACCEL_Y_ATTR:
    case ACCEL_Z_ATTR:
      if ( len == sizeof ( int8 ) ) 
      {
        uint8 idx = param-ACCEL_X_ATTR;
        
        accelCoordinates[idx] = *((int8*)value);
        
        // Check for notification
        if ( ( (accelConfigCoordinates[idx] == GATT_CLIENT_CFG_NOTIFY) ||
               (accelConfigCoordinates[idx] == GATT_CLIENT_CFG_INDICATE)  ) &&
               (charCfgConnHandle != INVALID_CONNHANDLE)                       &&
               (accelEnabled == TRUE) )
        {
          gattAttribute_t *attr;
          
          // Get the right table entry
          attr = GATTServApp_FindAttr( accelAttrTbl, 
                                       GATT_NUM_ATTRS( accelAttrTbl ), 
                                       (uint8*)&accelCoordinates[idx] );
          if ( attr != NULL )
          {          

            if ( accelConfigCoordinates[idx] == GATT_CLIENT_CFG_NOTIFY )
            {              
              attHandleValueNoti_t notify;
  
              // Send the notification               
              notify.handle = attr->handle;
              notify.len = sizeof ( int8 );
              notify.value[0] = accelCoordinates[idx];
              ret = GATT_Notification( charCfgConnHandle, &notify, FALSE );
            }
/*
            else //indication instead of notification
            {
              attHandleValueInd_t indicate;
  
              // Send the notification               
              indicate.handle = attr->handle;
              indicate.len = sizeof ( int8 );
              indicate.value[0] = accelCoordinates[idx];
              ret = GATT_HandleValueInd( charCfgConnHandle, &indicate, FALSE );
            }
            */
          }
        }
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
 * @fn      Accel_GetParameter
 *
 * @brief   Get an Accelerometer Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Accel_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case ACCEL_ENABLER:
      *((uint8*)value) = accelEnabled;
      break;
      
    case ACCEL_RANGE:
      *((uint16*)value) = accelRange;
      break;
      
    case ACCEL_X_ATTR:
    case ACCEL_Y_ATTR:
    case ACCEL_Z_ATTR:
      *((int8*)value) = accelCoordinates[param-ACCEL_X_ATTR];
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          accel_ReadAttr
 *
 * @brief       Read an attribute.
 *
 * @param       pAttr - pointer to attribute
 * @param       pLen - length of data to be read
 * @param       pValue - pointer to data to be read
 * @param       signature - whether to include Authentication Signature
 *
 * @return      Success or Failure
 */
static uint8 accel_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                               uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  uint16 uuid;
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {    
    // 16-bit UUID
    uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
      // gattserverapp handles those types for reads
      case ACCEL_RANGE_UUID:
        *pLen = 2;
        pValue[0] = LO_UINT16( *((uint16 *)pAttr->pValue) );
        pValue[1] = HI_UINT16( *((uint16 *)pAttr->pValue) );
        break;
  
      case ACCEL_ENABLER_UUID:
      case ACCEL_X_UUID:
      case ACCEL_Y_UUID:
      case ACCEL_Z_UUID:
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
    // 128-bit UUID
    *pLen = 0;
    status = ATT_ERR_INVALID_HANDLE;
  }


  return ( status );
}

/*********************************************************************
 * @fn      accel_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle – connection message was received on
 * @param   pReq - pointer to request
 *
 * @return  Success or Failure
 */
static bStatus_t accel_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                    uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint8 notify = 0xFF;

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      case ACCEL_ENABLER_UUID:
        //Validate the value
        // Make sure it's not a blob oper
        if ( offset == 0 )
        {
          if ( len > 1 )
            status = ATT_ERR_INVALID_VALUE_SIZE;
          else if ( pValue[0] != FALSE && pValue[0] != TRUE )
            status = ATT_ERR_INVALID_VALUE;
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
          uint8 *pCurValue = (uint8 *)pAttr->pValue;
          
          *pCurValue = pValue[0];
          notify = ACCEL_ENABLER;        
        }
             
        break;
          
      case GATT_CLIENT_CHAR_CFG_UUID:
        // Validate the value
        // Make sure it's not a blob operation
        if ( offset == 0 )
        {
          if ( len == 2 )
          {
            uint16 charCfg = BUILD_UINT16( pValue[0], pValue[1] );
            
            // Validate characteristic configuration bit field
            if ( !( charCfg == GATT_CLIENT_CFG_NOTIFY   ||
                    charCfg == GATT_CLIENT_CFG_INDICATE ||
                    charCfg == GATT_CFG_NO_OPERATION ) )
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
          status = ATT_ERR_ATTR_NOT_LONG;
        }
  
        // Write the value
        if ( status == SUCCESS )
        {
          uint16 *pCurValue = (uint16 *)pAttr->pValue;
          
          *pCurValue = BUILD_UINT16( pValue[0], pValue[1] );
          
          charCfgConnHandle = connHandle;
        }
        
        break;      
          
      default:
          // Should never get here!
          status = ATT_ERR_ATTR_NOT_FOUND;
    }
  }
  else
  {
    // 128-bit UUID
    status = ATT_ERR_INVALID_HANDLE;
  }  

  // If an attribute changed then callback function to notify application of change
  if ( (notify != 0xFF) && accel_AppCBs && accel_AppCBs->pfnAccelEnabler )
    accel_AppCBs->pfnAccelEnabler();  
  
  return ( status );
}

/*********************************************************************
*********************************************************************/
