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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <functional>

// The FileData interface spec.
#include "ifiledata.h"

typedef enum {
	ZOOM_RESET_ORIGINAL	= 0,
	ZOOM_RESET_FIT_WINDOW	= 1,
	ZOOM_RESET_NONE		= 2
} ZoomMode;

typedef enum {
	ZOOM_GEOMETRIC	= 0,
	ZOOM_ARITHMETIC	= 1
} ZoomStyle;

typedef enum {
	CLIPBOARD_PRIMARY	= 0,
	CLIPBOARD_CLIPBOARD = 1,
	CLIPBOARD_BOTH = 2
} ClipboardSelection;

typedef enum {
	MOUSE_BUTTON_LEFT	= 1,
	MOUSE_BUTTON_MIDDLE	= 2,
	MOUSE_BUTTON_RIGHT	= 3,
	MOUSE_BUTTON_WHEEL_UP	= 4,
	MOUSE_BUTTON_WHEEL_DOWN	= 5,
	MOUSE_BUTTON_8	= 8,
	MOUSE_BUTTON_9	= 9
} MouseButton;

typedef enum {
	DIRVIEW_LIST,
	DIRVIEW_TREE,

	DIRVIEW_LAST = DIRVIEW_TREE /**< Keep this up to date! */
} DirViewType;

typedef enum {
	FILEVIEW_LIST,
	FILEVIEW_ICON,

	FILEVIEW_LAST = FILEVIEW_ICON /**< Keep this up to date! */
} FileViewType;

#define	CMD_COPY     "geeqie-copy-command.desktop"
#define	CMD_MOVE     "geeqie-move-command.desktop"
#define	CMD_RENAME   "geeqie-rename-command.desktop"
#define	CMD_DELETE   "geeqie-delete-command.desktop"
#define	CMD_FOLDER   "geeqie-folder-command.desktop"

typedef enum {
	SORT_NONE,
	SORT_NAME,
	SORT_SIZE,
	SORT_TIME,
	SORT_CTIME,
	SORT_PATH,
	SORT_NUMBER,
	SORT_EXIFTIME,
	SORT_EXIFTIMEDIGITIZED,
	SORT_RATING,
	SORT_CLASS
} SortType;

/**
 * @typedef DnDAction
 * drag and drop default action
 */
typedef enum {
	DND_ACTION_ASK,
	DND_ACTION_COPY,
	DND_ACTION_MOVE
} DnDAction;

typedef enum {
	ALTER_NONE,		/**< do nothing */
	ALTER_ROTATE_90,
	ALTER_ROTATE_90_CC,	/**< counterclockwise */
	ALTER_ROTATE_180,
	ALTER_MIRROR,
	ALTER_FLIP,
} AlterType;

typedef enum {
	LAYOUT_HIDE   = 0,
	LAYOUT_LEFT   = 1 << 0,
	LAYOUT_RIGHT  = 1 << 1,
	LAYOUT_TOP    = 1 << 2,
	LAYOUT_BOTTOM = 1 << 3
} LayoutLocation;


typedef enum {
	IMAGE_STATE_NONE	= 0,
	IMAGE_STATE_IMAGE	= 1 << 0,
	IMAGE_STATE_LOADING	= 1 << 1,
	IMAGE_STATE_ERROR	= 1 << 2,
	IMAGE_STATE_COLOR_ADJ	= 1 << 3,
	IMAGE_STATE_ROTATE_AUTO	= 1 << 4,
	IMAGE_STATE_ROTATE_USER	= 1 << 5,
	IMAGE_STATE_DELAY_FLIP	= 1 << 6
} ImageState;

typedef enum {
	SPLIT_NONE = 0,
	SPLIT_VERT,
	SPLIT_HOR,
	SPLIT_QUAD,
} ImageSplitMode;

typedef enum {
	MTS_MODE_MINUS,
	MTS_MODE_SET,
	MTS_MODE_OR,
	MTS_MODE_AND
} MarkToSelectionMode;

typedef enum {
	STM_MODE_RESET,
	STM_MODE_SET,
	STM_MODE_TOGGLE
} SelectionToMarkMode;

extern gchar *format_class_list[]; /**< defined in preferences.c */

/**
 * @typedef SecureSaveErrno
 * see err field in #SecureSaveInfo
 */
typedef enum {
	SS_ERR_NONE = 0,
	SS_ERR_DISABLED, /**< secsave is disabled. */
	SS_ERR_OUT_OF_MEM, /**< memory allocation failure */

	SS_ERR_OPEN_READ,
	SS_ERR_OPEN_WRITE,
	SS_ERR_STAT,
	SS_ERR_ACCESS,
	SS_ERR_MKSTEMP,
	SS_ERR_RENAME,
	SS_ERR_OTHER,
} SecureSaveErrno;

