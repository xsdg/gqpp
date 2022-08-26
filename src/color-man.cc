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

#include "main.h"
#include "color-man.h"

#include "image.h"
#include "ui_fileops.h"


#ifdef HAVE_LCMS
/*** color support enabled ***/

#ifdef HAVE_LCMS2
#include <lcms2.h>
#else
#include <lcms.h>
#endif


typedef struct _ColorManCache ColorManCache;
struct _ColorManCache {
	cmsHPROFILE   profile_in;
	cmsHPROFILE   profile_out;
	cmsHTRANSFORM transform;

	ColorManProfileType profile_in_type;
	gchar *profile_in_file;

	ColorManProfileType profile_out_type;
	gchar *profile_out_file;

	gboolean has_alpha;

	gint refcount;
};

/* pixels to transform per idle call */
#define COLOR_MAN_CHUNK_SIZE 81900


static void color_man_lib_init(void)
{
	static gboolean init_done = FALSE;

	if (init_done) return;
	init_done = TRUE;

#ifndef HAVE_LCMS2
	cmsErrorAction(LCMS_ERROR_IGNORE);
#endif
}

static cmsHPROFILE color_man_create_adobe_comp(void)
{
	/* ClayRGB1998 is AdobeRGB compatible */
#include "ClayRGB1998_icc.h"
	return cmsOpenProfileFromMem(ClayRGB1998_icc, ClayRGB1998_icc_len);
}

/*
 *-------------------------------------------------------------------
 * color transform cache
 *-------------------------------------------------------------------
 */

static GList *cm_cache_list = NULL;


static void color_man_cache_ref(ColorManCache *cc)
{
	if (!cc) return;

	cc->refcount++;
}

static void color_man_cache_unref(ColorManCache *cc)
{
	if (!cc) return;

	cc->refcount--;
	if (cc->refcount < 1)
		{
		if (cc->transform) cmsDeleteTransform(cc->transform);
		if (cc->profile_in) cmsCloseProfile(cc->profile_in);
		if (cc->profile_out) cmsCloseProfile(cc->profile_out);

		g_free(cc->profile_in_file);
		g_free(cc->profile_out_file);

		g_free(cc);
		}
}

static cmsHPROFILE color_man_cache_load_profile(ColorManProfileType type, const gchar *file,
						guchar *data, guint data_len)
{
	cmsHPROFILE profile = NULL;

	switch (type)
		{
		case COLOR_PROFILE_FILE:
			if (file)
				{
				gchar *pathl;

				pathl = path_from_utf8(file);
				profile = cmsOpenProfileFromFile(pathl, "r");
				g_free(pathl);
				}
			break;
		case COLOR_PROFILE_SRGB:
			profile = cmsCreate_sRGBProfile();
			break;
		case COLOR_PROFILE_ADOBERGB:
			profile = color_man_create_adobe_comp();
			break;
		case COLOR_PROFILE_MEM:
			if (data)
				{
				profile = cmsOpenProfileFromMem(data, data_len);
				}
			break;
		case COLOR_PROFILE_NONE:
		default:
			break;
		}

	return profile;
}

