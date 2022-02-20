/*****************************************************************************/
/* ファイル操作に関する処理                          */
/*                                       */
/*  仕様の詳細は、ヘッダファイル file-op.h 参照              */
/*                                       */
/*****************************************************************************/

#include <filesystem>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "quasi88.h"
#include "file-op.h"
#include "menu.h"

/*****************************************************************************/

/*
 * Following dir names are pre-allocated char arrays with OSD_MAX_FILENAME
 * length. Don't forget to malloc() and free() them.
 */
static char *dir_cwd;   // Current directory
static char *dir_rom;   // Search directory for ROM files
static char *dir_disk;  // Search directory for DISK image files
static char *dir_tape;  // Search directory for TAPE image files
static char *dir_snap;  // Where to save snapshots
static char *dir_state; // Where to save states of machine
static char *dir_save;  // Where to save saves
static char *dir_home;  // Common directory of configuration
static char *dir_ini;   // Directory for local configuration

/* Gets directory pathnames (osd_dir_cwd should never returns NULL!) */
const char *osd_dir_cwd() { return dir_cwd; }
const char *osd_dir_rom() { return dir_rom; }
const char *osd_dir_disk() { return dir_disk; }
const char *osd_dir_tape() { return dir_tape; }
const char *osd_dir_save() { return dir_save; }
const char *osd_dir_snap() { return dir_snap; }
const char *osd_dir_state() { return dir_state; }
const char *osd_dir_gcfg() { return dir_home; }
const char *osd_dir_lcfg() { return dir_ini; }

static int set_new_dir(const char *newdir, char *dir) {
  if (strlen(newdir) < OSD_MAX_FILENAME) {
    strcpy(dir, newdir);
    return TRUE;
  }
  return FALSE;
}

int osd_set_dir_cwd(const char *d) { return set_new_dir(d, dir_cwd); }
int osd_set_dir_rom(const char *d) { return set_new_dir(d, dir_rom); }
int osd_set_dir_disk(const char *d) { return set_new_dir(d, dir_disk); }
int osd_set_dir_tape(const char *d) { return set_new_dir(d, dir_tape); }
int osd_set_dir_save(const char *d) { return set_new_dir(d, dir_save); }
int osd_set_dir_snap(const char *d) { return set_new_dir(d, dir_snap); }
int osd_set_dir_state(const char *d) { return set_new_dir(d, dir_state); }
int osd_set_dir_gcfg(const char *d) { return set_new_dir(d, dir_home); }
int osd_set_dir_lcfg(const char *d) { return set_new_dir(d, dir_ini); }

/****************************************************************************
 * ファイル名に使用されている漢字コードを取得
 *      0 … ASCII のみ
 *      1 … 日本語EUC
 *      2 … シフトJIS
 *      3 … UTF-8
 *****************************************************************************/
int osd_kanji_code() {
  if (file_coding == 2)
    return 3;
  else if (file_coding == 1)
    return 2;
  else
    return 1;
}

/****************************************************************************
 * ファイル操作
 *
 * OSD_FILE *osd_fopen(int type, const char *path, const char *mode)
 * int  osd_fclose(OSD_FILE *stream)
 * int  osd_fflush(OSD_FILE *stream)
 * int  osd_fseek(OSD_FILE *stream, long offset, int whence)
 * long osd_ftell(OSD_FILE *stream)
 * void osd_rewind(OSD_FILE *stream)
 * size_t osd_fread(void *ptr, size_t size, size_t nobj, OSD_FILE *stream)
 * size_t osd_fwrite(const void *ptr,size_t size,size_t nobj,OSD_FILE *stream)
 * int  osd_fputc(int c, OSD_FILE *stream)
 * int  osd_fgetc(OSD_FILE *stream)
 * char *osd_fgets(char *str, int size, OSD_FILE *stream)
 * int  osd_fputs(const char *str, OSD_FILE *stream)
 *****************************************************************************/