typedef enum {
	CHANGE_OK                      = 0,
	CHANGE_WARN_DEST_EXISTS        = 1 << 0,
	CHANGE_WARN_NO_WRITE_PERM      = 1 << 1,
	CHANGE_WARN_SAME               = 1 << 2,
	CHANGE_WARN_CHANGED_EXT        = 1 << 3,
	CHANGE_WARN_UNSAVED_META       = 1 << 4,
	CHANGE_WARN_NO_WRITE_PERM_DEST_DIR  = 1 << 5,
	CHANGE_ERROR_MASK              = (~0U) << 8, /**< the values below are fatal errors */
	CHANGE_NO_READ_PERM            = 1 << 8,
	CHANGE_NO_WRITE_PERM_DIR       = 1 << 9,
	CHANGE_NO_DEST_DIR             = 1 << 10,
	CHANGE_DUPLICATE_DEST          = 1 << 11,
	CHANGE_NO_WRITE_PERM_DEST      = 1 << 12,
	CHANGE_DEST_EXISTS             = 1 << 13,
	CHANGE_NO_SRC                  = 1 << 14,
	CHANGE_GENERIC_ERROR           = 1 << 16
} ChangeError;

typedef enum {
	METADATA_PLAIN		= 0, /**< format that can be edited and written back */
	METADATA_FORMATTED	= 1  /**< for display only */
} MetadataFormat;

typedef enum {
	STARTUP_PATH_CURRENT	= 0,
	STARTUP_PATH_LAST,
	STARTUP_PATH_HOME,
} StartUpPath;

typedef enum {
	TOOLBAR_MAIN,
	TOOLBAR_STATUS,
	TOOLBAR_COUNT
} ToolbarType;

typedef enum {
	PR_STEREO_NONE             = 0,	  /**< do nothing */
	PR_STEREO_DUAL             = 1 << 0, /**< independent stereo buffers, for example nvidia opengl */
	PR_STEREO_FIXED            = 1 << 1,  /**< custom position */
	PR_STEREO_HORIZ            = 1 << 2,  /**< side by side */
	PR_STEREO_VERT             = 1 << 3,  /**< above below */
	PR_STEREO_RIGHT            = 1 << 4,  /**< render right buffer */
	PR_STEREO_ANAGLYPH_RC      = 1 << 5,  /**< anaglyph red-cyan */
	PR_STEREO_ANAGLYPH_GM      = 1 << 6,  /**< anaglyph green-magenta */
	PR_STEREO_ANAGLYPH_YB      = 1 << 7,  /**< anaglyph yellow-blue */
	PR_STEREO_ANAGLYPH_GRAY_RC = 1 << 8,  /**< anaglyph gray red-cyan*/
	PR_STEREO_ANAGLYPH_GRAY_GM = 1 << 9,  /**< anaglyph gray green-magenta */
	PR_STEREO_ANAGLYPH_GRAY_YB = 1 << 10, /**< anaglyph gray yellow-blue */
	PR_STEREO_ANAGLYPH_DB_RC   = 1 << 11, /**< anaglyph dubois red-cyan */
	PR_STEREO_ANAGLYPH_DB_GM   = 1 << 12, /**< anaglyph dubois green-magenta */
	PR_STEREO_ANAGLYPH_DB_YB   = 1 << 13, /**< anaglyph dubois yellow-blue */
	PR_STEREO_ANAGLYPH         = PR_STEREO_ANAGLYPH_RC |
	                             PR_STEREO_ANAGLYPH_GM |
	                             PR_STEREO_ANAGLYPH_YB |
	                             PR_STEREO_ANAGLYPH_GRAY_RC |
	                             PR_STEREO_ANAGLYPH_GRAY_GM |
	                             PR_STEREO_ANAGLYPH_GRAY_YB |
	                             PR_STEREO_ANAGLYPH_DB_RC |
	                             PR_STEREO_ANAGLYPH_DB_GM |
	                             PR_STEREO_ANAGLYPH_DB_YB, /**< anaglyph mask */

	PR_STEREO_MIRROR_LEFT      = 1 << 14, /**< mirror */
	PR_STEREO_FLIP_LEFT        = 1 << 15, /**< flip */

	PR_STEREO_MIRROR_RIGHT     = 1 << 16, /**< mirror */
	PR_STEREO_FLIP_RIGHT       = 1 << 17, /**< flip */

	PR_STEREO_MIRROR           = PR_STEREO_MIRROR_LEFT | PR_STEREO_MIRROR_RIGHT, /**< mirror mask*/
	PR_STEREO_FLIP             = PR_STEREO_FLIP_LEFT | PR_STEREO_FLIP_RIGHT, /**< flip mask*/
	PR_STEREO_SWAP             = 1 << 18,  /**< swap left and right buffers */
	PR_STEREO_TEMP_DISABLE     = 1 << 19,  /**< temporarily disable stereo mode if source image is not stereo */
	PR_STEREO_HALF             = 1 << 20
} PixbufRendererStereoMode;

