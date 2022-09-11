## QUASI88改 / QUASI88kai
---

![](../../workflows/build/badge.svg)

QUASI88kai is an improved version of Showzoh Fukunaga's [QUASI88](http://www.eonet.ne.jp/~showtime/quasi88/).
The aim is to fix bugs and implement new features.

Main changes:
* General updates to the build system and migration to CMake
* All files converted to UTF-8
* Many improvements in Windows, X11, GTK and SDL backends
* Add a RetroAchievements-compatible version "RAQUASI88" (only for Windows at this time)

Please refer to issues with the "enhancement" tag for future plans.

The latest version is 0.7.0 (released 2022-09-11).

### Compilation

Preferred way to compile QUASI88kai is using CMake buildsystem. Please consult [README.cmake.md](README.cmake.md) for
more info.

### Usage

The following items are required to run QUASI88kai.

- ROM image file(s)
- Disk or tape images

For more details, please refer to `document/QUASI88.TXT` and `document/MANUAL.TXT` (in Japanese).

### License

QUASI88kai (excluding the sound processing portion) is free software, and copyright remains with the associated authors.
This software is provided without any guarantee, and the authors do not take any responsibility for any damage incurred
by its use. It is licensed under the Revised BSD license, as is QUASI88.

The sound processing portion of QUASI88kai uses source code from MAME and XMAME. The copyright to this source code
belongs to its corresponding authors. Please refer to license/MAME.TXT for licensing information.

The sound processing portion of QUASI88kai also uses source code from the FM audio generator "fmgen". The copyright to
this source code belongs to cisc. Please refer to license/FMGEN.TXT (in Japanese) for licensing information.

### Authors

- (c) 1998-2019 S.Fukunaga, R.Zumer
- (c) 2019-2022 Azamat H. Hackimov
