#ifndef FDC_H_INCLUDED
#define FDC_H_INCLUDED

extern int fdc_debug_mode; /* FDC デバッグモードのフラグ        */
extern int disk_exchange;  /* ディスク疑似入れ替えフラグ      */
extern int disk_ex_drv;    /* ディスク疑似入れ替えドライブ       */

extern int FDC_flag; /* FDC 割り込み信号   */
extern int fdc_wait; /* FDC の ウエイト */

extern int fdc_ignore_readonly; /* 読込専用時、ライトを無視する   */

int fdc_ctrl(int interval);

void fdc_write(byte data);
byte fdc_read();
byte fdc_status();
void fdc_TC();

void pc88fdc_break_point();

#endif /* FDC_H_INCLUDED */