typedef enum {
	STEREO_PIXBUF_DEFAULT  = 0,
	STEREO_PIXBUF_SBS      = 1,
	STEREO_PIXBUF_CROSS    = 2,
	STEREO_PIXBUF_NONE     = 3
} StereoPixbufData;

typedef enum {
	BAR_SORT_MODE_FOLDER = 0,
	BAR_SORT_MODE_COLLECTION,
	BAR_SORT_MODE_COUNT
} SortModeType;

typedef enum {
	BAR_SORT_COPY = 0,
	BAR_SORT_MOVE,
	BAR_SORT_FILTER,
	BAR_SORT_ACTION_COUNT
} SortActionType;

typedef enum {
	BAR_SORT_SELECTION_IMAGE = 0,
	BAR_SORT_SELECTION_SELECTED,
	BAR_SORT_SELECTION_COUNT
} SortSelectionType;

#define MAX_SPLIT_IMAGES 4

typedef struct _ImageLoader ImageLoader;
typedef struct _ThumbLoader ThumbLoader;

typedef struct _AnimationData AnimationData;

typedef struct _CollectInfo CollectInfo;
typedef struct _CollectionData CollectionData;
typedef struct _CollectTable CollectTable;
typedef struct _CollectWindow CollectWindow;

typedef struct _ImageWindow ImageWindow;

typedef struct _LayoutWindow LayoutWindow;
typedef struct _LayoutOptions LayoutOptions;

typedef struct _ViewDir ViewDir;
typedef struct _ViewDirInfoList ViewDirInfoList;
typedef struct _ViewDirInfoTree ViewDirInfoTree;

typedef struct _ViewFile ViewFile;
typedef struct _ViewFileInfoList ViewFileInfoList;
typedef struct _ViewFileInfoIcon ViewFileInfoIcon;

typedef struct _SlideShowData SlideShowData;
typedef struct _FullScreenData FullScreenData;

typedef struct _PixmapFolders PixmapFolders;
typedef struct _Histogram Histogram;

typedef struct _SecureSaveInfo SecureSaveInfo;

typedef struct _EditorDescription EditorDescription;

typedef struct _CommandLine CommandLine;

struct _Histogram {
	gint histogram_channel; /**< drawing mode for histogram */
	gint histogram_mode;     /**< logarithmical or not */
	guint vgrid; /**< number of vertical divisions, 0 for none */
	guint hgrid; /**< number of horizontal divisions, 0 for none */
	struct {
		int R; /**< red */
		int G; /**< green */
		int B; /**< blue */
		int A; /**< alpha */
	} grid_color;  /**< grid color */

};



struct _ImageLoader;

typedef void (* ThumbLoaderFunc)(ThumbLoader *tl, gpointer data);

typedef void (* FileUtilDoneFunc)(gboolean success, const gchar *done_path, gpointer data);

struct _ThumbLoader
{
	gboolean standard_loader;

	ImageLoader *il;
	FileData *fd;           /**< fd->pixbuf contains final (scaled) image when done */

	gboolean cache_enable;
	gboolean cache_hit;
	gdouble percent_done;

	gint max_w;
	gint max_h;

	ThumbLoaderFunc func_done;
	ThumbLoaderFunc func_error;
	ThumbLoaderFunc func_progress;

	gpointer data;

	guint idle_done_id; /**< event source id */
};

struct _AnimationData
{
	ImageWindow *iw;
	LayoutWindow *lw;
	GdkPixbufAnimation *gpa;
	GdkPixbufAnimationIter *iter;
	GdkPixbuf *gpb;
	FileData *data_adr;
	gint delay;
	gboolean valid;
	GCancellable *cancellable;
	GFile *in_file;
	GFileInputStream *gfstream;
};

struct _CollectInfo
{
	FileData *fd;
	GdkPixbuf *pixbuf;
	guint flag_mask;
};

struct _CollectionData
{
	gchar *path;
	gchar *name;
	GList *list;
	SortType sort_method;

	ThumbLoader *thumb_loader;
	CollectInfo *thumb_info;

	void (*info_updated_func)(CollectionData *, CollectInfo *, gpointer);
	gpointer info_updated_data;

	gint ref;

	/* geometry */
	gint window_read;
	gint window_x;
	gint window_y;
	gint window_w;
	gint window_h;

	gboolean changed; /**< contents changed since save flag */

	GHashTable *existence;
};

struct _CollectTable
{
	GtkWidget *scrolled;
	GtkWidget *listview;
	gint columns;
	gint rows;

	CollectionData *cd;

	GList *selection;
	CollectInfo *prev_selection;

	CollectInfo *click_info;

	GtkWidget *tip_window;
	guint tip_delay_id; /**< event source id */
	CollectInfo *tip_info;

	GdkWindow *marker_window;
	CollectInfo *marker_info;

