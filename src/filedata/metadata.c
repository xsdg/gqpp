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

/*
 *-----------------------------------------------------------------------------
 * file name, extension, sorting, ...
 *-----------------------------------------------------------------------------
 */

/*static*/ void FileData::file_data_set_collate_keys(FileData *fd)
{
	gchar *caseless_name;
	gchar *valid_name;

	valid_name = g_filename_display_name(fd->name);
	caseless_name = g_utf8_casefold(valid_name, -1);

	g_free(fd->collate_key_name);
	g_free(fd->collate_key_name_nocase);

#if GTK_CHECK_VERSION(2, 8, 0)
	if (options->file_sort.natural)
		{
	 	fd->collate_key_name = g_utf8_collate_key_for_filename(fd->name, -1);
	 	fd->collate_key_name_nocase = g_utf8_collate_key_for_filename(caseless_name, -1);
		}
	else
		{
		fd->collate_key_name = g_utf8_collate_key(valid_name, -1);
		fd->collate_key_name_nocase = g_utf8_collate_key(caseless_name, -1);
		}
#else
	fd->collate_key_name = g_utf8_collate_key(valid_name, -1);
	fd->collate_key_name_nocase = g_utf8_collate_key(caseless_name, -1);
#endif

	g_free(valid_name);
	g_free(caseless_name);
}

/*static*/ void FileData::file_data_set_path(FileData *fd, const gchar *path)
{
	g_assert(path /* && *path*/); /* view_dir_tree uses FileData with zero length path */
	g_assert(file_data_pool);

	g_free(fd->path);

	if (fd->original_path)
		{
		g_hash_table_remove(file_data_pool, fd->original_path);
		g_free(fd->original_path);
		}

	g_assert(!g_hash_table_lookup(file_data_pool, path));

	fd->original_path = g_strdup(path);
	g_hash_table_insert(file_data_pool, fd->original_path, fd);

	if (strcmp(path, G_DIR_SEPARATOR_S) == 0)
		{
		fd->path = g_strdup(path);
		fd->name = fd->path;
		fd->extension = fd->name + 1;
		file_data_set_collate_keys(fd);
		return;
		}

	fd->path = g_strdup(path);
	fd->name = filename_from_path(fd->path);

	if (strcmp(fd->name, "..") == 0)
		{
		gchar *dir = remove_level_from_path(path);
		g_free(fd->path);
		fd->path = remove_level_from_path(dir);
		g_free(dir);
		fd->name = "..";
		fd->extension = fd->name + 2;
		file_data_set_collate_keys(fd);
		return;
		}
	else if (strcmp(fd->name, ".") == 0)
		{
		g_free(fd->path);
		fd->path = remove_level_from_path(path);
		fd->name = ".";
		fd->extension = fd->name + 1;
		file_data_set_collate_keys(fd);
		return;
		}

	fd->extension = registered_extension_from_path(fd->path);
	if (fd->extension == NULL)
		{
		fd->extension = fd->name + strlen(fd->name);
		}

	fd->sidecar_priority = fd->sidecar->sidecar_file_priority(fd->extension);
	file_data_set_collate_keys(fd);
}

/*
 *-----------------------------------------------------------------------------
 * create or reuse Filedata
 *-----------------------------------------------------------------------------
 */


void FileData::read_exif_time_data(FileData *file)
{
	if (file->exifdate > 0)
		{
		DEBUG_1("%s set_exif_time_data: Already exists for %s", get_exec_time(), file->path);
		return;
		}

	if (!file->exif)
		{
		exif_read_fd(file);
		}

	if (file->exif)
		{
		gchar *tmp = exif_get_data_as_text(file->exif, "Exif.Photo.DateTimeOriginal");
		DEBUG_2("%s set_exif_time_data: reading %p %s", get_exec_time(), file, file->path);

		if (tmp)
			{
			struct tm time_str;
			uint year, month, day, hour, min, sec;

			sscanf(tmp, "%4d:%2d:%2d %2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec);
			time_str.tm_year  = year - 1900;
			time_str.tm_mon   = month - 1;
			time_str.tm_mday  = day;
			time_str.tm_hour  = hour;
			time_str.tm_min   = min;
			time_str.tm_sec   = sec;
			time_str.tm_isdst = 0;

			file->exifdate = mktime(&time_str);
			g_free(tmp);
			}
		}
}

void FileData::read_exif_time_digitized_data(FileData *file)
{
	if (file->exifdate_digitized > 0)
		{
		DEBUG_1("%s set_exif_time_digitized_data: Already exists for %s", get_exec_time(), file->path);
		return;
		}

	if (!file->exif)
		{
		exif_read_fd(file);
		}

	if (file->exif)
		{
		gchar *tmp = exif_get_data_as_text(file->exif, "Exif.Photo.DateTimeDigitized");
		DEBUG_2("%s set_exif_time_digitized_data: reading %p %s", get_exec_time(), file, file->path);

		if (tmp)
			{
			struct tm time_str;
			uint year, month, day, hour, min, sec;

			sscanf(tmp, "%4d:%2d:%2d %2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec);
			time_str.tm_year  = year - 1900;
			time_str.tm_mon   = month - 1;
			time_str.tm_mday  = day;
			time_str.tm_hour  = hour;
			time_str.tm_min   = min;
			time_str.tm_sec   = sec;
			time_str.tm_isdst = 0;

			file->exifdate_digitized = mktime(&time_str);
			g_free(tmp);
			}
		}
}

void FileData::read_rating_data(FileData *file)
{
	gchar *rating_str;

	rating_str = metadata_read_string(file, RATING_KEY, METADATA_PLAIN);
	if (rating_str)
		{
		file->rating = atoi(rating_str);
		g_free(rating_str);
		}
	else
		{
		file->rating = 0;
		}
}

void FileData::set_exif_time_data(GList *files)
{
	DEBUG_1("%s set_exif_time_data: ...", get_exec_time());

	while (files)
		{
		FileData *file = files->data;

		read_exif_time_data(file);
		files = files->next;
		}
}

void FileData::set_exif_time_digitized_data(GList *files)
{
	DEBUG_1("%s set_exif_time_digitized_data: ...", get_exec_time());

	while (files)
		{
		FileData *file = files->data;

		read_exif_time_digitized_data(file);
		files = files->next;
		}
}

void FileData::set_rating_data(GList *files)
{
	gchar *rating_str;
	DEBUG_1("%s set_rating_data: ...", get_exec_time());

	while (files)
		{
		FileData *file = files->data;
		rating_str = metadata_read_string(file, RATING_KEY, METADATA_PLAIN);
		if (rating_str )
			{
			file->rating = atoi(rating_str);
			g_free(rating_str);
			}
		files = files->next;
		}
}
