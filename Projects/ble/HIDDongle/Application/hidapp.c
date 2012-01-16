/**************************************************************************************************
    Filename:       hidapp.c
    Revised:        $Date: 2011-05-27 15:28:33 -0700 (Fri, 27 May 2011) $
    Revision:       $Revision: 26143 $

    Description:    This file contains the BLE sample application for HID dongle application.

  Copyright 2010-2011 Texas Instruments Incorporated. All rights reserved.

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

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

// HAL includes
#include "hal_types.h"
#include "hal_board.h"
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"

// HID includes
#include "usb_framework.h"
#include "usb_hid.h"
#include "usb_hid_reports.h"
#include "usb_suspend.h"

// OSAL includes
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "OSAL_Tasks.h"
#include "osal_snv.h"

#include "OnBoard.h"

/* LL */
#include "ll.h"

#if defined ( OSAL_CBTIMER_NUM_TASKS )
  #include "osal_cbTimer.h"
#endif

/* L2CAP */
#include "l2cap.h"

/* gap */
#include "gap.h"

/* GATT */
#include "gatt.h"

/* Application */
#include "hci_controller.h"

#include "central.h"

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */


#define CONNECT_ADDR                    { 0x22, 0x22, 0x22, 0x22, 0x22, 0x22 }
#define OWN_ADDR                        { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 }

#define APP_CONN_INTERVAL               8     // 10ms
#define APP_SLAVE_LATENCY               49    // 49 slave latency (500ms effective interval)
#define APP_CONN_TIMEOUT                160   // 1.6s supervision timeout

// OSAL events the application uses
#define HIDAPP_EVT_START                0x0001 // Start the application
#define HIDAPP_EVT_KEY_RELEASE_TIMER    0x0002 // timer to detect key release
#define HIDAPP_EVT_PROCESS              0x0004
#define HIDAPP_EVT_PROCESS_RELEASE      0x0008

// Key release detect timer duration in milliseconds
#define HIDAPP_KEY_RELEASE_DETECT_DUR   1000

// Key event repeat delay duration as multiples of CERC repeat commands
#define HIDAPP_KEY_REPEAT_DELAY         10

// Subset of HID key codes, used in this application
#define HIDAPP_KEYCODE_INVALID          0x00

// index to the modifier bits byte in the HID report buffer
#define HIDAPP_BUFIDX_MOD_BITS          1

// index to the first key code in the HID report buffer
#define HIDAPP_BUFIDX_FIRST_KEY         3

// index to the last key code in the HID report buffer
#define HIDAPP_BUFIDX_LAST_KEY          8

// output report buffer size
#define HIDAPP_OUTBUF_SIZE              3

// Vendor specific report identifiers
#define HID_CMD_REPORT_ID               1
#define HID_PAIR_ENTRY_REPORT_ID        2

// output report ID
#define HIDAPP_OUTPUT_REPORT_ID         HID_CMD_REPORT_ID

// Count of repetition polling for INPUT packet ready
#define USB_HID_INPUT_REPEAT_COUNT      3


/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

// Remote control 2 byte report
typedef struct
{
  uint8 data[2];
} usbHidReportRemote_t;

// Vendor specific control 2 byte report
typedef struct
{
  uint8 reportId;
  uint8 data[2];
} usbHidReportVendor_t;

// Vendor specific control 11 byte report
typedef struct
{
  uint8 reportId;
  uint8 pairRef;
  uint16 vendorId;
  uint8 ieeeAddr[8];
} usbHidReportPairEntry_t;

// enumerated type for current central BLE 
enum
{
  BLE_STATE_IDLE,
  BLE_STATE_CONNECTING,
  BLE_STATE_CONNECTED,
  BLE_STATE_DISCONNECTING
};


/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */

// Macros for the HID report

#define USB_HID_REPORT_REMOTE_INIT(_s) \
  st((_s).data[0] = 0; (_s).data[1] = 0;)
#define USB_HID_REPORT_REMOTE_HAS_SOME(_s) \
  ((_s).data[0] != 0 || (_s).data[1] != 0)

