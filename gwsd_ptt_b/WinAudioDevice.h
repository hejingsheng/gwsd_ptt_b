#ifndef _WIN_AUDIODEVICE_H_
#define _WIN_AUDIODEVICE_H_

#include "GWPttAudioDevice.h"

GWPttAudioModule* windowInitAudioDevice(int sampleRate, int bits, int channels);

#endif