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
 * file_data    - operates on the given fd
 * file_data_sc - operates on the given fd + sidecars - all fds linked via fd->sidecar_files or fd->parent
 */

gchar *FileData::file_data_get_sidecar_path(FileData *fd, gboolean existing_only)
{
	gchar *sidecar_path = NULL;
	GList *work;

	if (!file_data_can_write_sidecar(fd)) return NULL;

	work = fd->parent ? fd->parent->sidecar_files : fd->sidecar_files;
	gchar *extended_extension = g_strconcat(fd->parent ? fd->parent->extension : fd->extension, ".xmp", NULL);
	while (work)
		{
		FileData *sfd = work->data;
		work = work->next;
		if (g_ascii_strcasecmp(sfd->extension, ".xmp") == 0 || g_ascii_strcasecmp(sfd->extension, extended_extension) == 0)
			{
			sidecar_path = g_strdup(sfd->path);
			break;
			}
		}
	g_free(extended_extension);

	if (!existing_only && !sidecar_path)
		{
		if (options->metadata.sidecar_extended_name)
			sidecar_path = g_strconcat(fd->path, ".xmp", NULL);
		else
			{
			gchar *base = g_strndup(fd->path, fd->extension - fd->path);
			sidecar_path = g_strconcat(base, ".xmp", NULL);
			g_free(base);
			}
		}

	return sidecar_path;
}

/* return list of sidecar file extensions in a string */
gchar *FileData::file_data_sc_list_to_string(FileData *fd)
{
	GList *work;
	GString *result = g_string_new("");

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;

		result = g_string_append(result, "+ ");
		result = g_string_append(result, sfd->extension);
		work = work->next;
		if (work) result = g_string_append_c(result, ' ');
		}

	return g_string_free(result, FALSE);
}

/*static*/ gboolean FileData::file_data_list_contains_whole_group(GList *list, FileData *fd)
{
	GList *work;
	if (fd->parent) fd = fd->parent;
	if (!g_list_find(list, fd)) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		if (!g_list_find(list, work->data)) return FALSE;
		work = work->next;
		}
	return TRUE;
}

GList *FileData::file_data_process_groups_in_selection(GList *list, gboolean ungroup, GList **ungrouped_list)
{
	GList *out = NULL;
	GList *work = list;

	/* change partial groups to independent files */
	if (ungroup)
		{
		while (work)
			{
			FileData *fd = work->data;
			work = work->next;

			if (!file_data_list_contains_whole_group(list, fd))
				{
				::file_data_disable_grouping(fd, TRUE);
				if (ungrouped_list)
					{
					*ungrouped_list = g_list_prepend(*ungrouped_list, file_data_ref(fd));
					}
				}
			}
		}

	/* remove sidecars from the list,
	   they can be still accessed via main_fd->sidecar_files */
	work = list;
	while (work)
		{
		FileData *fd = work->data;
		work = work->next;

		if (!fd->parent ||
		    (!ungroup && !file_data_list_contains_whole_group(list, fd)))
			{
			out = g_list_prepend(out, file_data_ref(fd));
			}
		}

	filelist_free(list);
	out = g_list_reverse(out);

	return out;
}

/*static*/ gint FileData::sidecar_file_priority(const gchar *extension)
{
	gint i = 1;
	GList *work;

	if (extension == NULL)
		return 0;

	work = sidecar_ext_get_list();

	while (work) {
		gchar *ext = work->data;

		work = work->next;
		if (g_ascii_strcasecmp(extension, ext) == 0) return i;
		i++;
	}
	return 0;
}