/* ------------------------------------------------------------------------------------------------
 *                                           Local Functions
 * ------------------------------------------------------------------------------------------------
 */

void   hidappInit( uint8 taskId );
void   osalInitTasks( void );
static uint16 hidappProcessEvent(uint8 taskId, uint16 events);
void   hidappKeyCback(uint8 keys, uint8 state);
static void hidappStart( void );
static void hidappKeyReleaseDetected( void );
static void hidappOutputReport( void );
static void hidSendRemoteReport(void);
static void hidProcessCercCtrl(uint8 len, uint8 *pData);
static void processL2CAPDataMsg( l2capDataEvent_t *pMsg );
static void processGATTMsg( gattMsgEvent_t *pMsg );
static void centralEventCB( gapCentralRoleEvent_t *p  );


/* ------------------------------------------------------------------------------------------------
 *                                           Local Variables
 * ------------------------------------------------------------------------------------------------
 */

// HID output report buffer
static uint8 hidappOutBuf[HIDAPP_OUTBUF_SIZE];

// keyboard input report buffer
static uint8 hidappKeyRepBuf[9] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00 };

// current input report buffer index
static uint8 bufIdx = HIDAPP_BUFIDX_FIRST_KEY;

//static uint8 processPending = FALSE;

static uint8 releaseIdx = HIDAPP_BUFIDX_FIRST_KEY;

// OSAL task ID assigned to the application task
static uint8 hidappTaskId;

// HID Remote Report buffer.
static usbHidReportRemote_t hidappRemRepBuf;

// Peer device address
static uint8 connectAddr[B_ADDR_LEN] = CONNECT_ADDR;

// GAP Role Callbacks
static gapCentralRoleCB_t hidApp_centralCBs =
{
  NULL,                // When a valid RSSI is read from controllerNULL,
  centralEventCB,      // Profile State Change Callbacks
};

// Application state
static uint8 hidappBLEState = BLE_STATE_IDLE;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

// Sample Application task related data
const pTaskEventHandlerFn tasksArr[] = {
  /* To fulfill USB device responsibilities within specified maximum
   * limits, the HAL USB polling should be the highest priority.
   */
  Hal_ProcessEvent,
  LL_ProcessEvent,  
  HCI_ProcessEvent,
#if defined ( OSAL_CBTIMER_NUM_TASKS )
  OSAL_CBTIMER_PROCESS_EVENT( osal_CbTimerProcessEvent ),
#endif
  L2CAP_ProcessEvent,
  GAP_ProcessEvent,
  SM_ProcessEvent,
  GATT_ProcessEvent,
  GAPCentralRole_ProcessEvent,
  hidappProcessEvent
};

const uint8 tasksCnt = sizeof( tasksArr ) / sizeof( tasksArr[0] );
uint16 *tasksEvents;

/* ------------------------------------------------------------------------------------------------
 *                                   External Assembly Routines
 * ------------------------------------------------------------------------------------------------
 */

/**************************************************************************************************
 * @fn          main
 *
 * @brief       This function is the C-code main entry function.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      Reason for exiting.
 */
int main(void)
{
  osal_int_disable(INTS_ALL);

  HAL_BOARD_INIT();     // Essential H/W initialization: clock, cache, GPIO.

  // Initialization for HAL including USART for NPI (SPI or UART) or USB.
  HalDriverInit();
  
  osal_snv_init();      // NV system initialization.

  osal_init_system();   // Initialization of OSAL and tasks.

  /* Although overtly a HAL function, a call to osal_set_event() requires waiting
   * until after osal_init_system().
   */
  HalKeyConfig(HAL_KEY_INTERRUPT_ENABLE, hidappKeyCback);

  usbHidInit();
  osal_int_enable(INTS_ALL);
  osal_start_system();  // Enter the infinite tasking loop.
  
  HAL_SYSTEM_RESET();   // Execution should not fall through to here.
}

/**************************************************************************************************
 *
 * @fn          hidappSuspendEnter
 *
 * @brief       hook function to be called upon entry into USB suspend mode
 *
 * @param       none
 *
 * @return      none
 */
void hidappSuspendEnter(void)
{
}

