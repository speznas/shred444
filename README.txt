Texas Instruments, Inc.

CC2540 Bluetooth Low Energy Software Development Kit
Release Notes


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

Version 1.1a
Aug 10, 2011

Changes and Enhancements:
   
- The thermometer profile sample application has been updated to support stored measurements. The TI_BLE_Sample_Applications_Guide has been updated to match these changes.

Known Issues:

 - Use of the NV memory (to save application data or BLE Host bonding
   information) during a BLE connection may cause an unexpected disconnect.
   The likelihood of this happening increases with frequent usage, especially
   when using short connection intervals. The cause is related to the NV wear
   algorithm which at some point may cause an NV page erase which can disrupt
   system real-time processing. It is therefore recommended that the NV memory
   be used sparingly, or only when a connection is not active.

 - Duplicate filtering does not work when scan is used in combination with a
   connection.

For technical support please visit the Texas Instruments Bluetooth low energy
E2E Forum:

http://e2e.ti.com/support/low_power_rf/f/538.aspx

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

Version 1.1
July 13, 2011

Notices:

- This version of the Texas Instruments BLE stack and software features several
  changes, enhancements, and bug fixes from v1.0. Details of these can be found 
  below.


Changes and Enhancements:
   
- All projects have been migrated from IAR v7.51A to IAR v.7.60. In order to
  build all projects, be sure to upgrade and have IAR v7.60. Also, be sure to
  download and install all of the latst patches from IAR for full CC2540
  support.

- The stack now supports up to 3 simultaneous connection as a central / master
  device, with a few constraints:
   	
	- All connection intervals must be a multiple of the minimum connection
	  interval (i.e. the minimum connection interval is the greatest common
	  denominator of all connection intervals).
   	 
   	- The minimum connection interval allowed is 25ms when using more than
   	  one connection.

   	- When more than one connection is active, only one data packet per
   	  connection event will be allowed in each direction.
   	  
   	- Scanning is not supported while in a connection. The consequences of
   	  this is that device discovery is not possible while in a connection.
   	  Therefore, to discover and connect to multiple devices, the device
   	  discovery must occur before the first connection is established.   	

- Several new sample projects are included, with examples of many different BLE
  applications / profiles. Full details on the sample applications can be found
  in the BLE Sample Applications Guide, which can be accessed from the Windows
  Start Menu. These sample applications implement various functions. Some are
  based on adopted Bluetooth specifications, some are based on draft
  specifcations, and others are custom designed by Texas Instruments. These
  projects should serve as good examples for various other BLE applications.

