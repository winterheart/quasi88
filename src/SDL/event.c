/***********************************************************************
 * イベント処理 (システム依存)
 *
 *  詳細は、 event.h 参照
 ************************************************************************/

#include <SDL2/SDL.h>

#include <string.h>

#include "quasi88.h"
#include "graph.h"
#include "keyboard.h"

#include "drive.h"

#include "device.h"
#include "screen.h"
#include "event.h"

#include "intr.h" /* test */
#include "keymap.h"

int show_fps;                      /* test */
static int display_fps_init(void); /* test */
static void display_fps(void);     /* test */

int use_cmdkey = 1; /* Commandキーでメニューへ遷移     */

int keyboard_type = 1;      /* キーボードの種類                */
char *file_keyboard = NULL; /* キー設定ファイル名         */

int use_joydevice = TRUE; /* ジョイスティックデバイスを開く? */

#define JOY_MAX KEY88_PAD_MAX /* ジョイスティック上限(2個) */

#define BUTTON_MAX KEY88_PAD_BUTTON_MAX /* ボタン上限(8個)         */

#define AXIS_U 0x01
#define AXIS_D 0x02
#define AXIS_L 0x04
#define AXIS_R 0x08

typedef struct {

  SDL_Joystick *dev; /* オープンしたジョイスティックの構造体 */
  int num;           /* QUASI88 でのジョイスティック番号 0〜 */
  int axis;          /* 方向ボタン押下状態          */
  int nr_button;     /* 有効なボタンの数         */

} T_JOY_INFO;

static T_JOY_INFO joy_info[JOY_MAX];

static int joystick_num; /* オープンしたジョイスティックの数 */

static const char *debug_sdlkeysym(int code); /* デバッグ用 */
/*==========================================================================
 * キー配列について
 *
 *  一般キー(文字キー) は、
 *  106 キーボードの場合、PC-8801 と同じなので問題なし。
 *  101 キーボードの場合、一部配置が異なるし、キーが足りない。
 *  とりあえず、以下のように配置してみる。
 *      ` → \、 = → ^、 [ ] はそのまま、 \ → @、' → :、右CTRL → _
 *
 *  特殊キー(機能キー) は、
 *  ホスト側のキー刻印と似た雰囲気の機能を、PC-8801のキーに割り当てよう。
 *  Pause は STOP、 PrintScreen は COPY など。個人的な主観で決める。
 *
 *  テンキーは、
 *  PC-8801と106キーでキー刻印が若干異なるが、そのままのキー刻印を使う。
 *  となると、 = と , が無い。 mac ならあるが。
 *
 *  最下段のキーの配置は、適当に割り振る。 (カッコのキーには割り振らない)
 *
 *  PC-8801        かな GRPH 決定  スペース 変換  PC    全角
 *  101キー   Ctrl      Alt        スペース             Alt          Ctrl
 *  104キー   Ctrl Win  Alt        スペース             Alt  Win App Ctrl
 *  109キー   Ctrl Win  Alt 無変換 スペース 変換 (ひら) Alt  Win App Ctrl
 *  mac ?     Ctrl      Opt (Cmd)  スペース      (cmd) (Opt)        (Ctrl)
 *
 * SDLのキー入力についての推測 (Windows & 106キーの場合)
 *  ○環境変数 SDL_VIDEODRIVER が windib と directx とで挙動が異なる。
 *  ○「SHIFT」を押しながら 「1」 を押しても、keysym は 「1」 のまま。
 *    つまり、 「!」 は入力されないみたい。
 *    大文字 「A」 も入力されない。 keysym は 「a」 となる。
 *  ○キー配列は 101 がベースになっている。
 *    「^」 を押しても keysym は 「=」 になる。
 *  ○いくつかのキーで、 keycode が重複している
 *    windib  だと、カーソルキーとテンキーなど、たくさん。
 *    directx なら、重複は無い ?
 *  ○いくつかのキーで、 keysym が重複している
 *    windib  だと、￥ と ]  (ともに ￥ になる)
 *    directx なら、重複は無い ?
 *  ○いくつかのキーで、キーシンボルが未定義
 *    無変換、変換、カタカナひらがな が未定義
 *    windib  だと、＼ が未定義
 *    directx だと、＾￥＠：、半角/全角 が未定義
 *  ○いくつかのキーで、キーを離した時の検知が不安定(?)
 *    windib  だと 半角/全角、カタカナひらがな、PrintScreen
 *    directx だと ALT
 *  ○キーロックに難あり(?)
 *    NumLockはロックできる。
 *    windib  だと SHIFT + CapsLock がロック可。
 *    directx だと CapsLock、カタカナひらがな、半角/全角がロック可。
 *  ○NumLock中のテンキー入力に難あり(?)
 *    windib  だと NumLock中に SHIFT + テンキーで、SHIFTが一時的にオフ
 *    NumLockしてなければ問題なし。
 *    windib  だと この問題はない。
 *
 *  ○メニューモードでは、UNICODE を有効にする。
 *    こうすれば、「SHIFT」+「1」 を 「!」 と認識できるし、「SHIFT」+「2」
 *    は 「"」になる。しかし、  directx だと、入力できない文字があるぞ。
 *
 *  ○ところで、日本語Windowsでの101キーボードと、英語Windowsでの
 *    101キーボードって、同じ挙動なんだろうか・・・
 *    directx の時のキーコード割り当てが明らかに不自然なのだが。
 *===========================================================================*/


