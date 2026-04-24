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

/***************************************************************************
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : WWLIB32 library                          *
 *                                                                         *
 *                    File Name : DESCMGMT.H                               *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : August 3, 1994                           *
 *                                                                         *
 *                  Last Update : August 3, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef DESCMGMT_H
#define DESCMGMT_H


#ifndef WWSTD_H
#include "WWSTD.H"
#endif	   

//=====================================================================
// C type include files


// ====================================================================


// types
// These were taken from the legacy DOS header
//==========================================

// external functions
// ===================================================
extern uint32_t Map_Segment_To_Address(uint32_t address, uint32_t length);

// Assemble functions
extern uint16_t FixSelector(uint16_t sel);
extern uint16_t GetDs(void);
extern uint16_t GetCs(void);
extern void  GetDefaultSelectors(void);
extern uint16_t Get_Standard_Selector(void);


// Assembly data variables
extern uint16_t CodeSelector;
extern uint16_t DataSelector;
extern uint16_t ScreenSelector;
extern uint16_t GraphicsSelector;
extern uint16_t PspSelector;
extern uint16_t EnvSelector;
extern uint16_t DosMemSelector;
extern uint16_t Fp1167Selector;
extern uint16_t FpWeitekSelector;
extern uint16_t FpCyrixSelector;

#endif  // DESCMGMT_H

