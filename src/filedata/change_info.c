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
#include "ui_fileops.h"
#include "metadata.h"
#include "trash.h"

/*
 * add FileDataChangeInfo (see typedefs.h) for the given operation
 * uses file_data_add_change_info
 *
 * fails if the fd->change already exists - change operations can't run in parallel
 * fd->change_info works as a lock
 *
 * dest can be NULL - in this case the current name is used for now, it will
 * be changed later
 */

/*
   FileDataChangeInfo types:
   COPY
   MOVE   - path is changed, name may be changed too
   RENAME - path remains unchanged, name is changed
            extension should remain (FIXME should we allow editing extension? it will make problems with grouping)
	    sidecar names are changed too, extensions are not changed
   DELETE
   UPDATE - file size, date or grouping has been changed
*/

gboolean FileData::file_data_add_ci(FileData *fd, FileDataChangeType type, const gchar *src, const gchar *dest)
{
	FileDataChangeInfo *fdci;

	if (fd->change) return FALSE;

	fdci = g_new0(FileDataChangeInfo, 1);

	fdci->type = type;

	if (src)
		fdci->source = g_strdup(src);
	else
		fdci->source = g_strdup(fd->path);

	if (dest)
		fdci->dest = g_strdup(dest);

	fd->change = fdci;

	return TRUE;
}

/*static*/ void FileData::file_data_planned_change_remove(FileData *fd)
{
	if (file_data_planned_change_hash &&
	    (fd->change->type == FILEDATA_CHANGE_MOVE || fd->change->type == FILEDATA_CHANGE_RENAME))
		{
		if (g_hash_table_lookup(file_data_planned_change_hash, fd->change->dest) == fd)
			{
			DEBUG_1("planned change: removing %s -> %s", fd->change->dest, fd->path);
			g_hash_table_remove(file_data_planned_change_hash, fd->change->dest);
			file_data_unref(fd);
			if (g_hash_table_size(file_data_planned_change_hash) == 0)
				{
				g_hash_table_destroy(file_data_planned_change_hash);
				file_data_planned_change_hash = NULL;
				DEBUG_1("planned change: empty");
				}
			}
		}
}


void FileData::file_data_free_ci(FileData *fd)
{
	FileDataChangeInfo *fdci = fd->change;

	if (!fdci) return;

	file_data_planned_change_remove(fd);

	if (fdci->regroup_when_finished) file_data_disable_grouping(fd, FALSE);

	g_free(fdci->source);
	g_free(fdci->dest);

	g_free(fdci);

	fd->change = NULL;
}

void FileData::file_data_set_regroup_when_finished(FileData *fd, gboolean enable)
{
	FileDataChangeInfo *fdci = fd->change;
	if (!fdci) return;
	fdci->regroup_when_finished = enable;
}

gboolean FileData::file_data_add_ci_write_metadata_list(GList *fd_list)
{
	GList *work;
	gboolean ret = TRUE;

	work = fd_list;
	while (work)
		{
		FileData *fd = work->data;

		if (!fd->file_data_add_ci_write_metadata(fd)) ret = FALSE;
		work = work->next;
		}

	return ret;
}

void FileData::file_data_free_ci_list(GList *fd_list)
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = work->data;

		::file_data_free_ci(fd);
		work = work->next;
		}
}

/*
 * update existing fd->change, it will be used from dialog callbacks for interactive editing
 * fails if fd->change does not exist or the change type does not match
 */

/*static*/ void FileData::file_data_update_planned_change_hash(FileData *fd, const gchar *old_path, gchar *new_path)
{
	FileDataChangeType type = fd->change->type;

	if (type == FILEDATA_CHANGE_MOVE || type == FILEDATA_CHANGE_RENAME)
		{
		FileData *ofd;

		if (!file_data_planned_change_hash)
			file_data_planned_change_hash = g_hash_table_new(g_str_hash, g_str_equal);

		if (old_path && g_hash_table_lookup(file_data_planned_change_hash, old_path) == fd)
			{
			DEBUG_1("planned change: removing %s -> %s", old_path, fd->path);
			g_hash_table_remove(file_data_planned_change_hash, old_path);
			file_data_unref(fd);
			}

		ofd = g_hash_table_lookup(file_data_planned_change_hash, new_path);
		if (ofd != fd)
			{
			if (ofd)
				{
				DEBUG_1("planned change: replacing %s -> %s", new_path, ofd->path);
				g_hash_table_remove(file_data_planned_change_hash, new_path);
				file_data_unref(ofd);
				}

			DEBUG_1("planned change: inserting %s -> %s", new_path, fd->path);
			file_data_ref(fd);
			g_hash_table_insert(file_data_planned_change_hash, new_path, fd);
			}
		}
}

