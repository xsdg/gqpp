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

#include "collect-dlg.h"

#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "cache.h"
#include "collect-io.h"
#include "collect.h"
#include "compat.h"
#include "intl.h"
#include "main-defines.h"
#include "misc.h"
#include "options.h"
#include "ui-fileops.h"
#include "ui-utildlg.h"
#include "utilops.h"

enum {
	DIALOG_SAVE,
	DIALOG_SAVE_CLOSE,
	DIALOG_LOAD,
	DIALOG_APPEND
};

static void collection_save_confirmed(GenericDialog *gdlg, gboolean overwrite, CollectionData *cd);

static void collection_confirm_ok_cb(GenericDialog *gdlg, gpointer data)
{
	auto cd = static_cast<CollectionData *>(data);

	collection_save_confirmed(gdlg, TRUE, cd);
}

static void collection_confirm_cancel_cb(GenericDialog *gdlg, gpointer)
{
	generic_dialog_close(gdlg);
}

static void collection_save_confirmed(GenericDialog *gdlg, gboolean overwrite, CollectionData *cd)
{
	GenericDialog *gdlg_overwrite;

	if (!overwrite && isfile(cd->collection_path))
		{
		gdlg_overwrite = file_util_gen_dlg(_("Overwrite collection"), "dlg_confirm", nullptr, FALSE, collection_confirm_cancel_cb, cd);
		generic_dialog_add_message(gdlg_overwrite, GQ_ICON_DIALOG_QUESTION, _("Overwrite existing collection?"), cd->name, TRUE);
		generic_dialog_add_button(gdlg_overwrite, GQ_ICON_OK, _("Overwrite"), collection_confirm_ok_cb, TRUE);

		gtk_widget_show(gdlg_overwrite->dialog);

		return;
		}

	if (!collection_save(cd, cd->collection_path))
		{
		g_autofree gchar *buf = g_strdup_printf(_("Failed to save the collection:\n%s"), cd->collection_path);
		file_util_warning_dialog(_("Save Failed"), buf, GQ_ICON_DIALOG_ERROR, GENERIC_DIALOG(gdlg)->dialog);
		}

	collection_unref(cd);
	collection_window_close_by_collection(cd);
}

static void collection_save_cb(GtkFileChooser *chooser, gint response_id, gpointer data)
{
	auto cd = static_cast<CollectionData *>(data);

	if (response_id == GTK_RESPONSE_ACCEPT)
		{
		g_autofree gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));

		cd->collection_path = g_strdup(filename);
		collection_save_confirmed((GenericDialog *)(chooser), FALSE, cd);
		}

	gq_gtk_widget_destroy(GTK_WIDGET(chooser));
}

static void append_collection_cb(GtkFileChooser *chooser, gint response_id, gpointer data)
{
	auto cd = static_cast<CollectionData *>(data);

	if (response_id == GTK_RESPONSE_ACCEPT)
		{
		g_autofree gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));

		if (!collection_load(cd, filename, COLLECTION_LOAD_APPEND))
			{
			return;
			}
		}

	collection_unref(cd);

	gq_gtk_widget_destroy(GTK_WIDGET(chooser));
}

static void preview_file_cb(GtkFileChooser *chooser, gpointer data)
{
	GtkImage *image_widget = GTK_IMAGE(data);
	g_autofree gchar *file_name = gtk_file_chooser_get_filename(chooser);

	if (file_name)
		{
		/* Use a thumbnail file if one exists */
		g_autofree gchar *thumb_file = cache_find_location(CACHE_TYPE_THUMB, file_name);
		if (thumb_file)
			{
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(thumb_file, nullptr);
			if (pixbuf)
				{
				gtk_image_set_from_pixbuf(image_widget, pixbuf);
				}
			else
				{
				gtk_image_set_from_icon_name(image_widget, "image-missing", GTK_ICON_SIZE_DIALOG);
				}

			g_object_unref(pixbuf);
			}
		else
			{
			/* Use the standard pixbuf loader */
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(file_name, nullptr);
			if (pixbuf)
				{
				GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, options->thumbnails.max_width, options->thumbnails.max_height, GDK_INTERP_BILINEAR);
				gtk_image_set_from_pixbuf(image_widget, scaled_pixbuf);

				g_object_unref(pixbuf);
				}
			else
				{
				gtk_image_set_from_icon_name(image_widget, "image-missing", GTK_ICON_SIZE_DIALOG);
				}
			}
		}
	else
		{
		gtk_image_set_from_icon_name(image_widget, "image-missing", GTK_ICON_SIZE_DIALOG);
		}
}

static void collection_save_or_append_dialog(gint type, CollectionData *cd)
{
	GtkFileChooserDialog *dialog;

	if (!cd) return;
	collection_ref(cd);

	if (type == DIALOG_SAVE || type == DIALOG_SAVE_CLOSE)
		{
		GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;

		dialog = GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new(_("Save Collection As - Geeqie"), nullptr, action, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, nullptr));

		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), _("Untitled.gqv"));

		g_signal_connect(dialog, "response", G_CALLBACK(collection_save_cb), cd);
		}
	else
		{
		GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

		dialog = GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new(_("Append Collection - Geeqie"), nullptr, action, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Append"), GTK_RESPONSE_ACCEPT, nullptr));

		g_signal_connect(dialog, "response", G_CALLBACK(append_collection_cb), cd);
		}

	GtkWidget *preview_area = gtk_image_new();
	gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dialog), preview_area);

	g_signal_connect(dialog, "selection-changed", G_CALLBACK(preview_file_cb), preview_area);

	/* Add the default Collection dir to the dialog shortcuts box */
	gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog), get_collections_dir(), nullptr);

	GtkFileFilter *collection_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(collection_filter, _("Geeqie Collection files"));
	gtk_file_filter_add_pattern(collection_filter, "*" GQ_COLLECTION_EXT);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), collection_filter);

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), collection_filter);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), get_collections_dir());

	gq_gtk_widget_show_all(GTK_WIDGET(dialog));
}

void collection_dialog_save_as(CollectionData *cd)
{
	collection_save_or_append_dialog(DIALOG_SAVE, cd);
}

void collection_dialog_save_close(CollectionData *cd)
{
	collection_save_or_append_dialog(DIALOG_SAVE_CLOSE, cd);
}

void collection_dialog_append(CollectionData *cd)
{
	collection_save_or_append_dialog(DIALOG_APPEND, cd);
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
