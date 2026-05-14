/*
 * Copyright (C) 2008 - 2016 The Geeqie Team
 *
 * Author: Vladimir Nadvornik
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

#include "filecache.h"

#include <config.h>

#include "filedata.h"

/* this implements a simple LRU algorithm */

struct FileCacheData {
	FileCacheReleaseFunc release;
	GList *list;
	gulong max_size;
	gulong size;
};

namespace
{

struct FileCacheEntry {
	FileData *fd;
	gulong size;
	gboolean checking_if_changed;
};

#ifdef DEBUG
constexpr bool debug_file_cache = false; /* Set to true to add file cache dumps to the debug output */

void file_cache_dump(FileCacheData *fc)
{
	if (!debug_file_cache) return;

	DEBUG_1("cache dump: fc=%p max size:%lu size:%lu", (void *)fc, fc->max_size, fc->size);

	gulong n = 0;
	for (GList *work = fc->list; work; work = work->next)
		{
		auto fe = static_cast<FileCacheEntry *>(work->data);
		DEBUG_1("cache entry: fc=%p [%lu] %s %lu", (void *)fc, ++n, fe->fd->path, fe->size);
		}
}
#else
#  define file_cache_dump(fc)
#endif

gint file_cache_entry_compare_fd(const FileCacheEntry *fe, const FileData *fd)
{
	return (fe->fd == fd) ? 0 : 1;
}

void file_cache_remove_entry(FileCacheData *fc, GList *link)
{
	auto *fe = static_cast<FileCacheEntry *>(link->data);

	DEBUG_1("cache remove: fc=%p %s", (void *)fc, fe->fd->path);

	fc->list = g_list_delete_link(fc->list, link);
	fc->size -= fe->size;
	fc->release(fe->fd);
	file_data_unref(fe->fd);
	g_free(fe);
}

void file_cache_notify_cb(FileData *fd, NotifyType type, gpointer data)
{
	/* invalidate the entry on each file change */
	if (!(type & (NOTIFY_REREAD | NOTIFY_CHANGE))) return;

	DEBUG_1("Notify cache: %s %04x", fd->path, type);

	auto *fc = static_cast<FileCacheData *>(data);
	file_cache_dump(fc);

	GList *work = g_list_find_custom(fc->list, fd, reinterpret_cast<GCompareFunc>(file_cache_entry_compare_fd));
	if (!work) return;

	file_cache_remove_entry(fc, work);
}

void file_cache_shrink_to_max_size(FileCacheData *fc)
{
	file_cache_dump(fc);

	GList *work = g_list_last(fc->list);
	while (fc->size > fc->max_size && work)
		{
		GList *prev = work->prev;
		file_cache_remove_entry(fc, work);
		work = prev;
		}
}

} // namespace

FileCacheData *file_cache_new(FileCacheReleaseFunc release, gulong max_size)
{
	auto fc = g_new(FileCacheData, 1);

	fc->release = release;
	fc->list = nullptr;
	fc->max_size = max_size;
	fc->size = 0;

	file_data_register_notify_func(file_cache_notify_cb, fc, NOTIFY_PRIORITY_HIGH);

	return fc;
}

gboolean file_cache_get(FileCacheData *fc, FileData *fd)
{
	/* Operating theory of this function:
	 * This function must be re-entrant, which means it must specifically be implemented in a
	 * way that remains correct even when a re-entrant call happens.
	 *
	 * In particular, the file_data_check_changed_files function call may trigger another call
	 * into this function.  In order to handle that case correctly, we establish that if the
	 * FileData has changed (in which case we plan to evict it and return FALSE), any
	 * re-entrant calls into the function will return TRUE _without_ checking for changes.
	 * Then we will evict the FileData from the cache (if that hasn't happened already), and
	 * then we will return FALSE.
	 *
	 * That said, because it's also possible for a re-entrant call to target a _different_
	 * FileData than the one that we plan to evict, we stash a checking_if_changed bool in
	 * every FileCacheEntry.  That is, we need to handle the case where
	 * file_cache_get(fc, fd_A) triggers file_cache_get(fc, fd_B), which in turn triggers
	 * file_cache_get(fc, fd_A) again.
	 */

	g_assert(fc && fd);

	GList *work = g_list_find_custom(fc->list, fd, reinterpret_cast<GCompareFunc>(file_cache_entry_compare_fd));
	if (!work)
		{
		DEBUG_2("cache miss: fc=%p %s", (void *)fc, fd->path);
		return FALSE;
		}

	// Entry exists.
	DEBUG_2("cache hit: fc=%p %s", (void *)fc, fd->path);

	// Move it to the beginning, if needed.
	if (work != fc->list)
		{
		DEBUG_2("cache move to front: fc=%p %s", (void *)fc, fd->path);
		fc->list = g_list_remove_link(fc->list, work);
		fc->list = g_list_concat(work, fc->list);
		}

	// Most of the following code is defending against the case where
	// file_data_check_changed_files triggers a re-entrant call back into this file_cache_get.
	{
		auto *entry = static_cast<FileCacheEntry *>(work->data);
		if (entry->checking_if_changed) return TRUE;  // Avoid infinite recursion.

		entry->checking_if_changed = TRUE;
	}

	// We assume that file_data_check_changed_files may invalidate work.
	work = nullptr;
	const gboolean fd_changed = file_data_check_changed_files(fd);

	// Now we re-acquire work to take the appropriate action, if it still exists.
	work = g_list_find_custom(fc->list, fd, reinterpret_cast<GCompareFunc>(file_cache_entry_compare_fd));
	if (!work) return FALSE;

	// Doing this here for correctness, even though we might immediately evict the entry.
	{
		auto *entry = static_cast<FileCacheEntry *>(work->data);
		entry->checking_if_changed = FALSE;
	}

	if (fd_changed)
		{
		// Underlying file has been changed.  Evict the cache entry.
		file_cache_dump(fc);
		file_cache_remove_entry(fc, work);
		return FALSE;
		}

	file_cache_dump(fc);
	return TRUE;
}

void file_cache_put(FileCacheData *fc, FileData *fd, gulong size)
{
	FileCacheEntry *fe;

	if (file_cache_get(fc, fd)) return;

	DEBUG_2("cache add: fc=%p %s", (void *)fc, fd->path);
	// TODO[xsdg]: Switch to an stl container and do this initialization in a constructor.
	fe = g_new(FileCacheEntry, 1);
	fe->fd = file_data_ref(fd);
	fe->size = size;
	fe->checking_if_changed = FALSE;
	fc->list = g_list_prepend(fc->list, fe);
	fc->size += size;

	file_cache_shrink_to_max_size(fc);
}

void file_cache_set_max_size(FileCacheData *fc, gulong size)
{
	fc->max_size = size;
	file_cache_shrink_to_max_size(fc);
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
