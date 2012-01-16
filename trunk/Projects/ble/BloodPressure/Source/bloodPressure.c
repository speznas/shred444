/**************************************************************************************************
  Filename:       bloodPressure.c

  Revised:        $Date: 2011-07-06 08:36:31 -0700 (Wed, 06 Jul 2011) $
  Revision:       $Revision: 26609 $

  Description:    This file contains the Bloodpressure sample application 
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
#include "OSAL_PwrMgr.h"
#include "OnBoard.h"
#include "hal_adc.h"
#include "hal_led.h"
#include "hal_key.h"
#include "gatt.h"
#include "hci.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "linkdb.h"
#include "peripheral.h"
#include "gapbondmgr.h"
#include "bpservice.h"
#include "devinfoservice.h"
#include "bloodPressure.h"
#include "timeapp.h"
#include "OSAL_Clock.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// How often to perform periodic event
#define PERIODIC_EVT_PERIOD                   1000

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

// Some values used to simulate measurements
#define FLAGS_IDX_MAX                         7      //3 flags c/f -- timestamp -- site

// Length of bd addr as a string
#define B_ADDR_STR_LEN                        15

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         FALSE

// Default GAP pairing mode
#define DEFAULT_PAIRING_MODE                  GAPBOND_PAIRING_MODE_INITIATE

// Default MITM mode (TRUE to require passcode or OOB when pairing)
#define DEFAULT_MITM_MODE                     FALSE

// Default bonding mode, TRUE to bond
#define DEFAULT_BONDING_MODE                  TRUE

// Default GAP bonding I/O capabilities
#define DEFAULT_IO_CAPABILITIES               GAPBOND_IO_CAP_DISPLAY_ONLY

// Delay to begin discovery from start of connection in ms
#define DEFAULT_DISCOVERY_DELAY               1000

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Task ID
uint8 bloodPressureTaskId;

// Connection handle
uint16 gapConnHandle;

uint8 timeConfigDone;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// GAP State
static gaprole_States_t gapProfileState = GAPROLE_INIT;

// Service discovery state
static uint8 timeAppDiscState = DISC_IDLE;

// Service discovery complete
static uint8 timeAppDiscoveryCmpl = FALSE;

// Characteristic configuration state
static uint8 timeAppConfigState = TIMEAPP_CONFIG_START;

// TRUE if pairing started
static uint8 timeAppPairingStarted = FALSE;

// TRUE if discovery postponed due to pairing
static uint8 timeAppDiscPostponed = FALSE;


// GAP Profile - Name attribute for SCAN RSP data
static uint8 scanData[] =
{
  0x12,   // length of this data
  0x09,   // AD Type = Complete local name
  'B',
  'l',
  'o',
  'o',
  'd',
  'P',
  'r',
  'e',
  's',
  's',
  'u',
  'r',
  'e'
};

static uint8 advertData[] = 
{ 
  0x02,   // length of this data
  0x01,   // AD Type = Flags
  0x06    // Limited Discoverable (advertises for 30 seconds at a time)
          // and BR/EDR Not Supported
};

// Device name attribute value
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "BloodPressure Sensor";

// Bonded state
static bool timeAppBonded = FALSE;

// Bonded peer address
static uint8 timeAppBondedAddr[B_ADDR_LEN];

// GAP connection handle
static uint16 gapConnHandle;

// BloodPressure measurement value stored in this structure
static attHandleValueInd_t  bloodPressureMeas;
static attHandleValueNoti_t bloodPressureIMeas;
static uint16 bpSystolic = 120; //mmg  
static uint16 bpDiastolic = 80; //mmg
static uint16 bpMAP = 90; //70-110mmg
static uint16 bpPulseRate = 60; //pulseRate
static uint8  bpUserId = 1;
static uint16 bpMeasStatus = 0;

//Char Config variables
static bool bpMeasCharConfig = false;
static bool bpIMeasCharConfig = false;

// flags for simulated measurements
static const uint8 bloodPressureFlags[FLAGS_IDX_MAX] =
{
  BLOODPRESSURE_FLAGS_MMHG | BLOODPRESSURE_FLAGS_TIMESTAMP |BLOODPRESSURE_FLAGS_PULSE| BLOODPRESSURE_FLAGS_USER,
  BLOODPRESSURE_FLAGS_MMHG | BLOODPRESSURE_FLAGS_TIMESTAMP,
  BLOODPRESSURE_FLAGS_MMHG,
  BLOODPRESSURE_FLAGS_KPA,
  BLOODPRESSURE_FLAGS_KPA | BLOODPRESSURE_FLAGS_TIMESTAMP,
  BLOODPRESSURE_FLAGS_KPA | BLOODPRESSURE_FLAGS_TIMESTAMP | BLOODPRESSURE_FLAGS_PULSE,
  0x00
};

// initial value of flags
static uint8 bloodPressureFlagsIdx = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void bloodPressureProcessGattMsg( gattMsgEvent_t *pMsg );
static void bloodPressure_ProcessOSALMsg( osal_event_hdr_t *pMsg );
static void peripheralStateNotificationCB( gaprole_States_t newState );
static void performPeriodicTask( void );
static void performPeriodicIMeasTask( void );
static void performPeriodicTestCmdTask( void );
static void bloodPressure_HandleKeys( uint8 shift, uint8 keys );
static void bloodPressureMeasIndicate(void);
static void bloodPressureCB(uint8 event);
static void timeAppPasscodeCB( uint8 *deviceAddr, uint16 connectionHandle,
                                        uint8 uiInputs, uint8 uiOutputs );
static void timeAppPairStateCB( uint16 connHandle, uint8 state, uint8 status );

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t bloodPressure_PeripheralCBs =
{
  peripheralStateNotificationCB,  // Profile State Change Callbacks
  NULL                // When a valid RSSI is read from controller
};

// Bond Manager Callbacks
static const gapBondCBs_t timeAppBondCB =
{
  timeAppPasscodeCB,
  timeAppPairStateCB
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      BloodPressure_Init
 *
 * @brief   Initialization function for the BloodPressure App Task.
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
void BloodPressure_Init( uint8 task_id )
{
  bloodPressureTaskId = task_id;

  // Setup the GAP Peripheral Role Profile
  {
    #if defined( CC2540_MINIDK )   
      // For the CC2540DK-MINI keyfob, device doesn't start advertising until button is pressed
      uint8 initial_advertising_enable = FALSE;
    #else
      // For other hardware platforms, device starts advertising upon initialization
      uint8 initial_advertising_enable = TRUE;
    #endif 
    // By setting this to zero, the device will go into the waiting state after
    // being discoverable for 30.72 second, and will not being advertising again
    // until the enabler is set back to TRUE
    uint16 gapRole_AdvertOffTime = 0;
      
    uint8 enable_update_request = DEFAULT_ENABLE_UPDATE_REQUEST;
    uint16 desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
    uint16 desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
    uint16 desired_slave_latency = DEFAULT_DESIRED_SLAVE_LATENCY;
    uint16 desired_conn_timeout = DEFAULT_DESIRED_CONN_TIMEOUT;

    // Set the GAP Role Parameters
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
    GAPRole_SetParameter( GAPROLE_ADVERT_OFF_TIME, sizeof( uint16 ), &gapRole_AdvertOffTime );
    
    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanData ), scanData );
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );
    
    GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
    GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
    GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
    GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
    GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
  }
  
  // Set the GAP Characteristics
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );

  // Setup the GAP Bond Manager
  {
    uint32 passkey = 0; // passkey "000000"
    uint8 pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8 mitm = FALSE;
    uint8 ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
    uint8 bonding = TRUE;
    GAPBondMgr_SetParameter( GAPBOND_DEFAULT_PASSCODE, sizeof ( uint32 ), &passkey );
    GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE, sizeof ( uint8 ), &pairMode );
    GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION, sizeof ( uint8 ), &mitm );
    GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES, sizeof ( uint8 ), &ioCap );
    GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED, sizeof ( uint8 ), &bonding );
  }  

  // Stop config reads when done
  timeConfigDone = FALSE;
   
  // Initialize GATT Client
  VOID GATT_InitClient();

  // Register to receive incoming ATT Indications/Notifications
  GATT_RegisterForInd( bloodPressureTaskId );
  
  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );         // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES ); // GATT attributes
  BloodPressure_AddService( GATT_ALL_SERVICES );
  DevInfo_AddService( );

  // Register for BloodPressure service callback
  BloodPressure_Register ( bloodPressureCB );
  
  // Register for all key events - This app will handle all key events
  RegisterForKeys( bloodPressureTaskId );
  
 #if defined( CC2540_MINIDK ) 
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

#endif // #if defined( CC2540_MINIDK )  
  
  // Setup a delayed profile startup
  osal_set_event( bloodPressureTaskId, START_DEVICE_EVT );
}

/*********************************************************************
 * @fn      BloodPressure_ProcessEvent
 *
 * @brief   BloodPressure Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 BloodPressure_ProcessEvent( uint8 task_id, uint16 events )
{
  
  VOID task_id; // OSAL required parameter that isn't used in this function
  
  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( bloodPressureTaskId )) != NULL )
    {
      bloodPressure_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & START_DEVICE_EVT )
  {
    // Start the Device
    VOID GAPRole_StartDevice( &bloodPressure_PeripheralCBs );
 
   // Register with bond manager after starting device
    GAPBondMgr_Register( (gapBondCBs_t *) &timeAppBondCB );    
    
    // Start 1 Second timer to keep time
    //osal_start_timerEx( bloodPressureTaskId, PERIODIC_TIMESTAMP_EVT, PERIODIC_TIMESTAMP_EVT_PERIOD );   
    
    return ( events ^ START_DEVICE_EVT );
  }

  if ( events & START_DISCOVERY_EVT )
  {
    if ( timeAppPairingStarted )
    {
      // Postpone discovery until pairing completes
      timeAppDiscPostponed = TRUE;
    }
    else
    {
      timeAppDiscState = timeAppDiscStart();
    }  
    return ( events ^ START_DISCOVERY_EVT );
  }
  
  //periodic indications
  if ( events & PERIODIC_EVT )
  {
    // Perform periodic application task
    performPeriodicTask();
    return (events ^ PERIODIC_EVT);
  }

  //periodic notifcations
  if ( events & PERIODIC_IMEAS_EVT )
  {
    // Perform periodic application task
    performPeriodicIMeasTask();
    return (events ^ PERIODIC_IMEAS_EVT);
  }  

  //simulation event
  if ( events & PERIODIC_TESTCMD_EVT )
  {
    // Perform periodic testCmd task
    performPeriodicTestCmdTask();
    return (events ^ PERIODIC_TESTCMD_EVT);
  }

   return 0;
}

/*********************************************************************
 * @fn      bloodPressure_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void bloodPressure_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
  case KEY_CHANGE:
      bloodPressure_HandleKeys( ((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys );
      break;
 
  case GATT_MSG_EVENT:
      bloodPressureProcessGattMsg( (gattMsgEvent_t *) pMsg );
      break;
  default:
      break;
  }
}

/*********************************************************************
 * @fn      bloodPressure_HandleKeys
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
static void bloodPressure_HandleKeys( uint8 shift, uint8 keys )
{
 
  if ( keys & HAL_KEY_SW_1 )
  {
    // set simulated measurement flag index
    if (++bloodPressureFlagsIdx == FLAGS_IDX_MAX)
    {
      bloodPressureFlagsIdx = 0;
    }
  }
  
  if ( keys & HAL_KEY_SW_2 )
  {
    // if device is not in a connection, pressing the right key should toggle
    // advertising on and off
    if( gapProfileState != GAPROLE_CONNECTED )
    {
      uint8 current_adv_enabled_status;
      uint8 new_adv_enabled_status;
      
      //Find the current GAP advertisement status
      GAPRole_GetParameter( GAPROLE_ADVERT_ENABLED, &current_adv_enabled_status );
      
      if( current_adv_enabled_status == FALSE )
        new_adv_enabled_status = TRUE;
      else
        new_adv_enabled_status = FALSE;
      
      //change the GAP advertisement status to opposite of current status
      GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &new_adv_enabled_status );   
    }
    else //connected mode, simulate some measurements
    {
      if(bpSystolic < 150)
        bpSystolic +=1;
      else
        bpSystolic = 80;
      
      if(bpDiastolic < 110)
        bpDiastolic +=1;
      else
        bpDiastolic = 90;    
  
      if(bpMAP < 110)
        bpMAP +=1;
      else
        bpMAP =70;  
      
      if(bpPulseRate < 140)
        bpPulseRate +=1;
      else
        bpPulseRate =40;  
      
      if(bpUserId < 5)
        bpUserId +=1;
      else
        bpUserId =1;  
    }
  }
}

/*********************************************************************
 * @fn      timeAppProcessGattMsg
 *
 * @brief   Process GATT messages
 *
 * @return  none
 */
