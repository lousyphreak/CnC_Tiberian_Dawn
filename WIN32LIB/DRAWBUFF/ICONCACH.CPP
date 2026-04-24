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
 *                    File Name : Iconcach.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : November 8th, 1995                                           *
 *                                                                                             *
 *                  Last Update : November 13th, 1995 [ST]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview: This file cantains members of the IconCacheClass and associated non member        *
 *           functions. All functions are to do with caching individual icons from icon sets   *
 *           into video memory to improve the speed of subsequent drawing                      *
 *                                                                                             *
 * Functions:                                                                                  *
 *  Cache_New_Icon -- Call the Cache_It member to cache a registered icon to video memory      *
 *  Invalidate_Cached_Icons -- Uncache all the icons                                           *
 *  Restore_Cached_Icons -- restore cached icons after a focus loss                            *
 *  Register_Icon_Set -- register an icon set as cachable                                      *
 *  Get_Free_Cache_Slot -- find an empty cache slot                                            *
 *  IconCacheClass::IconCacheClass -- IconCacheClass constructor                               *
 *  IconCacheClass::~IconCacheClass -- IconCacheClass destructor                               *
 *  IconCacheClass::Restore -- restore the icons surface and recache it                        *
 *  IconCacheClass::Cache_It -- cache an icon into video memory                                *
 *  IconCacheClass::Uncache_It -- restore the video memory used by a cached icon               *
 *  IconCacheClass::Draw_It -- use the blitter to draw the cached icon                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "win32_compat.h"
#include "sdl_draw.h"
#include <SDL3/SDL_log.h>
#include "MISC.H"
#include "ICONCACH.H"
#include "GBUFFER.H"

static	WWLockData	IconLockData;

IconCacheClass	CachedIcons[MAX_CACHED_ICONS];

IconSetType		IconSetList[MAX_ICON_SETS];
int16_t				IconCacheLookup[MAX_LOOKUP_ENTRIES];

int32_t		CachedIconsDrawn=0;		//Counter of number of cache hits
int32_t		UnCachedIconsDrawn=0;	//Counter of number of cache misses
int32_t	CacheMemoryExhausted;	//Flag set if we have run out of video RAM




/***********************************************************************************************
 * Optimise_Video_Memory_Cache -- optimises usage of video memory                              *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   TRUE if memory was freed up                                                       *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/29/95 12:47PM ST : Created                                                            *
 *=============================================================================================*/
int32_t Optimize_Video_Memory_Cache (void)
{

	if (CacheMemoryExhausted &&
		(UnCachedIconsDrawn+CachedIconsDrawn > 1000) &&
		UnCachedIconsDrawn > CachedIconsDrawn){

		int32_t	cache_hits[MAX_CACHED_ICONS];
		int32_t	total_cache_misses=0;
		int32_t	total_cache_hits=0;
		int32_t	counter;
		int32_t	temp;
		int32_t	swapped;

		/*
		** make list of icons that have failed to cache more than 5 times
		*/
		for (counter=0 ; counter<MAX_CACHED_ICONS ; counter++){

			if (CachedIcons[counter].TimesFailed>5){
				total_cache_misses++;
			}
		}

		/*
		** Make list of icons that have been drawn less than 3 times
		*/
		for (counter=0 ; counter<MAX_CACHED_ICONS ; counter++){

			if (CachedIcons[counter].TimesDrawn<3){
				cache_hits[total_cache_hits++] = counter;
			}
		}


		/*
		** Sort drawn icons into order
		*/
		if (total_cache_hits > 1){
			for (int32_t i = 0 ; i<total_cache_hits ; i++){
				swapped=FALSE;
				for (int32_t j=0 ; j<total_cache_hits-1 ; j++){

					if (CachedIcons[cache_hits[j]].TimesDrawn > CachedIcons[cache_hits[j+1]].TimesDrawn){
						temp=cache_hits[j];
						cache_hits[j]=cache_hits[j+1];
						cache_hits[j+1]=temp;
						swapped = TRUE;
					}
				}
				if (!swapped) break;
			}
		}


		/*
		** Uncache icons up to the number of failed icons
		*/

		for (counter=0 ; counter<total_cache_misses && counter<total_cache_hits; counter++){
			CachedIcons[cache_hits[counter]].Uncache_It();
		}

		CacheMemoryExhausted=FALSE;
		CachedIconsDrawn=0;
		UnCachedIconsDrawn=0;
		return (TRUE);
	}
	return (FALSE);

}





/***********************************************************************************************
 * Cache_New_Icon -- cache a registered icon to video memory                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    icon_index -- index into registered icon table of icon to cache                   *
 *           icon_ptr -- ptr to icon data                                                      *
 *                                                                                             *
 * OUTPUT:   BOOL success                                                                      *
 *                                                                                             *
 * WARNINGS: icon must already have been registered and assigned an index                      *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/13/95 9:36AM ST : Created                                                             *
 *=============================================================================================*/
