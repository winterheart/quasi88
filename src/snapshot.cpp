/************************************************************************/
/*                                  */
/* スクリーン スナップショット                     */
/*                                  */
/************************************************************************/

#include <algorithm>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include "quasi88.h"

#include "crtcdmac.h"
#include "file-op.h"
#include "lodepng.h"
#include "screen.h"
#include "screen-func.h"
#include "snapshot.h"
#include "snddrv.h"

char file_snap[QUASI88_MAX_FILENAME];   /* スナップショットベース部 */
int snapshot_format = SNAPSHOT_FMT_BMP; /* スナップショットフォーマット   */

char snapshot_cmd[SNAPSHOT_CMD_SIZE];   /* スナップショット後コマンド    */
char snapshot_cmd_do = false;           /* コマンド実行の有無      */

#ifdef USE_SSS_CMD
char snapshot_cmd_enable = true; /* コマンド実行の可否      */
#else
char snapshot_cmd_enable = false; /* コマンド実行の可否      */
#endif

/* スナップショットを作成する一時バッファと、色のインデックス */

char screen_snapshot[640 * 400];
static PC88_PALETTE_T pal[16 + 1];

/* Initialize snapshot file name etc. */
void screen_snapshot_init() {
  const char *s;

  if (file_snap[0] == '\0') {
    filename_init_snap(false);
  }

  memset(snapshot_cmd, 0, SNAPSHOT_CMD_SIZE);
  s = getenv("QUASI88_SSS_CMD"); /* 実行コマンド */
  if (s && (strlen(s) < SNAPSHOT_CMD_SIZE)) {
    strcpy(snapshot_cmd, s);
  }

  snapshot_cmd_do = false; /* 初期値は、コマンド実行『しない』にする */

  if (file_wav[0] == '\0') {
    filename_init_wav(false);
  }
}
void screen_snapshot_exit() { waveout_save_stop(); }

/* Generate screen image */
typedef int (*SNAPSHOT_FUNC)();

static void make_snapshot() {
  int vram_mode, text_mode;
  SNAPSHOT_FUNC(*list)[4][2];

  /* skipline の場合は、予め snapshot_clear() を呼び出しておく */

  switch (use_interlace) {
  case SCREEN_INTERLACE_NO:
    list = snapshot_list_normal;
    break;
  case SCREEN_INTERLACE_YES:
    list = snapshot_list_itlace;
    break;
  case SCREEN_INTERLACE_SKIP:
    snapshot_clear();
    list = snapshot_list_skipln;
    break;
  default:
    fprintf(stderr, "Unknown interlace option %d. What I gonna do?!\n", use_interlace);
    exit(1);
  }

  /* VRAM/TEXT の内容を screen_snapshot[] に転送 */

  if (sys_ctrl & SYS_CTRL_80) {
    if (CRTC_SZ_LINES == 25) {
      text_mode = V_80x25;
    } else {
      text_mode = V_80x20;
    }
  } else {
    if (CRTC_SZ_LINES == 25) {
      text_mode = V_40x25;
    } else {
      text_mode = V_40x20;
    }
  }

  if (grph_ctrl & GRPH_CTRL_VDISP) {
    if (grph_ctrl & GRPH_CTRL_COLOR) {
      /* Color */
      vram_mode = V_COLOR;
    } else {
      if (grph_ctrl & GRPH_CTRL_200) {
        /* Black and white */
        vram_mode = V_MONO;
      } else {
        /* 400 lines */
        vram_mode = V_HIRESO;
      }
    }
  } else {
    /* 非表示 */
    vram_mode = V_UNDISP;
  }

  (list[vram_mode][text_mode][V_ALL])();

  /* パレットの内容を pal[] に転送 */

  screen_get_emu_palette(pal);

  /* pal[16] is used fixed to black */
  pal[16].red = 0;
  pal[16].green = 0;
  pal[16].blue = 0;
}

/* Create raw image from palette screen */
static std::vector<unsigned char> make_raw_image() {
  std::vector<unsigned char> raw;
  raw.reserve(640 * 400 * 3);
  for (auto i : screen_snapshot) {
    raw.push_back(pal[i].red);
    raw.push_back(pal[i].green);
    raw.push_back(pal[i].blue);
  }
  return raw;
}

