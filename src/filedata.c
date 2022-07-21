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


gchar *text_from_size(gint64 size)
{
    return FileData::Util::text_from_size( size);
}

gchar *text_from_size_abrev(gint64 size)
{
    return FileData::Util::text_from_size_abrev( size);
}

const gchar *text_from_time(time_t t)
{
    return FileData::Util::text_from_time(t);
}

/***************** CORE *****************/
/**
 * @headerfile file_data_new_group
 * scan for sidecar files - expensive
 */
FileData *file_data_new_group(const gchar *path_utf8)
{
    return FileData::file_data_new_group(path_utf8);
}

/**
 * @headerfile file_data_new_no_grouping
 * should be used on helper files which can't have sidecars
 */
FileData *file_data_new_no_grouping(const gchar *path_utf8)
{
    return FileData::file_data_new_no_grouping(path_utf8);
}

/**
 * @headerfile file_data_new_dir
 * should be used on dirs
 */
FileData *file_data_new_dir(const gchar *path_utf8)
{
    return FileData::file_data_new_dir(path_utf8);
}

FileData *file_data_new_simple(const gchar *path_utf8)
{
    return FileData::file_data_new_simple(path_utf8);
}

/*
#ifdef DEBUG_FILEDATA
FileData *file_data_ref_debug(const gchar *file, gint line, FileData *fd)
{
    return FileData::file_data_ref_debug( file, line, fd);
}

void file_data_unref_debug(const gchar *file, gint line, FileData *fd)
{
    FileData::file_data_unref_debug( file, line, fd);
}

#define file_data_ref(fd) file_data_ref_debug(__FILE__, __LINE__, fd)
#define file_data_unref(fd) file_data_unref_debug(__FILE__, __LINE__, fd)
#else
FileData *file_data_ref(FileData *fd)
{
    return FileData::file_data_ref( fd);
}

void file_data_unref(FileData *fd)
{
    fd->file_data_unref(fd);
}

#endif
*/

void file_data_lock(FileData *fd)
{
    fd->file_data_lock(fd);
}

void file_data_unlock(FileData *fd)
{
    fd->file_data_unlock(fd);
}

void file_data_lock_list(GList *list)
{
    FileData::file_data_lock_list(list);
}

void file_data_unlock_list(GList *list)
{
    FileData::file_data_unlock_list(list);
}



/***************** CHANGE *****************/

gboolean file_data_check_changed_files(FileData *fd)
{
    return fd->file_data_check_changed_files(fd);
}


void file_data_increment_version(FileData *fd)
{
    fd->file_data_increment_version(fd);
}

void file_data_change_info_free(FileDataChangeInfo *fdci, FileData *fd)
{
    fd->file_data_change_info_free(fdci, fd);
}


void file_data_disable_grouping(FileData *fd, gboolean disable)
{
    FileData::Sidecar::disable_grouping(fd, disable);
}

void file_data_disable_grouping_list(GList *fd_list, gboolean disable)
{
    FileData::Sidecar::disable_grouping_list( fd_list, disable);
}



/***************** FILELIST *****************/

gint filelist_sort_compare_filedata(FileData *fa, FileData *fb)
{
    return FileData::FileList::sort_compare_filedata(fa, fb);
}

gint filelist_sort_compare_filedata_full(FileData *fa, FileData *fb, SortType method, gboolean ascend)
{
    return FileData::FileList::sort_compare_filedata_full(fa, fb, method, ascend);
}

GList *filelist_sort(GList *list, SortType method, gboolean ascend)
{
    return FileData::FileList::sort( list, method, ascend);
}

GList *filelist_insert_sort(GList *list, FileData *fd, SortType method, gboolean ascend)
{
    return FileData::FileList::insert_sort( list, fd, method, ascend);
}

GList *filelist_sort_full(GList *list, SortType method, gboolean ascend, GCompareFunc cb)
{
    return FileData::FileList::sort_full( list, method, ascend, cb);
}

GList *filelist_insert_sort_full(GList *list, gpointer data, SortType method, gboolean ascend, GCompareFunc cb)
{
    return FileData::FileList::insert_sort_full( list, data, method, ascend, cb);
}


gboolean filelist_read(FileData *dir_fd, GList **files, GList **dirs)
{
    return FileData::FileList::read(dir_fd, files, dirs);
}

gboolean filelist_read_lstat(FileData *dir_fd, GList **files, GList **dirs)
{
    return FileData::FileList::read_lstat(dir_fd, files, dirs);
}

void filelist_free(GList *list)
{
    FileData::FileList::fl_free( list);
}

GList *filelist_copy(GList *list)
{
    return FileData::FileList::copy( list);
}

GList *filelist_from_path_list(GList *list)
{
    return FileData::FileList::from_path_list( list);
}

GList *filelist_to_path_list(GList *list)
{
    return FileData::FileList::to_path_list( list);
}


GList *filelist_filter(GList *list, gboolean is_dir_list)
{
    return FileData::FileList::filter( list, is_dir_list);
}


