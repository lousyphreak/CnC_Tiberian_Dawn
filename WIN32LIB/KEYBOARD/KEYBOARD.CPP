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

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Keyboard Library                                    *
 *                                                                                             *
 *                    File Name : KEYBOARD.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Philip W. Gorrow                                             *
 *                                                                                             *
 *                   Start Date : 10/16/95                                                     *
 *                                                                                             *
 *                  Last Update : October 26, 1995   []                    *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WWKeyboardClass::Put -- Logic to insert a key into the keybuffer]                         *
 *   WWKeyboardClass::Get -- Logic to get a metakey from the buffer                            *
 *   WWKeyboardClass::Check -- Checks to see if a key is in the buffer                         *
 *   WWKeyboardClass::Put_Key_Message -- Translates and inserts wParam into Keyboard Buffer    *
 *   WWKeyboardClass::Buff_Get -- Lowlevel function to get a key from key buffer               *
 *   WWKeyboardClass::Get_Mouse_X -- Returns the mouses current x position in pixels           *
 *   WWKeyboardClass::Get_Mouse_Y -- returns the mouses current y position in pixels           *
 *   WWKeyboardClass::Get_Mouse_XY -- Returns the mouses x,y position via reference vars       *
 *   Check_Key -- compatability routine for old 32 bit library             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "KEYBOARD.H"
#include "SDLINPUT.H"
#include "TIMER.H"
#include "MONO.H"

void Message_Loop(void);

WWKeyboardClass *_Kbd;

namespace {

constexpr int32_t LEGACY_MIDDLE_MOUSE = 0x04;

}