	GtkWidget *status_label;
	GtkWidget *extra_label;

	gint focus_row;
	gint focus_column;
	CollectInfo *focus_info;

	GtkWidget *popup;
	CollectInfo *drop_info;
	GList *drop_list;

	guint sync_idle_id; /**< event source id */
	guint drop_idle_id; /**< event source id */

	gboolean show_text;
	gboolean show_stars;

	GList *editmenu_fd_list; /**< file list for edit menu */
};

struct _CollectWindow
{
	GtkWidget *window;
	CollectTable *table;
	GtkWidget *status_box;

	GtkWidget *close_dialog;

	CollectionData *cd;
};

typedef gint (* ImageTileRequestFunc)(ImageWindow *imd, gint x, gint y,
				      gint width, gint height, GdkPixbuf *pixbuf, gpointer);
typedef void (* ImageTileDisposeFunc)(ImageWindow *imd, gint x, gint y,
				      gint width, gint height, GdkPixbuf *pixbuf, gpointer);

struct _ImageWindow
{
	GtkWidget *widget;	/**< use this to add it and show it */
	GtkWidget *pr;
	GtkWidget *frame;

	FileData *image_fd;

	gboolean unknown;		/**< failed to load image */

	ImageLoader *il;        /**< @FIXME image loader should probably go to FileData, but it must first support
				   sending callbacks to multiple ImageWindows in parallel */

	gint has_frame;  /**< not boolean, see image_new() */

	/* top level (not necessarily parent) window */
	gboolean top_window_sync;	/**< resize top_window when image dimensions change */
	GtkWidget *top_window;	/**< window that gets title, and window to resize when 'fitting' */
	gchar *title;		/**< window title to display left of file name */
	gchar *title_right;	/**< window title to display right of file name */
	gboolean title_show_zoom;	/**< option to include zoom in window title */

	gboolean completed;
	ImageState state;	/**< mask of IMAGE_STATE_* flags about current image */

	void (*func_update)(ImageWindow *imd, gpointer data);
	void (*func_complete)(ImageWindow *imd, gint preload, gpointer data);
	void (*func_state)(ImageWindow *imd, ImageState state, gpointer data);
	ImageTileRequestFunc func_tile_request;
	ImageTileDisposeFunc func_tile_dispose;

	gpointer data_update;
	gpointer data_complete;
	gpointer data_state;
	gpointer data_tile;

	/* button, scroll functions */
	void (*func_button)(ImageWindow *, GdkEventButton *event, gpointer);
	void (*func_drag)(ImageWindow *, GdkEventMotion *event, gdouble dx, gdouble dy, gpointer);
	void (*func_scroll)(ImageWindow *, GdkEventScroll *event, gpointer);
	void (*func_focus_in)(ImageWindow *, gpointer);

	gpointer data_button;
	gpointer data_drag;
	gpointer data_scroll;
	gpointer data_focus_in;

	/**
	 * @headerfile func_scroll_notify
	 * scroll notification (for scroll bar implementation)
	 */
	void (*func_scroll_notify)(ImageWindow *, gint x, gint y, gint width, gint height, gpointer);

	gpointer data_scroll_notify;

	/* collection info */
	CollectionData *collection;
	CollectInfo *collection_info;

	/* color profiles */
	gboolean color_profile_enable;
	gint color_profile_input;
	gboolean color_profile_use_image;
	gint color_profile_from_image;
	gpointer cm;

	AlterType delay_alter_type;

	FileData *read_ahead_fd;
	ImageLoader *read_ahead_il;

	gint prev_color_row;

	gboolean auto_refresh;

	gboolean delay_flip;
	gint orientation;
	gboolean desaturate;
	gboolean overunderexposed;
	gint user_stereo;

	gboolean mouse_wheel_mode;
};

//// Callback infrastructure.
// Function pointer to FileData::something with return type RType and argument
// types ArgTypes...
template <typename RType, typename... ArgTypes>
using FileDataWrappedFunc = RType (FileData::*)(ArgTypes...);
// Functor that will call obj->method(args with types ArgTypes..., userdata),
// expecting a return type RType.
template <typename RType, typename... ArgTypes>
struct FileDataFunctor {
    FileData *obj;
    FileDataWrappedFunc<RType, ArgTypes..., void*> method;
    void* user_data = NULL;
};

// Wrapper that can be called-back to execute a FileDataFunctor with a non-void
// return type.  Will _not_ free the functor (so should be used when functor is
// allocated on the stack).
template <typename RType, typename... ArgTypes>
RType wrapper(ArgTypes..., void* user_data) {
    auto* functor = (FileDataFunctor<RType, ArgTypes...>*) user_data;
    return std::invoke(functor->method, functor->obj, functor->user_data);
}

