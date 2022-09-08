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

#include "pan-view.h"

#include "main.h"
#include "bar-exif.h"
#include "dnd.h"
#include "editors.h"
#include "exif.h"
#include "fullscreen.h"
#include "history-list.h"
#include "image.h"
#include "img-view.h"
#include "layout-util.h"
#include "menu.h"
#include "metadata.h"
#include "misc.h"
#include "pan-calendar.h"
#include "pan-folder.h"
#include "pan-grid.h"
#include "pan-item.h"
#include "pan-timeline.h"
#include "pan-util.h"
#include "pan-view-filter.h"
#include "pan-view-search.h"
#include "pixbuf-renderer.h"
#include "pixbuf-util.h"
#include "thumb.h"
#include "ui-fileops.h"
#include "ui-menu.h"
#include "ui-misc.h"
#include "ui-tabcomp.h"
#include "ui-utildlg.h"
#include "uri-utils.h"
#include "utilops.h"
#include "window.h"

#include <gdk/gdkkeysyms.h> /* for keyboard values */

#include <math.h>


#define PAN_WINDOW_DEFAULT_WIDTH 720
#define PAN_WINDOW_DEFAULT_HEIGHT 500

#define PAN_TILE_SIZE 512

#define ZOOM_INCREMENT 1.0
#define ZOOM_LABEL_WIDTH 64


#define PAN_PREF_GROUP		"pan_view_options"
#define PAN_PREF_HIDE_WARNING	"hide_performance_warning"
#define PAN_PREF_EXIF_PAN_DATE	"use_exif_date"
#define PAN_PREF_INFO_IMAGE	"info_image_size"
#define PAN_PREF_INFO_EXIF	"info_includes_exif"


static GList *pan_window_list = NULL;


static void pan_layout_update_idle(PanWindow *pw);

static void pan_fullscreen_toggle(PanWindow *pw, gboolean force_off);

static void pan_window_close(PanWindow *pw);

static GtkWidget *pan_popup_menu(PanWindow *pw);

static void pan_window_dnd_init(PanWindow *pw);

/**
 * This array must be kept in sync with the contents of:\n
 * @link pan_window_key_press_cb @endlink \n
 * @link pan_popup_menu @endlink
 *
 * See also @link hard_coded_window_keys @endlink
 **/
hard_coded_window_keys pan_view_window_keys[] = {
	{GDK_CONTROL_MASK, 'C', N_("Copy")},
	{GDK_CONTROL_MASK, 'M', N_("Move")},
	{GDK_CONTROL_MASK, 'R', N_("Rename")},
	{GDK_CONTROL_MASK, 'D', N_("Move to Trash")},
	{GDK_CONTROL_MASK, 'W', N_("Close window")},
	{GDK_CONTROL_MASK, 'F', N_("Display Find search bar")},
	{GDK_CONTROL_MASK, 'G', N_("Start search")},
	{0, GDK_KEY_Escape, N_("Exit fullscreen")},
	{0, GDK_KEY_Escape, N_("Hide Find search bar")},
	{0, GDK_KEY_equal, N_("Zoom in")},
	{0, GDK_KEY_plus, N_("Zoom in")},
	{0, GDK_KEY_minus, N_("Zoom out")},
	{0, GDK_KEY_Z, N_("Zoom 1:1")},
	{0, GDK_KEY_1, N_("Zoom 1:1")},
	{0, GDK_KEY_KP_Divide, N_("Zoom 1:1")},
	{0, '2', N_("Zoom 2:1")},
	{0, '3', N_("Zoom 3:1")},
	{0, '4', N_("Zoom 4:1")},
	{0, '7', N_("Zoom 1:4")},
	{0, '8', N_("Zoom 1:3")},
	{0, '9', N_("Zoom 1:2")},
	{0, 'F', N_("Full screen")},
	{0, 'V', N_("Full screen")},
	{0, GDK_KEY_F11, N_("Full screen")},
	{0, '/', N_("Display Find search bar")},
	{0, GDK_KEY_Left, N_("Scroll left")},
	{0, GDK_KEY_Right, N_("Scroll right")},
	{0, GDK_KEY_Up, N_("Scroll up")},
	{0, GDK_KEY_Down, N_("Scroll down")},
	{GDK_SHIFT_MASK, GDK_KEY_Left, N_("Scroll left faster")},
	{GDK_SHIFT_MASK, GDK_KEY_Right, N_("Scroll right faster")},
	{GDK_SHIFT_MASK, GDK_KEY_Up, N_("Scroll up faster")},
	{GDK_SHIFT_MASK, GDK_KEY_Down, N_("Scroll down faster")},
	{0, GDK_KEY_Page_Up, N_("Scroll display half screen up")},
	{0, GDK_KEY_Page_Down, N_("Scroll display half screen down")},
	{0, GDK_KEY_Home, N_("Scroll display half screen left")},
	{0, GDK_KEY_End, N_("Scroll display half screen right")},
	{0, 0, NULL}
};

/*
 *-----------------------------------------------------------------------------
 * the image/thumb loader queue
 *-----------------------------------------------------------------------------
 */

static gboolean pan_queue_step(PanWindow *pw);


static void pan_queue_thumb_done_cb(ThumbLoader *tl, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	if (pw->queue_pi)
		{
		PanItem *pi;
		gint rc;

		pi = pw->queue_pi;
		pw->queue_pi = NULL;

		pi->queued = FALSE;

		if (pi->pixbuf) g_object_unref(pi->pixbuf);
		pi->pixbuf = thumb_loader_get_pixbuf(tl);

		rc = pi->refcount;
		image_area_changed(pw->imd, pi->x, pi->y, pi->width, pi->height);
		pi->refcount = rc;
		}

	thumb_loader_free(pw->tl);
	pw->tl = NULL;

	while (pan_queue_step(pw));
}

static void pan_queue_image_done_cb(ImageLoader *il, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	GdkPixbuf *rotated = NULL;

	if (pw->queue_pi)
		{
		PanItem *pi;
		gint rc;

		pi = pw->queue_pi;
		pw->queue_pi = NULL;

		pi->queued = FALSE;

		if (pi->pixbuf) g_object_unref(pi->pixbuf);
		pi->pixbuf = image_loader_get_pixbuf(pw->il);

		if (pi->pixbuf && options->image.exif_rotate_enable)
			{
			if (!il->fd->exif_orientation)
				{
				if (g_strcmp0(il->fd->format_name, "heif") != 0)
					{
					il->fd->exif_orientation = metadata_read_int(il->fd, ORIENTATION_KEY, EXIF_ORIENTATION_TOP_LEFT);
					}
				else
					{
					il->fd->exif_orientation = EXIF_ORIENTATION_TOP_LEFT;
					}
				}

			if (il->fd->exif_orientation != EXIF_ORIENTATION_TOP_LEFT)
				{
				rotated = pixbuf_apply_orientation(pi->pixbuf, il->fd->exif_orientation);
				pi->pixbuf = rotated;
				}
			}

		if (pi->pixbuf) g_object_ref(pi->pixbuf);

		if (pi->pixbuf && pw->size != PAN_IMAGE_SIZE_100 &&
		    (gdk_pixbuf_get_width(pi->pixbuf) > pi->width ||
		     gdk_pixbuf_get_height(pi->pixbuf) > pi->height))
			{
			GdkPixbuf *tmp;

			tmp = pi->pixbuf;
			pi->pixbuf = gdk_pixbuf_scale_simple(tmp, pi->width, pi->height,
							     (GdkInterpType)options->image.zoom_quality);
			g_object_unref(tmp);
			}

		rc = pi->refcount;
		image_area_changed(pw->imd, pi->x, pi->y, pi->width, pi->height);
		pi->refcount = rc;
		}

	image_loader_free(pw->il);
	pw->il = NULL;

	while (pan_queue_step(pw));
}

static gboolean pan_queue_step(PanWindow *pw)
{
	PanItem *pi;

	if (!pw->queue) return FALSE;

	pi = pw->queue->data;
	pw->queue = g_list_remove(pw->queue, pi);
	pw->queue_pi = pi;

	if (!pw->queue_pi->fd)
		{
		pw->queue_pi->queued = FALSE;
		pw->queue_pi = NULL;
		return TRUE;
		}

	image_loader_free(pw->il);
	pw->il = NULL;
	thumb_loader_free(pw->tl);
	pw->tl = NULL;

	if (pi->type == PAN_ITEM_IMAGE)
		{
		pw->il = image_loader_new(pi->fd);

		if (pw->size != PAN_IMAGE_SIZE_100)
			{
			image_loader_set_requested_size(pw->il, pi->width, pi->height);
			}

		g_signal_connect(G_OBJECT(pw->il), "error", (GCallback)pan_queue_image_done_cb, pw);
		g_signal_connect(G_OBJECT(pw->il), "done", (GCallback)pan_queue_image_done_cb, pw);

		if (image_loader_start(pw->il)) return FALSE;

		image_loader_free(pw->il);
		pw->il = NULL;
		}
	else if (pi->type == PAN_ITEM_THUMB)
		{
		pw->tl = thumb_loader_new(PAN_THUMB_SIZE, PAN_THUMB_SIZE);

		if (!pw->tl->standard_loader)
			{
			/* The classic loader will recreate a thumbnail any time we
			 * request a different size than what exists. This view will
			 * almost never use the user configured sizes so disable cache.
			 */
			thumb_loader_set_cache(pw->tl, FALSE, FALSE, FALSE);
			}

		thumb_loader_set_callbacks(pw->tl,
					   pan_queue_thumb_done_cb,
					   pan_queue_thumb_done_cb,
					   NULL, pw);

		if (thumb_loader_start(pw->tl, pi->fd)) return FALSE;

		thumb_loader_free(pw->tl);
		pw->tl = NULL;
		}

	pw->queue_pi->queued = FALSE;
	pw->queue_pi = NULL;
	return TRUE;
}

