Texas Instruments, Inc.

ZStack-CC2530 Release Notes

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

ZigBee 2007 Release
Version 2.5.1a
April 25, 2012


Notices:
 - ZStack-CC2530 has been certified for ZigBee/ZigBee-PRO compliance and
   has achieved certification from National Technical Systems (NTS) for the
   SE 1.1 profile (including OTA cluster). The SE 1.1 profile is certified to
   meet test criteria mandated by the SE 1.1 test specification (095312r30).

 - Z-Stack supports the ZigBee 2007 Specification, including features such
   as PanID Conflict Resolution, Frequency Agility, and Fragmentation. The
   ZigBee 2007 Specification (www.zigbee.org) defines two ZigBee stack
   profiles, ZigBee and ZigBee-Pro. ZStack-2.5.1 provides support for both
   of these profiles. See the Z-Stack Developer's Guide for details.

 - Z-Stack supports an IAR project to build "ZigBee Network Processor" (ZNP)
   devices. CC2530-based ZNP devices can be used with any host MCU that
   provides either an SPI or UART port to add ZigBee-Pro communication to
   existing or new designs. See the "CC2530ZNP Interface Specification" and
   "Z-Stack User's Guide for CC2530 ZigBee-PRO Network Processor - Sample
   Applications" documents for details and using the ZNP.

 - The library files have been built and tested with EW8051 version 8.10.4
   (8.10.4.40412) and may not work with other versions of the IAR tools.
   You can obtain the 8.10 installer and patches from the IAR website.

 - Z-Stack has been built and tested with IAR's CLIB library, which provides
   a light-weight C library which does not support Embedded C++. Use of DLIB
   is not recommended since Z-Stack is not tested with that library.

 - When programming a target for the first time with this release, make sure
   that you select the "Erase Flash" in the "Debugger->Texas Instruments->
   Download" tab in the project options. When programming completes, it is
   recommended that the "Erase Flash" box gets un-checked so that NV items
   are retained during later re-programming.

 - This is the final planned release with support for the following sample
   applications: SampleApp, SimpleApp, SerialApp and TransmitApp.

 - Please review the document "Upgrading To Z-Stack v2.5" for information
   about moving existing v2.4.0 applications to v2.5.1.


Changes:
 - [4049] The Monitor-Test (MT) API has been extended to provide ZDO leave
   indication to the host device. Refer to section 3.12.2.31 of the "Z-Stack
   Monitor and Test API" for use of this indication.

 - [4034] Added UK extensions for the Smart Energy 1.1 Message Cluster.
   This feature is enabled with the SE_UK_EXT compile option.

 - [4033] Added UK extensions for the Smart Energy 1.1 Alarms Cluster.
   This feature is enabled with the SE_UK_EXT compile option.

 - [4032] Added UK extensions for the Smart Energy 1.1 Prepayment Cluster.
   This feature is enabled with the SE_UK_EXT compile option.

 - [4031] Added UK extensions for the Smart Energy 1.1 Metering Cluster.
   This feature is enabled with the SE_UK_EXT compile option.

 - [4030] Added UK extensions for the Smart Energy 1.1 Price Cluster.
   This feature is enabled with the SE_UK_EXT compile option.

 - [4028] Added UK extensions for the Smart Energy 1.1 Device Management
   Cluster. This feature is enabled with the ZCL_DEVICE_MGMT compile option.

 - [4027] Added UK extensions for the Smart Energy 1.1 TOU Calendar Cluster.
   This feature is enabled with the ZCL_TOU compile option.

 - [4025] Added UK extensions for the Smart Energy 1.1 Mirroring function.
   This feature is enabled with the SE_MIRROR compile option.

 - [4021] Added the afDelete() function to remove an Endpoint. This feature
   is needed to support removal of a Mirror endpoint in SE-1.1 devices.

 - [3635] The Monitor-Test (MT) API has been extended to provide capability
   to set the transmit power level. Refer to section 3.8.1.21 of the "Z-Stack
   Monitor and Test API" for use of this command.

 - [3557] Added support for the Smart Energy 1.1 Tunneling Cluster. This
   feature is enabled with the ZCL_TUNNELING compile option.