/*
 * 全てのファイルに対して排他制御したほうがいいと思うけど、面倒なので、
 * ディスク・テープのイメージに関してのみ、多重にオープンしないようにする。
 *
 * osd_fopen が呼び出されたときに、ファイルの情報を stat にて取得し、
 * すでに開いているファイルの stat と一致しないかをチェックする。
 * ここで、ディスクイメージファイルの場合は、すでに開いているファイルの
 * ファイルポインタを返し、他の場合はオープン失敗として NULL を返す。
 */

struct OSD_FILE_STRUCT {

  FILE *fp;       /* !=NULL なら使用中   */
  struct stat sb; /* 開いたファイルの状態   */
  int type;       /* ファイル種別       */
  char mode[4];   /* 開いた際の、モード  */
};

#define MAX_STREAM 8
static OSD_FILE osd_stream[MAX_STREAM];

OSD_FILE *osd_fopen(int type, const char *path, const char *mode) {
  int i;
  struct stat sb;
  OSD_FILE *st;
  int stat_ok;

  st = nullptr;
  for (i = 0; i < MAX_STREAM; i++) {   /* 空きバッファを探す */
    if (osd_stream[i].fp == nullptr) { /* fp が NULL なら空き */
      st = &osd_stream[i];
      break;
    }
  }
  if (st == nullptr)
    return nullptr; /* 空きがなければ NG */

  if (stat(path, &sb) != 0) { /* ファイルの状態を取得する */
    if (mode[0] == 'r')
      return nullptr;
    stat_ok = FALSE;
  } else {
    stat_ok = TRUE;
  }

  switch (type) {

  case FTYPE_DISK:      /* "r+b" , "rb" */
  case FTYPE_TAPE_LOAD: /* "rb"     */
  case FTYPE_TAPE_SAVE: /* "ab"     */
  case FTYPE_PRN:       /* "ab"     */
  case FTYPE_COM_LOAD:  /* "rb"     */
  case FTYPE_COM_SAVE:  /* "ab"     */

    if (stat_ok) {
      /* すでに開いているファイルかどうかをチェックする */
      for (i = 0; i < MAX_STREAM; i++) {
        if (osd_stream[i].fp) {
          if (osd_stream[i].sb.st_dev == sb.st_dev && osd_stream[i].sb.st_ino == sb.st_ino) {

            /* DISKの場合かつ同じモードならばそれを返す */
            if (type == FTYPE_DISK && osd_stream[i].type == type && strcmp(osd_stream[i].mode, mode) == 0) {

              return &osd_stream[i];

            } else {
              /* DISK以外、ないしモードが違うならばNG */
              return nullptr;
            }
          }
        }
      }
    }
    /* FALLTHROUGH */

  default:
    st->fp = fopen(path, mode); /* ファイルを開く */

    if (st->fp) {

      if (stat_ok == FALSE) {       /* もう一度ファイル状態を */
        fflush(st->fp);             /* 取得してみよう。       */
        if (stat(path, &sb) != 0) { /* 必ず成功するはず……？ */
          sb.st_dev = 0;
          sb.st_ino = 0;
        }
      }

      st->type = type;
      st->sb = sb;
      strncpy(st->mode, mode, sizeof(st->mode));
      return st;

    } else {

      return nullptr;
    }
  }
}

int osd_fclose(OSD_FILE *stream) {
  FILE *fp = stream->fp;

  stream->fp = nullptr;
  return fclose(fp);
}

int osd_fflush(OSD_FILE *stream) {
  if (stream == nullptr)
    return fflush(nullptr);
  else
    return fflush(stream->fp);
}

int osd_fseek(OSD_FILE *stream, long offset, int whence) { return fseek(stream->fp, offset, whence); }

long osd_ftell(OSD_FILE *stream) { return ftell(stream->fp); }

void osd_rewind(OSD_FILE *stream) {
  (void)osd_fseek(stream, 0L, SEEK_SET);
  osd_fflush(stream);
}

size_t osd_fread(void *ptr, size_t size, size_t nobj, OSD_FILE *stream) { return fread(ptr, size, nobj, stream->fp); }