/******************************************************************************
 * イベントハンドリング
 *
 *  1/60毎に呼び出される。
 *****************************************************************************/
static int joystick_init(void);
static void joystick_exit(void);
static int analyze_keyconf_file(void);

void event_init(void) {
  /* Keymapping initialization */
  keymap_init();
  keymap_numlock_init();

  /* Joystick initialization */
  if (use_joydevice) {
    if (verbose_proc)
      printf("Initializing Joystick System ... ");
    int i = joystick_init();
    if (verbose_proc) {
      if (i >= 0)
        printf("OK (found %d joystick(s))\n", i);
      else
        printf("FAILED\n");
    }
  }

  /* test */
  if (show_fps) {
    if (display_fps_init() == FALSE) {
      show_fps = FALSE;
    }
  }
}

/*
 * 約 1/60 毎に呼ばれる
 */
void event_update(void) {
  SDL_Event E;
  SDL_Keycode keysym;
  int key88, x, y;

  SDL_PumpEvents(); /* イベントを汲み上げる */

  while (SDL_PeepEvents(&E, 1, SDL_GETEVENT, SDL_QUIT, SDL_USEREVENT)) {

    switch (E.type) {

    case SDL_KEYDOWN: /*------------------------------------------*/
    case SDL_KEYUP:
      keysym = E.key.keysym.sym;
      key88 = keymap2key88(keysym);
      if (key88 != KEY88_INVALID) {
        if (verbose_proc)
          printf("Found key: %s / %s (KEY88 %d)\n", E.type == SDL_KEYDOWN ? "DOWN" : "UP  ", SDL_GetKeyName(keysym),
                 key88);
        quasi88_key(key88, (E.type == SDL_KEYDOWN));
      }
      break;

    case SDL_MOUSEMOTION:       /*------------------------------------------*/
      if (sdl_mouse_rel_move) { /* マウスがウインドウの端に届いても */
                                /* 相対的な動きを検出できる場合     */
        x = E.motion.xrel;
        y = E.motion.yrel;

        quasi88_mouse_moved_rel(x, y);

      } else {

        x = E.motion.x;
        y = E.motion.y;

        quasi88_mouse_moved_abs(x, y);
      }
      break;

    case SDL_MOUSEWHEEL:
      if (E.wheel.y >= 0)
        key88 = KEY88_MOUSE_WUP;
      else
        key88 = KEY88_MOUSE_WDN;
      quasi88_mouse(key88, (E.type == SDL_MOUSEWHEEL)); // TODO: check this
      break;

    case SDL_MOUSEBUTTONDOWN: /*------------------------------------------*/
    case SDL_MOUSEBUTTONUP:
      /* マウス移動イベントも同時に処理する必要があるなら、
         quasi88_mouse_moved_abs/rel 関数をここで呼び出しておく */

      switch (E.button.button) {
      case SDL_BUTTON_LEFT:
        key88 = KEY88_MOUSE_L;
        break;
      case SDL_BUTTON_MIDDLE:
        key88 = KEY88_MOUSE_M;
        break;
      case SDL_BUTTON_RIGHT:
        key88 = KEY88_MOUSE_R;
        break;
      default:
        key88 = 0;
        break;
      }
      if (key88) {
        quasi88_mouse(key88, (E.type == SDL_MOUSEBUTTONDOWN));
      }
      break;

    case SDL_JOYAXISMOTION: /*------------------------------------------*/
      /*printf("%d %d %d\n",E.jaxis.which,E.jaxis.axis,E.jaxis.value);*/

      if (E.jbutton.which < JOY_MAX && joy_info[E.jbutton.which].dev != NULL) {

        int now, chg;
        T_JOY_INFO *joy = &joy_info[E.jbutton.which];
        int offset = (joy->num) * KEY88_PAD_OFFSET;

        if (E.jaxis.axis == 0) { /* 左右方向 */

          now = joy->axis & ~(AXIS_L | AXIS_R);

          if (E.jaxis.value < -0x4000)
            now |= AXIS_L;
          else if (E.jaxis.value > 0x4000)
            now |= AXIS_R;

          chg = joy->axis ^ now;
          if (chg & AXIS_L) {
            quasi88_pad(KEY88_PAD1_LEFT + offset, (now & AXIS_L));
          }
          if (chg & AXIS_R) {
            quasi88_pad(KEY88_PAD1_RIGHT + offset, (now & AXIS_R));
          }

        } else { /* 上下方向 */

          now = joy->axis & ~(AXIS_U | AXIS_D);

          if (E.jaxis.value < -0x4000)
            now |= AXIS_U;
          else if (E.jaxis.value > 0x4000)
            now |= AXIS_D;

          chg = joy->axis ^ now;
          if (chg & AXIS_U) {
            quasi88_pad(KEY88_PAD1_UP + offset, (now & AXIS_U));
          }
          if (chg & AXIS_D) {
            quasi88_pad(KEY88_PAD1_DOWN + offset, (now & AXIS_D));
          }
        }
        joy->axis = now;
      }
      break;

    case SDL_JOYBUTTONDOWN: /*------------------------------------------*/
    case SDL_JOYBUTTONUP:
      /*printf("%d %d\n",E.jbutton.which,E.jbutton.button);*/

      if (E.jbutton.which < JOY_MAX && joy_info[E.jbutton.which].dev != NULL) {

        T_JOY_INFO *joy = &joy_info[E.jbutton.which];
        int offset = (joy->num) * KEY88_PAD_OFFSET;

        if (E.jbutton.button < KEY88_PAD_BUTTON_MAX) {
          key88 = KEY88_PAD1_A + E.jbutton.button + offset;
          quasi88_pad(key88, (E.type == SDL_JOYBUTTONDOWN));
        }
      }
      break;

    case SDL_QUIT: /*------------------------------------------*/
      if (verbose_proc)
        printf("Window Closed !\n");
      quasi88_quit();
      break;

    case SDL_WINDOWEVENT: /*------------------------------------------*/
      /* -focus オプションを機能させたいなら、
         quasi88_focus_in / quasi88_focus_out を適宜呼び出す必要がある */

      switch (E.window.event) {
      case SDL_WINDOWEVENT_FOCUS_GAINED:
        quasi88_focus_in();
        graph_update(1, NULL);
        break;
      case SDL_WINDOWEVENT_FOCUS_LOST:
        quasi88_focus_out();
        break;
      case SDL_WINDOWEVENT_EXPOSED:
      case SDL_WINDOWEVENT_SIZE_CHANGED:
        graph_update(1, NULL);
        break;
      default:
        break;
      }
      break;

    case SDL_DISPLAYEVENT: /*------------------------------------------*/
      quasi88_expose();
      break;

    case SDL_USEREVENT: /*------------------------------------------*/
      if (E.user.code == 1) {
        display_fps(); /* test */
      }
      break;
    }
  }
}