/***********************************************************************************************
 * WWKeyboardClass::WWKeyBoardClass -- Construction for Westwood Keyboard Class                *
 *                                                                                             *
 * INPUT:		none							                                                        *
 *                                                                                             *
 * OUTPUT:     none							                                                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
WWKeyboardClass::WWKeyboardClass(void)
{
	_Kbd = this;
	//
	// Initialize the keyboard remap table for our system (note it would be bad if someone
	// switched keyboard modes after this happened.
	//
	memset(VKRemap, 0, sizeof(VKRemap));
	memset(AsciiRemap, 0, 2048);
	for (int16_t lp = 31; lp < 255; lp ++) {
		if (isprint(lp)) {
			int32_t vk_key = SDL_GameInput_AsciiToLegacyKey((uint8_t)lp);
			if (vk_key > 0 && vk_key < 2048) {
				AsciiRemap[vk_key] = (uint8_t)lp;
				VKRemap[lp] 	 = (uint8_t)(vk_key & 0xFF);
			}
		}
	}

	//
	// Build a remap table of the different keys which are affected by the caps lock and
	// the num lock.
	//
	memset(ToggleKeys, 0, sizeof(ToggleKeys));
	for (int16_t lp = 0; lp < 255; lp++ ) {
		if (isalpha(lp) && isupper(lp)) {
			ToggleKeys[lp] = 1;
		}
		if (lp >= 0x60 && lp <= 0x6F) {
			ToggleKeys[lp] = 2;
		}
	}

	//
	// Our buffer should start devoid of keys.
	//
	memset(Buffer, 0, sizeof(Buffer));
	Head				= 0;
	Tail				= 0;

	//
	// There should be no starting queued mouse events for us to have to worry
	// about.
	//
	MouseQX			= 0;
	MouseQY			= 0;
	MState			= 0;
	Conditional		= 0;
	CurrentCursor	= NULL;
}

/***********************************************************************************************
 * WWKeyboardClass::Buff_Get -- Lowlevel function to get a key from key buffer                 *
 *                                                                                             *
 * INPUT:		none                                                        						  *
 *                                                                                             *
 * OUTPUT:     int		- the key value that was pulled from buffer (includes bits)				  *                                                                                *
 *                                                                                             *
 * WARNINGS:   If the key was a mouse event MouseQX and MouseQY will be updated                *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/17/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int32_t WWKeyboardClass::Buff_Get(void)
{
	while (!Check()) {}										// wait for key in buffer
	int32_t 	temp		= Buffer[Head];						// get key out of the buffer
	int32_t   newhead	= Head;									// save off head for manipulation
	if (Is_Mouse_Key(temp)) {								// if key is a mouse then
		MouseQX	= Buffer[(Head + 1) & 255];			//		get the x and y pos
		MouseQY	= Buffer[(Head + 2) & 255];			//		from the buffer
		newhead += 3;		  									//		adjust head forward
	} else {
		newhead += 1;		  									//		adjust head forward
	}
	newhead	&= 255;
	Head		 = newhead;
	return(temp);
}

int32_t WWKeyboardClass::Is_Mouse_Key(int32_t key)
{
	key &= 0xFF;
	return (key == KN_LMOUSE || key == LEGACY_MIDDLE_MOUSE || key == KN_RMOUSE);
}


/***********************************************************************************************
 * WWKeyboardClass::Check -- Checks to see if a key is in the buffer                           *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int32_t WWKeyboardClass::Check(void)
{
	Message_Loop();
	uint16_t temp;											// store temp holding spot for key
	if (Head == Tail) return(FALSE);							// if no keys in buff then get out
	temp = Buffer[Head];											// get key out of the buffer
	return(temp);													// send it back to main program
}

/***********************************************************************************************
 * WWKeyboardClass::Get -- Logic to get a metakey from the buffer                              *
 *                                                                                             *
 * INPUT:		none                                                        						  *
 *                                                                                             *
 * OUTPUT:     int		- the meta key taken from the buffer.											  *
 *                                                                                             *
 * WARNINGS:	This routine will not return until a keypress is received							  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int32_t WWKeyboardClass::Get(void)
{
	int32_t temp,bits;										// store temp holding spot for key

	while (!Check()) {
		SDL_GameInput_Delay(1);
	}
	temp = Buff_Get();								// get key from the buffer

	bits = temp & 0xFF00;							// save of keyboard bits

	if (!(bits & WWKEY_VK_BIT)) {					// if its not a virtual key
		temp = AsciiRemap[temp&0x1FF] | bits;	//   convert to ascii equivalent
	}
	return(temp);							// return the key that we pulled out
}

/***********************************************************************************************
 * WWKeyboardClass::Put -- Logic to insert a key into the keybuffer]                           *
 *                                                                                             *
 * INPUT:		int	 	- the key to insert into the buffer          								  *
 *                                                                                             *
 * OUTPUT:     bool		- true if key is sucessfuly inserted.							              *
 *                                                                                             *
 * WARNINGS:   none							                                                        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int32_t WWKeyboardClass::Put(int32_t key)
{
	int32_t	temp		= (Tail + 1) & 255;
	if (temp != Head)
	{
		Buffer[Tail]	= (int16_t)key;

		//
		// Critical Line
		//
		Tail				= temp;
		return(TRUE);
	}
	return(FALSE);
}
/***********************************************************************************************
 * WWKeyboardClass::Put_Key_Message -- Translates and inserts wParam into Keyboard Buffer      *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/16/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int32_t WWKeyboardClass::Put_Key_Message(uint32_t vk_key, int32_t release, int32_t dbl)
{
	int32_t bits = 0;
	//
	// Get the status of all of the different keyboard modifiers.  Note, only pay attention
	// to numlock and caps lock if we are dealing with a key that is affected by them.  Note
	// that we do not want to set the shift, ctrl and alt bits for Mouse keypresses as this
	// would be incompatible with the dos version.
	//
	if (vk_key != KN_LMOUSE && vk_key != LEGACY_MIDDLE_MOUSE && vk_key != KN_RMOUSE) 	{
		int32_t	shift		= SDL_GameInput_IsModifierPressed(SDL_KMOD_SHIFT);
		int32_t	ctrl 		= SDL_GameInput_IsModifierPressed(SDL_KMOD_CTRL);
		int32_t	alt 		= SDL_GameInput_IsModifierPressed(SDL_KMOD_ALT);
		int32_t	caps		= SDL_GameInput_IsModifierToggled(SDL_KMOD_CAPS) && (ToggleKeys[vk_key] == 1);
		int32_t	nums		= SDL_GameInput_IsModifierToggled(SDL_KMOD_NUM) && (ToggleKeys[vk_key] == 2);

		//
		// Set the proper bits for whatever the key we got is.
		//
		if (shift || caps || nums) {
			bits |= WWKEY_SHIFT_BIT;
		}
		if (ctrl) {
			bits |= WWKEY_CTRL_BIT;
		}
		if (alt) {
			bits |= WWKEY_ALT_BIT;
		}
		}
	if (!AsciiRemap[vk_key|bits]) {
		bits |= WWKEY_VK_BIT;
	}
	if (release) {
		bits |= WWKEY_RLS_BIT;
	}
	if (dbl) {
		bits |= WWKEY_DBL_BIT;
	}
	//
	// Finally use the put command to enter the key into the keyboard
	// system.
	//
	return(Put(vk_key|bits));
}

void WWKeyboardClass::Clear(void)
{
	Head = Tail;
}

int32_t WWKeyboardClass::To_ASCII(int32_t key)
{
	if ( key & WWKEY_RLS_BIT)
	  return(KN_NONE);
	return(key);
}

int32_t WWKeyboardClass::Down(int32_t key)
{
	return SDL_GameInput_IsLegacyKeyPressed(key & 0xFF);
}

void WWKeyboardClass::Split(int32_t &key, int32_t &shift, int32_t &ctrl, int32_t &alt, int32_t &rls, int32_t &dbl)
{
	shift = (key & WWKEY_SHIFT_BIT)	!= 0;
	ctrl	= (key & WWKEY_CTRL_BIT)	!= 0;
	alt	= (key & WWKEY_ALT_BIT)		!= 0;
	rls	= (key & WWKEY_RLS_BIT)		!= 0;
	dbl	= (key & WWKEY_DBL_BIT)		!= 0;
	key	= (key & 0xFF);

}


void  Stop_Execution (void);





void Message_Loop(void)
{
	SDL_GameInput_Pump();
}



/***************************************************************************
 * CHECK_KEY -- compatability routine for old 32 bit library               *
 *                                                                         *
 * This routine checks to see if there is a key in the keyboard buffer     *
 * and returns it to the sender if there is.  It does not remove the key	*
 * from the buffer.																			*
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     The key that was pressed.                                   *
 *                                                                         *
 * WARNINGS:   You must declare a WWKeyboardClass object before calling    *
 *					this routine.																*
 *                                                                         *
 * HISTORY:                                                                *
 *   10/26/1995     : Created.                                             *
 *=========================================================================*/