/**************************************************************************************************
 *
 * @fn          hidappSuspendExit
 *
 * @brief       hook function to be called upon exit from USB suspend mode
 *
 * @param       none
 *
 * @return      none
 */
void hidappSuspendExit(void)
{
}

/**************************************************************************************************
 *
 * @fn          hidappInit
 *
 * @brief       This is the Sample Application task initialization called by OSAL.
 *
 * @param       taskId - task ID assigned after it was added in the OSAL task queue
 *
 * @return      none
 */
void hidappInit( uint8 taskId )
{
  uint8 ownBDaddr[] = OWN_ADDR;
  
  // save task ID assigned by OSAL
  hidappTaskId = taskId;


  
  // USB suspend entry/exit hook function setup
  pFnSuspendEnterHook= hidappSuspendEnter;
  pFnSuspendExitHook= hidappSuspendExit;

  osal_memset(hidappKeyRepBuf, HIDAPP_KEYCODE_INVALID, sizeof(hidappKeyRepBuf));

  // Initialize GATT Client
  VOID GATT_InitClient();
  
  // Register to receive notifications
  GATT_RegisterForInd( hidappTaskId );

  // Set connection parameters:
  {
    uint16 connectionInterval = APP_CONN_INTERVAL;
    uint16 slaveLatency = APP_SLAVE_LATENCY;
    uint16 timeout = APP_CONN_TIMEOUT;
    
    GAP_SetParamValue( TGAP_CONN_EST_INT_MIN, connectionInterval );
    GAP_SetParamValue( TGAP_CONN_EST_INT_MAX, connectionInterval );
    GAP_SetParamValue( TGAP_CONN_EST_LATENCY, slaveLatency );
    GAP_SetParamValue( TGAP_CONN_EST_SUPERV_TIMEOUT, timeout );
    
  }

  HCI_EXT_SetBDADDRCmd( ownBDaddr );
  
  // Turn on LED and start Central Role profile
  HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);

  // set up continued initialization from within OSAL task loop
  osal_start_timerEx( taskId, HIDAPP_EVT_START, 100 );
  
}


/**************************************************************************************************
 * @fn      osalInitTasks
 *
 * @brief   This function is called by OSAL to invoke the initialization
 *          function for each task.
 *
 * @param   void
 *
 * @return  none
 */
void osalInitTasks( void )
{
  uint8 taskID = 0;

  tasksEvents = (uint16 *)osal_mem_alloc( sizeof( uint16 ) * tasksCnt);
  osal_memset( tasksEvents, 0, (sizeof( uint16 ) * tasksCnt));

  Hal_Init( taskID++ );
  
  /* LL Task */
  LL_Init( taskID++ );
  
  /* HCI Task */
  HCI_Init( taskID++ );
  
#if defined ( OSAL_CBTIMER_NUM_TASKS )
  /* Callback Timer Tasks */
  osal_CbTimerInit( taskID );
  taskID += OSAL_CBTIMER_NUM_TASKS;
#endif

  /* L2CAP Task */
  L2CAP_Init( taskID++ );
  
  /* GAP Task */
  GAP_Init( taskID++ );
  
  /* SM Task */
  SM_Init( taskID++ );
  
  /* GATT Task */
  GATT_Init( taskID++ );
  
  GAPCentralRole_Init( taskID++ );

  hidappInit( taskID );
}


/**************************************************************************************************
 * @fn          hidappProcessEvent
 *
 * @brief       This function processes the OSAL events and messages for the
 *              Sample Application.
 *
 * input parameters
 *
 * @param taskId - The task ID assigned to this application by OSAL at system initialization.
 * @param events - A bit mask of the pending event(s).
 *
 * output parameters
 *
 * None.
 *
 * @return      The events bit map received via parameter with the bits cleared which correspond to
 *              the event(s) that were processed on this invocation.
 */
