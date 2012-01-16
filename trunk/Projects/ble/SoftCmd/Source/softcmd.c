/**************************************************************************************************
  Filename:       softcmd.c
  Revised:        $Date $
  Revision:       $Revision $

  Description:    This file contains the Soft Command sample application 
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
#include "OnBoard.h"
#include "hal_led.h"
#include "hal_key.h"
#include "gatt.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "linkdb.h"
#include "cmdenumservice.h"
#include "battservice.h"
#include "peripheral.h"
#include "gapbondmgr.h"
#include "softcmd.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         FALSE

// Minimum connection interval (units of 1.25ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     200

// Maximum connection interval (units of 1.25ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     1600

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         1

// Supervision timeout value (units of 10ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          1000

// Default passcode
#define DEFAULT_PASSCODE                      19655

// Default GAP pairing mode
#define DEFAULT_PAIRING_MODE                  GAPBOND_PAIRING_MODE_WAIT_FOR_REQ

// Default MITM mode (TRUE to require passcode or OOB when pairing)
#define DEFAULT_MITM_MODE                     FALSE

// Default bonding mode, TRUE to bond
#define DEFAULT_BONDING_MODE                  TRUE

// Default GAP bonding I/O capabilities
#define DEFAULT_IO_CAPABILITIES               GAPBOND_IO_CAP_DISPLAY_ONLY

// Delay to begin discovery from start of connection in ms
#define DEFAULT_DISCOVERY_DELAY               1000

// Battery level is critical when it is less than this %
#define DEFAULT_BATT_CRITICAL_LEVEL           6 

// Battery state
#define DEFAULT_BATT_STATE                    (BATT_FLAGS_PR_PRES | \
                                               BATT_FLAGS_DIS_DISCH | \
                                               BATT_FLAGS_CH_NOT_ABLE | \
                                               BATT_FLAGS_CR_GOOD)

// Battery measurement period in ms
#define DEFAULT_BATT_PERIOD                   15000

// UUIDs for generic control service
// These UUIDs are currently temporary placeholders
#define GEN_CTRL_SERVICE_UUID           0xF011  // Generic Control Service
#define CMD_ENUM_UUID                   0xE010  // Command Enumeration characteristic
#define GEN_CMD_UUID                    0xE011  // Generic Command Control Point

// Length of Characteristic declaration + handle with 16 bit UUID
#define CHAR_DESC_HDL_UUID16_LEN              7

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// Task ID for internal task/event processing
static uint8 softCmdTaskId;

// GAP state
static gaprole_States_t softCmdGapState = GAPROLE_INIT;

// Service discovery state
static uint8 softCmdDiscState = DISC_IDLE;

// Service discovery complete
static uint8 softCmdDiscoveryCmpl = FALSE;

// TRUE if pairing started
static uint8 softCmdPairingStarted = FALSE;

// TRUE if discovery postponed due to pairing
static uint8 softCmdDiscPostponed = FALSE;

// Attribute handles used during discovery
static uint16 softCmdSvcStartHdl;
static uint16 softCmdSvcEndHdl;

// Attribute andle cache
static uint16 softCmdHdlCache[HDL_CACHE_LEN];

// Bonded state
static bool softCmdBonded = FALSE;

// Bonded peer address
static uint8 softCmdBondedAddr[B_ADDR_LEN];

// Scan response data
static uint8 softCmdScanData[] =
{
  0x0D,   // length of this data
  GAP_ADTYPE_LOCAL_NAME_COMPLETE,
  'S',
  'o',
  'f',
  't',
  ' ',
  'C',
  'o',
  'm',
  'm',
  'a',
  'n',
  'd'
};

// Advertising data
static uint8 softCmdAdvData[] = 
{ 
  // flags
  0x02,
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED
};

// Device name attribute value
static uint8 softCmdDeviceName[GAP_DEVICE_NAME_LEN] = "Command Trigger";

// GAP connection handle
static uint16 softCmdConnHandle;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void softCmd_ProcessOSALMsg( osal_event_hdr_t *pMsg );
static void softCmdGapStateCB( gaprole_States_t newState );
static void softCmdPairStateCB( uint16 connHandle, uint8 state, uint8 status );
static void softCmdBattPeriodicTask( void );
static void softCmd_HandleKeys( uint8 shift, uint8 keys );
static void softCmdBattCB(uint8 event);
static void softCmdProcessGattMsg( gattMsgEvent_t *pMsg );
static void softCmdDisconnected( void );
static uint8 softCmdDiscStart( void );
static uint8 softCmdDiscGenCtrl( uint8 state, gattMsgEvent_t *pMsg );
static void softCmdSend( uint8 cmd );

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t softCmdPeripheralCBs =
{
  softCmdGapStateCB,      // Profile State Change Callbacks
  NULL                    // When a valid RSSI is read from controller
};

// Bond Manager Callbacks
static const gapBondCBs_t softCmdBondCB =
{
  NULL,                   // Passcode callback
  softCmdPairStateCB      // Pairing state callback
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SoftCmd_Init
 *
 * @brief   Initialization function for the Soft Command App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void SoftCmd_Init( uint8 task_id )
{
  softCmdTaskId = task_id;

  // Setup the GAP Peripheral Role Profile
  {
    uint8 advEnable = FALSE;      
    uint16 advertOffTime = 0;
    uint8 updateRequest = DEFAULT_ENABLE_UPDATE_REQUEST;
    uint16 minInterval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
    uint16 maxInterval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
    uint16 slaveLatency = DEFAULT_DESIRED_SLAVE_LATENCY;
    uint16 connTimeout = DEFAULT_DESIRED_CONN_TIMEOUT;

    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &advEnable );
    GAPRole_SetParameter( GAPROLE_ADVERT_OFF_TIME, sizeof( uint16 ), &advertOffTime );
    
    GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &updateRequest );
    GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &minInterval );
    GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &maxInterval );
    GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &slaveLatency );
    GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &connTimeout );
    
    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( softCmdScanData ), softCmdScanData );
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( softCmdAdvData ), softCmdAdvData );
  }
  
  // Set the GAP Characteristics
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, softCmdDeviceName );

  // Setup the GAP Bond Manager
  {
    uint32 passkey = DEFAULT_PASSCODE;
    uint8 pairMode = DEFAULT_PAIRING_MODE;
    uint8 mitm = DEFAULT_MITM_MODE;
    uint8 ioCap = DEFAULT_IO_CAPABILITIES;
    uint8 bonding = DEFAULT_BONDING_MODE;
    GAPBondMgr_SetParameter( GAPBOND_DEFAULT_PASSCODE, sizeof( uint32 ), &passkey );
    GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE, sizeof( uint8 ), &pairMode );
    GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION, sizeof( uint8 ), &mitm );
    GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES, sizeof( uint8 ), &ioCap );
    GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED, sizeof( uint8 ), &bonding );
  }  

  // Setup Battery Characteristic Values
  {
    uint8 critical = DEFAULT_BATT_CRITICAL_LEVEL;
    uint8 state = DEFAULT_BATT_STATE;
    Batt_SetParameter( BATT_PARAM_CRITICAL_LEVEL, sizeof (uint8 ), &critical );
    Batt_SetParameter( BATT_PARAM_STATE, sizeof (uint8 ), &state );    
  }

  // Initialize GATT Client
  VOID GATT_InitClient();
  
  // Initialize Services
  GGS_AddService( GATT_ALL_SERVICES );
  GATTServApp_AddService( GATT_ALL_SERVICES );
  CmdEnum_AddService( );
  Batt_AddService( );
  
  // Register for Battery service callback;
  Batt_Register( softCmdBattCB );
  
  // Register for all key events - This app will handle all key events
  RegisterForKeys( softCmdTaskId );
  
  // makes sure LEDs are off
  HalLedSet( (HAL_LED_1 | HAL_LED_2), HAL_LED_MODE_OFF );
  
  // For keyfob board set GPIO pins into a power-optimized state
  // Note that there is still some leakage current from the buzzer,
  // accelerometer, LEDs, and buttons on the PCB.
  
  P0SEL = 0; // Configure Port 0 as GPIO
  P1SEL = 0; // Configure Port 1 as GPIO
  P2SEL = 0; // Configure Port 2 as GPIO

  P0DIR = 0xFC; // Port 0 pins P0.0 and P0.1 as input (buttons),
                // all others (P0.2-P0.7) as output
  P1DIR = 0xFF; // All port 1 pins (P1.0-P1.7) as output
  P2DIR = 0x1F; // All port 1 pins (P2.0-P2.4) as output
  
  P0 = 0x03; // All pins on port 0 to low except for P0.0 and P0.1 (buttons)
  P1 = 0;   // All pins on port 1 to low
  P2 = 0;   // All pins on port 2 to low  
  
  // Setup a delayed profile startup
  osal_set_event( softCmdTaskId, START_DEVICE_EVT );
}

/*********************************************************************
 * @fn      SoftCmd_ProcessEvent
 *
 * @brief   Soft Command Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 SoftCmd_ProcessEvent( uint8 task_id, uint16 events )
{
  
  VOID task_id; // OSAL required parameter that isn't used in this function
  
  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( softCmdTaskId )) != NULL )
    {
      softCmd_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & START_DEVICE_EVT )
  {
    // Start the Device
    VOID GAPRole_StartDevice( &softCmdPeripheralCBs );

    // Register with bond manager after starting device
    GAPBondMgr_Register( (gapBondCBs_t *) &softCmdBondCB );
    
    return ( events ^ START_DEVICE_EVT );
  }

  if ( events & START_DISCOVERY_EVT )
  {
    if ( softCmdPairingStarted )
    {
      // Postpone discovery until pairing completes
      softCmdDiscPostponed = TRUE;
    }
    else
    {
      softCmdDiscState = softCmdDiscStart();
    }  
    return ( events ^ START_DISCOVERY_EVT );
  }

  if ( events & BATT_PERIODIC_EVT )
  {
    // Perform periodic battery task
    softCmdBattPeriodicTask();
    
    return (events ^ BATT_PERIODIC_EVT);
  }  
  
  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      softCmd_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void softCmd_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    case KEY_CHANGE:
      softCmd_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
      break;
      
    case GATT_MSG_EVENT:
      softCmdProcessGattMsg( (gattMsgEvent_t *) pMsg );
      break;
      
    default:
      break;
  }
}

/*********************************************************************
 * @fn      softCmd_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void softCmd_HandleKeys( uint8 shift, uint8 keys )
{
  if ( keys & HAL_KEY_SW_1 )
  {
    // if in a connection send a soft key
    if ( softCmdGapState == GAPROLE_CONNECTED )
    {
      softCmdSend( CMD_ENUM_SOFT_CMD_0 );
    }
  }
  
  if ( keys & HAL_KEY_SW_2 )
  {
    // if not in a connection, toggle advertising on and off
    if( softCmdGapState != GAPROLE_CONNECTED )
    {
      uint8 status;
      
      // toggle GAP advertisement status
      GAPRole_GetParameter( GAPROLE_ADVERT_ENABLED, &status );
      status = !status;
      GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &status );   
    }
    // if in a connection send a soft key
    else
    {
      softCmdSend( CMD_ENUM_SOFT_CMD_1 );
    }
  }
}

/*********************************************************************
 * @fn      softCmdProcessGattMsg
 *
 * @brief   Process GATT messages
 *
 * @return  none
 */