Bug Fixes:
 - [4362] The original ZStack-CC2530-2.5.1 installer (dated April 04, 2012)
   was missing two files, that are used for serial bootloader support. These
   files are now located at: 
     "C:\Texas Instruments\ZStack-CC2530-2.5.1a\Projects\zstack\Utilities\
         BootLoad\CC2530\source\_hal_uart_isr.c"
     "C:\Texas Instruments\ZStack-CC2530-2.5.1a\Projects\zstack\Utilities\
         BootLoad\CC2530ZNP\source\_hal_uart_isr.c"

 - [4251] Changed the device type to PHYSICAL (0x0507) on Smart Energy OTA
   and Key Establishment endpoints. This aligns with updated SE-1.1 errata,
   that permits OTA and KE clusters to be on a separate endpoint.

 - [4247] Fixed a problem where the incorrect sequence number was being sent
   with the ephemeral data response during a Smart Energy CBKE process.

 - [4245] Fixed a timer rollover problem where, approximately every 4 minutes,
   the OSAL system clock would be erroneously increased.

 - [4231] Corrected the debugger parameters for the OAD Boot application to
   use 'ioCC2530F256.ddf' instead of the deprecated 'ioCC2530.ddf'.

 - [4135] Fixed a problem where a device's information would continue to be
   included in a neighbor's Link Status after the device left the network.

 - [4104] Corrected possible use of NULL pointer in MT_ZdoHandleExceptions()
   that could occur if memory allocation fails when processing MT commands.

 - [4043] Corrected an issue with saving/restoring Address Manager NV items
   that was causing unnecessary transport key commands to be sent to already
   commissioned devices when a new device joined the Coordinator/TrustCenter.

 - [4042] Fixed a problem where a 'recommissioned' router would not join
   another router that was previously its child.

 - [4037] Fixed a problem with the Security Manager where devices that had
   established a link key via PLKE (Partner Link Key Exchange) would not
   continue to communicate after after one had a power cycle.

 - [3992] Improved the 'soft reset' operation of the serial boot loader to
   maintain uninterrupted virtual COM port via USB when crossing between the
   Application and Bootloader functionality.

 - [3990] Fixed a problem where the Trust Center, using APS security, could
   not properly reflect a message to all of its bound recipients.

 - [3989] Fixed a problem where entries in the Address Manager table would
   not be available for re-use after devices had left the network.

 - [3988] Corrected the MT command 'UTIL_ADDRMGR_EXT_ADDR_LOOKUP' to accept
   an 8-byte Extended address parameter and return a 2-byte Network address.
   Refer to section 3.10.1.19 of the "Z-Stack Monitor and Test API".

 - [3986] Changed the error return from 'ZFailure' to 'ZMemError' when an
   attempt to create a Reflector tracking record fails on memory allocation.

 - [3984] Fixed a problem where a 'ZMemError' code could mistakenly be used as
   a data table index during CBKE under heavy radio traffic conditions.

 - [3947] Fixed a problem observed when more than one device was registered
   on the network with the Trust Center; removing one of the devices on the
   network caused the entire binding table and security manager entries table
   to not be restored after a power cycle.

 - [3942] Fixed a memory leak in the ZDSecMgrIsLinkKeyValid() function.

 - [3926] Fixed a problem where a device's Link Status message would include
   neighbor device information with other PAN IDs that were discovered during
   a network scan.

 - [3912] Fixed a problem where an SE client did not terminate a CBKE process
   when receiving a "fraudulent certificate" (cryptographically valid but has
   a mismatch associated EUI-64 identity field).
   
 - [3883] Fixed a problem in LED blinking logic where the LED would not blink
   if it was ON when the change to BLINK state was attempted.

 - [3741] Fixed a problem where a parent device would hold a message for its
   child, not knowing that the child had issued a "device announce" after
   after rejoining the network with a different parent.

 - [3675] Improved APS ACK response time during CBKE ephemeral data request
   that was causing an occasional time-out with an SE-1.0 test harness.

 - [3617] Removed a NWK frame header filter that limited the radius to twice
   the #defined value of DEF_NWK_RADIUS. All values are now allowed, including
   0xFF, which is used to indicate indefinite (unlimited) groupcast in the
   Green Power specification.

 - [3132] Fixed a MAC problem which, on rare occasions under heavy traffic,
   would attempt to free unallocated heap memory, causing an assert.


Memory Sizes:
 - The CC2530 has 256K bytes of Flash memory to store executable program
   and non-volatile (NV) memory, and 8K bytes of RAM for program stack and
   data variables. Actual usage of Flash and RAM memory is variable, of course,
   and dependent on the specific application. Developers can adjust various
   parameters, such as, program stack size and dynamic memory heap size
   to meet their specific needs.

 - The following table provides a comparison of Flash and RAM sizes for one
   of the sample applications provided with ZStack - GenericApp that is found
   in the installed ..\Projects\zstack\Samples\GenericApp\CC2530DB folder. In
   most ZStack sample applications, generic heap settings are used which have
   been selected to accomodate a wide range of applications. For this example,
   heap settings were: Coordinator/Router = 3K bytes, EndDevice = 2K bytes.
   See the "Heap Memory Management" document for details on profiling heap
   memory usage.

 - Memory sizes (Flash / RAM) are shown below for the 3 ZigBee device types,
   ZigBee-PRO, with/without Security, and compiled to run on the SmartRF05EB
   board with CC2530EM module. See the Z-Stack User's Guide for more details.

   Security On
   ===========
   Coordinator   159.7K / 6.7K
   Router        159.3K / 6.7K
   End-Device    129.2K / 5.3K

   Security Off
   ============
   Coordinator   150.7K / 6.7K
   Router        149.1K / 6.7K
   End-Device    121.2K / 5.2K


Known Issues:
 - To disable security at build time, use the "SECURE=0" compile option. Do
   not attempt to disable security by setting the SECURITY_LEVEL to zero.

 - SerialApp is not reliable when used for high-speed, large file transfers -
   the receiving application occasionally drops a byte.

 - ZOAD is a simple application to demonstrate Over-Air-Download (OAD)
   and is not intended for manufacturing or production usage.

 - The ZDO Complex Descriptor is not supported.

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
