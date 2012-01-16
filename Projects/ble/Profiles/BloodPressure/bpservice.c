/**************************************************************************************************
  Filename:       bpservice.c
  Revised:        $Date $
  Revision:       $Revision $

  Description:    This file contains the BloodPressure sample service 
                  for use with the BloodPressure   sample application.

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
#include "bpservice.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Position of bloodPressure measurement value in attribute array
#define BLOODPRESSURE_MEAS_VALUE_POS       2
#define BLOODPRESSURE_MEAS_CONFIG_POS      3
#define BLOODPRESSURE_IMEAS_VALUE_POS      6
#define BLOODPRESSURE_IMEAS_CONFIG_POS     7 
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// BloodPressure service
CONST uint8 bloodPressureServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BLOODPRESSURE_SERV_UUID), HI_UINT16(BLOODPRESSURE_SERV_UUID)
};

// BloodPressure temperature characteristic
CONST uint8 bloodPressureTempUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BLOODPRESSURE_MEAS_UUID), HI_UINT16(BLOODPRESSURE_MEAS_UUID)
};

// BloodPressure Immediate Measurement
CONST uint8 bloodPressureImeasUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BLOODPRESSURE_IMEAS_UUID), HI_UINT16(BLOODPRESSURE_IMEAS_UUID)
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

static bloodPressureServiceCB_t bloodPressureServiceCB;

/*********************************************************************
 * Profile Attributes - variables
 */

// BloodPressure Service attribute
static CONST gattAttrType_t bloodPressureService = { ATT_BT_UUID_SIZE, bloodPressureServUUID };

// BloodPressure Characteristic
static uint8 bloodPressureTempProps = GATT_PROP_INDICATE;
static gattCharCfg_t bloodPressureMeasConfig[GATT_MAX_NUM_CONN];
static uint8 bloodPressureTemp = 0;
static uint8 bloodPressureTempFormat = 12; 

// Intermediate Measurement
static uint8  bloodPressureImeasProps = GATT_PROP_NOTIFY;
static uint8  bloodPressureImeas=0;
static gattCharCfg_t bloodPressureIMeasConfig[GATT_MAX_NUM_CONN];

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t bloodPressureAttrTbl[] = 
{
  // BloodPressure Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&bloodPressureService                /* pValue */
  },

    // 1. Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &bloodPressureTempProps 
    },

    // 2. Characteristic Value
    { 
      { ATT_BT_UUID_SIZE, bloodPressureTempUUID },
      0, //return READ_NOT_PERMITTED
      0, 
      &bloodPressureTemp 
    },

    // 3.Characteristic Configuration
    { 
      { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
      0, 
      (uint8 *)&bloodPressureMeasConfig
    },
    // 4.Presentation Format
    { 
      { ATT_BT_UUID_SIZE, charFormatUUID },
      GATT_PERMIT_READ, 
      0, 
      (uint8 *)&bloodPressureTempFormat
    },
  
 
    //////////////////////////////////////////////
    // IMMEDIATE MEASUREMENT
    //////////////////////////////////////////////
    
    // 5.Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &bloodPressureImeasProps 
    },

    // 6.Characteristic Value
    { 
      { ATT_BT_UUID_SIZE, bloodPressureImeasUUID },
      0, //return READ_NOT_PERMITTED
      0, 
      &bloodPressureImeas 
    },

    // 7.Characteristic Configuration
    { 
      { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
      0, 
      (uint8 *)&bloodPressureIMeasConfig
    },
   
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 bloodPressure_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t bloodPressure_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

static gattCharCfg_t *bloodPressure_FindCharCfgItem( uint16 connHandle, gattCharCfg_t *charCfgTbl );
static void bloodPressure_ResetCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl );
static uint16 bloodPressure_ReadCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl );
static uint8 bloodPressure_WriteCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl, uint16 value );
static void bloodPressure_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      BloodPressure_AddService
 *
 * @brief   Initializes the BloodPressure   service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t BloodPressure_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  for ( uint8 i = 0; i < GATT_MAX_NUM_CONN; i++ )
  {
    bloodPressureMeasConfig[i].connHandle = INVALID_CONNHANDLE;
    bloodPressureMeasConfig[i].value = GATT_CFG_NO_OPERATION;
 
    bloodPressureIMeasConfig[i].connHandle = INVALID_CONNHANDLE;
    bloodPressureIMeasConfig[i].value = GATT_CFG_NO_OPERATION;
  }
  
  // Register with Link DB to receive link status change callback
  VOID linkDB_Register( bloodPressure_HandleConnStatusCB ); 
  
  if ( services & BLOODPRESSURE_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( bloodPressureAttrTbl, GATT_NUM_ATTRS( bloodPressureAttrTbl ),
                                          bloodPressure_ReadAttrCB, bloodPressure_WriteAttrCB, NULL );
  }
  return ( status );
}