static void softCmdProcessGattMsg( gattMsgEvent_t *pMsg )
{
  softCmdDiscState = softCmdDiscGenCtrl( softCmdDiscState, pMsg );
  if ( softCmdDiscState == DISC_IDLE )
  {
    softCmdDiscoveryCmpl = TRUE;
  }
}

/*********************************************************************
 * @fn      softCmdDisconnected
 *
 * @brief   Handle disconnect. 
 *
 * @return  none
 */
static void softCmdDisconnected( void )
{
  // Initialize state variables
  softCmdDiscState = DISC_IDLE;
  softCmdPairingStarted = FALSE;
  softCmdDiscPostponed = FALSE;
}

/*********************************************************************
 * @fn      softCmdGapStateCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void softCmdGapStateCB( gaprole_States_t newState )
{
  // if connected
  if (newState == GAPROLE_CONNECTED)
  {
    linkDBItem_t  *pItem;

    // Get connection handle
    GAPRole_GetParameter( GAPROLE_CONNHANDLE, &softCmdConnHandle );

    // Get peer bd address
    if ( (pItem = linkDB_Find( softCmdConnHandle )) != NULL)
    {
      // If connected to device without bond do service discovery
      if ( !osal_memcmp( pItem->addr, softCmdBondedAddr, B_ADDR_LEN ) )
      {
        softCmdDiscoveryCmpl = FALSE;
      }
      
      // Initiate service discovery if necessary
      if ( softCmdDiscoveryCmpl == FALSE )
      {
        osal_start_timerEx( softCmdTaskId, START_DISCOVERY_EVT, DEFAULT_DISCOVERY_DELAY );
      }
    }      
  }
  
  // if disconnected
  else if (softCmdGapState == GAPROLE_CONNECTED && 
           newState != GAPROLE_CONNECTED)
  {
    softCmdDisconnected();
    
    // reset client characteristic configuration descriptors
    uint16 param = GATT_CFG_NO_OPERATION;
    Batt_SetParameter(BATT_PARAM_LVL_ST_CHAR_CFG, sizeof(uint16), (uint8 *) &param);
  }    
  
  softCmdGapState = newState;
}

/*********************************************************************
 * @fn      pairStateCB
 *
 * @brief   Pairing state callback.
 *
 * @return  none
 */