int32_t Check_Key(void)
{
	if (!_Kbd) return(KA_NONE);
	return(_Kbd->Check() & ~WWKEY_SHIFT_BIT);
}

void Clear_KeyBuffer(void)
{
	if (!_Kbd) return;
	_Kbd->Clear();
}

int32_t Check_Key_Num(void)
{
	if (!_Kbd) return(KN_NONE);
	int32_t key		= _Kbd->Check();
	int32_t flags	= key & 0xFF00;
	key		   = key & 0x00FF;

	if (isupper(key)) {
		key = tolower(key);
		if ( !(flags & WWKEY_VK_BIT) ) {
			flags |= WWKEY_SHIFT_BIT;
		}
	}

	return(key | flags);
}

int32_t Get_Key_Num(void)
{
	if (!_Kbd) return(KN_NONE);
	int32_t key		= _Kbd->Get();
	int32_t flags	= key & 0xFF00;
	key		   = key & 0x00FF;

	if (isupper(key)) {
		key = tolower(key);
		if ( !(flags & WWKEY_VK_BIT) ) {
			flags |= WWKEY_SHIFT_BIT;
		}
	}
	return(key | flags);
}

int32_t KN_To_KA(int32_t key)
{
	if ( key & WWKEY_RLS_BIT) {
		return(KA_NONE);
	}
	if (!(key & WWKEY_VK_BIT)) {
		int32_t flags = key & 0xFF00;
		key		 = key & 0x00FF;
		if (flags & WWKEY_SHIFT_BIT) {
			key		= toupper(key);
			flags	  &= ~WWKEY_SHIFT_BIT;
		}
	}else{
		/*
		** If its a numeric keypad key then fix it up
		*/
		if ((key & 0xff) >= 0x60 && (key & 0xff) <= 0x69) {
			key = (key & 0xff) - 0x60 + KA_0;
		}
	}
	return(key);
}

int32_t KN_To_VK(int32_t key)
{
	if (!_Kbd) return(KN_NONE);
	if ( key & WWKEY_RLS_BIT) {
		return(KN_NONE);
	}

	int32_t flags = key & 0xFF00;
	if (!(flags & WWKEY_VK_BIT)) {
		key 		 = _Kbd->VKRemap[key & 0x00FF] | flags;
	}
	key &= ~WWKEY_VK_BIT;
	return(key);
}

int32_t Key_Down(int32_t key)
{
	if (!_Kbd) return(FALSE);
	return(_Kbd->Down(key));
}

int32_t Get_Key(void)
{
	int32_t retval;

	if (!_Kbd) return(KN_NONE);
	retval = _Kbd->Get() & ~WWKEY_SHIFT_BIT;
	if (retval & WWKEY_RLS_BIT) {
		retval = KN_NONE;
	}
	return(retval);
}