static void pan_queue_add(PanWindow *pw, PanItem *pi)
{
	if (!pi || pi->queued || pi->pixbuf) return;
	if (pw->size <= PAN_IMAGE_SIZE_THUMB_NONE &&
	    (!pi->key || strcmp(pi->key, "info") != 0) )
		{
		return;
		}

	pi->queued = TRUE;
	pw->queue = g_list_prepend(pw->queue, pi);

	if (!pw->tl && !pw->il) while (pan_queue_step(pw));
}


/*
 *-----------------------------------------------------------------------------
 * tile request/dispose handlers
 *-----------------------------------------------------------------------------
 */

static gboolean pan_window_request_tile_cb(PixbufRenderer *pr, gint x, gint y,
				       	   gint width, gint height, GdkPixbuf *pixbuf, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	GList *list;
	GList *work;
	gint i;

	pixbuf_set_rect_fill(pixbuf,
			     0, 0, width, height,
			     PAN_BACKGROUND_COLOR, 255);

	for (i = (x / PAN_GRID_SIZE) * PAN_GRID_SIZE; i < x + width; i += PAN_GRID_SIZE)
		{
		gint rx, ry, rw, rh;

		if (util_clip_region(x, y, width, height,
				     i, y, 1, height,
				     &rx, &ry, &rw, &rh))
			{
			pixbuf_draw_rect_fill(pixbuf,
					      rx - x, ry - y, rw, rh,
					      PAN_GRID_COLOR, PAN_GRID_ALPHA);
			}
		}
	for (i = (y / PAN_GRID_SIZE) * PAN_GRID_SIZE; i < y + height; i += PAN_GRID_SIZE)
		{
		gint rx, ry, rw, rh;

		if (util_clip_region(x, y, width, height,
				     x, i, width, 1,
				     &rx, &ry, &rw, &rh))
			{
			pixbuf_draw_rect_fill(pixbuf,
					      rx - x, ry - y, rw, rh,
					      PAN_GRID_COLOR, PAN_GRID_ALPHA);
			}
		}

	list = pan_layout_intersect(pw, x, y, width, height);
	work = list;
	while (work)
		{
		PanItem *pi;
		gboolean queue = FALSE;

		pi = work->data;
		work = work->next;

		pi->refcount++;

		switch (pi->type)
			{
			case PAN_ITEM_BOX:
				queue = pan_item_box_draw(pw, pi, pixbuf, pr, x, y, width, height);
				break;
			case PAN_ITEM_TRIANGLE:
				queue = pan_item_tri_draw(pw, pi, pixbuf, pr, x, y, width, height);
				break;
			case PAN_ITEM_TEXT:
				queue = pan_item_text_draw(pw, pi, pixbuf, pr, x, y, width, height);
				break;
			case PAN_ITEM_THUMB:
				queue = pan_item_thumb_draw(pw, pi, pixbuf, pr, x, y, width, height);
				break;
			case PAN_ITEM_IMAGE:
				queue = pan_item_image_draw(pw, pi, pixbuf, pr, x, y, width, height);
				break;
			case PAN_ITEM_NONE:
			default:
				break;
			}

		if (queue) pan_queue_add(pw, pi);
		}

	g_list_free(list);

	return TRUE;
}

static void pan_window_dispose_tile_cb(PixbufRenderer *UNUSED(pr), gint x, gint y,
				       gint width, gint height, GdkPixbuf *UNUSED(pixbuf), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	GList *list;
	GList *work;

	list = pan_layout_intersect(pw, x, y, width, height);
	work = list;
	while (work)
		{
		PanItem *pi;

		pi = work->data;
		work = work->next;

		if (pi->refcount > 0)
			{
			pi->refcount--;

			if (pi->refcount == 0)
				{
				if (pi->queued)
					{
					pw->queue = g_list_remove(pw->queue, pi);
					pi->queued = FALSE;
					}
				if (pw->queue_pi == pi) pw->queue_pi = NULL;
				if (pi->pixbuf)
					{
					g_object_unref(pi->pixbuf);
					pi->pixbuf = NULL;
					}
				}
			}
		}

	g_list_free(list);
}


/*
 *-----------------------------------------------------------------------------
 * misc
 *-----------------------------------------------------------------------------
 */

static void pan_window_message(PanWindow *pw, const gchar *text)
{
	GList *work;
	gint count = 0;
	gint64 size = 0;
	gchar *ss;
	gchar *buf;

	if (text)
		{
		gtk_label_set_text(GTK_LABEL(pw->label_message), text);
		return;
		}

	work = pw->list_static;
	if (pw->layout == PAN_LAYOUT_CALENDAR)
		{
		while (work)
			{
			PanItem *pi;

			pi = work->data;
			work = work->next;

			if (pi->fd &&
			    pi->type == PAN_ITEM_BOX &&
			    pi->key && strcmp(pi->key, "dot") == 0)
				{
				size += pi->fd->size;
				count++;
				}
			}
		}
	else
		{
		while (work)
			{
			PanItem *pi;

			pi = work->data;
			work = work->next;

			if (pi->fd &&
			    (pi->type == PAN_ITEM_THUMB || pi->type == PAN_ITEM_IMAGE))
				{
				size += pi->fd->size;
				count++;
				}
			}
		}

	ss = text_from_size_abrev(size);
	buf = g_strdup_printf(_("%d images, %s"), count, ss);
	g_free(ss);
	gtk_label_set_text(GTK_LABEL(pw->label_message), buf);
	g_free(buf);
}

static void pan_warning_folder(const gchar *path, GtkWidget *parent)
{
	gchar *message;

	message = g_strdup_printf(_("The pan view does not support the folder \"%s\"."), path);
	warning_dialog(_("Folder not supported"), message,
		      GTK_STOCK_DIALOG_INFO, parent);
	g_free(message);
}

static void pan_window_zoom_limit(PanWindow *pw)
{
	gdouble min;

	switch (pw->size)
		{
		case PAN_IMAGE_SIZE_THUMB_DOTS:
		case PAN_IMAGE_SIZE_THUMB_NONE:
		case PAN_IMAGE_SIZE_THUMB_SMALL:
		case PAN_IMAGE_SIZE_THUMB_NORMAL:
#if 0
			/* easily requires > 512mb ram when window size > 1024x768 and zoom is <= -8 */
			min = -16.0;
			break;
#endif
		case PAN_IMAGE_SIZE_THUMB_LARGE:
			min = -6.0;
			break;
		case PAN_IMAGE_SIZE_10:
		case PAN_IMAGE_SIZE_25:
			min = -4.0;
			break;
		case PAN_IMAGE_SIZE_33:
		case PAN_IMAGE_SIZE_50:
		case PAN_IMAGE_SIZE_100:
		default:
			min = -2.0;
			break;
		}

	image_zoom_set_limits(pw->imd, min, 32.0);
}


/*
 *-----------------------------------------------------------------------------
 * cache
 *-----------------------------------------------------------------------------
 */

static gint pan_cache_sort_file_cb(gpointer a, gpointer b)
{
	PanCacheData *pca = a;
	PanCacheData *pcb = b;
	return filelist_sort_compare_filedata(pca->fd, pcb->fd);
}
GList *pan_cache_sort(GList *list, SortType method, gboolean ascend)
{
	return filelist_sort_full(list, method, ascend, (GCompareFunc) pan_cache_sort_file_cb);
}


static void pan_cache_free(PanWindow *pw)
{
	GList *work;

	work = pw->cache_list;
	while (work)
		{
		PanCacheData *pc;

		pc = work->data;
		work = work->next;

		cache_sim_data_free(pc->cd);
		file_data_unref(pc->fd);
		g_free(pc);
		}

	g_list_free(pw->cache_list);
	pw->cache_list = NULL;

	filelist_free(pw->cache_todo);
	pw->cache_todo = NULL;

	pw->cache_count = 0;
	pw->cache_total = 0;
	pw->cache_tick = 0;

	cache_loader_free(pw->cache_cl);
	pw->cache_cl = NULL;
}

static void pan_cache_fill(PanWindow *pw, FileData *dir_fd)
{
	GList *list;

	pan_cache_free(pw);

	list = pan_list_tree(dir_fd, SORT_NAME, TRUE, pw->ignore_symlinks);
	pw->cache_todo = g_list_reverse(list);

	pw->cache_total = g_list_length(pw->cache_todo);
}

static void pan_cache_step_done_cb(CacheLoader *cl, gint UNUSED(error), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	if (pw->cache_list)
		{
		PanCacheData *pc;
		pc = pw->cache_list->data;

		if (!pc->cd)
			{
			pc->cd = cl->cd;
			cl->cd = NULL;
			}
		}

	cache_loader_free(cl);
	pw->cache_cl = NULL;

	pan_layout_update_idle(pw);
}

static gboolean pan_cache_step(PanWindow *pw)
{
	FileData *fd;
	PanCacheData *pc;
	CacheDataType load_mask;

	if (!pw->cache_todo) return TRUE;

	fd = pw->cache_todo->data;
	pw->cache_todo = g_list_remove(pw->cache_todo, fd);

	pc = g_new0(PanCacheData, 1);
	pc->fd = file_data_ref(fd);

	pc->cd = NULL;

	pw->cache_list = g_list_prepend(pw->cache_list, pc);

	cache_loader_free(pw->cache_cl);

	load_mask = CACHE_LOADER_NONE;
	if (pw->size > PAN_IMAGE_SIZE_THUMB_LARGE) load_mask |= CACHE_LOADER_DIMENSIONS;
	if (pw->exif_date_enable) load_mask |= CACHE_LOADER_DATE;
	pw->cache_cl = cache_loader_new(pc->fd, load_mask,
					pan_cache_step_done_cb, pw);
	return (pw->cache_cl == NULL);
}

