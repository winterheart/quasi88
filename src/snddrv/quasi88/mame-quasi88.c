/*
 * MAME と QUASI88 とのインターフェイス関数
 */

#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

#include "mame-quasi88.h"

#include "2203intf.h"
#include "2608intf.h"

#include "beepintf.h"
#include "samples.h"

#ifdef USE_FMGEN
#include "2203fmgen.h"
#include "2608fmgen.h"
#endif

/*-------------------------------------------------------------------------*/

int use_sound = true;          /* 1:use sound / 0:not use */
int close_device = false;      /* 1:close audio device at menu mode / 0:not */
int fmvol = 100;               /* level of FM    (0-100)[%] */
int psgvol = 20;               /* level of PSG   (0-100)[%] */
int beepvol = 60;              /* level of BEEP  (0-100)[%] */
int rhythmvol = 100;           /* level of RHYTHM(0-100)[%] depend on fmvol */
int adpcmvol = 100;            /* level of ADPCM (0-100)[%] depend on fmvol */
int fmgenvol = 100;            /* level of fmgen (0-100)[%] */
int samplevol = 50;            /* level of SAMPLE(0-100)[%] */
int use_fmgen = false;         /* 1:use fmgen / 0:not use */
int has_samples = false;       /* 1:use samples / 0:not use */
int quasi88_is_paused = false; /* for mame_is_paused() */

typedef struct {               /* list of mame-sound-I/F functions */

  int (*sound_timer_over)(int c);

  UINT8 (*sound_in_data)(ATTR_UNUSED offs_t offset);
  UINT8 (*sound_in_status)(ATTR_UNUSED offs_t offset);
  void (*sound_out_reg)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);
  void (*sound_out_data)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);

  UINT8 (*sound2_in_data)(ATTR_UNUSED offs_t offset);
  UINT8 (*sound2_in_status)(ATTR_UNUSED offs_t offset);
  void (*sound2_out_reg)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);
  void (*sound2_out_data)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);

  void (*beep_out_data)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);
  void (*beep_out_ctrl)(ATTR_UNUSED offs_t offset, ATTR_UNUSED UINT8 data);

  void (*sample_motoron)(void);
  void (*sample_motoroff)(void);
  void (*sample_headdown)(void);
  void (*sample_headup)(void);
  void (*sample_seek)(void);

} T_XMAME_FUNC;

static T_XMAME_FUNC xmame_func_sound;
static T_XMAME_FUNC xmame_func_nosound = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static T_XMAME_FUNC *xmame_func = &xmame_func_nosound;

/****************************************************************
 * 起動時／終了時に呼ぶ
 ****************************************************************/

// was in machine.c

static struct YM2203interface ym2203_interface = {
    0, /* SSG port-A リード時に呼ばれる関数 */
    0, /* SSG port-B リード時の呼ばれる関数 */
    0, /* SSG port-A ライト時の呼ばれる関数 */
    0, /* SSG port-B ライト時の呼ばれる関数 */
    0  /* 割込フラグが立った時に呼ばれる関数 */
};

static struct YM2608interface ym2608_interface = {
    0, /* SSG port-A リード時に呼ばれる関数 */
    0, /* SSG port-B リード時の呼ばれる関数 */
    0, /* SSG port-A ライト時の呼ばれる関数 */
    0, /* SSG port-B ライト時の呼ばれる関数 */
    0, /* 割込フラグが立った時に呼ばれる関数 */
    0  /* ADPCMのメモリ番号(QUASI88では未使用) */
};

/* サンプルファイルは、モノラルであれば、bit数、周波数は不問 */
enum { SAMPLE_NUM_MOTORON, SAMPLE_NUM_MOTOROFF, SAMPLE_NUM_HEADDOWN, SAMPLE_NUM_HEADUP, SAMPLE_NUM_SEEK };
static const char *quasi88_sample_names[] = {
    "motoron.wav",  /* サンプル番号 0 */
    "motoroff.wav", /* サンプル番号 1 */
    "headdown.wav", /* サンプル番号 2 */
    "headup.wav",   /* サンプル番号 3 */
    "seek.wav",     /* サンプル番号 4 */
    0               /* end of array */
};