GList *filelist_sort_path(GList *list)
{
    return FileData::FileList::sort_path( list);
}

GList *filelist_recursive(FileData *dir_fd)
{
    return FileData::FileList::recursive( dir_fd);
}

GList *filelist_recursive_full(FileData *dir_fd, SortType method, gboolean ascend)
{
    return FileData::FileList::recursive_full( dir_fd, method, ascend);
}



/***************** MARKS *****************/

// typedef gboolean (* FileDataGetMarkFunc)(FileData *fd, gint n, gpointer data);
// typedef gboolean (* FileDataSetMarkFunc)(FileData *fd, gint n, gboolean value, gpointer data);
gboolean file_data_register_mark_func(gint n, FileDataGetMarkFunc get_mark_func, FileDataSetMarkFunc set_mark_func, gpointer data, GDestroyNotify notify)
{
    return FileData::file_data_register_mark_func( n, get_mark_func, set_mark_func, data, notify);
}

void file_data_get_registered_mark_func(gint n, FileDataGetMarkFunc *get_mark_func, FileDataSetMarkFunc *set_mark_func, gpointer *data)
{
    FileData::file_data_get_registered_mark_func( n, get_mark_func, set_mark_func, data);
}



gboolean file_data_get_mark(FileData *fd, gint n)
{
    return fd->file_data_get_mark(fd, n);
}

guint file_data_get_marks(FileData *fd)
{
    return fd->file_data_get_marks(fd);
}

void file_data_set_mark(FileData *fd, gint n, gboolean value)
{
    fd->file_data_set_mark(fd, n, value);
}

gboolean file_data_filter_marks(FileData *fd, guint filter)
{
    return fd->file_data_filter_marks(fd, filter);
}

GList *file_data_filter_marks_list(GList *list, guint filter)
{
    return FileData::file_data_filter_marks_list( list, filter);
}


gboolean file_data_filter_file_filter(FileData *fd, GRegex *filter)
{
    return fd->file_data_filter_file_filter(fd, filter);
}

GList *file_data_filter_file_filter_list(GList *list, GRegex *filter)
{
    return FileData::file_data_filter_file_filter_list( list, filter);
}


GList *file_data_filter_class_list(GList *list, guint filter)
{
    return FileData::file_data_filter_class_list( list, filter);
}


gint file_data_get_user_orientation(FileData *fd)
{
    return FileData::Util::get_user_orientation(fd);
}

void file_data_set_user_orientation(FileData *fd, gint value)
{
    FileData::Util::set_user_orientation(fd, value);
}


gchar *file_data_sc_list_to_string(FileData *fd)
{
    return FileData::Sidecar::sc_list_to_string(fd);
}


gchar *file_data_get_sidecar_path(FileData *fd, gboolean existing_only)
{
    return FileData::Sidecar::get_sidecar_path(fd, existing_only);
}



/***************** CHANGE_INFO *****************/

gboolean file_data_add_ci(FileData *fd, FileDataChangeType type, const gchar *src, const gchar *dest)
{
    return fd->file_data_add_ci(fd, type, src, dest);
}

gboolean file_data_sc_add_ci_copy(FileData *fd, const gchar *dest_path)
{
    return fd->file_data_sc_add_ci_copy(fd, dest_path);
}

gboolean file_data_sc_add_ci_move(FileData *fd, const gchar *dest_path)
{
    return fd->file_data_sc_add_ci_move(fd, dest_path);
}

gboolean file_data_sc_add_ci_rename(FileData *fd, const gchar *dest_path)
{
    return fd->file_data_sc_add_ci_rename(fd, dest_path);
}

gboolean file_data_sc_add_ci_delete(FileData *fd)
{
    return fd->file_data_sc_add_ci_delete(fd);
}

gboolean file_data_sc_add_ci_unspecified(FileData *fd, const gchar *dest_path)
{
    return fd->file_data_sc_add_ci_unspecified(fd, dest_path);
}


gboolean file_data_sc_add_ci_delete_list(GList *fd_list)
{
    return FileData::file_data_sc_add_ci_delete_list( fd_list);
}

gboolean file_data_sc_add_ci_copy_list(GList *fd_list, const gchar *dest)
{
    return FileData::file_data_sc_add_ci_copy_list( fd_list, dest);
}

gboolean file_data_sc_add_ci_move_list(GList *fd_list, const gchar *dest)
{
    return FileData::file_data_sc_add_ci_move_list( fd_list, dest);
}

gboolean file_data_sc_add_ci_rename_list(GList *fd_list, const gchar *dest)
{
    return FileData::file_data_sc_add_ci_rename_list( fd_list, dest);
}

gboolean file_data_sc_add_ci_unspecified_list(GList *fd_list, const gchar *dest)
{
    return FileData::file_data_sc_add_ci_unspecified_list( fd_list, dest);
}

gboolean file_data_add_ci_write_metadata_list(GList *fd_list)
{
    return FileData::file_data_add_ci_write_metadata_list( fd_list);
}


