/**************************************************************************************************
  Filename:       thermometerService.c
  Revised:        $Date: 2011-05-05 08:56:11 -0700 (Thur, 05 MAY 2011) $
  Revision:       $Revision: 23333 $

  Description:    This file contains the Simple BLE Peripheral sample application 
                  for use with the CC2540 Bluetooth Low Energy Protocol Stack.

  Copyright 2011 Texas Instruments Incorporated. All rights reserved.

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
#include "gapbondmgr.h"
#include "thermometerservice.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */




/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Thermometer service
CONST uint8 thermometerServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(THERMOMETER_SERV_UUID), HI_UINT16(THERMOMETER_SERV_UUID)
};

// Thermometer temperature characteristic
CONST uint8 thermometerTempUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(THERMOMETER_TEMP_UUID), HI_UINT16(THERMOMETER_TEMP_UUID)
};

// Thermometer Site
CONST uint8 thermometerTypeUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(THERMOMETER_TYPE_UUID), HI_UINT16(THERMOMETER_TYPE_UUID)
};

// Thermometer Immediate Measurement
CONST uint8 thermometerImeasUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(THERMOMETER_IMEAS_UUID), HI_UINT16(THERMOMETER_IMEAS_UUID)
};

// Thermometer Measurement Interval
CONST uint8 thermometerIntervalUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(THERMOMETER_INTERVAL_UUID), HI_UINT16(THERMOMETER_INTERVAL_UUID)
};

// Thermometer Test Commands
CONST uint8 thermometerIRangeUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(THERMOMETER_IRANGE_UUID), HI_UINT16(THERMOMETER_IRANGE_UUID)
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

static thermometerServiceCB_t thermometerServiceCB;

/*********************************************************************
 * Profile Attributes - variables
 */

// Thermometer Service attribute
static CONST gattAttrType_t thermometerService = { ATT_BT_UUID_SIZE, thermometerServUUID };

// Thermometer Temperature Characteristic
// Note characteristic value is not stored here
static uint8 thermometerTempProps = GATT_PROP_INDICATE;
static gattCharCfg_t thermometerTempConfig;
static uint8 thermometerTemp = 0;
static uint8 thermometerTempFormat = 12; 

// Site
static uint8 thermometerTypeProps = GATT_PROP_READ;
static uint8 thermometerType  = 0;

// Intermediate Measurement
static uint8  thermometerImeasProps = GATT_PROP_NOTIFY;
static uint8  thermometerImeas=0;
static gattCharCfg_t thermometerIMeasConfig;

// Measurement Interval
static uint8  thermometerIntervalProps = GATT_PROP_INDICATE|GATT_PROP_READ|GATT_PROP_WRITE;
static uint8  thermometerInterval=30;  //default
static gattCharCfg_t thermometerIntervalConfig;