static void softCmdPairStateCB( uint16 connHandle, uint8 state, uint8 status )
{
  if ( state == GAPBOND_PAIRING_STATE_STARTED )
  {
    softCmdPairingStarted = TRUE;
  }
  else if ( state == GAPBOND_PAIRING_STATE_COMPLETE )
  {
    softCmdPairingStarted = FALSE;

    if ( status == SUCCESS )
    {
      linkDBItem_t  *pItem;
      
      if ( (pItem = linkDB_Find( softCmdConnHandle )) != NULL )
      {
        // Store bonding state of pairing
        softCmdBonded = ( (pItem->stateFlags & LINK_BOUND) == LINK_BOUND );
        
        if ( softCmdBonded )
        {
          osal_memcpy( softCmdBondedAddr, pItem->addr, B_ADDR_LEN );
        }
      }
      
      // If discovery was postponed start discovery
      if ( softCmdDiscPostponed && softCmdDiscoveryCmpl == FALSE )
      {
        softCmdDiscPostponed = FALSE;
        osal_set_event( softCmdTaskId, START_DISCOVERY_EVT );
      }
    }
  }
}

/*********************************************************************
 * @fn      softCmdBattCB
 *
 * @brief   Callback function for battery service.
 *
 * @param   event - service event
 *
 * @return  none
 */
