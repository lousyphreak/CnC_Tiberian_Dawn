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


/*=========================================================================*/
/* The following prototypes are for the file: SOUNDIO.CPP						*/
/*=========================================================================*/

int32_t Decompress_Frame(void * source, void * dest, int32_t size);
int32_t  Stream_Sample_Vol(void *buffer, int32_t size, int32_t (*callback);
int32_t  Stream_Sample(void *buffer, int32_t size, int32_t (*callback);
int32_t  File_Stream_Sample(char const *filename);
int32_t  File_Stream_Sample_Vol(char const *filename, int32_t volume);
void    Sound_Callback(void);
void     maintenance_callback(void);
void   Load_Sample(char const *filename);
int32_t  Load_Sample_Into_Buffer(char const *filename, void *buffer, int32_t size);
int32_t  Sample_Read(int32_t fh, void *buffer, int32_t size);
void  Free_Sample(void const *sample);
void  VQA_TimerCallback(void);
void  Sound_End(void);
void  Stop_Sample(int32_t handle);
int32_t  Sample_Status(int32_t handle);
int32_t  Is_Sample_Playing(void const * sample);
void  Stop_Sample_Playing(void const * sample);
int32_t  Play_Sample(void const *sample);
int32_t  Play_Sample_Vol(void const *sample, int32_t priority, int32_t volume);
int32_t  Set_Sound_Vol(int32_t volume);
int32_t  Set_Score_Vol(int32_t volume);
void  Fade_Sample(int32_t handle, int32_t ticks);
