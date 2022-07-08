/*
 * Copyright (C) 2022 The Geeqie Team
 *
 * Author: Omari Stephens
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
#include "filedata.h"

#include "filefilter.h"
#include "cache.h"
#include "thumb_standard.h"
#include "ui_fileops.h"
#include "metadata.h"
#include "trash.h"
#include "histogram.h"
#include "secure_save.h"

#include "exif.h"
#include "misc.h"

#include <errno.h>
#include <grp.h>

static FileDataGetMarkFunc file_data_get_mark_func[FILEDATA_MARKS_SIZE];
static FileDataSetMarkFunc file_data_set_mark_func[FILEDATA_MARKS_SIZE];
static gpointer file_data_mark_func_data[FILEDATA_MARKS_SIZE];
static GDestroyNotify file_data_destroy_mark_func[FILEDATA_MARKS_SIZE];

gboolean file_data_get_mark(FileData *fd, gint n)
{
	gboolean valid = (fd->valid_marks & (1 << n));

	if (file_data_get_mark_func[n] && !valid)
		{
		guint old = fd->marks;
		gboolean value = (file_data_get_mark_func[n])(fd, n, file_data_mark_func_data[n]);

		if (!value != !(fd->marks & (1 << n)))
			{
			fd->marks = fd->marks ^ (1 << n);
			}

		fd->valid_marks |= (1 << n);
		if (old && !fd->marks) /* keep files with non-zero marks in memory */
			{
			file_data_unref(fd);
			}
		else if (!old && fd->marks)
			{
			file_data_ref(fd);
			}
		}

	return !!(fd->marks & (1 << n));
}

guint file_data_get_marks(FileData *fd)
{
	gint i;
	for (i = 0; i < FILEDATA_MARKS_SIZE; i++) file_data_get_mark(fd, i);
	return fd->marks;
}

void file_data_set_mark(FileData *fd, gint n, gboolean value)
{
	guint old;
	if (!value == !file_data_get_mark(fd, n)) return;

	if (file_data_set_mark_func[n])
		{
		(file_data_set_mark_func[n])(fd, n, value, file_data_mark_func_data[n]);
		}

	old = fd->marks;

	fd->marks = fd->marks ^ (1 << n);

	if (old && !fd->marks) /* keep files with non-zero marks in memory */
		{
		file_data_unref(fd);
		}
	else if (!old && fd->marks)
		{
		file_data_ref(fd);
		}

	file_data_increment_version(fd);
	file_data_send_notification(fd, NOTIFY_MARKS);
}

gboolean file_data_filter_marks(FileData *fd, guint filter)
{
	gint i;
	for (i = 0; i < FILEDATA_MARKS_SIZE; i++) if (filter & (1 << i)) file_data_get_mark(fd, i);
	return ((fd->marks & filter) == filter);
}

GList *file_data_filter_marks_list(GList *list, guint filter)
{
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd = work->data;
		GList *link = work;
		work = work->next;

		if (!file_data_filter_marks(fd, filter))
			{
			list = g_list_remove_link(list, link);
			file_data_unref(fd);
			g_list_free(link);
			}
		}

	return list;
}

gboolean file_data_filter_file_filter(FileData *fd, GRegex *filter)
{
	return g_regex_match(filter, fd->name, 0, NULL);
}

GList *file_data_filter_file_filter_list(GList *list, GRegex *filter)
{
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd = work->data;
		GList *link = work;
		work = work->next;

		if (!file_data_filter_file_filter(fd, filter))
			{
			list = g_list_remove_link(list, link);
			file_data_unref(fd);
			g_list_free(link);
			}
		}

	return list;
}

/*static*/ gboolean file_data_filter_class(FileData *fd, guint filter)
{
	gint i;

	for (i = 0; i < FILE_FORMAT_CLASSES; i++)
		{
		if (filter & (1 << i))
			{
			if ((FileFormatClass)i == filter_file_get_class(fd->path))
				{
				return TRUE;
				}
			}
		}

	return FALSE;
}

GList *file_data_filter_class_list(GList *list, guint filter)
{
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd = work->data;
		GList *link = work;
		work = work->next;

		if (!file_data_filter_class(fd, filter))
			{
			list = g_list_remove_link(list, link);
			file_data_unref(fd);
			g_list_free(link);
			}
		}

	return list;
}

/*static*/ void file_data_notify_mark_func(gpointer key, gpointer value, gpointer user_data)
{
	FileData *fd = value;
	file_data_increment_version(fd);
	file_data_send_notification(fd, NOTIFY_MARKS);
}