static uint16 hidappProcessEvent(uint8 taskId, uint16 events)
{
  (void) taskId; // Unused argument
  
  if ( events & SYS_EVENT_MSG )
  {
    osal_event_hdr_t  *pMsg;

    while ((pMsg = (osal_event_hdr_t *) osal_msg_receive(hidappTaskId)) != NULL)
    {
      switch (pMsg->event)
      {
        case L2CAP_DATA_EVENT:
          {
            l2capDataEvent_t *pPkt = (l2capDataEvent_t *)pMsg;
            
            processL2CAPDataMsg( pPkt );
            
            // Free the buffer - payload
            if ( pPkt->pkt.pPayload )
            {
              osal_bm_free( pPkt->pkt.pPayload );
            }
          }
          break;
          
        case GATT_MSG_EVENT:
          {
            processGATTMsg( (gattMsgEvent_t *)pMsg );
          }
      
        default:
          break;
      }

      osal_msg_deallocate((uint8 *) pMsg);
    }

    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & HIDAPP_EVT_START )
  {
    hidappStart();
    
    return (events ^ HIDAPP_EVT_START);
  }
  
  if ( events & HIDAPP_EVT_KEY_RELEASE_TIMER )
  {
    // Key release detection timer event
    hidappKeyReleaseDetected();
  }
  
  if ( events & HIDAPP_EVT_PROCESS )
  {

    if ( ( releaseIdx == (HIDAPP_BUFIDX_FIRST_KEY+1) ) && ( bufIdx == releaseIdx ) )
    {
      // buffer has no more data to be sent; process the release
      uint8 tempBuf[9] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
      osal_memcpy( hidappKeyRepBuf, tempBuf, 9 );
      releaseIdx = HIDAPP_BUFIDX_FIRST_KEY;
    }
    else
    {
      if ( releaseIdx == bufIdx )
      {
        // release has occured, but must send data in buffer first
        osal_start_timerEx( hidappTaskId, HIDAPP_EVT_PROCESS_RELEASE, 5 );
      }
      else
      {
        releaseIdx = HIDAPP_BUFIDX_FIRST_KEY;
      }
    }
    
    hidProcessCercCtrl( 2, hidappKeyRepBuf ); 
    bufIdx = HIDAPP_BUFIDX_FIRST_KEY;

    return (events ^ HIDAPP_EVT_PROCESS);
  }
  
  if ( events & HIDAPP_EVT_PROCESS_RELEASE )
  {
    // buffer has no more data to be sent; process the release
    uint8 tempBuf[9] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    osal_memcpy( hidappKeyRepBuf, tempBuf, 9 );
    releaseIdx = HIDAPP_BUFIDX_FIRST_KEY;
    
    hidProcessCercCtrl( 2, hidappKeyRepBuf ); 
    
    return ( events ^ HIDAPP_EVT_PROCESS_RELEASE );
  }
  
  return ( 0 );  /* Discard unknown events. */
}

/**************************************************************************************************
 *
 * @fn      hidappKeyCback
 *
 * @brief   Callback service for keys
 *
 * @param   keys  - key that was pressed (i.e. the scanned row/col index)
 *          state - shifted
 *
 * @return  void
 */
void hidappKeyCback(uint8 keys, uint8 state)
{
  // Unused arguments
  (void) state;

  if (keys & HAL_KEY_SW_1)
  {
    
    if ( hidappBLEState != BLE_STATE_CONNECTED )
    {
      // uint8 newConnectValue = TRUE;
      hidappBLEState = BLE_STATE_CONNECTING;
      GAPCentralRole_EstablishLink( TRUE, WL_NOTUSED, ADDRTYPE_PUBLIC, connectAddr );
    }
  }
  if (keys & HAL_KEY_SW_2)
  {
    if ( hidappBLEState == BLE_STATE_CONNECTED )
    {
      hidappBLEState = BLE_STATE_DISCONNECTING;
      GAPCentralRole_TerminateLink( 0x0000);
    }
  }

}

/**************************************************************************************************
 *
 * @fn      hidappStart
 *
 * @brief   Start up the application
 *
 * @param   None
 *
 * @return  None
 */
static void hidappStart(void)
{
  GAPCentralRole_StartDevice( &hidApp_centralCBs );
}

/**************************************************************************************************
 *
 * @fn      hidappKeyReleaseDetected
 *
 * @brief   Handles detection of key release
 *
 * @param   None
 *
 * @return  None
 */