static void bloodPressureProcessGattMsg( gattMsgEvent_t *pMsg )
{
  if ( pMsg->method == ATT_HANDLE_VALUE_NOTI ||
       pMsg->method == ATT_HANDLE_VALUE_IND )
  {
    timeAppIndGattMsg( pMsg );
  }
  else if ( pMsg->method == ATT_READ_RSP ||
            pMsg->method == ATT_WRITE_RSP )
  {
    timeAppConfigState = timeAppConfigGattMsg ( timeAppConfigState, pMsg );
    if ( timeAppConfigState == TIMEAPP_CONFIG_CMPL )
    {
      timeAppDiscoveryCmpl = TRUE;
    }
  }
  else
  {
    timeAppDiscState = timeAppDiscGattMsg( timeAppDiscState, pMsg );
    if ( timeAppDiscState == DISC_IDLE )
    {      
      // Start characteristic configuration
      timeAppConfigState = timeAppConfigNext( TIMEAPP_CONFIG_START );
    }
  }
}

/*********************************************************************
 * @fn      timeAppDisconnected
 *
 * @brief   Handle disconnect. 
 *
 * @return  none
 */
static void timeAppDisconnected( void )
{
  // Initialize state variables
  timeAppDiscState = DISC_IDLE;
  timeAppPairingStarted = FALSE;
  timeAppDiscPostponed = FALSE;
  
  // stop periodic measurement
  osal_stop_timerEx( bloodPressureTaskId, PERIODIC_EVT );

  // reset bloodPressure measurement client configuration
  uint16 param = 0;
  BloodPressure_SetParameter(BLOODPRESSURE_MEAS_CHAR_CFG, sizeof(uint16), (uint8 *) &param);

  // reset bloodPressure intermediate measurement client configuration
  BloodPressure_SetParameter(BLOODPRESSURE_IMEAS_CHAR_CFG, sizeof(uint16), (uint8 *) &param);

}