static struct Samplesinterface quasi88_samples_interface = {
    5,                    /* 同時に発音するチャンネル数  */
    quasi88_sample_names, /* サンプルファイル名一覧        */
    NULL                  /* 初期化成功時に呼び出す関数  */
};

static void SAMPLE_motoron(void) {
  /*  if (sample_loaded(SAMPLE_NUM_MOTORON))*/
  sample_start(0, SAMPLE_NUM_MOTORON, 0);
}
static void SAMPLE_motoroff(void) {
  /*  if (sample_loaded(SAMPLE_NUM_MOTOROFF))*/
  sample_start(1, SAMPLE_NUM_MOTOROFF, 0);
}
static void SAMPLE_headdown(void) {
  /*  if (sample_loaded(SAMPLE_NUM_HEADDOWN))*/
  sample_start(2, SAMPLE_NUM_HEADDOWN, 0);
}
static void SAMPLE_headup(void) {
  /*  if (sample_loaded(SAMPLE_NUM_HEADUP))*/
  sample_start(3, SAMPLE_NUM_HEADUP, 0);
}
static void SAMPLE_seek(void) {
  /*  if (sample_loaded(SAMPLE_NUM_SEEK))*/
  sample_start(4, SAMPLE_NUM_SEEK, 0);
}

static T_XMAME_FUNC pc88_sound_func = {
    YM2203_timer_over_0,
    YM2203_read_port_0_r,
    YM2203_status_port_0_r,
    YM2203_control_port_0_w,
    YM2203_write_port_0_w,
    NULL,
    NULL,
    NULL,
    NULL,
    BEEP88_write_port_0_w,
    BEEP88_control_port_0_w,
    SAMPLE_motoron,
    SAMPLE_motoroff,
    SAMPLE_headdown,
    SAMPLE_headup,
    SAMPLE_seek,
};

static T_XMAME_FUNC pc88_sound2_func = {
    YM2608_timer_over_0,
    YM2608_read_port_0_r,
    YM2608_status_port_0_A_r,
    YM2608_control_port_0_A_w,
    YM2608_data_port_0_A_w,
    NULL,
    YM2608_status_port_0_B_r,
    YM2608_control_port_0_B_w,
    YM2608_data_port_0_B_w,
    BEEP88_write_port_0_w,
    BEEP88_control_port_0_w,
    SAMPLE_motoron,
    SAMPLE_motoroff,
    SAMPLE_headdown,
    SAMPLE_headup,
    SAMPLE_seek,
};

#ifdef USE_FMGEN
static T_XMAME_FUNC pc88_fmgen_func = {
    NULL,
    FMGEN2203_read_port_0_r,
    FMGEN2203_status_port_0_r,
    FMGEN2203_control_port_0_w,
    FMGEN2203_write_port_0_w,
    NULL,
    NULL,
    NULL,
    NULL,
    BEEP88_write_port_0_w,
    BEEP88_control_port_0_w,
    SAMPLE_motoron,
    SAMPLE_motoroff,
    SAMPLE_headdown,
    SAMPLE_headup,
    SAMPLE_seek,
};

static T_XMAME_FUNC pc88_fmgen2_func = {
    NULL,
    FMGEN2608_read_port_0_r,
    FMGEN2608_status_port_0_A_r,
    FMGEN2608_control_port_0_A_w,
    FMGEN2608_data_port_0_A_w,
    NULL,
    FMGEN2608_status_port_0_B_r,
    FMGEN2608_control_port_0_B_w,
    FMGEN2608_data_port_0_B_w,
    BEEP88_write_port_0_w,
    BEEP88_control_port_0_w,
    SAMPLE_motoron,
    SAMPLE_motoroff,
    SAMPLE_headdown,
    SAMPLE_headup,
    SAMPLE_seek,
};
#endif