/*********************************************************************
 * @fn      BloodPressure_Register
 *
 * @brief   Register a callback function with the BloodPressure Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
extern void BloodPressure_Register( bloodPressureServiceCB_t pfnServiceCB )
{
  bloodPressureServiceCB = pfnServiceCB;
}

/*********************************************************************
 * @fn      BloodPressure_SetParameter
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
bStatus_t BloodPressure_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
 
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      BloodPressure_GetParameter
 *
 * @brief   Get a BloodPressure   parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t BloodPressure_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          BloodPressure_IMeasNotify
 *
 * @brief       Send a notification containing a bloodPressure
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t BloodPressure_MeasIndicate( uint16 connHandle, attHandleValueInd_t *pNoti,uint8 taskId )
{
 
    // Set the handle
    pNoti->handle = bloodPressureAttrTbl[BLOODPRESSURE_MEAS_VALUE_POS].handle;
  
    // Send the Indication
    return GATT_Indication( connHandle, pNoti, FALSE, taskId );
    
}


/*********************************************************************
 * @fn          BloodPressure_IMeasNotify
 *
 * @brief       Send a notification containing a bloodPressure
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t BloodPressure_IMeasNotify( uint16 connHandle, attHandleValueNoti_t *pNoti, uint8 taskId )
{
    // Set the handle
    pNoti->handle = bloodPressureAttrTbl[BLOODPRESSURE_IMEAS_VALUE_POS].handle;
  
    // Send the Indication
    return GATT_Notification( connHandle, pNoti, FALSE);
}
                              
/*********************************************************************
 * @fn          bloodPressure_ReadAttrCB
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
static uint8 bloodPressure_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
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
      // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
      // gattserverapp handles those types for reads
 
      case GATT_CLIENT_CHAR_CFG_UUID:
        {
          uint16 value = bloodPressure_ReadCharCfg( connHandle, 
                                               (gattCharCfg_t *)(pAttr->pValue) );
          *pLen = 2;
          pValue[0] = LO_UINT16( value );
          pValue[1] = HI_UINT16( value );
        }
        break;      
       
     
      default:
        // Should never get here! (characteristics 3 and 4 do not have read permissions)
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  
  return ( status );
}

/*********************************************************************
 * @fn      bloodPressure_WriteAttrCB
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
static bStatus_t bloodPressure_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
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

        // BloodPressure Indications
        if ( (charCfg == GATT_CLIENT_CFG_INDICATE ||
               charCfg == GATT_CFG_NO_OPERATION) &&
                 pAttr->handle == bloodPressureAttrTbl[BLOODPRESSURE_MEAS_CONFIG_POS].handle )
        {
          uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
          
          status = bloodPressure_WriteCharCfg( connHandle,
                                          (gattCharCfg_t *)(pAttr->pValue),
                                           value );
          if ( status == SUCCESS )
          {
              // update bond manager
              VOID GAPBondMgr_UpdateCharCfg( connHandle, pAttr->handle, value );         
          
              (*bloodPressureServiceCB)((charCfg == 0) ? BLOODPRESSURE_MEAS_NOTI_DISABLED :
                                                     BLOODPRESSURE_MEAS_NOTI_ENABLED);
        
          }        
        }
        // BloodPressure Notifications
        else if ( (charCfg == GATT_CLIENT_CFG_NOTIFY ||
                 charCfg == GATT_CFG_NO_OPERATION) &&
                 pAttr->handle == bloodPressureAttrTbl[BLOODPRESSURE_IMEAS_CONFIG_POS].handle )
        {
          uint16 value = BUILD_UINT16( pValue[0], pValue[1] );
          
          status = bloodPressure_WriteCharCfg( connHandle,
                                          (gattCharCfg_t *)(pAttr->pValue),
                                           value );
          if ( status == SUCCESS )
          {
            // update bond manager
            VOID GAPBondMgr_UpdateCharCfg( connHandle, pAttr->handle, value );          
           
            (*bloodPressureServiceCB)((charCfg == 0) ? BLOODPRESSURE_IMEAS_NOTI_DISABLED :
                                                     BLOODPRESSURE_IMEAS_NOTI_ENABLED);
          }
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
 
    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }
  
  return ( status );
}

/*********************************************************************
 * @fn      bloodPressure_ReadCharCfg
 *
 * @brief   Read the client characteristic configuration for a given
 *          client.
 *
 *          Note: Each client has its own instantiation of the Client
 *                Characteristic Configuration. Reads of the Client 
 *                Characteristic Configuration only shows the configuration
 *                for that client.
 *
 * @param   connHandle - connection handle.
 * @param   charCfgTbl - client characteristic configuration table.
 *
 * @return  attribute value
 */
