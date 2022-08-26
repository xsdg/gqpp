/*
 * Copyright (C) 2008 - 2016 The Geeqie Team
 *
 * Authors: John Ellis, Vladimir Nadvornik, Laurent Monin
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
#include "uri_utils.h"

#include "filedata.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"

void warning_dialog_dnd_uri_error(GList *uri_error_list)
{
	GList *work = uri_error_list;
	guint count = g_list_length(work);
	gchar *msg = g_strdup_printf("Failed to convert %d dropped item(s) to files\n", count);
	if(count < 10)
		{
		while (work)
			{
			gchar *prev = msg;
			msg = g_strdup_printf("%s\n%s", prev, (gchar *)work->data);
			work = work->next;
			g_free(prev);
			}
		}
	warning_dialog(_("Drag and Drop failed"), msg, GTK_STOCK_DIALOG_WARNING, NULL);
	g_free(msg);
}

gchar **uris_from_pathlist(GList *list)
{
	GList *work;
	guint i = 0;
	guint num = g_list_length(list);
	gchar **uris = g_new0(gchar *, num + 1);

	work = list;
	while (work)
		{
		const gchar *path = work->data;
		gchar *local_path = path_from_utf8(path);
		uris[i] = g_filename_to_uri(local_path, NULL, NULL);
		g_free(local_path);

		i++;
		work = work->next;
		}

	uris[i] = NULL;
	return uris;
}

gchar **uris_from_filelist(GList *list)
{
	GList *path_list = filelist_to_path_list(list);
	gchar **ret = uris_from_pathlist(path_list);
	string_list_free(path_list);
	return ret;
}

gboolean uri_selection_data_set_uris_from_filelist(GtkSelectionData *selection_data, GList *list)
{
	gchar **uris = uris_from_filelist(list);
	gboolean ret = gtk_selection_data_set_uris(selection_data, uris);
	if (!ret)
		{
		char *str = g_strjoinv("\r\n", uris);
		ret = gtk_selection_data_set_text(selection_data, str, -1);
		g_free(str);
		}

	g_strfreev(uris);
	return ret;
}

GList *uri_pathlist_from_uris(gchar **uris, GList **uri_error_list)
{
	GList *list = NULL;
	guint i = 0;
	GError *error = NULL;

	while (uris[i])
		{
		gchar *local_path = g_filename_from_uri(uris[i], NULL, &error);
		if (error)
			{
			DEBUG_1("g_filename_from_uri failed on uri \"%s\"", uris[i]);
			DEBUG_1("   error %d: %s", error->code, error->message);
			if (error->code == G_CONVERT_ERROR_BAD_URI)
				{
				GError *retry_error = NULL;
				gchar *escaped = g_uri_escape_string(uris[i], ":/", TRUE);
				local_path = g_filename_from_uri(escaped, NULL, &retry_error);
				if(retry_error)
					{
					DEBUG_1("manually escaped uri \"%s\" also failed g_filename_from_uri", escaped);
					DEBUG_1("   error %d: %s", retry_error->code, retry_error->message);
					g_error_free(retry_error);
					}
				g_free(escaped);
				}
			g_error_free(error);
			error = NULL;
			if (!local_path)
				{
				*uri_error_list = g_list_prepend(*uri_error_list, g_strdup(uris[i]));
				i++;
				continue;
				}
			}
		gchar *path = path_to_utf8(local_path);
		g_free(local_path);
		list = g_list_prepend(list, path);
		i++;
		}

	*uri_error_list = g_list_reverse(*uri_error_list);
	return g_list_reverse(list);
}

GList *uri_filelist_from_uris(gchar **uris, GList **uri_error_list)
{
	GList *path_list = uri_pathlist_from_uris(uris, uri_error_list);
	GList *filelist = filelist_from_path_list(path_list);
	string_list_free(path_list);
	return filelist;
}

GList *uri_filelist_from_gtk_selection_data(GtkSelectionData *selection_data)
{
	GList *errors = NULL;
	gchar **uris = gtk_selection_data_get_uris(selection_data);
	GList *ret = uri_filelist_from_uris(uris, &errors);
	if(errors)
		{
		warning_dialog_dnd_uri_error(errors);
		string_list_free(errors);
		}
	g_strfreev(uris);
	return ret;
}



/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
