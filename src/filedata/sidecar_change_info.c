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

/*static*/ gboolean file_data_sc_add_ci(FileData *fd, FileDataChangeType type)
{
	GList *work;

	if (fd->parent) fd = fd->parent;

	if (fd->change) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		if (sfd->change) return FALSE;
		work = work->next;
		}

	file_data_add_ci(fd, type, NULL, NULL);

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		file_data_add_ci(sfd, type, NULL, NULL);
		work = work->next;
		}

	return TRUE;
}

/*static*/ gboolean file_data_sc_check_ci(FileData *fd, FileDataChangeType type)
{
	GList *work;

	if (fd->parent) fd = fd->parent;

	if (!fd->change || fd->change->type != type) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		if (!sfd->change || sfd->change->type != type) return FALSE;
		work = work->next;
		}

	return TRUE;
}


gboolean file_data_sc_add_ci_copy(FileData *fd, const gchar *dest_path)
{
	if (!file_data_sc_add_ci(fd, FILEDATA_CHANGE_COPY)) return FALSE;
	file_data_sc_update_ci_copy(fd, dest_path);
	return TRUE;
}

gboolean file_data_sc_add_ci_move(FileData *fd, const gchar *dest_path)
{
	if (!file_data_sc_add_ci(fd, FILEDATA_CHANGE_MOVE)) return FALSE;
	file_data_sc_update_ci_move(fd, dest_path);
	return TRUE;
}

gboolean file_data_sc_add_ci_rename(FileData *fd, const gchar *dest_path)
{
	if (!file_data_sc_add_ci(fd, FILEDATA_CHANGE_RENAME)) return FALSE;
	file_data_sc_update_ci_rename(fd, dest_path);
	return TRUE;
}

gboolean file_data_sc_add_ci_delete(FileData *fd)
{
	return file_data_sc_add_ci(fd, FILEDATA_CHANGE_DELETE);
}

gboolean file_data_sc_add_ci_unspecified(FileData *fd, const gchar *dest_path)
{
	if (!file_data_sc_add_ci(fd, FILEDATA_CHANGE_UNSPECIFIED)) return FALSE;
	file_data_sc_update_ci_unspecified(fd, dest_path);
	return TRUE;
}

gboolean file_data_add_ci_write_metadata(FileData *fd)
{
	return file_data_add_ci(fd, FILEDATA_CHANGE_WRITE_METADATA, NULL, NULL);
}

void file_data_sc_free_ci(FileData *fd)
{
	GList *work;

	if (fd->parent) fd = fd->parent;

	file_data_free_ci(fd);

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		file_data_free_ci(sfd);
		work = work->next;
		}
}

gboolean file_data_sc_add_ci_delete_list(GList *fd_list)
{
	GList *work;
	gboolean ret = TRUE;

	work = fd_list;
	while (work)
		{
		FileData *fd = work->data;

		if (!file_data_sc_add_ci_delete(fd)) ret = FALSE;
		work = work->next;
		}

	return ret;
}

/*static*/ void file_data_sc_revert_ci_list(GList *fd_list)
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = work->data;

		file_data_sc_free_ci(fd);
		work = work->prev;
		}
}

/*static*/ gboolean file_data_sc_add_ci_list_call_func(GList *fd_list, const gchar *dest, gboolean (*func)(FileData *, const gchar *))
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = work->data;

		if (!func(fd, dest))
			{
			file_data_sc_revert_ci_list(work->prev);
			return FALSE;
			}
		work = work->next;
		}

	return TRUE;
}

gboolean file_data_sc_add_ci_copy_list(GList *fd_list, const gchar *dest)
{
	return file_data_sc_add_ci_list_call_func(fd_list, dest, file_data_sc_add_ci_copy);
}

gboolean file_data_sc_add_ci_move_list(GList *fd_list, const gchar *dest)
{
	return file_data_sc_add_ci_list_call_func(fd_list, dest, file_data_sc_add_ci_move);
}

gboolean file_data_sc_add_ci_rename_list(GList *fd_list, const gchar *dest)
{
	return file_data_sc_add_ci_list_call_func(fd_list, dest, file_data_sc_add_ci_rename);
}

gboolean file_data_sc_add_ci_unspecified_list(GList *fd_list, const gchar *dest)
{
	return file_data_sc_add_ci_list_call_func(fd_list, dest, file_data_sc_add_ci_unspecified);
}

