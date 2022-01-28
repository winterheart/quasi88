/***********************************************************************
 * グラフィック処理 (システム依存)
 *
 *  詳細は、 graph.h 参照
 ************************************************************************/

#include <gtk/gtk.h>
#include <stdio.h>

#include "quasi88.h"
#include "graph.h"
#include "device.h"

static T_GRAPH_SPEC graph_spec; /* 基本情報     */

static int graph_exist;         /* 真で、画面生成済み  */
static T_GRAPH_INFO graph_info; /* その時の、画面情報  */

/************************************************************************
 *  グラフィック処理の初期化
 *  グラフィック処理の動作
 *  グラフィック処理の終了
 ************************************************************************/

static GdkVisual *visual;
static GdkColormap *colormap;

const T_GRAPH_SPEC *graph_init(void) {
  if (verbose_proc) {
    printf("Initializing Graphic System ... ");
  }
  int found = FALSE;

  visual = gdk_visual_get_system();

  if (visual->type == GDK_VISUAL_TRUE_COLOR ||
      visual->type == GDK_VISUAL_PSEUDO_COLOR) {
    if (visual->depth == 24 || visual->depth == 32) {
#ifdef SUPPORT_32BPP
      found = TRUE;
#endif
    } else if (visual->depth == 16 || visual->depth == 15) {
#ifdef SUPPORT_16BPP
      found = TRUE;
#endif
    } else if (visual->depth == 8) {
#ifdef SUPPORT_8BPP
      found = TRUE;
#endif
    }
  }

  if (found) {
#ifdef LSB_FIRST
    if (visual->byte_order != GDK_LSB_FIRST)
      found = FALSE;
#else
    if (v->byte_order != GDK_MSB_FIRST)
      found = FALSE;
#endif
  }

  if (found) {
    colormap = gdk_colormap_get_system();
    graph_spec.window_max_width = 10000;
    graph_spec.window_max_height = 10000;
    graph_spec.fullscreen_max_width = 0;
    graph_spec.fullscreen_max_height = 0;
    graph_spec.forbid_status = FALSE;
    graph_spec.forbid_half = FALSE;

    if (verbose_proc)
      printf("OK (Cairo)\n");

    return &graph_spec;

  } else {
    if (verbose_proc)
      printf("FAILED\n");
    visual = NULL;
    return NULL;
  }
}

/************************************************************************/
GtkWidget *main_window;
static GtkWidget *menu_bar;
static GtkWidget *drawing_area;

static cairo_t *cairo_render;
static cairo_surface_t *cairo_surface;

static int create_image(int width, int height);

const T_GRAPH_INFO *graph_setup(int width, int height, int fullscreen,
                                double aspect) {
  GtkWidget *vbox;

  /* fullscreen, aspect は未使用 */

  if (graph_exist == FALSE) {

    /* ウインドウを生成する */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    {
      gtk_window_set_resizable(GTK_WINDOW(main_window), FALSE);
      gtksys_set_signal_frame(main_window);
    }

    /* メニューバーを生成する */
    {
      create_menubar(main_window, &menu_bar);
      gtk_widget_show(menu_bar);
    }

    /* 描画領域を生成する */
    drawing_area = gtk_drawing_area_new();
    {
      gtk_widget_set_size_request(GTK_WIDGET(drawing_area), width, height);
      gtksys_set_signal_view(drawing_area);
    }
    gtk_widget_show(drawing_area);

    /* アクセラレーター */

    /* メニューバーと描画領域をパックして表示 */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(main_window), vbox);
    gtk_widget_show(main_window);

    /* グラフィックコンテキストの設定 (表示後でないとだめ) ? */
    cairo_render = gdk_cairo_create(drawing_area->window);
  }

  if (create_image(width, height)) {
    cairo_set_source_surface(cairo_render, cairo_surface, 0, 0);

    graph_exist = TRUE;

    return &graph_info;
  }

  return NULL;
}

/*----------------------------------------------------------------------*/

static void color_trash(void);

static int create_image(int width, int height) {
  if (cairo_surface) {
    color_trash();
    cairo_surface_destroy(cairo_surface);
    cairo_surface = NULL;
  }

  cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);

  if (cairo_surface == NULL)
    return FALSE;

  graph_info.byte_per_pixel = 4; // CAIRO_FORMAT_RGB24 is 32 bit;
  graph_info.byte_per_line = cairo_image_surface_get_stride(cairo_surface);
  graph_info.buffer = cairo_image_surface_get_data(cairo_surface);
  graph_info.fullscreen = FALSE;
  graph_info.width = width;
  graph_info.height = height;
  graph_info.nr_color = 255;
  graph_info.write_only = FALSE;
  graph_info.broken_mouse = FALSE;
  graph_info.draw_start = NULL;
  graph_info.draw_finish = NULL;
  graph_info.dont_frameskip = FALSE;

  return TRUE;
}

/************************************************************************/

void graph_exit(void) {}

/************************************************************************
 *  色の確保
 *  色の解放
 ************************************************************************/
static GdkColor color_cell[256]; /* 確保した色の内容 */
static int nr_color_cell;        /* 確保した色の個数 */

void graph_add_color(const PC88_PALETTE_T color[], int nr_color,
                     unsigned long pixel[]) {
  int i;
  gboolean success[256];

  for (i = 0; i < nr_color; i++) {
    color_cell[nr_color_cell + i].red = (gushort)color[i].red << 8;
    color_cell[nr_color_cell + i].green = (gushort)color[i].green << 8;
    color_cell[nr_color_cell + i].blue = (gushort)color[i].blue << 8;
  }

  i = gdk_colormap_alloc_colors(colormap, &color_cell[nr_color_cell], nr_color,
                                FALSE, FALSE, success);

  /* debug */
  if (i != 0)
    printf("Color Alloc Failed %d/%d\n", i, nr_color);

  for (i = 0; i < nr_color; i++) {
    pixel[i] = color_cell[nr_color_cell + i].pixel;
  }

  nr_color_cell += nr_color;
}

/************************************************************************/

void graph_remove_color(int nr_pixel, unsigned long pixel[]) {
  nr_color_cell -= nr_pixel;

  gdk_colormap_free_colors(colormap, &color_cell[nr_color_cell], nr_pixel);
}

/*----------------------------------------------------------------------*/

static void color_trash(void) {
  if (nr_color_cell) {
    gdk_colormap_free_colors(colormap, &color_cell[0], nr_color_cell);
  }

  nr_color_cell = 0;
}

/************************************************************************
 *  グラフィックの更新
 ************************************************************************/

void graph_update(int nr_rect, T_GRAPH_RECT rect[]) {
  cairo_paint(cairo_render);
}

/************************************************************************
 *  タイトルの設定
 *  属性の設定
 ************************************************************************/

void graph_set_window_title(const char *title) {
  if (main_window) {
    gtk_window_set_title(GTK_WINDOW(main_window), title);
  }
}

/************************************************************************/

static int gtksys_keyrepeat_on = TRUE;

void graph_set_attribute(int mouse_show, int grab, int keyrepeat_on) {
  /* マウスは未対応 */
  /* グラブは未対応 */
  gtksys_keyrepeat_on = keyrepeat_on;
}