/* void construct_quasi88(machine_config *machine) */
static void construct_quasi88(machine_config *machine) {
  sound_config *sound = NULL;

  /* basic machine hardware */
  machine->frames_per_second = (float)vsync_freq_hz;

  /* sound hardware */
  driver_add_speaker(machine, "left", -0.2f, 0.0f, 1.0f);
  driver_add_speaker(machine, "right", 0.2f, 0.0f, 1.0f);

  sound = driver_add_sound(machine, NULL, SOUND_YM2203, 4000000);
  if (sound) {
    sound->config = &(ym2203_interface);

    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;
  }
  /* 0 = PSG(1ch), 1 = PSG(2ch), 2 = PSG(3ch), 3 = FM */

  sound = driver_add_sound(machine, NULL, SOUND_BEEP88, 2400);
  if (sound) {
    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;
  }
  /* 0 = BEEP */

  if (options.use_samples) {
    sound = driver_add_sound(machine, NULL, SOUND_SAMPLES, 0);
    if (sound) {
      sound->config = &(quasi88_samples_interface);

      sound->route[sound->routes].output = -1;
      sound->route[sound->routes].target = "left";
      sound->route[sound->routes].gain = 1.0f;
      sound->routes++;

      sound->route[sound->routes].output = -1;
      sound->route[sound->routes].target = "right";
      sound->route[sound->routes].gain = 1.0f;
      sound->routes++;
    }
    /* 0..4 = samples(1..5) */
  }

  xmame_func_sound = pc88_sound_func;
  if (options.use_samples) {
    has_samples = 1;
  } else {
    has_samples = 0;
    xmame_func_sound.sample_motoron = NULL;
    xmame_func_sound.sample_motoroff = NULL;
    xmame_func_sound.sample_headdown = NULL;
    xmame_func_sound.sample_headup = NULL;
    xmame_func_sound.sample_seek = NULL;
  }
}

/* void construct_quasi88sd2(machine_config *machine) */
static void construct_quasi88sd2(machine_config *machine) {
  sound_config *sound = NULL;

  /* basic machine hardware */
  machine->frames_per_second = ((float)(vsync_freq_hz));

  /* sound hardware */
  driver_add_speaker(machine, "left", -0.2f, 0.0f, 1.0f);
  driver_add_speaker(machine, "right", 0.2f, 0.0f, 1.0f);

  sound = driver_add_sound(machine, NULL, SOUND_YM2608, 8000000);
  if (sound)
    sound->config = &(ym2608_interface);
  if (sound) {
    sound->route[sound->routes].output = 0;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = 0;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = 1;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = 2;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;
  }
  /* 0 = PSG, 1 = FM(L), 2 = FM(R) */

  sound = driver_add_sound(machine, NULL, SOUND_BEEP88, 2400);
  if (sound) {
    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;
  }
  /* 0 = BEEP */

  if (options.use_samples) {
    sound = driver_add_sound(machine, NULL, SOUND_SAMPLES, 0);
    if (sound) {
      sound->config = &(quasi88_samples_interface);

      sound->route[sound->routes].output = -1;
      sound->route[sound->routes].target = "left";
      sound->route[sound->routes].gain = 1.0f;
      sound->routes++;

      sound->route[sound->routes].output = -1;
      sound->route[sound->routes].target = "right";
      sound->route[sound->routes].gain = 1.0f;
      sound->routes++;
    }
    /* 0..4 = samples(1..5) */
  }

  xmame_func_sound = pc88_sound2_func;
  if (options.use_samples) {
    has_samples = 1;
  } else {
    has_samples = 0;
    xmame_func_sound.sample_motoron = NULL;
    xmame_func_sound.sample_motoroff = NULL;
    xmame_func_sound.sample_headdown = NULL;
    xmame_func_sound.sample_headup = NULL;
    xmame_func_sound.sample_seek = NULL;
  }
}

#ifdef USE_FMGEN
/* void construct_quasi88fmgen(machine_config *machine) */
static void construct_quasi88fmgen(machine_config *machine) {
  sound_config *sound = NULL;

  /* basic machine hardware */
  machine->frames_per_second = (float)(vsync_freq_hz);

  /* sound hardware */
  driver_add_speaker(machine, "left", -0.2f, 0.0f, 1.0f);
  driver_add_speaker(machine, "right", 0.2f, 0.0f, 1.0f);

  sound = driver_add_sound(machine, NULL, SOUND_FMGEN2203, 4000000);
  if (sound) {
    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;
  }
  /* 0 = PSG/FM(L), 1 = PSG/FM(R) */

  sound = driver_add_sound(machine, NULL, SOUND_BEEP88, 2400);
  if (sound) {
    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;
  }
  /* 0 = BEEP */

  if (options.use_samples) {
    sound = driver_add_sound(machine, NULL, SOUND_SAMPLES, 0);
    if (sound) {
      sound->config = &(quasi88_samples_interface);

      sound->route[sound->routes].output = -1;
      sound->route[sound->routes].target = "left";
      sound->route[sound->routes].gain = 1.0f;
      sound->routes++;

      sound->route[sound->routes].output = -1;
      sound->route[sound->routes].target = "right";
      sound->route[sound->routes].gain = 1.0f;
      sound->routes++;
    }
    /* 0..4 = samples(1..5) */
  }

  xmame_func_sound = pc88_fmgen_func;
  if (options.use_samples) {
    has_samples = 1;
  } else {
    has_samples = 0;
    xmame_func_sound.sample_motoron = NULL;
    xmame_func_sound.sample_motoroff = NULL;
    xmame_func_sound.sample_headdown = NULL;
    xmame_func_sound.sample_headup = NULL;
    xmame_func_sound.sample_seek = NULL;
  }
}