// Wrapper that can be called-back to execute a FileDataFunctor with a void
// return type.  Will _not_ free the functor (so should be used when functor is
// allocated on the stack).
template <typename RType, typename... ArgTypes>
void v_wrapper(ArgTypes... args, void* user_data) {
    auto* functor = (FileDataFunctor<void, ArgTypes...>*) user_data;
    std::invoke(functor->method, functor->obj, functor->user_data, args...);
}

// Wrapper that can be called-back to execute a FileDataFunctor with a non-void
// return type.  _Will_ free the functor (so should be used when functor is
// allocated via malloc).
template <typename RType, typename... ArgTypes>
RType free_wrapper(ArgTypes..., void* user_data) {
    auto* functor = (FileDataFunctor<RType, ArgTypes...>*) user_data;
    RType retval = std::invoke(functor->method, functor->obj, functor->user_data);
    free(functor);
    return retval;
}

// Wrapper that can be called-back to execute a FileDataFunctor with a void
// return type.  _Will_ free the functor (so should be used when functor is
// allocated via malloc).
template <typename RType, typename... ArgTypes>
void free_v_wrapper(ArgTypes... args, void* user_data) {
    auto* functor = (FileDataFunctor<void, ArgTypes...>*) user_data;
    std::invoke(functor->method, functor->obj, functor->user_data, args...);
    free(functor);
}

struct FileData {
    // Child classes that encapsulate some functionality.
    struct FileList;
    struct Filter;
    struct Sidecar;
    struct Util;

    /**** CORE ****/
    private:
        static GHashTable *file_data_pool;  // = NULL;
        static GHashTable *file_data_planned_change_hash;  // = NULL;

    /**** BULK PASTE ****/
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


    /**** ORIGINAL PUBLIC INTERFACE ****/
    public:
	guint magick;
	gint type;
	gchar *original_path; /**< key to file_data_pool hash table */
	gchar *path;
	const gchar *name;
	const gchar *extension;
	gchar *extended_extension;
	FileFormatClass format_class;
	gchar *format_name; /**< set by the image loader */
	gchar *collate_key_name;
	gchar *collate_key_name_nocase;
	gint64 size;
	time_t date;
	time_t cdate;
	mode_t mode; /**< this is needed at least for notification in view_dir because it is preserved after the file/directory is deleted */
	gint sidecar_priority;

	guint marks; /**< each bit represents one mark */
	guint valid_marks; /**< zero bit means that the corresponding mark needs to be reread */


	GList *sidecar_files;
	FileData *parent; /**< parent file if this is a sidecar file, NULL otherwise */
	FileDataChangeInfo *change; /**< for rename, move ... */
	GdkPixbuf *thumb_pixbuf;

	GdkPixbuf *pixbuf; /**< full-size image, only complete images, NULL during loading
			      all FileData with non-NULL pixbuf are referenced by image_cache */

	HistMap *histmap;

	gboolean locked;
	gint ref;
	gint version; /**< increased when any field in this structure is changed */
	gboolean disable_grouping;

	gint user_orientation;
	gint exif_orientation;

	ExifData *exif;
	time_t exifdate;
	time_t exifdate_digitized;
	GHashTable *modified_xmp; /**< hash table which contains unwritten xmp metadata in format: key->list of string values */
	GList *cached_metadata;
	gint rating;
	gboolean metadata_in_idle_loaded;

	gchar *owner;
	gchar *group;
	gchar *sym_link;

	SelectionType selected;  /**< Used by view_file_icon. */

	gint page_num;
	gint page_total;
};

struct FileData::FileList
{
        FileList() = delete;

    public:
        static GList *copy(GList *list);
        static GList *from_path_list(GList *list);
        static GList *to_path_list(GList *list);
        static GList *filter(GList *list, gboolean is_dir_list);
        static GList *sort_path(GList *list);
        static GList *recursive(FileData *dir_fd);
        static GList *recursive_full(FileData *dir_fd, SortType method, gboolean ascend);
        static GList *sort_full(GList *list, SortType method, gboolean ascend, GCompareFunc cb);
        static GList *insert_sort_full(GList *list, gpointer data, SortType method, gboolean ascend, GCompareFunc cb);
        static GList *sort(GList *list, SortType method, gboolean ascend);
        static GList *insert_sort(GList *list, FileData *fd, SortType method, gboolean ascend);
        static gboolean read(FileData *dir_fd, GList **files, GList **dirs);
        static gboolean read_lstat(FileData *dir_fd, GList **files, GList **dirs);
        static void fl_free(GList *list);
        static gint sort_compare_filedata(FileData *fa, FileData *fb);
        static gint sort_compare_filedata_full(FileData *fa, FileData *fb, SortType method, gboolean ascend);

