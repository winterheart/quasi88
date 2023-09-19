/************************************************************************/
/*                                  */
/*              QUASI88                 */
/*                                  */
/************************************************************************/

#include <SDL2/SDL.h>

#include "quasi88.h"

#include "device.h"
#include "event.h"
#include "menu.h"     /* menu_about_osd_msg */
#include "suspend.h"  /* stateload_system */

extern "C" {

#include "getconf.h"  /* config_init */
#ifdef WIN32
#include "keyboard.h" /* WIN32 romaji_type */
#endif
}

/***********************************************************************
 * オプション
 ************************************************************************/
static int oo_env(const char *var, const char *str) {
  // TODO: Check this out
  SDL_setenv(var, str, true);
  return 0;
}
static int o_videodrv(char *str) { return oo_env("SDL_VIDEODRIVER", str); }
static int o_audiodrv(char *str) { return oo_env("SDL_AUDIODRIVER", str); }

static int invalid_arg;
static const T_CONFIG_TABLE sdl_options[] = {
    /* 300〜349: システム依存オプション */

    /*  -- GRAPHIC -- */

    /*  -- INPUT -- */
    {311, "use_joy", X_FIX, &use_joydevice, true, 0, nullptr, nullptr},
    {311, "nouse_joy", X_FIX, &use_joydevice, false, 0, nullptr, nullptr},
    {312, "keyboard", X_INT, &keyboard_type, 0, 2, nullptr, nullptr},
    {313, "keyconf", X_STR, &file_keyboard, 0, 0, nullptr, nullptr},
    {314, "cmdkey", X_FIX, &use_cmdkey, true, 0, nullptr, nullptr},
    {314, "nocmdkey", X_FIX, &use_cmdkey, false, 0, nullptr, nullptr},

    /*  -- SYSTEM -- */
    {320, "videodrv", X_STR, nullptr, 0, 0, o_videodrv, nullptr},
    {321, "audiodrv", X_STR, nullptr, 0, 0, o_audiodrv, nullptr},
    {322, "show_fps", X_FIX, &show_fps, true, 0, nullptr, nullptr},
    {322, "hide_fps", X_FIX, &show_fps, false, 0, nullptr, nullptr},

    /*  -- 無視 -- (他システムの引数つきオプション) */
    {0, "cmap", X_INV, &invalid_arg, 0, 0, nullptr, nullptr},
    {0, "sleepparm", X_INT, &invalid_arg, 0, 0, nullptr, nullptr},

    /* 終端 */
    {0, nullptr, X_INV, nullptr, 0, 0, nullptr, nullptr},
};

static void help_msg_sdl() {
  fprintf(stdout, "  ** INPUT (SDL depend) **\n"
                  "    -use_joy/-nouse_joy     Enable/Disabel system joystick [-use_joy]\n"
                  "    -keyboard <0|1|2>       Set keyboard type (0:config/1:106key/2:101key) [1]\n"
                  "    -keyconf <filename>     Specify keyboard configuration file <filename>\n"
                  "    -cmdkey/-nocmdkey       Command key to menu (Classic Mac OS only) [-cmdkey]\n"
                  "  ** SYSTEM (SDL depend) **\n"
                  "    -videodrv <drv>         do putenv(\"SDL_VIDEODRIVER=drv\")\n"
                  "    -audiodrv <drv>         do putenv(\"SDL_AUDIODRIVER=drv\")\n"
                  "    -show_fps/-hide_fps     Show/Hide FPS (experimentral)\n");
}

/***********************************************************************
 * メイン処理
 ************************************************************************/
static void finish();

int main(int argc, char *argv[]) {
  int x = 1;

  /* エンディアンネスチェック */

#ifdef LSB_FIRST
  if (*(char *)&x != 1) {
    fprintf(stderr,
            "%s CAN'T EXECUTE !\n"
            "This machine is Big-Endian.\n"
            "Compile again comment-out 'LSB_FIRST = 1' in Makefile.\n",
            argv[0]);
    return -1;
  }
#else
  if (*(char *)&x == 1) {
    fprintf(stderr,
            "%s CAN'T EXCUTE !\n"
            "This machine is Little-Endian.\n"
            "Compile again comment-in 'LSB_FIRST = 1' in Makefile.\n",
            argv[0]);
    return -1;
  }
#endif

#ifdef WIN32
  /* 一部の初期値を改変 (いいやり方はないかな…) */
  romaji_type = 1; /* ローマ字変換の規則を MS-IME風に */
#endif

  if (config_init(argc, argv, /* 環境初期化 & 引数処理 */
                  sdl_options, help_msg_sdl)) {

    if (sdl_init()) { /* SDL関連の初期化 */

      quasi88_atexit(finish); /* quasi88() 実行中に強制終了した際の
                     コールバック関数を登録する */
      quasi88();              /* PC-8801 エミュレーション */

      sdl_exit(); /* SDL関連後始末 */
    }

    config_exit(); /* 引数処理後始末 */
  }

  return 0;
}

/*
 * 強制終了時のコールバック関数 (quasi88_exit()呼出時に、処理される)
 */
static void finish() {
  sdl_exit();    /* SDL関連後始末 */
  config_exit(); /* 引数処理後始末 */
}

/***********************************************************************
 * ステートロード／ステートセーブ
 ************************************************************************/

/*  他の情報すべてがロード or セーブされた後に呼び出される。
 *  必要に応じて、システム固有の情報を付加してもいいかと。
 */

int stateload_system(void) { return true; }
int statesave_system(void) { return true; }

/***********************************************************************
 * メニュー画面に表示する、システム固有メッセージ
 ************************************************************************/

int menu_about_osd_msg(int req_japanese, int *result_code, const char *message[]) { return false; }
