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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : WWLIB												*
 *                                                                         *
 *                    File Name : PALETTE.C											*
 *                                                                         *
 *                   Programmer : BILL STOKES										*
 *                                                                         *
 *                   Start Date : 6/20/91												*
 *                                                                         *
 *                  Last Update : August 2, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Note: This module contains dependencies upon the video system,				*
 * specifically Get_Video_Mode().														*
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Set_Palette -- sets the current palette											*
 *   Set_Palette_Color -- Set a color number in a palette to the data.     *
 *	  Fade_Palette_To -- Fades the current palette into another					*
 *   Determine_Bump_Rate -- determines desired bump rate for fading        *
 *   Bump_Palette -- increments the palette one step, for fading           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*
********************************* Includes **********************************
*/
#include <string.h>
#include <WWSTD.H>
#include <PALETTE.H>
#include <TIMER.H>
/*
********************************* Constants *********************************
*/

/*
********************************** Globals **********************************
*/
uint8_t CurrentPalette[768];

void Set_Video_Palette(void *palette);
void WWDraw_Flush_Present(void);
extern uint8_t *InterpolationPalette;
extern int32_t InterpolationPaletteChanged;


/*
******************************** Prototypes *********************************
*/

PRIVATE void  Determine_Bump_Rate(void *palette, int32_t delay, int16_t *ticks, int16_t *rate);
PRIVATE int32_t  Bump_Palette(void *palette1, uint32_t step);
PRIVATE void Mark_Interpolation_Palette_Dirty(void);

PRIVATE void Mark_Interpolation_Palette_Dirty(void)
{
	InterpolationPalette = CurrentPalette;
	InterpolationPaletteChanged = TRUE;
}

/*
******************************** Code *********************************
*/

/***************************************************************************
 * Set_Palette -- sets the current palette											*
 *                                                                         *
 * INPUT:                                                                  *
 *		void *palette		- palette to set												*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/25/1994 SKB : Created.                                             *
 *   04/27/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
void  Set_Palette(void *palette)
{

	#if(IBM)
		Set_Palette_Range(palette);
	#else
		Copy_Palette(palette,CurrentPalette);
   	LoadRGB4(&Main_Screen->ViewPort,palette,32L);
	   LoadRGB4(AltVPort,palette,32L);
	#endif

}	/* end of Set_Palette */


/***************************************************************************
 * Set_Palette_Color -- Set a color number in a palette to the data.       *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *		void *palette	- palette to set color in										*
 *		int color		- which color index to set										*
 *		void *data		- RGB data for color												*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/25/1994 SKB : Created.                                             *
 *   04/27/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
void  Set_Palette_Color(void *palette, int32_t color, void *data)
{
	/*
	---------------------- Return if 'palette' is NULL -----------------------
	*/
	if (!palette) return;

	/*
	------------------- Change the color & set the palette -------------------
	*/
	#if(IBM)
		memcpy(&((uint8_t *)palette)[color * RGB_BYTES], data, RGB_BYTES);
		Set_Palette_Range(palette);
	#else
		palette[color] = *(uint16_t*)data;
		Set_Palette(palette);
	#endif

}	/* end of Set_Palette */


/***************************************************************************
 *	Fade_Palette_To -- Fades the current palette into another					*
 *                                                                         *
 * This will allow the palette to fade from current palette into the 		*
 * palette that was passed in.  This can be used to fade in and fade out.	*
 *                                                                         *
 * INPUT:      																				*
 *    char  *palette1 - this is the palette to fade to.           			*
 *		unsigned int delay		 -	fade with this timer count down							*
 *		void *callback  - user-defined callback function							*
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * WARNINGS:   none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/20/1991  BS : Created.                                             *
 *=========================================================================*/
void Fade_Palette_To(void *palette1, uint32_t delay, void (*callback)() )
{
	int32_t	changed;	// Flag that palette has changed this tick.
	int16_t	jump;		// Gun values to jump per palette set.
	uint32_t	timer;		// Tick count timer used for timing.
	int16_t	ticksper;	// The ticks (fixed point) per bit jump.
	int32_t	tickaccum;


	extern void (*cb_ptr)(void);	// callback function pointer

//	(void *)cb_ptr = callback;
	cb_ptr = callback;

	/*
	---------------------- Return if 'palette1' is NULL ----------------------
	*/
	if (!palette1)
		return;

	/*
	--------------------------- Get the bump rate ----------------------------
	*/
	Determine_Bump_Rate(palette1, delay, &ticksper, &jump);

	tickaccum = 0;							// init accumulated elapsed time
	timer = WWTickCount.Time(); 				// timer = current time
	do {
		changed = FALSE;

		tickaccum += ticksper;			// tickaccum = time of next change * 256
		timer += (tickaccum >> 8);		// timer = time of next change (rounded)
		tickaccum &= 0x0FF;				// shave off high byte, keep roundoff bits

		changed = Bump_Palette(palette1, jump);	// increment palette
		if (changed && !callback) {
			WWDraw_Flush_Present();
		}

		/*
		.................. Wait for time increment to elapse ..................
		*/
		if (changed) {
			while (static_cast<uint32_t>(WWTickCount.Time()) < timer) {
				/*
				................. Update callback while waiting .................
				*/
				if (callback) {
#if LIB_EXTERNS_RESOLVED
					Sound_Callback();		// should be removed!
#endif
					(*cb_ptr)();
				}
				if (static_cast<uint32_t>(WWTickCount.Time()) < timer) {
					SDL_Delay(1);
				}
			}
		}

#if LIB_EXTERNS_RESOLVED
		Sound_Callback();		// should be removed!
#endif
		if (callback) {
			(*cb_ptr)();
		}
	} while (changed);

}	/* end of Fade_Palette_To */


