/*
 * Copyright (C) 2004 John Ellis
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

#ifndef PIXBUF_UTIL_H
#define PIXBUF_UTIL_H


gboolean pixbuf_to_file_as_png (GdkPixbuf *pixbuf, const gchar *filename);
gboolean pixbuf_to_file_as_jpg(GdkPixbuf *pixbuf, const gchar *filename, gint quality);

void pixbuf_inline_register_stock_icons(void);
gboolean register_theme_icon_as_stock(const gchar *key, const gchar *icon);

GdkPixbuf *pixbuf_inline(const gchar *key);
GdkPixbuf *pixbuf_fallback(FileData *fd, gint requested_width, gint requested_height);

gboolean pixbuf_scale_aspect(gint req_w, gint req_h, gint old_w, gint old_h, gint *new_w, gint *new_h);

#define PIXBUF_INLINE_FOLDER_CLOSED	"folder_closed"
#define PIXBUF_INLINE_FOLDER_LOCKED	"folder_locked"
#define PIXBUF_INLINE_FOLDER_OPEN	"folder_open"
#define PIXBUF_INLINE_FOLDER_UP		"folder_up"
#define PIXBUF_INLINE_SCROLLER		"scroller"
#define PIXBUF_INLINE_BROKEN		"broken"
#define PIXBUF_INLINE_METADATA		"metadata"
#define PIXBUF_INLINE_UNKNOWN		"unknown"
#define PIXBUF_INLINE_VIDEO			"video"
#define PIXBUF_INLINE_COLLECTION	"collection"
#define PIXBUF_INLINE_ICON		"icon"
#define PIXBUF_INLINE_LOGO		"logo"

#define PIXBUF_INLINE_ICON_FLOAT	"icon_float"
#define PIXBUF_INLINE_ICON_THUMB	"icon_thumb"

#define PIXBUF_INLINE_ICON_BOOK		"icon_book"
#define PIXBUF_INLINE_ICON_CONFIG	"icon_config"
#define PIXBUF_INLINE_ICON_TOOLS	"icon_tools"
#define PIXBUF_INLINE_ICON_VIEW		"icon_view"
#define PIXBUF_INLINE_ICON_GUIDELINES	"icon_guidelines"
#define PIXBUF_INLINE_ICON_PANORAMA		"icon_panorama"
#define PIXBUF_INLINE_ICON_MAINTENANCE	"icon_maintenance"
#define PIXBUF_INLINE_ICON_ZOOMFILLHOR	"icon_zoomfillhor"
#define PIXBUF_INLINE_ICON_ZOOMFILLVERT	"icon_zoomfillvert"
#define PIXBUF_INLINE_ICON_HIDETOOLS	"icon_hidetools"
#define PIXBUF_INLINE_ICON_EXIF		"icon_exif"
#define PIXBUF_INLINE_ICON_MARKS	"icon_marks"
#define PIXBUF_INLINE_ICON_INFO		"icon_info"
#define PIXBUF_INLINE_ICON_SORT		"icon_sort"
#define PIXBUF_INLINE_ICON_PDF		"icon_pdf"
#define PIXBUF_INLINE_ICON_DRAW_RECTANGLE	"icon_draw_rectangle"
#define PIXBUF_INLINE_ICON_MOVE		"icon_move"
#define PIXBUF_INLINE_ICON_RENAME	"icon_rename"
#define PIXBUF_INLINE_ICON_SELECT_ALL	"icon_select_all"
#define PIXBUF_INLINE_ICON_SELECT_NONE	"icon_select_none"
#define PIXBUF_INLINE_ICON_SELECT_INVERT	"icon_select_invert"
#define PIXBUF_INLINE_ICON_SELECT_RECTANGLE	"icon_select_rectangle"
#define PIXBUF_INLINE_ICON_FILE_FILTER	"icon_file_filter"
#define PIXBUF_INLINE_ICON_TRASH	"icon_trash"
#define PIXBUF_INLINE_ICON_HEIF	"icon_heic"
#define PIXBUF_INLINE_ICON_GRAYSCALE	"icon_grayscale"
#define PIXBUF_INLINE_ICON_EXPOSURE		"icon_exposure"
#define PIXBUF_INLINE_SPLIT_PANE_SYNC	"icon_split_pane_sync"

#define PIXBUF_INLINE_ICON_CW	"icon_rotate_clockwise"
#define PIXBUF_INLINE_ICON_CCW	"icon_rotate_counter_clockwise"
#define PIXBUF_INLINE_ICON_180	"icon_rotate_180"
#define PIXBUF_INLINE_ICON_MIRROR	"icon_mirror"
#define PIXBUF_INLINE_ICON_FLIP	"icon_flip"
#define PIXBUF_INLINE_ICON_ORIGINAL	"icon_original"

GdkPixbuf *pixbuf_copy_rotate_90(GdkPixbuf *src, gboolean counter_clockwise);
GdkPixbuf *pixbuf_copy_mirror(GdkPixbuf *src, gboolean mirror, gboolean flip);
GdkPixbuf* pixbuf_apply_orientation(GdkPixbuf *pixbuf, gint orientation);

void pixbuf_draw_rect_fill(GdkPixbuf *pb,
			   gint x, gint y, gint w, gint h,
			   gint r, gint g, gint b, gint a);

void pixbuf_draw_rect(GdkPixbuf *pb,
		      gint x, gint y, gint w, gint h,
		      gint r, gint g, gint b, gint a,
		      gint left, gint right, gint top, gint bottom);

void pixbuf_set_rect_fill(GdkPixbuf *pb,
			  gint x, gint y, gint w, gint h,
			  gint r, gint g, gint b, gint a);

void pixbuf_set_rect(GdkPixbuf *pb,
		     gint x, gint y, gint w, gint h,
		     gint r, gint g, gint b, gint a,
		     gint left, gint right, gint top, gint bottom);

void pixbuf_pixel_set(GdkPixbuf *pb, gint x, gint y, gint r, gint g, gint b, gint a);


void pixbuf_draw_layout(GdkPixbuf *pixbuf, PangoLayout *layout, GtkWidget *widget,
			gint x, gint y,
			guint8 r, guint8 g, guint8 b, guint8 a);


void pixbuf_draw_triangle(GdkPixbuf *pb,
			  gint clip_x, gint clip_y, gint clip_w, gint clip_h,
			  gint x1, gint y1, gint x2, gint y2, gint x3, gint y3,
			  guint8 r, guint8 g, guint8 b, guint8 a);

void pixbuf_draw_line(GdkPixbuf *pb,
		      gint clip_x, gint clip_y, gint clip_w, gint clip_h,
		      gint x1, gint y1, gint x2, gint y2,
		      guint8 r, guint8 g, guint8 b, guint8 a);

void pixbuf_draw_shadow(GdkPixbuf *pb,
			gint clip_x, gint clip_y, gint clip_w, gint clip_h,
			gint x, gint y, gint w, gint h, gint border,
			guint8 r, guint8 g, guint8 b, guint8 a);

void pixbuf_desaturate_rect(GdkPixbuf *pb,
			    gint x, gint y, gint w, gint h);
void pixbuf_highlight_overunderexposed(GdkPixbuf *pb,
			    gint x, gint y, gint w, gint h);
void pixbuf_ignore_alpha_rect(GdkPixbuf *pb,
                 gint x, gint y, gint w, gint h);

/* clipping utils */

gboolean util_clip_region(gint x, gint y, gint w, gint h,
		          gint clip_x, gint clip_y, gint clip_w, gint clip_h,
		          gint *rx, gint *ry, gint *rw, gint *rh);
void util_clip_triangle(gint x1, gint y1, gint x2, gint y2, gint x3, gint y3,
			gint *rx, gint *ry, gint *rw, gint *rh);


#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
