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
 *                 Project Name : 32 bit library                           *
 *                                                                         *
 *                    File Name : MISC.H                                   *
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

#ifndef MISC_H
#define MISC_H

#include "win32_compat.h"
#include "sdl_draw.h"

extern	WWSurface*	PaletteSurface;

/*========================= C++ Routines ==================================*/


/*=========================================================================*/
/* The following prototypes are for the file: DDRAW.CPP							*/
/*=========================================================================*/
void 		Process_Draw_Result(HRESULT result, int32_t display_ok_msg);
int32_t 		Set_Video_Mode(RAWindow* window, int32_t w, int32_t h, int32_t bits_per_pixel);
void 		Reset_Video_Mode(void);
void 		Wait_Blit(void);

void Wait_Vert_Blank(void);void Set_Video_Palette (void *palette);
/*
** Pointer to function to call if we detect a focus loss
*/
extern	void (*Misc_Focus_Loss_Function)(void);
/*
** Pointer to function to call if we detect a surface restore
*/
extern	void (*Misc_Focus_Restore_Function)(void);


/*
 * Definition of surface monitor class
 *
 * This class keeps track of all the graphic buffers we generate in video memory so they
 *  can be restored after a focus switch.
*/

#define	MAX_SURFACES	20

class SurfaceMonitorClass {

	public:

		SurfaceMonitorClass();

		void	Add_Surface (WWSurface*);
		void	Remove_Surface (WWSurface*);
		int32_t	Got_Surface_Already (WWSurface*);
		void	Restore_Surfaces (void);
		void	Set_Surface_Focus ( int32_t in_focus );
		void	Release(void);

		int32_t	SurfacesRestored;

	private:

		WWSurface*	Surface[MAX_SURFACES];
		int32_t						InFocus;

};

extern	SurfaceMonitorClass	AllSurfaces;				//List of all direct draw surfaces


/*=========================================================================*/
/* The following variables are declared in: DDRAW.CPP								*/
/*=========================================================================*/
extern	WWDraw*	VideoDrawObject;
extern	RAWindow*			MainWindow;
extern int32_t				OverlappedVideoBlits;	// Can video driver blit overlapped regions?

/*=========================================================================*/
/* The following prototypes are for the file: EXIT.CPP							*/
/* Prog_End Must be supplied by the user program in startup.cpp				*/
/*=========================================================================*/
void  Prog_End(void);
void  Exit(int32_t errorval, const char *message, ...);

/*=========================================================================*/
/* The following prototypes are for the file: DELAY.CPP							*/
/*=========================================================================*/
void Delay(int32_t duration);
void Vsync(void);


/*=========================================================================*/
/* The following prototypes are for the file: FINDARGV.CPP						*/
/*=========================================================================*/

/*=========================================================================*/
/* The following prototypes are for the file: LIB.CPP								*/
/*=========================================================================*/
char *Find_Argv(char const *str);
void Mono_Mem_Dump(void const *databuf, int32_t bytes, int32_t y);
void Convert_RGB_To_HSV(uint32_t r, uint32_t g, uint32_t b, uint32_t *h, uint32_t *s, uint32_t *v);
void Convert_HSV_To_RGB(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);


/*=========================================================================*/
/* The following prototypes are for the file: VERSION.CPP						*/
/*=========================================================================*/

char const * Version(void);


/*=========================================================================*/
/* The following prototypes are for the file: IRANDOM.CPP						*/
/*=========================================================================*/
int32_t IRandom(int32_t minval, int32_t maxval);


/*========================= Assembly Routines ==============================*/


/*=========================================================================*/
/* The following prototypes are for the file: RANDOM.ASM							*/
/*=========================================================================*/

uint8_t  Random(void);
int32_t  Get_Random_Mask(int32_t maxval);

/*=========================================================================*/
/* The following prototype is for the file: SHAKESCR.ASM							*/
/*=========================================================================*/

void  Shake_Screen(int32_t shakes);

/*=========================================================================*/
/* The following prototypes are for the file: REVERSE.ASM						*/
/*=========================================================================*/

int32_t  Reverse_Long(int32_t number);
int16_t  Reverse_Short(int16_t number);
int32_t  Swap_Long(int32_t number);
#if (0)
/*=========================================================================*/
/* The following prototype is for the file: FACING8.ASM							*/
/*=========================================================================*/

int32_t  Desired_Facing8(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/*=========================================================================*/
/* The following prototype is for the file: FACING16.ASM							*/
/*=========================================================================*/

int32_t  Desired_Facing16(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/*=========================================================================*/
/* The following prototype is for the file: FACINGFF.ASM							*/
/*=========================================================================*/

int32_t  Desired_Facing256(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/*=========================================================================*/
/* The following prototype is for the file: FADING.ASM							*/
/*=========================================================================*/
#endif

void *  Build_Fading_Table(void const *palette, void const *dest, int32_t color, int32_t frac);
/*=========================================================================*/
/* The following prototype is for the file: CRC.ASM							*/
/*=========================================================================*/

int32_t  Calculate_CRC(void *buffer, int32_t length);

/*=========================================================================*/
/* The following prototypes are for the file: DETPROC.ASM						*/
/*=========================================================================*/

extern int16_t   Processor(void);
extern int16_t  Operating_System(void);
extern uint32_t Random_Mod(uint32_t mod);
#define random(mod) Random_Mod(mod)
//extern void  randomize ( void ) ;

extern int32_t  Clip_Rect ( int32_t * x , int32_t * y , int32_t * dw , int32_t * dh ,
	       	   			 int32_t width , int32_t height ) ;
extern int32_t  Confine_Rect ( int32_t * x , int32_t * y , int32_t dw , int32_t dh ,
	      	      			 int32_t width , int32_t height ) ;



/*=========================================================================*/
/* The following prototypes are for the file: OPSYS.ASM							*/
/*=========================================================================*/

extern int16_t OperationgSystem;


/*=========================================================================*/

#endif // MISC_H
