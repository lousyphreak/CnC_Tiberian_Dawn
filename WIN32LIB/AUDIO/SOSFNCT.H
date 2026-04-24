/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/****************************************************************************

   File              : sosfnct.h

   Programmer(s)     : Don Fowler, Nick Skrepetos
   Date              :

   Purpose           : Include Files For Zortech C++ Compiler

   Last Updated      :

****************************************************************************
               Copyright(c) 1993,1994 Human Machine Interfaces
                            All Rights Reserved
****************************************************************************/

#ifndef  _SOS_FUNCTIONS
#define  _SOS_FUNCTIONS

uint16_t sosDIGILockMemory          (  void  );
uint16_t sosDIGIUnLockMemory        (  void  );
uint16_t sosDIGIInitSystem           (  char *, uint16_t  );
uint16_t sosDIGIUnInitSystem         (  void  );
uint16_t sosDIGIInitDriver           (  uint16_t, _SOS_HARDWARE  *,
                                    _SOS_INIT_DRIVER  *, uint16_t  *   );
uint16_t sosDIGIUnInitDriver         (  uint16_t, int32_t, int32_t  );
uint16_t sosDIGILoadDriver          (  uint16_t, uint16_t, char *  *, char *  *, char *, char *, uint16_t * );
uint16_t sosDIGIUnLoadDriver         (  uint16_t  );
uint16_t sosDIGIGetDeviceCaps        (  uint16_t, LPSOSDEVICECAPS  );

#ifdef PHARLAP
char * sosDIGIAllocateBuffer      (  uint16_t , uint16_t *, uint16_t * );
#else
char * sosDIGIAllocateBuffer      (  uint16_t , uint16_t *, uint16_t * );
#endif

uint16_t sosDIGIStopSample           (  uint16_t, uint16_t  );
uint16_t sosDIGISamplesPlaying       (  uint16_t  );
int32_t sosDIGISampleDone           (  uint16_t, uint16_t  );
int32_t sosDIGISampleFilling        (  uint16_t, uint16_t  );
uint16_t sosDIGIStartSample          (  uint16_t, _SOS_START_SAMPLE  *  );
uint16_t sosDIGIContinueSample       (  uint16_t, uint16_t, _SOS_START_SAMPLE  * );


uint16_t  sosDIGIDetectInit          (  char *  );
uint16_t  sosDIGIDetectUnInit        (  void  );
uint16_t  sosDIGIDetectFindHardware  (  uint16_t, _SOS_CAPABILITIES  *, uint16_t  * );
uint16_t  sosDIGIDetectFindFirst     (  _SOS_CAPABILITIES  *, uint16_t  *  );
uint16_t  sosDIGIDetectFindNext      (  _SOS_CAPABILITIES  *, uint16_t  *  );
uint16_t  sosDIGIDetectGetSettings   (  _SOS_HARDWARE  *  );
uint16_t  sosDIGIDetectGetCaps       (  uint16_t, _SOS_CAPABILITIES  *  );
uint16_t  sosDIGIDetectVerifySettings(  _SOS_HARDWARE  *  );
char *  sosGetErrorString          (  uint16_t  );

uint16_t  sosDIGILoadTimer           (  uint16_t , char *  *, char *  *, char *, char *, uint16_t * );
uint16_t  sosDIGIUnLoadTimer         (  uint16_t  );

uint16_t  sosTIMERRegisterEvent      ( uint16_t wCallRate, void (  * lpTimerEvent )( void ), uint16_t  *lpTimerHandle );
uint16_t  sosTIMERInitSystem               (  uint16_t, uint16_t  );
uint16_t  sosTIMERUnInitSystem             (  uint16_t  );
uint16_t  sosTIMERSetRate                  (  uint16_t  );
uint16_t  sosTIMERRemoveEvent              (  uint16_t );
uint16_t  sosTIMERAlterEventRate           (  uint16_t, uint16_t  );
uint16_t  sosTIMERGetEventRate             (  uint16_t  );
void  sosTIMEROldHandler            (  void  );
void  sosTIMERHandler               (  void  );

// functions in soscntl.c
uint16_t  sosDIGISetSampleVolume     ( uint16_t, uint16_t, uint16_t );
uint16_t  sosDIGIGetSampleVolume     ( uint16_t, uint16_t );
uint16_t  sosDIGISetChannel          ( uint16_t, uint16_t, uint16_t );
uint16_t  sosDIGIGetChannel          ( uint16_t, uint16_t );
uint16_t  sosDIGIGetBytesProcessed   ( uint16_t, uint16_t );
uint16_t  sosDIGIGetLoopCount        ( uint16_t, uint16_t );
uint16_t  sosDIGISetPanLocation      ( uint16_t, uint16_t, uint16_t );
uint16_t  sosDIGIGetPanLocation      ( uint16_t, uint16_t );
uint32_t sosDIGISetPitch            ( uint16_t, uint16_t, uint32_t );
uint32_t sosDIGIGetPitch            ( uint16_t, uint16_t );
uint16_t  sosDIGIGetDMAPosition      ( uint16_t );
uint16_t  sosDIGISetPanSpeed         ( uint16_t, uint16_t, uint16_t );
uint16_t  sosDIGIGetPanSpeed         ( uint16_t, uint16_t );
uint16_t  sosDIGIGetSampleID         ( uint16_t, uint16_t );
uint16_t  sosDIGIGetSampleHandle     ( uint16_t, uint16_t );
uint16_t  sosDIGISetMasterVolume     ( uint16_t, uint16_t );
#ifdef PHARLAP
void  sosFreeVDSPage          ( uint16_t, uint16_t, uint32_t );
uint16_t  sosAllocVDSPage         ( uint16_t *, uint16_t *, uint32_t * );
#else
uint16_t  sosAllocVDSPage            ( char * *, uint16_t *, uint16_t * );
void  sosFreeVDSPage             ( uint16_t, uint16_t, int32_t );
#endif


