/**************************************************************************************************
  Filename:       hidKeyboardprofile.c
  Revised:        $Date: 2010-08-06 08:56:11 -0700 (Fri, 06 Aug 2010) $
  Revision:       $Revision: 23333 $

  Description:    This file contains the HID Keyboard profile sample GATT service 
                  profile for use with the BLE sample application.

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
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"

#include "hidKeyboardProfile.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED        14

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Simple GATT Profile Service UUID: 0xFFF0
CONST uint8 hidKeyboardServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HIDKEYBOARD_SERV_UUID), HI_UINT16(HIDKEYBOARD_SERV_UUID)
};

// Characteristic 1 UUID: 0xFFF1
CONST uint8 hidKeyboardDataUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(HIDKEYBOARD_DATA_UUID), HI_UINT16(HIDKEYBOARD_DATA_UUID)
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

static uint16 hidKeyboardConnHandle = 0;

/*********************************************************************
 * Profile Attributes - variables
 */

// HID Keyboard Service attribute
static CONST gattAttrType_t hidKeyboardService = { ATT_BT_UUID_SIZE, hidKeyboardServUUID };

// HID Keyboard Data Characteristic Properties
static uint8 hidKeyboardDataCharProps = GATT_PROP_NOTIFY;

// HID Keyboard Data Characteristic Value
// First byte represesnts whether key was pressed or released
// Second byte tells the key represented
// Third byte indicates if any modifier keys were pressed (shift, etc.)
static uint8 hidKeyboardData[3] = { 0x00, 0x00, 0x00 };

// HID Keyboard Data Characteristic Configuration
// (Default is to have notifications ENABLED)
static uint16 hidKeyboardDataCharConfig = GATT_CLIENT_CFG_NOTIFY;
//static uint16 hidKeyboardDataCharConfig = GATT_CFG_NO_OPERATION;

// HID Keyboard Data Characteristic User Description
static uint8 hidKeyboardCharDataUserDesp[14] = "Keyboard Data\0";

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t hidKeyboardAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = 
{
  // Simple Profile Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&hidKeyboardService            /* pValue */
  },

    // Keyboard Data Characteristic Declaration
    { 
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ, 
      0,
      &hidKeyboardDataCharProps 
    },

      // Keyboard DataCharacteristic Value
      { 
        { ATT_BT_UUID_SIZE, hidKeyboardDataUUID },
        0, 
        0, 
        hidKeyboardData 
      },

      // Keyboard Data Characteristic configuration
      { 
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
        0, 
        (uint8 *)&hidKeyboardDataCharConfig 
      },
      
      // Keyboard Data Characteristic User Description
      { 
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ, 
        0, 
        hidKeyboardCharDataUserDesp 
      },

};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 hidKeyboard_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen );
static bStatus_t hidKeyboard_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset );

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      HidKeyboard_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t HidKeyboard_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  if ( services & HIDKEYBOARD_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( hidKeyboardAttrTbl, GATT_NUM_ATTRS( hidKeyboardAttrTbl ),
                                          hidKeyboard_ReadAttrCB, hidKeyboard_WriteAttrCB, NULL );
  }

  return ( status );
}

/*********************************************************************
 * @fn      HidKeyboard_SetParameter
 *
 * @brief   Set a Simple Profile parameter.
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
bStatus_t HidKeyboard_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  const uint8 KEYPRESS_LEN = 3;
  
  switch ( param )
  {
    case HIDKEYBOARD_DATA:
      if ( len == KEYPRESS_LEN ) 
      {
        osal_memcpy( hidKeyboardData, value, KEYPRESS_LEN );
        
        // Send notification if enabled
        if ( ( hidKeyboardDataCharConfig == GATT_CLIENT_CFG_NOTIFY ) && 
             ( hidKeyboardConnHandle != INVALID_CONNHANDLE ) )
        {
          gattAttribute_t *attr;
      
          // Find the characteristic value attribute 
          attr = GATTServApp_FindAttr( hidKeyboardAttrTbl, 
                                       GATT_NUM_ATTRS( hidKeyboardAttrTbl ), 
                                       hidKeyboardData );
          if ( attr != NULL )
          {
            attHandleValueNoti_t notify;
          
            // Send the notification
            notify.handle = attr->handle;
            notify.len = KEYPRESS_LEN;
            osal_memcpy( notify.value, hidKeyboardData, KEYPRESS_LEN );

            ret = GATT_Notification( hidKeyboardConnHandle, &notify, FALSE );
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
 * @fn          hidKeyboard_ReadAttrCB
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
static uint8 hidKeyboard_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                            uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen )
{
  // Should never get here! (characteristic does not have read permissions)
  return ( ATT_ERR_ATTR_NOT_FOUND );
  
}

/*********************************************************************
 * @fn      hidKeyboard_WriteAttrCB
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
static bStatus_t hidKeyboard_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                 uint8 *pValue, uint8 len, uint16 offset )
{
  // Should never get here! (characteristic does does not have write permissions)
  return ( ATT_ERR_ATTR_NOT_FOUND );
}

/*********************************************************************
*********************************************************************/
