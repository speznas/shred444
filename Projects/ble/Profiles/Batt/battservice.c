/**************************************************************************************************
  Filename:       battservice.c
  Revised:        $Date $
  Revision:       $Revision $

  Description:    This file contains the Battery service.

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
#include "hal_adc.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "battservice.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// ADC voltage levels
#define BATT_ADC_LEVEL_3V           409
#define BATT_ADC_LEVEL_2V           273

// Position of level state value in attribute array
#define BATT_MEAS_VALUE_POS         6

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Battery service
CONST uint8 battServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATT_SERVICE_UUID), HI_UINT16(BATT_SERVICE_UUID)
};

// Battery level characteristic
CONST uint8 battLevelUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATT_LEVEL_UUID), HI_UINT16(BATT_LEVEL_UUID)
};

// Battery power state characteristic
CONST uint8 battStateUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATT_POWER_STATE_UUID), HI_UINT16(BATT_POWER_STATE_UUID)
};

// Battery level state characteristic
CONST uint8 battLevelStateUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATT_LEVEL_STATE_UUID), HI_UINT16(BATT_LEVEL_STATE_UUID)
};

// Battery removable characteristic
CONST uint8 battRemovableUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATT_REMOVABLE_UUID), HI_UINT16(BATT_REMOVABLE_UUID)
};

// Battery service required characteristic
CONST uint8 battServiceReqUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(BATT_SERVICE_REQ_UUID), HI_UINT16(BATT_SERVICE_REQ_UUID)
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

// Application callback
static battServiceCB_t battServiceCB;

// Critical battery level setting
static uint8 battCriticalLevel;

/*********************************************************************
 * Profile Attributes - variables
 */

// Battery Service attribute
static CONST gattAttrType_t battService = { ATT_BT_UUID_SIZE, battServUUID };

// Battery level characteristic
static uint8 battLevelProps = GATT_PROP_READ;
static uint8 battLevel = 100;

// Battery power state characteristic
static uint8 battStateProps = GATT_PROP_READ;
static uint8 battState;

// Battery level state characteristic
// Note value is not stored here
static uint8 battLevelStateProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8 battLevelState;
static gattCharCfg_t battLevelStateClientCharCfg = {INVALID_CONNHANDLE, GATT_CFG_NO_OPERATION};

// Battery removable characteristic
static uint8 battRemovableProps = GATT_PROP_READ;
static uint8 battRemovable = BATT_REMOVABLE;

// Battery service required characteristic
static uint8 battServiceReqProps = GATT_PROP_READ;
static uint8 battServiceReq = BATT_SVC_NOT_REQ;

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t battAttrTbl[] = 
{
  // Battery Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&battService                     /* pValue */
  },

    // Battery Level Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &battLevelProps 
    },

      // Battery Level Value
      { 
        { ATT_BT_UUID_SIZE, battLevelUUID },
        GATT_PERMIT_READ, 
        0, 
        &battLevel 
      },

    // Battery State Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &battStateProps 
    },

      // Battery State Value
      { 
        { ATT_BT_UUID_SIZE, battStateUUID },
        GATT_PERMIT_READ, 
        0, 
        &battState 
      },

    // Battery Level State Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &battLevelStateProps 
    },

      // Battery Level State Value
      { 
        { ATT_BT_UUID_SIZE, battLevelStateUUID },
        GATT_PERMIT_READ, 
        0, 
        &battLevelState 
      },

      // Battery Level State Client Characteristic Configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *) &battLevelStateClientCharCfg 
      },      

    // Battery Removable Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &battRemovableProps 
    },

      // Battery Removable Value
      { 
        { ATT_BT_UUID_SIZE, battRemovableUUID },
        GATT_PERMIT_READ, 
        0, 
        &battRemovable 
      },

    // Battery Service Required Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &battServiceReqProps 
    },

      // Battery Service Required Value
      { 
        { ATT_BT_UUID_SIZE, battServiceReqUUID },
        GATT_PERMIT_READ, 
        0, 
        &battServiceReq 
      }
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 battReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                             uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t battWriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                  uint8 *pValue, uint8 len, uint16 offset );
static void battNotifyCB( linkDBItem_t *pLinkItem );
static uint8 battMeasure( void );
static void battNotifyLevelState( void );

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Batt_AddService
 *
 * @brief   Initializes the Battery Service by registering
 *          GATT attributes with the GATT server.
 *
 * @return  Success or Failure
 */