// Measurement Interval Range
static thermometerIRange_t  thermometerIRange = {1,60};

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t thermometerAttrTbl[] = 
{
  // Thermometer Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&thermometerService              /* pValue */
  },

      
    // TEMPERATURE
    // 1. Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &thermometerTempProps 
    },

    // 2. Characteristic Value
    { 
      { ATT_BT_UUID_SIZE, thermometerTempUUID },
      0, 
      0, 
      &thermometerTemp 
    },

    // 3. Characteristic Configuration 
    { 
      { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
      0, 
      (uint8 *)&thermometerTempConfig
    },
    // 4. Presentation Format
    { 
      { ATT_BT_UUID_SIZE, charFormatUUID },
      GATT_PERMIT_READ, 
      0, 
      (uint8 *)&thermometerTempFormat
    },
 
    // MEASUREMENT TYPE
   
    // 5. Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &thermometerTypeProps 
    },

    // 6. Characteristic Value
    { 
      { ATT_BT_UUID_SIZE, thermometerTypeUUID },
      GATT_PERMIT_READ, 
      0, 
      &thermometerType 
    },

    // IMMEDIATE MEASUREMENT
    
    // 7. Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &thermometerImeasProps 
    },

    // 8. Characteristic Value 
    { 
      { ATT_BT_UUID_SIZE, thermometerImeasUUID },
      0, 
      0, 
      &thermometerImeas 
    },

    // 9. Characteristic Configuration
    { 
      { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
      0, 
      (uint8 *)&thermometerIMeasConfig
    },

    // INTERVAL
    
    // 10. Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &thermometerIntervalProps 
    },

    // 11. Characteristic Value
    { 
      { ATT_BT_UUID_SIZE, thermometerIntervalUUID },
      GATT_PERMIT_READ | GATT_PERMIT_AUTHEN_WRITE,
      0, 
      &thermometerInterval 
    },
    // 12. Characteristic Configuration
    { 
      { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
      0, 
      (uint8 *)&thermometerIntervalConfig
    },    
    // 13. Interval Range
    { 
      { ATT_BT_UUID_SIZE, thermometerIRangeUUID },
      GATT_PERMIT_READ,
      0, 
      (uint8 *)&thermometerIRange 
    },
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 thermometer_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t thermometer_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

//static gattCharCfg_t *thermometer_FindCharCfgItem( uint16 connHandle, gattCharCfg_t *charCfgTbl );
//static void thermometer_ResetCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl );
static void thermometer_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Thermometer_AddService
 *
 * @brief   Initializes the Thermometer   service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t Thermometer_AddService( uint32 services )
{
  uint8 status = SUCCESS;

    thermometerTempConfig.connHandle = INVALID_CONNHANDLE;
    thermometerTempConfig.value = GATT_CFG_NO_OPERATION;
    
    thermometerIMeasConfig.connHandle = INVALID_CONNHANDLE;
    thermometerIMeasConfig.value = GATT_CFG_NO_OPERATION;    

    thermometerIntervalConfig.connHandle = INVALID_CONNHANDLE;
    thermometerIntervalConfig.value = GATT_CFG_NO_OPERATION;     

  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( thermometer_HandleConnStatusCB );  
  
  if ( services & THERMOMETER_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( thermometerAttrTbl, GATT_NUM_ATTRS( thermometerAttrTbl ),
                                          thermometer_ReadAttrCB, thermometer_WriteAttrCB, NULL );
  }

  return ( status );
}

/*********************************************************************
 * @fn      Thermometer_Register
 *
 * @brief   Register a callback function with the Thermometer Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
extern void Thermometer_Register( thermometerServiceCB_t pfnServiceCB )
{
  thermometerServiceCB = pfnServiceCB;
}

/*********************************************************************
 * @fn      Thermometer_SetParameter
 *
 * @brief   Set a thermomter parameter.
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
bStatus_t Thermometer_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
 
    case THERMOMETER_TYPE:
      thermometerType = *((uint8*)value);
      break;
      
    case THERMOMETER_INTERVAL:
      thermometerInterval = *((uint8*)value);
      break;      
 
    case THERMOMETER_TEMP_CHAR_CFG:      
      thermometerTempConfig.value = *((uint8*)value);
    break;
      
    case THERMOMETER_IMEAS_CHAR_CFG:      
      thermometerIMeasConfig.value = *((uint8*)value);
      break; 

    case THERMOMETER_INTERVAL_CHAR_CFG:      
      thermometerIntervalConfig.value = *((uint8*)value);
      break;       
      
    case THERMOMETER_IRANGE:      
      thermometerIRange = *((thermometerIRange_t*)value);
      break;       
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      Thermometer_GetParameter
 *
 * @brief   Get a Thermometer   parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Thermometer_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;

  switch ( param )
  {
    case THERMOMETER_TYPE:
      *((uint8*)value) = thermometerType;
      break;

    case THERMOMETER_INTERVAL:
      *((uint8*)value) = thermometerInterval;
      break;
    
    case THERMOMETER_IRANGE:
      *((thermometerIRange_t*)value) = thermometerIRange;
      break;
      
  case THERMOMETER_TEMP_CHAR_CFG:
      *((uint16*)value) = thermometerTempConfig.value;
        break;
        
  case THERMOMETER_IMEAS_CHAR_CFG:
      *((uint16*)value) = thermometerIMeasConfig.value;
        break;        
        
  case THERMOMETER_INTERVAL_CHAR_CFG:
      *((uint16*)value) = thermometerIntervalConfig.value;
        break;        
    
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          Thermometer_Indicate
 *
 * @brief       Send a notification containing a thermometer
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t Thermometer_Indicate( uint16 connHandle, attHandleValueInd_t *pNoti,uint8 taskId )
{
  
    
    // Set the handle (uses stored relative handle to lookup actual handle)
    pNoti->handle = thermometerAttrTbl[pNoti->handle].handle;
  
    // Send the Indication
    return GATT_Indication( connHandle, pNoti, FALSE, taskId );
}

/*********************************************************************
 * @fn          Thermometer_IntervalIndicate
 *
 * @brief       Send a interval change notification
 *              
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
/*bStatus_t Thermometer_IntervalIndicate( uint16 connHandle, attHandleValueInd_t *pNoti,uint8 taskId )
{
    // Set the handle
    //pNoti->handle = thermometerAttrTbl[THERMOMETER_INTERVAL_VALUE_POS].handle;
  
    // Send the Indication
    return GATT_Indication( connHandle, pNoti, FALSE, taskId );
}
*/
/*********************************************************************
 * @fn          Thermometer_IMeasNotify
 *
 * @brief       Send a notification containing a thermometer
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t Thermometer_IMeasNotify( uint16 connHandle, attHandleValueNoti_t *pNoti)
{
    // Set the handle
    pNoti->handle = thermometerAttrTbl[THERMOMETER_IMEAS_VALUE_POS].handle;
  
    // Send the Notification
    return GATT_Notification( connHandle, pNoti, FALSE );
}

/*********************************************************************
 * @fn          thermometer_ReadAttrCB
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
static uint8 thermometer_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;

  // If attribute permissions require authorization to read, return error
  if ( gattPermitAuthorRead( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      case GATT_CLIENT_CHAR_CFG_UUID:
        {
          //Thermometer Temperature Client Char Config
          if(pAttr->handle == thermometerAttrTbl[THERMOMETER_TEMP_CHAR_CONFIG_POS].handle)
          {
            *pLen = 2;
            pValue[0] = LO_UINT16( thermometerTempConfig.value );
            pValue[1] = HI_UINT16( thermometerTempConfig.value );
          }
          //Thermometer Intermediate Measurement Client Char Config          
          if(pAttr->handle == thermometerAttrTbl[THERMOMETER_IMEAS_CHAR_CONFIG_POS].handle)
          {
            *pLen = 2;
            pValue[0] = LO_UINT16( thermometerIMeasConfig.value );
            pValue[1] = HI_UINT16( thermometerIMeasConfig.value );
          }
          //Thermometer Interval Client Char Config
          if(pAttr->handle == thermometerAttrTbl[THERMOMETER_INTERVAL_CHAR_CONFIG_POS].handle)
          {
            *pLen = 2;
            pValue[0] = LO_UINT16( thermometerIntervalConfig.value );
            pValue[1] = HI_UINT16( thermometerIntervalConfig.value );
          }          
        }
        break;      
      
      case THERMOMETER_TYPE_UUID:
        *pLen = THERMOMETER_TYPE_LEN;
        VOID osal_memcpy( pValue, &thermometerType, THERMOMETER_TYPE_LEN ) ;
        break;
        
      case THERMOMETER_INTERVAL_UUID:
        *pLen = THERMOMETER_INTERVAL_LEN;
        VOID osal_memcpy( pValue, &thermometerInterval, THERMOMETER_INTERVAL_LEN ) ;
        break;

      case THERMOMETER_IRANGE_UUID:
        *pLen = THERMOMETER_IRANGE_LEN;
         VOID osal_memcpy( pValue, &thermometerIRange, THERMOMETER_IRANGE_LEN ) ;
        break;        
        
      default:
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  
  return ( status );
}

/*********************************************************************
 * @fn      thermometer_WriteAttrCB
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
static bStatus_t thermometer_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);

  switch ( uuid )
  {
    
  case  GATT_CLIENT_CHAR_CFG_UUID:
    
    if ( len == 2 )
      {
        //two bye config
        uint16 charCfg = BUILD_UINT16( pValue[0], pValue[1] );
      
        // Validate Temperature measurement setting
        if ( (charCfg == GATT_CLIENT_CFG_INDICATE ||
               charCfg == GATT_CFG_NO_OPERATION) &&
                 pAttr->handle == thermometerAttrTbl[THERMOMETER_TEMP_CHAR_CONFIG_POS].handle )
        {

          uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
          thermometerTempConfig.connHandle = connHandle;
          thermometerTempConfig.value = value;

           // update bond manager
           VOID GAPBondMgr_UpdateCharCfg( connHandle, pAttr->handle, value );          
        
           (*thermometerServiceCB)((charCfg == 0) ? THERMOMETER_TEMP_IND_DISABLED :
                                                 THERMOMETER_TEMP_IND_ENABLED);

        }
        // Validate Intermediate measurement setting
        else if ( (charCfg == GATT_CLIENT_CFG_NOTIFY ||
                 charCfg == GATT_CFG_NO_OPERATION) &&
                 pAttr->handle == thermometerAttrTbl[THERMOMETER_IMEAS_CHAR_CONFIG_POS].handle )
        {
          uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
          thermometerIMeasConfig.connHandle = connHandle;
          thermometerIMeasConfig.value = value;
         
          // Update Bond Manager
          VOID GAPBondMgr_UpdateCharCfg( connHandle, pAttr->handle, value );
           
          // Notify application 
          (*thermometerServiceCB)((charCfg == 0) ? THERMOMETER_IMEAS_NOTI_DISABLED :
                                                     THERMOMETER_IMEAS_NOTI_ENABLED);              
        
        }
        // Interval Client Char Config
        else if ( (charCfg == GATT_CLIENT_CFG_INDICATE ||
                 charCfg == GATT_CFG_NO_OPERATION) &&
                 pAttr->handle == thermometerAttrTbl[THERMOMETER_INTERVAL_CHAR_CONFIG_POS].handle )
        {
          uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
          thermometerIntervalConfig.connHandle = connHandle;
          thermometerIntervalConfig.value = value;
         
          // Update Bond Manager
          VOID GAPBondMgr_UpdateCharCfg( connHandle, pAttr->handle, value );
           
          // Notify application 
          (*thermometerServiceCB)((charCfg == 0) ? THERMOMETER_INTERVAL_IND_DISABLED :
                                                     THERMOMETER_INTERVAL_IND_ENABLED);              
        
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
       
    break;
  
    case  THERMOMETER_INTERVAL_UUID:
         
    //Validate the value
    // Make sure it's not a blob oper
    if ( offset == 0 )
    {
      if ( len != THERMOMETER_INTERVAL_LEN )
        status = ATT_ERR_INVALID_VALUE_SIZE;
    }
    else
    {
      status = ATT_ERR_ATTR_NOT_LONG;
    }
    
    //validate range
    if ((*pValue >= thermometerIRange.high) | ((*pValue <= thermometerIRange.low) & (*pValue != 0)))
    {
      status = ATT_ERR_INVALID_VALUE;
    }
    
    //Write the value
    if ( status == SUCCESS )
    {
      uint8 *pCurValue = (uint8 *)pAttr->pValue;        
      // *pCurValue = *pValue; 
      VOID osal_memcpy( pCurValue, pValue, THERMOMETER_INTERVAL_LEN ) ;
      
      //notify application of write
      (*thermometerServiceCB)(THERMOMETER_INTERVAL_SET);
      
    }
    break;
    
    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }
  
  return ( status );
}


/*********************************************************************
 * @fn          thermometer_HandleConnStatusCB
 *
 * @brief       Simple Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void thermometer_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
        thermometerTempConfig.value  = GATT_CFG_NO_OPERATION;
        thermometerIMeasConfig.value = GATT_CFG_NO_OPERATION;
        thermometerIntervalConfig.value =GATT_CFG_NO_OPERATION;
    }
  }
}




/*********************************************************************
*********************************************************************/
