/***********************************************************************
 * グラフィック処理 (システム依存)
 *
 *  詳細は、 graph.h 参照
 ************************************************************************/

#include <cstdio>
#include <SDL2/SDL.h>

extern "C" {
#include "quasi88.h"
#include "graph.h"
#include "device.h"
}

int sdl_mouse_rel_move; /* マウス相対移動量検知可能か  */

static T_GRAPH_SPEC graph_spec; /* 基本情報     */
static T_GRAPH_INFO graph_info; /* その時の、画面情報  */

static SDL_Window *sdl_window;
static SDL_Renderer *sdl_renderer;

static SDL_Texture *sdl_texture;
static SDL_Surface *sdl_offscreen;

static SDL_DisplayMode sdl_display_mode = {SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0};

static int x_pos = SDL_WINDOWPOS_UNDEFINED, y_pos = SDL_WINDOWPOS_UNDEFINED;

int sdl_init(void) {
  if (verbose_proc) {
    SDL_version libver;
    SDL_GetVersion(&libver);
    printf("Initializing SDL (%d.%d.%d) ... ", libver.major, libver.minor, libver.patch);
    fflush(nullptr);
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    if (verbose_proc)
      printf("Failed\n");
    fprintf(stderr, "SDL Error: %s\n", SDL_GetError());

    return FALSE;
  } else {
    if (verbose_proc)
      printf("OK\n");
    return TRUE;
  }
}

void sdl_exit(void) { SDL_Quit(); }

/*
 * Initialize SDL window and renderer
 * Basically, we don't need to know bpp and depth settings of renderer.
 * Internally we are using SDL_PIXELFORMAT_RGB888 with 4 bpp and 32 bit depth.
 * During rendering SDL2 converts SDL_Texture to proper native conversion.
 */
const T_GRAPH_SPEC *graph_init(void) {
  if (SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_OPENGL, &sdl_window, &sdl_renderer) != 0) {
    printf("Failed to initialize SDL: %s!\n", SDL_GetError());
    return nullptr;
  }

  if (verbose_proc) {
    SDL_RendererInfo ri;
    if (SDL_GetRendererInfo(sdl_renderer, &ri) < 0) {
      printf("Failed to get info from SDL: %s\n", SDL_GetError());
      return nullptr;
    }
    printf("Initializing Graphic System (SDL:%s) ... \n", ri.name);
  }

  // Get the highest possible mode
  if (SDL_GetDisplayMode(0, 0, &sdl_display_mode) != 0) {
    printf("Failed to get info from SDL: %s\n", SDL_GetError());
    return nullptr;
  }
  printf("  Best mode: %dx%dx%d @ %d Hz\n", sdl_display_mode.w, sdl_display_mode.h,
         SDL_BITSPERPIXEL(sdl_display_mode.format), sdl_display_mode.refresh_rate);

  graph_spec.window_max_width = sdl_display_mode.w;
  graph_spec.window_max_height = sdl_display_mode.h;
  graph_spec.fullscreen_max_width = sdl_display_mode.w;
  graph_spec.fullscreen_max_height = sdl_display_mode.h;
  graph_spec.forbid_status = false;
  graph_spec.forbid_half = false;

  if (verbose_proc)
    printf("  INFO: Maxsize=win(%d,%d),full(%d,%d)\n", sdl_display_mode.w, sdl_display_mode.h, sdl_display_mode.w,
           sdl_display_mode.h);

  return &graph_spec;
}