/*
 * これは 終了時に1回だけ呼ばれる
 */
void event_exit(void) { joystick_exit(); }

/***********************************************************************
 * 現在のマウス座標取得関数
 *
 *  現在のマウスの絶対座標を *x, *y にセット
 ************************************************************************/

void event_get_mouse_pos(int *x, int *y) {
  int win_x, win_y;

  SDL_PumpEvents();
  SDL_GetMouseState(&win_x, &win_y);

  *x = win_x;
  *y = win_y;
}

/**
 * Rebind some keys on enabled software Numlock
 * @return always TRUE
 */
int event_numlock_on(void) {
  keymap_merge();

  return TRUE;
}

void event_numlock_off(void) { keymap_init(); }

/******************************************************************************
 * エミュレート／メニュー／ポーズ／モニターモード の 開始時の処理
 *
 *****************************************************************************/

void event_switch(void) {
  /* 既存のイベントをすべて破棄 */
  /* なんてことは、しない ? */
}

/******************************************************************************
 * ジョイスティック
 *****************************************************************************/
/**
 * Joystick initialization
 * @return -1 - initialization failed, >= 0 - number of found joysticks
 */
static int joystick_init(void) {
  SDL_Joystick *dev;
  int i, max, nr_button;

  joystick_num = 0;

  memset(joy_info, 0, sizeof(joy_info));
  for (i = 0; i < JOY_MAX; i++) {
    joy_info[i].dev = NULL;
  }

  /* ジョイスティックサブシステム初期化 */
  if (!SDL_WasInit(SDL_INIT_JOYSTICK)) {
    if (SDL_Init(SDL_INIT_JOYSTICK) != 0) {
      return -1;
    }
  }

  /* ジョイスティックの数を調べて、デバイスオープン */
  max = SDL_NumJoysticks();
  max = MIN(max, JOY_MAX); /* ワークの数だけ、有効 */

  for (i = 0; i < max; i++) {
    dev = SDL_JoystickOpen(i); /* i番目のジョイスティックをオープン */

    if (dev) {
      /* ボタンの数を調べる */
      nr_button = SDL_JoystickNumButtons(dev);
      nr_button = MIN(nr_button, BUTTON_MAX);

      joy_info[i].dev = dev;
      joy_info[i].num = joystick_num++;
      joy_info[i].nr_button = nr_button;
    }
  }

  if (joystick_num > 0) {               /* 1個以上オープンできたら  */
    SDL_JoystickEventState(SDL_ENABLE); /* イベント処理を有効にする */
  }

  return joystick_num; /* ジョイスティックの数を返す */
}

