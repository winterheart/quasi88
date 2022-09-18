#ifndef SUSPEND_H_INCLUDED
#define SUSPEND_H_INCLUDED

#include <stdio.h>
#include "file-op.h"

extern int resume_flag;  /* 起動時のレジューム  */
extern int resume_force; /* 強制レジューム    */
extern int resume_file;  /* ファイル名指定あり  */

/* ステートファイルのヘッダ内容 */

#define STATE_ID "QUASI88" /* 識別子 */
#define STATE_VER "0.6.0"  /* ファイルバージョン */
#define STATE_REV "1"      /* 変更バージョン */

enum suspend_type {
  TYPE_INT,
  TYPE_LONG,
  TYPE_SHORT,
  TYPE_CHAR,
  TYPE_BYTE,
  TYPE_WORD,
  TYPE_PAIR,
  TYPE_DOUBLE,
  TYPE_256,
  TYPE_STR,
  TYPE_END
};

typedef struct {
  enum suspend_type type;
  void *work;
} T_SUSPEND_W;

int stateload_emu();
int stateload_memory();
int stateload_pc88main();
int stateload_crtcdmac();
int stateload_sound();
int stateload_pio();
int stateload_screen();
int stateload_intr();
int stateload_keyboard();
int stateload_pc88sub();
int stateload_fdc();
int stateload_system();

int statesave_emu();
int statesave_memory();
int statesave_pc88main();
int statesave_crtcdmac();
int statesave_sound();
int statesave_pio();
int statesave_screen();
int statesave_intr();
int statesave_keyboard();
int statesave_pc88sub();
int statesave_fdc();
int statesave_system();

void filename_init_state(int synchronize);
const char *filename_get_state();
int filename_get_state_serial();
void filename_set_state(const char *filename);
void filename_set_state_serial(int serial);

void stateload_init();
int statesave();
int stateload();
int statesave_check_file_exist();
int stateload_check_file_exist();

int statefile_revision();

#define STATE_OK (0)        /* ロード/セーブ正常終了 */
#define STATE_ERR (-1)      /* ロード/セーブ異常終了 */
#define STATE_ERR_ID (-2)   /* ロード時 ID見つからず */
#define STATE_ERR_SIZE (-3) /* ロード時 サイズ不整合 */
int statesave_block(const char id[4], void *top, int size);
int stateload_block(const char id[4], void *top, int size);

int statesave_table(const char id[4], T_SUSPEND_W *tbl);
int stateload_table(const char id[4], T_SUSPEND_W *tbl);

#endif /* SUSPEND_H_INCLUDED */
