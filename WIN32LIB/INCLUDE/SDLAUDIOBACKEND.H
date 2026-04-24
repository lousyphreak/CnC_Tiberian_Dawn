#ifndef SDLAUDIOBACKEND_H
#define SDLAUDIOBACKEND_H

#include "win32_compat.h"

#include <cstdint>
#include <mutex>
#include <vector>

#define RAAUDIO_OK 0
#define RAAUDIOERR_BUFFERLOST ((HRESULT)-3000)
#define RAAUDIOERR_GENERIC ((HRESULT)-3001)

#define RAAUDIOPLAY_LOOPING 0x00000001U
#define RAAUDIOSTATUS_PLAYING 0x00000001U
#define RAAUDIOSTATUS_LOOPING 0x00000002U

#define RA_AUDIO_FORMAT_PCM 1

typedef struct AudioBufferFormat {
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;
	uint16_t cbSize;
} AudioBufferFormat;

class AudioBackendBuffer {
public:
	AudioBackendBuffer(class AudioBackendDevice *owner, uint32_t buffer_bytes, const AudioBufferFormat *format, bool primary);
	~AudioBackendBuffer();
	HRESULT GetStatus(uint32_t *status);
	HRESULT Stop(void);
	HRESULT Play(int32_t looping);
	HRESULT Lock(uint32_t write_cursor, uint32_t write_bytes, void * *ptr1, uint32_t *bytes1, void * *ptr2, uint32_t *bytes2);
	HRESULT Unlock(void * ptr1, uint32_t bytes1, void * ptr2, uint32_t bytes2);
	HRESULT SetCurrentPosition(uint32_t position);
	HRESULT GetCurrentPosition(uint32_t *play_cursor, uint32_t *write_cursor);
	HRESULT SetVolume(int32_t volume);
	HRESULT Restore(void);
	HRESULT SetFormat(const AudioBufferFormat *format);
	HRESULT Release(void);

private:
	void MixInto(float *mix_buffer, int32_t output_frames, int32_t output_channels, int32_t output_rate);

	friend class AudioBackendDevice;

	AudioBackendDevice *Owner;
	bool Primary;
	int32_t RefCount;
	uint32_t Status;
	uint32_t Position;
	bool Locked;
	double PlaybackCursorFrames;
	int32_t Volume;
	AudioBufferFormat FormatData;
	std::mutex Mutex;
	std::vector<uint8_t> Data;
};

class AudioBackendDevice {
public:
	AudioBackendDevice(void);
	~AudioBackendDevice();
	HRESULT SetWindow(RAWindow* window);
	HRESULT CreateBuffer(uint32_t buffer_bytes, const AudioBufferFormat *format, int32_t primary, AudioBackendBuffer **buffer);
	HRESULT Release(void);

private:
	friend class AudioBackendBuffer;

	static void SDLCALL AudioCallback(void *userdata, struct SDL_AudioStream *stream, int32_t additional_amount, int32_t total_amount);

	void FeedAudio(struct SDL_AudioStream *stream, int32_t additional_amount);
	void RemoveBuffer(AudioBackendBuffer *buffer);
	bool EnsureAudioStreamLocked(void);
	struct SDL_AudioStream *LockStreamForCallbackSync(void);
	void UpdatePrimaryPlayback(bool playing);
	void UpdatePrimaryPlaybackLocked(bool playing);
	void RefreshPrimaryFormat(void);
	void RefreshPrimaryFormatLocked(struct SDL_AudioStream **stream_to_destroy);

	int32_t RefCount;
	RAWindow* Window;
	std::mutex Mutex;
	struct SDL_AudioStream *Stream;
	struct SDL_AudioSpec MixSpec;
	SDL_AudioFormat MixFormat;
	int32_t MixChannels;
	int32_t MixRate;
	bool PrimaryPlaying;
	AudioBackendBuffer *PrimaryBuffer;
	std::vector<AudioBackendBuffer *> SecondaryBuffers;
};

HRESULT CreateAudioBackendDevice(AudioBackendDevice **device);

#endif
