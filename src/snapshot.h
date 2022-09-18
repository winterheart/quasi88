#ifndef SNAPSHOT_H_INCLUDED
#define SNAPSHOT_H_INCLUDED

enum { SNAPSHOT_FMT_BMP, SNAPSHOT_FMT_PPM, SNAPSHOT_FMT_RAW };
extern int snapshot_format; /* スナップショットフォーマット   */

/* スナップショットコマンド */
#define SNAPSHOT_CMD_SIZE (1024)
extern char snapshot_cmd[SNAPSHOT_CMD_SIZE];
extern char snapshot_cmd_do; /* コマンド実行の有無      */

extern char snapshot_cmd_enable; /* コマンド実行の可否      */

void filename_init_snap(int synchronize);
void filename_set_snap_base(const char *filename);
const char *filename_get_snap_base();

void screen_snapshot_init();
void screen_snapshot_exit();

int screen_snapshot_save();

void filename_init_wav(int synchronize);
void filename_set_wav_base(const char *filename);
const char *filename_get_wav_base();

int waveout_save_start();
void waveout_save_stop();

#endif /* SNAPSHOT_H_INCLUDED */