/* Output the captured content to a file in bmp format (win) */
static bool save_snapshot_bmp(OSD_FILE *fp, std::vector<uint8_t> raw) {
  static const unsigned char header[] = {
      /* Header */
      'B', 'M',               /* BM */
      0x36, 0xb8, 0x0b, 0x00, /* File size (0xbb836 / 768054) */
      0x00, 0x00, 0x00, 0x00, /* Reserved */
      0x36, 0x00, 0x00, 0x00, /* Image data offset (0x36) */
      /* InfoHeader */
      0x28, 0x00, 0x00, 0x00, /* Information size (0x28) */
      0x80, 0x02, 0x00, 0x00, /* Horizontal width of bitmap in pixels (0x280 / 640) */
      0x90, 0x01, 0x00, 0x00, /* Vertical height of bitmap in pixels (0x190 / 400) */
      0x01, 0x00,             /* Number of Planes */
      0x18, 0x00,             /* Bits per Pixel (0x18 / 24 - 24bit RGB) */
      0x00, 0x00, 0x00, 0x00, /* Compression (0 -  BI_RGB, no compression) */
      0x00, 0x00, 0x00, 0x00, /* (compressed) Size of Image */
      0x00, 0x00, 0x00, 0x00, /* Horizontal resolution: Pixels/meter */
      0x00, 0x00, 0x00, 0x00, /* Vertical resolution: Pixels/meter */
      0x00, 0x00, 0x00, 0x00, /* Number of actually used colors */
      0x00, 0x00, 0x00, 0x00, /* Number of important colors */
  };

  size_t size_write = sizeof(header);
  if (osd_fwrite(header, sizeof(char), size_write, fp) < size_write) {
    return false;
  }

  const int length = 3 * 640;

  // BMP creators, shame on you
  for (int y = 400; y > 0; y--) {
    auto first = raw.begin() + length * (y - 1);
    auto last = raw.end() + length * y;
    std::vector<uint8_t> line(first, last);
    if (osd_fwrite(reinterpret_cast<char *>(line.data()), sizeof(char), length, fp) < length) {
      return false;
    }
  }

  return true;
}

/* Output the captured content to a file in ppm format (raw) */
static bool save_snapshot_ppm(OSD_FILE *fp, std::vector<uint8_t> raw) {
  unsigned char buf[32];

  strcpy((char *)buf, "P6\n"
                      "# QUASI88kai\n"
                      "640 400\n"
                      "255\n");
  size_t size_write = strlen((char *)buf);
  if (osd_fwrite(buf, sizeof(char), size_write, fp) < size_write) {
    return false;
  }
  if (osd_fwrite(reinterpret_cast<char *>(raw.data()), sizeof(char), raw.size(), fp) < raw.size()) {
    return false;
  }

  return true;
}

/* Output the captured content to a file in raw format */
static bool save_snapshot_raw(OSD_FILE *fp, std::vector<uint8_t> raw) {
  if (osd_fwrite(reinterpret_cast<char *>(raw.data()), sizeof(char), raw.size(), fp) < raw.size()) {
    return false;
  }

  return true;
}

static bool save_snapshot_png(OSD_FILE *fp, std::vector<uint8_t> raw) {
  unsigned char *buf;
  size_t length;
  if (lodepng_encode24(&buf, &length, reinterpret_cast<unsigned char *>(raw.data()), 640, 400)) {
    return false;
  }
  if (osd_fwrite(buf, sizeof(char), length, fp) < length) {
    return false;
  }

  return true;
}

/***********************************************************************
 * 画面のスナップショットをセーブする
 *  現時点のVRAMをもとにスナップショットを作成する。
 *  現在表示されている画面をセーブするわけではない。
 *
 *  環境変数 ${QUASI88_SSS_CMD} が定義されている場合、セーブ後に
 *  この内容をコマンドとして実行する。この際、%a がファイル名に、
 *  %b がファイル名からサフィックスを削除したものに、置き換わる。
 *
 *  例) setenv QUASI88_SSS_CMD 'ppmtopng %a > %b.png'
 *
 ************************************************************************/

