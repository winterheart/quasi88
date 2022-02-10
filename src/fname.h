#ifndef FNAME_H_INCLUDED
#define FNAME_H_INCLUDED

extern int file_coding; /* ファイル名の漢字コード 0:EUC/1:SJIS/2:UTF8*/

int osd_kanji_code();

void imagefile_all_open(int stateload);
void imagefile_all_close(void);

#endif // FNAME_H_INCLUDED
