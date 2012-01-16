/**************************************************************************************************
  Filename:       gatt_uuid.h
  Revised:        $Date: 2009-06-29 16:20:52 -0700 (Mon, 29 Jun 2009) $
  Revision:       $Revision: 20240 $

  Description:    This file contains Generic Attribute Profile (GATT)
                  UUID types.


  Copyright 2010 Texas Instruments Incorporated. All rights reserved.

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

#ifndef GATT_UUID_H
#define GATT_UUID_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */
  
/*
 * WARNING: The 16-bit UUIDs are assigned by the Bluetooth SIG and published 
 *          in the Bluetooth Assigned Numbers page. Do not change these values.
 *          Changing them will cause Bluetooth interoperability issues.
 */

/**
 * GATT Services
 */
#define GAP_SERVICE_UUID                           0x1800 // Generic Access Profile
#define GATT_SERVICE_UUID                          0x1801 // Generic Attribute Profile
#define BATT_STATE_SERVICE_UUID                    0xA000 // Battery State
#define TMERMO_HUMID_SERVICE_UUID                  0xA001 // Thermometer Humidity
#define WEIGHT_SERVICE_UUID                        0xA002 // Weight
#define POSITION_SERVICE_UUID                      0xA003 // Position
#define ALERT_SERVICE_UUID                         0xA004 // Alert
#define MANUFACT_SERVICE_UUID                      0xA005 // Manufacturer
#define ADDRESS_SERVICE_UUID                       0xA006 // Address

/**
 * GATT Attribute Types 
 */
#define GATT_PRIMARY_SERVICE_UUID                  0x2800 // Primary Service
#define GATT_SECONDARY_SERVICE_UUID                0x2801 // Secondary Service
#define GATT_INCLUDE_UUID                          0x2802 // Include
#define GATT_CHARACTER_UUID                        0x2803 // Characteristic

/**
 * GATT Characteristic Descriptors
 */
#define GATT_CHAR_EXT_PROPS_UUID                   0x2900 // Characteristic Extended Properties
#define GATT_CHAR_USER_DESC_UUID                   0x2901 // Characteristic User Description
#define GATT_CLIENT_CHAR_CFG_UUID                  0x2902 // Client Characteristic Configuration
#define GATT_SERV_CHAR_CFG_UUID                    0x2903 // Server Characteristic Configuration
#define GATT_CHAR_FORMAT_UUID                      0x2904 // Characteristic Format
#define GATT_CHAR_AGG_FORMAT_UUID                  0x2905 // Characteristic Aggregate Format

/**
 * GATT Characteristic Types 
 */
#define DEVICE_NAME_UUID                           0x2A00 // Device Name
#define APPEARANCE_UUID                            0x2A01 // Appearance
#define PERI_PRIVACY_FLAG_UUID                     0x2A02 // Peripheral Privacy Flag
#define RECONNECT_ADDR_UUID                        0x2A03 // Reconnection Address
#define PERI_CONN_PARAM_UUID                       0x2A04 // Peripheral Preferred Connection Parameters
#define SERVICE_CHANGED_UUID                       0x2A05 // Service Changed

#define BATT_STATE_UUID                            0xB000 // Battery State
#define TEMPERATURE_UUID                           0xB001 // Temperature
#define RELATIVE_HUMID_UUID                        0xB002 // Relative Humidity
#define WEIGHT_KG_UUID                             0xB004 // Weight Kg
#define LATI_LONGI_UUID                            0xB005 // Latitude Longitude
#define LATI_LONGI_ELEV_UUID                       0xB006 // Latitude Longitude Elevation
#define ALERT_ENUM_UUID                            0xB007 // Alert Enumeration
#define MANUFACT_NAME_UUID                         0xB008 // Manufacturer Name
#define SERIAL_NUM_UUID                            0xB009 // Serial Number
#define MANUFACT_ADDR_UUID                         0xB00A // Manufacturer Address

/**
 * GATT Characteristic Descriptions
 */
#define GATT_DESC_HEART_RATE_UUID                  0x3100 // used with Unit beats per minute
#define GATT_DESC_CADENCE_UUID                     0x3101 // used with Unit revolution per minute
#define GATT_DESC_LATITUDE_UUID                    0x3102 // used with Unit degree
#define GATT_DESC_LONGITUDE_UUID                   0x3103 // used with Unit degree
#define GATT_DESC_REL_HUMIDITY_UUID                0x3104 // used with Unit percent
#define GATT_DESC_BODY_MASS_IDX_UUID               0x3105 // used with Unit surface density
#define GATT_DESC_BODY_FAT_UUID                    0x3106 // used with Unit percent
#define GATT_DESC_INSIDE_UUID                      0x3107 // the value is inside a building
#define GATT_DESC_OUTSIDE_UUID                     0x3108 // the value is outside a building
#define GATT_DESC_HANGING_UUID                     0x3109 // used with Unit mass kilogram
#define GATT_DESC_BATTERY_UUID                     0x310A // the value is associated with a battery
#define GATT_DESC_PWR_SUPPLY_UUID                  0x310B // the value is associated with a power supply
#define GATT_DESC_CURRENT_UUID                     0x310C // the value is the current value
#define GATT_DESC_TARGET_UUID                      0x310D // the value is the target value
#define GATT_DESC_LIGHT_STATE_UUID                 0x310E // false = off, true = on
#define GATT_DESC_APPLICANCE_PWR_UUID              0x310F // false = off, true = on
#define GATT_DESC_FRACTIONAL_PWR_UUID              0x3110 // used with Unit range nibble, range octet, range word
#define GATT_DESC_LENGTH_UUID                      0x3111 // used with Unit meter
#define GATT_DESC_ELEVATION_UUID                   0x3112 // used with Unit meter

