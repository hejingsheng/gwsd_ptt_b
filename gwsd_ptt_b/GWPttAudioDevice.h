/*
 * GWAudioDevice.h
 *
 *  Created on: 2024年5月24日
 *      Author: hejingsheng
 */

#ifndef COMPONENTS_GWSD_SDK_GWPTTAUDIODEVICE_H_
#define COMPONENTS_GWSD_SDK_GWPTTAUDIODEVICE_H_

typedef struct
{
	void (*startPlay)();
	void (*stopPlay)();
	void (*playData)(char *playdata, int len);
	void (*startRecord)();
	void (*stopRecord)();
	void (*muteSpeaker)(char mute);
	void (*muteRecorder)(char mute);
}GWPttAudioModule;

#endif /* COMPONENTS_GWSD_SDK_GWPTTAUDIODEVICE_H_ */
