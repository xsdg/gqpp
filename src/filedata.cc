/*
 * Copyright (C) 2006 John Ellis
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

#include "main.h"
#include "filedata.h"

#include "filefilter.h"
#include "cache.h"
#include "thumb-standard.h"
#include "ui-fileops.h"
#include "metadata.h"
#include "trash.h"
#include "histogram.h"
#include "secure-save.h"

#include "exif.h"
#include "misc.h"

#include <errno.h>
#include <grp.h>

#ifdef DEBUG_FILEDATA
gint global_file_data_count = 0;
#endif

static GHashTable *file_data_pool = NULL;
static GHashTable *file_data_planned_change_hash = NULL;

static gint sidecar_file_priority(const gchar *extension);
static void file_data_check_sidecars(const GList *basename_list);
static void file_data_disconnect_sidecar_file(FileData *target, FileData *sfd);


static SortType filelist_sort_method = SORT_NONE;
static gboolean filelist_sort_ascend = TRUE;

/*
 *-----------------------------------------------------------------------------
 * text conversion utils
 *-----------------------------------------------------------------------------
 */

gchar *text_from_size(gint64 size)
{
	gchar *a, *b;
	gchar *s, *d;
	gint l, n, i;

	/* what I would like to use is printf("%'d", size)
	 * BUT: not supported on every libc :(
	 */
	if (size > G_MAXINT)
		{
		/* the %lld conversion is not valid in all libcs, so use a simple work-around */
		a = g_strdup_printf("%d%09d", (guint)(size / 1000000000), (guint)(size % 1000000000));
		}
	else
		{
		a = g_strdup_printf("%d", (guint)size);
		}
	l = strlen(a);
	n = (l - 1)/ 3;
	if (n < 1) return a;

	b = g_new(gchar, l + n + 1);

	s = a;
	d = b;
	i = l - n * 3;
	while (*s != '\0')
		{
		if (i < 1)
			{
			i = 3;
			*d = ',';
			d++;
			}

		*d = *s;
		s++;
		d++;
		i--;
		}
	*d = '\0';

	g_free(a);
	return b;
}

gchar *text_from_size_abrev(gint64 size)
{
	if (size < (gint64)1024)
		{
		return g_strdup_printf(_("%d bytes"), (gint)size);
		}
	if (size < (gint64)1048576)
		{
		return g_strdup_printf(_("%.1f KiB"), (gdouble)size / 1024.0);
		}
	if (size < (gint64)1073741824)
		{
		return g_strdup_printf(_("%.1f MiB"), (gdouble)size / 1048576.0);
		}

	/* to avoid overflowing the gdouble, do division in two steps */
	size /= 1048576;
	return g_strdup_printf(_("%.1f GiB"), (gdouble)size / 1024.0);
}

/* note: returned string is valid until next call to text_from_time() */
const gchar *text_from_time(time_t t)
{
	static gchar *ret = NULL;
	gchar buf[128];
	gint buflen;
	struct tm *btime;
	GError *error = NULL;

	btime = localtime(&t);

	/* the %x warning about 2 digit years is not an error */
	buflen = strftime(buf, sizeof(buf), "%x %X", btime);
	if (buflen < 1) return "";

	g_free(ret);
	ret = g_locale_to_utf8(buf, buflen, NULL, NULL, &error);
	if (error)
		{
		log_printf("Error converting locale strftime to UTF-8: %s\n", error->message);
		g_error_free(error);
		return "";
		}

	return ret;
}

/*
 *-----------------------------------------------------------------------------
 * changed files detection and notification
 *-----------------------------------------------------------------------------
 */

void file_data_increment_version(FileData *fd)
{
	fd->version++;
	fd->valid_marks = 0;
	if (fd->parent)
		{
		fd->parent->version++;
		fd->parent->valid_marks = 0;
		}
}

static gboolean file_data_check_changed_single_file(FileData *fd, struct stat *st)
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

