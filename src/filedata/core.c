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

#ifdef DEBUG_FILEDATA
gint global_file_data_count = 0;
#endif

GHashTable *FileData::file_data_pool = NULL;
GHashTable *FileData::file_data_planned_change_hash = NULL;

// private
FileData *FileData::file_data_new(const gchar *path_utf8, struct stat *st, gboolean disable_sidecars)
{
	FileData *fd;
	struct passwd *user;
	struct group *group;

	DEBUG_2("file_data_new: '%s' %d", path_utf8, disable_sidecars);

	if (S_ISDIR(st->st_mode)) disable_sidecars = TRUE;

	if (!file_data_pool)
		file_data_pool = g_hash_table_new(g_str_hash, g_str_equal);

	fd = g_hash_table_lookup(file_data_pool, path_utf8);
	if (fd)
		{
		file_data_ref(fd);
		}

	if (!fd && file_data_planned_change_hash)
		{
		fd = g_hash_table_lookup(file_data_planned_change_hash, path_utf8);
		if (fd)
			{
			DEBUG_1("planned change: using %s -> %s", path_utf8, fd->path);
			if (!isfile(fd->path))
				{
				file_data_ref(fd);
				fd->file_data_apply_ci(fd);
				}
			else
				{
				fd = NULL;
				}
			}
		}

	if (fd)
		{
		gboolean changed;

		if (disable_sidecars) Sidecar::disable_grouping(fd, TRUE);


		changed = fd->file_data_check_changed_single_file(fd, st);

		DEBUG_2("file_data_pool hit: '%s' %s", fd->path, changed ? "(changed)" : "");

		return fd;
		}

	fd = g_new0(FileData, 1);
#ifdef DEBUG_FILEDATA
	global_file_data_count++;
	DEBUG_2("file data count++: %d", global_file_data_count);
#endif

	fd->size = st->st_size;
	fd->date = st->st_mtime;
	fd->cdate = st->st_ctime;
	fd->mode = st->st_mode;
	fd->ref = 1;
	fd->magick = FD_MAGICK;
	fd->exifdate = 0;
	fd->rating = STAR_RATING_NOT_READ;
	fd->format_class = filter_file_get_class(path_utf8);
	fd->page_num = 0;
	fd->page_total = 0;

	user = getpwuid(st->st_uid);
	if (!user)
		{
		fd->owner = g_strdup_printf("%u", st->st_uid);
		}
	else
		{
		fd->owner = g_strdup(user->pw_name);
		}

	group = getgrgid(st->st_gid);
	if (!group)
		{
		fd->group = g_strdup_printf("%u", st->st_gid);
		}
	else
		{
		fd->group = g_strdup(group->gr_name);
		}

	fd->sym_link = get_symbolic_link(path_utf8);

	if (disable_sidecars) fd->disable_grouping = TRUE;

	fd->file_data_set_path(fd, path_utf8); /* set path, name, collate_key_*, original_path */

	return fd;
}

// private
/*static*/ FileData *FileData::file_data_new_local(const gchar *path, struct stat *st, gboolean disable_sidecars)
{
	gchar *path_utf8 = path_to_utf8(path);
	FileData *ret = file_data_new(path_utf8, st, disable_sidecars);

	g_free(path_utf8);
	return ret;
}

/*static*/ FileData *FileData::file_data_new_simple(const gchar *path_utf8)
{
	struct stat st;
	FileData *fd;

	if (!stat_utf8(path_utf8, &st))
		{
		st.st_size = 0;
		st.st_mtime = 0;
		}

	fd = g_hash_table_lookup(file_data_pool, path_utf8);
	if (!fd) fd = file_data_new(path_utf8, &st, TRUE);
	if (fd)
		{
		file_data_ref(fd);
		}

	return fd;
}

FileData *FileData::file_data_new_group(const gchar *path_utf8)
{
	gchar *dir;
	struct stat st;
	FileData *fd;
	GList *files;

	if (!stat_utf8(path_utf8, &st))
		{
		st.st_size = 0;
		st.st_mtime = 0;
		}

	if (S_ISDIR(st.st_mode))
		return file_data_new(path_utf8, &st, TRUE);

	dir = remove_level_from_path(path_utf8);

	FileList::read_real(dir, &files, NULL, TRUE);

	fd = g_hash_table_lookup(file_data_pool, path_utf8);
	if (!fd) fd = file_data_new(path_utf8, &st, TRUE);
	if (fd)
		{
		file_data_ref(fd);
		}

	FileList::fl_free(files);
	g_free(dir);
	return fd;
}