static ColorManCache *color_man_cache_new(ColorManProfileType in_type, const gchar *in_file,
					  guchar *in_data, guint in_data_len,
					  ColorManProfileType out_type, const gchar *out_file,
					  guchar *out_data, guint out_data_len,
					  gboolean has_alpha)
{
	ColorManCache *cc;

	color_man_lib_init();

	cc = g_new0(ColorManCache, 1);
	cc->refcount = 1;

	cc->profile_in_type = in_type;
	cc->profile_in_file = g_strdup(in_file);

	cc->profile_out_type = out_type;
	cc->profile_out_file = g_strdup(out_file);

	cc->has_alpha = has_alpha;

	cc->profile_in = color_man_cache_load_profile(cc->profile_in_type, cc->profile_in_file,
						      in_data, in_data_len);
	cc->profile_out = color_man_cache_load_profile(cc->profile_out_type, cc->profile_out_file,
						       out_data, out_data_len);

	if (!cc->profile_in || !cc->profile_out)
		{
		DEBUG_1("failed to load color profile for %s: %d %s",
				  (!cc->profile_in) ? "input" : "screen",
				  (!cc->profile_in) ? cc->profile_in_type : cc->profile_out_type,
				  (!cc->profile_in) ? cc->profile_in_file : cc->profile_out_file);

		color_man_cache_unref(cc);
		return NULL;
		}

	cc->transform = cmsCreateTransform(cc->profile_in,
					   (has_alpha) ? TYPE_RGBA_8 : TYPE_RGB_8,
					   cc->profile_out,
					   (has_alpha) ? TYPE_RGBA_8 : TYPE_RGB_8,
					   options->color_profile.render_intent, 0);

	if (!cc->transform)
		{
		DEBUG_1("failed to create color profile transform");

		color_man_cache_unref(cc);
		return NULL;
		}

	if (cc->profile_in_type != COLOR_PROFILE_MEM && cc->profile_out_type != COLOR_PROFILE_MEM )
		{
		cm_cache_list = g_list_append(cm_cache_list, cc);
		color_man_cache_ref(cc);
		}

	return cc;
}

static void color_man_cache_free(ColorManCache *cc)
{
	if (!cc) return;

	cm_cache_list = g_list_remove(cm_cache_list, cc);
	color_man_cache_unref(cc);
}

static void color_man_cache_reset(void)
{
	while (cm_cache_list)
		{
		ColorManCache *cc;

		cc = cm_cache_list->data;
		color_man_cache_free(cc);
		}
}

static ColorManCache *color_man_cache_find(ColorManProfileType in_type, const gchar *in_file,
					   ColorManProfileType out_type, const gchar *out_file,
					   gboolean has_alpha)
{
	GList *work;

	work = cm_cache_list;
	while (work)
		{
		ColorManCache *cc;
		gboolean match = FALSE;

		cc = work->data;
		work = work->next;

		if (cc->profile_in_type == in_type &&
		    cc->profile_out_type == out_type &&
		    cc->has_alpha == has_alpha)
			{
			match = TRUE;
			}

		if (match && cc->profile_in_type == COLOR_PROFILE_FILE)
			{
			match = (cc->profile_in_file && in_file &&
				 strcmp(cc->profile_in_file, in_file) == 0);
			}
		if (match && cc->profile_out_type == COLOR_PROFILE_FILE)
			{
			match = (cc->profile_out_file && out_file &&
				 strcmp(cc->profile_out_file, out_file) == 0);
			}

		if (match) return cc;
		}

	return NULL;
}

static ColorManCache *color_man_cache_get(ColorManProfileType in_type, const gchar *in_file,
					  guchar *in_data, guint in_data_len,
					  ColorManProfileType out_type, const gchar *out_file,
					  guchar *out_data, guint out_data_len,
					  gboolean has_alpha)
{
	ColorManCache *cc;

	cc = color_man_cache_find(in_type, in_file, out_type, out_file, has_alpha);
	if (cc)
		{
		color_man_cache_ref(cc);
		return cc;
		}

	return color_man_cache_new(in_type, in_file, in_data, in_data_len,
				   out_type, out_file, out_data, out_data_len, has_alpha);
}


/*
 *-------------------------------------------------------------------
 * color manager
 *-------------------------------------------------------------------
 */

static void color_man_done(ColorMan *cm, ColorManReturnType type)
{
	if (cm->func_done)
		{
		cm->func_done(cm, type, cm->func_done_data);
		}
}