size_t osd_fwrite(const void *ptr, size_t size, size_t nobj, OSD_FILE *stream) {
  return fwrite(ptr, size, nobj, stream->fp);
}

int osd_fputc(int c, OSD_FILE *stream) { return fputc(c, stream->fp); }

int osd_fgetc(OSD_FILE *stream) { return fgetc(stream->fp); }

char *osd_fgets(char *str, int size, OSD_FILE *stream) { return fgets(str, size, stream->fp); }

int osd_fputs(const char *str, OSD_FILE *stream) { return fputs(str, stream->fp); }

/****************************************************************************
 * ディレクトリ閲覧
 *****************************************************************************/

struct T_DIR_INFO_STRUCT {
  int cur_entry;      /* 上位が取得したエントリ数 */
  int nr_entry;       /* エントリの全数        */
  T_DIR_ENTRY *entry; /* エントリ情報 (entry[0]〜) */
};

/*
 * ディレクトリ内のファイル名のソーティングに使う関数
 */
static int namecmp(const void *p1, const void *p2) {
  auto *s1 = (T_DIR_ENTRY *)p1;
  auto *s2 = (T_DIR_ENTRY *)p2;

  return strcmp(s1->name, s2->name);
}

/*---------------------------------------------------------------------------
 * T_DIR_INFO *osd_opendir(const char *filename)
 *  opendir()、rewinddir()、readdir()、closedir() を駆使し、
 *  ディレクトリの全てのエントリのファイル名をワークにセットする。
 *  ワークは malloc で確保するが、失敗時はそこでエントリの取得を打ち切る。
 *  処理後は、このワークをファイル名でソートしておく。
 *---------------------------------------------------------------------------*/
T_DIR_INFO *osd_opendir(const char *filename) {
  size_t len;
  char *p;
  int i;
  T_DIR_INFO *dir;

  DIR *dirp;
  struct dirent *dp;

  /* T_DIR_INFO ワークを 1個確保 */
  if ((dir = (T_DIR_INFO *)malloc(sizeof(T_DIR_INFO))) == nullptr) {
    return nullptr;
  }

  if (filename == nullptr || filename[0] == '\0') {
    filename = ".";
  }

  dirp = opendir(filename); /* ディレクトリを開く */
  if (dirp == nullptr) {
    free(dir);
    return nullptr;
  }

  dir->nr_entry = 0; /* ファイル数を数える */
  while (readdir(dirp)) {
    dir->nr_entry++;
  }
  rewinddir(dirp);

  /* T_DIR_ENTRY ワークを ファイル数分 確保 */
  dir->entry = (T_DIR_ENTRY *)malloc(dir->nr_entry * sizeof(T_DIR_ENTRY));
  if (dir->entry == nullptr) {
    closedir(dirp);
    free(dir);
    return nullptr;
  }
  for (i = 0; i < dir->nr_entry; i++) {
    dir->entry[i].name = nullptr;
    dir->entry[i].str = nullptr;
  }

  /* ファイル数分、処理ループ (情報を格納) */
  for (i = 0; i < dir->nr_entry; i++) {

    dp = readdir(dirp); /* ファイル名取得 */

    if (dp == nullptr) { /* 取得に失敗したら、中断  */
      dir->nr_entry = i; /* (これは正常扱いとする。 */
      break;             /*  おそらく途中でファイル */
    }                    /*  が削除されたのだろう)  */

    /* ファイルの種類をセット */
    {
      char *fullname; /* ディレクトリ名(filename) */
      struct stat sb; /* とファイル名(dp->d_name) */
                      /* からstat関数で属性取得。 */
                      /* 失敗しても気にしない     */

      dir->entry[i].type = FILE_STAT_FILE; /*  (失敗したら FILE 扱い) */

      fullname = (char *)malloc(strlen(filename) + 1 + strlen(dp->d_name) + 1);

      if (fullname) {
        sprintf(fullname, "%s%s%s", filename, "/", dp->d_name);

        if (stat(fullname, &sb) == 0) {
#if 1
          if (S_ISDIR(sb.st_mode))
#else
          if ((sb.st_mode & S_IFMT) == S_IFDIR)
#endif
          {
            dir->entry[i].type = FILE_STAT_DIR;
          } else {
            dir->entry[i].type = FILE_STAT_FILE;
          }
        }
        free(fullname);
      }
    }

    /* ファイル名バッファ確保 */

    len = strlen(dp->d_name) + 1;
    p = (char *)malloc((len + 1) + (len + 1));
    if (p == nullptr) { /* ↑ファイル名 と ↑表示名 のバッファを一気に確保 */
      dir->nr_entry = i;
      break; /* malloc に失敗したら中断 */
    }

    /* ファイル名・表示名セット */
    dir->entry[i].name = &p[0];
    dir->entry[i].str = &p[len + 1];

    strcpy(dir->entry[i].name, dp->d_name);
    strcpy(dir->entry[i].str, dp->d_name);

    if (dir->entry[i].type == FILE_STAT_DIR) { /* ディレクトリの場合、 */
      strcat(dir->entry[i].str, "/");          /* 表示名に / を付加    */
    }
  }

  closedir(dirp); /* ディレクトリを閉じる */

  /* ファイル名をソート */
  qsort(dir->entry, dir->nr_entry, sizeof(T_DIR_ENTRY), namecmp);

#if 0 /* この処理は無し。表示名 abc/ でも ファイル名は abc のまま */
    /* ディレクトリの場合、ファイル名に / を付加 (ソート後に行う) */
    for (i=0; i<dir->nr_entry; i++) {
    if (dir->entry[i].type == FILE_STAT_DIR) {
        strcat(dir->entry[i].name, "/");
    }
    }
#endif

  /* osd_readdir に備えて */
  dir->cur_entry = 0;
  return dir;
}