static void joystick_exit(void) {
  int i;

  if (joystick_num > 0) {

    for (i = 0; i < JOY_MAX; i++) {
      if (joy_info[i].dev) {
        SDL_JoystickClose(joy_info[i].dev);
        joy_info[i].dev = NULL;
      }
    }

    joystick_num = 0;
  }
}

int event_get_joystick_num(void) { return joystick_num; }

/****************************************************************************
 * Read the key setting file and set it.
 * False if there is no configuration file, if there is, process and return true
 *****************************************************************************/

/* A table that converts the SDL keysym string to an int value */

static const T_SYMBOL_TABLE sdlkeysym_list[] = {
    {"SDLK_BACKSPACE", SDLK_BACKSPACE},       /*   = 8,    */
    {"SDLK_TAB", SDLK_TAB},                   /*   = 9,    */
    {"SDLK_CLEAR", SDLK_CLEAR},               /*   = 12,   */
    {"SDLK_RETURN", SDLK_RETURN},             /*   = 13,   */
    {"SDLK_PAUSE", SDLK_PAUSE},               /*   = 19,   */
    {"SDLK_ESCAPE", SDLK_ESCAPE},             /*   = 27,   */
    {"SDLK_SPACE", SDLK_SPACE},               /*   = 32,   */
    {"SDLK_EXCLAIM", SDLK_EXCLAIM},           /*   = 33,   */
    {"SDLK_QUOTEDBL", SDLK_QUOTEDBL},         /*   = 34,   */
    {"SDLK_HASH", SDLK_HASH},                 /*   = 35,   */
    {"SDLK_DOLLAR", SDLK_DOLLAR},             /*   = 36,   */
    {"SDLK_AMPERSAND", SDLK_AMPERSAND},       /*   = 38,   */
    {"SDLK_QUOTE", SDLK_QUOTE},               /*   = 39,   */
    {"SDLK_LEFTPAREN", SDLK_LEFTPAREN},       /*   = 40,   */
    {"SDLK_RIGHTPAREN", SDLK_RIGHTPAREN},     /*   = 41,   */
    {"SDLK_ASTERISK", SDLK_ASTERISK},         /*   = 42,   */
    {"SDLK_PLUS", SDLK_PLUS},                 /*   = 43,   */
    {"SDLK_COMMA", SDLK_COMMA},               /*   = 44,   */
    {"SDLK_MINUS", SDLK_MINUS},               /*   = 45,   */
    {"SDLK_PERIOD", SDLK_PERIOD},             /*   = 46,   */
    {"SDLK_SLASH", SDLK_SLASH},               /*   = 47,   */
    {"SDLK_0", SDLK_0},                       /*   = 48,   */
    {"SDLK_1", SDLK_1},                       /*   = 49,   */
    {"SDLK_2", SDLK_2},                       /*   = 50,   */
    {"SDLK_3", SDLK_3},                       /*   = 51,   */
    {"SDLK_4", SDLK_4},                       /*   = 52,   */
    {"SDLK_5", SDLK_5},                       /*   = 53,   */
    {"SDLK_6", SDLK_6},                       /*   = 54,   */
    {"SDLK_7", SDLK_7},                       /*   = 55,   */
    {"SDLK_8", SDLK_8},                       /*   = 56,   */
    {"SDLK_9", SDLK_9},                       /*   = 57,   */
    {"SDLK_COLON", SDLK_COLON},               /*   = 58,   */
    {"SDLK_SEMICOLON", SDLK_SEMICOLON},       /*   = 59,   */
    {"SDLK_LESS", SDLK_LESS},                 /*   = 60,   */
    {"SDLK_EQUALS", SDLK_EQUALS},             /*   = 61,   */
    {"SDLK_GREATER", SDLK_GREATER},           /*   = 62,   */
    {"SDLK_QUESTION", SDLK_QUESTION},         /*   = 63,   */
    {"SDLK_AT", SDLK_AT},                     /*   = 64,   */
    {"SDLK_LEFTBRACKET", SDLK_LEFTBRACKET},   /*   = 91,   */
    {"SDLK_BACKSLASH", SDLK_BACKSLASH},       /*   = 92,   */
    {"SDLK_RIGHTBRACKET", SDLK_RIGHTBRACKET}, /*   = 93,   */
    {"SDLK_CARET", SDLK_CARET},               /*   = 94,   */
    {"SDLK_UNDERSCORE", SDLK_UNDERSCORE},     /*   = 95,   */
    {"SDLK_BACKQUOTE", SDLK_BACKQUOTE},       /*   = 96,   */
    {"SDLK_a", SDLK_a},                       /*   = 97,   */
    {"SDLK_b", SDLK_b},                       /*   = 98,   */
    {"SDLK_c", SDLK_c},                       /*   = 99,   */
    {"SDLK_d", SDLK_d},                       /*   = 100,  */
    {"SDLK_e", SDLK_e},                       /*   = 101,  */
    {"SDLK_f", SDLK_f},                       /*   = 102,  */
    {"SDLK_g", SDLK_g},                       /*   = 103,  */
    {"SDLK_h", SDLK_h},                       /*   = 104,  */
    {"SDLK_i", SDLK_i},                       /*   = 105,  */
    {"SDLK_j", SDLK_j},                       /*   = 106,  */
    {"SDLK_k", SDLK_k},                       /*   = 107,  */
    {"SDLK_l", SDLK_l},                       /*   = 108,  */
    {"SDLK_m", SDLK_m},                       /*   = 109,  */
    {"SDLK_n", SDLK_n},                       /*   = 110,  */
    {"SDLK_o", SDLK_o},                       /*   = 111,  */
    {"SDLK_p", SDLK_p},                       /*   = 112,  */
    {"SDLK_q", SDLK_q},                       /*   = 113,  */
    {"SDLK_r", SDLK_r},                       /*   = 114,  */
    {"SDLK_s", SDLK_s},                       /*   = 115,  */
    {"SDLK_t", SDLK_t},                       /*   = 116,  */
    {"SDLK_u", SDLK_u},                       /*   = 117,  */
    {"SDLK_v", SDLK_v},                       /*   = 118,  */
    {"SDLK_w", SDLK_w},                       /*   = 119,  */
    {"SDLK_x", SDLK_x},                       /*   = 120,  */
    {"SDLK_y", SDLK_y},                       /*   = 121,  */
    {"SDLK_z", SDLK_z},                       /*   = 122,  */
    {"SDLK_DELETE", SDLK_DELETE},             /*   = 127,  */
    {"SDLK_WORLD_0", 0xA0},                   /*   = 160,  */
    {"SDLK_WORLD_1", 0xA0},                   /*   = 161,  */
    {"SDLK_WORLD_2", 0xA0},                   /*   = 162,  */
    {"SDLK_WORLD_3", 0xA0},                   /*   = 163,  */
    {"SDLK_WORLD_4", 0xA0},                   /*   = 164,  */
    {"SDLK_WORLD_5", 0xA0},                   /*   = 165,  */
    {"SDLK_WORLD_6", 0xA0},                   /*   = 166,  */
    {"SDLK_WORLD_7", 0xA0},                   /*   = 167,  */
    {"SDLK_WORLD_8", 0xA0},                   /*   = 168,  */
    {"SDLK_WORLD_9", 0xA0},                   /*   = 169,  */
    {"SDLK_WORLD_10", 0xA0},                  /*   = 170,  */
    {"SDLK_WORLD_11", 0xA0},                  /*   = 171,  */
    {"SDLK_WORLD_12", 0xA0},                  /*   = 172,  */
    {"SDLK_WORLD_13", 0xA0},                  /*   = 173,  */
    {"SDLK_WORLD_14", 0xA0},                  /*   = 174,  */
    {"SDLK_WORLD_15", 0xA0},                  /*   = 175,  */
    {"SDLK_WORLD_16", 0xA0},                  /*   = 176,  */
    {"SDLK_WORLD_17", 0xA0},                  /*   = 177,  */
    {"SDLK_WORLD_18", 0xA0},                  /*   = 178,  */
    {"SDLK_WORLD_19", 0xA0},                  /*   = 179,  */
    {"SDLK_WORLD_20", 0xA0},                  /*   = 180,  */
    {"SDLK_WORLD_21", 0xA0},                  /*   = 181,  */
    {"SDLK_WORLD_22", 0xA0},                  /*   = 182,  */
    {"SDLK_WORLD_23", 0xA0},                  /*   = 183,  */
    {"SDLK_WORLD_24", 0xA0},                  /*   = 184,  */
    {"SDLK_WORLD_25", 0xA0},                  /*   = 185,  */
    {"SDLK_WORLD_26", 0xA0},                  /*   = 186,  */
    {"SDLK_WORLD_27", 0xA0},                  /*   = 187,  */
    {"SDLK_WORLD_28", 0xA0},                  /*   = 188,  */
    {"SDLK_WORLD_29", 0xA0},                  /*   = 189,  */
    {"SDLK_WORLD_30", 0xA0},                  /*   = 190,  */
    {"SDLK_WORLD_31", 0xA0},                  /*   = 191,  */
    {"SDLK_WORLD_32", 0xA0},                  /*   = 192,  */
    {"SDLK_WORLD_33", 0xA0},                  /*   = 193,  */
    {"SDLK_WORLD_34", 0xA0},                  /*   = 194,  */
    {"SDLK_WORLD_35", 0xA0},                  /*   = 195,  */
    {"SDLK_WORLD_36", 0xA0},                  /*   = 196,  */
    {"SDLK_WORLD_37", 0xA0},                  /*   = 197,  */
    {"SDLK_WORLD_38", 0xA0},                  /*   = 198,  */
    {"SDLK_WORLD_39", 0xA0},                  /*   = 199,  */
    {"SDLK_WORLD_40", 0xA0},                  /*   = 200,  */
    {"SDLK_WORLD_41", 0xA0},                  /*   = 201,  */
    {"SDLK_WORLD_42", 0xA0},                  /*   = 202,  */
    {"SDLK_WORLD_43", 0xA0},                  /*   = 203,  */
    {"SDLK_WORLD_44", 0xA0},                  /*   = 204,  */
    {"SDLK_WORLD_45", 0xA0},                  /*   = 205,  */
    {"SDLK_WORLD_46", 0xA0},                  /*   = 206,  */
    {"SDLK_WORLD_47", 0xA0},                  /*   = 207,  */
    {"SDLK_WORLD_48", 0xA0},                  /*   = 208,  */
    {"SDLK_WORLD_49", 0xA0},                  /*   = 209,  */
    {"SDLK_WORLD_50", 0xA0},                  /*   = 210,  */
    {"SDLK_WORLD_51", 0xA0},                  /*   = 211,  */
    {"SDLK_WORLD_52", 0xA0},                  /*   = 212,  */
    {"SDLK_WORLD_53", 0xA0},                  /*   = 213,  */
    {"SDLK_WORLD_54", 0xA0},                  /*   = 214,  */
    {"SDLK_WORLD_55", 0xA0},                  /*   = 215,  */
    {"SDLK_WORLD_56", 0xA0},                  /*   = 216,  */
    {"SDLK_WORLD_57", 0xA0},                  /*   = 217,  */
    {"SDLK_WORLD_58", 0xA0},                  /*   = 218,  */
    {"SDLK_WORLD_59", 0xA0},                  /*   = 219,  */
    {"SDLK_WORLD_60", 0xA0},                  /*   = 220,  */
    {"SDLK_WORLD_61", 0xA0},                  /*   = 221,  */
    {"SDLK_WORLD_62", 0xA0},                  /*   = 222,  */
    {"SDLK_WORLD_63", 0xA0},                  /*   = 223,  */
    {"SDLK_WORLD_64", 0xA0},                  /*   = 224,  */
    {"SDLK_WORLD_65", 0xA0},                  /*   = 225,  */
    {"SDLK_WORLD_66", 0xA0},                  /*   = 226,  */
    {"SDLK_WORLD_67", 0xA0},                  /*   = 227,  */
    {"SDLK_WORLD_68", 0xA0},                  /*   = 228,  */
    {"SDLK_WORLD_69", 0xA0},                  /*   = 229,  */
    {"SDLK_WORLD_70", 0xA0},                  /*   = 230,  */
    {"SDLK_WORLD_71", 0xA0},                  /*   = 231,  */
    {"SDLK_WORLD_72", 0xA0},                  /*   = 232,  */
    {"SDLK_WORLD_73", 0xA0},                  /*   = 233,  */
    {"SDLK_WORLD_74", 0xA0},                  /*   = 234,  */
    {"SDLK_WORLD_75", 0xA0},                  /*   = 235,  */
    {"SDLK_WORLD_76", 0xA0},                  /*   = 236,  */
    {"SDLK_WORLD_77", 0xA0},                  /*   = 237,  */
    {"SDLK_WORLD_78", 0xA0},                  /*   = 238,  */
    {"SDLK_WORLD_79", 0xA0},                  /*   = 239,  */
    {"SDLK_WORLD_80", 0xA0},                  /*   = 240,  */
    {"SDLK_WORLD_81", 0xA0},                  /*   = 241,  */
    {"SDLK_WORLD_82", 0xA0},                  /*   = 242,  */
    {"SDLK_WORLD_83", 0xA0},                  /*   = 243,  */
    {"SDLK_WORLD_84", 0xA0},                  /*   = 244,  */
    {"SDLK_WORLD_85", 0xA0},                  /*   = 245,  */
    {"SDLK_WORLD_86", 0xA0},                  /*   = 246,  */
    {"SDLK_WORLD_87", 0xA0},                  /*   = 247,  */
    {"SDLK_WORLD_88", 0xA0},                  /*   = 248,  */
    {"SDLK_WORLD_89", 0xA0},                  /*   = 249,  */
    {"SDLK_WORLD_90", 0xA0},                  /*   = 250,  */
    {"SDLK_WORLD_91", 0xA0},                  /*   = 251,  */
    {"SDLK_WORLD_92", 0xA0},                  /*   = 252,  */
    {"SDLK_WORLD_93", 0xA0},                  /*   = 253,  */
    {"SDLK_WORLD_94", 0xA0},                  /*   = 254,  */
    {"SDLK_WORLD_95", 0xA0},                  /*   = 255,  */
    {"SDLK_KP0", SDLK_KP_0},                  /*   = 256,  */
    {"SDLK_KP1", SDLK_KP_1},                  /*   = 257,  */
    {"SDLK_KP2", SDLK_KP_2},                  /*   = 258,  */
    {"SDLK_KP3", SDLK_KP_3},                  /*   = 259,  */
    {"SDLK_KP4", SDLK_KP_4},                  /*   = 260,  */
    {"SDLK_KP5", SDLK_KP_5},                  /*   = 261,  */
    {"SDLK_KP6", SDLK_KP_6},                  /*   = 262,  */
    {"SDLK_KP7", SDLK_KP_7},                  /*   = 263,  */
    {"SDLK_KP8", SDLK_KP_8},                  /*   = 264,  */
    {"SDLK_KP9", SDLK_KP_9},                  /*   = 265,  */
    {"SDLK_KP_PERIOD", SDLK_KP_PERIOD},       /*   = 266,  */
    {"SDLK_KP_DIVIDE", SDLK_KP_DIVIDE},       /*   = 267,  */
    {"SDLK_KP_MULTIPLY", SDLK_KP_MULTIPLY},   /*   = 268,  */
    {"SDLK_KP_MINUS", SDLK_KP_MINUS},         /*   = 269,  */
    {"SDLK_KP_PLUS", SDLK_KP_PLUS},           /*   = 270,  */
    {"SDLK_KP_ENTER", SDLK_KP_ENTER},         /*   = 271,  */
    {"SDLK_KP_EQUALS", SDLK_KP_EQUALS},       /*   = 272,  */
    {"SDLK_UP", SDLK_UP},                     /*   = 273,  */
    {"SDLK_DOWN", SDLK_DOWN},                 /*   = 274,  */
    {"SDLK_RIGHT", SDLK_RIGHT},               /*   = 275,  */
    {"SDLK_LEFT", SDLK_LEFT},                 /*   = 276,  */
    {"SDLK_INSERT", SDLK_INSERT},             /*   = 277,  */
    {"SDLK_HOME", SDLK_HOME},                 /*   = 278,  */
    {"SDLK_END", SDLK_END},                   /*   = 279,  */
    {"SDLK_PAGEUP", SDLK_PAGEUP},             /*   = 280,  */
    {"SDLK_PAGEDOWN", SDLK_PAGEDOWN},         /*   = 281,  */
    {"SDLK_F1", SDLK_F1},                     /*   = 282,  */
    {"SDLK_F2", SDLK_F2},                     /*   = 283,  */
    {"SDLK_F3", SDLK_F3},                     /*   = 284,  */
    {"SDLK_F4", SDLK_F4},                     /*   = 285,  */
    {"SDLK_F5", SDLK_F5},                     /*   = 286,  */
    {"SDLK_F6", SDLK_F6},                     /*   = 287,  */
    {"SDLK_F7", SDLK_F7},                     /*   = 288,  */
    {"SDLK_F8", SDLK_F8},                     /*   = 289,  */
    {"SDLK_F9", SDLK_F9},                     /*   = 290,  */
    {"SDLK_F10", SDLK_F10},                   /*   = 291,  */
    {"SDLK_F11", SDLK_F11},                   /*   = 292,  */
    {"SDLK_F12", SDLK_F12},                   /*   = 293,  */
    {"SDLK_F13", SDLK_F13},                   /*   = 294,  */
    {"SDLK_F14", SDLK_F14},                   /*   = 295,  */
    {"SDLK_F15", SDLK_F15},                   /*   = 296,  */
    {"SDLK_NUMLOCK", SDLK_NUMLOCKCLEAR},      /*   = 300,  */
    {"SDLK_CAPSLOCK", SDLK_CAPSLOCK},         /*   = 301,  */
    {"SDLK_SCROLLOCK", SDLK_SCROLLLOCK},      /*   = 302,  */
    {"SDLK_RSHIFT", SDLK_RSHIFT},             /*   = 303,  */
    {"SDLK_LSHIFT", SDLK_LSHIFT},             /*   = 304,  */
    {"SDLK_RCTRL", SDLK_RCTRL},               /*   = 305,  */
    {"SDLK_LCTRL", SDLK_LCTRL},               /*   = 306,  */
    {"SDLK_RALT", SDLK_RALT},                 /*   = 307,  */
    {"SDLK_LALT", SDLK_LALT},                 /*   = 308,  */
    {"SDLK_RMETA", SDLK_RGUI},                /*   = 309,  */
    {"SDLK_LMETA", SDLK_LGUI},                /*   = 310,  */
    {"SDLK_LSUPER", SDLK_LGUI},               /*   = 311,  */
    {"SDLK_RSUPER", SDLK_RGUI},               /*   = 312,  */
    {"SDLK_MODE", SDLK_MODE},                 /*   = 313,  */
    {"SDLK_COMPOSE", SDLK_LGUI},
    /*   = 314,  */                   // FIXME
    {"SDLK_HELP", SDLK_HELP},         /*   = 315,  */
    {"SDLK_PRINT", SDLK_PRINTSCREEN}, /*   = 316,  */
    {"SDLK_SYSREQ", SDLK_SYSREQ},     /*   = 317,  */
    {"SDLK_BREAK", SDLK_PAUSE},       /*   = 318,  */
    {"SDLK_MENU", SDLK_MENU},         /*   = 319,  */
    {"SDLK_POWER", SDLK_POWER},       /*   = 320,  */
    {"SDLK_EURO", SDLK_LGUI},
    /*   = 321,  */           // FIXME
    {"SDLK_UNDO", SDLK_UNDO}, /*   = 322,  */
};
/* デバッグ用 */
static const char *debug_sdlkeysym(int code) {
  int i;
  for (i = 0; i < COUNTOF(sdlkeysym_list); i++) {
    if (code == sdlkeysym_list[i].val)
      return sdlkeysym_list[i].name;
  }
  return "invalid";
}