static gboolean file_data_check_changed_files_recursive(FileData *fd, struct stat *st)
{
	gboolean ret = FALSE;
	GList *work;

	ret = file_data_check_changed_single_file(fd, st);

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;
		struct stat st;
		work = work->next;

		if (!stat_utf8(sfd->path, &st))
			{
			fd->size = 0;
			fd->date = 0;
			file_data_ref(sfd);
			file_data_disconnect_sidecar_file(fd, sfd);
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


gboolean file_data_check_changed_files(FileData *fd)
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
		sidecars = filelist_copy(fd->sidecar_files);
		file_data_ref(fd);
		work = sidecars;
		while (work)
			{
			sfd = (FileData *)work->data;
			work = work->next;

			file_data_disconnect_sidecar_file(fd, sfd);
			}
		file_data_check_sidecars(sidecars); /* this will group the sidecars back together */
		/* now we can release the sidecars */
		filelist_free(sidecars);
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

/*
 *-----------------------------------------------------------------------------
 * file name, extension, sorting, ...
 *-----------------------------------------------------------------------------
 */

static void file_data_set_collate_keys(FileData *fd)
{
	gchar *caseless_name;
	gchar *valid_name;

	valid_name = g_filename_display_name(fd->name);
	caseless_name = g_utf8_casefold(valid_name, -1);

	g_free(fd->collate_key_name);
	g_free(fd->collate_key_name_nocase);

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

	g_free(valid_name);
	g_free(caseless_name);
}

static void file_data_set_path(FileData *fd, const gchar *path)
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

	fd->sidecar_priority = sidecar_file_priority(fd->extension);
	file_data_set_collate_keys(fd);
}

/*
 *-----------------------------------------------------------------------------
 * create or reuse Filedata
 *-----------------------------------------------------------------------------
 */

static FileData *file_data_new(const gchar *path_utf8, struct stat *st, gboolean disable_sidecars)
{
	FileData *fd;
	struct passwd *user;
	struct group *group;

	DEBUG_2("file_data_new: '%s' %d", path_utf8, disable_sidecars);

	if (S_ISDIR(st->st_mode)) disable_sidecars = TRUE;

	if (!file_data_pool)
		file_data_pool = g_hash_table_new(g_str_hash, g_str_equal);

	fd = (FileData *)g_hash_table_lookup(file_data_pool, path_utf8);
	if (fd)
		{
		file_data_ref(fd);
		}

	if (!fd && file_data_planned_change_hash)
		{
		fd = (FileData *)g_hash_table_lookup(file_data_planned_change_hash, path_utf8);
		if (fd)
			{
			DEBUG_1("planned change: using %s -> %s", path_utf8, fd->path);
			if (!isfile(fd->path))
				{
				file_data_ref(fd);
				file_data_apply_ci(fd);
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

		if (disable_sidecars) file_data_disable_grouping(fd, TRUE);


		changed = file_data_check_changed_single_file(fd, st);

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

	file_data_set_path(fd, path_utf8); /* set path, name, collate_key_*, original_path */

	return fd;
}

static FileData *file_data_new_local(const gchar *path, struct stat *st, gboolean disable_sidecars)
{
	gchar *path_utf8 = path_to_utf8(path);
	FileData *ret = file_data_new(path_utf8, st, disable_sidecars);

	g_free(path_utf8);
	return ret;
}

FileData *file_data_new_simple(const gchar *path_utf8)
{
	struct stat st;
	FileData *fd;

	if (!stat_utf8(path_utf8, &st))
		{
		st.st_size = 0;
		st.st_mtime = 0;
		}

	fd = (FileData *)g_hash_table_lookup(file_data_pool, path_utf8);
	if (!fd) fd = file_data_new(path_utf8, &st, TRUE);
	if (fd)
		{
		file_data_ref(fd);
		}

	return fd;
}

void read_exif_time_data(FileData *file)
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
		DEBUG_2("%s set_exif_time_data: reading %p %s", get_exec_time(), (void *)file, file->path);

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

void read_exif_time_digitized_data(FileData *file)
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
		DEBUG_2("%s set_exif_time_digitized_data: reading %p %s", get_exec_time(), (void *)file, file->path);

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

void read_rating_data(FileData *file)
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

void set_exif_time_data(GList *files)
{
	DEBUG_1("%s set_exif_time_data: ...", get_exec_time());

	while (files)
		{
		FileData *file = (FileData *)files->data;

		read_exif_time_data(file);
		files = files->next;
		}
}

void set_exif_time_digitized_data(GList *files)
{
	DEBUG_1("%s set_exif_time_digitized_data: ...", get_exec_time());

	while (files)
		{
		FileData *file = (FileData *)files->data;

		read_exif_time_digitized_data(file);
		files = files->next;
		}
}

void set_rating_data(GList *files)
{
	gchar *rating_str;
	DEBUG_1("%s set_rating_data: ...", get_exec_time());

	while (files)
		{
		FileData *file = (FileData *)files->data;
		rating_str = metadata_read_string(file, RATING_KEY, METADATA_PLAIN);
		if (rating_str )
			{
			file->rating = atoi(rating_str);
			g_free(rating_str);
			}
		files = files->next;
		}
}

FileData *file_data_new_no_grouping(const gchar *path_utf8)
{
	struct stat st;

	if (!stat_utf8(path_utf8, &st))
		{
		st.st_size = 0;
		st.st_mtime = 0;
		}

	return file_data_new(path_utf8, &st, TRUE);
}

FileData *file_data_new_dir(const gchar *path_utf8)
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
FileData *file_data_ref_debug(const gchar *file, gint line, FileData *fd)
#else
FileData *file_data_ref(FileData *fd)
#endif
{
	if (fd == NULL) return NULL;
	if (fd->magick != FD_MAGICK)
#ifdef DEBUG_FILEDATA
		log_printf("Error: fd magick mismatch @ %s:%d  fd=%p", file, line, (void *)fd);
#else
		log_printf("Error: fd magick mismatch fd=%p", fd);
#endif
	g_assert(fd->magick == FD_MAGICK);
	fd->ref++;

#ifdef DEBUG_FILEDATA
	DEBUG_2("file_data_ref fd=%p (%d): '%s' @ %s:%d", (void *)fd, fd->ref, fd->path, file, line);
#else
	DEBUG_2("file_data_ref fd=%p (%d): '%s'", fd, fd->ref, fd->path);
#endif
	return fd;
}

static void file_data_free(FileData *fd)
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
static gboolean file_data_check_has_ref(FileData *fd)
{
	return fd->ref > 0 || fd->locked;
}

/**
 * @brief Consider freeing a FileData.
 *
 * This function will free a FileData and its children provided that neither its parent nor it has
 * a positive refcount, and provided that neither is locked.
 */
static void file_data_consider_free(FileData *fd)
{
	GList *work;
	FileData *parent = fd->parent ? fd->parent : fd;

	g_assert(fd->magick == FD_MAGICK);
	if (file_data_check_has_ref(fd)) return;
	if (file_data_check_has_ref(parent)) return;

	work = parent->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;
		if (file_data_check_has_ref(sfd)) return;
		work = work->next;
		}

	/* Neither the parent nor the siblings are referenced, so we can free everything */
	DEBUG_2("file_data_consider_free: deleting '%s', parent '%s'",
		fd->path, fd->parent ? parent->path : "-");

	work = parent->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;
		file_data_free(sfd);
		work = work->next;
		}

	g_list_free(parent->sidecar_files);
	parent->sidecar_files = NULL;

	file_data_free(parent);
}

#ifdef DEBUG_FILEDATA
void file_data_unref_debug(const gchar *file, gint line, FileData *fd)
#else
void file_data_unref(FileData *fd)
#endif
{
	if (fd == NULL) return;
	if (fd->magick != FD_MAGICK)
#ifdef DEBUG_FILEDATA
		log_printf("Error: fd magick mismatch @ %s:%d  fd=%p", file, line, (void *)fd);
#else
		log_printf("Error: fd magick mismatch fd=%p", fd);
#endif
	g_assert(fd->magick == FD_MAGICK);

	fd->ref--;
#ifdef DEBUG_FILEDATA
	DEBUG_2("file_data_unref fd=%p (%d:%d): '%s' @ %s:%d", (void *)fd, fd->ref, fd->locked, fd->path,
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
void file_data_lock(FileData *fd)
{
	if (fd == NULL) return;
	if (fd->magick != FD_MAGICK) log_printf("Error: fd magick mismatch fd=%p", (void *)fd);

	g_assert(fd->magick == FD_MAGICK);
	fd->locked = TRUE;

	DEBUG_2("file_data_ref fd=%p (%d): '%s'", (void *)fd, fd->ref, fd->path);
}

/**
 * @brief Reset the maintain-FileData-in-memory lock
 *
 * This again allows the FileData to be freed when its refcount drops to zero.  Automatically frees
 * the FileData if its refcount is already zero (which will happen if the lock is the only thing
 * keeping it from being freed.
 */
void file_data_unlock(FileData *fd)
{
	if (fd == NULL) return;
	if (fd->magick != FD_MAGICK) log_printf("Error: fd magick mismatch fd=%p", (void *)fd);

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
void file_data_lock_list(GList *list)
{
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd = (FileData *)work->data;
		work = work->next;
		file_data_lock(fd);
		}
}

/**
 * @brief Unlock all of the FileDatas in the provided list
 *
 * @see #file_data_unlock(#FileData)
 */
void file_data_unlock_list(GList *list)
{
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd = (FileData *)work->data;
		work = work->next;
		file_data_unlock(fd);
		}
}

/*
 *-----------------------------------------------------------------------------
 * sidecar file info struct
 *-----------------------------------------------------------------------------
 */

static gint file_data_sort_by_ext(gconstpointer a, gconstpointer b)
{
	const FileData *fda = (const FileData *)a;
	const FileData *fdb = (const FileData *)b;

	if (fda->sidecar_priority < fdb->sidecar_priority) return -1;
	if (fda->sidecar_priority > fdb->sidecar_priority) return 1;

	return strcmp(fdb->extension, fda->extension);
}


static gint sidecar_file_priority(const gchar *extension)
{
	gint i = 1;
	GList *work;

	if (extension == NULL)
		return 0;

	work = sidecar_ext_get_list();

	while (work) {
		gchar *ext = (gchar *)work->data;

		work = work->next;
		if (g_ascii_strcasecmp(extension, ext) == 0) return i;
		i++;
	}
	return 0;
}

static void file_data_check_sidecars(const GList *basename_list)
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
		FileData *fd = (FileData *)work->data;
		work = work->next;
		g_assert(fd->magick == FD_MAGICK);
		DEBUG_2("basename: %p %s", (void *)fd, fd->name);
		if (fd->parent)
			{
			g_assert(fd->parent->magick == FD_MAGICK);
			DEBUG_2("                  parent: %p", (void *)fd->parent);
			}
		s_work = fd->sidecar_files;
		while (s_work)
			{
			FileData *sfd = (FileData *)s_work->data;
			s_work = s_work->next;
			g_assert(sfd->magick == FD_MAGICK);
			DEBUG_2("                  sidecar: %p %s", (void *)sfd, sfd->name);
			}

		g_assert(fd->parent == NULL || fd->sidecar_files == NULL);
		}

	parent_fd = (FileData *)basename_list->data;

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
		FileData *fd = (FileData *)work->data;
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
			FileData *sfd = (FileData *)fd->sidecar_files->data;
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
		FileData *sfd = (FileData *)work->data;
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


static void file_data_disconnect_sidecar_file(FileData *target, FileData *sfd)
{
	g_assert(target->magick == FD_MAGICK);
	g_assert(sfd->magick == FD_MAGICK);
	g_assert(g_list_find(target->sidecar_files, sfd));

	file_data_ref(target);
	file_data_ref(sfd);

	g_assert(sfd->parent == target);

	file_data_increment_version(sfd); /* increments both sfd and target */

	target->sidecar_files = g_list_remove(target->sidecar_files, sfd);
	sfd->parent = NULL;
	g_free(sfd->extended_extension);
	sfd->extended_extension = NULL;

	file_data_unref(target);
	file_data_unref(sfd);
}

/* disables / enables grouping for particular file, sends UPDATE notification */
void file_data_disable_grouping(FileData *fd, gboolean disable)
{
	if (!fd->disable_grouping == !disable) return;

	fd->disable_grouping = !!disable;

	if (disable)
		{
		if (fd->parent)
			{
			FileData *parent = (FileData*)file_data_ref(fd->parent);
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
				FileData *sfd = (FileData *)work->data;
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

void file_data_disable_grouping_list(GList *fd_list, gboolean disable)
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = (FileData *)work->data;

		file_data_disable_grouping(fd, disable);
		work = work->next;
		}
}



/*
 *-----------------------------------------------------------------------------
 * filelist sorting
 *-----------------------------------------------------------------------------
 */


gint filelist_sort_compare_filedata(FileData *fa, FileData *fb)
{
	gint ret;
	if (!filelist_sort_ascend)
		{
		FileData *tmp = fa;
		fa = fb;
		fb = tmp;
		}

	switch (filelist_sort_method)
		{
		case SORT_NAME:
			break;
		case SORT_SIZE:
			if (fa->size < fb->size) return -1;
			if (fa->size > fb->size) return 1;
			/* fall back to name */
			break;
		case SORT_TIME:
			if (fa->date < fb->date) return -1;
			if (fa->date > fb->date) return 1;
			/* fall back to name */
			break;
		case SORT_CTIME:
			if (fa->cdate < fb->cdate) return -1;
			if (fa->cdate > fb->cdate) return 1;
			/* fall back to name */
			break;
		case SORT_EXIFTIME:
			if (fa->exifdate < fb->exifdate) return -1;
			if (fa->exifdate > fb->exifdate) return 1;
			/* fall back to name */
			break;
		case SORT_EXIFTIMEDIGITIZED:
			if (fa->exifdate_digitized < fb->exifdate_digitized) return -1;
			if (fa->exifdate_digitized > fb->exifdate_digitized) return 1;
			/* fall back to name */
			break;
		case SORT_RATING:
			if (fa->rating < fb->rating) return -1;
			if (fa->rating > fb->rating) return 1;
			/* fall back to name */
			break;
		case SORT_CLASS:
			if (fa->format_class < fb->format_class) return -1;
			if (fa->format_class > fb->format_class) return 1;
			/* fall back to name */
			break;
#ifdef HAVE_STRVERSCMP
		case SORT_NUMBER:
			ret = strverscmp(fa->name, fb->name);
			if (ret != 0) return ret;
			break;
#endif
		default:
			break;
		}

	if (options->file_sort.case_sensitive)
		ret = strcmp(fa->collate_key_name, fb->collate_key_name);
	else
		ret = strcmp(fa->collate_key_name_nocase, fb->collate_key_name_nocase);

	if (ret != 0) return ret;

	/* do not return 0 unless the files are really the same
	   file_data_pool ensures that original_path is unique
	*/
	return strcmp(fa->original_path, fb->original_path);
}

gint filelist_sort_compare_filedata_full(FileData *fa, FileData *fb, SortType method, gboolean ascend)
{
	filelist_sort_method = method;
	filelist_sort_ascend = ascend;
	return filelist_sort_compare_filedata(fa, fb);
}

static gint filelist_sort_file_cb(gpointer a, gpointer b)
{
	return filelist_sort_compare_filedata(a, b);
}

GList *filelist_sort_full(GList *list, SortType method, gboolean ascend, GCompareFunc cb)
{
	filelist_sort_method = method;
	filelist_sort_ascend = ascend;
	return g_list_sort(list, cb);
}

GList *filelist_insert_sort_full(GList *list, gpointer data, SortType method, gboolean ascend, GCompareFunc cb)
{
	filelist_sort_method = method;
	filelist_sort_ascend = ascend;
	return g_list_insert_sorted(list, data, cb);
}

GList *filelist_sort(GList *list, SortType method, gboolean ascend)
{
	return filelist_sort_full(list, method, ascend, (GCompareFunc) filelist_sort_file_cb);
}

GList *filelist_insert_sort(GList *list, FileData *fd, SortType method, gboolean ascend)
{
	return filelist_insert_sort_full(list, fd, method, ascend, (GCompareFunc) filelist_sort_file_cb);
}

/*
 *-----------------------------------------------------------------------------
 * basename hash - grouping of sidecars in filelist
 *-----------------------------------------------------------------------------
 */


static GHashTable *file_data_basename_hash_new(void)
{
	return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

static GList * file_data_basename_hash_insert(GHashTable *basename_hash, FileData *fd)
{
	GList *list;
	gchar *basename = g_strndup(fd->path, fd->extension - fd->path);

	list = (GList *)g_hash_table_lookup(basename_hash, basename);

	if (!list)
		{
		DEBUG_1("TG: basename_hash not found for %s",fd->path);
		const gchar *parent_extension = registered_extension_from_path(basename);

		if (parent_extension)
			{
			DEBUG_1("TG: parent extension %s",parent_extension);
			gchar *parent_basename = g_strndup(basename, parent_extension - basename);
			DEBUG_1("TG: parent basename %s",parent_basename);
			FileData *parent_fd = (FileData *)g_hash_table_lookup(file_data_pool, basename);
			if (parent_fd)
				{
				DEBUG_1("TG: parent fd found");
				list = (GList *)g_hash_table_lookup(basename_hash, parent_basename);
				if (!g_list_find(list, parent_fd))
					{
					DEBUG_1("TG: parent fd doesn't fit");
					g_free(parent_basename);
					list = NULL;
					}
				else
					{
					g_free(basename);
					basename = parent_basename;
					fd->extended_extension = g_strconcat(parent_extension, fd->extension, NULL);
					}
				}
			}
		}

	if (!g_list_find(list, fd))
		{
		list = g_list_insert_sorted(list, file_data_ref(fd), file_data_sort_by_ext);
		g_hash_table_insert(basename_hash, basename, list);
		}
	else
		{
		g_free(basename);
		}
	return list;
}

static void file_data_basename_hash_insert_cb(gpointer fd, gpointer basename_hash)
{
	file_data_basename_hash_insert((GHashTable *)basename_hash, (FileData *)fd);
}

static void file_data_basename_hash_remove_list(gpointer UNUSED(key), gpointer value, gpointer UNUSED(data))
{
	filelist_free((GList *)value);
}

static void file_data_basename_hash_free(GHashTable *basename_hash)
{
	g_hash_table_foreach(basename_hash, file_data_basename_hash_remove_list, NULL);
	g_hash_table_destroy(basename_hash);
}

/*
 *-----------------------------------------------------------------------------
 * handling sidecars in filelist
 *-----------------------------------------------------------------------------
 */

static GList *filelist_filter_out_sidecars(GList *flist)
{
	GList *work = flist;
	GList *flist_filtered = NULL;

	while (work)
		{
		FileData *fd = (FileData *)work->data;

		work = work->next;
		if (fd->parent) /* remove fd's that are children */
			file_data_unref(fd);
		else
			flist_filtered = g_list_prepend(flist_filtered, fd);
		}
	g_list_free(flist);

	return flist_filtered;
}

static void file_data_basename_hash_to_sidecars(gpointer UNUSED(key), gpointer value, gpointer UNUSED(data))
{
	GList *basename_list = (GList *)value;
	file_data_check_sidecars(basename_list);
}


static gboolean is_hidden_file(const gchar *name)
{
	if (name[0] != '.') return FALSE;
	if (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')) return FALSE;
	return TRUE;
}

/*
 *-----------------------------------------------------------------------------
 * the main filelist function
 *-----------------------------------------------------------------------------
 */

static gboolean filelist_read_real(const gchar *dir_path, GList **files, GList **dirs, gboolean follow_symlinks)
{
	DIR *dp;
	struct dirent *dir;
	gchar *pathl;
	GList *dlist = NULL;
	GList *flist = NULL;
	GList *xmp_files = NULL;
	gint (*stat_func)(const gchar *path, struct stat *buf);
	GHashTable *basename_hash = NULL;

	g_assert(files || dirs);

	if (files) *files = NULL;
	if (dirs) *dirs = NULL;

	pathl = path_from_utf8(dir_path);
	if (!pathl) return FALSE;

	dp = opendir(pathl);
	if (dp == NULL)
		{
		g_free(pathl);
		return FALSE;
		}

	if (files) basename_hash = file_data_basename_hash_new();

	if (follow_symlinks)
		stat_func = stat;
	else
		stat_func = lstat;

	while ((dir = readdir(dp)) != NULL)
		{
		struct stat ent_sbuf;
		const gchar *name = dir->d_name;
		gchar *filepath;

		if (!options->file_filter.show_hidden_files && is_hidden_file(name))
			continue;

		filepath = g_build_filename(pathl, name, NULL);
		if (stat_func(filepath, &ent_sbuf) >= 0)
			{
			if (S_ISDIR(ent_sbuf.st_mode))
				{
				/* we ignore the .thumbnails dir for cleanliness */
				if (dirs &&
				    !(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) &&
				    strcmp(name, GQ_CACHE_LOCAL_THUMB) != 0 &&
				    strcmp(name, GQ_CACHE_LOCAL_METADATA) != 0 &&
				    strcmp(name, THUMB_FOLDER_LOCAL) != 0)
					{
					dlist = g_list_prepend(dlist, file_data_new_local(filepath, &ent_sbuf, TRUE));
					}
				}
			else
				{
				if (files && filter_name_exists(name))
					{
					FileData *fd = file_data_new_local(filepath, &ent_sbuf, FALSE);
					flist = g_list_prepend(flist, fd);
					if (fd->sidecar_priority && !fd->disable_grouping)
						{
						if (strcmp(fd->extension, ".xmp") != 0)
							file_data_basename_hash_insert(basename_hash, fd);
						else
							xmp_files = g_list_append(xmp_files, fd);
						}
					}
				}
			}
		else
			{
			if (errno == EOVERFLOW)
				{
				log_printf("stat(): EOVERFLOW, skip '%s'", filepath);
				}
			}
		g_free(filepath);
		}

	closedir(dp);

	g_free(pathl);

	if (xmp_files)
		{
		g_list_foreach(xmp_files,file_data_basename_hash_insert_cb,basename_hash);
		g_list_free(xmp_files);
		}

	if (dirs) *dirs = dlist;

	if (files)
		{
		g_hash_table_foreach(basename_hash, file_data_basename_hash_to_sidecars, NULL);

		*files = filelist_filter_out_sidecars(flist);
		}
	if (basename_hash) file_data_basename_hash_free(basename_hash);

	return TRUE;
}

gboolean filelist_read(FileData *dir_fd, GList **files, GList **dirs)
{
	return filelist_read_real(dir_fd->path, files, dirs, TRUE);
}

gboolean filelist_read_lstat(FileData *dir_fd, GList **files, GList **dirs)
{
	return filelist_read_real(dir_fd->path, files, dirs, FALSE);
}

FileData *file_data_new_group(const gchar *path_utf8)
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

	filelist_read_real(dir, &files, NULL, TRUE);

	fd = (FileData *)g_hash_table_lookup(file_data_pool, path_utf8);
	if (!fd) fd = file_data_new(path_utf8, &st, TRUE);
	if (fd)
		{
		file_data_ref(fd);
		}

	filelist_free(files);
	g_free(dir);
	return fd;
}


void filelist_free(GList *list)
{
	GList *work;

	work = list;
	while (work)
		{
		file_data_unref((FileData *)work->data);
		work = work->next;
		}

	g_list_free(list);
}


GList *filelist_copy(GList *list)
{
	GList *new_list = NULL;
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd;

		fd = (FileData *)work->data;
		work = work->next;

		new_list = g_list_prepend(new_list, file_data_ref(fd));
		}

	return g_list_reverse(new_list);
}

GList *filelist_from_path_list(GList *list)
{
	GList *new_list = NULL;
	GList *work;

	work = list;
	while (work)
		{
		gchar *path;

		path = (gchar *)work->data;
		work = work->next;

		new_list = g_list_prepend(new_list, file_data_new_group(path));
		}

	return g_list_reverse(new_list);
}

GList *filelist_to_path_list(GList *list)
{
	GList *new_list = NULL;
	GList *work;

	work = list;
	while (work)
		{
		FileData *fd;

		fd = (FileData *)work->data;
		work = work->next;

		new_list = g_list_prepend(new_list, g_strdup(fd->path));
		}

	return g_list_reverse(new_list);
}

GList *filelist_filter(GList *list, gboolean is_dir_list)
{
	GList *work;

	if (!is_dir_list && options->file_filter.disable && options->file_filter.show_hidden_files) return list;

	work = list;
	while (work)
		{
		FileData *fd = (FileData *)(work->data);
		const gchar *name = fd->name;

		if ((!options->file_filter.show_hidden_files && is_hidden_file(name)) ||
		    (!is_dir_list && !filter_name_exists(name)) ||
		    (is_dir_list && name[0] == '.' && (strcmp(name, GQ_CACHE_LOCAL_THUMB) == 0 ||
						       strcmp(name, GQ_CACHE_LOCAL_METADATA) == 0)) )
			{
			GList *link = work;

			list = g_list_remove_link(list, link);
			file_data_unref(fd);
			g_list_free(link);
			}

		work = work->next;
		}

	return list;
}

/*
 *-----------------------------------------------------------------------------
 * filelist recursive
 *-----------------------------------------------------------------------------
 */

static gint filelist_sort_path_cb(gconstpointer a, gconstpointer b)
{
	return CASE_SORT(((FileData *)a)->path, ((FileData *)b)->path);
}

GList *filelist_sort_path(GList *list)
{
	return g_list_sort(list, filelist_sort_path_cb);
}

static void filelist_recursive_append(GList **list, GList *dirs)
{
	GList *work;

	work = dirs;
	while (work)
		{
		FileData *fd = (FileData *)(work->data);
		GList *f;
		GList *d;

		if (filelist_read(fd, &f, &d))
			{
			f = filelist_filter(f, FALSE);
			f = filelist_sort_path(f);
			*list = g_list_concat(*list, f);

			d = filelist_filter(d, TRUE);
			d = filelist_sort_path(d);
			filelist_recursive_append(list, d);
			filelist_free(d);
			}

		work = work->next;
		}
}

static void filelist_recursive_append_full(GList **list, GList *dirs, SortType method, gboolean ascend)
{
	GList *work;

	work = dirs;
	while (work)
		{
		FileData *fd = (FileData *)(work->data);
		GList *f;
		GList *d;

		if (filelist_read(fd, &f, &d))
			{
			f = filelist_filter(f, FALSE);
			f = filelist_sort_full(f, method, ascend, (GCompareFunc) filelist_sort_file_cb);
			*list = g_list_concat(*list, f);

			d = filelist_filter(d, TRUE);
			d = filelist_sort_path(d);
			filelist_recursive_append_full(list, d, method, ascend);
			filelist_free(d);
			}

		work = work->next;
		}
}

GList *filelist_recursive(FileData *dir_fd)
{
	GList *list;
	GList *d;

	if (!filelist_read(dir_fd, &list, &d)) return NULL;
	list = filelist_filter(list, FALSE);
	list = filelist_sort_path(list);

	d = filelist_filter(d, TRUE);
	d = filelist_sort_path(d);
	filelist_recursive_append(&list, d);
	filelist_free(d);

	return list;
}

GList *filelist_recursive_full(FileData *dir_fd, SortType method, gboolean ascend)
{
	GList *list;
	GList *d;

	if (!filelist_read(dir_fd, &list, &d)) return NULL;
	list = filelist_filter(list, FALSE);
	list = filelist_sort_full(list, method, ascend, (GCompareFunc) filelist_sort_file_cb);

	d = filelist_filter(d, TRUE);
	d = filelist_sort_path(d);
	filelist_recursive_append_full(&list, d, method, ascend);
	filelist_free(d);

	return list;
}

/*
 *-----------------------------------------------------------------------------
 * file modification support
 *-----------------------------------------------------------------------------
 */


void file_data_change_info_free(FileDataChangeInfo *fdci, FileData *fd)
{
	if (!fdci && fd) fdci = fd->change;

	if (!fdci) return;

	g_free(fdci->source);
	g_free(fdci->dest);

	g_free(fdci);

	if (fd) fd->change = NULL;
}

static gboolean file_data_can_write_directly(FileData *fd)
{
	return filter_name_is_writable(fd->extension);
}

static gboolean file_data_can_write_sidecar(FileData *fd)
{
	return filter_name_allow_sidecar(fd->extension) && !filter_name_is_writable(fd->extension);
}

gchar *file_data_get_sidecar_path(FileData *fd, gboolean existing_only)
{
	gchar *sidecar_path = NULL;
	GList *work;

	if (!file_data_can_write_sidecar(fd)) return NULL;

	work = fd->parent ? fd->parent->sidecar_files : fd->sidecar_files;
	gchar *extended_extension = g_strconcat(fd->parent ? fd->parent->extension : fd->extension, ".xmp", NULL);
	while (work)
		{
		FileData *sfd = (FileData *)work->data;
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

/*
 * marks and orientation
 */

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
		FileData *fd = (FileData *)work->data;
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
		FileData *fd = (FileData *)work->data;
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

static gboolean file_data_filter_class(FileData *fd, guint filter)
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
		FileData *fd = (FileData *)work->data;
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

static void file_data_notify_mark_func(gpointer UNUSED(key), gpointer value, gpointer UNUSED(user_data))
{
	FileData *fd = (FileData *)value;
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
	if (get_mark_func) *get_mark_func = ((get_mark_func)*)file_data_get_mark_func[n];
	if (set_mark_func) *set_mark_func = ((set_mark_func)*)file_data_set_mark_func[n];
	if (data) *data = ((data)*)file_data_mark_func_data[n];
}

gint file_data_get_user_orientation(FileData *fd)
{
	return fd->user_orientation;
}

void file_data_set_user_orientation(FileData *fd, gint value)
{
	if (fd->user_orientation == value) return;

	fd->user_orientation = value;
	file_data_increment_version(fd);
	file_data_send_notification(fd, NOTIFY_ORIENTATION);
}


/*
 * file_data    - operates on the given fd
 * file_data_sc - operates on the given fd + sidecars - all fds linked via fd->sidecar_files or fd->parent
 */


/* return list of sidecar file extensions in a string */
gchar *file_data_sc_list_to_string(FileData *fd)
{
	GList *work;
	GString *result = g_string_new("");

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;

		result = g_string_append(result, "+ ");
		result = g_string_append(result, sfd->extension);
		work = work->next;
		if (work) result = g_string_append_c(result, ' ');
		}

	return g_string_free(result, FALSE);
}



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

gboolean file_data_add_ci(FileData *fd, FileDataChangeType type, const gchar *src, const gchar *dest)
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

static void file_data_planned_change_remove(FileData *fd)
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


void file_data_free_ci(FileData *fd)
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

void file_data_set_regroup_when_finished(FileData *fd, gboolean enable)
{
	FileDataChangeInfo *fdci = fd->change;
	if (!fdci) return;
	fdci->regroup_when_finished = enable;
}

static gboolean file_data_sc_add_ci(FileData *fd, FileDataChangeType type)
{
	GList *work;

	if (fd->parent) fd = fd->parent;

	if (fd->change) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;

		if (sfd->change) return FALSE;
		work = work->next;
		}

	file_data_add_ci(fd, type, NULL, NULL);

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;

		file_data_add_ci(sfd, type, NULL, NULL);
		work = work->next;
		}

	return TRUE;
}

static gboolean file_data_sc_check_ci(FileData *fd, FileDataChangeType type)
{
	GList *work;

	if (fd->parent) fd = fd->parent;

	if (!fd->change || fd->change->type != type) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;

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
		FileData *sfd = (FileData *)work->data;

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
		FileData *fd = (FileData *)work->data;

		if (!file_data_sc_add_ci_delete(fd)) ret = FALSE;
		work = work->next;
		}

	return ret;
}

static void file_data_sc_revert_ci_list(GList *fd_list)
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = (FileData *)work->data;

		file_data_sc_free_ci(fd);
		work = work->prev;
		}
}