        // Were static, but called from elsewhere.
        static gboolean read_real(const gchar *dir_path, GList **files, GList **dirs, gboolean follow_symlinks);
    private:
        static gint sort_path_cb(gconstpointer a, gconstpointer b);
        static void recursive_append(GList **list, GList *dirs);
        static void recursive_append_full(GList **list, GList *dirs, SortType method, gboolean ascend);
        static gint sort_file_cb(gpointer a, gpointer b);
        static GList *filter_out_sidecars(GList *flist);
};

struct FileData::Filter
{
        Filter() = delete;

    public:
        static gboolean get_mark(FileData *fd, gint n);
        static guint get_marks(FileData *fd);
        static void set_mark(FileData *fd, gint n, gboolean value);

        static gboolean by_marks(FileData *fd, guint filter);
        static GList *by_marks(GList *list, guint filter);
        static gboolean by_file_filter(FileData *fd, GRegex *filter);
        static GList *by_file_filter(GList *list, GRegex *filter);
        static gboolean by_class(FileData *fd, guint filter);
        static GList *by_class(GList *list, guint filter);

        static void notify_mark_func(
                gpointer key, gpointer value, gpointer user_data);
        static gboolean register_mark_func(
                gint n, FileDataGetMarkFunc get_mark_func,
                FileDataSetMarkFunc set_mark_func, gpointer data,
                GDestroyNotify notify);
        static void get_registered_mark_func(
                gint n, FileDataGetMarkFunc *get_mark_func,
                FileDataSetMarkFunc *set_mark_func, gpointer *data);

        static void get_marks_files(gpointer key, gpointer value, gpointer userdata);
        static gboolean load_marks_list(const gchar *path);
        static gboolean save_marks_list(gchar *path, gboolean save);
        static void clear_marks(gpointer key, gpointer value, gpointer userdata);
        static void clear_all_marks();
};

struct FileData::Sidecar
{
        Sidecar() = delete;

    public:
        static gchar *get_sidecar_path(FileData *fd, gboolean existing_only);
        static gchar *sc_list_to_string(FileData *fd);
        static GList *process_groups_in_selection(GList *list, gboolean ungroup, GList **ungrouped_list);

        static gboolean list_contains_whole_group(GList *list, FileData *fd);
        static gint sidecar_file_priority(const gchar *extension);
        static void check_sidecars(const GList *basename_list);
        static void disconnect_sidecar_file(FileData *target, FileData *sfd);
        static void disable_grouping(FileData *fd, gboolean disable);
        static void disable_grouping_list(GList *fd_list, gboolean disable);
};

struct FileData::Util
{
        Util() = delete;

    public:
        static gchar *text_from_size(gint64 size);
        static gchar *text_from_size_abrev(gint64 size);
        static const gchar *text_from_time(time_t t);
        static GHashTable *basename_hash_new(void);
        static gchar *get_error_string(gint error);

        static gint sort_by_ext(gconstpointer a, gconstpointer b);
        static GList * basename_hash_insert(GHashTable *basename_hash, FileData *fd);
        static void basename_hash_insert_cb(gpointer fd, gpointer basename_hash);
        static void basename_hash_remove_list(gpointer key, gpointer value, gpointer data);
        static void basename_hash_free(GHashTable *basename_hash);
        static void basename_hash_to_sidecars(gpointer key, gpointer value, gpointer data);
        static gboolean is_hidden_file(const gchar *name);
        static gboolean can_write_directly(FileData *fd);
        static gboolean can_write_sidecar(FileData *fd);

        static gint get_user_orientation(FileData *fd);
        static void set_user_orientation(FileData *fd, gint value);
        static void set_page_num(FileData *fd, gint page_num);
        static void inc_page_num(FileData *fd);
        static void dec_page_num(FileData *fd);
        static void set_page_total(FileData *fd, gint page_total);
};

struct _LayoutOptions
{
	gchar *id;

	gchar *order;
	gint style;

	DirViewType dir_view_type;
	FileViewType file_view_type;

	struct {
		SortType method;
		gboolean ascend;
	} dir_view_list_sort;

	gboolean show_thumbnails;
	gboolean show_marks;
	gboolean show_file_filter;
	gboolean show_directory_date;
	gboolean show_info_pixel;
	gboolean split_pane_sync;
	gboolean ignore_alpha;

	struct {
		gint w;
		gint h;
		gint x;
		gint y;
		gboolean maximized;
		gint hdivider_pos;
		gint vdivider_pos;
	} main_window;

	struct {
		gint w;
		gint h;
		gint x;
		gint y;
		gint vdivider_pos;
	} float_window;

	struct {
		gint vdivider_pos;
	} folder_window;

	struct {
		gint w;
		gint h;
	} properties_window;

	struct {
		guint state;
		gint histogram_channel;
		gint histogram_mode;
	} image_overlay;

	struct {
		gint w;
		gint h;
		gint x;
		gint y;
	} log_window;

	struct {
		gint w;
		gint h;
		gint x;
		gint y;
		gint page_number;
	} preferences_window;

	struct {
		gint w;
		gint h;
		gint x;
		gint y;
	} search_window;

