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

#include "shortcuts.h"

#include <glib-object.h>
#include <glib.h>

#include "collect.h"
#include "compat.h"
#include "intl.h"
#include "layout.h"
#include "main-defines.h"
#include "ui-bookmark.h"
#include "ui-fileops.h"
#include "ui-misc.h"

namespace
{

struct ShortcutsData
{
	GtkWidget *bookmarks;
	GtkWidget *name_entry;
};

#define SHORTCUTS     "shortcuts"

void shortcuts_bookmark_select(const gchar *path, gpointer data)
{
	auto *lw = static_cast<LayoutWindow *>(data);

	if (file_extension_match(path, GQ_COLLECTION_EXT))
		{
		collection_window_new(path);
		}
	else
		{
		layout_set_path(lw, path);
		}
}

void add_shortcut_cb(GtkFileChooser *chooser, gint response_id, gpointer data)
{
	auto scd = static_cast<ShortcutsData *>(data);

	if (response_id == GTK_RESPONSE_ACCEPT)
		{
		const gchar *entry_text = gtk_entry_get_text(GTK_ENTRY(scd->name_entry));
		gboolean empty_name = (entry_text[0] == '\0');

		g_autoptr(GFile) file = gtk_file_chooser_get_file(chooser);
		g_autofree gchar *selected_dir = g_file_get_path(file);

		bookmark_list_add(scd->bookmarks, empty_name ? filename_from_path(selected_dir) : entry_text, selected_dir);
		}

	gq_gtk_widget_destroy(GTK_WIDGET(chooser));
}

void shortcuts_add_cb(GtkWidget *, gpointer data)
{
	auto scd = static_cast<ShortcutsData *>(data);
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;

	dialog = gtk_file_chooser_dialog_new(_("Add Shortcut - Geeqie"), nullptr, action, _("_Cancel"), GTK_RESPONSE_CANCEL, _("Add"), GTK_RESPONSE_ACCEPT, nullptr);

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), layout_get_path(get_current_layout()));
	gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog), layout_get_path(get_current_layout()), nullptr);

	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_placeholder_text(GTK_ENTRY(entry), _("Optional name..."));
	gtk_widget_set_tooltip_text(entry, _("Optional alias name for the shortcut.\nThis may be amended or added from the Sort Manager pane.\nIf none given, the basename of the folder is used"));
	scd->name_entry = entry;
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER(dialog), entry);

	gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);

	g_signal_connect(dialog, "response", G_CALLBACK(add_shortcut_cb), scd);

	gq_gtk_widget_show_all(GTK_WIDGET(dialog));
}

void shortcuts_destroy(gpointer data)
{
	auto scd = static_cast<ShortcutsData *>(data);

	g_free(scd);
}

} // namespace

GtkWidget *shortcuts_new(LayoutWindow *lw)
{
	if (!lw) return nullptr;

	auto *scd = g_new0(ShortcutsData, 1);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);
	g_object_set_data_full(G_OBJECT(vbox), "shortcuts_data", scd, shortcuts_destroy);

	scd->bookmarks = bookmark_list_new(SHORTCUTS, shortcuts_bookmark_select, lw);
	gq_gtk_box_pack_start(GTK_BOX(vbox), scd->bookmarks, TRUE, TRUE, 0);
	gtk_widget_show(scd->bookmarks);

	GtkWidget *tbar = pref_toolbar_new(vbox);

	pref_toolbar_button(tbar, GQ_ICON_ADD, _("Add"), FALSE,
	                    _("Add Shortcut"),
	                    G_CALLBACK(shortcuts_add_cb), scd);

	gtk_widget_show(vbox);
	return vbox;
}

/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