/*static*/ void FileData::file_data_update_ci_dest(FileData *fd, const gchar *dest_path)
{
	gchar *old_path = fd->change->dest;

	fd->change->dest = g_strdup(dest_path);
	file_data_update_planned_change_hash(fd, old_path, fd->change->dest);
	g_free(old_path);
}

/*static*/ void FileData::file_data_update_ci_dest_preserve_ext(FileData *fd, const gchar *dest_path)
{
	const gchar *extension = registered_extension_from_path(fd->change->source);
	gchar *base = remove_extension_from_path(dest_path);
	gchar *old_path = fd->change->dest;

	fd->change->dest = g_strconcat(base, fd->extended_extension ? fd->extended_extension : extension, NULL);
	file_data_update_planned_change_hash(fd, old_path, fd->change->dest);

	g_free(old_path);
	g_free(base);
}


/*
 * verify source and dest paths - dest image exists, etc.
 * it should detect all possible problems with the planned operation
 */

gint FileData::file_data_verify_ci(FileData *fd, GList *list)
{
	gint ret = CHANGE_OK;
	gchar *dir;
	GList *work = NULL;
	FileData *fd1 = NULL;

	if (!fd->change)
		{
		DEBUG_1("Change checked: no change info: %s", fd->path);
		return ret;
		}

	if (!isname(fd->path))
		{
		/* this probably should not happen */
		ret |= CHANGE_NO_SRC;
		DEBUG_1("Change checked: file does not exist: %s", fd->path);
		return ret;
		}

	dir = remove_level_from_path(fd->path);

	if (fd->change->type != FILEDATA_CHANGE_DELETE &&
	    fd->change->type != FILEDATA_CHANGE_MOVE && /* the unsaved metadata should survive move and rename operations */
	    fd->change->type != FILEDATA_CHANGE_RENAME &&
	    fd->change->type != FILEDATA_CHANGE_WRITE_METADATA &&
	    fd->modified_xmp)
		{
		ret |= CHANGE_WARN_UNSAVED_META;
		DEBUG_1("Change checked: unsaved metadata: %s", fd->path);
		}

	if (fd->change->type != FILEDATA_CHANGE_DELETE &&
	    fd->change->type != FILEDATA_CHANGE_WRITE_METADATA &&
	    !access_file(fd->path, R_OK))
		{
		ret |= CHANGE_NO_READ_PERM;
		DEBUG_1("Change checked: no read permission: %s", fd->path);
		}
	else if ((fd->change->type == FILEDATA_CHANGE_DELETE || fd->change->type == FILEDATA_CHANGE_MOVE) &&
	    	 !access_file(dir, W_OK))
		{
		ret |= CHANGE_NO_WRITE_PERM_DIR;
		DEBUG_1("Change checked: source dir is readonly: %s", fd->path);
		}
	else if (fd->change->type != FILEDATA_CHANGE_COPY &&
		 fd->change->type != FILEDATA_CHANGE_UNSPECIFIED &&
		 fd->change->type != FILEDATA_CHANGE_WRITE_METADATA &&
		 !access_file(fd->path, W_OK))
		{
		ret |= CHANGE_WARN_NO_WRITE_PERM;
		DEBUG_1("Change checked: no write permission: %s", fd->path);
		}
	/* WRITE_METADATA is special because it can be configured to silently write to ~/.geeqie/...
	   - that means that there are no hard errors and warnings can be disabled
	   - the destination is determined during the check
	*/
	else if (fd->change->type == FILEDATA_CHANGE_WRITE_METADATA)
		{
		/* determine destination file */
		gboolean have_dest = FALSE;
		gchar *dest_dir = NULL;

		if (options->metadata.save_in_image_file)
			{
			if (Util::can_write_directly(fd))
				{
				/* we can write the file directly */
				if (access_file(fd->path, W_OK))
					{
					have_dest = TRUE;
					}
				else
					{
					if (options->metadata.warn_on_write_problems)
						{
						ret |= CHANGE_WARN_NO_WRITE_PERM;
						DEBUG_1("Change checked: file is not writable: %s", fd->path);
						}
					}
				}
			else if (Util::can_write_sidecar(fd))
				{
				/* we can write sidecar */
				gchar *sidecar = file_data_get_sidecar_path(fd, FALSE);
				if (access_file(sidecar, W_OK) || (!isname(sidecar) && access_file(dir, W_OK)))
					{
					file_data_update_ci_dest(fd, sidecar);
					have_dest = TRUE;
					}
				else
					{
					if (options->metadata.warn_on_write_problems)
						{
						ret |= CHANGE_WARN_NO_WRITE_PERM;
						DEBUG_1("Change checked: file is not writable: %s", sidecar);
						}
					}
				g_free(sidecar);
				}
			}

		if (!have_dest)
			{
			/* write private metadata file under ~/.geeqie */

			/* If an existing metadata file exists, we will try writing to
			 * it's location regardless of the user's preference.
			 */
			gchar *metadata_path = NULL;
#ifdef HAVE_EXIV2
			/* but ignore XMP if we are not able to write it */
			metadata_path = cache_find_location(CACHE_TYPE_XMP_METADATA, fd->path);
#endif
			if (!metadata_path) metadata_path = cache_find_location(CACHE_TYPE_METADATA, fd->path);

			if (metadata_path && !access_file(metadata_path, W_OK))
				{
				g_free(metadata_path);
				metadata_path = NULL;
				}

			if (!metadata_path)
				{
				mode_t mode = 0755;

				dest_dir = cache_get_location(CACHE_TYPE_METADATA, fd->path, FALSE, &mode);
				if (recursive_mkdir_if_not_exists(dest_dir, mode))
					{
					gchar *filename = g_strconcat(fd->name, options->metadata.save_legacy_format ? GQ_CACHE_EXT_METADATA : GQ_CACHE_EXT_XMP_METADATA, NULL);

					metadata_path = g_build_filename(dest_dir, filename, NULL);
					g_free(filename);
					}
				}
			if (access_file(metadata_path, W_OK) || (!isname(metadata_path) && access_file(dest_dir, W_OK)))
				{
				file_data_update_ci_dest(fd, metadata_path);
				have_dest = TRUE;
				}
			else
				{
				ret |= CHANGE_NO_WRITE_PERM_DEST;
				DEBUG_1("Change checked: file is not writable: %s", metadata_path);
				}
			g_free(metadata_path);
			}
		g_free(dest_dir);
		}

	if (fd->change->dest && fd->change->type != FILEDATA_CHANGE_WRITE_METADATA)
		{
		gboolean same;
		gchar *dest_dir;

		same = (strcmp(fd->path, fd->change->dest) == 0);

		if (!same)
			{
			const gchar *dest_ext = registered_extension_from_path(fd->change->dest);
			if (!dest_ext) dest_ext = "";
			if (!options->file_filter.disable_file_extension_checks)
				{
				if (g_ascii_strcasecmp(fd->extension, dest_ext) != 0)
					{
					ret |= CHANGE_WARN_CHANGED_EXT;
					DEBUG_1("Change checked: source and destination have different extensions: %s -> %s", fd->path, fd->change->dest);
					}
				}
			}
		else
			{
			if (fd->change->type != FILEDATA_CHANGE_UNSPECIFIED) /** @FIXME this is now needed for running editors */
		   		{
				ret |= CHANGE_WARN_SAME;
				DEBUG_1("Change checked: source and destination are the same: %s -> %s", fd->path, fd->change->dest);
				}
			}

		dest_dir = remove_level_from_path(fd->change->dest);

		if (!isdir(dest_dir))
			{
			ret |= CHANGE_NO_DEST_DIR;
			DEBUG_1("Change checked: destination dir does not exist: %s -> %s", fd->path, fd->change->dest);
			}
		else if (!access_file(dest_dir, W_OK))
			{
			ret |= CHANGE_WARN_NO_WRITE_PERM_DEST_DIR;
			DEBUG_1("Change checked: destination dir is readonly: %s -> %s", fd->path, fd->change->dest);
			}
		else if (!same)
			{
			if (isfile(fd->change->dest))
				{
				if (!access_file(fd->change->dest, W_OK))
					{
					ret |= CHANGE_NO_WRITE_PERM_DEST;
					DEBUG_1("Change checked: destination file exists and is readonly: %s -> %s", fd->path, fd->change->dest);
					}
				else
					{
					ret |= CHANGE_WARN_DEST_EXISTS;
					DEBUG_1("Change checked: destination exists: %s -> %s", fd->path, fd->change->dest);
					}
				}
			else if (isdir(fd->change->dest))
				{
				ret |= CHANGE_DEST_EXISTS;
				DEBUG_1("Change checked: destination exists: %s -> %s", fd->path, fd->change->dest);
				}
			}

		g_free(dest_dir);
		}

	/* During a rename operation, check if another planned destination file has
	 * the same filename
	 */
 	if(fd->change->type == FILEDATA_CHANGE_RENAME ||
				fd->change->type == FILEDATA_CHANGE_COPY ||
				fd->change->type == FILEDATA_CHANGE_MOVE)
		{
		work = list;
		while (work)
			{
			fd1 = work->data;
			work = work->next;
			if (fd1 != NULL && fd != fd1 )
				{
				if (!strcmp(fd->change->dest, fd1->change->dest))
					{
					ret |= CHANGE_DUPLICATE_DEST;
					}
				}
			}
		}

	fd->change->error = ret;
	if (ret == 0) DEBUG_1("Change checked: OK: %s", fd->path);

	g_free(dir);
	return ret;
}

