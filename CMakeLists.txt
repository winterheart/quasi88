cmake_minimum_required(VERSION 3.10)

project(quasi88 VERSION 0.6.8)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Reasonable default
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

option(ENABLE_GTK2 "Enable building for GTK2" OFF)
option(ENABLE_SDL "Enable building for libSDL" OFF)
option(ENABLE_X11 "Enable building for X.org" ON)
option(ENABLE_SOUND "Enable sound support" ON)
option(ENABLE_FMGEN "Enable FM Generator" ON)
option(ENABLE_DOUBLE "Enable double screen" ON)
option(ENABLE_UTF8 "Enable UTF8" ON)
option(ENABLE_PC8801_KEYBOARD_BUG "Enable emulation of PC-8801 keyboard bug" OFF)
option(ENABLE_8BPP "Enable 8bpp support" ON)
option(ENABLE_16BPP "Enable 16bpp support" ON)
option(ENABLE_32BPP "Enable 32bpp support" ON)
option(ENABLE_SNAPSHOT "Enable snapshot command support" ON)
option(ENABLE_MONITOR "Enable Monitor (Debbuger) support" OFF)

# Joystick select for X
set(ENABLE_JOYSTICK "SDL" CACHE STRING "Enable Joystick support for X.org (NO/SDL/LINUX_USB/BSD_USB, default NO)")
set_property(CACHE ENABLE_JOYSTICK PROPERTY STRINGS "NO" "SDL" "LINUX_USB" "BSD_USB")


if(ENABLE_MONITOR)
	option(ENABLE_MONITOR_READLINE "Enable GNU Readline for Monitor support" ON)
	option(ENABLE_MONITOR_LOCALE "Enable localization of Monitor" ON)
endif(ENABLE_MONITOR)

set(ROMDIR "~/.quasi88/rom/" CACHE STRING "Location of ROM dir")
set(DISKDIR "~/.quasi88/disk/" CACHE STRING "Location of DISK dir")
set(TAPEDIR "~/.quasi88/tape/" CACHE STRING "Location of TAPE dir")

#### Basic definitions and includes
add_definitions(
	-DCLIB_DECL=
	-DROM_DIR="${ROMDIR}"
	-DDISK_DIR="${DISKDIR}"
	-DTAPE_DIR="${TAPEDIR}"
)

# Compiler options
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	add_definitions(-DINLINE=static\ __inline__)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Intel")
	add_definitions(-DINLINE=static\ inline)
else()
	add_definitions(-DINLINE=static)
endif()

# Check endianness
include(TestBigEndian)
TEST_BIG_ENDIAN(BIG_ENDIAN)
if(${BIG_ENDIAN} EQUAL 0)
	# FIXME Rework this, it should be simpler
	add_definitions(-DLSB_FIRST)
endif()

include(CheckSymbolExists)
# Check gettimeofday()
check_symbol_exists(gettimeofday "sys/time.h" HAVE_GETTIMEOFDAY)
if(HAVE_GETTIMEOFDAY)
	add_definitions(-DHAVE_GETTIMEOFDAY)
endif(HAVE_GETTIMEOFDAY)

include(CheckTypeSize)
check_type_size("long long" SIZEOF_LONG_LONG)
if(SIZEOF_LONG_LONG EQUAL "8")
	add_definitions(-DHAVE_LONG_LONG)
endif()


#### Common sources

include_directories(
	src
)

set(COMMON_SOURCES
	src/basic.c
	src/crtcdmac.c
	src/emu.c
	src/fdc.c
	src/getconf.c
	src/image.c
	src/intr.c
	src/keyboard.c
	src/memory.c
	src/menu.c
	src/menu-screen.c
	src/monitor.c
	src/pause.c
	src/pc88main.c
	src/pc88sub.c
	src/pio.c
	src/romaji.c
	src/screen.c
	src/screen-8bpp.c
	src/screen-16bpp.c
	src/screen-32bpp.c
	src/screen-snapshot.c
	src/snapshot.c
	src/soundbd.c
	src/status.c
	src/suspend.c
	src/q8tk.c
	src/q8tk-glib.c
	src/quasi88.cpp
	src/z80.c
	src/z80-debug.c
)

#### Double size screen support
if(ENABLE_DOUBLE)
	add_definitions(-DSUPPORT_DOUBLE)
endif(ENABLE_DOUBLE)

#### Unicode support
if(ENABLE_UTF8)
	add_definitions(-DSUPPORT_UTF8)
endif(ENABLE_UTF8)

#### Keyboard bug emulation
if(ENABLE_PC8801_KEYBOARD_BUG)
	add_definitions(-DUSE_KEYBOARD_BUG)
endif(ENABLE_PC8801_KEYBOARD_BUG)

#### 8bpp
if(ENABLE_8BPP)
	add_definitions(-DSUPPORT_8BPP)