void color_man_correct_region(ColorMan *cm, GdkPixbuf *pixbuf, gint x, gint y, gint w, gint h)
{
	ColorManCache *cc;
	guchar *pix;
	gint rs;
	gint i;
	gint pixbuf_width, pixbuf_height;


	pixbuf_width = gdk_pixbuf_get_width(pixbuf);
	pixbuf_height = gdk_pixbuf_get_height(pixbuf);

	cc = cm->profile;

	pix = gdk_pixbuf_get_pixels(pixbuf);
	rs = gdk_pixbuf_get_rowstride(pixbuf);

	/** @FIXME: x,y expected to be = 0. Maybe this is not the right place for scaling */
	w = w * scale_factor();
	h = h * scale_factor();

	w = MIN(w, pixbuf_width - x);
	h = MIN(h, pixbuf_height - y);

	pix += x * ((cc->has_alpha) ? 4 : 3);
	for (i = 0; i < h; i++)
		{
		guchar *pbuf;

		pbuf = pix + ((y + i) * rs);

		cmsDoTransform(cc->transform, pbuf, pbuf, w);
		}

}

static gboolean color_man_idle_cb(gpointer data)
{
	ColorMan *cm = data;
	gint width, height;
	gint rh;

	if (!cm->pixbuf) return FALSE;

	if (cm->imd &&
	    cm->pixbuf != image_get_pixbuf(cm->imd))
		{
		cm->idle_id = 0;
		color_man_done(cm, COLOR_RETURN_IMAGE_CHANGED);
		return FALSE;
		}

	width = gdk_pixbuf_get_width(cm->pixbuf);
	height = gdk_pixbuf_get_height(cm->pixbuf);

	if (cm->row > height)
		{
		if (!cm->incremental_sync && cm->imd)
			{
			image_area_changed(cm->imd, 0, 0, width, height);
			}

		cm->idle_id = 0;
		color_man_done(cm, COLOR_RETURN_SUCCESS);
		return FALSE;
		}

	rh = COLOR_MAN_CHUNK_SIZE / width + 1;
	color_man_correct_region(cm, cm->pixbuf, 0, cm->row, width, rh);
	if (cm->incremental_sync && cm->imd) image_area_changed(cm->imd, 0, cm->row, width, rh);
	cm->row += rh;

	return TRUE;
}

static ColorMan *color_man_new_real(ImageWindow *imd, GdkPixbuf *pixbuf,
				    ColorManProfileType input_type, const gchar *input_file,
				    guchar *input_data, guint input_data_len,
				    ColorManProfileType screen_type, const gchar *screen_file,
				    guchar *screen_data, guint screen_data_len)
{
	ColorMan *cm;
	gboolean has_alpha;

	if (imd) pixbuf = image_get_pixbuf(imd);

	cm = g_new0(ColorMan, 1);
	cm->imd = imd;
	cm->pixbuf = pixbuf;
	if (cm->pixbuf) g_object_ref(cm->pixbuf);

	has_alpha = pixbuf ? gdk_pixbuf_get_has_alpha(pixbuf) : FALSE;

	cm->profile = color_man_cache_get(input_type, input_file, input_data, input_data_len,
					  screen_type, screen_file, screen_data, screen_data_len, has_alpha);
	if (!cm->profile)
		{
		color_man_free(cm);
		return NULL;
		}

	return cm;
}

ColorMan *color_man_new(ImageWindow *imd, GdkPixbuf *pixbuf,
			ColorManProfileType input_type, const gchar *input_file,
			ColorManProfileType screen_type, const gchar *screen_file,
			guchar *screen_data, guint screen_data_len)
{
	return color_man_new_real(imd, pixbuf,
				  input_type, input_file, NULL, 0,
				  screen_type, screen_file, screen_data, screen_data_len);
}

void color_man_start_bg(ColorMan *cm, ColorManDoneFunc done_func, gpointer done_data)
{
	cm->func_done = done_func;
	cm->func_done_data = done_data;
	cm->idle_id = g_idle_add(color_man_idle_cb, cm);
}

