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
 *                 Project Name : Westwood Win32 Library                   *
 *                                                                         *
 *                    File Name : DDRAW.CPP                                *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : October 10, 1995                         *
 *                                                                         *
 *                  Last Update : October 10, 1995   []                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/


/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


#include "MISC.H"
#include "ra_messagebox.h"
#include <FUNCTION.H>
#include <GBUFFER.H>
#include <PALETTE.H>

WWDraw*		VideoDrawObject=NULL;	// Pointer to the active video backend
RAWindow*		MainWindow;					// Handle to programs main window
												// this is passed to SetCooperativeLevel
												// so the video backend knows which window is ours


PALETTEENTRY				PaletteEntries[256];		// 256 windows palette entries
WWPalette*	VideoPalette;					// Pointer to direct draw palette object
int32_t						FirstPaletteSet=FALSE;	// Is this the first time 'Set_Palette' has been called?
WWSurface*	PaletteSurface=NULL;
SurfaceMonitorClass	AllSurfaces;				//List of all direct draw surfaces
int32_t						CanVblankSync = TRUE;

int32_t						OverlappedVideoBlits = TRUE;	// Can video driver blit overlapped regions?

/*
** Function to call if we detect focus loss
*/
void (*Misc_Focus_Loss_Function)(void) = NULL;
void (*Misc_Focus_Restore_Function)(void) = NULL;


/***********************************************************************************************
 * Process_Draw_Result -- Does a message box based on the result of a DD command                 *
 *                                                                                             *
 * INPUT:		HRESULT result				- the result returned from the direct draw command		  *
 *             int     display_ok_msg	- should a message be displayed if command ok			  *                                                                                      *
 *                                                                                             *
 * OUTPUT:		none																									  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/27/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void Process_Draw_Result(HRESULT result, int32_t display_ok_msg)
{
	if (result == WWDRAW_OK) {
		if (display_ok_msg) {
			RA_ShowMessageBox("SDL draw request succeeded.", "Note", SDL_MESSAGEBOX_WARNING);
		}
		return;
	}

	const char* message = "SDL draw request failed.";
	if (result == WWDRAW_ERROR_SURFACELOST) {
		message = "SDL draw surface was lost.";
	} else if (result == WWDRAW_ERROR_INVALIDPARAMS) {
		message = "SDL draw request received invalid parameters.";
	} else if (result == WWDRAW_ERROR_NOPALETTEATTACHED) {
		message = "SDL draw surface has no palette attached.";
	}
	RA_ShowMessageBox(message, "Note", SDL_MESSAGEBOX_WARNING);
}



/***********************************************************************************************
 * Check_Overlapped_Blit_Capability -- See if video driver supports blitting overlapped regions*
 *                                                                                             *
 *  We will check for this by drawing something to a video page and blitting it over itself.   *
 * If we end up with the top line repeating then overlapped region blits dont work.            *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/7/96 5:06PM ST : Created                                                               *
 *=============================================================================================*/
void Check_Overlapped_Blit_Capability(void)
{

	/*
	** Assume we can until we find out otherwise
	*/
	OverlappedVideoBlits = TRUE;

	GraphicBufferClass test_buffer;

	test_buffer.Init (64, 64, NULL, 0, (GBC_Enum)GBC_VIDEOMEM);

	test_buffer.Clear();

	/*
	** Plot a pixel in the top left corner of the buffer.
	*/
	test_buffer.Put_Pixel(0, 0, 255);

	/*
	** Blit the buffer down by one line. If we end up with a vertical strip of pixel 255's then
	** overlapped blits dont work
	*/

	test_buffer.Blit(test_buffer, 0, 0, 0, 1, test_buffer.Get_Width(), test_buffer.Get_Height()-1);

	if (test_buffer.Get_Pixel (0 ,5) == 255) OverlappedVideoBlits = FALSE;
}



