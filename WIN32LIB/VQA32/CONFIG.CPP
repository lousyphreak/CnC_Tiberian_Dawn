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
*     VQA player library. (32-Bit protected mode)
*
* FILE
*     config.c
*
* DESCRIPTION
*     Player configuration routines.
*
* PROGRAMMER
*     Bill Randolph
*     Denzil E. Long, Jr.
*
* DATE
*     April 10, 1995
*
*----------------------------------------------------------------------------
*
* PUBLIC
*     VQA_INIConfig     - Initialize VQAConfig structure with INI settings.
*     VQA_DefaultConfig - Initialize VQAConfig structure with defaults.
*
****************************************************************************/

#include <stdint.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <VQM32/PROFILE.H>
#include "VQAPLAYP.H"

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/

/* Default configuration structure. */
static VQAConfig _defaultconfig = {

	/* DrawerCallback: This is a function that is called for every frame
	 * in the movie.
	 */
	NULL,

	/* EventHandler: This is a function that is called for every event that
	 * the client requested to be notified about.
	 */
	NULL,

	/* NotifyFlags: Flags representing the events the client wishes to be
	 * notified about during playback.
	 */
	0,

	/* Vmode: Legacy direct-video mode slot; ignored by the SDL buffer path. */
	0,

	/* VBIBit: Vertical blank bit polarity. */
	-1,

	/* ImageBuf: Pointer to image buffer to draw into. */
	NULL,

	/* ImageWidth, ImageHeight: Width and height dimensions of image buffer.
	 * A width and height value of -1 tells the player to consider the image
	 * buffer as having the same dimensions as the frames in the movie.
	 */
	320, 200, /* Image width and height */

	/* X1, Y1: These are the coordinates to put the movies frame in the image
	 * buffer. Values of -1 tell the drawer to center the frames in the buffer.
	 */
	-1, -1,

	/* FrameRate: The rate to load the frames at. A value of -1 tells the
	 * player to use the framerate of the movie.
	 */
	-1,

	/* DrawRate: The rate to draw the frames at. A value of -1 tells the
	 * player to use the framerate of the movie. A value of 0 tells the player
	 * to use a fixed rate based on the frame size.
	 */
	-1,

	/* TimerMethod: Timer method to use for playback. */
	-1,

	/* DrawFlags: Various drawing related flags. */
	0,

	/* OptionFlags: Various player options. */
	VQAOPTF_AUDIO,

	/* NumFrameBufs: The number of frame buffers to allocate/use. */
	6,

	/* NumCBBufs: The number of codebook buffers to allocate/use. */
	3,

	/* AudioObject, PrimaryBuffer: caller-owned SDL audio backend handles. */
	NULL,
	NULL,

	/* VocFile: Filename of audio track override. A value of 0 tells the
	 * player not to override the movies audio track.
	 */
	NULL,

	/* AudioBuf: Audio buffer to use. A value of 0 tells the player that
	 * it has to allocate a buffer itself.
	 */
	NULL,

	/* AudioBufSize: Size of audio buffer to use/allocate. A value of -1
	 * tells the player to compute the buffer size from the audio
	 * information in the movie.
	 */
	-1,

	/* AudioRate: Audio playback rate in samples per second. A value of -1
	 * tells the player to use the audio rate of the movie.
	 */
	-1,

	/* Volume: Volume level to playback audio track. */
	0x00FF,

	/* HMIBufSize: Size of HMIs internal buffer. */
	2048L,

	/* DigiHandle: Handle to an initialized HMI sound driver. A value of -1
	 * tells the player it must initialize the HMI sound driver itself.
	 */
	-1,

	/* DigiCard: HMI ID of audio card to use. A value of 0 tells the player
	 * not to use any card. A value of -1 tells the player to autodetect the
	 * card in the system.
	 */
	-1,

	/* DigiPort: Port address of the sound card. A value of -1 tells the player
	 * to autodetect this address.
	 */
	-1,

	/* DigiIRQ: Interrupt number of sound card. A value of -1 tells the player
	 * to autodetect the  used by the card.
	 */
	-1,

	/* DigiDMA: DMA channel of the sound card. A value of -1 tells the player
	 * to autodetect the channel used by the card.
 	 */
	-1,

	/* Language: Prefered language. */
	0,

	/* CaptionFont: Caption text font. */
	NULL,

	/* EVAFont: EVA text font. */
	NULL,
};