/* This sync date function is optimized for lists with a common sort */
void pan_cache_sync_date(PanWindow *pw, GList *list)
{
	GList *haystack;
	GList *work;

	haystack = g_list_copy(pw->cache_list);

	work = list;
	while (work)
		{
		FileData *fd;
		GList *needle;

		fd = work->data;
		work = work->next;

		needle = haystack;
		while (needle)
			{
			PanCacheData *pc;

			pc = needle->data;
			if (pc->fd == fd)
				{
				if (pc->cd && pc->cd->have_date && pc->cd->date >= 0)
					{
					fd->date = pc->cd->date;
					}

				haystack = g_list_delete_link(haystack, needle);
				needle = NULL;
				}
			else
				{
				needle = needle->next;
				}
			}
		}

	g_list_free(haystack);
}

/*
 *-----------------------------------------------------------------------------
 * item grid
 *-----------------------------------------------------------------------------
 */

static void pan_grid_clear(PanWindow *pw)
{
	GList *work;

	work = pw->list_grid;
	while (work)
		{
		PanGrid *pg;

		pg = work->data;
		work = work->next;

		g_list_free(pg->list);
		g_free(pg);
		}

	g_list_free(pw->list_grid);
	pw->list_grid = NULL;

	pw->list = g_list_concat(pw->list, pw->list_static);
	pw->list_static = NULL;
}

static void pan_grid_build(PanWindow *pw, gint width, gint height, gint grid_size)
{
	GList *work;
	gint col, row;
	gint cw, ch;
	gint l;
	gint i, j;

	pan_grid_clear(pw);

	l = g_list_length(pw->list);

	if (l < 1) return;

	col = (gint)(sqrt((gdouble)l / grid_size) * width / height + 0.999);
	col = CLAMP(col, 1, l / grid_size + 1);
	row = (gint)((gdouble)l / grid_size / col);
	if (row < 1) row = 1;

	/* limit minimum size of grid so that a tile will always fit regardless of position */
	cw = MAX((gint)ceil((gdouble)width / col), PAN_TILE_SIZE * 2);
	ch = MAX((gint)ceil((gdouble)height / row), PAN_TILE_SIZE * 2);

	row = row * 2 - 1;
	col = col * 2 - 1;

	DEBUG_1("intersect speedup grid is %dx%d, based on %d average per grid", col, row, grid_size);

	for (j = 0; j < row; j++)
	    for (i = 0; i < col; i++)
		{
		if ((i + 1) * cw / 2 < width && (j + 1) * ch / 2 < height)
			{
			PanGrid *pg;

			pg = g_new0(PanGrid, 1);
			pg->x = i * cw / 2;
			pg->y = j * ch / 2;
			pg->w = cw;
			pg->h = ch;

			pw->list_grid = g_list_prepend(pw->list_grid, pg);

			DEBUG_1("grid section: %d,%d (%dx%d)", pg->x, pg->y, pg->w, pg->h);
			}
		}

	work = pw->list;
	while (work)
		{
		PanItem *pi;
		GList *grid;

		pi = work->data;
		work = work->next;

		grid = pw->list_grid;
		while (grid)
			{
			PanGrid *pg;
			gint rx, ry, rw, rh;

			pg = grid->data;
			grid = grid->next;

			if (util_clip_region(pi->x, pi->y, pi->width, pi->height,
					     pg->x, pg->y, pg->w, pg->h,
					     &rx, &ry, &rw, &rh))
				{
				pg->list = g_list_prepend(pg->list, pi);
				}
			}
		}

	work = pw->list_grid;
	while (work)
		{
		PanGrid *pg;

		pg = work->data;
		work = work->next;

		pg->list = g_list_reverse(pg->list);
		}

	pw->list_static = pw->list;
	pw->list = NULL;
}


/*
 *-----------------------------------------------------------------------------
 * layout state reset
 *-----------------------------------------------------------------------------
 */

static void pan_window_items_free(PanWindow *pw)
{
	GList *work;

	pan_grid_clear(pw);

	work = pw->list;
	while (work)
		{
		PanItem *pi = work->data;
		work = work->next;

		pan_item_free(pi);
		}

	g_list_free(pw->list);
	pw->list = NULL;

	g_list_free(pw->queue);
	pw->queue = NULL;
	pw->queue_pi = NULL;

	image_loader_free(pw->il);
	pw->il = NULL;

	thumb_loader_free(pw->tl);
	pw->tl = NULL;

	pw->click_pi = NULL;
	pw->search_pi = NULL;
}


/*
 *-----------------------------------------------------------------------------
 * layout generation, queries, sizing
 *-----------------------------------------------------------------------------
 */

static void pan_layout_compute(PanWindow *pw, FileData *dir_fd,
			       gint *width, gint *height,
			       gint *scroll_x, gint *scroll_y)
{
	pan_window_items_free(pw);

	switch (pw->size)
		{
		case PAN_IMAGE_SIZE_THUMB_DOTS:
			pw->thumb_size = PAN_THUMB_SIZE_DOTS;
			pw->thumb_gap = PAN_THUMB_GAP_DOTS;
			break;
		case PAN_IMAGE_SIZE_THUMB_NONE:
			pw->thumb_size = PAN_THUMB_SIZE_NONE;
			pw->thumb_gap = PAN_THUMB_GAP_SMALL;
			break;
		case PAN_IMAGE_SIZE_THUMB_SMALL:
			pw->thumb_size = PAN_THUMB_SIZE_SMALL;
			pw->thumb_gap = PAN_THUMB_GAP_SMALL;
			break;
		case PAN_IMAGE_SIZE_THUMB_NORMAL:
		default:
			pw->thumb_size = PAN_THUMB_SIZE_NORMAL;
			pw->thumb_gap = PAN_THUMB_GAP_NORMAL;
			break;
		case PAN_IMAGE_SIZE_THUMB_LARGE:
			pw->thumb_size = PAN_THUMB_SIZE_LARGE;
			pw->thumb_gap = PAN_THUMB_GAP_LARGE;
			break;
		case PAN_IMAGE_SIZE_10:
			pw->image_size = 10;
			pw->thumb_gap = PAN_THUMB_GAP_NORMAL;
			break;
		case PAN_IMAGE_SIZE_25:
			pw->image_size = 25;
			pw->thumb_gap = PAN_THUMB_GAP_NORMAL;
			break;
		case PAN_IMAGE_SIZE_33:
			pw->image_size = 33;
			pw->thumb_gap = PAN_THUMB_GAP_LARGE;
			break;
		case PAN_IMAGE_SIZE_50:
			pw->image_size = 50;
			pw->thumb_gap = PAN_THUMB_GAP_HUGE;
			break;
		case PAN_IMAGE_SIZE_100:
			pw->image_size = 100;
			pw->thumb_gap = PAN_THUMB_GAP_HUGE;
			break;
		}

	*width = 0;
	*height = 0;
	*scroll_x = 0;
	*scroll_y = 0;

	switch (pw->layout)
		{
		case PAN_LAYOUT_GRID:
		default:
			pan_grid_compute(pw, dir_fd, width, height);
			break;
		case PAN_LAYOUT_FOLDERS_LINEAR:
			pan_folder_tree_compute(pw, dir_fd, width, height);
			break;
		case PAN_LAYOUT_FOLDERS_FLOWER:
			pan_flower_compute(pw, dir_fd, width, height, scroll_x, scroll_y);
			break;
		case PAN_LAYOUT_CALENDAR:
			pan_calendar_compute(pw, dir_fd, width, height);
			break;
		case PAN_LAYOUT_TIMELINE:
			pan_timeline_compute(pw, dir_fd, width, height);
			break;
		}

	pan_cache_free(pw);

	DEBUG_1("computed %d objects", g_list_length(pw->list));
}

static GList *pan_layout_intersect_l(GList *list, GList *item_list,
				     gint x, gint y, gint width, gint height)
{
	GList *work;

	work = item_list;
	while (work)
		{
		PanItem *pi;
		gint rx, ry, rw, rh;

		pi = work->data;
		work = work->next;

		if (util_clip_region(x, y, width, height,
				     pi->x, pi->y, pi->width, pi->height,
				     &rx, &ry, &rw, &rh))
			{
			list = g_list_prepend(list, pi);
			}
		}

	return list;
}

GList *pan_layout_intersect(PanWindow *pw, gint x, gint y, gint width, gint height)
{
	GList *list = NULL;
	GList *grid;
	PanGrid *pg = NULL;

	grid = pw->list_grid;
	while (grid && !pg)
		{
		pg = grid->data;
		grid = grid->next;

		if (x < pg->x || x + width > pg->x + pg->w ||
		    y < pg->y || y + height > pg->y + pg->h)
			{
			pg = NULL;
			}
		}

	list = pan_layout_intersect_l(list, pw->list, x, y, width, height);

	if (pg)
		{
		list = pan_layout_intersect_l(list, pg->list, x, y, width, height);
		}
	else
		{
		list = pan_layout_intersect_l(list, pw->list_static, x, y, width, height);
		}

	return list;
}