int32_t Cache_New_Icon (int32_t icon_index, void *icon_ptr)
{
	if (!CacheMemoryExhausted){
		return (CachedIcons[icon_index].Cache_It(icon_ptr));
	} else {
		CachedIcons[icon_index].TimesFailed++;
		if (Optimize_Video_Memory_Cache()){
			return (CachedIcons[icon_index].Cache_It(icon_ptr));
		} else {
			return (FALSE);
		}
	}
}





/***********************************************************************************************
 * Invalidat_Cached_Icons -- used to release any icons that have been cached                   *
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
 *    11/13/95 9:37AM ST : Created                                                             *
 *=============================================================================================*/
void Invalidate_Cached_Icons (void)
{
	for (int32_t i=0 ; i<MAX_CACHED_ICONS ; i++){
		CachedIcons[i].Uncache_It();
	}

	memset (&IconCacheLookup[0] , -1 ,MAX_LOOKUP_ENTRIES*sizeof(IconCacheLookup[0]));

	for (int32_t i=0 ; i<MAX_ICON_SETS ; i++){
		IconSetList[i].IconSetPtr=NULL;
	}

	CacheMemoryExhausted=FALSE;
}




/***********************************************************************************************
 * Restore_Cached_Icons -- re-cache icons into video memory after a loss of focus              *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: Assumes that the pointers that were originally used to cache the icons            *
 *           are still valid.                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/13/95 9:38AM ST : Created                                                             *
 *=============================================================================================*/
void Restore_Cached_Icons (void)
{
	for (int32_t i=0 ; i<MAX_CACHED_ICONS ; i++){
		CachedIcons[i].Restore();
	}
	CacheMemoryExhausted=FALSE;
}



/***********************************************************************************************
 * Register_Icon_Set -- used to register an icon set as cachable                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    icon_data - ptr to icon set                                                       *
 *           pre_cache -- should we pre-cache the icon data?                                   *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/13/95 9:39AM ST : Created                                                             *
 *=============================================================================================*/
void Register_Icon_Set (void *icon_data , int32_t pre_cache)
{

	for (int32_t i=0 ; i<MAX_ICON_SETS ; i++){
		if (!IconSetList[i].IconSetPtr){
			IconSetList[i].IconSetPtr = (IControl_Type*)icon_data;

			if (i){
				IControl_Type *previous_set = IconSetList[i-1].IconSetPtr;
				IconSetList[i].IconListOffset = IconSetList[i-1].IconListOffset + ((int32_t)previous_set->Count)*2;
				if (IconSetList[i].IconListOffset > MAX_LOOKUP_ENTRIES*2){
					IconSetList[i].IconSetPtr = NULL;
				}
			} else {
				IconSetList[i].IconListOffset = 0;
			}

			if (pre_cache){
				for (int32_t j=0 ; j<256 ; j++){
					Is_Icon_Cached(icon_data,j);
				}
			}
			return;
		}
	}
}



/***********************************************************************************************
 * Get_Free_Cache_Slot -- find a free slot in which to cache an icon                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   int - icon index                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/13/95 9:40AM ST : Created                                                             *
 *=============================================================================================*/
int32_t Get_Free_Cache_Slot (void)
{
	for (int32_t i=0 ; i<MAX_CACHED_ICONS ; i++){
		if (!CachedIcons[i].Get_Is_Cached()){
			return (i);
		}
	}
	return (-1);
}



/***********************************************************************************************
 * ICC::IconCacheClass -- constructor for icon cache class                                     *
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
 *    11/13/95 9:41AM ST : Created                                                             *
 *=============================================================================================*/
IconCacheClass::IconCacheClass (void)
{
	IsCached			=FALSE;
	SurfaceLost		=FALSE;
	DrawFrequency	=0;
	CacheSurface	=NULL;
	IconSource		=NULL;
}



/***********************************************************************************************
 * ICC::~IconCacheClass -- destructor for icon cache class                                     *
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
 *    11/13/95 9:41AM ST : Created                                                             *
 *=============================================================================================*/
IconCacheClass::~IconCacheClass (void)
{
	if (IsCached && CacheSurface){
		CacheSurface->Release();
	}
}




/***********************************************************************************************
 * ICC::Restore -- Restores the icons video surface memory and reloads it based on the original*
 *                 icon pointer                                                                *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: Relies on the icons original pointer still being valie                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/13/95 9:43AM ST : Created                                                             *
 *=============================================================================================*/
void IconCacheClass::Restore (void)
{
	if (IsCached && CacheSurface){
		CacheSurface->Restore();
		if (IconSource){
			Cache_It(IconSource);
		}
	}
}