/**
 * GATT Characteristic Units
 */
#define GATT_UNITLESS_UUID                         0x3000 // <Symbol>, <Expressed in terms of SI base units>
#define GATT_UNIT_LEN_METER_UUID                   0x3001 // m, m
#define GATT_UNIT_MASS_KILOGRAM_UUID               0x3002 // kg, kg
#define GATT_UNIT_TIME_SEC_UUID                    0x3003 // s, s
#define GATT_UNIT_ELEC_CURR_AMPERE_UUID            0x3004 // A, A
#define GATT_UNIT_TD_TEMP_KELVIN_UUID              0x3005 // K, K
#define GATT_UNIT_TD_TEMP_KELVIN_UUID              0x3005 // K, K
#define GATT_UNIT_AMOUNT_SUBS_MOLE_UUID            0x3006 // mol, mol
#define GATT_UNIT_LUMIN_INTENSITY_CD_UUID          0x3007 // cd, cd
#define GATT_UNIT_PLANE_ANGLE_RAD_UUID             0x3008 // rad, m m-1
#define GATT_UNIT_SOLID_ANGLE_STERAD_UUID          0x3009 // sr, m2 m-2
#define GATT_UNIT_FREQUENCY_HTZ_UUID               0x300A // Hz, s-1
#define GATT_UNIT_FORCE_NEWTON_UUID                0x300B // N, m kg s-2
#define GATT_UNIT_PRESSURE_PASCAL_UUID             0x300C // Pa, N/m2 = m2 kg s-2
#define GATT_UNIT_ENERGY_JOULE_UUID                0x300D // J, N m = m2 kg s-2
#define GATT_UNIT_PWR_WATT_UUID                    0x300E // W, J/s = m2 kg s-3
#define GATT_UNIT_E_CHARGE_COUL_UUID               0x300F // C, sA
#define GATT_UNIT_E_POTENTIAL_DIF_V_UUID           0x3010 // V, W/A = m2 kg s-3 A-1

#define GATT_UNIT_C_TEMP_UUID                      0x3017 // 'C, t/'C = T/K - 273.15

#define GATT_UNIT_MINUTE_UUID                      0x3030 // min, 60 s
#define GATT_UNIT_HOUR_UUID                        0x3031 // h, 3600 s
#define GATT_UNIT_DAY_UUID                         0x3032 // d, 86400 s
#define GATT_UNIT_DEGREE_UUID                      0x3033 // o, (pi/180) rad

#define GATT_UNIT_ASTRONOMICAL_UUID                0x303E // ua, 1.49597870691 × 1011 m

#define GATT_UNIT_REV_PER_MIN_UUID                 0x3047 // r/min, 0.1047198 rad s-1

#define GATT_UNIT_DECIBEL_UUID                     0x3057 // dB, 10 log10 (P1 / P0)
#define GATT_UNIT_PWR_RATIO_DECI_MEAS_UUID         0x3058 // dBm, 10 log10 P + 30

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */

/**
 * GATT Services
 */
extern CONST uint8 gapServiceUUID[];
extern CONST uint8 gattServiceUUID[];
extern CONST uint8 testServiceUUID[];
extern CONST uint8 manufactServiceUUID[];
extern CONST uint8 addressServiceUUID[];
extern CONST uint8 longServiceUUID[];
extern CONST uint8 long3ServiceUUID[];

/**
 * GATT Attribute Types
 */
extern CONST uint8 primaryServiceUUID[];
extern CONST uint8 secondaryServiceUUID[];
extern CONST uint8 includeUUID[];
extern CONST uint8 characterUUID[];

/**
 * GATT Characteristic Descriptors
 */
extern CONST uint8 charExtPropsUUID[];
extern CONST uint8 charUserDescUUID[];
extern CONST uint8 clientCharCfgUUID[];
extern CONST uint8 servCharCfgUUID[];
extern CONST uint8 charFormatUUID[];
extern CONST uint8 charAggFormatUUID[];

/**
 * GATT Characteristic Types
 */
extern CONST uint8 deviceNameUUID[];
extern CONST uint8 appearanceUUID[];
extern CONST uint8 periPrivacyFlagUUID[];
extern CONST uint8 reconnectAddrUUID[];
extern CONST uint8 periConnParamUUID[];
extern CONST uint8 serviceChangedUUID[];
extern CONST uint8 manuNameUUID[];
extern CONST uint8 serialNumUUID[];
extern CONST uint8 manuAddrUUID[];

/*********************************************************************
 * FUNCTIONS
 */
extern const uint8 *GATT_FindUUIDRec( uint8 *pUUID, uint8 len );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* GATT_UUID_H */
