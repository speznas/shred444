/**************************************************************************************************
  Filename:       hal_scancodes.h
  Revised:        $Date: 2010-08-06 08:56:11 -0700 (Fri, 06 Aug 2010) $
  Revision:       $Revision: 23333 $

  Description:    Defintion of HID keyboard scan codes.

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


#ifndef HAL_SCANCODES_H
#define HAL_SCANCODES_H

#ifdef __cplusplus
extern "C"
{
#endif


// Modifier key bits
#define     MODKEY_RIGHTGUI      0x80
#define     MODKEY_RIGHTALT      0x40
#define     MODKEY_RIGHTSHIFT    0x20
#define     MODKEY_RIGHTCONTROL  0x10
#define     MODKEY_LEFTGUI       0x08
#define     MODKEY_LEFTALT       0x04
#define     MODKEY_LEFTSHIFT     0x02
#define     MODKEY_LEFTCONTROL   0x01

// Modifier keys according to USB Spec
#define     KEY_LEFT_CTRL   0xE0
#define     KEY_LEFT_SHIFT  0xE1
#define     KEY_LEFT_ALT    0xE2
#define     KEY_RIGHT_CTRL  0xE3
#define     KEY_RIGHT_SHIFT 0xE4
#define     KEY_RIGHT_ALT   0xE5

// Keyboard HID scan codes
#define     KEY_A           0x04
#define     KEY_B           0x05
#define     KEY_C           0x06
#define     KEY_D           0x07
#define     KEY_E           0x08
#define     KEY_F           0x09
#define     KEY_G           0x0a
#define     KEY_H           0x0b
#define     KEY_I           0x0c
#define     KEY_J           0x0d
#define     KEY_K           0x0e
#define     KEY_L           0x0f
#define     KEY_M           0x10
#define     KEY_N           0x11
#define     KEY_O           0x12
#define     KEY_P           0x13
#define     KEY_Q           0x14
#define     KEY_R           0x15
#define     KEY_S           0x16
#define     KEY_T           0x17
#define     KEY_U           0x18
#define     KEY_V           0x19
#define     KEY_W           0x1A
#define     KEY_X           0x1B
#define     KEY_Y           0x1C
#define     KEY_Z           0x1D

#define     KEY_1           0x1E
#define     KEY_2           0x1F
#define     KEY_3           0x20
#define     KEY_4           0x21
#define     KEY_5           0x22
#define     KEY_6           0x23
#define     KEY_7           0x24
#define     KEY_8           0x25
#define     KEY_9           0x26
#define     KEY_0           0x27

#define     KEY_RETURN      0x28
#define     KEY_ESCAPE      0x29
#define     KEY_BACKSPACE   0x2A
#define     KEY_TAB         0x2B
#define     KEY_SPACE       0x2C
#define     KEY_HYPHEN      0x2D
#define     KEY_EQU         0x2E
#define     KEY_LEFT_BRACK  0x2F
#define     KEY_RIGHT_BRACK 0x30
#define     KEY_BACKSLASH   0x31
#define     KEY_EUROPE_1    0x32
#define     KEY_SEMICOLON   0x33
#define     KEY_QUOTE       0x34
#define     KEY_TILDE       0x35
#define     KEY_COMMA       0x36
#define     KEY_PERIOD      0x37
#define     KEY_SLASH       0x38
#define     KEY_CAPS_LOCK   0x39
#define     KEY_F1          0x3A
#define     KEY_F2          0x3B
#define     KEY_F3          0x3C
#define     KEY_F4          0x3D
#define     KEY_F5          0x3E
#define     KEY_F6          0x3F
#define     KEY_F7          0x40
#define     KEY_F8          0x41
#define     KEY_F9          0x42
#define     KEY_F10         0x43
#define     KEY_F11         0x44
#define     KEY_F12         0x45
#define     KEY_PRTSCRN     0x46
#define     KEY_SCROLL_LOCK 0x47
#define     KEY_BREAK       0x48
#define     KEY_INSERT      0x49
#define     KEY_HOME        0x4A
#define     KEY_PAGE_UP     0x4B
#define     KEY_DELETE      0x4C
#define     KEY_END         0x4D
#define     KEY_PAGE_DOWN   0x4E
#define     KEY_RIGHT_ARROW 0x4F
#define     KEY_LEFT_ARROW  0x50
#define     KEY_DOWN_ARROW  0x51
#define     KEY_UP_ARROW    0x52
#define     KEY_NUM_LOCK    0x53

#define     KEY_KEYPAD_SLASH 0x54
#define     KEY_KEYPAD_STAR  0x55
#define     KEY_KEYPAD_MINUS 0x56
#define     KEY_KEYPAD_PLUS  0x57
#define     KEY_KEYPAD_ENTER 0x58
#define     KEY_KEYPAD_1     0x59
#define     KEY_KEYPAD_2     0x5A
#define     KEY_KEYPAD_3     0x5B
#define     KEY_KEYPAD_4     0x5C
#define     KEY_KEYPAD_5     0x5D
#define     KEY_KEYPAD_6     0x5E
#define     KEY_KEYPAD_7     0x5F
#define     KEY_KEYPAD_8     0x60
#define     KEY_KEYPAD_9     0x61
#define     KEY_KEYPAD_0     0x62
#define     KEY_KEYPAD_DEL   0x63
#define     KEY_EUROPE_2     0x64

  

#ifdef __cplusplus
}
#endif

#endif    // HAL_SCANCODES_H