gint FileData::file_data_verify_ci_list(GList *list, gchar **desc, gboolean with_sidecars)
{
	GList *work;
	gint all_errors = 0;
	gint common_errors = ~0;
	gint num;
	gint *errors;
	gint i;

	if (!list) return 0;

	num = g_list_length(list);
	errors = g_new(int, num);
	work = list;
	i = 0;
	while (work)
		{
		FileData *fd;
		gint error;

		fd = work->data;
		work = work->next;

		error = with_sidecars ? ::file_data_sc_verify_ci(fd, list)
                              : ::file_data_verify_ci(fd, list);
		all_errors |= error;
		common_errors &= error;

		errors[i] = error;

		i++;
		}

	if (desc && all_errors)
		{
		GList *work;
		GString *result = g_string_new("");

		if (common_errors)
			{
			gchar *str = file_data_get_error_string(common_errors);
			g_string_append(result, str);
			g_string_append(result, "\n");
			g_free(str);
			}

		work = list;
		i = 0;
		while (work)
			{
			FileData *fd;
			gint error;

			fd = work->data;
			work = work->next;

			error = errors[i] & ~common_errors;

			if (error)
				{
				gchar *str = file_data_get_error_string(error);
				g_string_append_printf(result, "%s: %s\n", fd->name, str);
				g_free(str);
				}
			i++;
			}
		*desc = g_string_free(result, FALSE);
		}

	g_free(errors);
	return all_errors;
}


