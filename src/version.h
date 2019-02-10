#ifndef VERSION_H
#define VERSION_H

#ifndef BASE_TITLE
#define BASE_TITLE  "QUASI88"
#endif

#ifndef BASE_VERSION
#define BASE_VERSION    "0.6.6"
#endif

#ifndef BASE_COMMENT
#define BASE_COMMENT    ""
#endif

#if USE_RETROACHIEVEMENTS

#ifndef RAQ_TITLE
#define RAQ_TITLE   "RAQUASI88"
#endif

#ifndef RAQ_VERSION
#define RAQ_VERSION     "1.1.1"
#endif

#ifndef Q_TITLE
#define Q_TITLE RAQ_TITLE
#endif

#ifndef Q_TITLE_KANJI
#define Q_TITLE_KANJI RAQ_TITLE
#endif

#ifndef Q_VERSION
#define Q_VERSION RAQ_VERSION
#endif

#else

#ifndef Q_TITLE
#define Q_TITLE BASE_TITLE
#endif

#ifndef Q_TITLE_KANJI
#define Q_TITLE_KANJI BASE_TITLE "改"
#endif

#ifndef Q_VERSION
#define Q_VERSION BASE_VERSION
#endif

#endif




#ifndef Q_COPYRIGHT
#define Q_COPYRIGHT     "(c) 1998-2019 S.Fukunaga, R.Zumer"
#endif

#ifdef  USE_SOUND
#ifndef Q_MAME_COPYRIGHT
#define Q_MAME_COPYRIGHT    "Copyright (c) 1997-2007, Nicola Salmoria and the MAME team"
#endif

#ifdef  USE_FMGEN
#ifndef Q_FMGEN_COPYRIGHT
#define Q_FMGEN_COPYRIGHT   "Copyright (C) by cisc 1998, 2003."
#endif
#endif
#endif

#endif  /* VERSION_H */