#ifdef PHARLAP
extern   int32_t    sosRealFree          ( int32_t );
extern   int32_t    _sos_read( uint16_t, char *, uint16_t, uint16_t * );
extern   int32_t    sosRealAlloc( int32_t, int32_t *, int32_t * );
extern   void   sosDRVFarMemCopy( char *, char *, uint16_t );
extern   int32_t    sosGetCS( void );
extern   int32_t    sosGetES( void );
#else
extern   int32_t    sosRealAlloc         ( int32_t, int32_t *, int32_t * );
extern   int32_t    sosRealFree          ( int32_t );
#endif

// sos driver functions
extern   uint16_t   sosDRVLockMemory     ( uint32_t, uint32_t );
extern   uint16_t   sosDRVUnLockMemory   ( uint32_t, uint32_t );
extern   void   sosDRVGetCapsInfo    ( char *, char *, _SOS_CAPABILITIES  * );
extern   void   sosDetDRVGetCapsInfo ( char *, char *, _SOS_CAPABILITIES  * );
extern   void   sosDRVGetCapsPtr     ( char *, char *, _SOS_CAPABILITIES  * );
extern   void   sosDRVInit           ( char *, char *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t );
extern   void   sosDRVStart          ( char *, char *, int32_t, int32_t );
extern   void   sosDRVSetRate        ( char *, char *, int32_t );
extern   void   sosDRVSetAction      ( char *, char * );
extern   void   sosDRVStop           ( char *, char * );
extern   void   sosDRVUnInit         ( char *, char * );
extern   void   sosDRVGetFillInfo    ( char *, char *, char *, int32_t, int32_t, int32_t, _SOS_FILL_INFO * );
extern   void   sosFillSampleStructs ( char *, char * );
extern   uint16_t   sosDetDRVExist       ( char *, char * );
extern   uint16_t   sosDetDRVGetSettings ( char *, char * );
extern   uint16_t   sosDetDRVVerifySettings( char *, uint16_t, uint16_t, uint16_t, char * );
extern   uint16_t   sosDIGIInitForWindows( uint16_t );
extern   uint16_t   sosDIGIUnInitForWindows( uint16_t );
extern   char *  sosAllocateFarMem      ( uint16_t, char *, uint16_t * );
extern   char *  sosCreateAliasCS       ( char * );
extern   void   sosFreeSelector        ( char *, uint32_t );
extern   char *  sosMAKEDOSPtr          ( char * );
extern   void   sosDetDRVSetEnvString  ( uint32_t, char * );
extern   char *   sosDetDRVGetEnvString  ( uint32_t );
extern   void   sosDetDRVEnvStringInit ( char *, char * );
extern   void   sosDRVSetupCallFunctions( char *, char *, char *, char * );
extern   uint16_t   sosDRVGetFreeMemory     ( void );
extern   uint16_t   sosDRVAllocVDSStruct    ( uint16_t, uint16_t *, uint16_t * );
extern   uint16_t   sosDRVFreeVDSStruct     ( uint16_t, uint16_t );
extern   uint16_t   sosDRVIsWindowsActive   ( void );
extern   uint16_t   sosDRVVDSGetBuffer    ( uint16_t );
extern   uint16_t   sosDRVVDSFreeBuffer   ( uint16_t );
extern   uint16_t   getDS( void );
extern   uint16_t   sosDRVMakeDMASelector   ( uint16_t );
extern   uint16_t   sosDRVFreeDMASelector   ( uint16_t );


extern   void   sosTIMERDRVInit( int32_t wRate, void (  * )( void ) );
extern   void   sosTIMERDRVUnInit( void );
extern   void   sosTIMERDRVHandler( void );
extern   void   sosTIMERDRVFHandler( void );
extern   void   sosTIMERDRVEnable( void );
extern   void   sosTIMERDRVDisable( void );
extern   void   sosTIMERDRVCallOld( void );
extern   void   sosTIMERDRVSetRate( uint16_t );
extern   void   sosDIGITimer_Start( void );
extern   void   sosDIGITimer_End( void );
extern   void   sosDIGIDrv_Start( void );
extern   void   sosDIGIDrv_End( void );

// external functions for handling system initialization and
// uninitialization
uint16_t  sosEXDIGInitDriver               (  uint16_t, uint16_t, uint16_t, char *,
                                          _SOS_HARDWARE  *, uint16_t *  );
uint16_t  sosEXDIGIUnInitDriver            (  void  );

uint16_t  sosEXDETFindDriver               (  uint16_t, char *, _SOS_HARDWARE  *,
                                          _SOS_CAPABILITIES  *  );

// memory locking prototypes
void  sosDIGICaps_Start( void );
void  sosDIGICaps_End( void );
void  sosDIGIErr_Start( void );
void  sosDIGIErr_End( void );
void  sosDIGITmr_Start( void );
void  sosDIGITmr_End( void );
void  sosDIGIStart_Start( void );
void  sosDIGIStart_End( void );
void  sosDIGIPlyng_Start( void );
void  sosDIGIPlyng_End( void );
void  sosDIGIRate_Start( void );
void  sosDIGIRate_End( void );
void  sosDIGIDone_Start( void );
void  sosDIGIDone_End( void );
void  sosDIGIDetec_Start( void );
void  sosDIGIDetec_End( void );
void  sosDIGIInit_Start( void );
void  sosDIGIInit_End( void );
void  sosDIGILoad_Start( void );
void  sosDIGILoad_End( void );
void  sosDIGICntl_Start( void );
void  sosDIGICntl_End( void );

#endif