static gboolean file_data_sc_add_ci_list_call_func(GList *fd_list, const gchar *dest, gboolean (*func)(FileData *, const gchar *))
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = (FileData *)work->data;

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

gboolean file_data_add_ci_write_metadata_list(GList *fd_list)
{
	GList *work;
	gboolean ret = TRUE;

	work = fd_list;
	while (work)
		{
		FileData *fd = (FileData *)work->data;

		if (!file_data_add_ci_write_metadata(fd)) ret = FALSE;
		work = work->next;
		}

	return ret;
}

void file_data_free_ci_list(GList *fd_list)
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = (FileData *)work->data;

		file_data_free_ci(fd);
		work = work->next;
		}
}

void file_data_sc_free_ci_list(GList *fd_list)
{
	GList *work;

	work = fd_list;
	while (work)
		{
		FileData *fd = (FileData *)work->data;

		file_data_sc_free_ci(fd);
		work = work->next;
		}
}

/*
 * update existing fd->change, it will be used from dialog callbacks for interactive editing
 * fails if fd->change does not exist or the change type does not match
 */

static void file_data_update_planned_change_hash(FileData *fd, const gchar *old_path, gchar *new_path)
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

		ofd = (FileData *)g_hash_table_lookup(file_data_planned_change_hash, new_path);
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

