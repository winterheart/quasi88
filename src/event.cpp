#include <cstring>

extern "C" {
#include "quasi88.h"

#include "drive.h"
#include "event.h"
#include "fname.h"
#include "intr.h"
#include "menu.h"
#include "pc88main.h"
#include "pc88sub.h"
#include "snapshot.h"
#include "soundbd.h"
#include "status.h"
#include "suspend.h"
#include "wait.h"

#if USE_RETROACHIEVEMENTS
#include "retroachievements.h"
#endif
}

/***********************************************************************
 * QUASI88 起動中のステートロード処理関数
 *  TODO 引数で、ファイル名指定？
 ************************************************************************/
int quasi88_stateload(int serial) {
  if (serial >= 0) {                   /* 連番指定あり (>=0) なら */
    filename_set_state_serial(serial); /* 連番を設定する */
  }

  if (verbose_proc)
    printf("Stateload...start (%s)\n", filename_get_state());

  if (stateload_check_file_exist() == FALSE) { /* ファイルなし */
    if (quasi88_is_exec()) {
      status_message(1, STATUS_INFO_TIME, "State-Load file not found !");
    } /* メニューではダイアログ表示するので、ステータス表示は無しにする */

    if (verbose_proc)
      printf("State-file not found\n");
    return FALSE;
  }

#if USE_RETROACHIEVEMENTS
  if (!RA_WarnDisableHardcore("load a state")) {
    if (verbose_proc)
      printf("State-Load cancelled (RA)\n");
    return FALSE;
  }
#endif

  pc88main_term(); /* 念のため、ワークを終了状態に */
  pc88sub_term();
  imagefile_all_close(); /* イメージファイルを全て閉じる */

  /*xmame_sound_reset();*/ /* 念のため、サウンドリセット */
  /*quasi88_reset();*/     /* 念のため、全ワークリセット */

  int now_board = sound_board;

  int success = stateload(); /* ステートロード実行 */

  if (now_board != sound_board) { /* サウンドボードが変わったら */
    menu_sound_restart(FALSE);    /* サウンドドライバの再初期化 */
  }

  if (verbose_proc) {
    if (success) {
      printf("Stateload...done\n");
    } else
      printf("Stateload...Failed, Reset start\n");
  }

  if (success) { /* ステートロード成功したら・・・ */

    imagefile_all_open(TRUE); /* イメージファイルを全て開く*/

    pc88main_init(INIT_STATELOAD);
    pc88sub_init(INIT_STATELOAD);

#if USE_RETROACHIEVEMENTS
    RA_OnLoadState(filename_get_state());
#endif

  } else { /* ステートロード失敗したら・・・ */

    quasi88_reset(nullptr); /* とりあえずリセット */
  }

  if (quasi88_is_exec()) {
    if (success) {
      status_message(1, STATUS_INFO_TIME, "State-Load Successful");
    } else {
      status_message(1, STATUS_INFO_TIME, "State-Load Failed !  Reset done ...");
    }

    /* quasi88_loop の内部状態を INIT にするため、モード変更扱いとする */
    quasi88_event_flags |= EVENT_MODE_CHANGED;
  }
  /* メニューではダイアログ表示するので、ステータス表示は無しにする */

  return success;
}

/***********************************************************************
 * QUASI88 起動中のステートセーブ処理関数
 *  TODO 引数で、ファイル名指定？
 ************************************************************************/
int quasi88_statesave(int serial) {
  if (serial >= 0) {                   /* 連番指定あり (>=0) なら */
    filename_set_state_serial(serial); /* 連番を設定する */
  }

  if (verbose_proc)
    printf("Statesave...start (%s)\n", filename_get_state());

  int success = statesave(); /* ステートセーブ実行 */

  if (verbose_proc) {
    if (success) {
      printf("Statesave...done\n");
    } else
      printf("Statesave...Failed, Reset done\n");
  }

  if (quasi88_is_exec()) {
    if (success) {
#if USE_RETROACHIEVEMENTS
      RA_OnSaveState(filename_get_state());
#endif
      status_message(1, STATUS_INFO_TIME, "State-Save Successful");
    } else {
      status_message(1, STATUS_INFO_TIME, "State-Save Failed !");
    }
  } /* メニューではダイアログ表示するので、ステータス表示は無しにする */

  return success;
}

/***********************************************************************
 * 画面スナップショット保存
 *  TODO 引数で、ファイル名指定？
 ************************************************************************/
int quasi88_screen_snapshot(void) {
  int success = screen_snapshot_save();

  if (success) {
    status_message(1, STATUS_INFO_TIME, "Screen Capture Saved");
  } else {
    status_message(1, STATUS_INFO_TIME, "Screen Capture Failed !");
  }

  return success;
}