/*
 * perform the change described by FileFataChangeInfo
 * it is used for internal operations,
 * this function actually operates with files on the filesystem
 * it should implement safe delete
 */

/*static*/ gboolean FileData::file_data_perform_move(FileData *fd)
{
	g_assert(!strcmp(fd->change->source, fd->path));
	return move_file(fd->change->source, fd->change->dest);
}

/*static*/ gboolean FileData::file_data_perform_copy(FileData *fd)
{
	g_assert(!strcmp(fd->change->source, fd->path));
	return copy_file(fd->change->source, fd->change->dest);
}

/*static*/ gboolean FileData::file_data_perform_delete(FileData *fd)
{
	if (isdir(fd->path) && !islink(fd->path))
		return rmdir_utf8(fd->path);
	else
		if (options->file_ops.safe_delete_enable)
			return file_util_safe_unlink(fd->path);
		else
			return unlink_file(fd->path);
}

gboolean FileData::file_data_perform_ci(FileData *fd)
{
	FileDataChangeType type = fd->change->type;

	switch (type)
		{
		case FILEDATA_CHANGE_MOVE:
			return file_data_perform_move(fd);
		case FILEDATA_CHANGE_COPY:
			return file_data_perform_copy(fd);
		case FILEDATA_CHANGE_RENAME:
			return file_data_perform_move(fd); /* the same as move */
		case FILEDATA_CHANGE_DELETE:
			return file_data_perform_delete(fd);
		case FILEDATA_CHANGE_WRITE_METADATA:
			return metadata_write_perform(fd);
		case FILEDATA_CHANGE_UNSPECIFIED:
			/* nothing to do here */
			break;
		}
	return TRUE;
}

/*
 * updates FileData structure according to FileDataChangeInfo
 */