/* void construct_quasi88fmgen2(machine_config *machine) */
static void construct_quasi88fmgen2(machine_config *machine) {
  sound_config *sound = NULL;

  /* basic machine hardware */
  machine->frames_per_second = ((float)(vsync_freq_hz));

  /* sound hardware */
  driver_add_speaker(machine, "left", -0.2f, 0.0f, 1.0f);
  driver_add_speaker(machine, "right", 0.2f, 0.0f, 1.0f);

  sound = driver_add_sound(machine, NULL, SOUND_FMGEN2608, 8000000);
  if (sound) {
    sound->route[sound->routes].output = 0;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = 1;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;
  }
  /* 0 = PSG/FM(L), 1 = PSG/FM(R) */

  sound = driver_add_sound(machine, NULL, SOUND_BEEP88, 2400);
  if (sound) {
    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "left";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;

    sound->route[sound->routes].output = -1;
    sound->route[sound->routes].target = "right";
    sound->route[sound->routes].gain = 1.0f;
    sound->routes++;
  }
  /* 0 = BEEP */

  if (options.use_samples) {
    sound = driver_add_sound(machine, NULL, SOUND_SAMPLES, 0);
    if (sound) {
      sound->config = &(quasi88_samples_interface);

      sound->route[sound->routes].output = -1;
      sound->route[sound->routes].target = "left";
      sound->route[sound->routes].gain = 1.0f;
      sound->routes++;

      sound->route[sound->routes].output = -1;
      sound->route[sound->routes].target = "right";
      sound->route[sound->routes].gain = 1.0f;
      sound->routes++;
    }
    /* 0..4 = samples(1..5) */
  }

  xmame_func_sound = pc88_fmgen2_func;
  if (options.use_samples) {
    has_samples = 1;
  } else {
    has_samples = 0;
    xmame_func_sound.sample_motoron = NULL;
    xmame_func_sound.sample_motoroff = NULL;
    xmame_func_sound.sample_headdown = NULL;
    xmame_func_sound.sample_headup = NULL;
    xmame_func_sound.sample_seek = NULL;
  }
}

#endif

/*-------------------------------------------------------------------------*/

/* the active machine */
static running_machine active_machine;
running_machine *Machine;

/* the active game driver */
static machine_config internal_drv;

/* various game options filled in by the OSD */
global_options options = {
    44100, /* サンプリングレート   8000 〜 48000 */
    0,     /* サンプル音使用可否   1:可 0:否     */
};

/*  run_game()          [src/mame.c] */
/*  create_machine()    [src/mame.c] */
/*  init_machine()      [src/mame.c] */
static void f_create_machine(void) {

  /* create_machine() [src/mame.c] -------------------------------- */

  /* first give the machine a good cleaning */
  Machine = &active_machine;
  memset(Machine, 0, sizeof(*Machine));

  /* initialize the driver-related variables in the Machine */
  Machine->drv = &internal_drv;

  /* expand_machine_driver() [src/driver.c] */
  {
    memset(&internal_drv, 0, sizeof(internal_drv));

#ifdef USE_FMGEN
    if (use_fmgen) {
      if (sound_board == SOUND_I) {
        construct_quasi88fmgen(&internal_drv);
      } else {
        construct_quasi88fmgen2(&internal_drv);
      }
    } else
#endif
    {
      if (sound_board == SOUND_I) {
        construct_quasi88(&internal_drv);
      } else {
        construct_quasi88sd2(&internal_drv);
      }
    }
  }

  /* 固定値でいいの？ 可変にする場合はどう制御する？ */
  Machine->refresh_rate = Machine->drv->frames_per_second;

  /* initialize the samplerate */
  Machine->sample_rate = options.samplerate;

  /* init_machine() [src/mame.c] -------------------------------- */

  /* initialize basic can't-fail systems here */
  sndintrf_init();

  /* init the osd layer */
  /*  if (osd_init() != 0)                                */
  /*      fprintf(stderr, "FATAL: osd_init failed\n");    */
  /*      fatalerror("osd_init failed");                  */
  /*  }                                                   */

  /* ここで、 auto_malloc 関連の初期化をする */
  init_resource_tracking();
  begin_resource_tracking();
}

