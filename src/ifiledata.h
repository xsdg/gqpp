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

#ifndef IFILEDATA_H
#define IFILEDATA_H

#include "main.h"

typedef enum {
	FILEDATA_CHANGE_DELETE,
	FILEDATA_CHANGE_MOVE,
	FILEDATA_CHANGE_RENAME,
	FILEDATA_CHANGE_COPY,
	FILEDATA_CHANGE_UNSPECIFIED,
	FILEDATA_CHANGE_WRITE_METADATA
} FileDataChangeType;

typedef enum {
	NOTIFY_PRIORITY_HIGH = 0,
	NOTIFY_PRIORITY_MEDIUM,
	NOTIFY_PRIORITY_LOW
} NotifyPriority;

typedef enum {
	NOTIFY_MARKS		= 1 << 1, /**< changed marks */
	NOTIFY_PIXBUF		= 1 << 2, /**< image was read into fd->pixbuf */
	NOTIFY_HISTMAP		= 1 << 3, /**< histmap was read into fd->histmap */
	NOTIFY_ORIENTATION	= 1 << 4, /**< image was rotated */
	NOTIFY_METADATA		= 1 << 5, /**< changed image metadata, not yet written */
	NOTIFY_GROUPING		= 1 << 6, /**< change in fd->sidecar_files or fd->parent */
	NOTIFY_REREAD		= 1 << 7, /**< changed file size, date, etc., file name remains unchanged */
	NOTIFY_CHANGE		= 1 << 8  /**< generic change described by fd->change */
} NotifyType;

#define FILEDATA_MARKS_SIZE 10

struct FileDataChangeInfo {
	FileDataChangeType type;
	gchar *source;
	gchar *dest;
	gint error;
	gboolean regroup_when_finished;
};

struct FileData;

typedef gboolean (* FileDataGetMarkFunc)(FileData *fd, gint n, gpointer data);
typedef gboolean (* FileDataSetMarkFunc)(FileData *fd, gint n, gboolean value, gpointer data);
typedef void (*FileDataNotifyFunc)(FileData *fd, NotifyType type, gpointer data);

struct IFileData {
    // Child classes that encapsulate some functionality.
    struct FileList;
    struct Filter;
    struct Sidecar;
    struct Util;

    // TODO(xsdg): Mark non-public APIs as private.
    public:
        // change_info.c;
        gboolean file_data_add_ci(FileData *fd, FileDataChangeType type, const gchar *src, const gchar *dest);
        /*static*/ void file_data_planned_change_remove(FileData *fd);
        void file_data_free_ci(FileData *fd);
        void file_data_set_regroup_when_finished(FileData *fd, gboolean enable);
        static gboolean file_data_add_ci_write_metadata_list(GList *fd_list);
        static void file_data_free_ci_list(GList *fd_list);
        /*static*/ void file_data_update_planned_change_hash(FileData *fd, const gchar *old_path, gchar *new_path);
        /*static*/ void file_data_update_ci_dest(FileData *fd, const gchar *dest_path);
        /*static*/ void file_data_update_ci_dest_preserve_ext(FileData *fd, const gchar *dest_path);
        gint file_data_verify_ci(FileData *fd, GList *list);
        static gint file_data_verify_ci_list(GList *list, gchar **desc, gboolean with_sidecars);
        /*static*/ gboolean file_data_perform_move(FileData *fd);
        /*static*/ gboolean file_data_perform_copy(FileData *fd);
        /*static*/ gboolean file_data_perform_delete(FileData *fd);
        gboolean file_data_perform_ci(FileData *fd);
        gboolean file_data_apply_ci(FileData *fd);
        static gint file_data_notify_sort(gconstpointer a, gconstpointer b);
        static gboolean file_data_register_notify_func(FileDataNotifyFunc func, gpointer data, NotifyPriority priority);
        static gboolean file_data_unregister_notify_func(FileDataNotifyFunc func, gpointer data);
        static gboolean file_data_send_notification_idle_cb(gpointer data);
        static void file_data_send_notification(FileData *fd, NotifyType type);
        void file_data_change_info_free(FileDataChangeInfo *fdci, FileData *fd);

        // core.c;
        static FileData *file_data_new(const gchar *path_utf8, struct stat *st, gboolean disable_sidecars);
        /**/static/**/ FileData *file_data_new_local(const gchar *path, struct stat *st, gboolean disable_sidecars);
    public:
        static FileData *file_data_new_simple(const gchar *path_utf8);
        static FileData *file_data_new_group(const gchar *path_utf8);
        static FileData *file_data_new_no_grouping(const gchar *path_utf8);
        /**/static/**/ FileData *file_data_new_dir(const gchar *path_utf8);
    public:
        // file_data_ref and file_data_unref are part of the public API.
        FileData *file_data_ref_debug(const gchar *file, gint line, FileData *fd);
        FileData *file_data_ref(FileData *fd);
    /*private:*/

