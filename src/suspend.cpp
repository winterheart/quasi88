/************************************************************************/
/*                                  */
/* サスペンド、レジューム処理                      */
/*                                  */
/*                                  */
/*                                  */
/************************************************************************/

#include <cmath>    // for double_t on Windows
#include <cstring>
#include <cctype>

#include "quasi88.h"

#include "Core/Log.h"

#include "byteswap.h"
#include "file-op.h"
#include "suspend.h"
#include "utility.h"

int resume_flag = false;  /* 起動時のレジューム  */
int resume_force = false; /* 強制レジューム    */
int resume_file = false;  /* ファイル名指定あり  */

char file_state[QUASI88_MAX_FILENAME]; /* ステートファイル名   */

/*======================================================================
  ステートファイルの構成

    ヘッダ部    32バイト
    データ部    不定バイト
    データ部    不定バイト
      ：
      ：
    終端部


  ヘッダ部  32バイト とりあえず、内容は以下のとおり。_ は NUL文字
                QUASI88_0.6.0_1_________________
                    識別ID        QUASI88
                    バージョン 0.6.0
                    互換番号    1

  データ部  ID      ASCII4バイト
        データ長    4バイト整数 (リトルエンディアン)
        データ       不定バイト数

        データ長には、 ID と 自身のデータ長 の 8バイトは含まない

  終端部 ID      0x00 4バイト
        データ長    0x00 4バイト


  整数値はすべてリトルエンディアンにでもしておこう。

  データ部の詳細は、その都度考えることにします・・・
  ======================================================================*/

#define SZ_HEADER (32)

/*----------------------------------------------------------------------
 * ステートファイルにデータを記録する関数
 * ステートファイルに記録されたデータを取り出す関数
 *      整数データはリトルエンディアンで記録
 *      int 型、 short 型、char 型、pair 型、256バイトブロック、
 *      文字列(1023文字まで)、double型 (1000000倍してintに変換)
 *----------------------------------------------------------------------*/
INLINE bool statesave_int(OSD_FILE *fp, const int32_t *val) {
  int32_t r = QUASI88::convert_le(*val);
  return (osd_fwrite(&r, sizeof(r), 1, fp) == 1);
}

INLINE bool stateload_int(OSD_FILE *fp, int32_t *val) {
  int32_t r;
  if (osd_fread(&r, sizeof(r), 1, fp) != 1)
    return false;
  *val = QUASI88::convert_le(r);
  return true;
}

INLINE bool statesave_short(OSD_FILE *fp, const int16_t *val) {
  int16_t r = QUASI88::convert_le(*val);
  return (osd_fwrite(&r, sizeof(r), 1, fp) == 1);
}

INLINE int stateload_short(OSD_FILE *fp, int16_t *val) {
  int16_t r;
  if (osd_fread(&r, sizeof(r), 1, fp) != 1)
    return false;
  *val = QUASI88::convert_le(r);
  return true;
}

INLINE int statesave_char(OSD_FILE *fp, int8_t *val) {
  return (osd_fwrite(val, sizeof(int8_t), 1, fp) == 1);
}

INLINE int stateload_char(OSD_FILE *fp, int8_t *val) {
  if (osd_fread(val, sizeof(int8_t), 1, fp) != 1)
    return false;
  return true;
}

INLINE int statesave_pair(OSD_FILE *fp, pair *val) {
  uint16_t r = QUASI88::convert_le(val->W);
  return (osd_fwrite(QUASI88::convert_le(&r), sizeof(r), 1, fp) == 1);
}

INLINE int stateload_pair(OSD_FILE *fp, pair *val) {
  uint16_t r;
  if (osd_fread(&r, sizeof(r), 1, fp) != 1)
    return false;
  (*val).W = QUASI88::convert_le(r);
  return true;
}

INLINE int statesave_256(OSD_FILE *fp, char *array) {
  return (osd_fwrite(array, sizeof(char), 256, fp) == 256);
}

INLINE int stateload_256(OSD_FILE *fp, char *array) {
  if (osd_fread(array, sizeof(char), 256, fp) != 256)
    return false;
  return true;
}

INLINE int statesave_str(OSD_FILE *fp, char *str) {
  char wk[1024];

  if (strlen(str) >= 1024 - 1)
    return false;

  memset(wk, 0, 1024);
  strcpy(wk, str);

  return (osd_fwrite(wk, sizeof(char), 1024, fp) == 1024);
}