/*  destroy_machine()   [src/mame.c] */
static void f_destroy_machine(void) {
  Machine = NULL;

  /* ここで、 auto_malloc されたメモリをすべて free する */
  end_resource_tracking();
  exit_resource_tracking();
}

/****************************************************************
 * サウンドの開始
 * サウンドの更新
 * サウンドの出力
 * サウンドの終了
 * サウンドの中断
 * サウンドの再開
 * サウンドのリセット
 ****************************************************************/
int xmame_sound_start(void) {
  if (verbose_proc)
    printf("Initializing Sound System ... ");

  xmame_func = &xmame_func_nosound;

  if (!use_sound) {
    if (verbose_proc)
      printf("Canceled\n");
    return 1;
  }

#ifndef USE_FMGEN
  if (use_fmgen) {
    if (verbose_proc)
      printf("\n(cisc's fmgen liblary not available)...");
    use_fmgen = 0;
  }
#endif

  if (verbose_proc)
    printf("\n");

  f_create_machine();

  /* ↓ 内部で osd_start_audio_stream() が呼び出される */
  if (sound_init() == 0) {

    xmame_func = &xmame_func_sound;

    /* 各音源の出力レベルを設定 */
#ifdef USE_FMGEN
    if (use_fmgen) {
      if (sound_board == SOUND_I) {
        FMGEN2203_set_volume_0(fmvol / 100.0f);
      } else {
        FMGEN2608_set_volume_0(fmvol / 100.0f);
      }
    } else
#endif
    {
      if (sound_board == SOUND_I) {
        YM2203_set_volume_0(fmvol / 100.0f);
        YM2203_AY8910_set_volume_0(psgvol / 100.0f);
      } else {
        YM2608_set_volume_0(fmvol / 100.0f);
        YM2608_AY8910_set_volume_0(psgvol / 100.0f);
      }
    }
    BEEP88_set_volume(beepvol / 100.0f);

    xmame_dev_beep_cmd_sing((uint8_t)use_cmdsing);

    if (has_samples) {
      sample_set_volume(0, samplevol / 100.0f);
      sample_set_volume(1, samplevol / 100.0f);
      sample_set_volume(2, samplevol / 100.0f);
      sample_set_volume(3, samplevol / 100.0f);
      sample_set_volume(4, samplevol / 100.0f);
    }

    if (verbose_proc)
      printf("Done\n");

    sound_reset();

    return 1;

  } else {

    /* ここに来るのは、 osd_start_audio_stream() で異常応答、
       malloc 失敗、machineの初期値異常、のいずれかなので、継続できない */

    if (verbose_proc)
      printf("...FAILED, abort\n");

    return 0;
  }
}

void xmame_sound_update(void) {
  if (use_sound) {
    /* ↓ 内部で osd_update_audio_stream() が呼び出される */
    sound_frame_update();
  }
}

void xmame_update_video_and_audio(void) {
  if (use_sound) {
    osd_update_video_and_audio();
  }
}

void xmame_sound_stop(void) {
  if (use_sound) {
    /* ↓ 内部で osd_stop_audio_stream() が呼び出される */
    sound_exit();

    f_destroy_machine();
  }
}

