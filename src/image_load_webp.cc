/*
 * Copyright (C) 20019 - The Geeqie Team
 *
 * Author: Colin Clark
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

#include "image-load.h"
#include "image_load_webp.h"

#ifdef HAVE_WEBP
#include <webp/decode.h>

typedef struct _ImageLoaderWEBP ImageLoaderWEBP;
struct _ImageLoaderWEBP {
	ImageLoaderBackendCbAreaUpdated area_updated_cb;
	ImageLoaderBackendCbSize size_cb;
	ImageLoaderBackendCbAreaPrepared area_prepared_cb;
	gpointer data;
	GdkPixbuf *pixbuf;
	guint requested_width;
	guint requested_height;
	gboolean abort;
};

static void free_buffer(guchar *pixels, gpointer UNUSED(data))
{
	g_free(pixels);
}

static gboolean image_loader_webp_load(gpointer loader, const guchar *buf, gsize count, GError **UNUSED(error))
{
	ImageLoaderWEBP *ld = (ImageLoaderWEBP *) loader;
	guint8* data;
	gint width, height;
	gboolean res_info;
	WebPBitstreamFeatures features;
	VP8StatusCode status_code;

	res_info = WebPGetInfo(buf, count, &width, &height);
	if (!res_info)
		{
		log_printf("warning: webp reader error\n");
		return FALSE;
		}

	status_code = WebPGetFeatures(buf, count, &features);
	if (status_code != VP8_STATUS_OK)
		{
		log_printf("warning: webp reader error\n");
		return FALSE;
		}

	if (features.has_alpha)
		{
		data = WebPDecodeRGBA(buf, count, &width, &height);
		}
	else
		{
		data = WebPDecodeRGB(buf, count, &width, &height);
		}

	ld->pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, features.has_alpha, 8, width, height, width * (features.has_alpha ? 4 : 3), free_buffer, NULL);

	ld->area_updated_cb(loader, 0, 0, width, height, ld->data);

	return TRUE;
}

static gpointer image_loader_webp_new(ImageLoaderBackendCbAreaUpdated area_updated_cb, ImageLoaderBackendCbSize size_cb, ImageLoaderBackendCbAreaPrepared area_prepared_cb, gpointer data)
{
	ImageLoaderWEBP *loader = g_new0(ImageLoaderWEBP, 1);
	loader->area_updated_cb = area_updated_cb;
	loader->size_cb = size_cb;
	loader->area_prepared_cb = area_prepared_cb;
	loader->data = data;
	return (gpointer) loader;
}

static void image_loader_webp_set_size(gpointer loader, int width, int height)
{
	ImageLoaderWEBP *ld = (ImageLoaderWEBP *) loader;
	ld->requested_width = width;
	ld->requested_height = height;
}

static GdkPixbuf* image_loader_webp_get_pixbuf(gpointer loader)
{
	ImageLoaderWEBP *ld = (ImageLoaderWEBP *) loader;
	return ld->pixbuf;
}

static gchar* image_loader_webp_get_format_name(gpointer UNUSED(loader))
{
	return g_strdup("webp");
}

static gchar** image_loader_webp_get_format_mime_types(gpointer UNUSED(loader))
{
	static gchar *mime[] = {"image/webp", NULL};
	return g_strdupv(mime);
}

static gboolean image_loader_webp_close(gpointer UNUSED(loader), GError **UNUSED(error))
{
	return TRUE;
}

static void image_loader_webp_abort(gpointer loader)
{
	ImageLoaderWEBP *ld = (ImageLoaderWEBP *) loader;
	ld->abort = TRUE;
}

static void image_loader_webp_free(gpointer loader)
{
	ImageLoaderWEBP *ld = (ImageLoaderWEBP *) loader;
	if (ld->pixbuf) g_object_unref(ld->pixbuf);
	g_free(ld);
}

void image_loader_backend_set_webp(ImageLoaderBackend *funcs)
{
	funcs->loader_new = image_loader_webp_new;
	funcs->set_size = image_loader_webp_set_size;
	funcs->load = image_loader_webp_load;
	funcs->write = NULL;
	funcs->get_pixbuf = image_loader_webp_get_pixbuf;
	funcs->close = image_loader_webp_close;
	funcs->abort = image_loader_webp_abort;
	funcs->free = image_loader_webp_free;
	funcs->get_format_name = image_loader_webp_get_format_name;
	funcs->get_format_mime_types = image_loader_webp_get_format_mime_types;
}

#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
