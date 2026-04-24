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

   File              : sosdata.h

   Programmer(s)     : Don Fowler, Nick Skrepetos
   Date              :

   Purpose           : Include Files For Zortech C++ Compiler

   Last Updated      :

****************************************************************************
               Copyright(c) 1993,1994 Human Machine Interfaces
                            All Rights Reserved
****************************************************************************/

#ifndef  _SOS_DATA
#define  _SOS_DATA
#include <stdint.h>

#include <stddef.h>

extern   uint16_t     _sosDIGIData_Start;
extern   uint16_t     _sosDIGIData_End;
extern   uint16_t     _wSOSDriverLinear[];
extern   uint16_t     _wSOSTimerLinear[];
extern   char *    _lpSOSDriver[];
extern   char *    _lpSOSTimer[];
extern   char *    _lpSOSDriverCS[];
extern   char *    _lpSOSTimerCS[];
extern   int32_t     _wSOSDriverLoaded[];
extern   int32_t     _wSOSTimerLoaded[];
extern   int32_t     _wSOSDriverInitialized[];
extern   uint16_t     _wSOSOutputRate[];
extern   uint16_t     _wSOSDMABuffersize[];
extern   int32_t     _dwSOSDMABufferPhysical[];
extern   char *    _lpSOSDMABuffer[];
extern   int32_t     _wTimerUsed;
extern   void (  *_lpSOSFillHandler[] )( void );
extern   uint16_t     _wSOSTimerType[];
extern   uint16_t     _wSOSDriverType[];
extern   _SOS_SAMPLE  * _lpSOSSampleList[][ _MAX_VOICES ];
extern   uint16_t *      _lpSOSDMAIrqCount[];
extern   uint16_t *      _lpSOSDMAFillCount[];
extern   uint16_t        _wSOSTmrNextCount;
extern   void (   *_lpSOSOldTimer )( void );
extern   uint16_t     _wSOSDriverID[];
extern   _SOS_CAPABILITIES _sSOSDriverCaps[];
extern   uint16_t        _wSOSDMAPortList[];
extern   uint8_t        _bSOSDMAChannel[];
extern   _SOS_INIT_DRIVER     _sSOSDIGIInitDriver[];
extern   uint8_t  _pSOSDriverPath[];
extern   uint8_t  _pSOSTempDriverPath[];
extern   int32_t  _wTIMERUsed;
extern   uint16_t  _wTIMERValue;
extern   void (  * _lpTIMEREvents[] )( void );
extern   uint16_t _wTIMEREventRate[];
extern   uint16_t _dwTIMEREventFraction[];
extern   uint16_t _dwTIMEREventFractionCurrent[];
extern   uint8_t  _bSOSMIDITimerSongHandler[];
extern   uint8_t  _bSOSMIDISongHandle;
extern   uint16_t  _wSOSTimerMemHandle[];
extern   uint16_t  _wSOSDriverMemHandle[];
extern   uint16_t  _wSOSRealSeg[];

extern   _FILEHEADER    _sDETFileHeader;
extern   _DRIVERHEADER  _sDETDriverHeader;
extern   _FILEHEADER    sLOADFileHeader;
extern   _DRIVERHEADER  sLOADDriverHeader;
extern   int32_t           _wDETInitialized;
extern   uint16_t           _wDETLinear;
extern   char *          _lpDETDriverBuffer;
extern   char *          _lpDETDriverBufferCS;
extern   uint16_t           _hDETFile;
extern   uint32_t          _dwDETDriverIndex;
extern   uint16_t           _wDETDriverIndexCur;
extern   uint16_t           _wDETMemHandle;
extern   LPSOSDEVICECAPS _lpDETDeviceCaps;
extern   _SOS_CAPABILITIES _sDETCaps;
extern   char * _pSOSErrorStrings[];
extern   int32_t _wSOSBufferAllocated[];
extern   int32_t _wSOSSystemInitialized;
extern   VDS_STRUCT _sSOSVDSInfo;
extern   _SOS_FILL_INFO _sSOSFillInfo;
extern   uint16_t    _wSOSTimerEventIndex;
extern   uint16_t  _wSOSTimerEntered;
extern   uint16_t  _wSOSDriverSize[];
extern   uint16_t  _wSOSTimerSize[];

extern   uint16_t  _sosDIGIData1_Start;
extern   uint16_t  _sosDIGIData1_End;
extern   uint16_t  _sosDIGIData2_Start;
extern   uint16_t  _sosDIGIData2_End;
extern   uint8_t  _bTIMERInstalled;
extern   uint8_t  _bTIMERDPMI;
extern   uint16_t  wDetectPort;
extern   uint16_t  wDetectIRQ;
extern   uint16_t  wDetectDMA;
extern   uint16_t  wDetectParam;

#endif