void xmame_sound_suspend(void) {
  if (use_sound) {
    if (close_device) {
      sound_pause(1);
    }

    /* サウンド停止時は、無音を出力し続ける必要がある。
       mame_is_paused() が真なら無音が生成されるので、そのように設定する。
       (sound_global_enable(0) という関数もあるが、これは特定のポートを叩
       くと無音になるようなハードのエミュレートっぽいので、無関係(?)) */

    quasi88_is_paused = true; /* mame_is_paused() が真を返すように */
  }
}
void xmame_sound_resume(void) {
  if (use_sound) {
    if (close_device) {
      sound_pause(0);
    }

    quasi88_is_paused = false; /* mame_is_paused() が偽を返すように */
  }
}
void xmame_sound_reset(void) {
  if (use_sound) {
    sound_reset();
  }
}

/****************************************************************
 * サウンドポート入出力毎に呼ぶ
 ****************************************************************/
uint8_t xmame_dev_sound_in_data(void) {
  if (xmame_func->sound_in_data)
    return (xmame_func->sound_in_data)(0);
  else
    return 0xff;
}
uint8_t xmame_dev_sound_in_status(void) {
  if (xmame_func->sound_in_status)
    return (xmame_func->sound_in_status)(0);
  else
    return 0;
}
void xmame_dev_sound_out_reg(uint8_t data) {
  if (xmame_func->sound_out_reg)
    (xmame_func->sound_out_reg)(0, data);
}
void xmame_dev_sound_out_data(uint8_t data) {
  if (xmame_func->sound_out_data)
    (xmame_func->sound_out_data)(0, data);
}

uint8_t xmame_dev_sound2_in_data(void) {
  if (use_sound) {
    if (sound_board == SOUND_I)
      return 0xff;
    else
      return 0;
  } else {
    return 0xff;
  }
}
uint8_t xmame_dev_sound2_in_status(void) {
  if (xmame_func->sound2_in_status)
    return (xmame_func->sound2_in_status)(0);
  else
    return 0xff;
}
void xmame_dev_sound2_out_reg(uint8_t data) {
  if (xmame_func->sound2_out_reg)
    (xmame_func->sound2_out_reg)(0, data);
}
void xmame_dev_sound2_out_data(uint8_t data) {
  if (xmame_func->sound2_out_data)
    (xmame_func->sound2_out_data)(0, data);
}

void xmame_dev_beep_out_data(uint8_t data) {
  if (xmame_func->beep_out_data)
    (xmame_func->beep_out_data)(0, data);
}
void xmame_dev_beep_cmd_sing(uint8_t flag) {
  if (xmame_func->beep_out_ctrl)
    (xmame_func->beep_out_ctrl)(0, flag);
}

void xmame_dev_sample_motoron(void) {
  if (xmame_func->sample_motoron)
    (xmame_func->sample_motoron)();
}
void xmame_dev_sample_motoroff(void) {
  if (xmame_func->sample_motoroff)
    (xmame_func->sample_motoroff)();
}
void xmame_dev_sample_headdown(void) {
  if (xmame_func->sample_headdown)
    (xmame_func->sample_headdown)();
}
void xmame_dev_sample_headup(void) {
  if (xmame_func->sample_headup)
    (xmame_func->sample_headup)();
}
void xmame_dev_sample_seek(void) {
  if (xmame_func->sample_seek)
    (xmame_func->sample_seek)();
}

/****************************************************************
 * サウンドのタイマーオーバーフロー時に呼ぶ
 *      timer = 0 TimerAOver / 1 TimerBOver
 ****************************************************************/
void xmame_dev_sound_timer_over(int timer) {
  if (xmame_func->sound_timer_over)
    (xmame_func->sound_timer_over)(timer);
}

/****************************************************************
 * サウンド機能有無を取得
 *      真ならサウンドあり。偽なら無し。
 ****************************************************************/
int xmame_has_sound(void) {
  if (use_sound)
    return true;
  else
    return false;
}

/****************************************************************
 * ボリューム取得
 *      現在の音量を取得する。範囲は、-32[db]〜0[db]
 ****************************************************************/
int xmame_cfg_get_mastervolume(void) {
  if (use_sound) {
    return osd_get_mastervolume();
  } else {
    return -32;
  }
}

/****************************************************************
 * ボリューム変更
 *      引数に、音量を与える。範囲は、-32[db]〜0[db]
 ****************************************************************/
void xmame_cfg_set_mastervolume(int vol) {
  if (use_sound) {
    if (vol > VOL_MAX)
      vol = VOL_MAX;
    if (vol < VOL_MIN)
      vol = VOL_MIN;
    osd_set_mastervolume(vol);
  }
}

