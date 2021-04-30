/*******************************************************************************

Copyright Datapath Ltd. 2012, 2013.

File:    rgb133audio.h

Purpose: 

History:
         12 DEC 13   DMJ  Created.

*******************************************************************************/

struct rgb133_pcm_chip {
   struct snd_card* card;
   struct snd_pcm* pcm;
   struct rgb133_audio_data* linux_audio;
   struct rgb133_audio_config linux_audio_config;

   /// Assuming we only have 1 substream (as Minivosc), we can place this structure here
   /// If there is more substreams, it will be incorrect!
   struct snd_pcm_substream* substream;
   ///
   // index to audio channel
   int index;
   // handle to AudioCapture
   unsigned int hndlAudioCapture;
   int buffer_size;
   int period_size;
   int init_success;
};

typedef struct rgb133_audio_data {
   struct snd_card* card;
   struct rgb133_pcm_chip* pcm_chip;
   struct rgb133_dev* rgb133_device;
   struct timer_list rgb133_audio_timer;
   void* pAudio;
} RGB133AUDIODATA, *PRGB133AUDIODATA;

extern struct snd_device_ops rgb133_pcm_ops;

extern struct snd_pcm_ops snd_rgb133_pcm_capture_ops;

#ifdef IN_RGB133AUDIO_C
#define STATIC static
#else
#define STATIC
#endif

STATIC int rgb133_mychip_capture_open(struct snd_pcm_substream *substream);
STATIC int rgb133_mychip_capture_close(struct snd_pcm_substream *substream);
int rgb133_create_new_pcm(struct rgb133_pcm_chip * chip);
STATIC snd_pcm_uframes_t rgb133_mychip_pcm_pointer(struct snd_pcm_substream *substream);
STATIC int rgb133_mychip_pcm_trigger(struct snd_pcm_substream *substream, int cmd);
STATIC int rgb133_mychip_pcm_prepare(struct snd_pcm_substream *substream);
STATIC int rgb133_mychip_pcm_hw_free(struct snd_pcm_substream *substream);
STATIC int rgb133_mychip_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *hw_params);

STATIC int rgb133_set_dma_setup(struct rgb133_pcm_chip *chip, dma_addr_t dma_addr, snd_pcm_uframes_t buffer_size, snd_pcm_uframes_t period_size);
