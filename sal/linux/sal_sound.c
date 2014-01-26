#include <stdlib.h>
#include <SDL.h>

#include <sal.h>

#define BUFFER_FRAMES 2
// 48000 Hz maximum; 1/50 of a second; 3 frames to hold (2 plus a bit extra)
#define BUFFER_SAMPLES (48000 / 50 * (BUFFER_FRAMES + 1))

static SDL_AudioSpec audiospec;

volatile static unsigned int ReadPos, WritePos;

// 2 channels per sample (stereo); 2 bytes per sample-channel (16-bit)
static uint8_t Buffer[BUFFER_SAMPLES * 2 * 2];
static u32 SamplesPerFrame, BytesPerSample;

static void sdl_audio_callback (void *userdata, Uint8 *stream, int len)
{
	u32 SamplesRequested = len / BytesPerSample, SamplesBuffered, LocalWritePos = WritePos /* isolate a bit against races */;
	if (ReadPos <= LocalWritePos)
		SamplesBuffered = LocalWritePos - ReadPos;
	else
		SamplesBuffered = BUFFER_SAMPLES - (ReadPos - LocalWritePos);

	if (SamplesRequested > SamplesBuffered)
		return;

	if (ReadPos + SamplesRequested > BUFFER_SAMPLES)
	{
		memcpy(stream, &Buffer[ReadPos * BytesPerSample], (BUFFER_SAMPLES - ReadPos) * BytesPerSample);
		memcpy(&stream[(BUFFER_SAMPLES - ReadPos) * BytesPerSample], &Buffer[0], (SamplesRequested - (BUFFER_SAMPLES - ReadPos)) * BytesPerSample);
	}
	else
	{
		memcpy(stream, &Buffer[ReadPos * BytesPerSample], len);
	}
	ReadPos = (ReadPos + SamplesRequested) % BUFFER_SAMPLES;
}

s32 sal_AudioInit(s32 rate, s32 bits, s32 stereo, s32 Hz)
{
	audiospec.freq = rate;
	audiospec.channels = (stereo + 1);
	audiospec.format = AUDIO_S16;

	audiospec.samples = (rate / Hz);
	if (!stereo && audiospec.samples & 1)
		audiospec.samples--;

	 
	SamplesPerFrame = audiospec.samples;
	BytesPerSample = audiospec.channels * (bits >> 3);


	audiospec.callback = sdl_audio_callback;

	if (SDL_OpenAudio(&audiospec, NULL) < 0) {
		fprintf(stderr, "Unable to initialize audio.\n");
		return SAL_ERROR;
	}

	WritePos = ReadPos = 0;

	return SAL_OK;
}

void sal_AudioPause(void)
{
	SDL_PauseAudio(1);
}

void sal_AudioResume(void)
{
	SDL_PauseAudio(0);
}

void sal_AudioClose(void)
{
	SDL_CloseAudio();
}

u32 sal_AudioGenerate(u32 samples)
{
	u32 SamplesAvailable, LocalReadPos = ReadPos /* isolate a bit against races */;
	if (LocalReadPos <= WritePos)
		SamplesAvailable = BUFFER_SAMPLES - (WritePos - LocalReadPos);
	else
		SamplesAvailable = LocalReadPos - WritePos;
	if (samples >= SamplesAvailable)
	{
		samples = SamplesAvailable - 1;
	}
	if (samples > BUFFER_SAMPLES - WritePos)
	{
		sal_AudioGenerate(BUFFER_SAMPLES - WritePos);
		sal_AudioGenerate(samples - (BUFFER_SAMPLES - WritePos));
	}
	else
	{
		S9xMixSamples(&Buffer[WritePos * BytesPerSample], samples * audiospec.channels);
		WritePos = (WritePos + samples) % BUFFER_SAMPLES;
	}
}

u32 sal_AudioGetFramesBuffered()
{
	u32 SamplesBuffered, LocalReadPos = ReadPos /* isolate a bit against races */;
	if (LocalReadPos <= WritePos)
		SamplesBuffered = WritePos - LocalReadPos;
	else
		SamplesBuffered = BUFFER_SAMPLES - (LocalReadPos - WritePos);
	return SamplesBuffered / SamplesPerFrame;
}

u32 sal_AudioGetMaxFrames()
{
	return BUFFER_FRAMES;
}

u32 sal_AudioGetSamplesPerFrame()
{
	return SamplesPerFrame;
}

u32 sal_AudioGetBytesPerSample()
{
	return BytesPerSample;
}

void sal_AudioSetVolume(s32 l, s32 r)
{
}
