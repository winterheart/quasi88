#ifndef FNAME_H_INCLUDED
#define FNAME_H_INCLUDED

extern int file_coding; /* ファイル名の漢字コード 0:EUC/1:SJIS/2:UTF8*/

int osd_kanji_code();

void imagefile_all_open(int stateload);
void imagefile_all_close(void);

const char  *filename_get_disk(int drv);
const char  *filename_get_tape(int mode);
const char  *filename_get_prn(void);
const char  *filename_get_sin(void);
const char  *filename_get_sout(void);
const char  *filename_get_disk_or_dir(int drv);
const char  *filename_get_tape_or_dir(int mode);
const char  *filename_get_disk_name(int drv);
const char  *filename_get_tape_name(int mode);

char    *filename_alloc_diskname(const char *filename);
char    *filename_alloc_romname(const char *filename);
char    *filename_alloc_global_cfgname(void);
char    *filename_alloc_local_cfgname(const char *imagename);
char    *filename_alloc_keyboard_cfgname(void);

#endif // FNAME_H_INCLUDED