/*static*/ FileData *FileData::file_data_new_no_grouping(const gchar *path_utf8)
{
	struct stat st;

	if (!stat_utf8(path_utf8, &st))
		{
		st.st_size = 0;
		st.st_mtime = 0;
		}

	return file_data_new(path_utf8, &st, TRUE);
}

/*static*/ FileData *FileData::file_data_new_dir(const gchar *path_utf8)
{
	struct stat st;

	if (!stat_utf8(path_utf8, &st))
		{
		st.st_size = 0;
		st.st_mtime = 0;
		}
	else
		/* dir or non-existing yet */
		g_assert(S_ISDIR(st.st_mode));

	return file_data_new(path_utf8, &st, TRUE);
}

/*
 *-----------------------------------------------------------------------------
 * reference counting
 *-----------------------------------------------------------------------------
 */

#ifdef DEBUG_FILEDATA
FileData *FileData::file_data_ref_debug(const gchar *file, gint line, FileData *fd)
#else
FileData *FileData::file_data_ref(FileData *fd)
#endif
{
	if (fd == NULL) return NULL;
	if (fd->magick != FD_MAGICK)
#ifdef DEBUG_FILEDATA
		log_printf("Error: fd magick mismatch @ %s:%d  fd=%p", file, line, fd);
#else
		log_printf("Error: fd magick mismatch fd=%p", fd);
#endif
	g_assert(fd->magick == FD_MAGICK);
	fd->ref++;

#ifdef DEBUG_FILEDATA
	DEBUG_2("file_data_ref fd=%p (%d): '%s' @ %s:%d", fd, fd->ref, fd->path, file, line);
#else
	DEBUG_2("file_data_ref fd=%p (%d): '%s'", fd, fd->ref, fd->path);
#endif
	return fd;
}

/*static*/ void FileData::file_data_free(FileData *fd)
{
	g_assert(fd->magick == FD_MAGICK);
	g_assert(fd->ref == 0);
	g_assert(!fd->locked);

#ifdef DEBUG_FILEDATA
	global_file_data_count--;
	DEBUG_2("file data count--: %d", global_file_data_count);
#endif

	metadata_cache_free(fd);
	g_hash_table_remove(file_data_pool, fd->original_path);

	g_free(fd->path);
	g_free(fd->original_path);
	g_free(fd->collate_key_name);
	g_free(fd->collate_key_name_nocase);
	g_free(fd->extended_extension);
	if (fd->thumb_pixbuf) g_object_unref(fd->thumb_pixbuf);
	histmap_free(fd->histmap);
	g_free(fd->owner);
	g_free(fd->group);
	g_free(fd->sym_link);
	g_free(fd->format_name);
	g_assert(fd->sidecar_files == NULL); /* sidecar files must be freed before calling this */

	file_data_change_info_free(NULL, fd);
	g_free(fd);
}

/**
 * @brief Checks if the FileData is referenced
 *
 * Checks the refcount and whether the FileData is locked.
 */
/*static*/ gboolean FileData::file_data_check_has_ref(FileData *fd)
{
	return fd->ref > 0 || fd->locked;
}

/**
 * @brief Consider freeing a FileData.
 *
 * This function will free a FileData and its children provided that neither its parent nor it has
 * a positive refcount, and provided that neither is locked.
 */
/*static*/ void FileData::file_data_consider_free(FileData *fd)
{
	GList *work;
	FileData *parent = fd->parent ? fd->parent : fd;

	g_assert(fd->magick == FD_MAGICK);
	if (file_data_check_has_ref(fd)) return;
	if (file_data_check_has_ref(parent)) return;

	work = parent->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;
		if (file_data_check_has_ref(sfd)) return;
		work = work->next;
		}

	/* Neither the parent nor the siblings are referenced, so we can free everything */
	DEBUG_2("file_data_consider_free: deleting '%s', parent '%s'",
		fd->path, fd->parent ? parent->path : "-");

	work = parent->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;
		file_data_free(sfd);
		work = work->next;
		}

	g_list_free(parent->sidecar_files);
	parent->sidecar_files = NULL;

	file_data_free(parent);
}

