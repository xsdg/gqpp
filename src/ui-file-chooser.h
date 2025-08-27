/*
 * Copyright (C) 2004 John Ellis
 * Copyright (C) 2008 - 2025 The Geeqie Team
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

#ifndef UI_FILE_CHOOSER_H
#define UI_FILE_CHOOSER_H

#include <gtk/gtk.h>

struct FileChooserDialogData {
	GCallback response_callback;
	GtkFileChooserAction action;
	const gchar *accept_text;
	const gchar *entry_text;
	const gchar *entry_tooltip;
	const gchar *filter_description;
	const gchar *history_key;
	const gchar *suggested_name;
	const gchar *title;
	gchar *filename;
	gchar *filter;
	gchar *shortcuts;
	gpointer data;
};

GtkFileChooserDialog *file_chooser_dialog_new(FileChooserDialogData *fcdd);
void file_chooser_dialog_data_free(FileChooserDialogData *fcdd);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(FileChooserDialogData, file_chooser_dialog_data_free)

#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