INLINE int stateload_str(OSD_FILE *fp, char *str) {
  if (osd_fread(str, sizeof(char), 1024, fp) != 1024)
    return false;
  return true;
}

INLINE int statesave_double(OSD_FILE *fp, double_t *val) {
  auto r = (int32_t)(*val * 1000000.0);
  return (osd_fwrite(QUASI88::convert_le(&r), sizeof(r), 1, fp) == 1);
}

INLINE int stateload_double(OSD_FILE *fp, double_t *val) {
  int32_t r;
  if (osd_fread(&r, sizeof(r), 1, fp) != 1)
    return false;
  *val = QUASI88::convert_le(r) / 1000000.0;
  return true;
}

/*----------------------------------------------------------------------
 * IDを検索する関数  戻り値：データサイズ (-1でエラー、-2でデータなし)
 * IDを書き込む関数  戻り値：データサイズ (-1でエラー)
 *----------------------------------------------------------------------*/

static int read_id(OSD_FILE *fp, const char id[4]) {
  char c[4];
  int size;

  /* ファイル先頭から検索。まずはヘッダをスキップ */
  if (osd_fseek(fp, SZ_HEADER, SEEK_SET) != 0)
    return -1;

  /* ID が合致するまで SEEK していく */
  for (;;) {
    if (osd_fread(c, sizeof(char), 4, fp) != 4)
      return -1;
    if (!stateload_int(fp, &size))
      return -1;

    if (memcmp(c, id, 4) == 0) { /* ID合致した */
      return size;
    }

    if (memcmp(c, "\0\0\0\0", 4) == 0)
      return -2; /* データ終端 */

    if (osd_fseek(fp, size, SEEK_CUR) != 0)
      return -1;
  }
}

static int write_id(OSD_FILE *fp, const char id[4], int size) {
  /* ファイル現在位置に、書き込む */

  if (osd_fwrite(id, sizeof(char), 4, fp) != 4)
    return -1;
  if (!statesave_int(fp, &size))
    return -1;

  return size;
}

/*======================================================================
 *
 * ステートファイルにデータを記録
 *
 *======================================================================*/
static OSD_FILE *statesave_fp;

/* ヘッダ情報を書き込む */
static int statesave_header() {
  size_t off;
  char header[SZ_HEADER];
  OSD_FILE *fp = statesave_fp;

  memset(header, 0, SZ_HEADER);
  off = 0;
  memcpy(&header[off], STATE_ID, sizeof(STATE_ID));
  off += sizeof(STATE_ID);
  memcpy(&header[off], STATE_VER, sizeof(STATE_VER));
  off += sizeof(STATE_VER);
  memcpy(&header[off], STATE_REV, sizeof(STATE_REV));

  if (osd_fseek(fp, 0, SEEK_SET) == 0 && osd_fwrite(header, sizeof(char), SZ_HEADER, fp) == SZ_HEADER) {
    return STATE_OK;
  }

  return STATE_ERR;
}

/* メモリブロックを書き込む */
int statesave_block(const char id[4], void *top, int size) {
  OSD_FILE *fp = statesave_fp;

  if (write_id(fp, id, size) == size && osd_fwrite((char *)top, sizeof(char), size, fp) == (size_t)size) {
    return STATE_OK;
  }

  return STATE_ERR;
}

