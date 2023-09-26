#ifndef FNAME_H_INCLUDED
#define FNAME_H_INCLUDED

enum {
  FILE_CODING_EUC,
  FILE_CODING_SJIS,
  FILE_CODING_UTF8,
};

extern int file_coding; /* Kanji code for file name 0 - EUC,  1 - SJIS, 2 - UTF8 */

int osd_kanji_code();

void imagefile_all_open(int stateload);
void imagefile_all_close();

const char *filename_get_disk(int drv);
const char *filename_get_tape(int mode);
const char *filename_get_prn();
const char *filename_get_sin();
const char *filename_get_sout();
const char *filename_get_disk_or_dir(int drv);
const char *filename_get_tape_or_dir(int mode);
const char *filename_get_disk_name(int drv);
const char *filename_get_tape_name(int mode);

char *filename_alloc_diskname(const char *filename);
char *filename_alloc_romname(const char *filename);
char *filename_alloc_global_cfgname();
char *filename_alloc_local_cfgname(const char *imagename);
char *filename_alloc_keyboard_cfgname();

#endif // FNAME_H_INCLUDED