/***********************************************************************************************
 * Set_Video_Mode -- Initializes Direct Draw and sets the required Video Mode                  *
 *                                                                                             *
 * INPUT:  		int width   			- the width of the video mode in pixels						  *
 *					int height           - the height of the video mode in pixels                   *
 *					int bits_per_pixel	- the number of bits per pixel the video mode supports     *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
int32_t Set_Video_Mode(RAWindow* window, int32_t w, int32_t h, int32_t bits_per_pixel)
{
	HRESULT result;
	//
	// If there is not currently a direct draw object then we need to define one.
	//
	if ( VideoDrawObject == NULL ){
		//MessageBox(MainWindow, "In Set_Video_Mode. About to call WWDraw_Create.","Note", MB_ICONEXCLAMATION|MB_OK);
		result = WWDraw_Create(&VideoDrawObject);
		Process_Draw_Result(result, FALSE);
		if (result == WWDRAW_OK){
			VideoDrawObject->SetWindow(window);
		}else{
			return (FALSE);
		}
	}

	//
	// Set the required display mode with 8 bits per pixel
	//
	//MessageBox(MainWindow, "In Set_Video_Mode. About to call call SetDisplayMode.","Note", MB_ICONEXCLAMATION|MB_OK);
	result = VideoDrawObject->SetDisplayMode ( w , h , bits_per_pixel );
	if (result != WWDRAW_OK){
//		Process_Draw_Result(result, FALSE);
		VideoDrawObject->Release();
		VideoDrawObject = NULL;
		return(FALSE);
	}

	//
	// Create a direct draw palette object
	//
	//MessageBox(MainWindow, "In Set_Video_Mode. About to call CreatePalette.","Note", MB_ICONEXCLAMATION|MB_OK);
	result = VideoDrawObject->CreatePalette(&PaletteEntries[0], &VideoPalette);
	Process_Draw_Result(result, FALSE);
	if (result != WWDRAW_OK){
		return (FALSE);
	}

	Check_Overlapped_Blit_Capability();

	//MessageBox(MainWindow, "In Set_Video_Mode. About to return success.","Note", MB_ICONEXCLAMATION|MB_OK);
	//MessageBox(MainWindow, "In Set_Video_Mode. About to return success.","Note", MB_ICONEXCLAMATION|MB_OK);

	return (TRUE);

}

/***********************************************************************************************
 * Reset_Video_Mode -- Resets video mode and deletes Direct Draw Object                        *
 *                                                                                             *
 * INPUT:		none                                                                            *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void Reset_Video_Mode(void)
{
	HRESULT result;

	//
	// If a direct draw object has been declared and a video mode has been set
	// then reset the video mode and release the direct draw object.
	//
	if ( VideoDrawObject ) {
		result = VideoDrawObject->RestoreDisplayMode();
		Process_Draw_Result(result, FALSE);
		result = VideoDrawObject->Release();
		Process_Draw_Result(result, FALSE);

		VideoDrawObject = NULL;
	}
}




/***********************************************************************************************
 * Wait_Vert_Blank -- Waits for the start (leading edge) of a vertical blank                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/
extern int32_t ScreenWidth;
void Wait_Vert_Blank(void)
{
	if( ScreenWidth!=320 && CanVblankSync){
		HRESULT result = VideoDrawObject->WaitForVerticalBlank();
		if (result == E_NOTIMPL){
			CanVblankSync = FALSE;
			return;
		}
		Process_Draw_Result(result, FALSE);
	}
}





/***********************************************************************************************
 * Set_Palette -- set a direct draw palette                                                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to 768 rgb palette bytes                                                      *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/11/95 3:33PM ST : Created                                                             *
 *=============================================================================================*/
void Set_Video_Palette ( void *palette )
{

	/*
	** Trap null ptr
	*/
	if (!palette) return;

	int32_t	j;
	int32_t	k;
		uint8_t const *palette_get;

	if ( VideoDrawObject && PaletteSurface ){
		k=0;

		palette_get = (uint8_t const*)palette;

		for( j=0 ; j<768 ; j+=3 )
		{
			PaletteEntries[k].peRed = (uint8_t)((uint32_t)(*palette_get++) << 2);
			PaletteEntries[k].peGreen = (uint8_t)((uint32_t)(*palette_get++) << 2);
			PaletteEntries[k].peBlue = (uint8_t)((uint32_t)(*palette_get++) << 2);
			k++;
		}

		if ( !FirstPaletteSet ){
			//MessageBox(MainWindow, "In Set_Video_Palette. About to call SetPalette","Note", MB_ICONEXCLAMATION|MB_OK);
			PaletteSurface->SetPalette( VideoPalette );
			FirstPaletteSet=TRUE;
		}

		//MessageBox(MainWindow, "In Set_Video_Palette. About to call SetEntries","Note", MB_ICONEXCLAMATION|MB_OK);
		VideoPalette->SetEntries(0, 256, &PaletteEntries[0]);
	}
	//MessageBox(MainWindow, "Leaving Set_Video_Palette","Note", MB_ICONEXCLAMATION|MB_OK);

}





/***********************************************************************************************
 * Wait_Blit -- waits for the active blitter path to become idle                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07-25-95 03:53pm ST : Created                                                             *
 *=============================================================================================*/