void pan_layout_resize(PanWindow *pw)
{
	gint width = 0;
	gint height = 0;
	GList *work;
	PixbufRenderer *pr;

	work = pw->list;
	while (work)
		{
		PanItem *pi;

		pi = work->data;
		work = work->next;

		if (width < pi->x + pi->width) width = pi->x + pi->width;
		if (height < pi->y + pi->height) height = pi->y + pi->height;
		}
	work = pw->list_static;
	while (work)
		{
		PanItem *pi;

		pi = work->data;
		work = work->next;

		if (width < pi->x + pi->width) width = pi->x + pi->width;
		if (height < pi->y + pi->height) height = pi->y + pi->height;
		}

	width += PAN_BOX_BORDER * 2;
	height += PAN_BOX_BORDER * 2;

	pr = PIXBUF_RENDERER(pw->imd->pr);
	if (width < pr->window_width) width = pr->window_width;
	if (height < pr->window_width) height = pr->window_height;

	pixbuf_renderer_set_tiles_size(PIXBUF_RENDERER(pw->imd->pr), width, height);
}

static gint pan_layout_update_idle_cb(gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	gint width;
	gint height;
	gint scroll_x;
	gint scroll_y;

	if (pw->size > PAN_IMAGE_SIZE_THUMB_LARGE ||
	    (pw->exif_date_enable && (pw->layout == PAN_LAYOUT_TIMELINE || pw->layout == PAN_LAYOUT_CALENDAR)))
		{
		if (!pw->cache_list && !pw->cache_todo)
			{
			pan_cache_fill(pw, pw->dir_fd);
			if (pw->cache_todo)
				{
				pan_window_message(pw, _("Reading image data..."));
				return TRUE;
				}
			}
		if (pw->cache_todo)
			{
			pw->cache_count++;
			pw->cache_tick++;
			if (pw->cache_count == pw->cache_total)
				{
				pan_window_message(pw, _("Sorting..."));
				}
			else if (pw->cache_tick > 9)
				{
				gchar *buf;

				buf = g_strdup_printf("%s %d / %d", _("Reading image data..."),
						      pw->cache_count, pw->cache_total);
				pan_window_message(pw, buf);
				g_free(buf);

				pw->cache_tick = 0;
				}

			if (pan_cache_step(pw)) return TRUE;

			pw->idle_id = 0;
			return FALSE;
			}
		}

	pan_layout_compute(pw, pw->dir_fd, &width, &height, &scroll_x, &scroll_y);

	pan_window_zoom_limit(pw);

	if (width > 0 && height > 0)
		{
		gdouble align;

		DEBUG_1("Canvas size is %d x %d", width, height);

		pan_grid_build(pw, width, height, 1000);

		pixbuf_renderer_set_tiles(PIXBUF_RENDERER(pw->imd->pr), width, height,
					  PAN_TILE_SIZE, PAN_TILE_SIZE, 10,
					  pan_window_request_tile_cb,
					  pan_window_dispose_tile_cb, pw, 1.0);

		if (scroll_x == 0 && scroll_y == 0)
			{
			align = 0.0;
			}
		else
			{
			align = 0.5;
			}
		pixbuf_renderer_scroll_to_point(PIXBUF_RENDERER(pw->imd->pr), scroll_x, scroll_y, align, align);
		}

	pan_window_message(pw, NULL);

	pw->idle_id = 0;
	return FALSE;
}

static void pan_layout_update_idle(PanWindow *pw)
{
	if (!pw->idle_id)
		{
		pw->idle_id = g_idle_add(pan_layout_update_idle_cb, pw);
		}
}

void pan_layout_update(PanWindow *pw)
{
	pan_window_message(pw, _("Sorting images..."));
	pan_layout_update_idle(pw);
}

static void pan_layout_set_fd(PanWindow *pw, FileData *dir_fd)
{
	if (!dir_fd) return;

	if (strcmp(dir_fd->path, G_DIR_SEPARATOR_S) == 0)
		{
		pan_warning_folder(dir_fd->path, pw->window);
		return;
		}

	file_data_unref(pw->dir_fd);
	pw->dir_fd = file_data_ref(dir_fd);

	pan_layout_update(pw);
}


/*
 *-----------------------------------------------------------------------------
 * keyboard handlers
 *-----------------------------------------------------------------------------
 */

FileData *pan_menu_click_fd(PanWindow *pw)
{
	if (pw->click_pi && pw->click_pi->fd) return pw->click_pi->fd;
	return NULL;
}

static void pan_window_menu_pos_cb(GtkMenu *menu, gint *x, gint *y, gboolean *UNUSED(push_in), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	gdk_window_get_origin(gtk_widget_get_window(pw->imd->pr), x, y);
	popup_menu_position_clamp(menu, x, y, 0);
}

static gboolean pan_window_key_press_cb(GtkWidget *UNUSED(widget), GdkEventKey *event, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	PixbufRenderer *pr;
	FileData *fd;
	gboolean stop_signal = FALSE;
	GtkWidget *menu;
	GtkWidget *imd_widget;
	gint x = 0;
	gint y = 0;
	gint focused;
	gint on_entry;

	pr = PIXBUF_RENDERER(pw->imd->pr);
	fd = pan_menu_click_fd(pw);

	imd_widget = gtk_container_get_focus_child(GTK_CONTAINER(pw->imd->widget));
	focused = (pw->fs || (imd_widget && gtk_widget_has_focus(imd_widget)));
	on_entry = (gtk_widget_has_focus(pw->path_entry) ||
		    gtk_widget_has_focus(pw->search_ui->search_entry) ||
		    gtk_widget_has_focus(pw->filter_ui->filter_entry));

	if (focused)
		{
		stop_signal = TRUE;
		switch (event->keyval)
			{
			case GDK_KEY_Left: case GDK_KEY_KP_Left:
				x -= 1;
				break;
			case GDK_KEY_Right: case GDK_KEY_KP_Right:
				x += 1;
				break;
			case GDK_KEY_Up: case GDK_KEY_KP_Up:
				y -= 1;
				break;
			case GDK_KEY_Down: case GDK_KEY_KP_Down:
				y += 1;
				break;
			case GDK_KEY_Page_Up: case GDK_KEY_KP_Page_Up:
				pixbuf_renderer_scroll(pr, 0, 0 - pr->vis_height / 2);
				break;
			case GDK_KEY_Page_Down: case GDK_KEY_KP_Page_Down:
				pixbuf_renderer_scroll(pr, 0, pr->vis_height / 2);
				break;
			case GDK_KEY_Home: case GDK_KEY_KP_Home:
				pixbuf_renderer_scroll(pr, 0 - pr->vis_width / 2, 0);
				break;
			case GDK_KEY_End: case GDK_KEY_KP_End:
				pixbuf_renderer_scroll(pr, pr->vis_width / 2, 0);
				break;
			default:
				stop_signal = FALSE;
				break;
			}

		if (x != 0 || y!= 0)
			{
			if (event->state & GDK_SHIFT_MASK)
				{
				x *= 3;
				y *= 3;
				}
			keyboard_scroll_calc(&x, &y, event);
			pixbuf_renderer_scroll(pr, x, y);
			}
		}

	if (stop_signal) return stop_signal;

	if (event->state & GDK_CONTROL_MASK)
		{
		stop_signal = TRUE;
		switch (event->keyval)
			{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '0':
				break;
			case 'C': case 'c':
				if (fd) file_util_copy(fd, NULL, NULL, GTK_WIDGET(pr));
				break;
			case 'M': case 'm':
				if (fd) file_util_move(fd, NULL, NULL, GTK_WIDGET(pr));
				break;
			case 'R': case 'r':
				if (fd) file_util_rename(fd, NULL, GTK_WIDGET(pr));
				break;
			case 'D': case 'd':
				if (fd)
					{
					options->file_ops.safe_delete_enable = TRUE;
					file_util_delete(fd, NULL, GTK_WIDGET(pr));
					}
				break;
			case 'F': case 'f':
				pan_search_toggle_visible(pw, TRUE);
				break;
			case 'G': case 'g':
				pan_search_activate(pw);
				break;
			case 'W': case 'w':
				pan_window_close(pw);
				break;
			default:
				stop_signal = FALSE;
				break;
			}
		}
	else
		{
		stop_signal = TRUE;
		switch (event->keyval)
			{
			case GDK_KEY_Escape:
				if (pw->fs)
					{
					pan_fullscreen_toggle(pw, TRUE);
					}
				else
					{
					pan_search_toggle_visible(pw, FALSE);
					}
				break;
			default:
				stop_signal = FALSE;
				break;
			}

		if (stop_signal) return stop_signal;

		// Don't steal characters from entry boxes.
		if (!on_entry)
			{
			stop_signal = TRUE;
			switch (event->keyval)
				{
				case '+': case '=': case GDK_KEY_KP_Add:
					pixbuf_renderer_zoom_adjust(pr, ZOOM_INCREMENT);
					break;
				case '-': case GDK_KEY_KP_Subtract:
					pixbuf_renderer_zoom_adjust(pr, -ZOOM_INCREMENT);
					break;
				case 'Z': case 'z': case GDK_KEY_KP_Divide: case '1':
					pixbuf_renderer_zoom_set(pr, 1.0);
					break;
				case '2':
					pixbuf_renderer_zoom_set(pr, 2.0);
					break;
				case '3':
					pixbuf_renderer_zoom_set(pr, 3.0);
					break;
				case '4':
					pixbuf_renderer_zoom_set(pr, 4.0);
					break;
				case '7':
					pixbuf_renderer_zoom_set(pr, -4.0);
					break;
				case '8':
					pixbuf_renderer_zoom_set(pr, -3.0);
					break;
				case '9':
					pixbuf_renderer_zoom_set(pr, -2.0);
					break;
				case 'F': case 'f':
				case 'V': case 'v':
				case GDK_KEY_F11:
					pan_fullscreen_toggle(pw, FALSE);
					break;
				case 'I': case 'i':
					break;
				case GDK_KEY_Delete: case GDK_KEY_KP_Delete:
					break;
				case GDK_KEY_Menu:
				case GDK_KEY_F10:
					menu = pan_popup_menu(pw);
					gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
						       pan_window_menu_pos_cb, pw, 0, GDK_CURRENT_TIME);
					break;
				case '/':
					pan_search_toggle_visible(pw, TRUE);
					break;
					stop_signal = FALSE;
					break;
				}
			}
		}
	if (!stop_signal && is_help_key(event))
		{
		help_window_show("GuideOtherWindowsPanView.html");
		stop_signal = TRUE;
		}

	return stop_signal;
}

