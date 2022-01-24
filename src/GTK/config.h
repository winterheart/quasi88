#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/* GTK版 QUASI88 のための識別用 */

#ifndef QUASI88_GTK
#define QUASI88_GTK
#endif

/* エンディアンネスをチェック */

#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
#undef LSB_FIRST
#define LSB_FIRST
#else
#undef LSB_FIRST
#endif

/* メニューのタイトル／バージョン表示にて追加で表示する言葉 (任意の文字列) */

#define Q_COMMENT "GTK port"

/* 画面の bpp の定義。最低でもどれか一つは定義しなくてはならない */

#ifndef SUPPORT_8BPP
#define SUPPORT_8BPP
#endif
#ifndef SUPPORT_16BPP
#define SUPPORT_16BPP
#endif
#undef SUPPORT_32BPP

#endif /* CONFIG_H_INCLUDED */