#ifdef DEBUG_FILEDATA
void FileData::file_data_unref_debug(const gchar *file, gint line, FileData *fd)
#else
void FileData::file_data_unref(FileData *fd)
#endif
{
	if (fd == NULL) return;
	if (fd->magick != FD_MAGICK)
#ifdef DEBUG_FILEDATA
		log_printf("Error: fd magick mismatch @ %s:%d  fd=%p", file, line, fd);
#else
		log_printf("Error: fd magick mismatch fd=%p", fd);
#endif
	g_assert(fd->magick == FD_MAGICK);

	fd->ref--;
#ifdef DEBUG_FILEDATA
	DEBUG_2("file_data_unref fd=%p (%d:%d): '%s' @ %s:%d", fd, fd->ref, fd->locked, fd->path,
		file, line);
#else
	DEBUG_2("file_data_unref fd=%p (%d:%d): '%s'", fd, fd->ref, fd->locked, fd->path);
#endif

	// Free FileData if it's no longer ref'd
	file_data_consider_free(fd);
}

/**
 * @brief Lock the FileData in memory.
 *
 * This allows the caller to prevent a FileData from being freed, even after its refcount is zero.
 * This is intended to be used in cases where a FileData _should_ stay in memory as an optimization,
 * even if the code would continue to function properly even if the FileData were freed.  Code that
 * _requires_ the FileData to remain in memory should continue to use file_data_(un)ref.
 * <p />
 * Note: This differs from file_data_ref in that the behavior is reentrant -- after N calls to
 * file_data_lock, a single call to file_data_unlock will unlock the FileData.
 */
void FileData::file_data_lock(FileData *fd)
{
	if (fd == NULL) return;
	if (fd->magick != FD_MAGICK) log_printf("Error: fd magick mismatch fd=%p", fd);

	g_assert(fd->magick == FD_MAGICK);
	fd->locked = TRUE;

	DEBUG_2("file_data_ref fd=%p (%d): '%s'", fd, fd->ref, fd->path);
}

/**
 * @brief Reset the maintain-FileData-in-memory lock
 *
 * This again allows the FileData to be freed when its refcount drops to zero.  Automatically frees
 * the FileData if its refcount is already zero (which will happen if the lock is the only thing
 * keeping it from being freed.
 */
void FileData::file_data_unlock(FileData *fd)
{
	if (fd == NULL) return;
	if (fd->magick != FD_MAGICK) log_printf("Error: fd magick mismatch fd=%p", fd);

	g_assert(fd->magick == FD_MAGICK);
	fd->locked = FALSE;

	// Free FileData if it's no longer ref'd
	file_data_consider_free(fd);
}

/**
 * @brief Lock all of the FileDatas in the provided list
 *
 * @see file_data_lock(#FileData)
 */
void FileData::file_data_lock_list(GList *list)
{
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd = work->data;
		work = work->next;
		fd->file_data_lock(fd);
		}
}

/**
 * @brief Unlock all of the FileDatas in the provided list
 *
 * @see #file_data_unlock(#FileData)
 */
void FileData::file_data_unlock_list(GList *list)
{
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd = work->data;
		work = work->next;
		fd->file_data_unlock(fd);
		}
}

/*
 *-----------------------------------------------------------------------------
 * changed files detection and notification
 *-----------------------------------------------------------------------------
 */

void FileData::file_data_increment_version(FileData *fd)
{
	fd->version++;
	fd->valid_marks = 0;
	if (fd->parent)
		{
		fd->parent->version++;
		fd->parent->valid_marks = 0;
		}
}

/*static*/ gboolean FileData::file_data_check_changed_single_file(FileData *fd, struct stat *st)
{
	if (fd->size != st->st_size ||
	    fd->date != st->st_mtime)
		{
		fd->size = st->st_size;
		fd->date = st->st_mtime;
		fd->cdate = st->st_ctime;
		fd->mode = st->st_mode;
		if (fd->thumb_pixbuf) g_object_unref(fd->thumb_pixbuf);
		fd->thumb_pixbuf = NULL;
		file_data_increment_version(fd);
		file_data_send_notification(fd, NOTIFY_REREAD);
		return TRUE;
		}
	return FALSE;
}

