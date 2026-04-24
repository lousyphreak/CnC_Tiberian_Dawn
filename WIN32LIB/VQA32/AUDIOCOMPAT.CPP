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

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <cstdlib>

#include "VQAPLAYP.H"

#include <stdint.h>
#include <string.h>

namespace {

static VQAHandleP *VQAP = NULL;
static uint32_t AudioFlags = 0;
static int32_t TimerSysCount = 0;
static int32_t TimerIntCount = 0;
static int32_t TimerMethod = VQA_TMETHOD_DOS;
static int32_t TickOffset = 0;
static uint64_t TimerStartMs = 0;
static SDL_AudioStream *VQAAudioStream = NULL;
static uint64_t VQAAudioBytesQueued = 0;
static bool VQAAudioPaused = false;

static uint64_t Current_Time_Ms(void)
{
	return ((uint64_t)SDL_GetTicks());
}


static void Set_State(uint32_t *flags, uint32_t mask, int shift,
		uint32_t state)
{
	*flags = (*flags & ~mask) | (state << shift);
}


static SDL_AudioDeviceID Get_Stream_Device(void)
{
	if (VQAAudioStream == NULL) {
		return (0);
	}

	return (SDL_GetAudioStreamDevice(VQAAudioStream));
}


static SDL_AudioFormat Resolve_Audio_Format(uint8_t bits_per_sample)
{
	switch (bits_per_sample) {
		case 8:
			return (SDL_AUDIO_U8);

		case 16:
			return (SDL_AUDIO_S16LE);

		default:
			return (SDL_AUDIO_UNKNOWN);
	}
}


static uint32_t Effective_Audio_Rate(VQAHandleP *vqap)
{
	VQAAudio *audio = &vqap->VQABuf->Audio;
	VQAConfig *config = &vqap->Config;

	if (config->AudioRate > 0) {
		return ((uint32_t)config->AudioRate);
	}

	return ((uint32_t)audio->SampleRate);
}


static int Queued_Audio_Bytes(void)
{
	int queued;

	if (VQAAudioStream == NULL) {
		return (0);
	}

	queued = SDL_GetAudioStreamQueued(VQAAudioStream);
	return ((queued < 0) ? 0 : queued);
}


static uint32_t Played_Audio_Bytes(void)
{
	const uint64_t queued = (uint64_t)Queued_Audio_Bytes();

	if (queued >= VQAAudioBytesQueued) {
		return (0);
	}

	return ((uint32_t)(VQAAudioBytesQueued - queued));
}


static int32_t Drain_Audio_Blocks(VQAHandleP *vqap)
{
	VQAAudio *audio;
	VQAConfig *config;
	uint32_t queue_limit;

	if (vqap == NULL || VQAAudioStream == NULL || (AudioFlags & VQAAUDF_ISPLAYING) == 0) {
		return (0);
	}

	audio = &vqap->VQABuf->Audio;
	config = &vqap->Config;

	if (audio->Buffer == NULL || audio->IsLoaded == NULL || audio->NumAudBlocks == 0) {
		return (-1);
	}

	queue_limit = (config->AudioBufSize > 0) ? (uint32_t)config->AudioBufSize
			: (uint32_t)(config->HMIBufSize * 2);

	while (audio->IsLoaded[audio->NextBlock] == 1) {
		const uint32_t block = audio->NextBlock;
		const uint32_t block_offset = block * (uint32_t)config->HMIBufSize;

		if ((uint32_t)Queued_Audio_Bytes() >= queue_limit) {
			break;
		}

		if (!SDL_PutAudioStreamData(VQAAudioStream, audio->Buffer + block_offset,
				(int)config->HMIBufSize)) {
			return (-1);
		}

		audio->CurBlock = block;
		audio->PlayPosition = block_offset;
		audio->IsLoaded[block] = 0;
		audio->NextBlock = block + 1;
		if (audio->NextBlock >= audio->NumAudBlocks) {
			audio->NextBlock = 0;
		}

		VQAAudioBytesQueued += (uint64_t)(uint32_t)config->HMIBufSize;
	}

	audio->SamplesPlayed = Played_Audio_Bytes();
	return (0);
}


static uint32_t System_Ticks(void)
{
	return ((uint32_t)((Current_Time_Ms() * VQA_TIMETICKS) / 1000ULL));
}

} /* namespace */

char *HMIDevName = (char *)"<none>";