/*---------------------------------------------------------------------------
 * T_DIR_ENTRY *osd_readdir(T_DIR_INFO *dirp)
 *  osd_opendir() の時に確保した、エントリ情報ワークへのポインタを
 *  順次、返していく。
 *---------------------------------------------------------------------------*/
T_DIR_ENTRY *osd_readdir(T_DIR_INFO *dirp) {
  T_DIR_ENTRY *ret_value = nullptr;

  if (dirp->cur_entry != dirp->nr_entry) {
    ret_value = &dirp->entry[dirp->cur_entry];
    dirp->cur_entry++;
  }
  return ret_value;
}

/*---------------------------------------------------------------------------
 * void osd_closedir(T_DIR_INFO *dirp)
 *  osd_opendir() 時に確保した全てのメモリを開放する。
 *---------------------------------------------------------------------------*/
void osd_closedir(T_DIR_INFO *dirp) {
  int i;

  for (i = 0; i < dirp->nr_entry; i++) {
    if (dirp->entry[i].name) {
      free(dirp->entry[i].name);
    }
  }
  free(dirp->entry);
  free(dirp);
}

/****************************************************************************
 * パス名の操作
 *****************************************************************************/

/*---------------------------------------------------------------------------
 * int  osd_path_normalize(const char *path, char resolved_path[], int size)
 *
 *  処理内容:
 *      ./ は削除、 ../ は親ディレクトリに置き換え、 /…/ は / に置換。
 *      面倒なので、リンクやカレントディレクトリは展開しない。
 *      末尾に / が残った場合、それは削除する。
 *  例:
 *      "../dir1/./dir2///dir3/../../file" → "../dir1/file"
 *---------------------------------------------------------------------------*/