/***********************************************************************
 * サウンドデータのファイル出力
 *  TODO 引数で、ファイル名指定？
 ************************************************************************/
int quasi88_waveout(int start) {
  int success;

  if (start) {
    success = waveout_save_start();

    if (success) {
      status_message(1, STATUS_INFO_TIME, "Sound Record Start ...");
    } else {
      status_message(1, STATUS_INFO_TIME, "Sound Record Failed !");
    }

  } else {

    success = TRUE;

    waveout_save_stop();
    status_message(1, STATUS_INFO_TIME, "Sound Record Stopped");
  }

  return success;
}

/***********************************************************************
 * ドラッグアンドドロップ
 ************************************************************************/
int quasi88_drag_and_drop(const char *filename) {
  if (!quasi88_is_exec() && !quasi88_is_pause()) {
    return FALSE;
  }

  int success;

#if USE_RETROACHIEVEMENTS
  RA_ToggleLoad(FALSE);
#endif

  if (quasi88_disk_insert_all(filename, FALSE)) {
    status_message(1, STATUS_INFO_TIME, "Disk Image Set and Reset");
    success = TRUE;
  } else if (quasi88_load_tape_insert(filename)) {
    status_message(1, STATUS_INFO_TIME, "Tape Image Set and Reset");
    success = TRUE;
  } else {
    status_message(1, STATUS_WARN_TIME, "D&D Failed !  Disk Unloaded ...");
    success = FALSE;
  }

#if USE_RETROACHIEVEMENTS
  RA_ToggleLoad(TRUE);
#endif

  if (success) {
    quasi88_reset(nullptr);

    if (quasi88_is_pause()) {
      quasi88_exec();
    }

#if USE_RETROACHIEVEMENTS
    RA_UpdateAppTitle(loaded_title->name);
    RA_ActivateGame(loaded_title->title_id);
#endif
  }
#if USE_RETROACHIEVEMENTS
  else
    RA_AbortLoadNewRom();
#endif

  return success;
}

/***********************************************************************
 * ウェイトの比率設定
 * ウェイトの有無設定
 ************************************************************************/
int quasi88_cfg_now_wait_rate(void) { return wait_rate; }
void quasi88_cfg_set_wait_rate(int rate) {
  int time = STATUS_INFO_TIME;
  char str[32];
  long dt;

#if USE_RETROACHIEVEMENTS
  if (rate < 100) {
    if (!RA_WarnDisableHardcore("set the speed below 100%"))
      return;
  }
#endif
  if (rate < 5)
    rate = 5;
  if (rate > 5000)
    rate = 5000;

  if (wait_rate != rate) {
    wait_rate = rate;

    if (quasi88_is_exec()) {

      sprintf(str, "WAIT  %4d[%%]", wait_rate);

      status_message(1, time, str);
      /* ↑ ウェイト変更したので、表示時間はウェイト倍になる */

      dt = (long)((1000000.0 / (CONST_VSYNC_FREQ * wait_rate / 100)));
      wait_vsync_setup(dt, wait_by_sleep);
    }
  }
}
int quasi88_cfg_now_no_wait(void) { return no_wait; }
void quasi88_cfg_set_no_wait(int enable) {
  int time = STATUS_INFO_TIME;
  char str[32];
  long dt;

  if (no_wait != enable) {
    no_wait = enable;

    if (quasi88_is_exec()) {

      if (no_wait) {
        sprintf(str, "WAIT  OFF");
        time *= 10;
      } else
        sprintf(str, "WAIT  ON");

      status_message(1, time, str);
      /* ↑ ウェイトなしなので、表示時間は実際のところ不定 */

      dt = (long)((1000000.0 / (CONST_VSYNC_FREQ * wait_rate / 100)));
      wait_vsync_setup(dt, wait_by_sleep);
    }
  }
}

/***********************************************************************
 * ディスクイメージファイル設定
 *  ・両ドライブに挿入
 *  ・指定ドライブに挿入
 *  ・反対ドライブのイメージファイルを、挿入
 *  ・両ドライブ取り出し
 *  ・指定ドライブ取り出し
 ************************************************************************/