/* テーブル情報に従い、書き込む */
int statesave_table(const char id[4], T_SUSPEND_W *tbl) {
  OSD_FILE *fp = statesave_fp;
  T_SUSPEND_W *p = tbl;
  int size = 0;
  int loop = true;

  while (loop) { /* 書き込むサイズの総計を計算 */
    switch (p->type) {
    case TYPE_END:
      loop = false;
      break;
    case TYPE_DOUBLE:
    case TYPE_INT:
    case TYPE_LONG:
      size += 4;
      break;
    case TYPE_PAIR:
    case TYPE_SHORT:
    case TYPE_WORD:
      size += 2;
      break;
    case TYPE_CHAR:
    case TYPE_BYTE:
      size += 1;
      break;
    case TYPE_STR:
      size += 1024;
      break;
    case TYPE_256:
      size += 256;
      break;
    }
    p++;
  }

  if (write_id(fp, id, size) != size)
    return STATE_ERR;

  for (;;) {
    switch (tbl->type) {

    case TYPE_END:
      return STATE_OK;

    case TYPE_INT:
    case TYPE_LONG:
      if (!statesave_int(fp, (int32_t *)tbl->work))
        return STATE_ERR;
      break;

    case TYPE_SHORT:
    case TYPE_WORD:
      if (!statesave_short(fp, (int16_t *)tbl->work))
        return STATE_ERR;
      break;

    case TYPE_CHAR:
    case TYPE_BYTE:
      if (!statesave_char(fp, (int8_t *)tbl->work))
        return STATE_ERR;
      break;

    case TYPE_PAIR:
      if (!statesave_pair(fp, (pair *)tbl->work))
        return STATE_ERR;
      break;

    case TYPE_DOUBLE:
      if (!statesave_double(fp, (double_t *)tbl->work))
        return STATE_ERR;
      break;

    case TYPE_STR:
      if (!statesave_str(fp, (char *)tbl->work))
        return STATE_ERR;
      break;

    case TYPE_256:
      if (!statesave_256(fp, (char *)tbl->work))
        return STATE_ERR;
      break;

    default:
      return STATE_ERR;
    }

    tbl++;
  }
}

/*======================================================================
 *
 * ステートファイルからデータを取り出す
 *
 *======================================================================*/
static OSD_FILE *stateload_fp;
static int statefile_rev = 0;

/* ヘッダ情報を取り出す */
static int stateload_header() {
  char header[SZ_HEADER + 1];
  char *title, *ver, *rev;
  OSD_FILE *fp = stateload_fp;

  if (osd_fseek(fp, 0, SEEK_SET) == 0 && osd_fread(header, sizeof(char), SZ_HEADER, fp) == SZ_HEADER) {
    header[SZ_HEADER] = '\0';

    title = header;
    ver = title + strlen(title) + 1;
    rev = ver + strlen(ver) + 1;
    QLOG_DEBUG("suspend", "Stateload: file header is \"{}\", \"{}\", \"{}\".", title, ver, rev);

    if (memcmp(title, STATE_ID, sizeof(STATE_ID)) != 0) {
      QLOG_WARN("suspend", "Stateload: ID mismatch ('{}' != '{}')", STATE_ID, title);
    } else {
      if (memcmp(ver, STATE_VER, sizeof(STATE_VER)) != 0) {
        QLOG_ERROR("suspend", "Stateload: version mismatch ('{}' != '{}')", STATE_VER, ver);
        if (!resume_force)
          return STATE_ERR;

      } else {
        if (memcmp(rev, STATE_REV, sizeof(STATE_REV)) != 0) {
          QLOG_WARN("suspend", "Stateload: older revision ('{}' != '{}')", STATE_REV, rev);
        }
      }

      if (rev[0] == '1')
        statefile_rev = 1;
      else
        statefile_rev = 0;
      return STATE_OK;
    }
  }

  return STATE_ERR;
}

/* メモリブロックを取り出す */
int stateload_block(const char id[4], void *top, int size) {
  OSD_FILE *fp = stateload_fp;

  int s = read_id(fp, id);

  if (s == -1)
    return STATE_ERR;
  if (s == -2)
    return STATE_ERR_ID;
  if (s != size)
    return STATE_ERR_SIZE;

  if (osd_fread((char *)top, sizeof(char), size, fp) == (size_t)size) {
    return STATE_OK;
  }

  return STATE_ERR;
}

/* テーブル情報に従い、取り出す */
int stateload_table(const char id[4], T_SUSPEND_W *tbl) {
  OSD_FILE *fp = stateload_fp;
  int size = 0;
  int s = read_id(fp, id);

  if (s == -1)
    return STATE_ERR;
  if (s == -2)
    return STATE_ERR_ID;

  for (;;) {
    switch (tbl->type) {

    case TYPE_END:
      if (s != size)
        return STATE_ERR_SIZE;
      else
        return STATE_OK;

    case TYPE_INT:
    case TYPE_LONG:
      if (!stateload_int(fp, (int *)tbl->work))
        return STATE_ERR;
      size += 4;
      break;

    case TYPE_SHORT:
    case TYPE_WORD:
      if (!stateload_short(fp, (short *)tbl->work))
        return STATE_ERR;
      size += 2;
      break;

    case TYPE_CHAR:
    case TYPE_BYTE:
      if (!stateload_char(fp, (int8_t *)tbl->work))
        return STATE_ERR;
      size += 1;
      break;

    case TYPE_PAIR:
      if (!stateload_pair(fp, (pair *)tbl->work))
        return STATE_ERR;
      size += 2;
      break;

    case TYPE_DOUBLE:
      if (!stateload_double(fp, (double *)tbl->work))
        return STATE_ERR;
      size += 4;
      break;

    case TYPE_STR:
      if (!stateload_str(fp, (char *)tbl->work))
        return STATE_ERR;
      size += 1024;
      break;

    case TYPE_256:
      if (!stateload_256(fp, (char *)tbl->work))
        return STATE_ERR;
      size += 256;
      break;

    default:
      return STATE_ERR;
    }

    tbl++;
  }
}