gboolean file_data_register_mark_func(gint n, FileDataGetMarkFunc get_mark_func, FileDataSetMarkFunc set_mark_func, gpointer data, GDestroyNotify notify)
{
	if (n < 0 || n >= FILEDATA_MARKS_SIZE) return FALSE;

	if (file_data_destroy_mark_func[n]) (file_data_destroy_mark_func[n])(file_data_mark_func_data[n]);

	file_data_get_mark_func[n] = get_mark_func;
        file_data_set_mark_func[n] = set_mark_func;
        file_data_mark_func_data[n] = data;
        file_data_destroy_mark_func[n] = notify;

	if (get_mark_func && file_data_pool)
		{
		/* this effectively changes all known files */
		g_hash_table_foreach(file_data_pool, file_data_notify_mark_func, NULL);
		}

        return TRUE;
}

void file_data_get_registered_mark_func(gint n, FileDataGetMarkFunc *get_mark_func, FileDataSetMarkFunc *set_mark_func, gpointer *data)
{
	if (get_mark_func) *get_mark_func = file_data_get_mark_func[n];
	if (set_mark_func) *set_mark_func = file_data_set_mark_func[n];
	if (data) *data = file_data_mark_func_data[n];
}

/*
 *-----------------------------------------------------------------------------
 * Saving marks list, clearing marks
 * Uses file_data_pool
 *-----------------------------------------------------------------------------
 */

/*static*/ void marks_get_files(gpointer key, gpointer value, gpointer userdata)
{
	gchar *file_name = key;
	GString *result = userdata;
	FileData *fd;

	if (isfile(file_name))
		{
		fd = value;
		if (fd && fd->marks > 0)
			{
			g_string_append_printf(result, "%s,%i\n", fd->path, fd->marks);
			}
		}
}

gboolean marks_list_load(const gchar *path)
{
	FILE *f;
	gchar s_buf[1024];
	gchar *pathl;
	gchar *file_path;
	gchar *marks_value;

	pathl = path_from_utf8(path);
	f = fopen(pathl, "r");
	g_free(pathl);
	if (!f) return FALSE;

	/* first line must start with Marks comment */
	if (!fgets(s_buf, sizeof(s_buf), f) ||
					strncmp(s_buf, "#Marks", 6) != 0)
		{
		fclose(f);
		return FALSE;
		}

	while (fgets(s_buf, sizeof(s_buf), f))
		{
		if (s_buf[0]=='#') continue;
			file_path = strtok(s_buf, ",");
			marks_value = strtok(NULL, ",");
			if (isfile(file_path))
				{
				FileData *fd = file_data_new_no_grouping(file_path);
				file_data_ref(fd);
				gint n = 0;
				while (n <= 9)
					{
					gint mark_no = 1 << n;
					if (atoi(marks_value) & mark_no)
						{
						file_data_set_mark(fd, n , 1);
						}
					n++;
					}
				}
		}

	fclose(f);
	return TRUE;
}

gboolean marks_list_save(gchar *path, gboolean save)
{
	SecureSaveInfo *ssi;
	gchar *pathl;
	GString  *marks = g_string_new("");

	pathl = path_from_utf8(path);
	ssi = secure_open(pathl);
	g_free(pathl);
	if (!ssi)
		{
		log_printf(_("Error: Unable to write marks lists to: %s\n"), path);
		return FALSE;
		}

	secure_fprintf(ssi, "#Marks lists\n");

	if (save)
		{
		g_hash_table_foreach(file_data_pool, marks_get_files, marks);
		}
	secure_fprintf(ssi, "%s", marks->str);
	g_string_free(marks, FALSE);

	secure_fprintf(ssi, "#end\n");
	return (secure_close(ssi) == 0);
}

/*static*/ void marks_clear(gpointer key, gpointer value, gpointer userdata)
{
	gchar *file_name = key;
	gint mark_no;
	gint n;
	FileData *fd;

	if (isfile(file_name))
		{
		fd = value;
		if (fd && fd->marks > 0)
			{
			n = 0;
			while (n <= 9)
				{
				mark_no = 1 << n;
				if (fd->marks & mark_no)
					{
					file_data_set_mark(fd, n , 0);
					}
				n++;
				}
			}
		}
}

void marks_clear_all()
{
	g_hash_table_foreach(file_data_pool, marks_clear, NULL);
}