int32_t VQA_StartTimerInt(VQAHandleP *vqap, int32_t init)
{
	VQAAudio *audio;

	audio = &vqap->VQABuf->Audio;

	if (TimerSysCount == 0) {
		TimerStartMs = Current_Time_Ms();
	}

	++TimerSysCount;
	++TimerIntCount;

	Set_State(&audio->Flags, VQAAUDF_TIMERINIT, VQAAUDB_TIMERINIT,
			(init == 0) ? HMI_VQAINIT : HMI_APPINIT);
	Set_State(&audio->Flags, VQAAUDF_HMITIMER, VQAAUDB_HMITIMER, HMI_VQAINIT);
	Set_State(&AudioFlags, VQAAUDF_TIMERINIT, VQAAUDB_TIMERINIT, HMI_VQAINIT);
	Set_State(&AudioFlags, VQAAUDF_HMITIMER, VQAAUDB_HMITIMER, HMI_VQAINIT);

	return (0);
}


void VQA_StopTimerInt(VQAHandleP *vqap)
{
	VQAAudio *audio = (vqap != NULL && vqap->VQABuf != NULL) ? &vqap->VQABuf->Audio : NULL;

	if (TimerIntCount > 0) {
		--TimerIntCount;
	}

	if (TimerSysCount > 0) {
		--TimerSysCount;
	}

	if (audio != NULL) {
		Set_State(&audio->Flags, VQAAUDF_HMITIMER, VQAAUDB_HMITIMER, HMI_UNINIT);
		Set_State(&audio->Flags, VQAAUDF_TIMERINIT, VQAAUDB_TIMERINIT, HMI_UNINIT);
	}

	if (TimerIntCount == 0) {
		Set_State(&AudioFlags, VQAAUDF_HMITIMER, VQAAUDB_HMITIMER, HMI_UNINIT);
	}

	if (TimerSysCount == 0) {
		Set_State(&AudioFlags, VQAAUDF_TIMERINIT, VQAAUDB_TIMERINIT, HMI_UNINIT);
	}
}


int32_t VQA_OpenAudio(VQAHandleP *vqap)
{
	VQAData *vqabuf;
	VQAAudio *audio;
	VQAConfig *config;
	VQAHeader *header;
	SDL_AudioSpec spec;
	SDL_AudioDeviceID device;
	SDL_AudioFormat format;
	float gain;

	config = &vqap->Config;
	header = &vqap->Header;
	vqabuf = vqap->VQABuf;
	audio = &vqabuf->Audio;

	if ((config->OptionFlags & VQAOPTF_AUDIO) == 0 || audio->Buffer == NULL || config->DigiCard == 0) {
		return (-1);
	}

	if (config->AudioRate != -1) {
		audio->sSOSInitDriver.wSampleRate = (int16_t)config->AudioRate;
	} else if (config->FrameRate != header->FPS) {
		audio->sSOSInitDriver.wSampleRate = (int16_t)(((uint32_t)audio->SampleRate
				* (uint32_t)config->FrameRate) / (uint32_t)header->FPS);
		config->AudioRate = (int32_t)audio->sSOSInitDriver.wSampleRate;
	} else {
		audio->sSOSInitDriver.wSampleRate = audio->SampleRate;
		config->AudioRate = audio->SampleRate;
	}

	format = Resolve_Audio_Format(audio->BitsPerSample);
	if (format == SDL_AUDIO_UNKNOWN) {
		return (-1);
	}

	if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0) {
		if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
			return (-1);
		}
	}

	if (VQAAudioStream != NULL) {
		SDL_DestroyAudioStream(VQAAudioStream);
		VQAAudioStream = NULL;
	}

	memset(&spec, 0, sizeof(spec));
	spec.freq = (int)audio->sSOSInitDriver.wSampleRate;
	spec.format = format;
	spec.channels = audio->Channels;

	VQAAudioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (VQAAudioStream == NULL) {
		return (-1);
	}

	device = Get_Stream_Device();
	if (device == 0) {
		SDL_DestroyAudioStream(VQAAudioStream);
		VQAAudioStream = NULL;
		return (-1);
	}

	gain = (config->Volume <= 0) ? 0.0f : ((config->Volume >= 0x00FF)
			? 1.0f : ((float)config->Volume / 255.0f));
	SDL_SetAudioStreamGain(VQAAudioStream, gain);
	SDL_PauseAudioStreamDevice(VQAAudioStream);

	audio->CurBlock = 0;
	audio->NextBlock = 0;
	audio->PlayPosition = 0;
	audio->SamplesPlayed = 0;
	audio->NumSkipped = 0;
	audio->DigiHandle = 0;
	config->DigiHandle = 0;
	HMIDevName = (char *)"SDL3";

	Set_State(&audio->Flags, VQAAUDF_DIGIINIT, VQAAUDB_DIGIINIT, HMI_VQAINIT);
	Set_State(&AudioFlags, VQAAUDF_DIGIINIT, VQAAUDB_DIGIINIT, HMI_VQAINIT);

	if (getenv("RA_TRACE_STARTUP") != NULL) {
		fprintf(stderr,
				"[vqa] audio open audio=%p buffer=%p tempbuf=%p isloaded=%p blocks=%u rate=%u ch=%u bits=%u\n",
				audio,
				audio->Buffer,
				audio->TempBuf,
				audio->IsLoaded,
				(unsigned)audio->NumAudBlocks,
				(unsigned)audio->sSOSInitDriver.wSampleRate,
				(unsigned)audio->Channels,
				(unsigned)audio->BitsPerSample);
		fflush(stderr);
	}
	return (0);
}