/*
 *-----------------------------------------------------------------------------
 * info popup
 *-----------------------------------------------------------------------------
 */

static void pan_info_add_exif(PanTextAlignment *ta, FileData *fd)
{
	GList *exif_list;
	gchar *text;
	gchar *title;
	gchar *key;

	if (!fd) return;

	exif_list = bar_pane_exif_list();
	while (exif_list)
		{
		title = exif_list->data;
		exif_list = exif_list->next;
		key = exif_list->data;
		exif_list = exif_list->next;

		text = metadata_read_string(fd, key, METADATA_FORMATTED);
		if (text && text[0] != '\0')
			{
			pan_text_alignment_add(ta, title, text);
			}

		g_free(text);
		}

	string_list_free(exif_list);
}


void pan_info_update(PanWindow *pw, PanItem *pi)
{
	PanTextAlignment *ta;
	PanItem *pbox;
	PanItem *p;
	gchar *buf;
	gint x1, y1, x2, y2, x3, y3;
	gint x, y, w, h;

	if (pw->click_pi == pi) return;
	if (pi && !pi->fd) pi = NULL;

	while ((p = pan_item_find_by_key(pw, PAN_ITEM_NONE, "info"))) pan_item_remove(pw, p);
	pw->click_pi = pi;

	if (!pi) return;

	DEBUG_1("info set to %s", pi->fd->path);

	pbox = pan_item_box_new(pw, NULL, pi->x + pi->width + 4, pi->y, 10, 10,
				PAN_POPUP_BORDER,
				PAN_POPUP_COLOR, PAN_POPUP_ALPHA,
				PAN_POPUP_BORDER_COLOR, PAN_POPUP_ALPHA);
	pan_item_set_key(pbox, "info");

	if (pi->type == PAN_ITEM_THUMB && pi->pixbuf)
		{
		w = gdk_pixbuf_get_width(pi->pixbuf);
		h = gdk_pixbuf_get_height(pi->pixbuf);

		x1 = pi->x + pi->width - (pi->width - w) / 2 - 8;
		y1 = pi->y + (pi->height - h) / 2 + 8;
		}
	else
		{
		x1 = pi->x + pi->width - 8;
		y1 = pi->y + 8;
		}

	x2 = pbox->x + 1;
	y2 = pbox->y + 36;
	x3 = pbox->x + 1;
	y3 = pbox->y + 12;
	util_clip_triangle(x1, y1, x2, y2, x3, y3,
			   &x, &y, &w, &h);

	p = pan_item_tri_new(pw, NULL, x, y, w, h,
			     x1, y1, x2, y2, x3, y3,
			     PAN_POPUP_COLOR, PAN_POPUP_ALPHA);
	pan_item_tri_border(p, PAN_BORDER_1 | PAN_BORDER_3, PAN_POPUP_BORDER_COLOR, PAN_POPUP_ALPHA);
	pan_item_set_key(p, "info");
	pan_item_added(pw, p);

	ta = pan_text_alignment_new(pw, pbox->x + PREF_PAD_BORDER, pbox->y + PREF_PAD_BORDER, "info");

	pan_text_alignment_add(ta, _("Filename:"), pi->fd->name);
	buf = remove_level_from_path(pi->fd->path);
	pan_text_alignment_add(ta, _("Location:"), buf);
	g_free(buf);
	pan_text_alignment_add(ta, _("Date:"), text_from_time(pi->fd->date));
	buf = text_from_size(pi->fd->size);
	pan_text_alignment_add(ta, _("Size:"), buf);
	g_free(buf);

	if (pw->info_includes_exif)
		{
		pan_info_add_exif(ta, pi->fd);
		}

	pan_text_alignment_calc(ta, pbox);
	pan_text_alignment_free(ta);

	pan_item_box_shadow(pbox, PAN_SHADOW_OFFSET * 2, PAN_SHADOW_FADE * 2);
	pan_item_added(pw, pbox);

	if (pw->info_image_size > PAN_IMAGE_SIZE_THUMB_NONE)
		{
		gint iw, ih;
		if (image_load_dimensions(pi->fd, &iw, &ih))
			{
			gint scale = 25;

			switch (pw->info_image_size)
				{
				case PAN_IMAGE_SIZE_10:
					scale = 10;
					break;
				case PAN_IMAGE_SIZE_25:
					scale = 25;
					break;
				case PAN_IMAGE_SIZE_33:
					scale = 33;
					break;
				case PAN_IMAGE_SIZE_50:
					scale = 50;
					break;
				case PAN_IMAGE_SIZE_100:
					scale = 100;
					break;
				}

			iw = MAX(1, iw * scale / 100);
			ih = MAX(1, ih * scale / 100);

			pbox = pan_item_box_new(pw, NULL, pbox->x, pbox->y + pbox->height + 8, 10, 10,
						PAN_POPUP_BORDER,
						PAN_POPUP_COLOR, PAN_POPUP_ALPHA,
						PAN_POPUP_BORDER_COLOR, PAN_POPUP_ALPHA);
			pan_item_set_key(pbox, "info");

			p = pan_item_image_new(pw, file_data_new_group(pi->fd->path),
					       pbox->x + PREF_PAD_BORDER, pbox->y + PREF_PAD_BORDER, iw, ih);
			pan_item_set_key(p, "info");
			pan_item_size_by_item(pbox, p, PREF_PAD_BORDER);

			pan_item_box_shadow(pbox, PAN_SHADOW_OFFSET * 2, PAN_SHADOW_FADE * 2);
			pan_item_added(pw, pbox);
			}
		}

	pan_layout_resize(pw);
}


/*
 *-----------------------------------------------------------------------------
 * main window
 *-----------------------------------------------------------------------------
 */

static void button_cb(PixbufRenderer *pr, GdkEventButton *event, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	PanItem *pi = NULL;
	GtkWidget *menu;
	gint rx, ry;

	rx = ry = 0;
	if (pr->scale)
		{
		rx = (gdouble)(pr->x_scroll + event->x - pr->x_offset) / pr->scale;
		ry = (gdouble)(pr->y_scroll + event->y - pr->y_offset) / pr->scale;
		}

	pi = pan_item_find_by_coord(pw, PAN_ITEM_BOX, rx, ry, "info");
	if (pi && event->button == MOUSE_BUTTON_LEFT)
		{
		pan_info_update(pw, NULL);
		return;
		}

	pi = pan_item_find_by_coord(pw, (pw->size > PAN_IMAGE_SIZE_THUMB_LARGE) ? PAN_ITEM_IMAGE : PAN_ITEM_THUMB,
				    rx, ry, NULL);

	switch (event->button)
		{
		case MOUSE_BUTTON_LEFT:
			pan_info_update(pw, pi);

			if (!pi && pw->layout == PAN_LAYOUT_CALENDAR)
				{
				pi = pan_item_find_by_coord(pw, PAN_ITEM_BOX, rx, ry, "day");
				pan_calendar_update(pw, pi);
				}
			break;
		case MOUSE_BUTTON_MIDDLE:
			break;
		case MOUSE_BUTTON_RIGHT:
			pan_info_update(pw, pi);
			menu = pan_popup_menu(pw);
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, event->time);
			break;
		default:
			break;
		}
}

static void scroll_cb(PixbufRenderer *pr, GdkEventScroll *event, gpointer UNUSED(data))
{
	gint w, h;

	w = pr->vis_width;
	h = pr->vis_height;

	if (!(event->state & GDK_SHIFT_MASK))
		{
		w /= 3;
		h /= 3;
		}

	if (event->state & GDK_CONTROL_MASK)
		{
		switch (event->direction)
			{
			case GDK_SCROLL_UP:
				pixbuf_renderer_zoom_adjust_at_point(pr, ZOOM_INCREMENT,
								     (gint)event->x, (gint)event->y);
				break;
			case GDK_SCROLL_DOWN:
				pixbuf_renderer_zoom_adjust_at_point(pr, -ZOOM_INCREMENT,
								     (gint)event->x, (gint)event->y);
				break;
			default:
				break;
			}
		}
	else
		{
		switch (event->direction)
			{
			case GDK_SCROLL_UP:
				pixbuf_renderer_scroll(pr, 0, -h);
				break;
			case GDK_SCROLL_DOWN:
				pixbuf_renderer_scroll(pr, 0, h);
				break;
			case GDK_SCROLL_LEFT:
				pixbuf_renderer_scroll(pr, -w, 0);
				break;
			case GDK_SCROLL_RIGHT:
				pixbuf_renderer_scroll(pr, w, 0);
				break;
			default:
				break;
			}
		}
}

static void pan_image_set_buttons(PanWindow *pw, ImageWindow *imd)
{
	g_signal_connect(G_OBJECT(imd->pr), "clicked",
			 G_CALLBACK(button_cb), pw);
	g_signal_connect(G_OBJECT(imd->pr), "scroll_event",
			 G_CALLBACK(scroll_cb), pw);
}

static void pan_fullscreen_stop_func(FullScreenData *UNUSED(fs), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pw->fs = NULL;
	pw->imd = pw->imd_normal;
}