static void file_data_update_ci_dest(FileData *fd, const gchar *dest_path)
{
	gchar *old_path = fd->change->dest;

	fd->change->dest = g_strdup(dest_path);
	file_data_update_planned_change_hash(fd, old_path, fd->change->dest);
	g_free(old_path);
}

static void file_data_update_ci_dest_preserve_ext(FileData *fd, const gchar *dest_path)
{
	const gchar *extension = registered_extension_from_path(fd->change->source);
	gchar *base = remove_extension_from_path(dest_path);
	gchar *old_path = fd->change->dest;

	fd->change->dest = g_strconcat(base, fd->extended_extension ? fd->extended_extension : extension, NULL);
	file_data_update_planned_change_hash(fd, old_path, fd->change->dest);

	g_free(old_path);
	g_free(base);
}

static void file_data_sc_update_ci(FileData *fd, const gchar *dest_path)
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
		FileData *sfd = (FileData *)work->data;

		file_data_update_ci_dest_preserve_ext(sfd, dest_path);
		work = work->next;
		}

	g_free(dest_path_full);
}

static gboolean file_data_sc_check_update_ci(FileData *fd, const gchar *dest_path, FileDataChangeType type)
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
		FileData *fd = (FileData *)work->data;

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


/*
 * verify source and dest paths - dest image exists, etc.
 * it should detect all possible problems with the planned operation
 */

