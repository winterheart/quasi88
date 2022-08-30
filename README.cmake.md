# Compiling with CMake

## Requirements

Currently, cmake build framework supports modern Linux distributions (such as recent Debian/Ubuntu or Fedora), Windows
and (probably) *BSD and macOS. Minimal development requirements are:

* a recent compiler with C99 and C++17 support (GCC, LLVM, MSVC)
* cmake 3.10
* ninja (optionally)
* libSDL 2.0.12 (for SDL backend)
* libX11 1.6.9 (for X.Org backend)
* libxxf86dga (for X.Org backend)
* libxxf86vm-dev (for X.Org backend)
* GTK2+ 2.24 (for GTK backend)
* ReadLine 7 (optional for Monitor)

On Windows dependencies are controlled via [vcpkg package manager](https://vcpkg.io/). Here quick commands how to get
it:

```
# Assuming that vcpkg will placed at C:\users\user
cd C:\users\user
git clone https://github.com/microsoft/vcpkg
```

After that just add `-DCMAKE_TOOLCHAIN_FILE=C:/users/user/scripts/buildsystems/vcpkg.cmake` to cmake input during
configuration: 

```
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/users/user/scripts/buildsystems/vcpkg.cmake <build options>
# Now build quasi88
cmake --build build -j2
```

## Quick start

```
cmake -B build -G Ninja -DENABLE_GTK2=ON -DENABLE_X11=ON -DENABLE_SDL=ON -DENABLE_JOYSTICK=SDL
cmake --build build -j2
```

## Configuration and compilation

Call cmake with one of supported backend (X11, SDL, GTK2, WIN). By default, enabled only X11, but you can enable all of
them. Here is example:

```
cmake -B build -G Ninja -DENABLE_GTK2=ON -DENABLE_X11=ON -DENABLE_SDL=ON
```

After that you ready to compile quasi88kai:

```
cmake --build build -j2
```

Compiled binaries can be found in `build` directory.

### All compilation options

Here list of all options that can be enabled/disabled via -DENABLE_FOO=ON/OFF directives:

| Option                     | Meaning                           | Default |
|----------------------------|-----------------------------------|---------|
| ENABLE_GTK2                | Enable GTK2 backend               |     OFF |
| ENABLE_SDL                 | Enable SDL backend                |     OFF |
| ENABLE_X11                 | Enable X.Org backend              |      ON |
| ENABLE_WIN                 | Enable Windows backend            |     OFF |
| ENABLE_JOYSTICK            | Enable joystick support for X.Org (options: NO, SDL, LINUX_USB, BSD_USB) | NO |
| ENABLE_SOUND               | Enable sound support              |      ON |
| ENABLE_FMGEN               | Enable FM sound generator         |      ON |
| ENABLE_DOUBLE              | Enable double screen              |      ON |
| ENABLE_UTF8                | Enable UTF-8                      |      ON |
| ENABLE_PC8801_KEYBOARD_BUG | Enable emulation of keyboard bug  |     OFF |
| ENABLE_8BPP                | Enable 8bpp support               |      ON |
| ENABLE_16BPP               | Enable 16bpp support              |      ON |
| ENABLE_32BPP               | Enable 32bpp support              |      ON |
| ENABLE_SNAPSHOT            | Enable snapshot command support   |      ON |
| ENABLE_MONITOR             | Enable Monitor (Debbuger) support |     OFF |