static uint16 bloodPressure_ReadCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl )
{
  gattCharCfg_t *pItem;

  pItem = bloodPressure_FindCharCfgItem( connHandle, charCfgTbl );
  if ( pItem != NULL )
  {
    return ( (uint16)(pItem->value) );
  }

  return ( (uint16)GATT_CFG_NO_OPERATION );
}

/*********************************************************************
 * @fn      bloodPressure_FindCharCfgItem
 *
 * @brief   Find the characteristic configuration for a given client.
 *          Uses the connection handle to search the charactersitic 
 *          configuration table of a client.
 *
 * @param   connHandle - connection handle.
 * @param   charCfgTbl - characteristic configuration table.
 *
 * @return  pointer to the found item. NULL, otherwise.
 */
static gattCharCfg_t *bloodPressure_FindCharCfgItem( uint16 connHandle, gattCharCfg_t *charCfgTbl )
{
  for ( uint8 i = 0; i < GATT_MAX_NUM_CONN; i++ )
  {
    if ( charCfgTbl[i].connHandle == connHandle )
    {
      // Entry found
      return ( &(charCfgTbl[i]) );
    }
  }

  return ( (gattCharCfg_t *)NULL );
}

/*********************************************************************
 * @fn      bloodPressure_ResetCharCfg
 *
 * @brief   Reset the client characteristic configuration for a given
 *          client.
 *
 * @param   connHandle - connection handle.
 * @param   charCfgTbl - client characteristic configuration table.
 *
 * @return  none
 */

static void bloodPressure_ResetCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl )
{
  gattCharCfg_t *pItem;

  pItem = bloodPressure_FindCharCfgItem( connHandle, charCfgTbl );
  if ( pItem != NULL )
  {
    pItem->connHandle = INVALID_CONNHANDLE;
    pItem->value = GATT_CFG_NO_OPERATION;
  }
}

/*********************************************************************
 * @fn          bloodPressure_HandleConnStatusCB
 *
 * @brief       Simple Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void bloodPressure_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{ 
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) && 
           ( !linkDB_Up( connHandle ) ) ) )
    { 
      bloodPressure_ResetCharCfg( connHandle, bloodPressureMeasConfig );
      bloodPressure_ResetCharCfg( connHandle, bloodPressureIMeasConfig );
    }
  }
}

/*********************************************************************
 * @fn      bloodPressure_WriteCharCfg
 *
 * @brief   Write the client characteristic configuration for a given
 *          client.
 *
 *          Note: Each client has its own instantiation of the Client 
 *                Characteristic Configuration. Writes of the Client
 *                Characteristic Configuration only only affect the 
 *                configuration of that client.
 *
 * @param   connHandle - connection handle.
 * @param   charCfgTbl - client characteristic configuration table.
 * @param   value - attribute new value.
 *
 * @return  Success or Failure
 */
static uint8 bloodPressure_WriteCharCfg( uint16 connHandle, gattCharCfg_t *charCfgTbl,
                                    uint16 value )
{
  gattCharCfg_t *pItem;

  pItem = bloodPressure_FindCharCfgItem( connHandle, charCfgTbl );
  if ( pItem == NULL )
  {
    pItem = bloodPressure_FindCharCfgItem( INVALID_CONNHANDLE, charCfgTbl );
    if ( pItem == NULL )
    {
      return ( ATT_ERR_INSUFFICIENT_RESOURCES );
    }

    pItem->connHandle = connHandle;
  }

  // Write the new value for this client
  pItem->value = value;

  return ( SUCCESS );
}

/*********************************************************************
*********************************************************************/
