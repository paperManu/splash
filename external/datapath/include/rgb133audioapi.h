/*
 * rgb133audioapi.h
 *
 * Copyright (c) 2009 Datapath Limited All rights reserved.
 *
 * All information contained herein is proprietary and
 * confidential to Datapath Limited and is licensed under
 * the terms of the Datapath Limited Software License.
 * Please read the LICENCE file for full license terms
 * and conditions.
 *
 * http://www.datapath.co.uk/
 * support@datapath.co.uk
 *
 */

#ifndef RGB133AUDIOAPI_H_
#define RGB133AUDIOAPI_H_

/* Elements in control */
// for example, a stereo volume control can have 2 elements
#define ALSA_CONTROL_ELEMENTS_1                  1

/* Items (== states) for enumerated controls */
#define ALSA_CONTROL_ENUM_TYPE_2_STATES          2
#define ALSA_CONTROL_ENUM_TYPE_4_STATES          4

/* Controls IDs */
#define ALSA_CONTROL_INDEX_BASE                   0x0
#define ALSA_CONTROL_INDEX_DIG_MUTE              (ALSA_CONTROL_INDEX_BASE+0)
#define ALSA_CONTROL_INDEX_ANA_MUTE              (ALSA_CONTROL_INDEX_BASE+1)
#define ALSA_CONTROL_INDEX_ANA_UNBAL_VOLUME      (ALSA_CONTROL_INDEX_BASE+2)
#define ALSA_CONTROL_INDEX_ANA_BAL_VOLUME        (ALSA_CONTROL_INDEX_BASE+3)
#define ALSA_CONTROL_INDEX_SDI_MUTE              (ALSA_CONTROL_INDEX_BASE+4)

struct rgb133_audio_config {
   /* Digital */
   unsigned int digital_mute;
   /* Analog */
   // special member which holds analog mute for both unbalanced and balanced audio 
   // (used when dealing with our enumerated type mute control)
   unsigned int analog_mute;
   long analog_min_gain_unbalanced;
   long analog_cur_gain_unbalanced;
   long analog_max_gain_unbalanced;
   long analog_min_gain_balanced;
   long analog_cur_gain_balanced;
   long analog_max_gain_balanced;
   /* SDI */
   unsigned int sdi_mute;
};

typedef enum _LINUXAUDIOCAPTURESOURCE
{
   LINUXAUDIOCAPTURESOURCE_ANALOGUE = 1,
   LINUXAUDIOCAPTURESOURCE_HDMI = 2,
   LINUXAUDIOCAPTURESOURCE_SDI = 3,
   LINUXAUDIOCAPTURESOURCE_HDMI_OR_ANALOGUE = 4,
   LINUXAUDIOCAPTURESOURCE_SDI_OR_ANALOGUE = 5,
   LINUXAUDIOCAPTURESOURCE_ANALOG_UNBALANCED = 6,
   LINUXAUDIOCAPTURESOURCE_ANALOG_BALANCED = 7,
}  LINUXAUDIOCAPTURESOURCE, *PLINUXAUDIOCAPTURESOURCE;

typedef enum _LINUXAUDIOCAPTURETYPE
{
   LINUXAUDIOCAPTURETYPE_EMBEDDED = 0,
   LINUXAUDIOCAPTURETYPE_EXTERNAL = 1,
}  LINUXAUDIOCAPTURETYPE, *PLINUXAUDIOCAPTURETYPE;

int LinuxAudioInitialisationPost(PDEAPI _pDE, int channel);

int DoLinuxAudioInitialisation(PDEAPI _pDE, PAUDIOCHANNELAPI _pAudio, int channel);
int DoLinuxAudioUninitialisation(PDEAPI _pDE, int channel);
int AudioGetMinMaxCurrentConfig(PDEAPI _pDE, PAUDIOCHANNELAPI _pAudio, int channel, struct rgb133_audio_config * pLinuxAudioConfig);

int AudioDGCAudioOpenInput(PDEAPI pDE, void* chip, int channel);
int AudioDGCAudioCloseInput(PDEAPI pDE, int channel, unsigned int handle);
int AudioDGCAudioSetCaps(PDEAPI pDE, unsigned int format, unsigned int rate, unsigned int channels, int channel);

unsigned char* AudioGetDmaBuffer (void* pAudioChannel);
unsigned int AudioGetDmaWritePointer (void* pAudioChannel);

void LinuxAudioNewDataNotify(void* chip);

int AudioGetChannelType(PAUDIOCHANNELAPI _pAudio);
int AudioGetCaptureType(PAUDIOCHANNELAPI _pAudio);

int AudioSetControlMute(PDEAPI _pDE, int channel, unsigned int value);
int AudioSetControlVolume(PDEAPI _pDE, int channel, LINUXAUDIOCAPTURESOURCE audioSource, long value);

int
AudioGetUserDefinedParms (
   PAUDIOCHANNELAPI _pAudio);

#endif /*RGB133AUDIOAPI_H_*/