/*********************************************************************
 * @fn      gapProfileStateCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB( gaprole_States_t newState )
{
  // if connected
  if ( newState == GAPROLE_CONNECTED )
  {
    linkDBItem_t  *pItem;

    // Get connection handle
    GAPRole_GetParameter( GAPROLE_CONNHANDLE, &gapConnHandle );

    // Get peer bd address
    if ( (pItem = linkDB_Find( gapConnHandle )) != NULL)
    {
      // If connected to device without bond do service discovery
      if ( !osal_memcmp( pItem->addr, timeAppBondedAddr, B_ADDR_LEN ) )
      {
        timeAppDiscoveryCmpl = FALSE;
      }
      
      // Initiate service discovery if necessary
      if ( timeAppDiscoveryCmpl == FALSE )
      {
        osal_start_timerEx( bloodPressureTaskId, START_DISCOVERY_EVT, DEFAULT_DISCOVERY_DELAY );
      }
    }
  }
  // if disconnected
  else if ( gapProfileState == GAPROLE_CONNECTED && 
            newState != GAPROLE_CONNECTED )
  {
    timeAppDisconnected();
  }    
  // if started
  else if ( newState == GAPROLE_STARTED )
  {
    uint8 bdAddr[B_ADDR_LEN];
    
    GAPRole_GetParameter( GAPROLE_BD_ADDR, &bdAddr );
   }
  gapProfileState = newState;
}

/*********************************************************************
 * @fn      pairStateCB
 *
 * @brief   Pairing state callback.
 *
 * @return  none
 */