        /*static*/ void file_data_free(FileData *fd);
        /*static*/ gboolean file_data_check_has_ref(FileData *fd);
        /*static*/ void file_data_consider_free(FileData *fd);
    public:
        void file_data_unref_debug(const gchar *file, gint line, FileData *fd);
        void file_data_unref(FileData *fd);
        void file_data_lock(FileData *fd);
        void file_data_unlock(FileData *fd);
        static void file_data_lock_list(GList *list);
        static void file_data_unlock_list(GList *list);
        // TODO(xsdg): Make metadata.c a friend class and then make
        // increment_version a private method.
        void file_data_increment_version(FileData *fd);
    /*private:*/
        /*static*/ gboolean file_data_check_changed_single_file(FileData *fd, struct stat *st);
        /*static*/ gboolean file_data_check_changed_files_recursive(FileData *fd, struct stat *st);
        /**static**/ gboolean file_data_check_changed_files(FileData *fd);
        static void realtime_monitor_check_cb(gpointer key, gpointer value, gpointer data);
        static gboolean realtime_monitor_cb(gpointer data);
        gboolean file_data_register_real_time_monitor(FileData *fd);
        gboolean file_data_unregister_real_time_monitor(FileData *fd);

        // metadata.c;
        /*static*/ void file_data_set_collate_keys(FileData *fd);
        /*static*/ void file_data_set_path(FileData *fd, const gchar *path);
        void read_exif_time_data(FileData *file);
        void read_exif_time_digitized_data(FileData *file);
        void read_rating_data(FileData *file);
        void set_exif_time_data(GList *files);
        void set_exif_time_digitized_data(GList *files);
        void set_rating_data(GList *files);

        // sidecar_change_info.c;
        /*static*/ gboolean file_data_sc_add_ci(FileData *fd, FileDataChangeType type);
        /*static*/ gboolean file_data_sc_check_ci(FileData *fd, FileDataChangeType type);
        gboolean file_data_sc_add_ci_copy(FileData *fd, const gchar *dest_path);
        gboolean file_data_sc_add_ci_move(FileData *fd, const gchar *dest_path);
        gboolean file_data_sc_add_ci_rename(FileData *fd, const gchar *dest_path);
        gboolean file_data_sc_add_ci_delete(FileData *fd);
        gboolean file_data_sc_add_ci_unspecified(FileData *fd, const gchar *dest_path);
        gboolean file_data_add_ci_write_metadata(FileData *fd);
        void file_data_sc_free_ci(FileData *fd);
        static gboolean file_data_sc_add_ci_delete_list(GList *fd_list);
        static void file_data_sc_revert_ci_list(GList *fd_list);
        using CiListCallFunc = gboolean (FileData::*)(FileData *, const gchar *);
        // using CiListCallFunc = gboolean (*)(FileData *, const gchar *);
        static gboolean file_data_sc_add_ci_list_call_func(
                GList *fd_list, const gchar *dest, CiListCallFunc func);
        static gboolean file_data_sc_add_ci_copy_list(GList *fd_list, const gchar *dest);
        static gboolean file_data_sc_add_ci_move_list(GList *fd_list, const gchar *dest);
        static gboolean file_data_sc_add_ci_rename_list(GList *fd_list, const gchar *dest);
        static gboolean file_data_sc_add_ci_unspecified_list(GList *fd_list, const gchar *dest);
        static void file_data_sc_free_ci_list(GList *fd_list);
        /*static*/ void file_data_sc_update_ci(FileData *fd, const gchar *dest_path);
        /*static*/ gboolean file_data_sc_check_update_ci(FileData *fd, const gchar *dest_path, FileDataChangeType type);
        gboolean file_data_sc_update_ci_copy(FileData *fd, const gchar *dest_path);
        gboolean file_data_sc_update_ci_move(FileData *fd, const gchar *dest_path);
        gboolean file_data_sc_update_ci_rename(FileData *fd, const gchar *dest_path);
        gboolean file_data_sc_update_ci_unspecified(FileData *fd, const gchar *dest_path);
        static gboolean file_data_sc_update_ci_list_call_func(
                GList *fd_list, const gchar *dest, CiListCallFunc func);
        static gboolean file_data_sc_update_ci_move_list(GList *fd_list, const gchar *dest);
        static gboolean file_data_sc_update_ci_copy_list(GList *fd_list, const gchar *dest);
        static gboolean file_data_sc_update_ci_unspecified_list(GList *fd_list, const gchar *dest);
        gint file_data_sc_verify_ci(FileData *fd, GList *list);
        gboolean file_data_sc_perform_ci(FileData *fd);
        gboolean file_data_sc_apply_ci(FileData *fd);
};

#endif  // IFILEDATA_H
