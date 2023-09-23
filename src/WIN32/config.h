#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/*----------------------------------------------------------------------*/
/* WIN32 固有の定義                            */
/*----------------------------------------------------------------------*/

#define NOMINMAX
#include <windows.h>

/* WIN32版 QUASI88 のための識別用 */

#ifndef QUASI88_WIN32
#define QUASI88_WIN32
#endif

/* エンディアンネス */

#ifndef LSB_FIRST
#define LSB_FIRST
#endif

/* メニューのタイトル／バージョン表示にて追加で表示する言葉 (任意の文字列) */

#if USE_RETROACHIEVEMENTS
#define RAQ_COMMENT BASE_TITLE " ver " BASE_VERSION
#define Q_COMMENT RAQ_COMMENT
#else
#define Q_COMMENT "WIN32 port"
#endif

/* WIN32版は 32bpp(bit per pixel) 固定とする */

#undef SUPPORT_8BPP
#undef SUPPORT_16BPP
#ifndef SUPPORT_32BPP
#define SUPPORT_32BPP
#endif

/* VC のインラインキーワード */
#define INLINE __inline

/* サウンドドライバ用に、PI(π)とM_PI(π)を定義 */
#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef M_PI
#define M_PI PI
#endif

#endif /* CONFIG_H_INCLUDED */