static void timeAppPairStateCB( uint16 connHandle, uint8 state, uint8 status )
{
  if ( state == GAPBOND_PAIRING_STATE_STARTED )
  {
    timeAppPairingStarted = TRUE;
  }
  else if ( state == GAPBOND_PAIRING_STATE_COMPLETE )
  {
    timeAppPairingStarted = FALSE;

    if ( status == SUCCESS )
    {
      linkDBItem_t  *pItem;
      
      if ( (pItem = linkDB_Find( gapConnHandle )) != NULL )
      {
        // Store bonding state of pairing
        timeAppBonded = ( (pItem->stateFlags & LINK_BOUND) == LINK_BOUND );
        
        if ( timeAppBonded )
        {
          osal_memcpy( timeAppBondedAddr, pItem->addr, B_ADDR_LEN );
        }
      }
      
      // If discovery was postponed start discovery
      if ( timeAppDiscPostponed && timeAppDiscoveryCmpl == FALSE )
      {
        timeAppDiscPostponed = FALSE;
        osal_set_event( bloodPressureTaskId, START_DISCOVERY_EVT );
      }

    }
  }
}

/*********************************************************************
 * @fn      timeAppPasscodeCB
 *
 * @brief   Passcode callback.
 *
 * @return  none
 */
static void timeAppPasscodeCB( uint8 *deviceAddr, uint16 connectionHandle,
                                        uint8 uiInputs, uint8 uiOutputs )
{
  // Send passcode response
  GAPBondMgr_PasscodeRsp( connectionHandle, SUCCESS, 0 );
}