void Wait_Blit (void)
{
	HRESULT	return_code;

	do {
		return_code = PaletteSurface->IsBlitDone() ? WWDRAW_OK : WWDRAW_ERROR_SURFACELOST;
	} while (return_code != WWDRAW_OK && return_code != WWDRAW_ERROR_SURFACELOST);

}



/***********************************************************************************************
 * SMC::SurfaceMonitorClass -- constructor for surface monitor class                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/3/95 3:23PM ST : Created                                                              *
 *=============================================================================================*/

SurfaceMonitorClass::SurfaceMonitorClass(void)
{
	for (int32_t i=0 ; i<MAX_SURFACES ; i++)
	{
		Surface[i]=NULL;
	}
	InFocus=FALSE;
	SurfacesRestored=FALSE;
}


/***********************************************************************************************
 * SMC::Add_Surface -- add a new surface to the list                                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to surface                                                                    *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *                 11/3/95 3:24PM ST : Created                                                 *
 *=============================================================================================*/
void SurfaceMonitorClass::Add_Surface (WWSurface* new_surface)
{
	if ( !Got_Surface_Already (new_surface) ){
		for (int32_t i=0 ; i<MAX_SURFACES ; i++)
		{
			if ( Surface[i]==NULL ){
				Surface[i]=new_surface;
				return;
			}
		}
	}
}


/***********************************************************************************************
 * SMC::Remove_Surface -- remove a direct draw surface from the list                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to Surface                                                                    *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/3/95 3:25PM ST : Created                                                              *
 *=============================================================================================*/

void SurfaceMonitorClass::Remove_Surface (WWSurface* old_surface)
{
	for (int32_t i=0 ; i<MAX_SURFACES ; i++)
	{
		if ( Surface[i]==old_surface ){
			Surface[i]=NULL;
			return;
		}
	}
}


/***********************************************************************************************
 * SMC::Got_Surface_Already -- check if a surface is already in the list                       *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to surface                                                                    *
 *                                                                                             *
 * OUTPUT:   True if surface is in list                                                        *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/3/95 3:25PM ST : Created                                                              *
 *=============================================================================================*/

int32_t SurfaceMonitorClass::Got_Surface_Already (WWSurface* test_surface)
{
	for (int32_t i=0 ; i<MAX_SURFACES ; i++)
	{
		if ( Surface[i]==test_surface ){
			return(TRUE);
		}
	}
	return (FALSE);
}

/***********************************************************************************************
 * SMC::Restore_Surfaces -- restore the direct draw surfaces in the list                       *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/3/95 3:26PM ST : Created                                                              *
 *=============================================================================================*/

void SurfaceMonitorClass::Restore_Surfaces (void)
{
	if (InFocus){
		/*
		** Call restore for each Direct Draw surface
		*/
		for (int32_t i=0 ; i<MAX_SURFACES ; i++)
		{
			if ( Surface[i] ){
				if (Surface[i]->Restore() != WWDRAW_OK){
					if (Misc_Focus_Loss_Function){
						Misc_Focus_Loss_Function();
					}
					return;
				}
			}
		}

		/*
		** PWG/ST: Now that we know all the surfaces are restored call
		** the function pointer to notify the program that it has
		** happened.  This function pointer is used to clear the pages,
		** etc.
		*/
		if (Misc_Focus_Restore_Function){
			Misc_Focus_Restore_Function();
		}

		SurfacesRestored = TRUE;

		/*
		** Restore the palette
		*/
		Set_Video_Palette (CurrentPalette);
	}
}


/***********************************************************************************************
 * SMC::Set_Surface_Focus -- set the InFocus flag to the given state                           *
 *                                                                                             *
 *  The InFocus flag is used to keep track of whether our application is currently in focus.   *
 *  We dont want to be restoring video surfaces when we are supposed to be running in the      *
 *  background.                                                                                *
 *                                                                                             *
 * INPUT:    bool in focus                                                                     *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/6/95 12:21PM ST : Created                                                             *
 *=============================================================================================*/

void SurfaceMonitorClass::Set_Surface_Focus ( int32_t in_focus )
{
	InFocus=in_focus;
}




/***********************************************************************************************
 * SMC::Release -- releases all direct draw surfaces                                           *
 *                                                                                             *
 *  Call this at the end of the game before called RestoreDisplayMode                          *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    6/6/96 12:23PM ST : Created                                                              *
 *=============================================================================================*/

void SurfaceMonitorClass::Release(void)
{
	/*
	** Call release for each Direct Draw surface
	*/
	for (int32_t i=0 ; i<MAX_SURFACES ; i++)
	{
		if ( Surface[i] ){
			Surface[i]->Release();
			Surface[i] = 0;
		}
	}

}