static void softCmdBattCB(uint8 event)
{
  if (event == BATT_LEVEL_NOTI_ENABLED)
  {
    // if connected start periodic measurement
    if (softCmdGapState == GAPROLE_CONNECTED)
    {
      osal_start_timerEx( softCmdTaskId, BATT_PERIODIC_EVT, DEFAULT_BATT_PERIOD );
    } 
  }
  else if (event == BATT_LEVEL_NOTI_DISABLED)
  {
    // stop periodic measurement
    osal_stop_timerEx( softCmdTaskId, BATT_PERIODIC_EVT );
  }
}

/*********************************************************************
 * @fn      softCmdBattPeriodicTask
 *
 * @brief   Perform a periodic task for battery measurement.
 *
 * @param   none
 *
 * @return  none
 */
static void softCmdBattPeriodicTask( void )
{
  if (softCmdGapState == GAPROLE_CONNECTED)
  {
    // perform battery level check
    Batt_MeasLevel( );
    
    // Restart timer
    osal_start_timerEx( softCmdTaskId, BATT_PERIODIC_EVT, DEFAULT_BATT_PERIOD );
  }
}

/*********************************************************************
 * @fn      softCmdDiscStart()
 *
 * @brief   Start service discovery. 
 *
 *
 * @return  New discovery state.
 */
static uint8 softCmdDiscStart( void )
{
  // Clear handle cache
  osal_memset( softCmdHdlCache, 0, sizeof(softCmdHdlCache) );
  
  // Start discovery with first service
  return softCmdDiscGenCtrl( DISC_GEN_CTRL_START, NULL );
}

/*********************************************************************
 * @fn      softCmdDiscGenCtrl()
 *
 * @brief   Generic control service and characteristic discovery. 
 *
 * @param   state - Discovery state.
 * @param   pMsg - GATT message.
 *
 * @return  New discovery state.
 */
