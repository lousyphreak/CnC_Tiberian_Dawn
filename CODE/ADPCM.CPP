/*
**	Command & Conquer(tm)
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

#include "SOSCOMP.H"

#include <stdint.h>

namespace {

static const int16_t CodecIndexTable[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

static const int16_t CodecStepTable[89] = {
	7, 8, 9, 10, 11, 12, 13, 14,
	16, 17, 19, 21, 23, 25, 28, 31,
	34, 37, 41, 45, 50, 55, 60, 66,
	73, 80, 88, 97, 107, 118, 130, 143,
	157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411,
	1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
	3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767
};

static inline int32_t Clamp_Signed_16(int32_t value)
{
	if (value > 32767) {
		return 32767;
	}
	if (value < -32768) {
		return -32768;
	}
	return value;
}

static inline uint8_t To_Unsigned_8(int32_t value)
{
	return (uint8_t)((((uint32_t)value) >> 8) ^ 0x80U);
}

static inline int32_t Decode_ADPCM_Nibble(uint8_t code, int16_t& index, int16_t& step, int32_t& predicted, int32_t& difference)
{
	difference = (int32_t)(step >> 3);
	if (code & 0x04U) {
		difference += step;
	}
	if (code & 0x02U) {
		difference += (step >> 1);
	}
	if (code & 0x01U) {
		difference += (step >> 2);
	}
	if (code & 0x08U) {
		difference = -difference;
	}

	predicted = Clamp_Signed_16(predicted + difference);

	int32_t next_index = (int32_t)index + CodecIndexTable[code & 0x0FU];
	if (next_index < 0) {
		next_index = 0;
	} else if (next_index > 88) {
		next_index = 88;
	}

	index = (int16_t)next_index;
	step = CodecStepTable[next_index];
	return predicted;
}

}

void sosCODECInitStream(_SOS_COMPRESS_INFO* info)
{
	if (info == 0) {
		return;
	}

	info->dwSampleIndex = 0;
	info->dwPredicted = 0;
	info->dwDifference = 0;
	info->wCodeBuf = 0;
	info->wCode = 0;
	info->wStep = 7;
	info->wIndex = 0;
	info->dwSampleIndex2 = 0;
	info->dwPredicted2 = 0;
	info->dwDifference2 = 0;
	info->wCodeBuf2 = 0;
	info->wCode2 = 0;
	info->wStep2 = 7;
	info->wIndex2 = 0;
}

uint32_t sosCODECDecompressData(_SOS_COMPRESS_INFO* info, uint32_t numbytes)
{
	return General_sosCODECDecompressData(info, numbytes);
}

uint32_t General_sosCODECDecompressData(_SOS_COMPRESS_INFO* info, uint32_t numbytes)
{
	if (info == 0 || info->lpSource == 0 || info->lpDest == 0) {
		return 0;
	}

	const bool is_16_bit = (info->wBitSize == 16);
	const bool is_stereo = (info->wChannels == 2);
	const uint32_t sample_size = is_16_bit ? 2U : 1U;
	const uint32_t frame_size = sample_size * (is_stereo ? 2U : 1U);
	if (frame_size == 0) {
		return 0;
	}

	uint8_t* left_source = (uint8_t*)info->lpSource;
	uint8_t* right_source = is_stereo ? left_source + 1 : left_source;
	uint8_t left_codebuf = 0;
	uint8_t right_codebuf = 0;
	uint8_t left_code = 0;
	uint8_t right_code = 0;
	uint32_t left_sample_index = 0;
	uint32_t right_sample_index = 0;
	int32_t left_predicted = (int32_t)info->dwPredicted;
	int32_t left_difference = (int32_t)info->dwDifference;
	int16_t left_step = (info->wStep != 0) ? info->wStep : 7;
	int16_t left_index = info->wIndex;
	int32_t right_predicted = (int32_t)info->dwPredicted2;
	int32_t right_difference = (int32_t)info->dwDifference2;
	int16_t right_step = (info->wStep2 != 0) ? info->wStep2 : 7;
	int16_t right_index = info->wIndex2;
	uint8_t* dest8 = (uint8_t*)info->lpDest;
	int16_t* dest16 = (int16_t*)info->lpDest;
	const uint32_t frame_count = numbytes / frame_size;

	info->dwSampleIndex = 0;
	info->dwSampleIndex2 = 0;

	for (uint32_t frame = 0; frame < frame_count; ++frame) {
		if ((left_sample_index & 1U) == 0U) {
			left_codebuf = *left_source;
			left_source += is_stereo ? 2 : 1;
			left_code = (uint8_t)(left_codebuf & 0x0FU);
		} else {
			left_code = (uint8_t)((left_codebuf >> 4) & 0x0FU);
		}
		const int32_t left_sample = Decode_ADPCM_Nibble(left_code, left_index, left_step, left_predicted, left_difference);
		++left_sample_index;

		if (is_stereo) {
			if ((right_sample_index & 1U) == 0U) {
				right_codebuf = *right_source;
				right_source += 2;
				right_code = (uint8_t)(right_codebuf & 0x0FU);
			} else {
				right_code = (uint8_t)((right_codebuf >> 4) & 0x0FU);
			}
			const int32_t right_sample = Decode_ADPCM_Nibble(right_code, right_index, right_step, right_predicted, right_difference);
			++right_sample_index;

			if (is_16_bit) {
				dest16[frame * 2] = (int16_t)left_sample;
				dest16[(frame * 2) + 1] = (int16_t)right_sample;
			} else {
				dest8[frame * 2] = To_Unsigned_8(left_sample);
				dest8[(frame * 2) + 1] = To_Unsigned_8(right_sample);
			}
		} else {
			if (is_16_bit) {
				dest16[frame] = (int16_t)left_sample;
			} else {
				dest8[frame] = To_Unsigned_8(left_sample);
			}
		}
	}

	info->dwSampleIndex = left_sample_index;
	info->dwPredicted = left_predicted;
	info->dwDifference = left_difference;
	info->wCodeBuf = left_codebuf;
	info->wCode = left_code;
	info->wStep = left_step;
	info->wIndex = left_index;
	info->dwSampleIndex2 = right_sample_index;
	info->dwPredicted2 = right_predicted;
	info->dwDifference2 = right_difference;
	info->wCodeBuf2 = right_codebuf;
	info->wCode2 = right_code;
	info->wStep2 = right_step;
	info->wIndex2 = right_index;

	return numbytes;
}
