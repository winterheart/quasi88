/*****************************************************************************/
/* ファイル操作に関する処理                          */
/*                                       */
/*  仕様の詳細は、ヘッダファイル file-op.h 参照              */
/*                                       */
/*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "quasi88.h"
#include "file-op.h"
#include "menu.h"

/*****************************************************************************/

struct T_DIR_INFO_STRUCT {
  int cur_entry;      /* 上位が取得したエントリ数 */
  int nr_entry;       /* エントリの全数        */
  T_DIR_ENTRY *entry; /* エントリ情報 (entry[0]〜) */
};

struct OSD_FILE_STRUCT {
  FILE *fp;     /* In use if !=NULL */
  int type;     /* File type */
  char *path;   /* File path */
  char mode[4]; /* Mode on open */
};

#define MAX_STREAM 8
static OSD_FILE osd_stream[MAX_STREAM];

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

OSD_FILE *osd_fopen(int type, const char *path, const char *mode) {
  OSD_FILE *st = nullptr;

  /* Find a free buffer */
  for (auto & i : osd_stream) {
    if (i.fp == nullptr) { /* fp が NULL なら空き */
      st = &i;
      break;
    }
  }
  if (st == nullptr)
    return nullptr; /* 空きがなければ NG */
  st->path = nullptr;

  std::filesystem::path fullpath = std::filesystem::absolute(path);
  if (!exists(fullpath)) {
    return nullptr;
  }

  switch (type) {

  case FTYPE_DISK:      /* "r+b" , "rb" */
  case FTYPE_TAPE_LOAD: /* "rb"     */
  case FTYPE_TAPE_SAVE: /* "ab"     */
  case FTYPE_PRN:       /* "ab"     */
  case FTYPE_COM_LOAD:  /* "rb"     */
  case FTYPE_COM_SAVE:  /* "ab"     */

    /* すでに開いているファイルかどうかをチェックする */
    for (auto & i : osd_stream) {
      if (i.fp) {
        if (fullpath == std::filesystem::path{i.path}) {
          /* Check if the file is already open. NB: Is this code ever needed? */
          if (type == FTYPE_DISK && i.type == type && strcmp(i.mode, mode) == 0) {
            return &i;
          } else {
            /* DISK以外、ないしモードが違うならばNG */
            return nullptr;
          }
        }
      }
    }

    /* FALLTHROUGH */

  default:
    st->path = (char *)malloc(strlen(fullpath.string().c_str()) + 1);
    if (st->path != nullptr) {
      strcpy(st->path, fullpath.string().c_str());
    }
    st->fp = fopen(st->path, mode); /* ファイルを開く */

    if (st->fp) {
      st->type = type;
      strncpy(st->mode, mode, sizeof(st->mode));
      return st;
    } else {
      free(st->path);
      return nullptr;
    }
  }
}