/****************************************************************
 * チャンネル別レベル変更
 *      引数の、音源の種類とレベルを与える
 *      レベルは、    0〜100 まで
 *      音源の種類は、XMAME_MIXER_XXX で指定
 ****************************************************************/
void xmame_cfg_set_mixer_volume(int ch, int level) {
  if (use_sound) {
    switch (ch) {
    case XMAME_MIXER_PSG:
      if (level < PSGVOL_MIN)
        level = PSGVOL_MIN;
      if (level > PSGVOL_MAX)
        level = PSGVOL_MAX;
      if (!use_fmgen) {
        if (sound_board == SOUND_I) {
          YM2203_AY8910_set_volume_0(level / 100.0f);
        } else {
          YM2608_AY8910_set_volume_0(level / 100.0f);
        }
      }
      psgvol = level;
      break;

    case XMAME_MIXER_FM:
      if (level < FMVOL_MIN)
        level = FMVOL_MIN;
      if (level > FMVOL_MAX)
        level = FMVOL_MAX;
      if (!use_fmgen) {
        if (sound_board == SOUND_I) {
          YM2203_set_volume_0(level / 100.0f);
        } else {
          YM2608_set_volume_0(level / 100.0f);
        }
      }
      fmvol = level;
      break;

    case XMAME_MIXER_BEEP:
      if (level < BEEPVOL_MIN)
        level = BEEPVOL_MIN;
      if (level > BEEPVOL_MAX)
        level = BEEPVOL_MAX;
      BEEP88_set_volume(level / 100.0f);
      beepvol = level;
      break;

    case XMAME_MIXER_RHYTHM:
      if (level < RHYTHMVOL_MIN)
        level = RHYTHMVOL_MIN;
      if (level > RHYTHMVOL_MAX)
        level = RHYTHMVOL_MAX;
      rhythmvol = level;
      break;

    case XMAME_MIXER_ADPCM:
      if (level < ADPCMVOL_MIN)
        level = ADPCMVOL_MIN;
      if (level > ADPCMVOL_MAX)
        level = ADPCMVOL_MAX;
      adpcmvol = level;
      break;

    case XMAME_MIXER_FMGEN:
      if (level < FMGENVOL_MIN)
        level = FMGENVOL_MIN;
      if (level > FMGENVOL_MAX)
        level = FMGENVOL_MAX;
#ifdef USE_FMGEN
      if (use_fmgen) {
        if (sound_board == SOUND_I) {
          FMGEN2203_set_volume_0(level / 100.0f);
        } else {
          FMGEN2608_set_volume_0(level / 100.0f);
        }
      }
#endif
      fmgenvol = level;
      break;

    case XMAME_MIXER_SAMPLE:
      if (level < SAMPLEVOL_MIN)
        level = SAMPLEVOL_MIN;
      if (level > SAMPLEVOL_MAX)
        level = SAMPLEVOL_MAX;
      if (has_samples) {
        sample_set_volume(0, level / 100.0f);
        sample_set_volume(1, level / 100.0f);
        sample_set_volume(2, level / 100.0f);
        sample_set_volume(3, level / 100.0f);
        sample_set_volume(4, level / 100.0f);
      }
      samplevol = level;
      break;

    default:
      /* モニター用。各ミキサーのレベルを表示 */
      printf("...can't get mixing-level\n");
      break;
    }
  }
}

/****************************************************************
 * チャンネル別レベル取得 (レベルは、 0〜100)
 *      引数に、チャンネルを与える
 *      チャンネルは、XMAME_MIXER_XXX
 *
 *  ※ mame 内部から、レベルを取得できないので、
 *     ワークの値をそのまま返す。
 ****************************************************************/
int xmame_cfg_get_mixer_volume(int ch) {
  switch (ch) {
  case XMAME_MIXER_PSG:
    return psgvol;
  case XMAME_MIXER_FM:
    return fmvol;
  case XMAME_MIXER_BEEP:
    return beepvol;

  case XMAME_MIXER_RHYTHM:
    return rhythmvol;
  case XMAME_MIXER_ADPCM:
    return adpcmvol;

  case XMAME_MIXER_FMGEN:
    return fmgenvol;
  case XMAME_MIXER_SAMPLE:
    return samplevol;
  }

  return 0;
}

