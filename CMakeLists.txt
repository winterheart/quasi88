cmake_minimum_required(VERSION 3.10)

project(quasi88 VERSION 0.7.1)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Reasonable default
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(ENABLE_SDL "Enable building for libSDL" ON)
option(ENABLE_WIN "Enable building for Windows" OFF)
option(ENABLE_SOUND "Enable sound support" ON)
option(ENABLE_FMGEN "Enable FM Generator" ON)
option(ENABLE_DOUBLE "Enable double screen" ON)
option(ENABLE_UTF8 "Enable UTF8" ON)
option(ENABLE_PC8801_KEYBOARD_BUG "Enable emulation of PC-8801 keyboard bug" OFF)
option(ENABLE_8BPP "Enable 8bpp support" ON)
option(ENABLE_16BPP "Enable 16bpp support" ON)
option(ENABLE_32BPP "Enable 32bpp support" ON)
option(ENABLE_SNAPSHOT "Enable snapshot command support" ON)
option(ENABLE_MONITOR "Enable Monitor (Debugger) support" OFF)

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
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	add_definitions(-DINLINE=__inline)
else()
	add_definitions(-DINLINE=static)
endif()

if(UNIX)
	add_definitions(-DQUASI88_FUNIX)
elseif(WIN32)
	add_definitions(-DQUASI88_FWIN)
else()
	message(FATAL_ERROR "Unknown architecture!")
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

if (BUILD_TESTING)
	add_subdirectory(tests)
endif()

#### Common sources

include_directories(
	dependencies/lodepng
	src
	${CMAKE_CURRENT_BINARY_DIR}
)

set(COMMON_SOURCES
	dependencies/lodepng/lodepng.cpp
	src/basic.cpp
	src/crtcdmac.cpp
	src/debug.cpp
	src/emu.cpp
	src/event.cpp
	src/fdc.cpp
	src/file-op.cpp
	src/fname.cpp
	src/getconf.cpp
	src/image.cpp
	src/intr.cpp
	src/keyboard.cpp
	src/memory.cpp
	src/menu.cpp
	src/menu-screen.cpp
	src/monitor.cpp
	src/pause.cpp
	src/pc88main.cpp
	src/pc88sub.cpp
	src/pio.cpp
	src/romaji.cpp
	src/screen.cpp
	src/screen-8bpp.cpp
	src/screen-16bpp.cpp
	src/screen-32bpp.cpp
	src/screen-snapshot.cpp
	src/snapshot.cpp
	src/soundbd.cpp
	src/status.cpp
	src/suspend.cpp
	src/q8tk.cpp
	src/q8tk-glib.cpp
	src/quasi88.cpp
	src/utility.cpp
	src/z80.cpp
	src/z80-debug.cpp
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

#### SDL target
if(ENABLE_SDL)
	add_definitions(-DQUASI88_SDL)
	find_package(SDL2 CONFIG REQUIRED)

	list(APPEND SDL_LIBS SDL2::SDL2 SDL2::SDL2main)
	list(APPEND SDL_INCLUDES
		src/SDL
	)

	set(SDL_SOURCES
		src/SDL/event.cpp
		src/SDL/graph.cpp
		src/SDL/keymap.cpp
		src/SDL/main.cpp
		src/SDL/wait.cpp
	)

	#### SDL Sound
	if(ENABLE_SOUND)
		set(SDL_SOUND_SOURCES
			src/snddrv/quasi88/SDL/audio.cpp
			src/snddrv/quasi88/SDL/sdl.cpp
		)
		list(APPEND SDL_INCLUDES src/snddrv/SDL)
	endif(ENABLE_SOUND)

	add_executable(
		${PROJECT_NAME}.sdl ${COMMON_SOURCES} ${SDL_SOURCES} ${SOUND_SOURCES} ${SDL_SOUND_SOURCES}
	)
	target_include_directories(${PROJECT_NAME}.sdl PRIVATE ${SDL_INCLUDES})
	target_link_libraries(${PROJECT_NAME}.sdl	${COMMON_LIBS} ${SDL_LIBS})

	#### macOS needs rpath set if using a Framework
	if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
 		if(SDL2_BINDIR MATCHES ".*SDL2\\.framework.*")
			add_custom_command(
				TARGET ${PROJECT_NAME}.sdl POST_BUILD
				COMMAND install_name_tool -add_rpath ${SDL2_FRAMEWORK_PARENT_PATH} ${PROJECT_NAME}.sdl
			VERBATIM)
   		endif()
	endif()
endif(ENABLE_SDL)

if(ENABLE_WIN)
	add_definitions(-DQUASI88_WIN32)
	add_definitions(-DWIN32 -D_WINDOWS -DLANG_EN -D_CRT_SECURE_NO_WARNINGS)

	list(APPEND WIN32_INCLUDES
		src/WIN32
	)

	set(WIN32_SOURCES
		src/WIN32/audio.cpp
		src/WIN32/event.cpp
		src/WIN32/graph.cpp
		src/WIN32/main.cpp
		src/WIN32/menubar.cpp
		src/WIN32/wait.cpp
		src/WIN32/quasi88.rc
	)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GF")
	add_executable(
		${PROJECT_NAME}.win WIN32 ${COMMON_SOURCES} ${WIN32_SOURCES} ${SOUND_SOURCES}
	)
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
	set_target_properties(${PROJECT_NAME}.win PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	target_include_directories(${PROJECT_NAME}.win PRIVATE ${WIN32_INCLUDES})
	target_link_options(${PROJECT_NAME}.win PRIVATE /DYNAMICBASE:NO)
	target_link_libraries(${PROJECT_NAME}.win imm32.lib winmm.lib)
	# libraries for RA: odbc32.lib odbccp32.lib winhttp.lib
endif(ENABLE_WIN)

configure_file(src/num_ver.h.cmake.in num_ver.h @ONLY)

# Packaging stuff
set(CPACK_SOURCE_GENERATOR "TXZ")
set(CPACK_SOURCE_IGNORE_FILES
	\\.git/
	build/
	cmake-build-*
	".*~$"
	)
set(CPACK_VERBATIM_VARIABLES YES)
include(CPack)