/*********************************************************************
 * @fn      bloodPressureMeasIndicate
 *
 * @brief   Prepare and send a bloodPressure measurement indication
 *
 * @return  none
 */
static void bloodPressureMeasIndicate(void)
{
    
  // att value notification structure 
  uint8 *p = bloodPressureMeas.value;
  
  //flags
  uint8 flags = bloodPressureFlags[bloodPressureFlagsIdx];
  
  // flags 1 byte long
  *p++ = flags;

  //bloodpressure components
  *p++ = bpSystolic;
  *p++;
  *p++ = bpDiastolic;
  *p++;
  *p++ = bpMAP;
  *p++;
  
  //timestamp
  if (flags & BLOODPRESSURE_FLAGS_TIMESTAMP)
  {
   
    UTCTimeStruct time;
  
    // Get time structure from OSAL
    osal_ConvertUTCTime( &time, osal_getClock() );
      
    *p++ = 0x07;
    *p++ = 0xdb;
    *p++=time.month;    
    *p++=time.day;  
    *p++=time.hour;    
    *p++=time.minutes;    
    *p++=time.seconds;   
  }
  
  if(flags & BLOODPRESSURE_FLAGS_PULSE)
  {
    *p++ =  bpPulseRate;
    *p++;    
  }
 
    if(flags & BLOODPRESSURE_FLAGS_USER)
  {
    *p++ =  bpUserId;    
  }

    if(flags & BLOODPRESSURE_FLAGS_STATUS)
  {
    *p++ =  bpMeasStatus;      
  }
  
  bloodPressureMeas.len = (uint8) (p - bloodPressureMeas.value);
  //Send Measurement or Store(ToDo)
  if(bpMeasCharConfig == true)
    BloodPressure_MeasIndicate( gapConnHandle, &bloodPressureMeas,  bloodPressureTaskId);
}

/*********************************************************************
 * @fn      bloodPressureIMeasNotify
 *
 * @brief   Prepare and send a bloodPressure measurement notification
 *
 * @return  none
 */