static void pan_fullscreen_toggle(PanWindow *pw, gboolean force_off)
{
	if (force_off && !pw->fs) return;

	if (pw->fs)
		{
		fullscreen_stop(pw->fs);
		}
	else
		{
		pw->fs = fullscreen_start(pw->window, pw->imd, pan_fullscreen_stop_func, pw);
		pan_image_set_buttons(pw, pw->fs->imd);
		g_signal_connect(G_OBJECT(pw->fs->window), "key_press_event",
				 G_CALLBACK(pan_window_key_press_cb), pw);

		pw->imd = pw->fs->imd;
		}
}

static void pan_window_image_zoom_cb(PixbufRenderer *UNUSED(pr), gdouble UNUSED(zoom), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	gchar *text;

	text = image_zoom_get_as_text(pw->imd);
	gtk_label_set_text(GTK_LABEL(pw->label_zoom), text);
	g_free(text);
}

static void pan_window_image_scroll_notify_cb(PixbufRenderer *pr, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	GtkAdjustment *adj;
	GdkRectangle rect;
	gint width, height;

	if (pr->scale == 0.0) return;

	pixbuf_renderer_get_visible_rect(pr, &rect);
	pixbuf_renderer_get_image_size(pr, &width, &height);

	adj = gtk_range_get_adjustment(GTK_RANGE(pw->scrollbar_h));
	gtk_adjustment_set_page_size(adj, rect.width);
	gtk_adjustment_set_page_increment(adj, gtk_adjustment_get_page_size(adj) / 2.0);
	gtk_adjustment_set_step_increment(adj, 48.0 / pr->scale);
	gtk_adjustment_set_lower(adj, 0.0);
	gtk_adjustment_set_upper(adj, MAX((gdouble)width, 1.0));
	gtk_adjustment_set_value(adj, (gdouble)rect.x);

	pref_signal_block_data(pw->scrollbar_h, pw);
	gtk_adjustment_changed(adj);
	gtk_adjustment_value_changed(adj);
	pref_signal_unblock_data(pw->scrollbar_h, pw);

	adj = gtk_range_get_adjustment(GTK_RANGE(pw->scrollbar_v));
	gtk_adjustment_set_page_size(adj, rect.height);
	gtk_adjustment_set_page_increment(adj, gtk_adjustment_get_page_size(adj) / 2.0);
	gtk_adjustment_set_step_increment(adj, 48.0 / pr->scale);
	gtk_adjustment_set_lower(adj, 0.0);
	gtk_adjustment_set_upper(adj, MAX((gdouble)height, 1.0));
	gtk_adjustment_set_value(adj, (gdouble)rect.y);

	pref_signal_block_data(pw->scrollbar_v, pw);
	gtk_adjustment_changed(adj);
	gtk_adjustment_value_changed(adj);
	pref_signal_unblock_data(pw->scrollbar_v, pw);
}

static void pan_window_scrollbar_h_value_cb(GtkRange *range, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	PixbufRenderer *pr;
	gint x;

	pr = PIXBUF_RENDERER(pw->imd_normal->pr);

	if (!pr->scale) return;

	x = (gint)gtk_range_get_value(range);

	pixbuf_renderer_scroll_to_point(pr, x, (gint)((gdouble)pr->y_scroll / pr->scale), 0.0, 0.0);
}

static void pan_window_scrollbar_v_value_cb(GtkRange *range, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	PixbufRenderer *pr;
	gint y;

	pr = PIXBUF_RENDERER(pw->imd_normal->pr);

	if (!pr->scale) return;

	y = (gint)gtk_range_get_value(range);

	pixbuf_renderer_scroll_to_point(pr, (gint)((gdouble)pr->x_scroll / pr->scale), y, 0.0, 0.0);
}

static void pan_window_layout_change_cb(GtkWidget *combo, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pw->layout = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	pan_layout_update(pw);
}

static void pan_window_layout_size_cb(GtkWidget *combo, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pw->size = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	pan_layout_update(pw);
}

static void pan_window_entry_activate_cb(const gchar *new_text, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	gchar *path;

	path = remove_trailing_slash(new_text);
	parse_out_relatives(path);

	if (!isdir(path))
		{
		warning_dialog(_("Folder not found"),
			       _("The entered path is not a folder"),
			       GTK_STOCK_DIALOG_WARNING, pw->path_entry);
		}
	else
		{
		FileData *dir_fd = (FileData*)file_data_new_dir(path);
		tab_completion_append_to_history(pw->path_entry, path);

		pan_layout_set_fd(pw, dir_fd);
		file_data_unref(dir_fd);
		}

	g_free(path);
}

static void pan_window_close(PanWindow *pw)
{
	pan_window_list = g_list_remove(pan_window_list, pw);

	pref_list_int_set(PAN_PREF_GROUP, PAN_PREF_EXIF_PAN_DATE, pw->exif_date_enable);
	pref_list_int_set(PAN_PREF_GROUP, PAN_PREF_INFO_IMAGE, pw->info_image_size);
	pref_list_int_set(PAN_PREF_GROUP, PAN_PREF_INFO_EXIF, pw->info_includes_exif);

	if (pw->idle_id)
		{
		g_source_remove(pw->idle_id);
		}

	pan_fullscreen_toggle(pw, TRUE);
	pan_search_ui_destroy(&pw->search_ui);
	pan_filter_ui_destroy(&pw->filter_ui);
	gtk_widget_destroy(pw->window);

	pan_window_items_free(pw);
	pan_cache_free(pw);

	file_data_unref(pw->dir_fd);

	g_free(pw);
}

static gboolean pan_window_delete_cb(GtkWidget *UNUSED(w), GdkEventAny *UNUSED(event), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pan_window_close(pw);
	return TRUE;
}

static void pan_window_new_real(FileData *dir_fd)
{
	PanWindow *pw;
	GtkWidget *vbox;
	GtkWidget *box;
	GtkWidget *combo;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkWidget *table;
	GdkGeometry geometry;

	pw = g_new0(PanWindow, 1);

	pw->dir_fd = file_data_ref(dir_fd);
	pw->layout = PAN_LAYOUT_TIMELINE;
	pw->size = PAN_IMAGE_SIZE_THUMB_NORMAL;
	pw->thumb_size = PAN_THUMB_SIZE_NORMAL;
	pw->thumb_gap = PAN_THUMB_GAP_NORMAL;

	if (!pref_list_int_get(PAN_PREF_GROUP, PAN_PREF_EXIF_PAN_DATE, &pw->exif_date_enable))
		{
		pw->exif_date_enable = FALSE;
		}
	if (!pref_list_int_get(PAN_PREF_GROUP, PAN_PREF_INFO_IMAGE, &pw->info_image_size))
		{
		pw->info_image_size = PAN_IMAGE_SIZE_THUMB_NONE;
		}
	if (!pref_list_int_get(PAN_PREF_GROUP, PAN_PREF_INFO_EXIF, &pw->info_includes_exif))
		{
		pw->info_includes_exif = TRUE;
		}

	pw->ignore_symlinks = TRUE;

	pw->idle_id = 0;

	pw->window = window_new(GTK_WINDOW_TOPLEVEL, "panview", NULL, NULL, _("Pan View"));
	DEBUG_NAME(pw->window);

	geometry.min_width = DEFAULT_MINIMAL_WINDOW_SIZE;
	geometry.min_height = DEFAULT_MINIMAL_WINDOW_SIZE;
	gtk_window_set_geometry_hints(GTK_WINDOW(pw->window), NULL, &geometry, GDK_HINT_MIN_SIZE);

	gtk_window_set_resizable(GTK_WINDOW(pw->window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(pw->window), 0);

	vbox = gtk_vbox_new(FALSE, 0);
	DEBUG_NAME(vbox);
	gtk_container_add(GTK_CONTAINER(pw->window), vbox);
	gtk_widget_show(vbox);

	box = pref_box_new(vbox, FALSE, GTK_ORIENTATION_HORIZONTAL, PREF_PAD_SPACE);

	pref_spacer(box, 0);
	pref_label_new(box, _("Location:"));
	combo = tab_completion_new_with_history(&pw->path_entry, dir_fd->path, "pan_view_path", -1,
						pan_window_entry_activate_cb, pw);
	gtk_box_pack_start(GTK_BOX(box), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);

	combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Timeline"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Calendar"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Folders"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Folders (flower)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Grid"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), pw->layout);
	g_signal_connect(G_OBJECT(combo), "changed",
			 G_CALLBACK(pan_window_layout_change_cb), pw);
	gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 0);
	gtk_widget_show(combo);

	combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Dots"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("No Images"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Small Thumbnails"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Normal Thumbnails"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("Large Thumbnails"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("1:10 (10%)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("1:4 (25%)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("1:3 (33%)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("1:2 (50%)"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), _("1:1 (100%)"));

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), pw->size);
	g_signal_connect(G_OBJECT(combo), "changed",
			 G_CALLBACK(pan_window_layout_size_cb), pw);
	gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 0);
	gtk_widget_show(combo);

	table = pref_table_new(vbox, 2, 2, FALSE, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 2);

	/** @FIXME pan view does not work correctly when renderer-clutter()
	 * is used.
	 */
	gint temp = options->image.use_clutter_renderer;
	options->image.use_clutter_renderer = FALSE;
	pw->imd = image_new(TRUE);
	options->image.use_clutter_renderer = temp;

	pw->imd_normal = pw->imd;

	g_signal_connect(G_OBJECT(pw->imd->pr), "zoom",
			 G_CALLBACK(pan_window_image_zoom_cb), pw);
	g_signal_connect(G_OBJECT(pw->imd->pr), "scroll_notify",
			 G_CALLBACK(pan_window_image_scroll_notify_cb), pw);

	gtk_table_attach(GTK_TABLE(table), pw->imd->widget, 0, 1, 0, 1,
			 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
	gtk_widget_show(GTK_WIDGET(pw->imd->widget));

	pan_window_dnd_init(pw);

	pan_image_set_buttons(pw, pw->imd);

	pw->scrollbar_h = gtk_hscrollbar_new(NULL);
	g_signal_connect(G_OBJECT(pw->scrollbar_h), "value_changed",
			 G_CALLBACK(pan_window_scrollbar_h_value_cb), pw);
	gtk_table_attach(GTK_TABLE(table), pw->scrollbar_h, 0, 1, 1, 2,
			 GTK_FILL | GTK_EXPAND, 0, 0, 0);
	gtk_widget_show(pw->scrollbar_h);

	pw->scrollbar_v = gtk_vscrollbar_new(NULL);
	g_signal_connect(G_OBJECT(pw->scrollbar_v), "value_changed",
			 G_CALLBACK(pan_window_scrollbar_v_value_cb), pw);
	gtk_table_attach(GTK_TABLE(table), pw->scrollbar_v, 1, 2, 0, 1,
			 0, GTK_FILL | GTK_EXPAND, 0, 0);
	gtk_widget_show(pw->scrollbar_v);

	/* find bar */

	pw->search_ui = pan_search_ui_new(pw);
	gtk_box_pack_start(GTK_BOX(vbox), pw->search_ui->search_box, FALSE, FALSE, 2);

    /* filter bar */
    pw->filter_ui = pan_filter_ui_new(pw);
    gtk_box_pack_start(GTK_BOX(vbox), pw->filter_ui->filter_box, FALSE, FALSE, 2);

	/* status bar */

	box = pref_box_new(vbox, FALSE, GTK_ORIENTATION_HORIZONTAL, 0);

	frame = gtk_frame_new(NULL);
	DEBUG_NAME(frame);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_set_size_request(frame, ZOOM_LABEL_WIDTH, -1);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);
	gtk_widget_show(frame);

	hbox = gtk_hbox_new(FALSE, PREF_PAD_SPACE);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	gtk_widget_show(hbox);

	pref_spacer(hbox, 0);
	pw->label_message = pref_label_new(hbox, "");

	frame = gtk_frame_new(NULL);
	DEBUG_NAME(frame);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_widget_set_size_request(frame, ZOOM_LABEL_WIDTH, -1);
	gtk_box_pack_end(GTK_BOX(box), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	pw->label_zoom = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(frame), pw->label_zoom);
	gtk_widget_show(pw->label_zoom);

	// Add the "Find" button to the status bar area.
	gtk_box_pack_end(GTK_BOX(box), pw->search_ui->search_button, FALSE, FALSE, 0);
	gtk_widget_show(pw->search_ui->search_button);

	// Add the "Filter" button to the status bar area.
	gtk_box_pack_end(GTK_BOX(box), pw->filter_ui->filter_button, FALSE, FALSE, 0);
	gtk_widget_show(pw->filter_ui->filter_button);

	g_signal_connect(G_OBJECT(pw->window), "delete_event",
			 G_CALLBACK(pan_window_delete_cb), pw);
	g_signal_connect(G_OBJECT(pw->window), "key_press_event",
			 G_CALLBACK(pan_window_key_press_cb), pw);

	gtk_window_set_default_size(GTK_WINDOW(pw->window), PAN_WINDOW_DEFAULT_WIDTH, PAN_WINDOW_DEFAULT_HEIGHT);

	pan_layout_update(pw);

	gtk_widget_grab_focus(GTK_WIDGET(pw->imd->widget));
	gtk_widget_show(pw->window);

	pan_window_list = g_list_append(pan_window_list, pw);
}

