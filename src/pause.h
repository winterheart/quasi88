#ifndef PAUSE_H_INCLUDED
#define PAUSE_H_INCLUDED

/************************************************************************/
/* 一時停止モード                            */
/************************************************************************/

void pause_init();
void pause_main();

/*----------------------------------------------------------------------
 * イベント処理の対処
 *----------------------------------------------------------------------*/
void pause_event_focus_out_when_exec();
void pause_event_focus_in_when_pause();
void pause_event_key_on_esc();
void pause_event_key_on_menu();

#endif /* PAUSE_H_INCLUDED */