int osd_path_normalize(const char *path, char resolved_path[], int size) {
  char *buf, *s, *d, *p;
  int is_abs, is_dir, success = FALSE;
  size_t len = strlen(path);

  if (len == 0) {
    if (size) {
      resolved_path[0] = '\0';
      success = TRUE;
    }
  } else {

    is_abs = (path[0] == '/') ? TRUE : FALSE;
    is_dir = (path[len - 1] == '/') ? TRUE : FALSE;

    buf = (char *)malloc((len + 3) * 2); /* path と同サイズ位の   */
    if (buf) {                           /* バッファを2個分 確保    */
      strcpy(buf, path);
      d = &buf[len + 3];
      d[0] = '\0';

      s = strtok(buf, "/"); /* / で 区切っていく */

      if (s == nullptr) { /* 区切れないなら、 */
                          /* それは / そのものだ  */
        if (size > 1) {
          strcpy(resolved_path, "/");
          success = TRUE;
        }

      } else { /* 区切れたなら、分析  */

        for (; s; s = strtok(nullptr, "/")) {

          if (strcmp(s, ".") == 0) { /* . は無視  */
            ;

          } else if (strcmp(s, "..") == 0) { /* .. は直前を削除 */

            p = strrchr(d, '/'); /* 直前の/を探す */

            if (p && strcmp(p, "/..") != 0) { /* 見つかれば    */
              *p = '\0';                      /*    そこで分断 */
            } else {                          /* 見つからない  */
              if (p == nullptr && is_abs) {   /*   絶対パスなら*/
                ;                             /*     無視する  */
              } else {                        /*   相対パスなら*/
                strcat(d, "/..");             /*     .. にする */
              }
            }

          } else {          /* 上記以外は連結 */
            strcat(d, "/"); /* 常に / を前置 */
            strcat(d, s);
          }
        }

        if (d[0] == '\0') { /* 結果が空文字列になったら */
          if (is_abs)
            strcpy(d, "/"); /*   元が絶対パスなら /     */
                            /* else         ;        *   元が相対パスから 空    */

        } else {
          if (is_abs == FALSE) { /* 元が相対パスなら */
            d++;                 /* 先頭の / を削除  */
          }
#if 0 /* この処理は無し。元が a/b/c/ でも a/b/c とする */
            if (is_dir) {       /* 元の末尾が / なら */
            strcat(d, "/");     /* 末尾に / を付加   */
            }
#endif
        }

        if (strlen(d) < (size_t)size) {
          strcpy(resolved_path, d);
          success = TRUE;
        }
      }

      free(buf);
    }
  }

  /*printf("NORM:\"%s\" => \"%s\"\n",path,resolved_path);*/
  return success;
}

/*---------------------------------------------------------------------------
 * int  osd_path_split(const char *path, char dir[], char file[], int size)
 *
 *  処理内容:
 *      path の最後の / より前を dir、後ろを file にセットする
 *          dir の末尾に / はつかない。
 *      path の末尾が / なら、予め削除してから処理する
 *          よって、 file の末尾にも / はつかない。
 *      path は予め、正規化されているものとする。
 *---------------------------------------------------------------------------*/
int osd_path_split(const char *path, char dir[], char file[], int size) {
  size_t pos = strlen(path);

  /* dir, file は十分なサイズを確保しているはずなので、軽くチェック */
  if (pos == 0 || size <= pos) {
    dir[0] = '\0';
    file[0] = '\0';
    strncat(file, path, size - 1);
    if (pos)
      fprintf(stderr, "internal overflow %d\n", __LINE__);
    return FALSE;
  }

  if (strcmp(path, "/") == 0) { /* "/" の場合、別処理    */
    strcpy(dir, "/");           /* ディレクトリは "/"    */
    strcpy(file, "");           /* ファイルは ""   */
    return TRUE;
  }

  if (path[pos - 1] == '/') { /* path 末尾の / は無視   */
    pos--;
  }

  do { /* / を末尾から探す  */
    if (path[pos - 1] == '/') {
      break;
    }
    pos--;
  } while (pos);

  if (pos) {                 /* / が見つかったら  */
    strncpy(dir, path, pos); /* 先頭〜 / までをコピー*/
    if (pos > 1)
      dir[pos - 1] = '\0'; /* 末尾の / は削除する  */
    else                   /* ただし        */
      dir[pos] = '\0';     /* "/"の場合は / は残す */

    strcpy(file, &path[pos]);

  } else {              /* / が見つからなかった    */
    strcpy(dir, "");    /* ディレクトリは "" */
    strcpy(file, path); /* ファイルは path全て   */
  }

  pos = strlen(file); /* ファイル末尾の / は削除 */
  if (pos && file[pos - 1] == '/') {
    file[pos - 1] = '\0';
  }

  /*printf("SPLT:\"%s\" = \"%s\" + \"%s\")\n",path,dir,file);*/
  return TRUE;
}