	struct {
		gint w;
		gint h;
		gint x;
		gint y;
	} dupe_window;

	struct {
		gint w;
		gint h;
		gint x;
		gint y;
	} advanced_exif_window;

	gboolean tools_float;
	gboolean tools_hidden;
	gboolean toolbar_hidden;

	struct {
		gboolean info;
		gboolean sort;
		gboolean tools_float;
		gboolean tools_hidden;
		gboolean hidden;
	} bars_state;

	gchar *home_path;
	gchar *last_path;

	StartUpPath startup_path;

	gboolean animate;
	gint workspace;

	SortActionType action;
	SortModeType mode;
	SortSelectionType selection;
	gchar *filter_key;
};

struct _LayoutWindow
{
	LayoutOptions options;

	FileData *dir_fd;

	/* base */

	GtkWidget *window;

	GtkWidget *main_box;

	GtkWidget *group_box;
	GtkWidget *h_pane;
	GtkWidget *v_pane;

	/* menus, path selector */

	GtkActionGroup *action_group;
	GtkActionGroup *action_group_editors;
	guint ui_editors_id;
	GtkUIManager *ui_manager;
	guint toolbar_merge_id[TOOLBAR_COUNT];
	GList *toolbar_actions[TOOLBAR_COUNT];

	GtkWidget *path_entry;

	/* image */

	LayoutLocation image_location;

	ImageWindow *image;

	ImageWindow *split_images[MAX_SPLIT_IMAGES];
	ImageSplitMode split_mode;
	gint active_split_image;

	GtkWidget *split_image_widget;
	GtkSizeGroup *split_image_sizegroup;

	/* tools window (float) */

	GtkWidget *tools;
	GtkWidget *tools_pane;

//	gint tools_float;
//	gint tools_hidden;
	GtkWidget *menu_tool_bar; /**< Combined menu and toolbar box */
	GtkWidget *menu_bar; /**< referenced by lw, exist during whole lw lifetime */
	/* toolbar */

	GtkWidget *toolbar[TOOLBAR_COUNT]; /**< referenced by lw, exist during whole lw lifetime */
//	gint toolbar_hidden;

//	GtkWidget *thumb_button;
//	gint thumbs_enabled;
//	gint marks_enabled;

	GtkWidget *back_button;

	/* dir view */

	LayoutLocation dir_location;

	ViewDir *vd;
	GtkWidget *dir_view;

//	DirViewType dir_view_type;

	/* file view */

	LayoutLocation file_location;

	ViewFile *vf;
//	FileViewType file_view_type;

	GtkWidget *file_view;

	SortType sort_method;
	gboolean sort_ascend;

	GtkWidget *info_box; /**< status bar */
	GtkWidget *info_progress_bar; /**< status bar */
	GtkWidget *info_sort; /**< status bar */
	GtkWidget *info_status; /**< status bar */
	GtkWidget *info_details; /**< status bar */
	GtkWidget *info_zoom; /**< status bar */
	GtkWidget *info_pixel; /**< status bar */

	/* slide show */

	SlideShowData *slideshow;

	/* full screen */

	FullScreenData *full_screen;

	/* dividers */

//	gint div_h;
//	gint div_v;
//	gint div_float;

	/* misc */

	GtkWidget *utility_box; /**< referenced by lw, exist during whole lw lifetime */
	GtkWidget *utility_paned; /**< between image and bar */
	GtkWidget *bar_sort;
	GtkWidget *bar;

	gboolean bar_sort_enabled; /**< Set during start-up, and checked when the editors have loaded */
//	gint bar_enabled;

//	gint bar_width;

	GtkWidget *exif_window;
	GtkWidget *sar_window; /**< Search and Run window */

	AnimationData *animation;

	GtkWidget *log_window;
};

struct _ViewDir
{
	DirViewType type;
	gpointer info;

	GtkWidget *widget;
	GtkWidget *view;

	FileData *dir_fd;

	FileData *click_fd;

	FileData *drop_fd;
	GList *drop_list;
	guint drop_scroll_id; /**< event source id */

	/* func list */
	void (*select_func)(ViewDir *vd, FileData *fd, gpointer data);
	gpointer select_data;

	void (*dnd_drop_update_func)(ViewDir *vd);
	void (*dnd_drop_leave_func)(ViewDir *vd);

	LayoutWindow *layout;

	GtkWidget *popup;

	PixmapFolders *pf;
};

struct _ViewDirInfoList
{
	GList *list;
};

struct _ViewDirInfoTree
{
	guint drop_expand_id; /**< event source id */
	gint busy_ref;
};


struct _ViewFile
{
	FileViewType type; 	/**< @todo (xsdg): Turn this into a union (see VFLIST and VFICON from view_file.h). */

	gpointer info;