void VQA_CloseAudio(VQAHandleP *vqap)
{
	VQAAudio *audio;

	audio = &vqap->VQABuf->Audio;

	if (VQAAudioStream != NULL) {
		SDL_ClearAudioStream(VQAAudioStream);
		SDL_DestroyAudioStream(VQAAudioStream);
		VQAAudioStream = NULL;
	}

	Set_State(&audio->Flags, VQAAUDF_DIGIINIT, VQAAUDB_DIGIINIT, HMI_UNINIT);
	AudioFlags &= ~(VQAAUDF_DIGIINIT | VQAAUDF_ISPLAYING);
	VQAAudioBytesQueued = 0;
	VQAAudioPaused = false;
	VQAP = NULL;
	HMIDevName = (char *)"<none>";
}


int32_t VQA_StartAudio(VQAHandleP *vqap)
{
	VQAAudio *audio;

	audio = &vqap->VQABuf->Audio;

	if (VQAAudioStream == NULL || (AudioFlags & VQAAUDF_ISPLAYING) != 0) {
		return (-1);
	}

	VQAP = vqap;
	VQAAudioBytesQueued = 0;
	VQAAudioPaused = false;
	audio->CurBlock = 0;
	audio->NextBlock = 0;
	audio->PlayPosition = 0;
	audio->SamplesPlayed = 0;
	audio->NumSkipped = 0;

	SDL_ClearAudioStream(VQAAudioStream);
	audio->Flags |= VQAAUDF_ISPLAYING;
	AudioFlags |= VQAAUDF_ISPLAYING;

	if (Drain_Audio_Blocks(vqap) != 0) {
		audio->Flags &= ~VQAAUDF_ISPLAYING;
		AudioFlags &= ~VQAAUDF_ISPLAYING;
		return (-1);
	}

	if (!SDL_ResumeAudioStreamDevice(VQAAudioStream)) {
		audio->Flags &= ~VQAAUDF_ISPLAYING;
		AudioFlags &= ~VQAAUDF_ISPLAYING;
		return (-1);
	}

	return (0);
}


void VQA_StopAudio(VQAHandleP *vqap)
{
	VQAAudio *audio;

	audio = &vqap->VQABuf->Audio;

	if (VQAAudioStream != NULL) {
		SDL_PauseAudioStreamDevice(VQAAudioStream);
		SDL_ClearAudioStream(VQAAudioStream);
	}

	audio->Flags &= ~VQAAUDF_ISPLAYING;
	AudioFlags &= ~VQAAUDF_ISPLAYING;
	audio->CurBlock = 0;
	audio->NextBlock = 0;
	audio->PlayPosition = 0;
	audio->SamplesPlayed = 0;
	VQAAudioBytesQueued = 0;
	VQAAudioPaused = false;
	VQAP = NULL;
}


void VQA_PauseAudio(void)
{
	if ((AudioFlags & VQAAUDF_ISPLAYING) == 0 || VQAAudioPaused) {
		return;
	}

	if (VQAAudioStream != NULL) {
		SDL_PauseAudioStreamDevice(VQAAudioStream);
		VQAAudioPaused = true;
	}
}


void VQA_ResumeAudio(void)
{
	if ((AudioFlags & VQAAUDF_ISPLAYING) == 0 || !VQAAudioPaused) {
		return;
	}

	if (VQAP != NULL) {
		Drain_Audio_Blocks(VQAP);
	}

	if (VQAAudioStream != NULL && SDL_ResumeAudioStreamDevice(VQAAudioStream)) {
		VQAAudioPaused = false;
	}
}


