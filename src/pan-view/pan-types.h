/*
 * Copyright (C) 2006 John Ellis
 * Copyright (C) 2008 - 2016 The Geeqie Team
 *
 * Author: John Ellis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef PAN_VIEW_PAN_TYPES_H
#define PAN_VIEW_PAN_TYPES_H

#include "cache-loader.h"
#include "filedata.h"

/* thumbnail sizes and spacing */

#define PAN_THUMB_SIZE_DOTS 4
#define PAN_THUMB_SIZE_NONE 24
#define PAN_THUMB_SIZE_SMALL 64
#define PAN_THUMB_SIZE_NORMAL 128
#define PAN_THUMB_SIZE_LARGE 256
#define PAN_THUMB_SIZE pw->thumb_size

#define PAN_THUMB_GAP_DOTS 2
#define PAN_THUMB_GAP_SMALL 14
#define PAN_THUMB_GAP_NORMAL 30
#define PAN_THUMB_GAP_LARGE 40
#define PAN_THUMB_GAP_HUGE 50
#define PAN_THUMB_GAP pw->thumb_gap

/* basic sizes, colors, spacings */

#define PAN_SHADOW_OFFSET 6
#define PAN_SHADOW_FADE 5
#define PAN_SHADOW_COLOR 0, 0, 0
#define PAN_SHADOW_ALPHA 64

#define PAN_OUTLINE_THICKNESS 1
#define PAN_OUTLINE_COLOR_1 255, 255, 255
#define PAN_OUTLINE_COLOR_2 64, 64, 64
#define PAN_OUTLINE_ALPHA 180

#define PAN_BACKGROUND_COLOR 150, 150, 150

#define PAN_GRID_SIZE 60
#define PAN_GRID_COLOR 0, 0, 0
#define PAN_GRID_ALPHA 20

#define PAN_BOX_COLOR 255, 255, 255
#define PAN_BOX_ALPHA 100
#define PAN_BOX_BORDER 20

#define PAN_BOX_OUTLINE_THICKNESS 4
#define PAN_BOX_OUTLINE_COLOR 0, 0, 0
#define PAN_BOX_OUTLINE_ALPHA 128

#define PAN_TEXT_BORDER_SIZE ((PanBorderType)4)
#define PAN_TEXT_COLOR 0, 0, 0

/* popup info box */

#define PAN_POPUP_COLOR 255, 255, 225
#define PAN_POPUP_ALPHA 255
#define PAN_POPUP_BORDER 1
#define PAN_POPUP_BORDER_COLOR 0, 0, 0
#define PAN_POPUP_TEXT_COLOR 0, 0, 0


#define PAN_GROUP_MAX 16



typedef enum {
	PAN_LAYOUT_TIMELINE = 0,
	PAN_LAYOUT_CALENDAR,
	PAN_LAYOUT_FOLDERS_LINEAR,
	PAN_LAYOUT_FOLDERS_FLOWER,
	PAN_LAYOUT_GRID,
	PAN_LAYOUT_COUNT
} PanLayoutType;

typedef enum {
	PAN_IMAGE_SIZE_THUMB_DOTS = 0,
	PAN_IMAGE_SIZE_THUMB_NONE,
	PAN_IMAGE_SIZE_THUMB_SMALL,
	PAN_IMAGE_SIZE_THUMB_NORMAL,
	PAN_IMAGE_SIZE_THUMB_LARGE,
	PAN_IMAGE_SIZE_10,
	PAN_IMAGE_SIZE_25,
	PAN_IMAGE_SIZE_33,
	PAN_IMAGE_SIZE_50,
	PAN_IMAGE_SIZE_100,
	PAN_IMAGE_SIZE_COUNT
} PanImageSize;

typedef enum {
	PAN_ITEM_NONE,
	PAN_ITEM_THUMB,
	PAN_ITEM_BOX,
	PAN_ITEM_TRIANGLE,
	PAN_ITEM_TEXT,
	PAN_ITEM_IMAGE
} PanItemType;

typedef enum {
	PAN_TEXT_ATTR_NONE = 0,
	PAN_TEXT_ATTR_BOLD = 1 << 0,
	PAN_TEXT_ATTR_HEADING = 1 << 1,
	PAN_TEXT_ATTR_MARKUP = 1 << 2
} PanTextAttrType;

typedef enum {
	PAN_BORDER_NONE = 0,
	PAN_BORDER_1 = 1 << 0,
	PAN_BORDER_2 = 1 << 1,
	PAN_BORDER_3 = 1 << 2,
	PAN_BORDER_4 = 1 << 3
} PanBorderType;

#define PAN_BORDER_TOP		PAN_BORDER_1
#define PAN_BORDER_RIGHT		PAN_BORDER_2
#define PAN_BORDER_BOTTOM	PAN_BORDER_3
#define PAN_BORDER_LEFT		PAN_BORDER_4


typedef struct _PanItem PanItem;
struct _PanItem {
	PanItemType type;
	gint x;
	gint y;
	gint width;
	gint height;
	gchar *key;

	FileData *fd;

	GdkPixbuf *pixbuf;
	gint refcount;

	gchar *text;
	PanTextAttrType text_attr;

	guint8 color_r;
	guint8 color_g;
	guint8 color_b;
	guint8 color_a;

	guint8 color2_r;
	guint8 color2_g;
	guint8 color2_b;
	guint8 color2_a;
	gint border;

	gpointer data;

	gboolean queued;
};

typedef struct _PanViewSearchUi PanViewSearchUi;
struct _PanViewSearchUi
{
	GtkWidget *search_box;
	GtkWidget *search_entry;
	GtkWidget *search_label;
	GtkWidget *search_button;
	GtkWidget *search_button_arrow;
};

/**
 * @typedef _PanViewFilterUi
 * Defined in pan-view-filter.h
 */
typedef struct _PanViewFilterUi PanViewFilterUi;

typedef struct _PanWindow PanWindow;
struct _PanWindow
{
	GtkWidget *window;
	ImageWindow *imd;
	ImageWindow *imd_normal;
	FullScreenData *fs;

	GtkWidget *path_entry;

	GtkWidget *label_message;
	GtkWidget *label_zoom;

	PanViewSearchUi *search_ui;
	PanViewFilterUi *filter_ui;

	GtkWidget *date_button;

	GtkWidget *scrollbar_h;
	GtkWidget *scrollbar_v;

	FileData *dir_fd;
	PanLayoutType layout;
	PanImageSize size;
	gint thumb_size;
	gint thumb_gap;
	gint image_size;
	gboolean exif_date_enable;

	gint info_image_size;
	gboolean info_includes_exif;

	gboolean ignore_symlinks;

	GList *list;
	GList *list_static;
	GList *list_grid;

	GList *cache_list;
	GList *cache_todo;
	gint cache_count;
	gint cache_total;
	gint cache_tick;
	CacheLoader *cache_cl;

	ImageLoader *il;
	ThumbLoader *tl;
	PanItem *queue_pi;
	GList *queue;

	PanItem *click_pi;
	PanItem *search_pi;

	gint idle_id;
};

typedef struct _PanGrid PanGrid;
struct _PanGrid {
	gint x;
	gint y;
	gint w;
	gint h;
	GList *list;
};

typedef struct _PanCacheData PanCacheData;
struct _PanCacheData {
	FileData *fd;
	CacheData *cd;
};

#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