	GtkWidget *widget;
	GtkWidget *listview;
	GtkWidget *scrolled;
	GtkWidget *filter;
	GtkWidget *filter_check[FILEDATA_MARKS_SIZE];

	struct {
		GtkWidget *combo;
		GtkWidget *frame;
		gint count;
		gint last_selected;
		gboolean case_sensitive;
	} file_filter;

	FileData *dir_fd;
	GList *list;

	SortType sort_method;
	gboolean sort_ascend;

	/* func list */
	void (*func_thumb_status)(ViewFile *vf, gdouble val, const gchar *text, gpointer data);
	gpointer data_thumb_status;

	void (*func_status)(ViewFile *vf, gpointer data);
	gpointer data_status;

	LayoutWindow *layout;

	GtkWidget *popup;

	/* thumbs updates*/
	gboolean thumbs_running;
	ThumbLoader *thumbs_loader;
	FileData *thumbs_filedata;

	/* marks */
	gboolean marks_enabled;
	gint active_mark;
	gint clicked_mark;

	/* stars */
	FileData *stars_filedata;
	guint stars_id;

	/* refresh */
	guint refresh_idle_id; /**< event source id */
	time_t time_refresh_set; /**< time when refresh_idle_id was set */

	GList *editmenu_fd_list; /**< file list for edit menu */

	guint read_metadata_in_idle_id;
};

struct _ViewFileInfoList
{
	FileData *click_fd;
	FileData *select_fd;

	gboolean thumbs_enabled;

	guint select_idle_id; /**< event source id */
};

struct _ViewFileInfoIcon
{
	/* table stuff */
	gint columns;
	gint rows;

	GList *selection;
	FileData *prev_selection;

	GtkWidget *tip_window;
	guint tip_delay_id; /**< event source id */
	FileData *tip_fd;

	FileData *click_fd;

	FileData *focus_fd;
	gint focus_row;
	gint focus_column;

	gboolean show_text;
};

struct _SlideShowData
{
	LayoutWindow *lw;        /**< use this window to display the slideshow */
	ImageWindow *imd;        /**< use this window only if lw is not available,
	                            @FIXME it is probably required only by img-view.c and should be dropped with it */

	GList *filelist;
	CollectionData *cd;
	FileData *dir_fd;

	GList *list;
	GList *list_done;

	FileData *slide_fd;

	guint slide_count;
	guint timeout_id; /**< event source id */

	gboolean from_selection;

	void (*stop_func)(SlideShowData *, gpointer);
	gpointer stop_data;

	gboolean paused;
};

struct _FullScreenData
{
	GtkWidget *window;
	ImageWindow *imd;

	GtkWidget *normal_window;
	ImageWindow *normal_imd;

	guint hide_mouse_id; /**< event source id */
	guint busy_mouse_id; /**< event source id */

	gint cursor_state;

	guint saver_block_id; /**< event source id */

	void (*stop_func)(FullScreenData *, gpointer);
	gpointer stop_data;

	gboolean same_region; /**< the returned region will overlap the current location of widget. */
};

struct _PixmapFolders
{
	GdkPixbuf *close;
	GdkPixbuf *open;
	GdkPixbuf *deny;
	GdkPixbuf *parent;
	GdkPixbuf *link;
};

struct _SecureSaveInfo {
	FILE *fp; /**< file stream pointer */
	gchar *file_name; /**< final file name */
	gchar *tmp_file_name; /**< temporary file name */
	gint err; /**< set to non-zero value in case of error */
	gboolean secure_save; /**< use secure save for this file, internal use only */
	gboolean preserve_perms; /**< whether to preserve perms, TRUE by default */
	gboolean preserve_mtime; /**< whether to preserve mtime, FALSE by default */
	gboolean unlink_on_error; /**< whether to remove temporary file on save failure, TRUE by default */
};

struct _CommandLine
{
	int argc;
	gchar **argv;
	gboolean startup_blank;
	gboolean startup_full_screen;
	gboolean startup_in_slideshow;
	gboolean startup_command_line_collection;
	gboolean tools_hide;
	gboolean tools_show;
	gboolean log_window_show;
	gchar *path;
	gchar *file;
	GList *cmd_list;
	GList *collection_list;
	gchar *geometry;
	gchar *regexp;
	gchar *log_file;
	SecureSaveInfo *ssi;
	gboolean new_instance;
};

/**
 * @struct _hard_coded_window_keys
 * @brief hard coded window shortcut keys
 * 
 * Used for two purposes:\n
 * to display the shortcuts keys in popup menus\n
 * used by ./doc/create-shortcuts-xml.sh to generate shortcut documentation in the Help files
 * 
 */
typedef struct _hard_coded_window_keys hard_coded_window_keys;
struct _hard_coded_window_keys {
	GdkModifierType mask; /**< modifier key mask */
	guint key_value;  /**< GDK_keyval */
	gchar *text;  /**< menu item label - NULL if end of list */
};

#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