static void hidappKeyReleaseDetected( void )
{
  if (hidappKeyRepBuf[HIDAPP_BUFIDX_FIRST_KEY] != HIDAPP_KEYCODE_INVALID)
  {
    // Keyboard report key is released
    usbHidProcessKeyboard(hidappKeyRepBuf);
  }
  if (USB_HID_REPORT_REMOTE_HAS_SOME(hidappRemRepBuf))
  {
    // Consumer control key is released
    USB_HID_REPORT_REMOTE_INIT(hidappRemRepBuf);
    hidSendRemoteReport();
  }
}

/**************************************************************************************************
 *
 * @fn      hidappOutputReport
 *
 * @brief   Handles output report from host
 *
 * @param   None
 *
 * @return  None
 */
static void hidappOutputReport( void )
{
  // Decode the output report
  if (HIDAPP_OUTPUT_REPORT_ID == hidappOutBuf[0])
  {
    // correct report ID
    uint8 cmdId = hidappOutBuf[1];
    switch (cmdId)
    {
      // Process output buffer here
    }
  }
}

/**************************************************************************************************
 * @fn      hidSendRemoteReport
 *
 * @brief   Send the remote input report stored in global data structure
 *
 * @param   None
 *
 * @return  None
 */
static void hidSendRemoteReport(void)
{
  halIntState_t intState;
  uint8 i;

  for (i = 0; i < USB_HID_INPUT_REPEAT_COUNT; i++)
  {
    // Repeat polling a while since another packet might be in
    // input (transmit) buffer.
    // But it cannot repeat forever as it could overflow heap, etc.
    HAL_ENTER_CRITICAL_SECTION(intState);
    USBFW_SELECT_ENDPOINT(2);
    if (!(USBCSIL & USBCSIL_INPKT_RDY))
    {
      // Send the report
      usbfwWriteFifo(&USBF2, sizeof(usbHidReportRemote_t), &hidappRemRepBuf);
      USBCSIL |= USBCSIL_INPKT_RDY;
      HAL_EXIT_CRITICAL_SECTION(intState);
      break;
    }
    HAL_EXIT_CRITICAL_SECTION(intState);
  }
}

/**************************************************************************************************
 * @fn      hidProcessCercCtrl
 *
 * @brief   Process the received data buffer of a valid CERC control frame.
 *
 * @param   len - The length of the pData buffer.
 * @param   pData - A valid pointer to the received data buffer.
 *
 * @return  None
 */
static void hidProcessCercCtrl(uint8 len, uint8 *pData)
{

  // put pressed key and modifier key data into appropriate bytes in buffer

  // send HID report over USB
  usbHidProcessKeyboard(hidappKeyRepBuf);

  osal_set_event(hidappTaskId, HIDAPP_EVT_KEY_RELEASE_TIMER);
}


/**************************************************************************************************
 *
 * @fn      usbHidAppPoll
 *
 * @brief   Poll output reports from PC
 *
 * @param   None
 *
 * @return  None
 */
void usbHidAppPoll(void)
{
  uint8 controlReg;
  uint16 bytesNow;
  uint8 oldEndpoint;
  uint8 *pBuf;
  
  // Save the old index setting, then select endpoint 0 and fetch the control register
  oldEndpoint = USBFW_GET_SELECTED_ENDPOINT();
  USBFW_SELECT_ENDPOINT(3);
  
  // Read registers for interrupt reason
  controlReg = USBCSOL;
  
  // Receive OUT packets
  if (controlReg & USBCSOL_OUTPKT_RDY)
  {
    // Read FIFO
    bytesNow = (uint16) USBCNTL;
    bytesNow |= ((uint16) USBCNTH) << 8;
    
    if (bytesNow > HIDAPP_OUTBUF_SIZE)
    {
      // Ignore invalid length report.
      while (bytesNow > HIDAPP_OUTBUF_SIZE)
      {
        usbfwReadFifo(&USBF3, HIDAPP_OUTBUF_SIZE, hidappOutBuf);
        bytesNow -= HIDAPP_OUTBUF_SIZE;
      }
      if (bytesNow > 0)
      {
        usbfwReadFifo(&USBF3, bytesNow, hidappOutBuf);
      }
      // clear outpkt_ready flag and overrun, sent_stall, etc. just in case
      USBCSOL = 0;  
    }
    else
    {
      pBuf = hidappOutBuf;
      while (bytesNow > 255)
      {
        // since usbfwReadFifo function supports only upto 255 byte read
        // multiple calls are made in the loop
        usbfwReadFifo(&USBF3, 255, pBuf);
        bytesNow -= 255;
        pBuf += 255;
      }
      if (bytesNow > 0)
      {
        // read the remainder
        usbfwReadFifo(&USBF3, bytesNow, pBuf);
      }
      // clear outpkt_ready flag and overrun, sent_stall, etc. just in case
      USBCSOL = 0;  

      // application specific handling
      hidappOutputReport();
    }
  }

  // Restore the old index setting
  USBFW_SELECT_ENDPOINT(oldEndpoint);
}