/* リビジョン取得 */
int statefile_revision() { return statefile_rev; }

/***********************************************************************
 *
 *
 *
 ************************************************************************/

/*
  statesave() / stateload() でセーブ/ロードされるファイル名は、
  自動的に設定されるので、セーブ/ロード時に指定する必要はない。
  (ディスクイメージの名前などに基づき、設定される)

  でも、これだと1種類しかロード/セーブできずに不便なので、
  filename_set_state_serial(int serial) で連番を指定できる。

            ステートファイル名
    引数 '5'  /my/state/dir/file-5.sta
    引数 'z'  /my/state/dir/file-z.sta
    引数 0        /my/state/dir/file.sta

  よって、連番指定でステートセーブする場合は、
      filename_set_state_serial('1');
      statesave();
  のように呼び出す。

  ----------------------------------------------------------------------
  ファイル名を変更したい場合は、以下の関数を使う。

  ファイル名の取得 … filename_get_state()
    現在設定されているステートファイル名が取得できる。
    /my/state/dir/file-a.sta のような文字列が返る。

  ファイル連番の取得 … filename_get_state_serial()
    現在設定されているステートファイル名の連番が取得できる。
    /my/state/dir/file-Z.sta ならば、 'Z' が返る。
    /my/state/dir/file.sta ならば、   0 が返る。
    拡張子が .sta でないなら、        -1 が返る。

  ファイル名の設定 … filename_set_state(name)
    ステートファイル名を name に設定する。
    連番つきのファイル名でも、連番なしでもよい。
    なお、NULL を指定すると、初期値がセットされる。

  ファイル連番の設定 … filename_set_state_serial(num)
    連番を num に設定する。  ファイル名の拡張子が .sta でないなら付加する。
    num が 0 なら、連番無し。ファイル名の拡張子が .sta でないなら付加する。
    num が負 なら、連番無し。ファイル名の拡張子はそのままとする。
*/

const char *filename_get_state() { return file_state; }

int filename_get_state_serial() {
  const char *str_sfx = STATE_SUFFIX;          /* ".sta" */
  const size_t len_sfx = strlen(STATE_SUFFIX); /* 4      */
  size_t len = strlen(file_state);

  if (len > len_sfx && my_strcmp(&file_state[len - len_sfx], str_sfx) == 0) {

    if (len > len_sfx + 2 && /* ファイル名が xxx-N.sta */
        '-' == file_state[len - len_sfx - 2] && isalnum(file_state[len - len_sfx - 1])) {
      /* '0'-'9','a'-'z' を返す */
      return file_state[len - len_sfx - 1];

    } else { /* ファイル名が xxx.sta */
      return 0;
    }
  } else { /* ファイル名が その他 */
    return -1;
  }
}

void filename_set_state(const char *filename) {
  if (filename) {
    strncpy(file_state, filename, QUASI88_MAX_FILENAME - 1);
    file_state[QUASI88_MAX_FILENAME - 1] = '\0';
  } else {
    filename_init_state(false);
  }
}