- The following updates have been made to BTool (more information on these
  updates can be found in the CC2540DK-MINI User Guide which can be downloaded
  here: http://www.ti.com/lit/pdf/swru270):
	
	- Improved GUI and robustness.
	
	- All functions on the GUI been updated to handle multiple simultaneous
	  connections.
	
	- A new "Pairing / Bonding" tab has been added, allowing link
	  encryption and authentication, passkey entry, and saving / loading of
	  long-term key data (from bonding) to file.
	  
	- Ability to "Cancel" a link establishment while the dongle is
	  initiating.
   	
- The following additional new controller stack features are included in this
  release:
  
	- Support for multiple simultaneous connections as a master (details
	  above)
	
	- HCI Vendor Specific function HCI_EXT_SetSCACmd allows you to specify
	  the exact sleep clock accuracy as any value from 0 to 500 PPM, in
	  order	to support any crystal accuracy with optimal power consumption.
	  This feature is only available for slave / peripheral applications.
	
	- HCI Vendor Specific function HCI_EXT_SetMaxDtmTxPowerCmd allows you
	  to set the maximum transmit output power for Direct Test Mode. This
	  allows you to perform use the LE Transmitter Test command with power
	  levels less than +4dBm.
	  
	- A master device can now advertise while in a connection.
	
	- New production test mode (PTM) has been added allowing the CC2540 to
	  run Direct Test Mode (DTM) while connected to a tester using a
	  "single-chip" BLE libary.
	  
	- The controller now uses DMA to more efficiently encrypt and decrypt
	  packets. All BLE projects must now define HAL_AES_DMA=TRUE in the
	  preprocessor settings when using the v1.1 libraries.
	  
- The following additional new host stack features are included in this
  release:

	- A new GAP central role profile for single-chip embedded central
	  applications is included, with functions similar to the GAP
	  peripheral role profile. The SimpleBLECentral project serves as an
	  example of an application making use of the central role profile.
	
	- The GAP peripheral role has been optimized to significantly improve
	  power consumption while advertising with small amounts of data by
	  no longer transmitting non-significant bytes from in the
	  advertisement and scan response data.

- The following additional new application / profile features are included in
  this release:
  
  	- The GAP peripheral bond manager has been replaced with a general GAP
	  bond manager, capable of managing bond data for both peripheral and
	  central role devices. The gap peripheral bond manager has been
	  included for legacy support; however it is recommend to switch to the
	  general GAP bond manager (gapbondmgr.c/h).
	
	- The bond manager also now manages the storage of client
	  characteristic configurations for each bond as per the Bluetooth 4.0
	  spec.
  	
  	- The simple GATT profile has a new fifth characteristic. This
	  characteristic is 5 bytes long, and has readable permissions only
	  while in an authenticated connection. It should serve as a reference
	  for development of other profiles which require an encrypted link.
  	
  	- All GATT profiles have been updated to properly handle client
  	  characteristic configurations for both single and multiple
  	  connections. Characteristic configurations now get reset to zero
  	  (notifications / indications off) after a connection is terminated,
  	  and the bond manager now stores client characteristic configurations
  	  for bonded devices so that they are remembered for next time when the 
  	  device reconnects.
  	  
  	- Added linker configuration file for support of 128kB flash versions
  	  of the CC2540. An example is included in the SimpleBLEPeripheral
  	  project.
  	  
  	- The SimpleBLEPeripheral project "CC2540 Slave" configuration has been
  	  updated to better support the SmartRF05EB + CC2540EM hardware
  	  platform, making use of the LCD display.


Bug Fixes:

 - The following bugs have been fixed in the controller stack:

	- Scanning now working for master devices with power savings enabled.
	
	- RSSI reads no longer require a data packet to update.
	
	- Improved stablity when using very high slave latency setting
	
	- HCI LE direct test modes now working properly.
	
	- HCI Read Local Supported Features now returns the proper value.
	
	- Use of two advertising channels now works.
	
	- When connecting to a device on the whitelist, the correct peer device
	  address is returned to the host.  
	
 - The following bugs have been fixed in the host stack:
 
	- Pairing no longer fails when either device is using a static, private 
	  resolvable, or private non-resolvable address.

 - The following bugs have been fixed in the profiles / applications:
 
	- Reading of RSSI with peripheral role profile now working.
	
	- Peripheral role profile now allows all legal whitelist modes.
	
	- Can now connect with short connection intervals (such as 7.5ms), 
	  since bond manager now reads data from NV memory upon initialization
	  rather than immediately after a connection is established. Pairing
	  still may not be stable when using the bond manager with very short
	  connection intervals (for reason noted in "Known Issues" below)


Known Issues:

 - Use of the NV memory (to save application data or BLE Host bonding
   information) during a BLE connection may cause an unexpected disconnect.
   The likelihood of this happening increases with frequent usage, especially
   when using short connection intervals. The cause is related to the NV wear
   algorithm which at some point may cause an NV page erase which can disrupt
   system real-time processing. It is therefore recommended that the NV memory
   be used sparingly, or only when a connection is not active.

 - Duplicate filtering does not work when scan is used in combination with a
   connection.


For technical support please visit the Texas Instruments Bluetooth low energy
E2E Forum:

http://e2e.ti.com/support/low_power_rf/f/538.aspx


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

Version 1.0
October 7, 2010

Notices:
 - The Texas Instruments Bluetooth® low energy (BLE) software development kit 
   includes all necessary software to get started on the development of 
   single-mode BLE applications using the CC2540 system-on-chip. It includes 
   object code with the BLE protocol stack, a sample project and applications 
   with source code, and BTool, a Windows PC application for testing BLE 
   applications. In addition to the software, the kit contains documentation, 
   including a developer’s guide and BLE API guide.

 - For complete information on the BLE software development kit, please read
   the developer's guide:
   
	BLE Software Developer's Guide:
	<Install Directory>\Documents\TI_BLE_Software_Developer's_Guide.pdf
	(Also can be accessed through the Windows Start Menu)
 
 - The following additional documentation is included:
 	 	
 	BLE API Guide:
 	<Install Directory>\Documents\BLE_API_Guide_main.htm
 	
 	Vendor Specific HCI Guide:
 	<Install Directory>\Documents\TI_BLE_Vendor_Specific_HCI_Guide.pdf
 	
 	HAL Drive API Guide:
 	<Install Directory>\Documents\hal\HAL Driver API.pdf
 	
 	OSAL API Guide:
 	<Install Directory>\Documents\osal\OSAL API.pdf
 	
 - The following software projects are included, all built using IAR Embedded
   Workbench v7.51A:
   
   	SimpleBLEPeripheral:
   	<Install Directory>\Projects\ble\SimpleBLEPeripheral\CC2540DB\SimpleBLEPeripheral.eww

	HostTestRelease:
	<Install Directory>\Projects\ble\HostTestApp\CC2540\HostTestRelease.eww

  - The following Windows PC application is included:
  
  	BTool:
  	<Install Directory>\Projects\BTool\BTool.exe
  	(Also can be accessed through the Windows Start Menu)
 
Changes:
 - Initial Release

Bug Fixes:
 - Initial Release

Known Issues:
 - Use of the NV memory (to save application data or BLE Host bonding
   information) during a BLE connection may cause an unexpected disconnect.
   The likelihood of this happening increases with frequent usage, especially
   when using short connection intervals. The cause is related to the NV wear
   algorithm which at some point may cause an NV page erase which can disrupt
   system real-time processing. It is therefore recommended that the NV memory
   be used sparingly, or only when a connection is not active.

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