/*
  screen_snapshot_save() で、セーブされるファイル名は、
  自動的に設定されるので、セーブ時に指定する必要はない。
  (ディスクイメージの名前などに基づき、設定される)

    ファイル名は、 /my/snap/dir/save0001.bmp のように、連番 + 拡張子 が
    付加される。連番は、 0000 〜 9999 で、既存のファイルと重複しない
    ように適宜決定される。

  ----------------------------------------------------------------------
  ファイル名を変更したい場合は、以下の関数を使う。

  ファイル名の取得 … filename_get_snap_base()
    取得できるファイル名は、/my/snap/dir/save のように、連番と拡張子は
    削除されている。

  ファイル名の設定 … filename_set_snap_base()
    例えば、/my/snap/dir/save0001.bmp と設定しても、末尾の 連番と 拡張子は
    削除される。そのため、filename_set_snap_base() で設定したファイル名と
    filename_get_snap_base() で取得したファイル名は一致しないことがある。
    なお、NULL を指定すると、初期値がセットされる。

*/

/* file_snap[] の末尾が NNNN.suffix となっている場合、末尾を削除する。
   NNNN は連番で 0000〜9999。
   suffix は拡張子で、以下のものを対象とする。*/
static const char *snap_suffix[] = {".ppm",  ".PPM",  ".xpm", ".XPM", ".png",  ".PNG",  ".bmp", ".BMP",  ".rgb",
                                    ".RGB",  ".raw",  ".RAW", ".gif", ".GIF",  ".xwd",  ".XWD", ".pict", ".PICT",
                                    ".tiff", ".TIFF", ".tif", ".TIF", ".jpeg", ".JPEG", ".jpg", ".JPG",  nullptr};
static void truncate_filename(char filename[], const char *suffix[]) {
  int i;
  char *p;

  for (i = 0; suffix[i]; i++) {

    if (strlen(filename) > strlen(suffix[i]) + 4) {

      p = &filename[strlen(filename) - strlen(suffix[i]) - 4];
      if (isdigit(*(p + 0)) && isdigit(*(p + 1)) && isdigit(*(p + 2)) && isdigit(*(p + 3)) &&
          strcmp(p + 4, suffix[i]) == 0) {

        *p = '\0';
        /*  printf("screen-snapshot : filename truncated (%s)\n", filename);*/
        break;
      }
    }
  }
}

void filename_set_snap_base(const char *filename) {
  if (filename) {
    strncpy(file_snap, filename, QUASI88_MAX_FILENAME - 1);
    file_snap[QUASI88_MAX_FILENAME - 1] = '\0';

    truncate_filename(file_snap, snap_suffix);
  } else {
    filename_init_snap(false);
  }
}
const char *filename_get_snap_base() { return file_snap; }

