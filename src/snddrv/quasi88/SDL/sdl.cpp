#ifndef SYSDEP_DSP_SDL /* QUASI88 */
#define SYSDEP_DSP_SDL
#endif                 /* QUASI88 */

#ifdef SYSDEP_DSP_SDL

/* Sysdep SDL sound dsp driver

   Copyright 2001 Jack Burton aka Stefano Ceccherini
   <burton666@freemail.it>

   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/* Thank goes to Caz Jones <turok2@currantbun.com>, who fixed this plugin
   and made it finally sound good.
*/
/* Changelog
Version 0.1, January 2002
-initial release, based on various xmame dsp plugins and Yoshi's code


*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if 0  /* QUASI88 */
#include <strings.h>
#endif /* QUASI88 */
#include <SDL2/SDL.h>
#if 0  /* QUASI88 */
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"
#endif /* QUASI88 */

#if 1  /* QUASI88 */
#include "audio.h"
#include "quasi88.h"

int sdl_buffersize = 2048;

#define fprintf                                                                                                        \
  if (verbose_proc)                                                                                                    \
  fprintf
#endif /* QUASI88 */

/* private variables */
static struct {
  uint8_t *data;
  int dataSize;
  int amountRemain;
  int amountWrite;
  int amountRead;
  int tmp;
  uint32_t soundlen;
  int sound_n_pos;
  int sound_w_pos;
  int sound_r_pos;
} sample;

static int sdl_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

/* callback function prototype */
static void sdl_fill_sound(void *unused, uint8_t *stream, int len);

/* public methods prototypes */
#if 0  /* QUASI88 */
static void *sdl_dsp_create(const void *flags);
#endif /* QUASI88 */
static void sdl_dsp_destroy(dsp_struct *dsp);
static int sdl_dsp_write(dsp_struct *dsp, unsigned char *data, int count);

#if 0  /* QUASI88 */
/* public variables */
const struct plugin_struct sysdep_dsp_sdl = {
   "sdl",
   "sysdep_dsp",
   "Simple Direct Library DSP plugin",
   NULL, /* no options */
   NULL, /* no init */
   NULL, /* no exit */
   sdl_dsp_create,
   3     /* high priority */
};
#endif /* QUASI88 */