endif(ENABLE_8BPP)

#### 16bpp
if(ENABLE_16BPP)
	add_definitions(-DSUPPORT_16BPP)
endif(ENABLE_16BPP)

#### 32bpp
if(ENABLE_32BPP)
	add_definitions(-DSUPPORT_32BPP)
endif(ENABLE_32BPP)

if(ENABLE_SNAPSHOT)
	add_definitions(-DUSE_SSS_CMD)
endif(ENABLE_SNAPSHOT)

#### Sound support, base files
if(ENABLE_SOUND)
	set(SOUND_SOURCES
		src/snddrv/quasi88/beep.c
		src/snddrv/quasi88/beepintf.c
		src/snddrv/quasi88/mame-quasi88.c
		src/snddrv/src/sound/2203intf.c
		src/snddrv/src/sound/2608intf.c
		src/snddrv/src/sound/ay8910.c
		src/snddrv/src/sound/flt_rc.c
		src/snddrv/src/sound/flt_vol.c
		src/snddrv/src/sound/fm.c
		src/snddrv/src/sound/samples.c
		src/snddrv/src/sound/wavwrite.c
		src/snddrv/src/sound/ymdeltat.c
		src/snddrv/src/driver.c
		src/snddrv/src/restrack.c
		src/snddrv/src/sndintrf.c
		src/snddrv/src/sound.c
		src/snddrv/src/streams.c
	)

	add_definitions(-DUSE_SOUND)
	include_directories(
		src/snddrv/quasi88
		src/snddrv/src/sound
		src/snddrv/src
		src/snddrv/src/unix
		src/snddrv/src/unix/sysdep
	)

	if(ENABLE_FMGEN)
		list(APPEND SOUND_SOURCES
			src/snddrv/quasi88/2203fmgen.cpp
			src/snddrv/quasi88/2608fmgen.cpp
			src/fmgen/fmgen.cpp
			src/fmgen/fmtimer.cpp
			src/fmgen/opna.cpp
			src/fmgen/psg.cpp
		)
		add_definitions(-DUSE_FMGEN)
		include_directories(src/fmgen)
	endif(ENABLE_FMGEN)
endif(ENABLE_SOUND)

if(ENABLE_MONITOR)
	add_definitions(-DUSE_MONITOR)

	if(ENABLE_MONITOR_READLINE)
		find_package(Readline REQUIRED)
		add_definitions(-DUSE_GNU_READLINE)
		include_directories(${READLINE_INCLUDE_DIRS})
		list(APPEND COMMON_LIBS ${READLINE_LIBRARIES})
	endif(ENABLE_MONITOR_READLINE)

	if(ENABLE_MONITOR_LOCALE)
		add_definitions(-DUSE_LOCALE)
	endif(ENABLE_MONITOR_LOCALE)

endif(ENABLE_MONITOR)

if(${ENABLE_JOYSTICK} STREQUAL "NO")
	add_definitions(-DJOY_NOTHING)
elseif(${ENABLE_JOYSTICK} STREQUAL "SDL")
	find_package(SDL REQUIRED)
	add_definitions(-DJOY_SDL)
	include_directories(${SDL_INCLUDE_DIR})
	list(APPEND COMMON_LIBS ${SDL_LIBRARY})
elseif(${ENABLE_JOYSTICK} STREQUAL "LINUX_USB")
	add_definitions(-DJOY_LINUX_USB)
	include(FindPkgConfig)
	pkg_check_modules(LIBUSB REQUIRED libusb)
	include_directories(${LIBUSB_INCLUDE_DIRS})
	list(APPEND COMMON_LIBS ${LIBUSB_LIBRARIES})
elseif(${ENABLE_JOYSTICK} STREQUAL "BSD_USB")
	add_definitions(-DJOY_BSD_USB)
	add_definitions(-DHAVE_USBHID_H)
	# FIXME need implementation for *BSD USBHID, -lusbhid
endif()