int screen_snapshot_save() {
  static char filename[QUASI88_MAX_FILENAME + sizeof("NNNN.suffix")];
  static int snapshot_no = 0; /* 連番 */

  static const char *suffix[] = {
      ".bmp",
      ".ppm",
      ".raw",
      ".png",
  };

  OSD_FILE *fp;
  int i, j, success;

  if (snapshot_format >= COUNTOF(suffix))
    return false;

  /* ファイル名が未指定の場合、初期ファイル名にする */

  if (file_snap[0] == '\0') {
    filename_init_snap(false);
  }

  /* file_snap[] の末端が NNNN.suffix なら削除 */

  truncate_filename(file_snap, snap_suffix);

  /* 存在しないファイル名を探しだす (0000.suffix〜 9999.suffix) */

  success = false;
  for (j = 0; j < 10000; j++) {

    size_t len = sprintf(filename, "%s%04d", file_snap, snapshot_no);
    if (++snapshot_no > 9999)
      snapshot_no = 0;

    for (i = 0; snap_suffix[i]; i++) {
      filename[len] = '\0';
      strcat(filename, snap_suffix[i]);
      if (osd_file_stat(filename) != FILE_STAT_NOEXIST)
        break;
    }
    if (snap_suffix[i] == nullptr) { /* 見つかった */
      filename[len] = '\0';
      strcat(filename, suffix[snapshot_format]);
      success = true;
      break;
    }
  }

  /* ファイルを開いて、スナップショットデータを書き込む */

  if (success) {

    success = false;
    if ((fp = osd_fopen(FTYPE_SNAPSHOT_PPM, filename, "wb"))) {

      make_snapshot();
      std::vector<uint8_t> raw = make_raw_image();

      switch (snapshot_format) {
      case SNAPSHOT_FMT_BMP:
        success = save_snapshot_bmp(fp, raw);
        break;
      case SNAPSHOT_FMT_PPM:
        success = save_snapshot_ppm(fp, raw);
        break;
      case SNAPSHOT_FMT_RAW:
        success = save_snapshot_raw(fp, raw);
        break;
      case SNAPSHOT_FMT_PNG:
        success = save_snapshot_png(fp, raw);
        break;
      default:
        success = false;
      }

      osd_fclose(fp);
    }
    /*
        printf("screen-snapshot : %s ... %s\n",
                    filename, (success) ? "OK" : "FAILED");
    */
  }

  /* 書き込み成功後、コマンドを実行する */

#ifdef USE_SSS_CMD

  if (success && snapshot_cmd_enable && snapshot_cmd_do && snapshot_cmd[0]) {

    size_t a_len, b_len;
    char *cmd, *s, *d;

    a_len = strlen(filename);
    b_len = a_len - 4; /* サフィックス ".???" の4文字分減算 */

    size_t len = 0;
    s = snapshot_cmd; /* コマンドの %a, %b は置換するので   */
    while (*s) {      /* コマンドの文字長がどうなるか数える */
      if (*s == '%') {
        switch (*(s + 1)) {
        case '%':
          len++;
          s++;
          break;
        case 'a':
          len += a_len;
          s++;
          break;
        case 'b':
          len += b_len;
          s++;
          break;
        default:
          len++;
          break;
        }
      } else {
        len++;
      }

      s++;
    }
    /* 数えた文字数分、malloc する */
    cmd = (char *)malloc(len + 1);
    if (cmd) {

      s = snapshot_cmd;
      d = cmd;
      while (*s) { /* コマンドの %a, %b を置換して格納していく */
        if (*s == '%') {
          switch (*(s + 1)) {
          case '%':
            *d++ = *s;
            s++;
            break;
          case 'a':
            memcpy(d, filename, a_len);
            d += a_len;
            s++;
            break;
          case 'b':
            memcpy(d, filename, b_len);
            d += b_len;
            s++;
            break;
          default:
            *d++ = *s;
          }
        } else {
          *d++ = *s;
        }
        s++;
      }
      *d = '\0';
      /* 出来上がったコマンドを実行 */
      printf("[SNAPSHOT command]%% %s\n", cmd);
      system(cmd);

      free(cmd);
    }
  }
#endif /* USE_SSS_CMD */

  return success;
}

/* Save sound output */
char file_wav[QUASI88_MAX_FILENAME]; /* サウンド出力ベース部   */

static const char *wav_suffix[] = {".wav", ".WAV", nullptr};

void filename_set_wav_base(const char *filename) {
  if (filename) {
    strncpy(file_wav, filename, QUASI88_MAX_FILENAME - 1);
    file_wav[QUASI88_MAX_FILENAME - 1] = '\0';

    truncate_filename(file_wav, wav_suffix);
  } else {
    filename_init_wav(false);
  }
}

const char *filename_get_wav_base() { return file_wav; }

int waveout_save_start() {
  static char filename[QUASI88_MAX_FILENAME + sizeof("NNNN.suffix")];
  static int waveout_no = 0; /* 連番 */

  static const char *suffix = ".wav";

  int i, j, len, success;

  /* ファイル名が未指定の場合、初期ファイル名にする */

  if (file_wav[0] == '\0') {
    filename_init_wav(false);
  }

  /* file_wav[] の末端が NNNN.suffix なら削除 */

  truncate_filename(file_wav, wav_suffix);

  /* 存在しないファイル名を探しだす (0000.suffix〜 9999.suffix) */

  success = false;
  for (j = 0; j < 10000; j++) {

    len = sprintf(filename, "%s%04d", file_wav, waveout_no);
    if (++waveout_no > 9999)
      waveout_no = 0;

    for (i = 0; wav_suffix[i]; i++) {
      filename[len] = '\0';
      strcat(filename, wav_suffix[i]);
      if (osd_file_stat(filename) != FILE_STAT_NOEXIST)
        break;
    }
    if (wav_suffix[i] == nullptr) { /* 見つかった */
      filename[len] = '\0';
      strcat(filename, suffix);
      success = true;
      break;
    }
  }

  /* ファイル名が決まったので、ファイルをを開く */

  if (success) {
    success = xmame_wavout_open(filename);
  }

  return success;
}

void waveout_save_stop() { xmame_wavout_close(); }