/* キー設定ファイルの、識別タグをチェックするコールバック関数 */

static const char *identify_callback(const char *parm1, const char *parm2, const char *parm3) {
  if (my_strcmp(parm1, "[SDL]") == 0) {
    return NULL; /* 有効 */
  }

  return ""; /* 無効 */
}

/* キー設定ファイルの、設定を処理するコールバック関数 */

static const char *setting_callback(int type, int code, int key88, int numlock_key88) {
  keymap_add(code, key88);

  if (numlock_key88 >= 0) {
    keymap_numlock_add(code, numlock_key88);
  }

  return NULL; /* 有効 */
}

/* キー設定ファイルの処理関数 */

static int analyze_keyconf_file(void) {
  return config_read_keyconf_file(file_keyboard,           /* キー設定ファイル*/
                                  identify_callback,       /* 識別タグ行 関数 */
                                  sdlkeysym_list,          /* 変換テーブル    */
                                  COUNTOF(sdlkeysym_list), /* テーブルサイズ  */
                                  TRUE,                    /* 大小文字無視    */
                                  setting_callback);       /* 設定行 関数     */
}

/******************************************************************************
 * FPS
 *****************************************************************************/

/* test */

#define FPS_INTRVAL (1000) /* 1000ms毎に表示する */
static Uint32 display_fps_callback(Uint32 interval, void *dummy);