/***************************************************************************
 * Determine_Bump_Rate -- determines desired bump rate for fading          *
 *                                                                         *
 * INPUT:                                                                  *
 *		unsigned char *palette	- palette to fade to												*
 *		int delay		- desired time delay in 60ths of a second					*
 *		short *ticks		- output: loop ticks per color jump							*
 *		short *rate		- output: color gun increment rate							*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none																						*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/27/1994 BR : Converted to 32-bit                                   *
 *   08/02/1994 SKB : Made private                                         *
 *=========================================================================*/
PRIVATE void  Determine_Bump_Rate(void *palette, int32_t delay, int16_t *ticks,
	int16_t *rate)
{
	int32_t gun1;		// Palette 1 gun value.
	int32_t gun2;		// Palette 2 gun value.
	int32_t diff;		// Maximum color gun difference.
	int32_t tp;			// Temporary tick accumulator.
	int32_t index;		// Color gun working index.
	int32_t t;			// Working tick intermediate value.
	int32_t adiff;		// Absolute difference between guns.

	/*
	------------------------ Find max gun difference -------------------------
	*/
	diff = 0;
	for (index = 0; index < PALETTE_BYTES; index++) {
		gun1 = ((uint8_t *)palette)[index];
		gun2 = CurrentPalette[index];
		adiff = ABS(gun1-gun2);
		diff = MAX(diff, adiff);
	}

	/*------------------------------------------------------------------------
	ticks = (total time delay ) / (max gun diff)
	The value is computed based on (delay * 256), for fixed-point math;
	the lower bits represent the leftover from the division;  'ticks' is
	returned still shifted, so the low bits can be used to accumulate the
	time more accurately; the caller must shift the accumulated value down
	8 bits to determine the actual elapsed time!
	------------------------------------------------------------------------*/
	t = ((int32_t)delay) << 8;
	if (diff) {
		t /= diff;
		t = MIN((int32_t)t, (int32_t)0x7FFF);
	}
	*ticks = (int16_t)t;

	/*------------------------------------------------------------------------
	Adjust the color gun rate value if the time to fade is faster than can
	reasonably be performed given the palette change, ie if (ticks>>8)==0,
	and thus less than 1/60 of a second
	------------------------------------------------------------------------*/
	tp = *ticks;
	*rate = 1;
	while (*rate <= diff && *ticks < 256) {
		*ticks += tp;
		*rate += 1;
	}

}	/* end of Determine_Bump_Rate */


/***************************************************************************
 * Bump_Palette -- increments the palette one step, for fading             *
 *                                                                         *
 * INPUT:                                                                  *
 *		palette1		- palette to fade towards											*
 *		step			- max step amount, determined by Determine_Bump_Rate		*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		FALSE = no change, TRUE = changed												*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/27/1994 BR : Created.                                              *
 *   08/02/1994 SKB : Made private                                         *
 *=========================================================================*/
#if(IBM)
PRIVATE int32_t  Bump_Palette(void *palette1, uint32_t step)
{
	int32_t	changed=FALSE;				// Flag that palette has changed this tick.
	int32_t	index;						// Index to DAC register gun.
	int32_t	gun1,gun2;					// Palette 1 gun value.
	uint8_t	palette[PALETTE_BYTES];	// copy of current palette

	/*
	---------------------- Return if 'palette1' is NULL ----------------------
	*/
	if (!palette1)
		return (FALSE);


	/*
	------------------------ Copy the current palette ------------------------
	*/
	memcpy(palette, CurrentPalette, 768);

	/*
	----------------------- Loop through palette bytes -----------------------
	*/
	for (index = 0; index < PALETTE_BYTES; index++) {
		gun1 = ((uint8_t *)palette1)[index];
		gun2 = palette[index];

		/*
		............. If the colors match, go on to the next one ..............
		*/
		if (gun1 == gun2) continue;

		changed = TRUE;

		/*
		.................. Increment current palette's color ..................
		*/
		if (gun2 < gun1) {
			gun2 += step;
			gun2 = MIN(gun2, gun1);		// make sure we didn't overshoot it
		}
		/*
		.................. Decrement current palette's color ..................
		*/
		else {
			gun2 -= step;
			gun2 = MAX(gun2, gun1);		// make sure we didn't overshoot it
		}

		palette[index] = (uint8_t)gun2;
	}

	/*
	----------------- Set current palette to the new palette -----------------
	*/
	if (changed) {
		Set_Palette(&palette[0]);
	}

	return (changed);

}	/* end of Bump_Palette */

#else

	/* This is already implemented in asm on the Amiga */

#endif

void (*cb_ptr)(void);	// callback function pointer

/**************************** End of palette.cpp ***************************/
void  Set_Palette_Range(void *palette){
	if (!palette) {
		return;
	}

	memcpy(CurrentPalette, palette, 768);
	Mark_Interpolation_Palette_Dirty();
	Set_Video_Palette(CurrentPalette);
}


void Set_Palette_Register(int32_t number, int32_t red, int32_t green, int32_t blue){
	uint8_t *entry;

	if (number < 0 || number >= 256) {
		return;
	}

	entry = &CurrentPalette[number * 3];
	entry[0] = (uint8_t)MAX(0, MIN(red, 63));
	entry[1] = (uint8_t)MAX(0, MIN(green, 63));
	entry[2] = (uint8_t)MAX(0, MIN(blue, 63));
	Mark_Interpolation_Palette_Dirty();
	Set_Video_Palette(CurrentPalette);
}
