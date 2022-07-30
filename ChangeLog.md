# QUASI88kai Changelog

All notable changes to this project will be documented in this file. An old version of
this file can be found in `document/HISTORY.TXT` file.

## 0.7.0 - Unreleased
* Removed old Classic Mac OS support.
* Unify file operations functions for all supported platforms. Now project requires C++17 capable compiler.
* Updated SDL backend to use of SDL2 library.

## 0.6.9 - 2022-02-06

* RetroAchievements improvements: updated RAIntegration to 0.76.3, some fixes.
* Migration buildsystem to CMake - initial support for Linux and Windows.
* Integration with Github Actions CI pipelines.
* Major update to X11, GTK, SDL backends:
  * GTK migrated from GTK1 to GTK2, some improvements towards to GTK3.
  * Removed deprecated code in X11.
  * Fixed pointer-to-int-cast warnings.
* Fixed some memory leaks in X11 and SDL backends.