int quasi88_disk_insert_all(const char *filename, int ro) {
  if (!quasi88_disk_eject_all())
    return FALSE;

  int success = quasi88_disk_insert(DRIVE_1, filename, 0, ro);

  if (success) {
    if (disk_image_num(DRIVE_1) > 1) {
      quasi88_disk_insert_A_to_B(DRIVE_1, DRIVE_2, 1);
    }
  }

  if (quasi88_is_exec()) {
    status_message_default(1, nullptr);
  }
  return success;
}
int quasi88_disk_insert(int drv, const char *filename, int image, int ro) {
  int success = FALSE;

#if USE_RETROACHIEVEMENTS
  if (drv == DRIVE_1) {
    if (!RA_PrepareLoadNewRom(filename, FTYPE_DISK))
      return FALSE;
  }
#endif

  if (!quasi88_disk_eject(drv))
    return FALSE;

  if (strlen(filename) < QUASI88_MAX_FILENAME) {

    if (disk_insert(drv, filename, image, ro) == 0)
      success = TRUE;
    else
      success = FALSE;

    if (success) {

      if (drv == DRIVE_1)
        boot_from_rom = FALSE;

      strcpy(file_disk[drv], filename);
      readonly_disk[drv] = ro;

      if (filename_synchronize) {
        filename_init_state(TRUE);
        filename_init_snap(TRUE);
        filename_init_wav(TRUE);
      }

#if USE_RETROACHIEVEMENTS
      if (drv == DRIVE_1)
        RA_CommitLoadNewRom();
#endif
    }
  }

  if (quasi88_is_exec()) {
    status_message_default(1, nullptr);
  }
  return success;
}
int quasi88_disk_insert_A_to_B(int src, int dst, int img) {
  int success;

  if (!quasi88_disk_eject(dst))
    return FALSE;

  if (disk_insert_A_to_B(src, dst, img) == 0)
    success = TRUE;
  else
    success = FALSE;

  if (success) {
    strcpy(file_disk[dst], file_disk[src]);
    readonly_disk[dst] = readonly_disk[src];

    if (filename_synchronize) {
      filename_init_state(TRUE);
      filename_init_snap(TRUE);
      filename_init_wav(TRUE);
    }
  }

  if (quasi88_is_exec()) {
    status_message_default(1, NULL);
  }
  return success;
}
int quasi88_disk_eject_all(void) {
  int drv;

  for (drv = 0; drv < 2; drv++) {
    if (!quasi88_disk_eject(drv))
      return FALSE;
  }

  boot_from_rom = TRUE;

  if (quasi88_is_exec()) {
    status_message_default(1, nullptr);
  }

  return TRUE;
}
int quasi88_disk_eject(int drv) {
  if (disk_image_exist(drv)) {
#if USE_RETROACHIEVEMENTS
    if (drv == DRIVE_1 && loaded_title != NULL && loaded_title->file_type == FTYPE_DISK) {
      if (!RA_ConfirmLoadNewRom(false))
        return FALSE;
    }
#endif
    disk_eject(drv);
    memset(file_disk[drv], 0, QUASI88_MAX_FILENAME);

    if (filename_synchronize) {
      filename_init_state(TRUE);
      filename_init_snap(TRUE);
      filename_init_wav(TRUE);
    }

#if USE_RETROACHIEVEMENTS
    if (drv == DRIVE_1) {
#if !RA_RELOAD_MULTI_DISK
      if (loaded_title != NULL && (loaded_title->title_id == 0 || loaded_title->title_id != loading_file.title_id))
#endif
        RA_ClearTitle();
    }
#endif
  }

  if (quasi88_is_exec()) {
    status_message_default(1, nullptr);
  }

  return TRUE;
}

/***********************************************************************
 * ディスクイメージファイル設定
 *  ・ドライブを一時的に空の状態にする
 *  ・ドライブのイメージを変更する
 *  ・ドライブのイメージを前のイメージに変更する
 *  ・ドライブのイメージを次のイメージに変更する
 ************************************************************************/
enum { TYPE_SELECT, TYPE_EMPTY, TYPE_NEXT, TYPE_PREV };

static void disk_image_sub(int drv, int type, int img) {
  int d;
  char str[48];

  if (disk_image_exist(drv)) {
    switch (type) {

    case TYPE_EMPTY:
      drive_set_empty(drv);
      sprintf(str, "DRIVE %d:  <<<< Eject >>>>         ", drv + 1);
      break;

    case TYPE_NEXT:
    case TYPE_PREV:
      if (type == TYPE_NEXT)
        d = +1;
      else
        d = -1;

      img = disk_image_selected(drv) + d;
      /* FALLTHROUGH */

    default:
      if (img < 0)
        img = disk_image_num(drv) - 1;
      if (img >= disk_image_num(drv))
        img = 0;

      drive_unset_empty(drv);
      disk_change_image(drv, img);

      sprintf(str, "DRIVE %d:  %-16s   %s  ", drv + 1, drive[drv].image[disk_image_selected(drv)].name,
              (drive[drv].image[disk_image_selected(drv)].protect) ? "(p)" : "   ");
      break;
    }
  } else {
    sprintf(str, "DRIVE %d:   --  No Disk  --        ", drv + 1);
  }

  if (quasi88_is_exec()) {
    status_message_default(1, nullptr);
  }
  status_message(1, STATUS_INFO_TIME, str);
}
void quasi88_disk_image_select(int drv, int img) { disk_image_sub(drv, TYPE_SELECT, img); }
void quasi88_disk_image_empty(int drv) { disk_image_sub(drv, TYPE_EMPTY, 0); }
void quasi88_disk_image_next(int drv) { disk_image_sub(drv, TYPE_NEXT, 0); }
void quasi88_disk_image_prev(int drv) { disk_image_sub(drv, TYPE_PREV, 0); }