gint file_data_verify_ci(FileData *fd, GList *list)
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
			if (file_data_can_write_directly(fd))
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
			else if (file_data_can_write_sidecar(fd))
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
			fd1 = (FileData *)work->data;
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


gint file_data_sc_verify_ci(FileData *fd, GList *list)
{
	GList *work;
	gint ret;

	ret = file_data_verify_ci(fd, list);

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;

		ret |= file_data_verify_ci(sfd, list);
		work = work->next;
		}

	return ret;
}

gchar *file_data_get_error_string(gint error)
{
	GString *result = g_string_new("");

	if (error & CHANGE_NO_SRC)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("file or directory does not exist"));
		}

	if (error & CHANGE_DEST_EXISTS)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination already exists"));
		}

	if (error & CHANGE_NO_WRITE_PERM_DEST)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination can't be overwritten"));
		}

	if (error & CHANGE_WARN_NO_WRITE_PERM_DEST_DIR)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination directory is not writable"));
		}

	if (error & CHANGE_NO_DEST_DIR)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination directory does not exist"));
		}

	if (error & CHANGE_NO_WRITE_PERM_DIR)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("source directory is not writable"));
		}

	if (error & CHANGE_NO_READ_PERM)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("no read permission"));
		}

	if (error & CHANGE_WARN_NO_WRITE_PERM)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("file is readonly"));
		}

	if (error & CHANGE_WARN_DEST_EXISTS)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination already exists and will be overwritten"));
		}

	if (error & CHANGE_WARN_SAME)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("source and destination are the same"));
		}

	if (error & CHANGE_WARN_CHANGED_EXT)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("source and destination have different extension"));
		}

	if (error & CHANGE_WARN_UNSAVED_META)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("there are unsaved metadata changes for the file"));
		}

	if (error & CHANGE_DUPLICATE_DEST)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("another destination file has the same filename"));
		}

	return g_string_free(result, FALSE);
}