/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */
#if 0  /* QUASI88 */
static void *sdl_dsp_create(const void *flags)
#else  /* QUASI88 */
extern void *sdl_dsp_create(const dsp_create_params *flags)
#endif /* QUASI88 */
{
  struct sdl_dsp_priv_data *priv = nullptr;
  dsp_struct *dsp = nullptr;
  const dsp_create_params *params = flags;
  const char *device = params->device;
  SDL_AudioSpec *audiospec;

  /* allocate the dsp struct */
  if (!(dsp = (dsp_struct *)calloc(1, sizeof(dsp_struct)))) {
    fprintf(stderr, "error malloc failed for struct sysdep_dsp_struct\n");
    return nullptr;
  }

  if (!(audiospec = (SDL_AudioSpec *)calloc(1, sizeof(SDL_AudioSpec)))) {
    fprintf(stderr, "error malloc failed for SDL_AudioSpec\n");
    sdl_dsp_destroy(dsp);
    return nullptr;
  }

  /* fill in the functions and some data */
  dsp->_priv = priv;
  dsp->write = sdl_dsp_write;
  dsp->destroy = sdl_dsp_destroy;
  dsp->hw_info.type = params->type;
  dsp->hw_info.samplerate = params->samplerate;

  /* set the number of bits */
  audiospec->format = (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? AUDIO_S16SYS : AUDIO_S8;

  /* set the number of channels */
  audiospec->channels = (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? 2 : 1;

  /* set the samplerate */
  audiospec->freq = dsp->hw_info.samplerate;

  /* set samples size */
#if 0  /* QUASI88 */
   audiospec->samples = 2048;
#else  /* QUASI88 */
  audiospec->samples = sdl_buffersize;
#endif /* QUASI88 */

  /* set callback funcion */
  audiospec->callback = sdl_fill_sound;

  audiospec->userdata = nullptr;

#if 0 /* QUASI88 */
   /* Open audio device */
   if(SDL_WasInit(SDL_INIT_VIDEO)!=0)   /* If sdl video system is already */
      SDL_InitSubSystem(SDL_INIT_AUDIO);/* initialized, we just initialize */
   else                                 /* the audio subsystem */
      SDL_Init(SDL_INIT_AUDIO);         /* else we MUST use "SDL_Init" */
                                        /* (untested) */
#else /* QUASI88 */
  if (!SDL_WasInit(SDL_INIT_AUDIO))
    SDL_Init(SDL_INIT_AUDIO);
#endif /* QUASI88 */

  if (SDL_OpenAudio(audiospec, nullptr) != 0) {
    fprintf(stderr, "failed opening audio device\n");
    return nullptr;
  }

  sample.dataSize = audiospec->size * 4;
  free(audiospec);
  if (!(sample.data = (uint8_t *)calloc(sample.dataSize, sizeof(uint8_t)))) {
    fprintf(stderr, "error malloc failed for data\n");
    sdl_dsp_destroy(dsp);
    return nullptr;
  }

  SDL_PauseAudio(0);

  fprintf(stderr, "info: audiodevice %s set to %dbit linear %s %dHz\n", device,
          (dsp->hw_info.type & SYSDEP_DSP_16BIT) ? 16 : 8, (dsp->hw_info.type & SYSDEP_DSP_STEREO) ? "stereo" : "mono",
          dsp->hw_info.samplerate);

#if 1             /* QUASI88 */
  SDL_Delay(500); /* Really Need? */
#endif            /* QUASI88 */
  return dsp;
}

static void sdl_dsp_destroy(dsp_struct *dsp) {
  SDL_CloseAudio();

  free(dsp);

#if 1 /* QUASI88 */
  if (sample.data) {
    free(sample.data);
  }
  memset(&sample, 0, sizeof(sample));
  sample.data = nullptr;
#endif /* QUASI88 */
}

static int sdl_dsp_write(dsp_struct *dsp, unsigned char *data, int count) {
  /* sound_n_pos = normal position
     sound_r_pos = read position
     and so on.                   */
  uint8_t *src;
  int bytes_written = 0;
  SDL_LockAudio();

  sample.amountRemain = sample.dataSize - sample.sound_n_pos;
  sample.amountWrite = count * sdl_dsp_bytes_per_sample[dsp->hw_info.type];

  if (sample.amountRemain <= 0) {
    SDL_UnlockAudio();
    return 0;
  }

  if (sample.amountRemain < sample.amountWrite)
    sample.amountWrite = sample.amountRemain;
  sample.sound_n_pos += sample.amountWrite;

  src = (uint8_t *)data;
  sample.tmp = sample.dataSize - sample.sound_w_pos;

  if (sample.tmp < sample.amountWrite) {
    memcpy(sample.data + sample.sound_w_pos, src, sample.tmp);
    bytes_written += sample.tmp;
    sample.amountWrite -= sample.tmp;
    src += sample.tmp;
    memcpy(sample.data, src, sample.amountWrite);
    bytes_written += sample.amountWrite;
    sample.sound_w_pos = sample.amountWrite;
  } else {
    memcpy(sample.data + sample.sound_w_pos, src, sample.amountWrite);
    bytes_written += sample.amountWrite;
    sample.sound_w_pos += sample.amountWrite;
  }
  SDL_UnlockAudio();

  return bytes_written / sdl_dsp_bytes_per_sample[dsp->hw_info.type];
}

/* Private method */
static void sdl_fill_sound(void *unused, uint8_t *stream, int len) {
  int result;
  uint8_t *dst;
  // Initialize stream before audio playback to prevent noise
  memset(stream, 0, len);
  sample.amountRead = len;
  if (sample.sound_n_pos <= 0)
    return;

  if (sample.sound_n_pos < sample.amountRead)
    sample.amountRead = sample.sound_n_pos;
  result = (int)sample.amountRead;
  sample.sound_n_pos -= sample.amountRead;

  dst = (uint8_t *)stream;

  sample.tmp = sample.dataSize - sample.sound_r_pos;
  if (sample.tmp < sample.amountRead) {
    memcpy(dst, sample.data + sample.sound_r_pos, sample.tmp);
    sample.amountRead -= sample.tmp;
    dst += sample.tmp;
    memcpy(dst, sample.data, sample.amountRead);
    sample.sound_r_pos = sample.amountRead;
  } else {
    memcpy(dst, sample.data + sample.sound_r_pos, sample.amountRead);
    sample.sound_r_pos += sample.amountRead;
  }
}

#endif /* ifdef SYSDEP_DSP_SDL */
