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
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* PROJECT
*     VQAPlay32 library. (32-Bit protected mode)
*
* FILE
*     drawer.c
*
* DESCRIPTION
*     Frame drawing and page flip control.
*
* PROGRAMMER
*     Bill Randolph
*     Denzil E. Long, Jr.
*
* DATE
*     June 26, 1995
*
*----------------------------------------------------------------------------
*
* PUBLIC
*     VQA_Configure_Drawer - Configure the drawer routines.
*
* PRIVATE
*     Select_Frame             - Selects frame to draw and preforms frame
*                                skip.
*     Prepare_Frame            - Process/Decompress frame information.
 *     DrawFrame_Buffer         - Draw a frame to a buffer.
 *     PageFlip_Nop             - Do nothing page flip.
 *     UnVQ_Nop                 - Do nothing UnVQ.
*     Mask_Rect                - Sets non-drawable rectangle in image.
*     Mask_Pointers            - Mask vector pointer that are in the mask
*                                rectangle.
*
****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <VQ.H>
#include <VQM32/PALETTE.H>
#include "UNVQ.H"
#include "VQAPLAYP.H"
#include <VQM32/COMPRESS.H>

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/
static int32_t Select_Frame(VQAHandleP *vqap);
static void Prepare_Frame(VQAData *vqabuf);




static int32_t DrawFrame_Buffer(VQAHandle *vqa);
static int32_t PageFlip_Nop(VQAHandle *vqa);

#ifndef PHARLAP_TNT
static void  UnVQ_Nop(uint8_t *codebook, uint8_t *pointers,
		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth);
#else
static void  UnVQ_Nop(uint8_t *codebook, uint8_t *pointers,
		char * buffer, uint32_t blocksperrow, uint32_t numrows,
		uint32_t bufwidth);
#endif

#if(0)
static void Mask_Rect(VQAHandle *vqa, uint32_t x1, uint32_t y1,
		uint32_t x2, uint32_t y2);
static void Mask_Pointers(VQAData *vqabuf);
#endif

static int drawer_trace_enabled(void)
{
	static int enabled = -1;
	if (enabled == -1) {
		enabled = (getenv("RA_TRACE_STARTUP") != NULL) ? 1 : 0;
	}
	return (enabled);
}