/*static*/ void FileData::file_data_check_sidecars(const GList *basename_list)
{
	/* basename_list contains the new group - first is the parent, then sorted sidecars */
	/* all files in the list have ref count > 0 */

	const GList *work;
	GList *s_work, *new_sidecars;
	FileData *parent_fd;

	if (!basename_list) return;


	DEBUG_2("basename start");
	work = basename_list;
	while (work)
		{
		FileData *fd = work->data;
		work = work->next;
		g_assert(fd->magick == FD_MAGICK);
		DEBUG_2("basename: %p %s", fd, fd->name);
		if (fd->parent)
			{
			g_assert(fd->parent->magick == FD_MAGICK);
			DEBUG_2("                  parent: %p", fd->parent);
			}
		s_work = fd->sidecar_files;
		while (s_work)
			{
			FileData *sfd = s_work->data;
			s_work = s_work->next;
			g_assert(sfd->magick == FD_MAGICK);
			DEBUG_2("                  sidecar: %p %s", sfd, sfd->name);
			}

		g_assert(fd->parent == NULL || fd->sidecar_files == NULL);
		}

	parent_fd = basename_list->data;

	/* check if the second and next entries of basename_list are already connected
	   as sidecars of the first entry (parent_fd) */
	work = basename_list->next;
	s_work = parent_fd->sidecar_files;

	while (work && s_work)
		{
		if (work->data != s_work->data) break;
		work = work->next;
		s_work = s_work->next;
		}

	if (!work && !s_work)
		{
		DEBUG_2("basename no change");
		return; /* no change in grouping */
		}

	/* we have to regroup it */

	/* first, disconnect everything and send notification*/

	work = basename_list;
	while (work)
		{
		FileData *fd = work->data;
		work = work->next;
		g_assert(fd->parent == NULL || fd->sidecar_files == NULL);

		if (fd->parent)
			{
			FileData *old_parent = fd->parent;
			g_assert(old_parent->parent == NULL || old_parent->sidecar_files == NULL);
			file_data_ref(old_parent);
			file_data_disconnect_sidecar_file(old_parent, fd);
			file_data_send_notification(old_parent, NOTIFY_REREAD);
			file_data_unref(old_parent);
			}

		while (fd->sidecar_files)
			{
			FileData *sfd = fd->sidecar_files->data;
			g_assert(sfd->parent == NULL || sfd->sidecar_files == NULL);
			file_data_ref(sfd);
			file_data_disconnect_sidecar_file(fd, sfd);
			file_data_send_notification(sfd, NOTIFY_REREAD);
			file_data_unref(sfd);
			}
		file_data_send_notification(fd, NOTIFY_GROUPING);

		g_assert(fd->parent == NULL && fd->sidecar_files == NULL);
		}

	/* now we can form the new group */
	work = basename_list->next;
	new_sidecars = NULL;
	while (work)
		{
		FileData *sfd = work->data;
		g_assert(sfd->magick == FD_MAGICK);
		g_assert(sfd->parent == NULL && sfd->sidecar_files == NULL);
		sfd->parent = parent_fd;
		new_sidecars = g_list_prepend(new_sidecars, sfd);
		work = work->next;
		}
	g_assert(parent_fd->sidecar_files == NULL);
	parent_fd->sidecar_files = g_list_reverse(new_sidecars);
	DEBUG_1("basename group changed for %s", parent_fd->path);
}


/*static*/ void FileData::file_data_disconnect_sidecar_file(FileData *target, FileData *sfd)
{
	g_assert(target->magick == FD_MAGICK);
	g_assert(sfd->magick == FD_MAGICK);
	g_assert(g_list_find(target->sidecar_files, sfd));

	file_data_ref(target);
	file_data_ref(sfd);

	g_assert(sfd->parent == target);

	sfd->file_data_increment_version(sfd); /* increments both sfd and target */

	target->sidecar_files = g_list_remove(target->sidecar_files, sfd);
	sfd->parent = NULL;
	g_free(sfd->extended_extension);
	sfd->extended_extension = NULL;

	file_data_unref(target);
	file_data_unref(sfd);
}

/* disables / enables grouping for particular file, sends UPDATE notification */
void FileData::file_data_disable_grouping(FileData *fd, gboolean disable)
{
	if (!fd->disable_grouping == !disable) return;

	fd->disable_grouping = !!disable;

	if (disable)
		{
		if (fd->parent)
			{
			FileData *parent = file_data_ref(fd->parent);
			file_data_disconnect_sidecar_file(parent, fd);
			file_data_send_notification(parent, NOTIFY_GROUPING);
			file_data_unref(parent);
			}
		else if (fd->sidecar_files)
			{
			GList *sidecar_files = filelist_copy(fd->sidecar_files);
			GList *work = sidecar_files;
			while (work)
				{
				FileData *sfd = work->data;
				work = work->next;
				file_data_disconnect_sidecar_file(fd, sfd);
				file_data_send_notification(sfd, NOTIFY_GROUPING);
				}
			file_data_check_sidecars(sidecar_files); /* this will group the sidecars back together */
			filelist_free(sidecar_files);
			}
		else
			{
			file_data_increment_version(fd); /* the functions called in the cases above increments the version too */
			}
		}
	else
		{
		file_data_increment_version(fd);
		/* file_data_check_sidecars call is not necessary - the file will be re-grouped on next dir read */
		}
	file_data_send_notification(fd, NOTIFY_GROUPING);
}

void FileData::file_data_disable_grouping_list(GList *fd_list, gboolean disable)
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = work->data;

		::file_data_disable_grouping(fd, disable);
		work = work->next;
		}
}