if(ENABLE_X11)
	find_package(X11 REQUIRED)
	list(APPEND X11_LIBS ${X11_X11_LIB} ${X11_Xext_LIB} ${X11_Xxf86vm_LIB})

	if(ENABLE_SOUND)
		add_definitions(-DSYSDEP_DSP_SDL)
		find_package(SDL REQUIRED)
		list(APPEND X11_INCLUDES
			src/snddrv/quasi88/X11
			${SDL_INCLUDE_DIR}
		)
		list(APPEND X11_LIBS ${SDL_LIBRARY})

		set(X11_SOUND_SOURCES
			src/snddrv/quasi88/X11/audio.c
			src/snddrv/src/unix/sysdep/dsp-drivers/sdl.c
			src/snddrv/src/unix/sysdep/misc.c
			src/snddrv/src/unix/sysdep/plugin_manager.c
			src/snddrv/src/unix/sysdep/rc.c
			src/snddrv/src/unix/sysdep/sysdep_dsp.c
			src/snddrv/src/unix/sysdep/sysdep_mixer.c
			src/snddrv/src/unix/sysdep/sysdep_sound_stream.c
			src/snddrv/src/unix/sound.c
		)
	endif(ENABLE_SOUND)

	if(X11_XShm_FOUND)
		list(APPEND X11_INCLUDES ${X11_XShm_INCLUDE_PATH})
		add_definitions(-DMITSHM)
	endif(X11_XShm_FOUND)
	if(X11_Xv_FOUND)
		list(APPEND X11_INCLUDES ${X11_Xv_INCLUDE_PATH})
		list(APPEND X11_LIBS ${X11_Xv_LIB})
		add_definitions(-DUSE_XV)
	endif(X11_Xv_FOUND)

	check_library_exists(Xxf86dga XDGAQueryExtension "${X11_LIB_SEARCH_PATH}" HAVE_XDGA)
    if(HAVE_XDGA)
			add_definitions(-DUSE_DGA)
			list(APPEND X11_LIBS Xxf86dga)
    endif()

	add_definitions(
		-DQUASI88_X11
		-DUSE_DGA
	)
	list(APPEND X11_INCLUDES
		src/FUNIX
		src/X11
		${X11_Xext_INCLUDE_PATH}
	)

	set(X11_SOURCES
		src/FUNIX/file-op.c
		src/X11/event.c
		src/X11/graph.c
		src/X11/joystick.c
		src/X11/main.c
		src/X11/wait.c
	)
	add_executable(
		${PROJECT_NAME} ${COMMON_SOURCES} ${X11_SOURCES} ${SOUND_SOURCES} ${X11_SOUND_SOURCES}
	)
	target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDES})
	target_link_libraries(${PROJECT_NAME} ${COMMON_LIBS} ${X11_LIBS})
endif(ENABLE_X11)

#### SDL target
if(ENABLE_SDL)
	add_definitions(-DQUASI88_SDL)
	find_package(SDL REQUIRED)

	list(APPEND SDL_LIBS ${SDL_LIBRARY})
	list(APPEND SDL_INCLUDES
		src/FUNIX
		src/SDL
		${SDL_INCLUDE_DIR}
	)

	set(SDL_SOURCES
		src/FUNIX/file-op.c
		src/SDL/event.c
		src/SDL/graph.c
		src/SDL/main.c
		src/SDL/wait.c
	)

	#### SDL Sound
	if(ENABLE_SOUND)
		set(SDL_SOUND_SOURCES
			src/snddrv/quasi88/SDL/audio.c
			src/snddrv/quasi88/SDL/sdl.c
		)
		list(APPEND SDL_INCLUDES src/snddrv/SDL)
	endif(ENABLE_SOUND)

	add_executable(
		${PROJECT_NAME}.sdl ${COMMON_SOURCES} ${SDL_SOURCES} ${SOUND_SOURCES} ${SDL_SOUND_SOURCES}
	)
	target_include_directories(${PROJECT_NAME}.sdl PRIVATE ${SDL_INCLUDES})
	target_link_libraries(${PROJECT_NAME}.sdl	${COMMON_LIBS} ${SDL_LIBS})
endif(ENABLE_SDL)

if(ENABLE_GTK2)
	add_definitions(-DQUASI88_GTK)
	find_package(GTK2 REQUIRED)
	find_package(SDL REQUIRED)

	list(APPEND GTK_LIBS ${GTK2_LIBRARIES} ${SDL_LIBRARY})

	list(APPEND GTK_INCLUDES
		src/FUNIX
		src/GTK
		${GTK2_INCLUDE_DIRS}
		${SDL_INCLUDE_DIR}
	)

	set(GTK_SOURCES
		src/FUNIX/file-op.c
		src/GTK/event.c
		src/GTK/graph.c
		src/GTK/main.c
		src/GTK/menubar.c
		src/GTK/wait.c
	)

	if(ENABLE_SOUND)
		set(GTK_SOUND_SOURCES
			src/snddrv/quasi88/SDL/audio.c
			src/snddrv/quasi88/SDL/sdl.c
		)
		list(APPEND GTK_INCLUDES src/snddrv/SDL)
	endif(ENABLE_SOUND)

	add_executable(
		${PROJECT_NAME}.gtk ${COMMON_SOURCES} ${GTK_SOURCES} ${SOUND_SOURCES} ${GTK_SOUND_SOURCES}
	)
	target_include_directories(${PROJECT_NAME}.gtk PRIVATE ${GTK_INCLUDES})
	target_link_libraries(${PROJECT_NAME}.gtk ${COMMON_LIBS} ${GTK_LIBS})

endif(ENABLE_GTK2)