/*static*/ gboolean FileData::file_data_check_changed_files_recursive(FileData *fd, struct stat *st)
{
	gboolean ret = FALSE;
	GList *work;

	ret = file_data_check_changed_single_file(fd, st);

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = work->data;
		struct stat st;
		work = work->next;

		if (!stat_utf8(sfd->path, &st))
			{
			fd->size = 0;
			fd->date = 0;
			file_data_ref(sfd);
			Sidecar::disconnect_sidecar_file(fd, sfd);
			ret = TRUE;
			file_data_increment_version(sfd);
			file_data_send_notification(sfd, NOTIFY_REREAD);
			file_data_unref(sfd);
			continue;
			}

		ret |= file_data_check_changed_files_recursive(sfd, &st);
		}
	return ret;
}


gboolean FileData::file_data_check_changed_files(FileData *fd)
{
	gboolean ret = FALSE;
	struct stat st;

	if (fd->parent) fd = fd->parent;

	if (!stat_utf8(fd->path, &st))
		{
		GList *sidecars;
		GList *work;
		FileData *sfd = NULL;

		/* parent is missing, we have to rebuild whole group */
		ret = TRUE;
		fd->size = 0;
		fd->date = 0;

		/* file_data_disconnect_sidecar_file might delete the file,
		   we have to keep the reference to prevent this */
		sidecars = FileList::copy(fd->sidecar_files);
		file_data_ref(fd);
		work = sidecars;
		while (work)
			{
			sfd = work->data;
			work = work->next;

			Sidecar::disconnect_sidecar_file(fd, sfd);
			}
		Sidecar::check_sidecars(sidecars); /* this will group the sidecars back together */
		/* now we can release the sidecars */
		FileList::fl_free(sidecars);
		file_data_increment_version(fd);
		file_data_send_notification(fd, NOTIFY_REREAD);
		file_data_unref(fd);
		}
	else
		{
		ret |= file_data_check_changed_files_recursive(fd, &st);
		}

	return ret;
}

static GHashTable *file_data_monitor_pool = NULL;
static guint realtime_monitor_id = 0; /* event source id */

/*static*/ void FileData::realtime_monitor_check_cb(gpointer key, gpointer value, gpointer data)
{
	FileData *fd = key;

	::file_data_check_changed_files(fd);

	DEBUG_1("monitor %s", fd->path);
}

/*static*/ gboolean FileData::realtime_monitor_cb(gpointer data)
{
	if (!options->update_on_time_change) return TRUE;
	g_hash_table_foreach(
            file_data_monitor_pool, &FileData::realtime_monitor_check_cb, NULL);
	return TRUE;
}

gboolean FileData::file_data_register_real_time_monitor(FileData *fd)
{
	gint count;

	file_data_ref(fd);

	if (!file_data_monitor_pool)
		file_data_monitor_pool = g_hash_table_new(g_direct_hash, g_direct_equal);

	count = GPOINTER_TO_INT(g_hash_table_lookup(file_data_monitor_pool, fd));

	DEBUG_1("Register realtime %d %s", count, fd->path);

	count++;
	g_hash_table_insert(file_data_monitor_pool, fd, GINT_TO_POINTER(count));

	if (!realtime_monitor_id)
		{
		realtime_monitor_id = g_timeout_add(
                5000, &FileData::realtime_monitor_cb, NULL);
		}

	return TRUE;
}

gboolean FileData::file_data_unregister_real_time_monitor(FileData *fd)
{
	gint count;

	g_assert(file_data_monitor_pool);

	count = GPOINTER_TO_INT(g_hash_table_lookup(file_data_monitor_pool, fd));

	DEBUG_1("Unregister realtime %d %s", count, fd->path);

	g_assert(count > 0);

	count--;

	if (count == 0)
		g_hash_table_remove(file_data_monitor_pool, fd);
	else
		g_hash_table_insert(file_data_monitor_pool, fd, GINT_TO_POINTER(count));

	file_data_unref(fd);

	if (g_hash_table_size(file_data_monitor_pool) == 0)
		{
		g_source_remove(realtime_monitor_id);
		realtime_monitor_id = 0;
		return FALSE;
		}

	return TRUE;
}