gint file_data_verify_ci_list(GList *list, gchar **desc, gboolean with_sidecars)
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

		fd = (FileData *)work->data;
		work = work->next;

		error = with_sidecars ? file_data_sc_verify_ci(fd, list) : file_data_verify_ci(fd, list);
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
			gchar *str = (gchar*)file_data_get_error_string(common_errors);
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

			fd = (FileData *)work->data;
			work = work->next;

			error = errors[i] & ~common_errors;

			if (error)
				{
				gchar *str = (gchar*)file_data_get_error_string(error);
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

static gboolean file_data_perform_move(FileData *fd)
{
	g_assert(!strcmp(fd->change->source, fd->path));
	return move_file(fd->change->source, fd->change->dest);
}

static gboolean file_data_perform_copy(FileData *fd)
{
	g_assert(!strcmp(fd->change->source, fd->path));
	return copy_file(fd->change->source, fd->change->dest);
}

static gboolean file_data_perform_delete(FileData *fd)
{
	if (isdir(fd->path) && !islink(fd->path))
		return rmdir_utf8(fd->path);
	else
		if (options->file_ops.safe_delete_enable)
			return file_util_safe_unlink(fd->path);
		else
			return unlink_file(fd->path);
}

gboolean file_data_perform_ci(FileData *fd)
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



gboolean file_data_sc_perform_ci(FileData *fd)
{
	GList *work;
	gboolean ret = TRUE;
	FileDataChangeType type = fd->change->type;

	if (!file_data_sc_check_ci(fd, type)) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;

		if (!file_data_perform_ci(sfd)) ret = FALSE;
		work = work->next;
		}

	if (!file_data_perform_ci(fd)) ret = FALSE;

	return ret;
}