int osd_fclose(OSD_FILE *stream) {
  FILE *fp = stream->fp;
  stream->fp = nullptr;
  if (stream->path) {
    free(stream->path);
    stream->path = nullptr;
  }
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

/*
 * ディレクトリ内のファイル名のソーティングに使う関数
 */
static int namecmp(const void *p1, const void *p2) {
  auto *s1 = (T_DIR_ENTRY *)p1;
  auto *s2 = (T_DIR_ENTRY *)p2;

  // Directories first
  if (s1->type == s2->type) {
    return strcmp(s1->name, s2->name);
  }
  return (s1->type > s2->type);
}

/*---------------------------------------------------------------------------
 * T_DIR_INFO *osd_opendir(const char *filename)
 *  opendir()、rewinddir()、readdir()、closedir() を駆使し、
 *  ディレクトリの全てのエントリのファイル名をワークにセットする。
 *  ワークは malloc で確保するが、失敗時はそこでエントリの取得を打ち切る。
 *  処理後は、このワークをファイル名でソートしておく。
 *---------------------------------------------------------------------------*/
T_DIR_INFO *osd_opendir(const char *filename) {
  T_DIR_INFO *dir;

  /* T_DIR_INFO ワークを 1個確保 */
  if ((dir = (T_DIR_INFO *)malloc(sizeof(T_DIR_INFO))) == nullptr) {
    return nullptr;
  }

  if (filename == nullptr || filename[0] == '\0') {
    filename = ".";
  }

  std::filesystem::path root{filename};
  if (!exists(root) || !is_directory(root)) {
    free(dir);
    return nullptr;
  }

  dir->nr_entry = std::distance(std::filesystem::directory_iterator(root),
                                std::filesystem::directory_iterator{}) + 1;

  /* T_DIR_ENTRY ワークを ファイル数分 確保 */
  dir->entry = (T_DIR_ENTRY *)malloc(dir->nr_entry * sizeof(T_DIR_ENTRY));
  if (dir->entry == nullptr) {
    free(dir);
    return nullptr;
  }

  int i = 0;

  for (const auto& it : std::filesystem::directory_iterator(root)) {
    dir->entry[i].name = nullptr;
    dir->entry[i].str = nullptr;
    size_t size_name, size_str;
    std::filesystem::path basename = it.path().filename();
    size_name = strlen(basename.string().c_str()) + 1;
    if (it.is_directory()) {
      dir->entry[i].type = FILE_STAT_DIR;
      size_str = size_name + 1;
    } else {
      dir->entry[i].type = FILE_STAT_FILE;
      size_str = size_name;
    }
    dir->entry[i].name = (char *)malloc(size_name);
    dir->entry[i].str = (char *)malloc(size_str);
    strcpy(dir->entry[i].name, basename.string().c_str());
    strcpy(dir->entry[i].str, basename.string().c_str());
    if (it.is_directory()) {
      // append "/" to directory
      strcat(dir->entry[i].str, "/");
    }
    i++;
  }
  // Add ".."
  dir->entry[i].name = nullptr;
  dir->entry[i].str = nullptr;
  dir->entry[i].type = FILE_STAT_DIR;
  dir->entry[i].name = (char *)malloc(3);
  dir->entry[i].str = (char *)malloc(4);
  strcpy(dir->entry[i].name, "..");
  strcpy(dir->entry[i].str, "../");

  qsort(dir->entry, dir->nr_entry, sizeof(T_DIR_ENTRY), namecmp);

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
    if (dirp->entry[i].str) {
      free(dirp->entry[i].str);
    }
  }
  free(dirp->entry);
  free(dirp);
}

/* Manipulating path names */

/**
 * Normalize path, i.e. remove extra separators, resolve '..' and '.'. Path must be exists.
 * @note resolved_path should be pre-allocated with size bytes
 * @param path input path
 * @param resolved_path result path
 * @param size size of resolved_path
 * @return false on failure, true on success
 */
int osd_path_normalize(const char *path, char resolved_path[], int size) {
  std::filesystem::path input{canonical(std::filesystem::path(path))};

  if (strlen(input.string().c_str()) >= size) {
    return false;
  }
  strcpy(resolved_path, canonical(input).string().c_str());

  // printf("NORM: \"%s\" => \"%s\"\n", path, resolved_path);
  return true;
}

/**
 * Split path into dirname dir and basename name, i.e. "/tmp/file.txt"
 * produces "/tmp" and "file.txt".
 * @note dir and file should be pre-allocated with size bytes
 * @param path input path
 * @param dir dirname of path
 * @param file basename of path
 * @param size size of dir and file entries
 * @return false on failure, true on success
 */
int osd_path_split(const char *path, char dir[], char file[], int size) {
  std::filesystem::path fullpath{path};
  std::filesystem::path dirname = fullpath.parent_path();
  std::filesystem::path basename = fullpath.filename();

  if (strlen(dirname.string().c_str()) < size) {
    strcpy(dir, dirname.string().c_str());
  } else {
    return false;
  }
  if (strlen(basename.string().c_str()) < size) {
    strcpy(file, basename.string().c_str());
  } else {
    return false;
  }

  // printf("SPLT: \"%s\" = \"%s\" + \"%s\")\n", path, dir, file);
  return true;
}

/**
 * Joins dir and file paths into one, normalize it and returns result as path.
 * @note path should be pre-allocated with size bytes.
 * @param dir directory path
 * @param file filename
 * @param path result path
 * @param size expected size of path
 * @return false on failure, true on success
 */
int osd_path_join(const char *dir, const char *file, char path[], int size) {
  std::filesystem::path result{weakly_canonical(std::filesystem::path(dir) / std::filesystem::path(file))};
  if (result.string().size() > size) {
    return false;
  }
  strcpy(path, result.string().c_str());

  // printf("JOIN: \"%s\" + \"%s\" = \"%s\"\n", dir, file, path);
  return true;
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