/****************************************************************
 * fmgen 使用有無
 ****************************************************************/
int xmame_cfg_get_use_fmgen(void) {
#ifdef USE_FMGEN
  return use_fmgen;
#else
  return false;
#endif
}
int xmame_cfg_set_use_fmgen(int enable) {
#ifdef USE_FMGEN
  use_fmgen = enable;
  return use_fmgen;
#else
  return false;
#endif
}

/****************************************************************
 * サンプル周波数
 ****************************************************************/
int xmame_cfg_get_sample_freq(void) {
  if (use_sound) {
    return Machine->sample_rate;
  } else {
    return options.samplerate;
  }
}
int xmame_cfg_set_sample_freq(int freq) {
  if (8000 <= freq && freq <= 48000) {
    options.samplerate = freq;
  } else {
    options.samplerate = 44100;
  }

  return options.samplerate;
}

/****************************************************************
 * サンプル音の使用有無
 ****************************************************************/
int xmame_cfg_get_use_samples(void) { return options.use_samples; }
int xmame_cfg_set_use_samples(int enable) {
  if (enable)
    options.use_samples = 1;
  else
    options.use_samples = 0;

  return options.use_samples;
}

/****************************************************************
 * WAVファイル出力
 ****************************************************************/
int xmame_wavout_open(const char *filename) {
  if (use_sound) {
    return sound_wavfile_open(filename);
  } else {
    return false;
  }
}
int xmame_wavout_opened(void) {
  if (use_sound) {
    return sound_wavfile_opened();
  } else {
    return false;
  }
}
void xmame_wavout_close(void) {
  if (use_sound) {
    sound_wavfile_close();
  }
}
int xmame_wavout_damaged(void) {
  if (use_sound) {
    return sound_wavfile_damaged();
  } else {
    return false;
  }
}

/****************************************************************
 * MAMEバージョン取得関数
 ****************************************************************/
const char *xmame_version_mame(void) { return "   Based on MAME 0.112/XMAME 0.106"; }
const char *xmame_version_fmgen(void) {
#ifdef USE_FMGEN
  return "   Based on fmgen008-current";
#else
  return "";
#endif
}

/****************************************************************
 * ファイル入出力関数
 ****************************************************************/

/*  assemble_3_strings()    [src/fileio.h] */
char *assemble_3_strings(const char *dummy1, const char *summy2, const char *s3) {
  char *tempbuf = (char *)malloc_or_die(strlen(s3) + 1);
  strcpy(tempbuf, s3);
  return tempbuf;
}

/*  mame_fopen()            [src/fileio.c] */
mame_file_error mame_fopen(const char *dummypath, char *filename, UINT32 dummyflags, mame_file **file) {
  OSD_FILE *fp = NULL;
  char buf[1024] = "";
  const char *dir = osd_dir_rom();

  *file = NULL;

  if (dir) {
    if (osd_path_join(dir, filename, buf, 1024)) {
      fp = osd_fopen(FTYPE_ROM, buf, "rb");
    }

    if (fp == NULL) { /* 開けなかったら、小文字のファイル名を試す */
      char *p = filename;
      for (; *p; p++) {
        *p = tolower(*p);
      }

      if (osd_path_join(dir, filename, buf, 1024)) {
        fp = osd_fopen(FTYPE_ROM, buf, "rb");
      }
    }

    if (fp == NULL) { /* 開けなかったら、大文字のファイル名を試す */
      char *p = filename;
      for (; *p; p++) {
        *p = toupper(*p);
      }

      if (osd_path_join(dir, filename, buf, 1024)) {
        fp = osd_fopen(FTYPE_ROM, buf, "rb");
      }
    }

    if (fp) { /* 開けられたら、fp をセットし、成功を返す */
      *file = fp;
      return FILERR_NONE;
    }
  }

  /* エラー */
  return true;
}

/****************************************************************
 * 異常分岐
 ****************************************************************/

/*  fatalerror()            [src/mame.c] */

void CLIB_DECL fatalerror(const char *text, ...) {
  va_list arg;

  va_start(arg, text);
  vfprintf(stderr, text, arg);
  va_end(arg);

  fprintf(stderr, "\n");

  /* quasi88_exit(-1) */
  exit(-1);
}