static void trace_buffer_frame(VQAHandle *vqa, VQAFrameNode *curframe,
		uint8_t const *buffer)
{
	static uint32_t traced_frames = 0;
	uint32_t image_width;
	uint32_t image_height;
	uint32_t pixel_count;
	uint32_t nonzero = 0;
	uint32_t sample_count;
	uint32_t index;

	if (!drawer_trace_enabled() || traced_frames >= 8 || curframe == NULL
			|| buffer == NULL || curframe->Codebook == NULL
			|| curframe->Codebook->Buffer == NULL || curframe->Pointers == NULL) {
		return;
	}

	image_width = ((VQAHandleP *)vqa)->Header.ImageWidth;
	image_height = ((VQAHandleP *)vqa)->Header.ImageHeight;
	pixel_count = image_width * image_height;
	sample_count = (pixel_count < 16) ? pixel_count : 16;

	for (index = 0; index < pixel_count; ++index) {
		if (buffer[index] != 0) {
			++nonzero;
		}
	}

	fprintf(stderr,
			"[vqa] draw frame=%d flags=0x%08x pal=%d ptr=%02x%02x%02x%02x cb=%02x%02x%02x%02x pix_nonzero=%u/%u first=",
			(int)curframe->FrameNum,
			(unsigned)curframe->Flags,
			(int)curframe->PaletteSize,
			(unsigned)curframe->Pointers[0],
			(unsigned)curframe->Pointers[1],
			(unsigned)curframe->Pointers[2],
			(unsigned)curframe->Pointers[3],
			(unsigned)curframe->Codebook->Buffer[0],
			(unsigned)curframe->Codebook->Buffer[1],
			(unsigned)curframe->Codebook->Buffer[2],
			(unsigned)curframe->Codebook->Buffer[3],
			(unsigned)nonzero,
			(unsigned)pixel_count);
	for (index = 0; index < sample_count; ++index) {
		fprintf(stderr, "%s%02x", (index == 0) ? "" : ",",
				(unsigned)buffer[index]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
	++traced_frames;
}

/****************************************************************************
*
* NAME
*     VQA_Configure_Drawer - Configure the drawer routines.
*
* SYNOPSIS
*     VQA_Configure_Drawer(VQA)
*
*     void VQA_Configure_Drawer(VQAHandleP *);
*
* FUNCTION
*     Configure the drawing system for the current movie and configuration
*     options.
*
* INPUTS
*     VQA - Pointer to private VQAHandle.
*
* RESULT
*     NONE
*
****************************************************************************/

void VQA_Configure_Drawer(VQAHandleP *vqap)
{
	VQAData   *vqabuf;
	VQAConfig *config;
	VQAHeader *header;
	VQADrawer *drawer;
	int32_t      origin;
	int32_t      blkdim;

	/* Dereference commonly used data members for quicker access. */
	vqabuf = vqap->VQABuf;
	drawer = &vqabuf->Drawer;
	header = &vqap->Header;
	config = &vqap->Config;
	origin = (config->DrawFlags & VQACFGF_ORIGIN);

	/*-------------------------------------------------------------------------
	 * SET THE DRAW POSITION OF THE MOVIE.
	 *
	 * X1 = -1 -- Center image of the X axis, otherwise use X1 value.
	 * Y1 = -1 -- Center image of the Y axis, otherwise use Y1 value.
	 *-----------------------------------------------------------------------*/
	if ((config->X1 == -1) && (config->Y1 == -1)) {
		drawer->X1 = ((drawer->ImageWidth - header->ImageWidth) / 2);
		drawer->Y1 = ((drawer->ImageHeight - header->ImageHeight) / 2);

		drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
		drawer->Y2 = ((drawer->Y1 + header->ImageHeight) - 1);
	} else {
		switch (origin) {
			default:
			case VQACFGF_TOPLEFT:
				drawer->X1 = config->X1;
				drawer->Y1 = config->Y1;
				drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
				drawer->Y2 = ((drawer->Y1 + header->ImageHeight) - 1);
				break;

			case VQACFGF_BOTLEFT:
				drawer->X1 = config->X1;
				drawer->Y1 = (drawer->ImageHeight - config->Y1);
				drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
				drawer->Y2 = ((drawer->Y2 - header->ImageHeight) - 1);
				break;

			case VQACFGF_BOTRIGHT:
				drawer->X1 = (drawer->ImageWidth - config->X1);
				drawer->Y1 = (drawer->ImageHeight - config->Y1);
				drawer->X2 = (drawer->X1 - header->ImageWidth);
				drawer->Y2 = (drawer->Y1 - header->ImageHeight);
				break;
		}
	}

	/*-------------------------------------------------------------------------
	 * INITIALIZE THE UNVQ ROUTINE FOR THE SPECIFIED VIDEO MODE AND BLOCK SIZE.
	 *-----------------------------------------------------------------------*/

	/* Pre-compute commonly used values for speed. */
	drawer->BlocksPerRow = header->ImageWidth / header->BlockWidth;
	drawer->NumRows = header->ImageHeight / header->BlockHeight;
	drawer->NumBlocks = drawer->BlocksPerRow * drawer->NumRows;
	blkdim = BLOCK_DIM(header->BlockWidth, header->BlockHeight);

	/* Initialize draw routine vectors to a NOP routine in order to prevent
	 * a crash.
	 */
	vqabuf->UnVQ = UnVQ_Nop;
	vqabuf->Page_Flip = PageFlip_Nop;

	/* If the client specifies buffering then go ahead an set the unvq
	 * vector. All of the buffered modes use the same unvq routines.
	 */
	if (config->DrawFlags & VQACFGF_BUFFER) {
		switch (blkdim) {
			#if(VQABLOCK_2X2)
			case BLOCK_2X2:
				vqabuf->UnVQ = UnVQ_2x2;
				break;
			#endif

			#if(VQABLOCK_2X3)
			case BLOCK_2X3:
				vqabuf->UnVQ = UnVQ_2x3;
				break;
			#endif

			#if(VQABLOCK_4X2)
			case BLOCK_4X2:
				vqabuf->UnVQ = UnVQ_4x2;
				break;
			#endif

			#if(VQABLOCK_4X4)
			case BLOCK_4X4:
				vqabuf->UnVQ = (header->Version <= VQAHD_VER2)
						? UnVQ_4x4_V2
						: UnVQ_4x4;
				break;
			#endif

			default:
				break;
 		}
 	}

	/* Initialize the draw vectors for the SDL-buffered movie path. */
	vqabuf->Draw_Frame = DrawFrame_Buffer;
	drawer->ScreenOffset = ((drawer->ImageWidth * drawer->Y1) + drawer->X1);
}


/****************************************************************************
*
* NAME
*     Select_Frame - Selects frame to draw and preforms frame skip.
*
* SYNOPSIS
*     Error = Select_Frame(VQA)
*
*     int32_t Select_Frame(VQAHandleP *);
*
* FUNCTION
*     Select a frame to draw. This is were the frame skipping/delay is
*     performed.
*
* INPUTS
*     VQA - Pointer to private VQAHandle.
*
* RESULT
*     Error - 0 if successful, or VQAERR_??? error code.
*
****************************************************************************/

static int32_t Select_Frame(VQAHandleP *vqap)
{
	VQAData      *vqabuf;
	VQADrawer    *drawer;
	VQAConfig    *config;
	VQAFrameNode *curframe;
	int32_t         curtime;
	int32_t         desiredframe;

	/* Dereference commonly used data members for quicker access. */
	config = &vqap->Config;
	vqabuf = vqap->VQABuf;
	drawer = &vqabuf->Drawer;
	curframe = drawer->CurFrame;

	/* Make sure the current frame is drawable. If the frame is not ready
	 * then we must wait for the loader to catch up.
	 */
	if ((curframe->Flags & VQAFRMF_LOADED) == 0) {
		drawer->WaitsOnLoader++;
		return (VQAERR_NOBUFFER);
	}

	/* If single stepping then return with the next frame.*/
	if (config->OptionFlags & VQAOPTF_STEP) {
		drawer->LastFrame = curframe->FrameNum;
		return (0);
	}

	/* Find the frame # we should play (rounded to nearest frame): */
	curtime = VQA_GetTime(vqap);
	desiredframe = ((curtime * config->FrameRate) / VQA_TIMETICKS);

	#if(VQAMONO_ON)
	drawer->DesiredFrame = desiredframe;
	#endif

	/* Handle the cases where the player is going so fast that it's not time 
	 * to draw this frame yet.
	 *
	 * - If the Drawer is using a slower frame rate than the Loader, use a
	 *   delta-time-based wait; otherwise, use the frame number as the wait.
	 */
	if (config->DrawRate != config->FrameRate) {
		if (curtime - drawer->LastTime < (VQA_TIMETICKS / config->DrawRate)) {
			return (VQAERR_NOT_TIME);
		}
	} else {
		if (curframe->FrameNum > desiredframe) {
			return (VQAERR_NOT_TIME);
		}
	}

	/* Make sure we draw at least 5 frames per second */
	if ((curframe->FrameNum - drawer->LastFrame) >= (config->FrameRate / 5)) {
		drawer->LastFrame = curframe->FrameNum;
		return (0);
	}

	/* If frame skipping is disabled then draw every frame. */
	if (config->DrawFlags & VQACFGF_NOSKIP) {
		drawer->LastFrame = curframe->FrameNum;
		return (0);
	}

	/* Handle the case where the player is going too slow, so we have to skip 
	 * some frames:
	 *
	 * - If this is a Key Frame, draw it
	 * - If this frame's # is less than what we're supposed to draw, skip it
	 *   (Because the 1st 'desiredframe' will be 0, FrameNum MUST be typecast 
	 *   to signed int16_t for the comparison; otherwise, the comparison uses 
	 *   UWORDs, and the first frame is always skipped.)
	 * - If this is a palette-set frame, set the palette before skipping it
	 * - Loop until we get the frame we need, or there's no frames available
	 */
	while (1) {

		/* No frame available; return */
		if ((curframe->Flags & VQAFRMF_LOADED) == 0) {
			return (VQAERR_NOBUFFER);
		}

		/* Force drawing of a Key Frame */
		if (curframe->Flags & VQAFRMF_KEY) {
			break;
		}

		/* Skip the frame */
		if (curframe->FrameNum < desiredframe) {

			/* Handle a palette in a skipped frame:
			 *
			 * - Stash the palette in Drawer.Palette_24
			 * - Set the Drawer.Flags VQADRWF_SETPAL bit, to tell the page-flip
			 *   routines that this palette must be set
			 */
			if (curframe->Flags & VQAFRMF_PALETTE) {

				/* Un-LCW if needed */
				if (curframe->Flags & VQAFRMF_PALCOMP) {
					curframe->PaletteSize = LCW_Uncompress((char *)curframe->Palette
							+ curframe->PalOffset, (char *)curframe->Palette,
							vqabuf->Max_Pal_Size);

					curframe->Flags &= ~VQAFRMF_PALCOMP;
				}

				/* Stash the palette */
				memcpy(drawer->Palette_24, curframe->Palette, curframe->PaletteSize);
				drawer->CurPalSize = curframe->PaletteSize;
				drawer->Flags |= VQADRWF_SETPAL;
			}

			/* Invoke callback with NULL screen ptr */
			if (config->DrawerCallback != NULL) {
				if ((config->DrawerCallback(NULL, curframe->FrameNum)) != 0) {
					return (VQAERR_EOF);
				}
			}

			/* Skip the frame */
			curframe->Flags = 0L;
			curframe = curframe->Next;
			drawer->CurFrame = curframe;
			drawer->NumSkipped++;
		} else {
			break;
		}
	}
	
	drawer->LastFrame = curframe->FrameNum;
	drawer->LastTime = curtime;

	return (0);
}


/****************************************************************************
*
* NAME
*     Prepare_Frame - Process/Decompress frame information.
*
* SYNOPSIS
*     Prepare_Frame(VQAData)
*
*     void Prepare_Frame(VQAData *);
*
* FUNCTION
*     Decompress and preprocess the various frame elements (codebook,
*     pointers, palette, etc...)
*
* INPUTS
*     VQAData - Pointer to VQAData structure.
*
* RESULT
*     NONE
*
****************************************************************************/

static void Prepare_Frame(VQAData *vqabuf)
{
	VQADrawer    *drawer;
	VQAFrameNode *curframe;
	VQACBNode    *codebook;

	/* Dereference commonly used data members for quicker access. */
	drawer = &vqabuf->Drawer;
	curframe = drawer->CurFrame;
	codebook = curframe->Codebook;

	/* Decompress the codebook, if needed */
	if (codebook->Flags & VQACBF_CBCOMP) {

		/* Decompress the codebook. */
		LCW_Uncompress((char *)codebook->Buffer + codebook->CBOffset,
				(char *)codebook->Buffer, vqabuf->Max_CB_Size);

		/* Mark as uncompressed for the next time we use it */
		codebook->Flags &= (~VQACBF_CBCOMP);
	}

	/* Decompress the palette, if needed */
	if (curframe->Flags & VQAFRMF_PALCOMP) {
		curframe->PaletteSize = LCW_Uncompress((char *)curframe->Palette +
				curframe->PalOffset,(char *)curframe->Palette,vqabuf->Max_Pal_Size);

		/* Mark as uncompressed */
		curframe->Flags &= ~VQAFRMF_PALCOMP;
	}

	/* Decompress the pointer data, if needed */
	if (curframe->Flags & VQAFRMF_PTRCOMP) {
		LCW_Uncompress((char *)curframe->Pointers + curframe->PtrOffset,
				(char *)curframe->Pointers, vqabuf->Max_Ptr_Size);

		/* Mark as uncompressed */
		curframe->Flags &= ~VQAFRMF_PTRCOMP;
	}

	/* Mask the pointers */
	#if(0)
	Mask_Pointers(vqabuf);
	#endif
}








/****************************************************************************
*
* NAME
*     DrawFrame_Buffer - Draw a frame to a buffer.
*
* SYNOPSIS
*     Error = DrawFrame_Buffer(VQA)
*
*     int32_t DrawFrame_Buffere(VQAHandle *);
*
* FUNCTION
*
* INPUTS
*     VQA - Pointer to VQA handle.
*
* RESULT
*     Error - 0 if successful, otherwise VQAERR_??? error code.
*
****************************************************************************/

static int32_t DrawFrame_Buffer(VQAHandle *vqa)
{
	VQAData      *vqabuf;
	VQADrawer    *drawer;
	VQAFrameNode *curframe;
	VQAConfig    *config;
	uint8_t *pal;
	int32_t         palsize;
	int32_t         slowpal;
	int32_t         rc;

	#ifndef PHARLAP_TNT
	uint8_t *buff;
	#else
	char *        buff;
	#endif

	/* Dereference data members for quicker access. */
	config = &((VQAHandleP *)vqa)->Config;
	vqabuf = ((VQAHandleP *)vqa)->VQABuf;
	drawer = &vqabuf->Drawer;

	/* Check our "sleep" state */
	if (!(vqabuf->Flags & VQADATF_DSLEEP)) {

		/* Find the frame to draw */
		if ((rc = Select_Frame((VQAHandleP *)vqa)) != 0) {
			return (rc);
		}

		/* Uncompress the frame data */
		Prepare_Frame(vqabuf);
	}
 
	/* Wait for Update_Enabled to be set low */
	if (vqabuf->Flags & VQADATF_UPDATE) {
		vqabuf->Flags |= VQADATF_DSLEEP;
		return (VQAERR_SLEEPING);
	}

	if (vqabuf->Flags & VQADATF_DSLEEP) {
		drawer->WaitsOnFlipper++;
		vqabuf->Flags &= (~VQADATF_DSLEEP);
	}

	/* Dereference current frame for quicker access. */
	curframe = drawer->CurFrame;
	pal = curframe->Palette;
	palsize = curframe->PaletteSize;
	slowpal = (config->OptionFlags & VQAOPTF_SLOWPAL) ? 1 : 0;

	#ifndef PHARLAP_TNT
	buff = (uint8_t *)(drawer->ImageBuf + drawer->ScreenOffset);
	#else
	FP_SET(buff, drawer->ImageBuf + drawer->ScreenOffset, 0x14);
	#endif

	/* Un-VQ the image */
	vqabuf->UnVQ(curframe->Codebook->Buffer, curframe->Pointers, buff,
			drawer->BlocksPerRow, drawer->NumRows, drawer->ImageWidth);
	trace_buffer_frame(vqa, curframe, buff);

	/* Update data for mono output */
	drawer->LastFrameNum = curframe->FrameNum;

	/* Tell the flipper which frame to use */
	vqabuf->Flipper.CurFrame = curframe;

	/* Set the page-avail flag for the flipper */
	vqabuf->Flags |= VQADATF_UPDATE;

	/* Buffered playback still needs to apply frame palettes before the
	 * client callback copies the image to the visible page.
	 */
	if (curframe->Flags & VQAFRMF_PALETTE) {
		WaitNoVB(vqabuf->VBIBit);
		WaitVB(vqabuf->VBIBit);

		if (!(config->OptionFlags & VQAOPTF_PALOFF)) {
			SetPalette(pal, palsize, slowpal);
		}

		if ((config->NotifyFlags & VQAEVENT_PALETTE)
				&& (config->EventHandler != NULL)) {
			config->EventHandler(VQAEVENT_PALETTE, (void *)pal, (int32_t)palsize);
		}
	}
	else if (drawer->Flags & VQADRWF_SETPAL) {
		drawer->Flags &= (~VQADRWF_SETPAL);
		WaitNoVB(vqabuf->VBIBit);
		WaitVB(vqabuf->VBIBit);

		if (!(config->OptionFlags & VQAOPTF_PALOFF)) {
			SetPalette(drawer->Palette_24, drawer->CurPalSize, slowpal);
		}

		if ((config->NotifyFlags & VQAEVENT_PALETTE)
				&& (config->EventHandler != NULL)) {
			config->EventHandler(VQAEVENT_PALETTE, (void *)drawer->Palette_24,
					(int32_t)drawer->CurPalSize);
		}
	}

	/* Invoke user's callback routine */
	if (config->DrawerCallback != NULL) {
		if ((config->DrawerCallback(drawer->ImageBuf, curframe->FrameNum)) != 0) {
			return (VQAERR_EOF);
		}
	}

	/* Move to the next frame */
	drawer->CurFrame = curframe->Next;

	return (0);
}


/****************************************************************************
*
* NAME
*     UnVQ_Nop - Do nothing UnVQ.
*
* SYNOPSIS
*     UnVQ_Nop(Codebook, Pointers, Buffer, BPR, Rows, BufWidth)
*
*     void UnVQ_Nop(uint8_t *, uint8_t *, uint8_t *,
*                   uint32_t, uint32_t, uint32_t);
* FUNCTION
*
* INPUTS
*     Codebook - Not used. (Prototype placeholder)
*     Pointers - Not used. (Prototype placeholder)
*     Buffer   - Not used. (Prototype placeholder)
*     BPR      - Not used. (Prototype placeholder)
*     Rows     - Not used. (Prototype placeholder)
*     BufWidth - Not used. (Prototype placeholder)
*
* RESULT
*     NONE
*
****************************************************************************/

#ifndef PHARLAP_TNT
static void  UnVQ_Nop(uint8_t *codebook, uint8_t *pointers,
		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth)
#else
static void  UnVQ_Nop(uint8_t *, uint8_t *,
		char *, uint32_t, uint32_t,
		uint32_t)
#endif
{
}


/****************************************************************************
*
* NAME
*     PageFlip_Nop - Do nothing page flip.
*
* SYNOPSIS
*     PageFlip_Nop(VQA)
*
*     void PageFlip_Nop(VQAHandle *);
*
* FUNCTION
*
* INPUTS
*     VQA - Pointer to VQA handle.
*
* RESULT
*     NONE
*
****************************************************************************/

static int32_t PageFlip_Nop(VQAHandle *)
{
	return (0);
}


#if(0)
/****************************************************************************
*
* NAME
*     Mask_Rect - Sets non-drawable rectangle in image.
*
* SYNOPSIS
*     Mask_Rect(VQA, X1, Y1, X2, Y2)
*
*     void Mask_Rect(VQAHandle *, uint32_t, uint32_t,
*                    uint32_t, uint32_t);
*
* FUNCTION
*
* INPUTS
*     VQA - Pointer to VQA handle.
*     X1  - X coordinate of upper-left corner
*     Y1  - Y coordinate of upper-left corner
*     X2  - X coordinate of lower-right corner
*     Y2  - Y coordinate of lower-right corner
*
* RESULT
*     NONE
*
****************************************************************************/

static void Mask_Rect(VQAHandle *vqa, uint32_t x1, uint32_t y1,
		uint32_t x2, uint32_t y2)
{
	VQAData   *vqabuf;
	VQADrawer *drawer;
	VQAHeader *header;
	int32_t      blocks_per_row;

	/* Dereference data members for quicker access. */
	vqabuf = ((VQAHandleP *)vqa)->VQABuf;
	header = &((VQAHandleP *)vqa)->Header;
	drawer =  &vqabuf->Drawer;

	/* Clip coords to image size */
	if (x1 < vqabuf->Drawer.X1) {
		x1 = vqabuf->Drawer.X1;
	}

	if (y1 < vqabuf->Drawer.Y1) {
		y1 = vqabuf->Drawer.Y1;
	}

	if (x2 > vqabuf->Drawer.X2) {
		x2 = vqabuf->Drawer.X2;
	}

	if (y2 > vqabuf->Drawer.Y2) {
		y2 = vqabuf->Drawer.Y2;
	}

	/* Convert pixel coords to block coords */
	x1 /= header->block_width;
	x2 /= header->block_width;
	y1 /= header->block_height;
	y2 /= header->block_height;

	/* Compute the mask values */
	blocks_per_row = (header->image_width / header->block_width);
	vqabuf->Drawer.MaskStart = blocks_per_row * y1 + x1;

	if (x1 == x2) {
		drawer->MaskWidth = 0;
	} else {
		drawer->MaskWidth = x2 - x1 + 1;
	}

	if (y1 == y2) {
		drawer->MaskHeight = 0;
	} else {
		drawer->MaskHeight = y2 - y1 + 1;
	}
}


/****************************************************************************
*
* NAME
*     Mask_Pointers - Mask vector pointer that are in the mask rectangle.
*
* SYNOPSIS
*     Mask_Pointers(VQAData)
*
*     void Mask_Pointers(VQAData *);
*
* FUNCTION
*
* INPUTS
*     VQAData - Pointer to VQAData structure.
*
* RESULT
*     NONE
*
****************************************************************************/

static void Mask_Pointers(VQAData *vqabuf)
{
	VQADrawer     *drawer;
	VQAFrameNode  *curframe;
	uint32_t *ptr;
	uint32_t i,j;
	uint32_t start;

	/* Dereference data members for quicker access. */
	drawer = &vqabuf->Drawer;
	curframe = drawer->CurFrame;
	start = vqabuf->Drawer.MaskStart;

	for (i = 0; i < drawer->MaskHeight; i++) {
		ptr = (uint32_t *)(curframe->Pointers) + start;

		for (j = 0; j < drawer->MaskWidth; j++) {
			ptr[j] = VQA_MASK_POINTER;
		}

		start += drawer->BlocksPerRow;
	}
}
#endif