static void bloodPressureIMeasNotify(void)
{
    
  // att value notification structure 
  uint8 *p = bloodPressureIMeas.value;
  
  //flags
  uint8 flags = bloodPressureFlags[bloodPressureFlagsIdx];
  
  // flags 1 byte long
  *p++ = flags;

  //bloodpressure components
  *p++ = bpSystolic;
  *p++;
  *p++ = 0;   //not used in cutoff
  *p++;
  *p++ = 0;   //not used in cutoff
  *p++;
 
  if(flags & BLOODPRESSURE_FLAGS_STATUS)
  {
    *p++ =  bpMeasStatus;      
  }
  
  bloodPressureIMeas.len = (uint8) (p - bloodPressureIMeas.value);
  
  //Send Measurement or Store(ToDo)
  if(bpIMeasCharConfig == true)
  {
    BloodPressure_IMeasNotify( gapConnHandle, &bloodPressureIMeas,  bloodPressureTaskId);
  }
}


/*********************************************************************
 * @fn      bloodPressureCB
 *
 * @brief   Callback function for bloodPressure service.
 *
 * @param   event - service event
 *
 * @return  none
 */
static void bloodPressureCB(uint8 event)
{

  switch (event)
  {
  case BLOODPRESSURE_MEAS_NOTI_ENABLED:
    bpMeasCharConfig = true;
    if (gapProfileState == GAPROLE_CONNECTED)
    {
      osal_start_timerEx( bloodPressureTaskId, PERIODIC_EVT, 1000 );
    }      
    break;

  case  BLOODPRESSURE_MEAS_NOTI_DISABLED:
    bpMeasCharConfig = false;
    osal_stop_timerEx( bloodPressureTaskId, PERIODIC_EVT );  
    break;

  case BLOODPRESSURE_IMEAS_NOTI_ENABLED:
    bpIMeasCharConfig = true;
    if (gapProfileState == GAPROLE_CONNECTED)
    {
      osal_start_timerEx( bloodPressureTaskId, PERIODIC_IMEAS_EVT, 1000 );
    }      
    break;

  case  BLOODPRESSURE_IMEAS_NOTI_DISABLED:
    bpIMeasCharConfig = false;
    osal_stop_timerEx( bloodPressureTaskId, PERIODIC_IMEAS_EVT );  
    break;    
   
  default:
    break;    
  }
}

/*********************************************************************
 * @fn      performPeriodicTask
 *
 * @brief   Perform a periodic application task.
 *
 * @param   none
 *
 * @return  none
 */
static void performPeriodicTask( void )
{
  if (gapProfileState == GAPROLE_CONNECTED)
  {
    // send bloodPressure measurement notification
    bloodPressureMeasIndicate();
    
    // send bloodPressure measurement notification
    bloodPressureIMeasNotify();    
   
    // Start interval timer to send BP, just for simulation
    osal_start_timerEx( bloodPressureTaskId, PERIODIC_EVT, 3000 );
  }
}

/*********************************************************************
 * @fn      performPeriodicIMeasTask
 *
 * @brief   Perform a periodic bloodPressure cutoff measurement. 
 *
 * @param   none
 *
 * @return  none
 */
static void performPeriodicIMeasTask( void )
{
  if (gapProfileState == GAPROLE_CONNECTED)
  {
    // send bloodPressure measurement notification
    bloodPressureIMeasNotify();    
   
    // Start interval timer to send BP, just for simulation
    osal_start_timerEx( bloodPressureTaskId, PERIODIC_IMEAS_EVT, 1000 );
  }
}

/*********************************************************************
 * @fn      performPeriodicTask
 *
 * @brief   Perform a periodic application task.
 *
 * @param   none
 *
 * @return  none
 */
static void performPeriodicTestCmdTask( void )
{
  
  if (gapProfileState == GAPROLE_CONNECTED)
  {

    if(bpSystolic < 150)
      bpSystolic +=1;
    else
      bpSystolic = 80;
    
    if(bpDiastolic < 110)
      bpDiastolic +=1;
    else
      bpDiastolic = 90;    

    if(bpMAP < 110)
      bpMAP +=1;
    else
      bpMAP =70;  
    
    if(bpPulseRate < 140)
      bpPulseRate +=1;
    else
      bpPulseRate =40;  
    
    if(bpUserId < 5)
      bpUserId +=1;
    else
      bpUserId =1;  
   
    // Start interval timer to send BP, just for simulation
    osal_start_timerEx( bloodPressureTaskId, PERIODIC_TESTCMD_EVT, 1000 );

  }
}

/*********************************************************************
*********************************************************************/