/*
 *-----------------------------------------------------------------------------
 * performance warnings
 *-----------------------------------------------------------------------------
 */

static void pan_warning_ok_cb(GenericDialog *gd, gpointer data)
{
	FileData *dir_fd = (FileData*)data;

	generic_dialog_close(gd);

	pan_window_new_real(dir_fd);
	file_data_unref(dir_fd);
}

static void pan_warning_hide_cb(GtkWidget *button, gpointer UNUSED(data))
{
	gboolean hide_dlg;

	hide_dlg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	pref_list_int_set(PAN_PREF_GROUP, PAN_PREF_HIDE_WARNING, hide_dlg);
}

static gboolean pan_warning(FileData *dir_fd)
{
	GenericDialog *gd;
	GtkWidget *box;
	GtkWidget *group;
	GtkWidget *button;
	GtkWidget *ct_button;
	gboolean hide_dlg;

	if (dir_fd && strcmp(dir_fd->path, G_DIR_SEPARATOR_S) == 0)
		{
		pan_warning_folder(dir_fd->path, NULL);
		return TRUE;
		}

	if (options->thumbnails.enable_caching &&
	    options->thumbnails.spec_standard) return FALSE;

	if (!pref_list_int_get(PAN_PREF_GROUP, PAN_PREF_HIDE_WARNING, &hide_dlg)) hide_dlg = FALSE;
	if (hide_dlg) return FALSE;

	gd = generic_dialog_new(_("Pan View Performance"), "pan_view_warning", NULL, FALSE,
				NULL, NULL);
	gd->data = file_data_ref(dir_fd);
	generic_dialog_add_button(gd, GTK_STOCK_OK, NULL,
				  pan_warning_ok_cb, TRUE);

	box = generic_dialog_add_message(gd, GTK_STOCK_DIALOG_INFO,
					 _("Pan view performance may be poor."),
					 _("To improve the performance of thumbnails in\npan view the following options can be enabled.\n\nNote that both options must be enabled to\nnotice a change in performance."), TRUE);

	group = pref_box_new(box, FALSE, GTK_ORIENTATION_HORIZONTAL, 0);
	pref_spacer(group, PREF_PAD_INDENT);
	group = pref_box_new(group, TRUE, GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);

	ct_button = pref_checkbox_new_int(group, _("Cache thumbnails"),
					  options->thumbnails.enable_caching, &options->thumbnails.enable_caching);
	button = pref_checkbox_new_int(group, _("Use shared thumbnail cache"),
				       options->thumbnails.spec_standard, &options->thumbnails.spec_standard);
	pref_checkbox_link_sensitivity(ct_button, button);

	pref_line(box, 0);

	pref_checkbox_new(box, _("Do not show this dialog again"), hide_dlg,
			  G_CALLBACK(pan_warning_hide_cb), NULL);

	gtk_widget_show(gd->dialog);

	return TRUE;
}


/*
 *-----------------------------------------------------------------------------
 * entry point
 *-----------------------------------------------------------------------------
 */

void pan_window_new(FileData *dir_fd)
{
	if (pan_warning(dir_fd)) return;

	pan_window_new_real(dir_fd);
}


/*
 *-----------------------------------------------------------------------------
 * menus
 *-----------------------------------------------------------------------------
 */

#define INFO_IMAGE_SIZE_KEY "image_size_data"


static void pan_new_window_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd)
		{
		pan_fullscreen_toggle(pw, TRUE);
		view_window_new(fd);
		}
}

static void pan_go_to_original_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	LayoutWindow *lw = NULL;
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	if (!layout_valid(&lw)) return;

	fd = pan_menu_click_fd(pw);
	if (fd)
		{
		layout_set_fd(lw, fd);
		}
}

static void pan_edit_cb(GtkWidget *widget, gpointer data)
{
	PanWindow *pw;
	FileData *fd;
	const gchar *key = (gchar*)data;

	pw = submenu_item_get_data(widget);
	if (!pw) return;

	fd = pan_menu_click_fd(pw);
	if (fd)
		{
		if (!editor_window_flag_set(key))
			{
			pan_fullscreen_toggle(pw, TRUE);
			}
		file_util_start_editor_from_file(key, fd, pw->imd->widget);
		}
}

static void pan_zoom_in_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	image_zoom_adjust(pw->imd, ZOOM_INCREMENT);
}

static void pan_zoom_out_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	image_zoom_adjust(pw->imd, -ZOOM_INCREMENT);
}

static void pan_zoom_1_1_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	image_zoom_set(pw->imd, 1.0);
}

static void pan_copy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd) file_util_copy(fd, NULL, NULL, pw->imd->widget);
}

static void pan_move_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd) file_util_move(fd, NULL, NULL, pw->imd->widget);
}

static void pan_rename_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd) file_util_rename(fd, NULL, pw->imd->widget);
}

static void pan_delete_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd)
		{
		options->file_ops.safe_delete_enable = FALSE;
		file_util_delete(fd, NULL, pw->imd->widget);
		}
}

static void pan_move_to_trash_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd)
		{
		options->file_ops.safe_delete_enable = TRUE;
		file_util_delete(fd, NULL, pw->imd->widget);
		}
}

static void pan_copy_path_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd) file_util_copy_path_to_clipboard(fd, TRUE);
}

static void pan_copy_path_unquoted_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd) file_util_copy_path_to_clipboard(fd, FALSE);
}

static void pan_exif_date_toggle_cb(GtkWidget *widget, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pw->exif_date_enable = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
	pan_layout_update(pw);
}

static void pan_info_toggle_exif_cb(GtkWidget *widget, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pw->info_includes_exif = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
	/** @FIXME sync info now */
}

static void pan_info_toggle_image_cb(GtkWidget *widget, gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pw->info_image_size = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), INFO_IMAGE_SIZE_KEY));
	/** @FIXME sync info now */
}

static void pan_fullscreen_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pan_fullscreen_toggle(pw, FALSE);
}

static void pan_close_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	pan_window_close(pw);
}

static void pan_popup_menu_destroy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	GList *editmenu_fd_list = (GList*)data;

	filelist_free(editmenu_fd_list);
}

static void pan_play_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	start_editor_from_file(options->image_l_click_video_editor, pw->click_pi->fd);
}