int32_t CopyAudio(VQAHandleP *vqap)
{
	VQAAudio  *audio;
	VQAConfig *config;
	int32_t   startblock;
	int32_t   endblock;
	int32_t   len1;
	int32_t   len2;
	int32_t   i;

	audio = &vqap->VQABuf->Audio;
	config = &vqap->Config;

	if (((config->OptionFlags & VQAOPTF_AUDIO) == 0) || (audio->Buffer == NULL)
			|| (audio->TempBufLen == 0)) {
		return (0);
	}

	startblock = (int32_t)(audio->AudBufPos / (uint32_t)config->HMIBufSize);
	endblock = (int32_t)((audio->AudBufPos + audio->TempBufLen) / (uint32_t)config->HMIBufSize);

	if (endblock >= (int32_t)audio->NumAudBlocks) {
		endblock -= (int32_t)audio->NumAudBlocks;
	}

	if (audio->IsLoaded[endblock] == 1) {
		return (VQAERR_SLEEPING);
	}

	if (startblock <= endblock) {
		memcpy(audio->Buffer + audio->AudBufPos, audio->TempBuf, audio->TempBufLen);
		audio->AudBufPos += audio->TempBufLen;
		audio->TempBufLen = 0;

		for (i = startblock; i < endblock; ++i) {
			audio->IsLoaded[i] = 1;
		}
	} else {
		len1 = (int32_t)config->AudioBufSize - (int32_t)audio->AudBufPos;
		len2 = (int32_t)audio->TempBufLen - len1;

		memcpy(audio->Buffer + audio->AudBufPos, audio->TempBuf, (size_t)len1);
		memcpy(audio->Buffer, audio->TempBuf + len1, (size_t)len2);

		audio->AudBufPos = (uint32_t)len2;
		audio->TempBufLen = 0;

		for (i = startblock; i < (int32_t)audio->NumAudBlocks; ++i) {
			audio->IsLoaded[i] = 1;
		}

		for (i = 0; i < endblock; ++i) {
			audio->IsLoaded[i] = 1;
		}
	}

	return (Drain_Audio_Blocks(vqap));
}


void VQA_SetTimer(VQAHandleP *vqap, int32_t time, int32_t method)
{
	uint32_t curtime;

	if (method == VQA_TMETHOD_DEFAULT) {
		if ((AudioFlags & VQAAUDF_ISPLAYING) != 0) {
			method = VQA_TMETHOD_AUDIO;
		} else if ((AudioFlags & VQAAUDF_HMITIMER) != 0) {
			method = VQA_TMETHOD_INT;
		} else {
			method = VQA_TMETHOD_DOS;
		}
	} else {
		if ((AudioFlags & VQAAUDF_ISPLAYING) == 0 && method == VQA_TMETHOD_AUDIO) {
			method = ((AudioFlags & VQAAUDF_HMITIMER) != 0) ? VQA_TMETHOD_INT : VQA_TMETHOD_DOS;
		}

		if ((AudioFlags & VQAAUDF_HMITIMER) == 0 && method == VQA_TMETHOD_INT) {
			method = VQA_TMETHOD_DOS;
		}
	}

	TimerMethod = method;
	TickOffset = 0;
	curtime = VQA_GetTime(vqap);
	TickOffset = (time - (int32_t)curtime);
}


uint32_t VQA_GetTime(VQAHandleP *vqap)
{
	VQAAudio *audio;
	uint32_t ticks;

	switch (TimerMethod) {
		case VQA_TMETHOD_AUDIO:
			audio = &vqap->VQABuf->Audio;
			audio->SamplesPlayed = Played_Audio_Bytes();
			if (audio->SampleRate == 0 || audio->Channels == 0 || audio->BitsPerSample == 0) {
				ticks = 0;
			} else {
				const uint32_t bytes_per_sample = (uint32_t)audio->Channels
						* (uint32_t)(audio->BitsPerSample >> 3);
				const uint32_t samples = audio->SamplesPlayed / bytes_per_sample;
				ticks = (uint32_t)((samples * VQA_TIMETICKS) / Effective_Audio_Rate(vqap));
			}
			break;

		case VQA_TMETHOD_INT:
			ticks = (uint32_t)(((Current_Time_Ms() - TimerStartMs) * VQA_TIMETICKS) / 1000ULL);
			break;

		case VQA_TMETHOD_DOS:
		default:
			ticks = System_Ticks();
			break;
	}

	return ((uint32_t)((int32_t)ticks + TickOffset));
}


int32_t VQA_TimerMethod(void)
{
	return (TimerMethod);
}
