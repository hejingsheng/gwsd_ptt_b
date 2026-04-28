#include "WinAudioDevice.h"
#include "GWLog.h"
#include "GWPttEngine.h"
#include "datalist.h"
#include "SDL.h"
#include <windows.h>

#pragma comment(lib, "SDL2.lib")

static void startRecord();
static void stopRecord();
static void startPlay();
static void stopPlay();
static void playbackPlayData(char *pcm, int len);
static void muteSpeaker(char mute);
static void muteRecorder(char mute);

static GWPttAudioModule pttAudioDevice = {
		.startPlay = startPlay,
		.stopPlay = stopPlay,
		.playData = playbackPlayData,
		.startRecord = startRecord,
		.stopRecord = stopRecord,
		.muteSpeaker = muteSpeaker,
		.muteRecorder = muteRecorder,
};

struct PcmData
{
	unsigned char pcm[640];
	int len;
};

CRITICAL_SECTION pcmDataMutex_;
static int packetSize_;
static DataList *pcmDataList_;
static char jitterFull = 0;
static int jitterCache = 15; // 15*20ms = 300ms

GWPttAudioModule* windowInitAudioDevice(int sampleRate, int bits, int channels)
{
	SDL_Init(SDL_INIT_AUDIO);
	InitializeCriticalSection(&pcmDataMutex_);
	packetSize_ = ((sampleRate / (1000 / GW_PTT_AUDIO_DURATION_MS_PER_FRAME)*bits / 8)*channels);
	pcmDataList_ = datalist_create(sizeof(struct PcmData));
	return &pttAudioDevice;
}

static SDL_AudioDeviceID recordDevice = 0;
void onReportRecordData(void *userdata, Uint8 * stream, int len)
{
	//printf("recv data %d\n", len);
	pttOnPcmData(stream, 320);
}

static void startRecord()
{
	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = GW_PTT_AUDIO_SAMPLERATE;
	want.format = AUDIO_S16SYS;
	want.channels = GW_PTT_AUDIO_CHANNELS;
	want.samples = GW_PTT_AUDIO_BYTES_PER_FRAME / 2;
	want.callback = onReportRecordData;
	want.size = GW_PTT_AUDIO_BYTES_PER_FRAME;
	want.userdata = NULL;
	recordDevice = SDL_OpenAudioDevice(NULL, 1, &want, &have, 0);
	if (recordDevice == 0)
	{
		GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "Failed to open audio record device: %s\n", SDL_GetError());
		return;
	}
	else
	{
		printf("Buffer size in samples: %u\n", have.samples);
		printf("Callback will receive approx %u bytes per call\n", have.samples * have.channels * (SDL_AUDIO_BITSIZE(have.format) / 8));
	}
	SDL_PauseAudioDevice(recordDevice, 0);
}

static void stopRecord()
{
	SDL_CloseAudioDevice(recordDevice);
}

static SDL_AudioDeviceID playDevice = 0;
void onNeedPlayData(void *userdata, Uint8 * stream, int len)
{
	struct PcmData *data = NULL;
	if (jitterFull)
	{
		EnterCriticalSection(&pcmDataMutex_);
		if (datalist_size(pcmDataList_) <= 0)
		{
			memset(stream, 0, len);
			pttUpdateLeftVoicePacket(0);
		}
		else
		{
			Node *node = datalist_front(pcmDataList_);
			if (node == NULL)
			{
				memset(stream, 0, len);
				pttUpdateLeftVoicePacket(0);
			}
			else
			{
				data = (struct PcmData*)node->rptr;
				int length = data->len;
				unsigned char *pcm = data->pcm;
				if (length < len)
				{
					memcpy(stream, pcm, length);
				}
				else
				{
					memcpy(stream, pcm, len);
				}
				datalist_pop_front(pcmDataList_);
				pttUpdateLeftVoicePacket(datalist_size(pcmDataList_));
			}
		}
		LeaveCriticalSection(&pcmDataMutex_);
	}
	else
	{
		memset(stream, 0, len);
	}
}

static void startPlay()
{
	SDL_AudioSpec want, have;
	SDL_zero(want);
	want.freq = GW_PTT_AUDIO_SAMPLERATE;
	want.format = AUDIO_S16SYS;
	want.channels = GW_PTT_AUDIO_CHANNELS;
	want.samples = GW_PTT_AUDIO_BYTES_PER_FRAME / 2;
	want.callback = onNeedPlayData;
	want.size = GW_PTT_AUDIO_BYTES_PER_FRAME;
	want.userdata = NULL;
	playDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (playDevice == 0)
	{
		GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "Failed to open audio play device: %s\n", SDL_GetError());
		return;
	}
	else
	{
		printf("Buffer size in samples: %u\n", have.samples);
		printf("Callback will receive approx %u bytes per call\n", have.samples * have.channels * (SDL_AUDIO_BITSIZE(have.format) / 8));
	}
	SDL_PauseAudioDevice(playDevice, 0);
}

static void stopPlay()
{
	// stop play non block
	SDL_CloseAudioDevice(playDevice);
}

static void playbackPlayData(char *pcm, int len)
{
	int idx = 0;

	EnterCriticalSection(&pcmDataMutex_);
	while (idx < len)
	{
		size_t length = ((len - idx) > packetSize_) ? packetSize_ : (len - idx);
		struct PcmData pcmData;
		memset(&pcmData, 0, sizeof(struct PcmData));
		memcpy(pcmData.pcm, pcm + idx, length);
		pcmData.len = length;
		datalist_push_back(pcmDataList_, &pcmData, sizeof(struct PcmData));
		idx += length;
	}
	if (datalist_size(pcmDataList_) > jitterCache && jitterFull == 0)
	{
		GWLOG_PRINT(GW_LOG_LEVEL_INFO, "pkg cache finish start play");
		jitterFull = 1;
	}
	LeaveCriticalSection(&pcmDataMutex_);
}

static void muteSpeaker(char mute)
{

}

static void muteRecorder(char mute)
{
}