static uint8 softCmdDiscGenCtrl( uint8 state, gattMsgEvent_t *pMsg )
{
  uint8 newState = state;
  
  switch ( state )
  {
    case DISC_GEN_CTRL_START:  
      {
        uint8 uuid[ATT_BT_UUID_SIZE] = { LO_UINT16(GEN_CTRL_SERVICE_UUID),
                                         HI_UINT16(GEN_CTRL_SERVICE_UUID) };

        // Initialize service discovery variables
        softCmdSvcStartHdl = softCmdSvcEndHdl = 0;
        
        // Discover service by UUID
        GATT_DiscPrimaryServiceByUUID( softCmdConnHandle, uuid,
                                       ATT_BT_UUID_SIZE, softCmdTaskId );      

        newState = DISC_GEN_CTRL_SVC;
      } 
      break;

    case DISC_GEN_CTRL_SVC:
      // Service found, store handles
      if ( pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
           pMsg->msg.findByTypeValueRsp.numInfo > 0 )
      {
        softCmdSvcStartHdl = pMsg->msg.findByTypeValueRsp.handlesInfo[0].handle;
        softCmdSvcEndHdl = pMsg->msg.findByTypeValueRsp.handlesInfo[0].grpEndHandle;
      }
      
      // If procedure complete
      if ( ( pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP  && 
             pMsg->hdr.status == bleProcedureComplete ) ||
           ( pMsg->method == ATT_ERROR_RSP ) )
      {
        // If service found
        if ( softCmdSvcStartHdl != 0 )
        {
          // Discover all characteristics
          GATT_DiscAllChars( softCmdConnHandle, softCmdSvcStartHdl,
                             softCmdSvcEndHdl, softCmdTaskId );
          
          newState = DISC_GEN_CTRL_CHAR;
        }
        else
        {
          // Service not found
          newState = DISC_FAILED;
        }
      }    
      break;

    case DISC_GEN_CTRL_CHAR:
      {
        uint8   i;
        uint8   *p;
        uint16  handle;
        uint16  uuid;
        
        // Characteristics found
        if ( pMsg->method == ATT_READ_BY_TYPE_RSP &&
             pMsg->msg.readByTypeRsp.numPairs > 0 && 
             pMsg->msg.readByTypeRsp.len == CHAR_DESC_HDL_UUID16_LEN )
        {
          // For each characteristic declaration
          p = pMsg->msg.readByTypeRsp.dataList;
          for ( i = pMsg->msg.readByTypeRsp.numPairs; i > 0; i-- )
          {
            // Parse characteristic declaration
            handle = BUILD_UINT16(p[3], p[4]);
            uuid = BUILD_UINT16(p[5], p[6]);
                   
            // If UUID is of interest, store handle
            switch ( uuid )
            {
              case GEN_CMD_UUID:
                softCmdHdlCache[HDL_GEN_CTRL_GEN_CMD] = handle;
                break;

              case CMD_ENUM_UUID:
                softCmdHdlCache[HDL_GEN_CTRL_CMD_ENUM] = handle;
                break;
                
              default:
                break;
            }
            
            p += CHAR_DESC_HDL_UUID16_LEN;
          }
          
        }
          
        // If procedure complete
        if ( ( pMsg->method == ATT_READ_BY_TYPE_RSP  && 
               pMsg->hdr.status == bleProcedureComplete ) ||
             ( pMsg->method == ATT_ERROR_RSP ) )
        {
          // If didn't find Generic Command then fail
          if ( softCmdHdlCache[HDL_GEN_CTRL_GEN_CMD] == 0 )
          {
            newState = DISC_FAILED;
          }
          // If found Command Enumeration read it
          else if ( softCmdHdlCache[HDL_GEN_CTRL_CMD_ENUM] != 0 )
          {
            attReadReq_t  readReq;

            readReq.handle = softCmdHdlCache[HDL_GEN_CTRL_CMD_ENUM];
            GATT_ReadCharValue( softCmdConnHandle, &readReq, softCmdTaskId );
            
            newState = DISC_GEN_CTRL_READ_ENUM;
          }
          else
          {
            newState = DISC_IDLE;
          }
        }
      }      
      break;

    case DISC_GEN_CTRL_READ_ENUM:
      // Currently nothing is done when this characteristic is read
      newState = DISC_IDLE;
      break;
      
    default:
      break;
  }

  return newState;
}

/*********************************************************************
 * @fn      softCmdSend
 *
 * @brief   Send a soft command to peer
 *
 * @param   cmd - Command to send
 *
 * @return  status
 */
static void softCmdSend( uint8 cmd )
{
  attWriteReq_t req;

  if ( softCmdHdlCache[HDL_GEN_CTRL_CMD_ENUM] != 0 )
  {
    // Send write command
    req.len = 1;
    req.value[0] = cmd;
    req.sig = 0;
    req.cmd = 1;
    req.handle = softCmdHdlCache[HDL_GEN_CTRL_CMD_ENUM];
    GATT_WriteNoRsp( softCmdConnHandle, &req );
  }
}

/*********************************************************************
*********************************************************************/