ColorMan *color_man_new_embedded(ImageWindow *imd, GdkPixbuf *pixbuf,
				 guchar *input_data, guint input_data_len,
				 ColorManProfileType screen_type, const gchar *screen_file,
				 guchar *screen_data, guint screen_data_len)
{
	return color_man_new_real(imd, pixbuf,
				  COLOR_PROFILE_MEM, NULL, input_data, input_data_len,
				  screen_type, screen_file, screen_data, screen_data_len);
}

static gchar *color_man_get_profile_name(ColorManProfileType type, cmsHPROFILE profile)
{
	switch (type)
		{
		case COLOR_PROFILE_SRGB:
			return g_strdup(_("sRGB"));
		case COLOR_PROFILE_ADOBERGB:
			return g_strdup(_("Adobe RGB compatible"));
			break;
		case COLOR_PROFILE_MEM:
		case COLOR_PROFILE_FILE:
			if (profile)
				{
#ifdef HAVE_LCMS2
				char buffer[20];
				buffer[0] = '\0';
				cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", "US", buffer, 20);
				buffer[19] = '\0'; /* Just to be sure */
				return g_strdup(buffer);
#else
				return g_strdup(cmsTakeProductName(profile));
#endif
				}
			return g_strdup(_("Custom profile"));
			break;
		case COLOR_PROFILE_NONE:
		default:
			return g_strdup("");
		}
}

gboolean color_man_get_status(ColorMan *cm, gchar **image_profile, gchar **screen_profile)
{
	ColorManCache *cc;
	if (!cm) return FALSE;

	cc = cm->profile;

	if (image_profile) *image_profile = color_man_get_profile_name(cc->profile_in_type, cc->profile_in);
	if (screen_profile) *screen_profile = color_man_get_profile_name(cc->profile_out_type, cc->profile_out);
	return TRUE;
}

void color_man_free(ColorMan *cm)
{
	if (!cm) return;

	if (cm->idle_id) g_source_remove(cm->idle_id);
	if (cm->pixbuf) g_object_unref(cm->pixbuf);

	color_man_cache_unref(cm->profile);

	g_free(cm);
}

void color_man_update(void)
{
	color_man_cache_reset();
}

#else /* define HAVE_LCMS */
/*** color support not enabled ***/


ColorMan *color_man_new(ImageWindow *UNUSED(imd), GdkPixbuf *UNUSED(pixbuf),
			ColorManProfileType UNUSED(input_type), const gchar *UNUSED(input_file),
			ColorManProfileType UNUSED(screen_type), const gchar *UNUSED(screen_file),
			guchar *UNUSED(screen_data), guint UNUSED(screen_data_len))
{
	/* no op */
	return NULL;
}

ColorMan *color_man_new_embedded(ImageWindow *UNUSED(imd), GdkPixbuf *UNUSED(pixbuf),
				 guchar *UNUSED(input_data), guint UNUSED(input_data_len),
				 ColorManProfileType UNUSED(screen_type), const gchar *UNUSED(screen_file),
				 guchar *UNUSED(screen_data), guint UNUSED(screen_data_len))
{
	/* no op */
	return NULL;
}

void color_man_free(ColorMan *UNUSED(cm))
{
	/* no op */
}

void color_man_update(void)
{
	/* no op */
}

void color_man_correct_region(ColorMan *UNUSED(cm), GdkPixbuf *UNUSED(pixbuf), gint UNUSED(x), gint UNUSED(y), gint UNUSED(w), gint UNUSED(h))
{
	/* no op */
}

void color_man_start_bg(ColorMan *UNUSED(cm), ColorManDoneFunc UNUSED(done_func), gpointer UNUSED(done_data))
{
	/* no op */
}

gboolean color_man_get_status(ColorMan *UNUSED(cm), gchar **UNUSED(image_profile), gchar **UNUSED(screen_profile))
{
	return FALSE;
}

#endif /* define HAVE_LCMS */
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