bStatus_t Batt_AddService( void )
{
  uint8 status = SUCCESS;

  // Register GATT attribute list and CBs with GATT Server App
  status = GATTServApp_RegisterService( battAttrTbl, GATT_NUM_ATTRS( battAttrTbl ),
                                        battReadAttrCB, battWriteAttrCB, NULL );

  return ( status );
}

/*********************************************************************
 * @fn      Batt_Register
 *
 * @brief   Register a callback function with the Battery Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
extern void Batt_Register( battServiceCB_t pfnServiceCB )
{
  battServiceCB = pfnServiceCB;
}

/*********************************************************************
 * @fn      Batt_SetParameter
 *
 * @brief   Set a Battery Service parameter.
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
bStatus_t Batt_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
    
  switch ( param )
  {
    case BATT_PARAM_STATE:
      // If state changed notify it
      if (battState != *((uint8*)value))
      {
        battState = *((uint8*)value);
        battNotifyLevelState();
      }
      break;      

    case BATT_PARAM_REMOVABLE:
      battRemovable = *((uint8*)value);
      break;

    case BATT_PARAM_SERVICE_REQ:
      battServiceReq = *((uint8*)value);
      break;

    case BATT_PARAM_LVL_ST_CHAR_CFG:
      battLevelStateClientCharCfg.value = *((uint16*)value);
      break;

    case BATT_PARAM_CRITICAL_LEVEL:
      battCriticalLevel = *((uint8*)value);
      
      // If below the critical level and critical state not set, notify it
      if (battLevel < battCriticalLevel &&
          ((battState & BATT_FLAGS_CR_CRIT) != BATT_FLAGS_CR_CRIT))
      {
        battState |= BATT_FLAGS_CR_CRIT;
        battNotifyLevelState();
      }
      break;      

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      Batt_GetParameter
 *
 * @brief   Get a Battery Service parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Batt_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case BATT_PARAM_LEVEL:
      *((uint8*)value) = battLevel;
      break;      

    case BATT_PARAM_STATE:
      *((uint8*)value) = battState;
      break;
      
    case BATT_PARAM_REMOVABLE:
      *((uint8*)value) = battRemovable;
      break;  

    case BATT_PARAM_SERVICE_REQ:
      *((uint8*)value) = battServiceReq;
      break;      

    case BATT_PARAM_LVL_ST_CHAR_CFG:
      *((uint16*)value) = battLevelStateClientCharCfg.value;
      break;      

    case BATT_PARAM_CRITICAL_LEVEL:
      *((uint8*)value) = battCriticalLevel;
      break;      

    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          Batt_MeasLevel
 *
 * @brief       Measure the battery level and update the battery
 *              level value in the service characteristics.  If
 *              the battery level-state characteristic is configured
 *              for notification and the battery level has changed
 *              since the last measurement, then a notification
 *              will be sent.
 *
 * @return      Success
 */
bStatus_t Batt_MeasLevel( void )
{
  uint8 level;
  
  level = battMeasure();
  
  // If level has gone down
  if (level < battLevel)
  {
    // Update level
    battLevel = level;

    // check for critical level
    if (level < battCriticalLevel)
    {
      battState |= BATT_FLAGS_CR_CRIT;
    }
          
    // Send a notification
    battNotifyLevelState();
  }  
  
  return SUCCESS;
}
                               