gboolean file_data_sc_update_ci_copy_list(GList *fd_list, const gchar *dest)
{
    return FileData::file_data_sc_update_ci_copy_list( fd_list, dest);
}

gboolean file_data_sc_update_ci_move_list(GList *fd_list, const gchar *dest)
{
    return FileData::file_data_sc_update_ci_move_list( fd_list, dest);
}

gboolean file_data_sc_update_ci_unspecified_list(GList *fd_list, const gchar *dest)
{
    return FileData::file_data_sc_update_ci_unspecified_list( fd_list, dest);
}



gboolean file_data_sc_update_ci_copy(FileData *fd, const gchar *dest_path)
{
    return fd->file_data_sc_update_ci_copy(fd, dest_path);
}

gboolean file_data_sc_update_ci_move(FileData *fd, const gchar *dest_path)
{
    return fd->file_data_sc_update_ci_move(fd, dest_path);
}

gboolean file_data_sc_update_ci_rename(FileData *fd, const gchar *dest_path)
{
    return fd->file_data_sc_update_ci_rename(fd, dest_path);
}

gboolean file_data_sc_update_ci_unspecified(FileData *fd, const gchar *dest_path)
{
    return fd->file_data_sc_update_ci_unspecified(fd, dest_path);
}


gchar *file_data_get_error_string(gint error)
{
    return FileData::Util::get_error_string( error);
}


gint file_data_verify_ci(FileData *fd, GList *list)
{
    return fd->file_data_verify_ci(fd, list);
}

gint file_data_verify_ci_list(GList *list, gchar **desc, gboolean with_sidecars)
{
    return FileData::file_data_verify_ci_list( list, desc, with_sidecars);
}


gboolean file_data_perform_ci(FileData *fd)
{
    return fd->file_data_perform_ci(fd);
}

gboolean file_data_apply_ci(FileData *fd)
{
    return fd->file_data_apply_ci(fd);
}

void file_data_free_ci(FileData *fd)
{
    fd->file_data_free_ci(fd);
}

void file_data_free_ci_list(GList *fd_list)
{
    FileData::file_data_free_ci_list( fd_list);
}


void file_data_set_regroup_when_finished(FileData *fd, gboolean enable)
{
    fd->file_data_set_regroup_when_finished(fd, enable);
}


gint file_data_sc_verify_ci(FileData *fd, GList *list)
{
    return fd->file_data_sc_verify_ci(fd, list);
}


gboolean file_data_sc_perform_ci(FileData *fd)
{
    return fd->file_data_sc_perform_ci(fd);
}

gboolean file_data_sc_apply_ci(FileData *fd)
{
    return fd->file_data_sc_apply_ci(fd);
}

void file_data_sc_free_ci(FileData *fd)
{
    fd->file_data_sc_free_ci(fd);
}

void file_data_sc_free_ci_list(GList *fd_list)
{
    FileData::file_data_sc_free_ci_list( fd_list);
}


GList *file_data_process_groups_in_selection(GList *list, gboolean ungroup, GList **ungrouped)
{
    return FileData::Sidecar::process_groups_in_selection(list, ungroup, ungrouped);
}



// typedef void (*FileDataNotifyFunc)(FileData *fd, NotifyType type, gpointer data);
gboolean file_data_register_notify_func(FileDataNotifyFunc func, gpointer data, NotifyPriority priority)
{
    return FileData::file_data_register_notify_func( func, data, priority);
}

gboolean file_data_unregister_notify_func(FileDataNotifyFunc func, gpointer data)
{
    return FileData::file_data_unregister_notify_func( func, data);
}

void file_data_send_notification(FileData *fd, NotifyType type)
{
    fd->file_data_send_notification(fd, type);
}


gboolean file_data_register_real_time_monitor(FileData *fd)
{
    return fd->file_data_register_real_time_monitor(fd);
}

gboolean file_data_unregister_real_time_monitor(FileData *fd)
{
    return fd->file_data_unregister_real_time_monitor(fd);
}


void read_exif_time_data(FileData *file)
{
    file->read_exif_time_data(file);
}

void read_exif_time_digitized_data(FileData *file)
{
    file->read_exif_time_digitized_data(file);
}


gboolean marks_list_save(gchar *path, gboolean clear)
{
    return FileData::marks_list_save( path, clear);
}

gboolean marks_list_load(const gchar *path)
{
    return FileData::marks_list_load( path);
}

void marks_clear_all()
{
    FileData::marks_clear_all();
}

void read_rating_data(FileData *file)
{
    file->read_rating_data(file);
}


void file_data_inc_page_num(FileData *fd)
{
    FileData::Util::inc_page_num(fd);
}

void file_data_dec_page_num(FileData *fd)
{
    FileData::Util::dec_page_num(fd);
}

void file_data_set_page_total(FileData *fd, gint page_total)
{
    FileData::Util::set_page_total(fd, page_total);
}

void file_data_set_page_num(FileData *fd, gint page_num)
{
    FileData::Util::set_page_num(fd, page_num);
}


/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
