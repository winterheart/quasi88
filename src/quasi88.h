#ifndef QUASI88_H_INCLUDED
#define QUASI88_H_INCLUDED


/*----------------------------------------------------------------------*/
/* システム・環境依存の定義                     */
/*----------------------------------------------------------------------*/
#include "config.h"

/*----------------------------------------------------------------------*/
/* ファイルシステム依存の定義                      */
/*----------------------------------------------------------------------*/
#include "filename.h"

/*----------------------------------------------------------------------*/
/* バージョン情報                            */
/*----------------------------------------------------------------------*/
#include "version.h"

/*----------------------------------------------------------------------*/
/* 共通定義                             */
/*----------------------------------------------------------------------*/

typedef unsigned char Uchar;
typedef unsigned short Ushort;
typedef unsigned int Uint;
typedef unsigned long Ulong;

typedef unsigned char byte;
typedef unsigned short word;
typedef signed char offset;

typedef unsigned char bit8;
typedef unsigned short bit16;
typedef unsigned int bit32;

#define COUNTOF(arr) (int)(sizeof(arr) / sizeof((arr)[0]))
#define OFFSETOF(s, m) ((size_t)(&((s *)0)->m))
#undef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#undef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#undef ABS
#define ABS(x) (((x) >= 0) ? (x) : -(x))
#define SGN(x) (((x) > 0) ? 1 : (((x) < 0) ? -1 : 0))
#define BETWEEN(l, x, h) ((l) <= (x) && (x) <= (h))

#ifdef LSB_FIRST /* リトルエンディアン */

typedef union {
  struct {
    uint8_t l, h;
  } B;
  uint16_t W;
} pair;

#else /* ビッグエンデイアン */

typedef union {
  struct {
    uint8_t h, l;
  } B;
  uint16_t W;
} pair;

#endif

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#ifndef INLINE
#define INLINE static
#endif

/*----------------------------------------------------------------------*/
/* リセット依存の定義                      */
/*----------------------------------------------------------------------*/
#include "event.h"

/*----------------------------------------------------------------------*/
/* 変数 (verbose_*)、関数                      */
/*----------------------------------------------------------------------*/
extern int verbose_level;   /* 冗長レベル          */
extern int verbose_proc;    /* 処理の進行状況の表示       */
extern int verbose_z80;     /* Z80処理エラーを表示      */
extern int verbose_io;      /* 未実装 I/Oアクセスを報告   */
extern int verbose_pio;     /* PIO の不正使用を表示     */
extern int verbose_fdc;     /* FDイメージ異常を報告        */
extern int verbose_wait;    /* ウエイト待ち時の異常を報告  */
extern int verbose_suspend; /* サスペンド時の異常を報告 */
extern int verbose_snd;     /* サウンドのメッセージ       */

enum {
  EVENT_NONE = 0x0000,
  EVENT_FRAME_UPDATE = 0x0001,
  EVENT_AUDIO_UPDATE = 0x0002,
  EVENT_MODE_CHANGED = 0x0004,
  EVENT_DEBUG = 0x0008,
  EVENT_QUIT = 0x0010
};
extern int quasi88_event_flags;
extern int quasi88_debug_pause; /* 1ならpause, 0ならmonitor */

enum EmuMode {
  EXEC,

  GO,
  TRACE,
  STEP,
  TRACE_CHANGE,

  MONITOR,
  MENU,
  PAUSE,

  QUIT
};

#define INIT_POWERON (0)
#define INIT_RESET (1)
#define INIT_STATELOAD (2)

void quasi88();

void quasi88_start();
void quasi88_main();
void quasi88_stop(int normal_exit);
enum { QUASI88_LOOP_EXIT, QUASI88_LOOP_ONE, QUASI88_LOOP_BUSY };
int quasi88_loop();

void quasi88_atexit(void (*function)());
void quasi88_exit(int status);

void quasi88_exec();
void quasi88_exec_step();
void quasi88_exec_trace();
void quasi88_exec_trace_change();
void quasi88_menu();
void quasi88_pause();
void quasi88_monitor();
void quasi88_debug();
void quasi88_quit();
int quasi88_is_exec();
int quasi88_is_menu();
int quasi88_is_pause();
int quasi88_is_monitor();
void quasi88_get_reset_cfg(T_RESET_CFG *cfg);
void quasi88_reset(const T_RESET_CFG *cfg);

/*----------------------------------------------------------------------*/
/* その他    (実体は、 quasi88.c  にて定義してある)          */
/*----------------------------------------------------------------------*/
void wait_vsync_switch();

#endif /* QUASI88_H_INCLUDED */