const T_GRAPH_INFO *graph_setup(int width, int height, int fullscreen, double aspect) {
  Uint32 flags = SDL_WINDOW_OPENGL;
  int real_w, real_h;
  if (verbose_proc) {
    SDL_RendererInfo ri;
    SDL_GetRendererInfo(sdl_renderer, &ri);
    printf("Setting up Graphic System (SDL:%s) ...\n", ri.name);
  }

  if (fullscreen) {
    real_w = sdl_display_mode.w;
    real_h = sdl_display_mode.h;
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (verbose_proc)
      printf("  Trying full screen mode ... ");
  } else {
    real_w = width;
    real_h = height;
    if (verbose_proc)
      printf("  Opening window ... ");
  }

  SDL_SetWindowSize(sdl_window, real_w, real_h);
  SDL_RenderSetLogicalSize(sdl_renderer, width, height);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_SetWindowFullscreen(sdl_window, flags);

  if (x_pos == SDL_WINDOWPOS_UNDEFINED || y_pos == SDL_WINDOWPOS_UNDEFINED) {
    x_pos = y_pos = SDL_WINDOWPOS_CENTERED;
  } else {
    SDL_GetWindowPosition(sdl_window, &x_pos, &y_pos);
  }
  SDL_SetWindowPosition(sdl_window, x_pos, y_pos);

  if (verbose_proc)
    printf("OK (%dx%d -> %dx%d)\n", width, height, real_w, real_h);

  if (verbose_proc)
    printf("  Allocating screen buffer ... ");

  sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, width, height);
  sdl_offscreen = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
  if (verbose_proc) {
    printf("%s\n", (sdl_offscreen && sdl_texture) ? "OK" : "FAILED");
  }

  if (sdl_offscreen == nullptr || sdl_texture == nullptr)
    return nullptr;

  graph_info.fullscreen = fullscreen;
  graph_info.width = sdl_offscreen->w;
  graph_info.height = sdl_offscreen->h;
  graph_info.byte_per_pixel = sdl_offscreen->format->BytesPerPixel;
  graph_info.byte_per_line = sdl_offscreen->pitch;
  graph_info.buffer = sdl_offscreen->pixels;
  graph_info.nr_color = 255;
  graph_info.write_only = FALSE;
  graph_info.broken_mouse = FALSE;
  graph_info.draw_start = nullptr;
  graph_info.draw_finish = nullptr;
  graph_info.dont_frameskip = FALSE;

  if (verbose_proc) {
    int w, h;
    SDL_GetWindowSize(sdl_window, &w, &h);
    printf("    VideoMode %dx%d -> %dx%dx%d(%d)  %c  R:%x G:%x B:%x\n", width, height, w, h,
           sdl_offscreen->format->BitsPerPixel, sdl_offscreen->format->BytesPerPixel,
           (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 'F' : '-', sdl_offscreen->format->Rmask,
           sdl_offscreen->format->Gmask, sdl_offscreen->format->Bmask);
  }

  return &graph_info;
}

void graph_exit(void) {
  SDL_SetRelativeMouseMode(SDL_FALSE);
  SDL_FreeSurface(sdl_offscreen);
  SDL_DestroyTexture(sdl_texture);
  SDL_DestroyRenderer(sdl_renderer);
  SDL_DestroyWindow(sdl_window);

  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void graph_add_color(const PC88_PALETTE_T color[], int nr_color, unsigned long pixel[]) {
  int i;
  for (i = 0; i < nr_color; i++) {
    pixel[i] = SDL_MapRGB(sdl_offscreen->format, color[i].red, color[i].green, color[i].blue);
  }
}

void graph_remove_color(int nr_pixel, unsigned long pixel[]) {
  /* Nothing to do */
}

/*
 * Update main texture and draw it to render
 */
void graph_update(int nr_rect, T_GRAPH_RECT rect[]) {
  SDL_UpdateTexture(sdl_texture, nullptr, sdl_offscreen->pixels, sdl_offscreen->pitch);
  SDL_RenderClear(sdl_renderer);
  SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, nullptr);
  SDL_RenderPresent(sdl_renderer);
}

void graph_set_window_title(const char *title) { SDL_SetWindowTitle(sdl_window, title); }

void graph_set_attribute(int mouse_show, int grab, int keyrepeat_on) {
  if (mouse_show)
    SDL_ShowCursor(SDL_ENABLE);
  else
    SDL_ShowCursor(SDL_DISABLE);

  if (grab)
    SDL_SetRelativeMouseMode(SDL_TRUE);
  else
    SDL_SetRelativeMouseMode(SDL_FALSE);
  /*
    if (keyrepeat_on)
      SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    else
      SDL_EnableKeyRepeat(0, 0);
  */
  sdl_mouse_rel_move = (!mouse_show && grab) ? TRUE : FALSE;

  /* SDL は、グラブ中かつマウスオフなら、ウインドウの端にマウスが
     ひっかかっても、マウス移動の相対量を検知できる。

     なので、この条件を sdl_mouse_rel_move にセットしておき、
     真なら、マウス移動は相対量、偽なら絶対位置とする (event.c)

     メニューでは、かならずグラブなし (マウスはあり or なし) なので、
     この条件にはかからず、常にウインドウの端でマウスは停止する。
  */
}