static int display_fps_init(void) {
  if (show_fps == FALSE)
    return TRUE;

  if (!SDL_WasInit(SDL_INIT_TIMER)) {
    if (SDL_InitSubSystem(SDL_INIT_TIMER)) {
      return FALSE;
    }
  }

  SDL_AddTimer(FPS_INTRVAL, display_fps_callback, NULL);
  return TRUE;
}

static Uint32 display_fps_callback(Uint32 interval, void *dummy) {
#if 0

    /* コールバック関数の内部からウインドウタイトルを変更するのは危険か ?
       「コールバック関数内ではどんな関数も呼び出すべきでない」となっている */

    display_fps();

#else

  /* SDL_PushEvent だけは呼び出しても安全となっているので、
     ユーザイベントで処理してみよう */

  SDL_Event user_event;

  user_event.type = SDL_USEREVENT;
  user_event.user.code = 1;
  user_event.user.data1 = NULL;
  user_event.user.data2 = NULL;
  SDL_PushEvent(&user_event); /* エラーは無視 */
#endif

  return FPS_INTRVAL;
}

static void display_fps(void) {
  static int prev_drawn_count;
  static int prev_vsync_count;
  int now_drawn_count;
  int now_vsync_count;

  if (show_fps == FALSE)
    return;

  now_drawn_count = quasi88_info_draw_count();
  now_vsync_count = quasi88_info_vsync_count();

  if (quasi88_is_exec()) {
    char buf[32];

    sprintf(buf, "FPS: %3d (VSYNC %3d)", now_drawn_count - prev_drawn_count, now_vsync_count - prev_vsync_count);

    graph_set_window_title(buf);
  }

  prev_drawn_count = now_drawn_count;
  prev_vsync_count = now_vsync_count;
}
