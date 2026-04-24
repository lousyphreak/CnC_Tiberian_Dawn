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
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Drawbuff - Westwood win95 library                            *
 *                                                                                             *
 *                    File Name : Iconcach.H                                                   *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : November 8th, 1995                                           *
 *                                                                                             *
 *                  Last Update : November 16th, 1995 [ST]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview: This file cantains definition of the IconCacheClass and associated non member     *
 *           function prototypes.                                                              *
 *                                                                                             *
 * Functions:                                                                                  *
 *  IconCacheClass::Get_Is_Cached -- member to allow access to private IsCached flag           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */




#ifndef	ICONCACH_H
#define	ICONCACH_H

#include	<TILE.H>
#define	ICON_WIDTH			24			// Icons must be this width to be cached
#define	ICON_HEIGHT			24			// Icons must be this height to be cached
#define	MAX_CACHED_ICONS	500		// Maximum number of icons that can be cached
#define	MAX_ICON_SETS		100		// Maximum number of icon sets that can be registered
#define	MAX_LOOKUP_ENTRIES 3000		// Size of icon index table


/*
** IconCacheClass for tracking individual icons cached into video memory
**
** Use Register_Icon_Set to identify a set of icons as cachable. Once registered, the icons
** will be cached automatically when drawn.
** Use Invalidate_Cached_Icons at the end of icon drawing to release the video memory used by the
** caching system.
** Restore_Cached_Icons may be used to reload the icons into video memory after a focus loss.
**
*/

class IconCacheClass {

	public:

		IconCacheClass (void);					// class constructor
		~IconCacheClass (void);					// class destructor

		void Restore(void);						// restore the surface
		int32_t Cache_It (void * icon_ptr);		// Cache the icon to video memory
		void Uncache_It (void);					// Restore the video memory and flag the icon as uncached
		void Draw_It (WWSurface* dest_surface , int32_t x_pixel, int32_t y_pixel, int32_t window_left , int32_t window_top , int32_t window_width , int32_t window_height);
		inline int32_t Get_Is_Cached(void);		// Return the IsCached member

		int32_t						TimesDrawn;		// counter of times cached icon has been drawn
		int32_t						TimesFailed;	// counter of times cached icon has failed to draw


	private:

		WWSurface*	CacheSurface;	// Ptr to direct draw surface where icon resides
		int32_t						IsCached;		// Flag to say whether an icon is cached
		int32_t						SurfaceLost;	// Flag to indicate that our icons surface has been lost
		int32_t						DrawFrequency;	// Number of times icon has been drawn
		void						*IconSource;	// Ptr to original icon data in system memory

};



/*
** Structure to keep track of registered icon sets
**
*/

typedef struct tIconSetType{
	IControl_Type	*IconSetPtr;				// Ptr to icon set data
	int32_t				IconListOffset;			// Offset into icon index table for this icon set
}IconSetType;


extern	IconCacheClass	CachedIcons[MAX_CACHED_ICONS];

extern void Invalidate_Cached_Icons (void);
extern void Restore_Cached_Icons (void);
extern void Register_Icon_Set (void *icon_data , int32_t pre_cache);

//
// Prototypes for assembly language procedures in STMPCACH.ASM
//
void Clear_Icon_Pointers (void);void Cache_Copy_Icon (void const *icon_ptr ,void * , int32_t);int32_t Is_Icon_Cached (void const *icon_data , int32_t icon);int32_t Get_Icon_Index (void *icon_ptr);int32_t Get_Free_Index (void);int32_t Cache_New_Icon (int32_t icon_index, void *icon_ptr);int32_t Get_Free_Cache_Slot(void);

extern	int32_t	CachedIconsDrawn;
extern	int32_t	UnCachedIconsDrawn;


/***********************************************************************************************
 * ICC::Get_Is_Cached -- member to allow access to the private IsCached flag                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   IsCached                                                                          *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/13/95 9:42AM ST : Created                                                             *
 *=============================================================================================*/
inline int32_t IconCacheClass::Get_Is_Cached (void)
{
	return (IsCached);
}





#endif	//ICONCACH_H