/*======================================================================
 * テープイメージファイル設定
 *      ・ロード用テープイメージファイルセット
 *      ・ロード用テープイメージファイル巻き戻し
 *      ・ロード用テープイメージファイル取り外し
 *      ・セーブ用テープイメージファイルセット
 *      ・セーブ用テープイメージファイル取り外し
 *======================================================================*/
int quasi88_load_tape_insert(const char *filename) {
#if USE_RETROACHIEVEMENTS
  if (!RA_PrepareLoadNewRom(filename, FTYPE_TAPE_LOAD))
    return FALSE;
#endif

  if (!quasi88_load_tape_eject())
    return FALSE;

  if (strlen(filename) < QUASI88_MAX_FILENAME && sio_open_tapeload(filename)) {

    strcpy(file_tape[CLOAD], filename);

#if USE_RETROACHIEVEMENTS
    RA_CommitLoadNewRom();
#endif

    return TRUE;
  }
  return FALSE;
}
int quasi88_load_tape_rewind(void) {
  if (sio_tape_rewind()) {

    return TRUE;
  }
  quasi88_load_tape_eject();
  return FALSE;
}
int quasi88_load_tape_eject(void) {
#if USE_RETROACHIEVEMENTS
  if (loaded_title != NULL && loaded_title->file_type == FTYPE_TAPE_LOAD && loaded_title->data_len > 0) {
    if (!RA_ConfirmLoadNewRom(false))
      return FALSE;
  }
#endif

  sio_close_tapeload();
  memset(file_tape[CLOAD], 0, QUASI88_MAX_FILENAME);

#if USE_RETROACHIEVEMENTS
  if (loaded_tape.data_len > 0) {
#if !RA_RELOAD_MULTI_DISK
    if (loaded_title != NULL && (loaded_title->title_id == 0 || loaded_title->title_id != loading_file.title_id))
#endif
      RA_ClearTitle();
  }
#endif

  return TRUE;
}

int quasi88_save_tape_insert(const char *filename) {
  quasi88_save_tape_eject();

  if (strlen(filename) < QUASI88_MAX_FILENAME && sio_open_tapesave(filename)) {

    strcpy(file_tape[CSAVE], filename);
    return TRUE;
  }
  return FALSE;
}
int quasi88_save_tape_eject(void) {
  sio_close_tapesave();
  memset(file_tape[CSAVE], 0, QUASI88_MAX_FILENAME);

  return TRUE;
}

/*======================================================================
 * シリアル・パラレルイメージファイル設定
 *      ・シリアル入力用ファイルセット
 *      ・シリアル入力用ファイル取り外し
 *      ・シリアル出力用ファイルセット
 *      ・シリアル出力用ファイル取り外し
 *      ・プリンタ出力用ファイルセット
 *      ・プリンタ入力用ファイルセット
 *======================================================================*/
int quasi88_serial_in_connect(const char *filename) {
  quasi88_serial_in_remove();

  if (strlen(filename) < QUASI88_MAX_FILENAME && sio_open_serialin(filename)) {

    strcpy(file_sin, filename);
    return TRUE;
  }
  return FALSE;
}
void quasi88_serial_in_remove(void) {
  sio_close_serialin();
  memset(file_sin, 0, QUASI88_MAX_FILENAME);
}
int quasi88_serial_out_connect(const char *filename) {
  quasi88_serial_out_remove();

  if (strlen(filename) < QUASI88_MAX_FILENAME && sio_open_serialout(filename)) {

    strcpy(file_sout, filename);
    return TRUE;
  }
  return FALSE;
}
void quasi88_serial_out_remove(void) {
  sio_close_serialout();
  memset(file_sout, 0, QUASI88_MAX_FILENAME);
}
int quasi88_printer_connect(const char *filename) {
  quasi88_printer_remove();

  if (strlen(filename) < QUASI88_MAX_FILENAME && printer_open(filename)) {

    strcpy(file_prn, filename);
    return TRUE;
  }
  return FALSE;
}
void quasi88_printer_remove(void) {
  printer_close();
  memset(file_prn, 0, QUASI88_MAX_FILENAME);
}