/***********************************************************************************************
 * ICC::Cache_It -- allocate video memory and copy an icon to it                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    icon_ptr -- ptr to icon data                                                      *
 *                                                                                             *
 * OUTPUT:   bool -- success?                                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/13/95 9:44AM ST : Created                                                             *
 *=============================================================================================*/
int32_t IconCacheClass::Cache_It (void *icon_ptr)
{
	int32_t		return_value;

	/*
	** If we dont have a direct draw interface yet then just fail
	*/
	if (!VideoDrawObject) return(FALSE);

	/*
	** Set up the description of the surface we want to create
	*/
	if (!CacheSurface){
		if (WWDRAW_OK!=VideoDrawObject->CreateSurface(ICON_WIDTH, ICON_HEIGHT, &CacheSurface)){
			CacheMemoryExhausted = TRUE;
			return(FALSE);
		}

	}

	return_value=FALSE;
	/*
	** Lock the surface so we can copy the icon to it
	*/
	if (WWDRAW_OK== CacheSurface->Lock(NULL, &IconLockData)){
		/*
		** Copy the icon to the surface and flag that icon is cached
		*/
		Cache_Copy_Icon (icon_ptr , IconLockData.pixels , IconLockData.pitch);
		IsCached=TRUE;
		SurfaceLost=FALSE;
		IconSource=icon_ptr;
		return_value=TRUE;
	}
	CacheSurface->Unlock(NULL);
	return (return_value);
}


/***********************************************************************************************
 * ICC::Uncache_It -- release the video memory used to cache an icon                           *
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
 *    11/13/95 9:48AM ST : Created                                                             *
 *=============================================================================================*/
void IconCacheClass::Uncache_It(void)
{

	if (IsCached && CacheSurface){
		CacheSurface->Release();
		IsCached=FALSE;
		CacheSurface=NULL;
		IconSource=NULL;
		CacheMemoryExhausted=FALSE;
	}
}



/***********************************************************************************************
 * ICC::Draw_It -- use the blitter to draw a cached icon                                       *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    surface to draw to                                                                *
 *           x coord to draw to (relative to window)                                           *
 *           y coord to draw to (relative to window)                                           *
 *           window left coord                                                                 *
 *           window top coord                                                                  *
 *           window width                                                                      *
 *           window height                                                                     *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/13/95 9:48AM ST : Created                                                             *
 *=============================================================================================*/
void IconCacheClass::Draw_It (WWSurface* dest_surface , int32_t x_pixel, int32_t y_pixel, int32_t window_left , int32_t window_top , int32_t window_width , int32_t window_height)
{
	RECT		source_rectangle;
	RECT		dest_rectangle;
	int32_t		clip;
	HRESULT	return_code;

	/*
	** Set up the source and destination coordinates as required by direct draw
	*/
	source_rectangle.left 	= 0;
	source_rectangle.top  	= 0;
	source_rectangle.right	= ICON_WIDTH;
	source_rectangle.bottom	= ICON_HEIGHT;

	dest_rectangle.left 	= window_left+x_pixel;
	dest_rectangle.top  	= window_top+y_pixel;
	dest_rectangle.right	= dest_rectangle.left+ICON_WIDTH;
	dest_rectangle.bottom	= dest_rectangle.top+ICON_HEIGHT;

	/*
	** Clip the coordinates to the window
	*/
	if (dest_rectangle.left<window_left){
		source_rectangle.left += window_left-dest_rectangle.left;
		dest_rectangle.left=window_left;
	}

	if (dest_rectangle.right>=window_left+window_width){
		clip = dest_rectangle.right-(window_left+window_width);
		source_rectangle.right -= clip;
		dest_rectangle.right -= clip;
	}

	if (dest_rectangle.top<window_top){
		source_rectangle.top += window_top-dest_rectangle.top;
		dest_rectangle.top=window_top;
	}

	if (dest_rectangle.bottom>=window_top+window_height){
		clip = dest_rectangle.bottom-(window_top+window_height);
		source_rectangle.bottom -= clip;
		dest_rectangle.bottom -= clip;
	}

	if (source_rectangle.left>=source_rectangle.right){
		return;
	}

	if (source_rectangle.top>=source_rectangle.bottom){
		return;
	}

	/*
	** Do the blit
	*/
		return_code = dest_surface->Blit(&dest_rectangle, CacheSurface, &source_rectangle, false);

	if (return_code == WWDRAW_ERROR_SURFACELOST && Gbuffer_Focus_Loss_Function){
		Gbuffer_Focus_Loss_Function();
	}

	if ( return_code != WWDRAW_ERROR_SURFACELOST && return_code != WWDRAW_OK ) {
		char temp[100];
		sprintf(temp,"DD Error code %d\n", return_code & 0xFFFF);
		SDL_Log("%s", temp);
	}

	TimesDrawn++;

}