/*********************************************************************
 * @fn          processL2CAPDataMsg
 *
 * @brief       Process incoming L2CAP messages.
 *
 * @param       pMsg - pointer to message.
 *
 * @return      none
 */
static void processL2CAPDataMsg( l2capDataEvent_t *pMsg )
{
  // Process the incoming L2CAP Generic message
  hidProcessCercCtrl( 2, pMsg->pkt.pPayload );
}

/*********************************************************************
 * @fn          processGATTMsg
 *
 * @brief       Process incoming GATT messages.
 *
 * @param       pPkt - pointer to message.
 *
 * @return      none
 */

static void processGATTMsg( gattMsgEvent_t *pPkt )
{
  if ( pPkt->hdr.status == SUCCESS )
  {
    // Build the message first
    switch ( pPkt->method )
    {
      case ATT_HANDLE_VALUE_NOTI:

        // Keyboard event
        {
          if ( bufIdx <= HIDAPP_BUFIDX_LAST_KEY )
          {
            
            hidappKeyRepBuf[HIDAPP_BUFIDX_MOD_BITS] = pPkt->msg.handleValueNoti.value[2];
            
            if ( bufIdx == HIDAPP_BUFIDX_FIRST_KEY )
            {
              HalLedSet(HAL_LED_2, HAL_LED_MODE_BLINK);
              
              // 8ms delay before sending HID report to wait for additional
              // packets to be received              
              osal_start_timerEx( hidappTaskId, HIDAPP_EVT_PROCESS, 5 );
            }
            
            if ( pPkt->msg.handleValueNoti.value[1] != 0x00 )
            {
              // notification indicates that a key was pressed, not released
        
              hidappKeyRepBuf[bufIdx] = pPkt->msg.handleValueNoti.value[1];
            }
            else
            {
              // notification indicates that key was released
              releaseIdx = bufIdx + 1;

            }

            bufIdx++;

          }
        }
        break;
         
      default:
        // Unknown event
        break;      
    }
  }
}

/*********************************************************************
 * @fn      centralEventCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void centralEventCB( gapCentralRoleEvent_t *pEvent )
{
  switch( pEvent->gap.opcode )
  {
  case GAP_DEVICE_INIT_DONE_EVENT:  
    {
      // turn on red LED to indicate initialization done
      HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
    }
    break;    
    
  case GAP_LINK_ESTABLISHED_EVENT:
    {
      // turn off the red LED upon connection
      
      if ( pEvent->gap.hdr.status == SUCCESS )
      {
        hidappBLEState = BLE_STATE_CONNECTED;
        HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
      }
    }
    break;

  case GAP_LINK_TERMINATED_EVENT:  
    {    
      // act as if a release has occured, but must send data in buffer first
      osal_start_timerEx( hidappTaskId, HIDAPP_EVT_PROCESS_RELEASE, 5 );
      
      hidappBLEState = BLE_STATE_IDLE;
      HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
    }
      
  default:
    {
      // For any other state including init, turn LED on
      HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
    }
    break;
  }
  
}

/**************************************************************************************************
**************************************************************************************************/