void filename_set_state_serial(int serial) {
  const char *str_sfx = STATE_SUFFIX;          /* ".sta"   */
  const size_t len_sfx = strlen(STATE_SUFFIX); /* 4        */
  char add_sfx[] = "-N" STATE_SUFFIX;          /* "-N.sta" */
  size_t len;
  int now_serial;

  add_sfx[1] = serial;

  len = strlen(file_state);

  now_serial = filename_get_state_serial();

  if (now_serial > 0) { /* 元のファイル名が xxx-N.sta */

    file_state[len - len_sfx - 2] = '\0'; /* -N.sta を削除 */

    if (serial <= 0) { /* xxx → xxx.sta */
      strcat(file_state, str_sfx);
    } else { /* xxx → xxx-M.sta */
      strcat(file_state, add_sfx);
    }

  } else if (now_serial == 0) { /* 元のファイル名が xxx.sta */

    if (serial <= 0) { /* xxx.sta のまま */
      ;
    } else {
      if (len + 2 < QUASI88_MAX_FILENAME) {
        file_state[len - len_sfx] = '\0'; /* .sta を削除 */
        strcat(file_state, add_sfx);      /* xxx → xxx-M.sta */
      }
    }

  } else { /* 元のファイル名が その他 xxx */

    if (serial < 0) { /* xxx のまま */
      ;
    } else if (serial == 0) { /* xxx → xxx.sta */
      if (len + len_sfx < QUASI88_MAX_FILENAME) {
        strcat(file_state, str_sfx);
      }
    } else { /* xxx → xxx-M.sta */
      if (len + len_sfx + 2 < QUASI88_MAX_FILENAME) {
        strcat(file_state, add_sfx);
      }
    }
  }
}

int statesave_check_file_exist() {
  OSD_FILE *fp;

  if (file_state[0] && (fp = osd_fopen(FTYPE_STATE_LOAD, file_state, "rb"))) {
    osd_fclose(fp);
    return true;
  }
  return false;
}

int statesave() {
  int success = false;

  if (file_state[0] == '\0') {
    QLOG_WARN("suspend", "state-file name not defined");
    return false;
  }

  QLOG_DEBUG("suspend", "statesave: {}", file_state);

  if ((statesave_fp = osd_fopen(FTYPE_STATE_SAVE, file_state, "wb"))) {
    if (statesave_header() == STATE_OK) {
      do {
        if (!statesave_emu())
          break;
        if (!statesave_memory())
          break;
        if (!statesave_pc88main())
          break;
        if (!statesave_crtcdmac())
          break;
        if (!statesave_sound())
          break;
        if (!statesave_pio())
          break;
        if (!statesave_screen())
          break;
        if (!statesave_intr())
          break;
        if (!statesave_keyboard())
          break;
        if (!statesave_pc88sub())
          break;
        if (!statesave_fdc())
          break;
        if (!statesave_system())
          break;

        success = true;
      } while (false);
    }

    osd_fclose(statesave_fp);
  }

  return success;
}

int stateload_check_file_exist() {
  int success = false;

  if (file_state[0] && (stateload_fp = osd_fopen(FTYPE_STATE_LOAD, file_state, "rb"))) {
    if (stateload_header() == STATE_OK) { /* ヘッダだけチェック */
      success = true;
    }
    osd_fclose(stateload_fp);
  }

  QLOG_DEBUG("suspend", "Stateload: file check ... {}", (success) ? "OK" : "FAILED");
  return success;
}

int stateload() {
  int success = false;

  if (file_state[0] == '\0') {
    printf("state-file name not defined\n");
    return false;
  }

  QLOG_DEBUG("suspend", "Stateload: {}", file_state);

  if ((stateload_fp = osd_fopen(FTYPE_STATE_LOAD, file_state, "rb"))) {

    if (stateload_header() == STATE_OK) {
      do {
        if (!stateload_emu())
          break;
        if (!stateload_sound())
          break;
        if (!stateload_memory())
          break;
        if (!stateload_pc88main())
          break;
        if (!stateload_crtcdmac())
          break;
        /*if( stateload_sound()    == false ) break; memoryの前に！ */
        if (!stateload_pio())
          break;
        if (!stateload_screen())
          break;
        if (!stateload_intr())
          break;
        if (!stateload_keyboard())
          break;
        if (!stateload_pc88sub())
          break;
        if (!stateload_fdc())
          break;
        if (!stateload_system())
          break;

        success = true;
      } while (false);
    }

    osd_fclose(stateload_fp);
  }

  return success;
}

/***********************************************************************
 * ステートファイル名を初期化
 ************************************************************************/
void stateload_init() {
  if (file_state[0] == '\0') {
    filename_init_state(false);
  }

  /* 起動時のオプションでステートロードが指示されている場合、
     なんらかのファイル名がすでにセットされているはず */
}
