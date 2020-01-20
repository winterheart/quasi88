# Compiling with CMake

## Requirements

Currently cmake build framework supports modern Linux distributions (such as recent Debian/Ubuntu or Fedora) and 
(probably) *BSD and macOS. Minimal development requirements are:

* recent compiler (GCC or llvm)
* cmake 3.10
* libSDL 1.2 (for SDL backend)
* libX11 1.6.9 (for X.Org backend)
* GTK2+ 2.24 (for GTK backend)
* ReadLine 7 (optional for Monitor)

## Quick start

```
mkdir build
cd build
cmake -DENABLE_GTK2=ON -DENABLE_X11=ON -DENABLE_SDL=ON -DENABLE_JOYSTICK=SDL ..
make -j2
```

## Configuration and compilation

First, prepare build directory:

```
mkdir build
cd build
```

Then call cmake with one of supported backend (X11, SDL, GTK2). By default enabled only X11, but you can enable all of
them. Here is example:

```
cmake -DENABLE_GTK2=ON -DENABLE_X11=ON -DENABLE_SDL=ON ..
```

After that you ready to compile quasi88kai:

```
make
```

### All compilation options

Here list of all options that can be enabled/disabled via -DENABLE_FOO=ON/OFF directives:

| Option          | Meaning                           | Default |
+-----------------+-----------------------------------+-----|
| ENABLE_GTK2     | Enable GTK2 backend               | OFF |
| ENABLE_SDL      | Enable SDL backend                | OFF |
| ENABLE_X11      | Enable X.Org backend              |  ON |
| ENABLE_JOYSTICK | Enable joystick support for X.Org (available options: NO, SDL, LINUX_USB, BSD_USB) | NO |
| ENABLE_SOUND    | Enable sound support              |  ON |
| ENABLE_FMGEN    | Enable FM sound generator         |  ON |
| ENABLE_DOUBLE   | Enable double screen              |  ON |
| ENABLE_UTF8     | Enable UTF-8                      |  ON |
| ENABLE_PC8801_KEYBOARD_BUG | Enable emulation of keyboard bug | OFF |
| ENABLE_8BPP     | Enable 8bpp support               |  ON |
| ENABLE_16BPP    | Enable 16bpp support              |  ON |
| ENABLE_32BPP    | Enable 32bpp support              |  ON |
| ENABLE_SNAPSHOT | Enable snapshot command support   |  ON |
| ENABLE_MONITOR  | Enable Monitor (Debbuger) support | OFF |
