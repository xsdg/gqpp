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

#include "main.h"
#include "dnd.h"

#include "collect.h"
#include "image.h"
#include "ui_fileops.h"
#include "pixbuf_util.h"


GtkTargetEntry dnd_file_drag_types[] = {
	{ "text/uri-list", 0, TARGET_URI_LIST },
	{ "text/plain", 0, TARGET_TEXT_PLAIN }
};
gint dnd_file_drag_types_count = 2;

GtkTargetEntry dnd_file_drop_types[] = {
	{ TARGET_APP_COLLECTION_MEMBER_STRING, 0, TARGET_APP_COLLECTION_MEMBER },
	{ "text/uri-list", 0, TARGET_URI_LIST },
	{ "text/plain", 0, TARGET_TEXT_PLAIN },
};
gint dnd_file_drop_types_count = 3;


#define DND_ICON_SIZE (options->dnd_icon_size)


static void pixbuf_draw_border(GdkPixbuf *pixbuf, gint w, gint h)
{
	gboolean alpha;
	gint rs;
	guchar *pix;
	guchar *p;
	gint i;

	alpha = gdk_pixbuf_get_has_alpha(pixbuf);
	rs = gdk_pixbuf_get_rowstride(pixbuf);
	pix = gdk_pixbuf_get_pixels(pixbuf);

	p = pix;
	for (i = 0; i < w; i++)
		{
		*p = 0; p++; *p = 0; p++; *p = 0; p++;
		if (alpha) { *p= 255; p++; }
		}
	for (i = 1; i < h - 1; i++)
		{
		p = pix + rs * i;
		*p = 0; p++; *p = 0; p++; *p = 0; p++;
		if (alpha) *p= 255;

		p = pix + rs * i + (w - 1) * ((alpha == TRUE) ? 4 : 3);
		*p = 0; p++; *p = 0; p++; *p = 0; p++;
		if (alpha) *p= 255;
		}
	p = pix + rs * (h - 1);
	for (i = 0; i < w; i++)
		{
		*p = 0; p++; *p = 0; p++; *p = 0; p++;
		if (alpha) { *p= 255; p++; }
		}
}

/*
static void pixbuf_draw_rect(GdkPixbuf *pixbuf, gint x, gint y, gint w, gint h, guint8 val)
{
	gboolean alpha;
	gint rs;
	guchar *pix;
	guchar *p;
	gint i, j;

	alpha = gdk_pixbuf_get_has_alpha(pixbuf);
	rs = gdk_pixbuf_get_rowstride(pixbuf);
	pix = gdk_pixbuf_get_pixels(pixbuf);

	for (j = 0; j < h; j++)
		{
		p = pix + (rs * (y + j)) + (x * ((alpha) ? 4 : 3));
		for (i = 0; i < w; i++)
			{
			*p = (*p * (256-val)) >> 8; p++;
			*p = (*p * (256-val)) >> 8; p++;
			*p = (*p * (256-val)) >> 8; p++;
			if (alpha) { *p = 255; p++; }
			}
		}
}
*/
void dnd_set_drag_icon(GtkWidget *widget, GdkDragContext *context, GdkPixbuf *pixbuf, gint items)
{
	GdkPixbuf *dest;
	gint w, h;
	gint sw, sh;
	PangoLayout *layout = NULL;
	gint x, y;

	x = y = 0;

	sw = gdk_pixbuf_get_width(pixbuf);
	sh = gdk_pixbuf_get_height(pixbuf);

	if (sw <= DND_ICON_SIZE && sh <= DND_ICON_SIZE)
		{
		w = sw;
		h = sh;
		}
	else if (sw < sh)
		{
		w = sw * DND_ICON_SIZE / sh;
		h = DND_ICON_SIZE;
		}
	else
		{
		w = DND_ICON_SIZE;
		h = sh * DND_ICON_SIZE / sw;
		}

	dest = gdk_pixbuf_scale_simple(pixbuf, w, h, GDK_INTERP_BILINEAR);
	pixbuf_draw_border(dest, w, h);

	if (items > 1)
		{
		gchar *buf;
		gint lw,lh;

		layout = gtk_widget_create_pango_layout(widget, NULL);
		buf = g_strdup_printf("<small> %d </small>", items);
		pango_layout_set_markup(layout, buf, -1);
		g_free(buf);

		pango_layout_get_pixel_size(layout, &lw, &lh);

		x = MAX(0, w - lw);
		y = MAX(0, h - lh);
		lw = CLAMP(lw, 0, w - x - 1);
		lh = CLAMP(lh, 0, h - y - 1);

		pixbuf_draw_rect_fill(dest, x, y, lw, lh, 128, 128, 128, 255);
		}

	if (layout)
		{
		pixbuf_draw_layout(dest, layout, NULL, x+1, y+1, 0, 0, 0, 255);
		pixbuf_draw_layout(dest, layout, NULL, x, y, 255, 255, 255, 255);

		g_object_unref(G_OBJECT(layout));
		}

	gtk_drag_set_icon_pixbuf(context, dest, -8, -6);

	g_object_unref(dest);
}

static void dnd_set_drag_label_end_cb(GtkWidget *widget, GdkDragContext *UNUSED(context), gpointer data)
{
	GtkWidget *window = data;
	g_signal_handlers_disconnect_by_func(widget, (gpointer)dnd_set_drag_label_end_cb, data);
	gtk_widget_destroy(window);
}

void dnd_set_drag_label(GtkWidget *widget, GdkDragContext *context, const gchar *text)
{
	GtkWidget *window;
	GtkWidget *label;

	window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_realize (window);

	label = gtk_label_new(text);
	gtk_container_add(GTK_CONTAINER (window), label);
	gtk_widget_show(label);
	gtk_drag_set_icon_widget(context, window, -15, 10);
	g_signal_connect(G_OBJECT(widget), "drag_end",
			 G_CALLBACK(dnd_set_drag_label_end_cb), window);
}


/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