/****************************************************************************
*
* NAME
*     VQA_INIConfig - Initialize VQAConfig structure with INI settings.
*
* SYNOPSIS
*     VQA_INIConfig(Config)
*
*     void VQA_INIConfig(VQAConfig *);
*
* FUNCTION
*     Initializes the configuration structure from the player INI file.
*
* INPUTS
*     Config - Pointer to VQAConfig structure.
*
* RESULT
*     NONE
*
****************************************************************************/

void VQA_INIConfig(VQAConfig *config)
{
	char const * ininame;
	char buf[80];

	/* Set all Config entries to 0. */
	memset(config, 0, sizeof(VQAConfig));

	/* Retrieve player INI filename from an enviroment variable if
	 * it is provided.
	 */
	if ((ininame = getenv("VQACFG")) == NULL) {
		ininame = "PLAYER.INI";
	}


	/* Get framerate and drawrate. */
	GetINIString("Player", "FrameRate", "-1", buf, 80, ininame);
	config->FrameRate = atoi(buf);

	GetINIString("Player", "DrawRate", "Variable", buf, 80, ininame);

	if (!SDL_strcasecmp(buf, "Variable")) {
		config->DrawRate = -1;
	} else {
		config->DrawRate = 0;
	}

	/*-------------------------------------------------------------------------
	 * AUDIO SETTINGS	 
	 *-----------------------------------------------------------------------*/
	GetINIString("Player", "AudioRate", "-1", buf, 80, ininame);
	config->AudioRate = atoi(buf);

	/* OptionFlags */
	GetINIString("Player", "SoundEnabled", "True", buf, 80, ininame);

	if (!SDL_strcasecmp(buf, "True") || !SDL_strcasecmp(buf, "1")) {
		config->OptionFlags |= VQAOPTF_AUDIO;
	} else {
		config->OptionFlags &= (~VQAOPTF_AUDIO);
	}

	/* Default audio settings. */
	config->AudioBufSize = 32768U;
	config->HMIBufSize = 2048;
	config->DigiHandle = -1;
	config->Volume = 0x00FF;
	config->DigiCard = 0xFFFF;
	config->DigiPort = -1;
	config->DigiIRQ = -1;
	config->DigiDMA = -1;

	/* Configure sound hardware */
	GetINIString("Player", "Port", "-1", buf, 80, ininame);

	if (!SDL_strcasecmp(buf, "-1")) {
		config->DigiPort = -1;
	} else {
		sscanf(buf, "%x", &config->DigiPort);
	}

	GetINIString("Player", "IRQ", "-1", buf, 80, ininame);
	config->DigiIRQ = atoi(buf);
	GetINIString("Player", "DMA", "-1", buf, 80, ininame);
	config->DigiDMA = atoi(buf);

	/*-------------------------------------------------------------------------
	 * GENERAL OPTIONS
	 *-----------------------------------------------------------------------*/

	/* Enable/Disable single stepping */ 
	GetINIString("Player", "SingleStep", "False", buf, 80, ininame);

	if (!SDL_strcasecmp(buf, "True") || !SDL_strcasecmp(buf, "1")) {
		config->OptionFlags |= VQAOPTF_STEP;
		config->DrawFlags |= VQACFGF_NOSKIP;
	} else {
		config->OptionFlags &= (~VQAOPTF_STEP);
	}

	/* Enable/Disable Slowpalette */
	GetINIString("Player", "SlowPalette", "False", buf, 80, ininame);

	if (!SDL_strcasecmp(buf, "True") || !SDL_strcasecmp(buf, "1")) {
		config->OptionFlags |= VQAOPTF_SLOWPAL;
	} else {
		config->OptionFlags &= (~VQAOPTF_SLOWPAL);
	}

	/* Enable/Disable monochrome display */
	GetINIString("Player", "MonoOutput", "False", buf, 80, ininame);

	if (!SDL_strcasecmp(buf, "True") || !SDL_strcasecmp(buf, "1")) {
		config->OptionFlags |= VQAOPTF_MONO;
	} else {
		config->OptionFlags &= (~VQAOPTF_MONO);
	}

	/* Frame and codebook buffers */
	config->NumFrameBufs = 6;
	config->NumCBBufs = 3;
}


/****************************************************************************
*
* NAME
*     VQA_DefaultConfig - Initialize VQAConfig structure with defaults.
*
* SYNOPSIS
*     VQA_DefaultConfig(Config);
*
*     void VQA_DefaultConfig(VQAConfig *);
*
* FUNCTION
*     Initialize configuration with default settings.
*
* INPUTS
*     Config - Pointer to VQA configuration structure.
*
* RESULT
*     NONE
*
****************************************************************************/

void VQA_DefaultConfig(VQAConfig *config)
{
	memcpy(config, &_defaultconfig, sizeof(VQAConfig));
}