static GList *pan_view_get_fd_list(PanWindow *pw)
{
	GList *list = NULL;
	FileData *fd = pan_menu_click_fd(pw);

	if (fd) list = g_list_prepend(filelist_copy(fd->sidecar_files), file_data_ref(fd));

	return list;
}

/**
 * @brief Add file selection list to a collection
 * @param[in] widget 
 * @param[in] data Index to the collection list menu item selected, or -1 for new collection
 * 
 * 
 */
static void pan_pop_menu_collections_cb(GtkWidget *widget, gpointer data)
{
	PanWindow *pw;
	GList *selection_list = NULL;

	pw = submenu_item_get_data(widget);
	selection_list = g_list_append(selection_list, pan_menu_click_fd(pw));
	pop_menu_collections(selection_list, data);

	filelist_free(selection_list);
}

static GtkWidget *pan_popup_menu(PanWindow *pw)
{
	GtkWidget *menu;
	GtkWidget *submenu;
	GtkWidget *item;
	gboolean active, video;
	GList *editmenu_fd_list;
	GtkAccelGroup *accel_group;

	active = (pw->click_pi != NULL);
	video = (active && pw->click_pi->fd && pw->click_pi->fd->format_class == FORMAT_CLASS_VIDEO);

	menu = popup_menu_short_lived();
	accel_group = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);

	g_object_set_data(G_OBJECT(menu), "window_keys", pan_view_window_keys);
	g_object_set_data(G_OBJECT(menu), "accel_group", accel_group);

	menu_item_add_stock_sensitive(menu, _("_Play"), GTK_STOCK_MEDIA_PLAY, video,
			    G_CALLBACK(pan_play_cb), pw);
	menu_item_add_divider(menu);

	menu_item_add_stock(menu, _("Zoom _in"), GTK_STOCK_ZOOM_IN,
			    G_CALLBACK(pan_zoom_in_cb), pw);
	menu_item_add_stock(menu, _("Zoom _out"), GTK_STOCK_ZOOM_OUT,
			    G_CALLBACK(pan_zoom_out_cb), pw);
	menu_item_add_stock(menu, _("Zoom _1:1"), GTK_STOCK_ZOOM_100,
			    G_CALLBACK(pan_zoom_1_1_cb), pw);
	menu_item_add_divider(menu);

	editmenu_fd_list = pan_view_get_fd_list(pw);
	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(pan_popup_menu_destroy_cb), editmenu_fd_list);

	submenu_add_edit(menu, &item, G_CALLBACK(pan_edit_cb), pw, editmenu_fd_list);
	gtk_widget_set_sensitive(item, active);

	menu_item_add_stock_sensitive(menu, _("View in _new window"), GTK_STOCK_NEW, active,
				      G_CALLBACK(pan_new_window_cb), pw);
	menu_item_add_stock(menu, _("Go to original"), GTK_STOCK_FIND,
			G_CALLBACK(pan_go_to_original_cb), pw);

	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu, _("_Copy..."), GTK_STOCK_COPY, active,
				      G_CALLBACK(pan_copy_cb), pw);
	menu_item_add_sensitive(menu, _("_Move..."), active,
				G_CALLBACK(pan_move_cb), pw);
	menu_item_add_sensitive(menu, _("_Rename..."), active,
				G_CALLBACK(pan_rename_cb), pw);
	menu_item_add_sensitive(menu, _("_Copy path to clipboard"), active,
				G_CALLBACK(pan_copy_path_cb), pw);
	menu_item_add_sensitive(menu, _("_Copy path unquoted to clipboard"), active,
				G_CALLBACK(pan_copy_path_unquoted_cb), pw);

	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu,
				options->file_ops.confirm_move_to_trash ? _("Move to Trash...") :
					_("Move to Trash"), PIXBUF_INLINE_ICON_TRASH, active,
						G_CALLBACK(pan_move_to_trash_cb), pw);
	menu_item_add_stock_sensitive(menu,
				options->file_ops.confirm_delete ? _("_Delete...") :
					_("_Delete"), GTK_STOCK_DELETE, active,
						G_CALLBACK(pan_delete_cb), pw);

	menu_item_add_divider(menu);

	submenu = submenu_add_collections(menu, &item,
				G_CALLBACK(pan_pop_menu_collections_cb), pw);
	gtk_widget_set_sensitive(item, TRUE);
	menu_item_add_divider(menu);


	item = menu_item_add_check(menu, _("Sort by E_xif date"), pw->exif_date_enable,
				   G_CALLBACK(pan_exif_date_toggle_cb), pw);
	gtk_widget_set_sensitive(item, (pw->layout == PAN_LAYOUT_TIMELINE || pw->layout == PAN_LAYOUT_CALENDAR));

	menu_item_add_divider(menu);

	menu_item_add_check(menu, _("_Show Exif information"), pw->info_includes_exif,
			    G_CALLBACK(pan_info_toggle_exif_cb), pw);
	item = menu_item_add(menu, _("Show im_age"), NULL, NULL);
	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	item = menu_item_add_check(submenu, _("_None"), (pw->info_image_size == PAN_IMAGE_SIZE_THUMB_NONE),
				   G_CALLBACK(pan_info_toggle_image_cb), pw);
	g_object_set_data(G_OBJECT(item), INFO_IMAGE_SIZE_KEY, GINT_TO_POINTER(PAN_IMAGE_SIZE_THUMB_NONE));

	item = menu_item_add_check(submenu, _("_Full size"), (pw->info_image_size == PAN_IMAGE_SIZE_100),
				   G_CALLBACK(pan_info_toggle_image_cb), pw);
	g_object_set_data(G_OBJECT(item), INFO_IMAGE_SIZE_KEY, GINT_TO_POINTER(PAN_IMAGE_SIZE_100));

	item = menu_item_add_check(submenu, _("1:2 (50%)"), (pw->info_image_size == PAN_IMAGE_SIZE_50),
				   G_CALLBACK(pan_info_toggle_image_cb), pw);
	g_object_set_data(G_OBJECT(item), INFO_IMAGE_SIZE_KEY, GINT_TO_POINTER(PAN_IMAGE_SIZE_50));

	item = menu_item_add_check(submenu, _("1:3 (33%)"), (pw->info_image_size == PAN_IMAGE_SIZE_33),
				   G_CALLBACK(pan_info_toggle_image_cb), pw);
	g_object_set_data(G_OBJECT(item), INFO_IMAGE_SIZE_KEY, GINT_TO_POINTER(PAN_IMAGE_SIZE_33));

	item = menu_item_add_check(submenu, _("1:4 (25%)"), (pw->info_image_size == PAN_IMAGE_SIZE_25),
				   G_CALLBACK(pan_info_toggle_image_cb), pw);
	g_object_set_data(G_OBJECT(item), INFO_IMAGE_SIZE_KEY, GINT_TO_POINTER(PAN_IMAGE_SIZE_25));

	item = menu_item_add_check(submenu, _("1:10 (10%)"), (pw->info_image_size == PAN_IMAGE_SIZE_10),
				   G_CALLBACK(pan_info_toggle_image_cb), pw);
	g_object_set_data(G_OBJECT(item), INFO_IMAGE_SIZE_KEY, GINT_TO_POINTER(PAN_IMAGE_SIZE_10));



	menu_item_add_divider(menu);

	if (pw->fs)
		{
		menu_item_add(menu, _("Exit _full screen"), G_CALLBACK(pan_fullscreen_cb), pw);
		}
	else
		{
		menu_item_add(menu, _("_Full screen"), G_CALLBACK(pan_fullscreen_cb), pw);
		}

	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("C_lose window"), GTK_STOCK_CLOSE, G_CALLBACK(pan_close_cb), pw);

	return menu;
}


/*
 *-----------------------------------------------------------------------------
 * drag and drop
 *-----------------------------------------------------------------------------
 */

static void pan_window_get_dnd_data(GtkWidget *UNUSED(widget), GdkDragContext *context,
				    gint UNUSED(x), gint UNUSED(y),
				    GtkSelectionData *selection_data, guint info,
				    guint UNUSED(time), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;

	if (gtk_drag_get_source_widget(context) == pw->imd->pr) return;

	if (info == TARGET_URI_LIST)
		{
		GList *list;

		list = uri_filelist_from_gtk_selection_data(selection_data);
		if (list && isdir(((FileData *)list->data)->path))
			{
			FileData *fd = list->data;

			pan_layout_set_fd(pw, fd);
			}

		filelist_free(list);
		}
}

static void pan_window_set_dnd_data(GtkWidget *UNUSED(widget), GdkDragContext *UNUSED(context),
				    GtkSelectionData *selection_data, guint UNUSED(info),
				    guint UNUSED(time), gpointer data)
{
	PanWindow *pw = (PanWindow*)data;
	FileData *fd;

	fd = pan_menu_click_fd(pw);
	if (fd)
		{
		GList *list;

		list = g_list_append(NULL, fd);
		uri_selection_data_set_uris_from_filelist(selection_data, list);
		g_list_free(list);
		}
	else
		{
		gtk_selection_data_set(selection_data, gtk_selection_data_get_target(selection_data),
				       8, NULL, 0);
		}
}

static void pan_window_dnd_init(PanWindow *pw)
{
	GtkWidget *widget;

	widget = pw->imd->pr;

	gtk_drag_source_set(widget, GDK_BUTTON2_MASK,
			    dnd_file_drag_types, dnd_file_drag_types_count,
			    GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
	g_signal_connect(G_OBJECT(widget), "drag_data_get",
			 G_CALLBACK(pan_window_set_dnd_data), pw);

	gtk_drag_dest_set(widget,
			  GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
			  dnd_file_drop_types, dnd_file_drop_types_count,
			  GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
	g_signal_connect(G_OBJECT(widget), "drag_data_received",
			 G_CALLBACK(pan_window_get_dnd_data), pw);
}

/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