/*********************************************************************
 * @fn          battReadAttrCB
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
static uint8 battReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                             uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1] );

  // These are one byte so handle them together
  if ( uuid == BATT_REMOVABLE_UUID ||
       uuid == BATT_SERVICE_REQ_UUID )
  {
    *pLen = 1;
    pValue[0] = *pAttr->pValue;
  }
  // Measure battery level if reading these
  else if ( uuid == BATT_LEVEL_UUID ||
            uuid == BATT_POWER_STATE_UUID )
  {
    uint8 level;
    
    level = battMeasure();
    
    // If level has gone down
    if (level < battLevel)
    {
      // Update level
      battLevel = level;
  
      // check for critical level
      if (level < battCriticalLevel)
      {
        battState |= BATT_FLAGS_CR_CRIT;
      }
    }
    
    if (uuid == BATT_LEVEL_UUID)
    {
      *pLen = 1;
      pValue[0] = battLevel;
    }
    else
    {
      *pLen = 1;
      pValue[0] = battState;
    }      
  }
  else if ( uuid == BATT_LEVEL_STATE_UUID )
  {
    *pLen = 2;
    pValue[0] = battLevel;
    pValue[1] = battState;
  }
  else if ( uuid == GATT_CLIENT_CHAR_CFG_UUID )
  {
    *pLen = 2;
    pValue[0] = LO_UINT16( battLevelStateClientCharCfg.value );
    pValue[1] = HI_UINT16( battLevelStateClientCharCfg.value );
  }
  else
  {
    status = ATT_ERR_ATTR_NOT_FOUND;
  }
  
  return ( status );
}

/*********************************************************************
 * @fn      battWriteAttrCB
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
static bStatus_t battWriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                  uint8 *pValue, uint8 len, uint16 offset )
{
  bStatus_t status = SUCCESS;

  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);

  // Only one writeable attribute
  if (uuid == GATT_CLIENT_CHAR_CFG_UUID)
  {
    if ( len == 2 )
    {
      uint16 charCfg = BUILD_UINT16( pValue[0], pValue[1] );
      
      // Validate characteristic configuration bit field
      if ( charCfg == GATT_CLIENT_CFG_NOTIFY ||
           charCfg == GATT_CFG_NO_OPERATION )
      {
        battLevelStateClientCharCfg.connHandle = connHandle;
        battLevelStateClientCharCfg.value = charCfg;
        
        (*battServiceCB)((charCfg == 0) ? BATT_LEVEL_NOTI_DISABLED :
                                          BATT_LEVEL_NOTI_ENABLED);
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
 * @fn          battNotifyCB
 *
 * @brief       Send a notification of the level state characteristic.
 *
 * @param       connHandle - linkDB item
 *
 * @return      None.
 */
static void battNotifyCB( linkDBItem_t *pLinkItem )
{
  attHandleValueNoti_t noti;
  
  if (pLinkItem->stateFlags & LINK_CONNECTED)
  {
    noti.handle = battAttrTbl[BATT_MEAS_VALUE_POS].handle;
    noti.len = 2;
    noti.value[0] = battLevel;    
    noti.value[1] = battState;
    
    GATT_Notification( pLinkItem->connectionHandle, &noti, FALSE );
  }
}

/*********************************************************************
 * @fn      battMeasure
 *
 * @brief   Measure the battery level with the ADC and return
 *          it as a percentage 0-100%.
 *
 * @return  Battery level.
 */
static uint8 battMeasure( void )
{
  uint16 adc;
  uint8 percent;
  
  /**
   * Battery level conversion from ADC to a percentage:
   *
   * The maximum ADC value for the battery voltage level is 511 for a
   * 10-bit conversion.  The ADC value is references vs. 1.25v and
   * this maximum value corresponds to a voltage of 3.75v.
   *
   * For a coin cell battery 3.0v = 100%.  The minimum operating
   * voltage of the CC2540 is 2.0v so 2.0v = 0%.
   *
   * To convert a voltage to an ADC value use:
   *
   *   (v/3)/1.25 * 511 = adc
   *   
   * 3.0v = 409 ADC
   * 2.0v = 273 ADC
   *
   * We need to map ADC values from 409-273 to 100%-0%.
   *
   * Normalize the ADC values to zero:
   *
   * 409 - 273 = 136
   *
   * And convert ADC range to percentage range:
   *
   * percent/adc = 100/136 = 25/34
   *
   * Resulting in the final equation, with round:
   *
   * percent = ((adc - 273) * 25) + 33 / 34
   */
 
  // Configure ADC and perform a read
  HalAdcSetReference( HAL_ADC_REF_125V );
  adc = HalAdcRead( HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_10 );
  
  if (adc >= BATT_ADC_LEVEL_3V)
  {
    percent = 100;
  }
  else if (adc <= BATT_ADC_LEVEL_2V)
  {
    percent = 0;
  }
  else
  {
    percent = (uint8) ((((adc - BATT_ADC_LEVEL_2V) * 25) + 33) / 34);
  }
  
  return percent;
}

/*********************************************************************
 * @fn      battNotifyLevelState
 *
 * @brief   Send a notification of the battery level state
 *          characteristic if a connection is established.
 *
 * @return  None.
 */
static void battNotifyLevelState( void )
{
  // If are notifications enabled send a notification
  if (battLevelStateClientCharCfg.value == GATT_CLIENT_CFG_NOTIFY)
  {
    // Execute linkDB callback to send notification
    linkDB_PerformFunc( battNotifyCB );
  }
  
}

/*********************************************************************
*********************************************************************/