/*---------------------------------------------------------------------------
 * int  osd_path_join(const char *dir, const char *file, char path[], int size)
 *
 *  処理内容:
 *      file が / で始まっていたら、そのまま path にセット
 *      そうでなければ、"dir" + "/" + "file" を path にセット
 *      出来上がった path は正規化しておく
 *---------------------------------------------------------------------------*/
int osd_path_join(const char *dir, const char *file, char path[], int size) {
  size_t len;
  char *p;

  if (dir == nullptr || dir[0] == '\0' || /* ディレクトリ名なし or  */
      file[0] == '/') {                   /* ファイル名が、絶対パス */

    if ((size_t)size <= strlen(file)) {
      return FALSE;
    }
    strcpy(path, file);

  } else { /* ファイル名は、相対パス */

    path[0] = '\0';
    strncat(path, dir, size - 1);

    len = strlen(path);                   /* ディレクトリ末尾  */
    if (len && path[len - 1] != '/') {    /* が '/' でないなら */
      strncat(path, "/", size - len - 1); /* 付加する          */
    }

    len = strlen(path);
    strncat(path, file, size - len - 1);
  }

  p = (char *)malloc(size); /* 正規化しておこう */
  if (p) {
    strcpy(p, path);
    if (osd_path_normalize(p, path, size) == FALSE) {
      strcpy(path, p);
    }
    free(p);
  }

  /*printf("JOIN:\"%s\" + \"%s\" = \"%s\"\n",dir,file,path);*/
  return TRUE;
}

/**
 * Get attribute of file/directory
 * @param pathname pathname of file/directory
 * @return One of following values:
 * FILE_STAT_NOEXIST: file or directory not exists;
 * FILE_STAT_DIR: pathname is directory;
 * FILE_STAT_FILE: pathname is regular file.
 * @note FILE_STAT_FILE will return as default value, i.e. even if pathname is not a regular file.
 */
int osd_file_stat(const char *pathname) {
  std::filesystem::path path{pathname};

  if (!exists(path) || path.empty()) {
    return FILE_STAT_NOEXIST;
  }

  if (is_directory(path)) {
    return FILE_STAT_DIR;
  } else if (is_regular_file(path)) {
    return FILE_STAT_FILE;
  } else {
    // What I gonna do??
    fprintf(stderr, "Unknown type of pathname, returning FILE_STAT_FILE!\n");
    return FILE_STAT_FILE;
  }
}

static int make_dir(const char *dname);

/**
 * Sets pointer to *dir to directory defined by environment variable env_dir.
 * If env_dir undefined, cwd_dir / alt_dir will be used instead as default value.
 * @param dir directory that will be defined
 * @param env_dir environment variable that holds requested path (if defined)
 * @param alt_dir alternative path that will be used if environment var undefined or empty
 */
static void set_dir(char **dir, const char *env_dir, const char *alt_dir) {
  std::filesystem::path path;
  char *env_path = getenv(env_dir);
  if (env_path) {
    path = std::filesystem::path(env_path);
  }

  if (!path.empty()) {
    strcpy(*dir, path.string().c_str());
  } else {
    if (alt_dir) {
      path = std::filesystem::path(dir_home) / std::filesystem::path(alt_dir);
      strcpy(*dir, path.string().c_str());
    } else {
      // Failsafe
      path = std::filesystem::current_path();
      strcpy(*dir, path.string().c_str());
    }
    make_dir((const char *)*dir);
  }
}