gboolean FileData::file_data_apply_ci(FileData *fd)
{
	FileDataChangeType type = fd->change->type;

	/** @FIXME delete ?*/
	if (type == FILEDATA_CHANGE_MOVE || type == FILEDATA_CHANGE_RENAME)
		{
		DEBUG_1("planned change: applying %s -> %s", fd->change->dest, fd->path);
		file_data_planned_change_remove(fd);

		if (g_hash_table_lookup(file_data_pool, fd->change->dest))
			{
			/* this change overwrites another file which is already known to other modules
			   renaming fd would create duplicate FileData structure
			   the best thing we can do is nothing
			*/
			/**  @FIXME maybe we could copy stuff like marks
			*/
			DEBUG_1("can't rename fd, target exists %s -> %s", fd->change->dest, fd->path);
			}
		else
			{
			file_data_set_path(fd, fd->change->dest);
			}
		}
	file_data_increment_version(fd);
	file_data_send_notification(fd, NOTIFY_CHANGE);

	return TRUE;
}

/*
 * notify other modules about the change described by FileDataChangeInfo
 */

/* might use file_maint_ functions for now, later it should be changed to a system of callbacks */
/** @FIXME do we need the ignore_list? It looks like a workaround for ineffective
   implementation in view_file_list.c */


typedef struct _NotifyIdleData NotifyIdleData;

struct _NotifyIdleData {
	FileData *fd;
	NotifyType type;
};


typedef struct _NotifyData NotifyData;

struct _NotifyData {
	FileDataNotifyFunc func;
	gpointer data;
	NotifyPriority priority;
};

static GList *notify_func_list = NULL;

/*static*/ gint FileData::file_data_notify_sort(gconstpointer a, gconstpointer b)
{
	NotifyData *nda = (NotifyData *)a;
	NotifyData *ndb = (NotifyData *)b;

	if (nda->priority < ndb->priority) return -1;
	if (nda->priority > ndb->priority) return 1;
	return 0;
}

gboolean FileData::file_data_register_notify_func(FileDataNotifyFunc func, gpointer data, NotifyPriority priority)
{
	NotifyData *nd;
	GList *work = notify_func_list;

	while (work)
		{
		NotifyData *nd = (NotifyData *)work->data;

		if (nd->func == func && nd->data == data)
			{
			g_warning("Notify func already registered");
			return FALSE;
			}
		work = work->next;
		}

	nd = g_new(NotifyData, 1);
	nd->func = func;
	nd->data = data;
	nd->priority = priority;

	notify_func_list = g_list_insert_sorted(notify_func_list, nd, file_data_notify_sort);
	DEBUG_2("Notify func registered: %p", nd);

	return TRUE;
}

gboolean FileData::file_data_unregister_notify_func(FileDataNotifyFunc func, gpointer data)
{
	GList *work = notify_func_list;

	while (work)
		{
		NotifyData *nd = (NotifyData *)work->data;

		if (nd->func == func && nd->data == data)
			{
			notify_func_list = g_list_delete_link(notify_func_list, work);
			g_free(nd);
			DEBUG_2("Notify func unregistered: %p", nd);
			return TRUE;
			}
		work = work->next;
		}

	g_warning("Notify func not found");
	return FALSE;
}


gboolean FileData::file_data_send_notification_idle_cb(gpointer data)
{
	NotifyIdleData *nid = (NotifyIdleData *)data;
	GList *work = notify_func_list;

	while (work)
		{
		NotifyData *nd = (NotifyData *)work->data;

		nd->func(nid->fd, nid->type, nd->data);
		work = work->next;
		}
	file_data_unref(nid->fd);
	g_free(nid);
	return FALSE;
}

void FileData::file_data_send_notification(FileData *fd, NotifyType type)
{
	GList *work = notify_func_list;

	while (work)
		{
		NotifyData *nd = (NotifyData *)work->data;

		nd->func(fd, type, nd->data);
		work = work->next;
		}
    /*
	NotifyIdleData *nid = g_new0(NotifyIdleData, 1);
	nid->fd = file_data_ref(fd);
	nid->type = type;
	g_idle_add_full(G_PRIORITY_HIGH, file_data_send_notification_idle_cb, nid, NULL);
    */
}

void FileData::file_data_change_info_free(FileDataChangeInfo *fdci, FileData *fd)
{
	if (!fdci && fd) fdci = fd->change;

	if (!fdci) return;

	g_free(fdci->source);
	g_free(fdci->dest);

	g_free(fdci);

	if (fd) fd->change = NULL;
}