void file_data_sc_free_ci_list(GList *fd_list)
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = work->data;

		file_data_sc_free_ci(fd);
		work = work->next;
		}
}

/*static*/ void file_data_sc_update_ci(FileData *fd, const gchar *dest_path)
{
	GList *work;
	gchar *dest_path_full = NULL;

	if (fd->parent) fd = fd->parent;

	if (!dest_path)
		{
		dest_path = fd->path;
		}
	else if (!strchr(dest_path, G_DIR_SEPARATOR)) /* we got only filename, not a full path */
		{
		gchar *dir = remove_level_from_path(fd->path);

		dest_path_full = g_build_filename(dir, dest_path, NULL);
		g_free(dir);
		dest_path = dest_path_full;
		}
	else if (fd->change->type != FILEDATA_CHANGE_RENAME && isdir(dest_path)) /* rename should not move files between directories */
		{
		dest_path_full = g_build_filename(dest_path, fd->name, NULL);
		dest_path = dest_path_full;
		}

	file_data_update_ci_dest(fd, dest_path);

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		file_data_update_ci_dest_preserve_ext(sfd, dest_path);
		work = work->next;
		}

	g_free(dest_path_full);
}

/*static*/ gboolean file_data_sc_check_update_ci(FileData *fd, const gchar *dest_path, FileDataChangeType type)
{
	if (!file_data_sc_check_ci(fd, type)) return FALSE;
	file_data_sc_update_ci(fd, dest_path);
	return TRUE;
}

gboolean file_data_sc_update_ci_copy(FileData *fd, const gchar *dest_path)
{
	return file_data_sc_check_update_ci(fd, dest_path, FILEDATA_CHANGE_COPY);
}

gboolean file_data_sc_update_ci_move(FileData *fd, const gchar *dest_path)
{
	return file_data_sc_check_update_ci(fd, dest_path, FILEDATA_CHANGE_MOVE);
}

gboolean file_data_sc_update_ci_rename(FileData *fd, const gchar *dest_path)
{
	return file_data_sc_check_update_ci(fd, dest_path, FILEDATA_CHANGE_RENAME);
}

gboolean file_data_sc_update_ci_unspecified(FileData *fd, const gchar *dest_path)
{
	return file_data_sc_check_update_ci(fd, dest_path, FILEDATA_CHANGE_UNSPECIFIED);
}

static gboolean file_data_sc_update_ci_list_call_func(GList *fd_list,
						      const gchar *dest,
						      gboolean (*func)(FileData *, const gchar *))
{
	GList *work;
	gboolean ret = TRUE;

	work = fd_list;
	while (work)
		{
		FileData *fd = work->data;

		if (!func(fd, dest)) ret = FALSE;
		work = work->next;
		}

	return ret;
}

gboolean file_data_sc_update_ci_move_list(GList *fd_list, const gchar *dest)
{
	return file_data_sc_update_ci_list_call_func(fd_list, dest, file_data_sc_update_ci_move);
}

gboolean file_data_sc_update_ci_copy_list(GList *fd_list, const gchar *dest)
{
	return file_data_sc_update_ci_list_call_func(fd_list, dest, file_data_sc_update_ci_copy);
}

gboolean file_data_sc_update_ci_unspecified_list(GList *fd_list, const gchar *dest)
{
	return file_data_sc_update_ci_list_call_func(fd_list, dest, file_data_sc_update_ci_unspecified);
}

gint file_data_sc_verify_ci(FileData *fd, GList *list)
{
	GList *work;
	gint ret;

	ret = file_data_verify_ci(fd, list);

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		ret |= file_data_verify_ci(sfd, list);
		work = work->next;
		}

	return ret;
}

gboolean file_data_sc_perform_ci(FileData *fd)
{
	GList *work;
	gboolean ret = TRUE;
	FileDataChangeType type = fd->change->type;

	if (!file_data_sc_check_ci(fd, type)) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		if (!file_data_perform_ci(sfd)) ret = FALSE;
		work = work->next;
		}

	if (!file_data_perform_ci(fd)) ret = FALSE;

	return ret;
}


gboolean file_data_sc_apply_ci(FileData *fd)
{
	GList *work;
	FileDataChangeType type = fd->change->type;

	if (!file_data_sc_check_ci(fd, type)) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		file_data_apply_ci(sfd);
		work = work->next;
		}

	file_data_apply_ci(fd);

	return TRUE;
}