/*
 * updates FileData structure according to FileDataChangeInfo
 */

gboolean file_data_apply_ci(FileData *fd)
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

gboolean file_data_sc_apply_ci(FileData *fd)
{
	GList *work;
	FileDataChangeType type = fd->change->type;

	if (!file_data_sc_check_ci(fd, type)) return FALSE;

	work = fd->sidecar_files;
	while (work)
		{
		FileData *sfd = (FileData *)work->data;

		file_data_apply_ci(sfd);
		work = work->next;
		}

	file_data_apply_ci(fd);

	return TRUE;
}

static gboolean file_data_list_contains_whole_group(GList *list, FileData *fd)
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

GList *file_data_process_groups_in_selection(GList *list, gboolean ungroup, GList **ungrouped_list)
{
	GList *out = NULL;
	GList *work = list;

	/* change partial groups to independent files */
	if (ungroup)
		{
		while (work)
			{
			FileData *fd = (FileData *)work->data;
			work = work->next;

			if (!file_data_list_contains_whole_group(list, fd))
				{
				file_data_disable_grouping(fd, TRUE);
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
		FileData *fd = (FileData *)work->data;
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





/*
 * notify other modules about the change described by FileDataChangeInfo
 */

/* might use file_maint_ functions for now, later it should be changed to a system of callbacks */
/** @FIXME do we need the ignore_list? It looks like a workaround for ineffective
   implementation in view-file-list.cc */


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

static gint file_data_notify_sort(gconstpointer a, gconstpointer b)
{
	NotifyData *nda = (NotifyData *)a;
	NotifyData *ndb = (NotifyData *)b;

	if (nda->priority < ndb->priority) return -1;
	if (nda->priority > ndb->priority) return 1;
	return 0;
}

gboolean file_data_register_notify_func(FileDataNotifyFunc func, gpointer data, NotifyPriority priority)
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
	DEBUG_2("Notify func registered: %p", (void *)nd);

	return TRUE;
}

gboolean file_data_unregister_notify_func(FileDataNotifyFunc func, gpointer data)
{
	GList *work = notify_func_list;

	while (work)
		{
		NotifyData *nd = (NotifyData *)work->data;

		if (nd->func == func && nd->data == data)
			{
			notify_func_list = g_list_delete_link(notify_func_list, work);
			g_free(nd);
			DEBUG_2("Notify func unregistered: %p", (void *)nd);
			return TRUE;
			}
		work = work->next;
		}

	g_warning("Notify func not found");
	return FALSE;
}


gboolean file_data_send_notification_idle_cb(gpointer data)
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

void file_data_send_notification(FileData *fd, NotifyType type)
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

static GHashTable *file_data_monitor_pool = NULL;
static guint realtime_monitor_id = 0; /* event source id */

static void realtime_monitor_check_cb(gpointer key, gpointer UNUSED(value), gpointer UNUSED(data))
{
	FileData *fd = (FileData *)key;

	file_data_check_changed_files(fd);

	DEBUG_1("monitor %s", fd->path);
}

static gboolean realtime_monitor_cb(gpointer UNUSED(data))
{
	if (!options->update_on_time_change) return TRUE;
	g_hash_table_foreach(file_data_monitor_pool, realtime_monitor_check_cb, NULL);
	return TRUE;
}

gboolean file_data_register_real_time_monitor(FileData *fd)
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
		realtime_monitor_id = g_timeout_add(5000, realtime_monitor_cb, NULL);
		}

	return TRUE;
}

gboolean file_data_unregister_real_time_monitor(FileData *fd)
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

/*
 *-----------------------------------------------------------------------------
 * Saving marks list, clearing marks
 * Uses file_data_pool
 *-----------------------------------------------------------------------------
 */

static void marks_get_files(gpointer key, gpointer value, gpointer userdata)
{
	gchar *file_name = (gchar *)key;
	GString *result = (GString*)userdata;
	FileData *fd;

	if (isfile(file_name))
		{
		fd = (FileData *)value;
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
				FileData *fd = (FileData*)file_data_new_no_grouping(file_path);
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

static void marks_clear(gpointer key, gpointer value, gpointer UNUSED(userdata))
{
	gchar *file_name = (gchar *)key;
	gint mark_no;
	gint n;
	FileData *fd;

	if (isfile(file_name))
		{
		fd = (FileData *)value;
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

void file_data_set_page_num(FileData *fd, gint page_num)
{
	if (fd->page_total > 1 && page_num < 0)
		{
		fd->page_num = fd->page_total - 1;
		}
	else if (fd->page_total > 1 && page_num <= fd->page_total)
		{
		fd->page_num = page_num - 1;
		}
	else
		{
		fd->page_num = 0;
		}
	file_data_send_notification(fd, NOTIFY_REREAD);
}

void file_data_inc_page_num(FileData *fd)
{
	if (fd->page_total > 0 && fd->page_num < fd->page_total - 1)
		{
		fd->page_num = fd->page_num + 1;
		}
	else if (fd->page_total == 0)
		{
		fd->page_num = fd->page_num + 1;
		}
	file_data_send_notification(fd, NOTIFY_REREAD);
}

void file_data_dec_page_num(FileData *fd)
{
	if (fd->page_num > 0)
		{
		fd->page_num = fd->page_num - 1;
		}
	file_data_send_notification(fd, NOTIFY_REREAD);
}

void file_data_set_page_total(FileData *fd, gint page_total)
{
	fd->page_total = page_total;
}

/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