int osd_file_config_init() {

  /* ワークを確保 (固定長で処理する予定なので静的確保でもいいんだけど) */

  dir_cwd = (char *)malloc(OSD_MAX_FILENAME);
  dir_rom = (char *)malloc(OSD_MAX_FILENAME);
  dir_disk = (char *)malloc(OSD_MAX_FILENAME);
  dir_tape = (char *)malloc(OSD_MAX_FILENAME);
  dir_snap = (char *)malloc(OSD_MAX_FILENAME);
  dir_state = (char *)malloc(OSD_MAX_FILENAME);
  dir_save = (char *)malloc(OSD_MAX_FILENAME);
  dir_home = (char *)malloc(OSD_MAX_FILENAME);
  dir_ini = (char *)malloc(OSD_MAX_FILENAME);

  if (!dir_cwd || !dir_rom || !dir_disk || !dir_tape || !dir_snap || !dir_state || !dir_home || !dir_ini)
    return false;

  /* カレントワーキングディレクトリ名 (CWD) を取得する */
  std::filesystem::path dir_path;
  std::error_code ec;
  dir_path = std::filesystem::current_path(ec);
  if (ec) {
    fprintf(stderr, "error: can't get CWD: %s\n", ec.message().c_str());
    strcpy(dir_cwd, ""); // Dunno, is this safe?
  } else {
    strcpy(dir_cwd, dir_path.string().c_str());
  }

  // Now check if current directory contains quasi88.ini file. If it is, make it as home.
  if (is_regular_file(dir_path / CONFIG_FILENAME CONFIG_SUFFIX)) {
    strcpy(dir_home, dir_path.string().c_str());
  } else {
    /* Retrieve home directory via getenv */
#if defined(QUASI88_FUNIX)
    char *env_home = getenv("HOME");
#elif defined(QUASI88_FWIN)
    char *env_home = getenv("USERPROFILE");
#else
    fprintf(stderr, "cannot detect homedir, trying getenv(\"HOME\") as default!\n");
    char *env_home = getenv("HOME");
#endif

    if (env_home) {
      dir_path = std::filesystem::path(env_home) / ".quasi88";
    } else {
      // Cannot get HOME, setting current as home
      fprintf(stderr, "error: can't get HOME\n");
      dir_path = std::filesystem::current_path();
    }
    strcpy(dir_home, dir_path.string().c_str());
  }
  make_dir((const char *)dir_home);

  // All set, now populate dir variables
  set_dir(&dir_ini, "QUASI88_INI_DIR", "ini");
  set_dir(&dir_rom, "QUASI88_ROM_DIR", "rom");
  set_dir(&dir_disk, "QUASI88_DISK_DIR", "disk");
  set_dir(&dir_tape, "QUASI88_TAPE_DIR", "tape");
  set_dir(&dir_snap, "QUASI88_SNAP_DIR", "snap");
  set_dir(&dir_state, "QUASI88_STATE_DIR", "state");
  set_dir(&dir_save, "QUASI88_SAVE_DIR", "save");

  if (!dir_ini || !dir_rom || !dir_disk || !dir_tape || !dir_snap || !dir_state || !dir_save)
    return false;

  return true;
}

/*
 *  ディレクトリ dname があるかチェック。無ければ作る。
 *      成功したら、真を返す
 */
static int make_dir(const char *dname) {
  std::filesystem::path path{dname};
  if (exists(path) && is_directory(path)) {
    // Already exists, nothing to do
    return true;
  }
  std::error_code ec;
  create_directories(path, ec);
  if (ec) {
    printf("error: can't make dir \"%s\": %s\n", dname, ec.message().c_str());
    return false;
  }
  printf("make dir \"%s\"\n", dname);
  return true;
}

/****************************************************************************
 * int  osd_file_config_exit(void)
 *
 *  この関数は、終了後に1度だけ呼び出される。
 *
 ****************************************************************************/
void osd_file_config_exit() {
  if (dir_cwd)
    free(dir_cwd);
  if (dir_rom)
    free(dir_rom);
  if (dir_disk)
    free(dir_disk);
  if (dir_tape)
    free(dir_tape);
  if (dir_snap)
    free(dir_snap);
  if (dir_state)
    free(dir_state);
  if (dir_save)
    free(dir_save);
  if (dir_home)
    free(dir_home);
  if (dir_ini)
    free(dir_ini);
}
