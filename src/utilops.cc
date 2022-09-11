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
#include "utilops.h"

#include "cache.h"
#include "cache-maint.h"
#include "collect.h"
#include "dupe.h"
#include "filedata.h"
#include "filefilter.h"
#include "image.h"
#include "img-view.h"
#include "layout.h"
#include "search.h"
#include "thumb-standard.h"
#include "trash.h"
#include "ui-bookmark.h"
#include "ui-fileops.h"
#include "ui-misc.h"
#include "ui-tabcomp.h"
#include "editors.h"
#include "metadata.h"
#include "exif.h"

#define DIALOG_WIDTH 750

static GdkPixbuf *file_util_get_error_icon(FileData *fd, GList *list, GtkWidget *widget);

/*
 *--------------------------------------------------------------------------
 * Adds 1 or 2 images (if 2, side by side) to a GenericDialog
 *--------------------------------------------------------------------------
 */

#define DIALOG_DEF_IMAGE_DIM_X 150
#define DIALOG_DEF_IMAGE_DIM_Y 100

static void generic_dialog_add_image(GenericDialog *gd, GtkWidget *box,
				     FileData *fd1, const gchar *header1,
				     gboolean second_image,
				     FileData *fd2, const gchar *header2,
				     gboolean show_filename)
{
	ImageWindow *imd;
	GtkWidget *preview_box = NULL;
	GtkWidget *vbox;
	GtkWidget *label = NULL;

	if (!box) box = gd->vbox;

	if (second_image)
		{
		preview_box = pref_box_new(box, FALSE, GTK_ORIENTATION_VERTICAL, PREF_PAD_SPACE);
		}

	/* image 1 */

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);
	if (preview_box)
		{
		GtkWidget *sep;

		gtk_box_pack_start(GTK_BOX(preview_box), vbox, FALSE, TRUE, 0);

		sep = gtk_hseparator_new();
		gtk_box_pack_start(GTK_BOX(preview_box), sep, FALSE, FALSE, 0);
		gtk_widget_show(sep);
		}
	else
		{
		gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, TRUE, PREF_PAD_GAP);
		}
	gtk_widget_show(vbox);

	if (header1)
		{
		GtkWidget *head;

		head = pref_label_new(vbox, header1);
		pref_label_bold(head, TRUE, FALSE);
		gtk_label_set_xalign(GTK_LABEL(head), 0.0);
		gtk_label_set_yalign(GTK_LABEL(head), 0.5);
		}

	imd = image_new(FALSE);
	g_object_set(G_OBJECT(imd->pr), "zoom_expand", FALSE, NULL);
	gtk_widget_set_size_request(imd->widget, DIALOG_DEF_IMAGE_DIM_X, DIALOG_DEF_IMAGE_DIM_Y);
	gtk_box_pack_start(GTK_BOX(vbox), imd->widget, TRUE, TRUE, 0);
	image_change_fd(imd, fd1, 0.0);
	gtk_widget_show(imd->widget);

	if (show_filename)
		{
		label = pref_label_new(vbox, (fd1 == NULL) ? "" : fd1->name);
		}

	/* only the first image is stored (for use in gd_image_set) */
	g_object_set_data(G_OBJECT(gd->dialog), "img_image", imd);
	g_object_set_data(G_OBJECT(gd->dialog), "img_label", label);


	/* image 2 */

	if (preview_box)
		{
		vbox = pref_box_new(preview_box, TRUE, GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);

		if (header2)
			{
			GtkWidget *head;

			head = pref_label_new(vbox, header2);
			pref_label_bold(head, TRUE, FALSE);
			gtk_label_set_xalign(GTK_LABEL(head), 0.0);
			gtk_label_set_yalign(GTK_LABEL(head), 0.5);
			}

		imd = image_new(FALSE);
		g_object_set(G_OBJECT(imd->pr), "zoom_expand", FALSE, NULL);
		gtk_widget_set_size_request(imd->widget, DIALOG_DEF_IMAGE_DIM_X, DIALOG_DEF_IMAGE_DIM_Y);
		gtk_box_pack_start(GTK_BOX(vbox), imd->widget, TRUE, TRUE, 0);
		if (fd2) image_change_fd(imd, fd2, 0.0);
		gtk_widget_show(imd->widget);

		if (show_filename)
			{
			label = pref_label_new(vbox, (fd2 == NULL) ? "" : fd2->name);
			}
		g_object_set_data(G_OBJECT(gd->dialog), "img_image2", imd);
		g_object_set_data(G_OBJECT(gd->dialog), "img_label2", label);
		}
}

/*
 *--------------------------------------------------------------------------
 * Wrappers to aid in setting additional dialog properties (unde mouse, etc.)
 *--------------------------------------------------------------------------
 */

GenericDialog *file_util_gen_dlg(const gchar *title,
				 const gchar *role,
				 GtkWidget *parent, gboolean auto_close,
				 void (*cancel_cb)(GenericDialog *, gpointer), gpointer data)
{
	GenericDialog *gd;

	gd = generic_dialog_new(title, role, parent, auto_close, cancel_cb, data);
	if (options->place_dialogs_under_mouse)
		{
		gtk_window_set_position(GTK_WINDOW(gd->dialog), GTK_WIN_POS_MOUSE);
		}

	return gd;
}

FileDialog *file_util_file_dlg(const gchar *title,
			       const gchar *role,
			       GtkWidget *parent,
			       void (*cancel_cb)(FileDialog *, gpointer), gpointer data)
{
	FileDialog *fdlg;

	fdlg = file_dialog_new(title, role, parent, cancel_cb, data);
	if (options->place_dialogs_under_mouse)
		{
		gtk_window_set_position(GTK_WINDOW(GENERIC_DIALOG(fdlg)->dialog), GTK_WIN_POS_MOUSE);
		}

	return fdlg;
}

/* this warning dialog is copied from SLIK's ui_utildg.c,
 * because it does not have a mouse center option,
 * and we must center it before show, implement it here.
 */
static void file_util_warning_dialog_ok_cb(GenericDialog *UNUSED(gd), gpointer UNUSED(data))
{
	/* no op */
}

GenericDialog *file_util_warning_dialog(const gchar *heading, const gchar *message,
					const gchar *icon_stock_id, GtkWidget *parent)
{
	GenericDialog *gd;

	gd = file_util_gen_dlg(heading, "warning", parent, TRUE, NULL, NULL);
	generic_dialog_add_message(gd, icon_stock_id, heading, message, TRUE);
	generic_dialog_add_button(gd, GTK_STOCK_OK, NULL, file_util_warning_dialog_ok_cb, TRUE);
	if (options->place_dialogs_under_mouse)
		{
		gtk_window_set_position(GTK_WINDOW(gd->dialog), GTK_WIN_POS_MOUSE);
		}
	gtk_widget_show(gd->dialog);

	return gd;
}

static gint filename_base_length(const gchar *name)
{
	gint n;

	if (!name) return 0;

	n = strlen(name);

	if (filter_name_exists(name))
		{
		const gchar *ext;

		ext = registered_extension_from_path(name);
		if (ext) n -= strlen(ext);
		}

	return n;
}




typedef enum {
	UTILITY_TYPE_COPY,
	UTILITY_TYPE_MOVE,
	UTILITY_TYPE_RENAME,
	UTILITY_TYPE_RENAME_FOLDER,
	UTILITY_TYPE_EDITOR,
	UTILITY_TYPE_FILTER,
	UTILITY_TYPE_DELETE,
	UTILITY_TYPE_DELETE_LINK,
	UTILITY_TYPE_DELETE_FOLDER,
	UTILITY_TYPE_CREATE_FOLDER,
	UTILITY_TYPE_WRITE_METADATA
} UtilityType;

typedef enum {
	UTILITY_PHASE_START = 0,
	UTILITY_PHASE_INTERMEDIATE,
	UTILITY_PHASE_ENTERING,
	UTILITY_PHASE_CHECKED,
	UTILITY_PHASE_DONE,
	UTILITY_PHASE_CANCEL,
	UTILITY_PHASE_DISCARD
} UtilityPhase;

enum {
	UTILITY_RENAME = 0,
	UTILITY_RENAME_AUTO,
	UTILITY_RENAME_FORMATTED
};

typedef struct _UtilityDataMessages UtilityDataMessages;
struct _UtilityDataMessages {
	gchar *title;
	gchar *question;
	gchar *desc_flist;
	gchar *desc_source_fd;
	gchar *fail;
};

typedef struct _UtilityData UtilityData;

struct _UtilityData {
	UtilityType type;
	UtilityPhase phase;

	FileData *dir_fd;
	GList *content_list;
	GList *flist;

	FileData *sel_fd;

	GtkWidget *parent;
	GenericDialog *gd;
	FileDialog *fdlg;

	guint update_idle_id; /* event source id */
	guint perform_idle_id; /* event source id */

	gboolean with_sidecars; /* operate on grouped or single files; TRUE = use file_data_sc_, FALSE = use file_data_ functions */

	/* alternative dialog parts */
	GtkWidget *notebook;

	UtilityDataMessages messages;

	/* helper entries for various modes */
	GtkWidget *rename_entry;
	GtkWidget *rename_label;
	GtkWidget *auto_entry_front;
	GtkWidget *auto_entry_end;
	GtkWidget *auto_spin_start;
	GtkWidget *auto_spin_pad;
	GtkWidget *format_entry;
	GtkWidget *format_spin;

	GtkWidget *listview;


	gchar *dest_path;

	/* data for the operation itself, internal or external */
	gboolean external; /* TRUE for external command, FALSE for internal */

	gchar *external_command;
	gpointer resume_data;

	FileUtilDoneFunc done_func;
	void (*details_func)(UtilityData *ud, FileData *fd);
	gboolean (*finalize_func)(FileData *fd);
	gboolean (*discard_func)(FileData *fd);
	gpointer done_data;
};

enum {
	UTILITY_COLUMN_FD = 0,
	UTILITY_COLUMN_PIXBUF,
	UTILITY_COLUMN_PATH,
	UTILITY_COLUMN_NAME,
	UTILITY_COLUMN_SIDECARS,
	UTILITY_COLUMN_DEST_PATH,
	UTILITY_COLUMN_DEST_NAME,
	UTILITY_COLUMN_COUNT
};

typedef struct _UtilityDelayData UtilityDelayData;

struct _UtilityDelayData {
	UtilityType type;
	UtilityPhase phase;
	GList *flist;
	gchar *dest_path;
	gchar *editor_key;
	GtkWidget *parent;
	guint idle_id; /* event source id */
	};

static void generic_dialog_image_set(UtilityData *ud, FileData *fd)
{
	ImageWindow *imd;
	GtkWidget *label;
	FileData *fd2 = NULL;
	gchar *buf;

	imd = (ImageWindow *)g_object_get_data(G_OBJECT(ud->gd->dialog), "img_image");
	label = (GtkWidget *)g_object_get_data(G_OBJECT(ud->gd->dialog), "img_label");

	if (!imd) return;

	image_change_fd(imd, fd, 0.0);
	buf = g_strjoin("\n", text_from_time(fd->date), text_from_size(fd->size), NULL);
	if (label) gtk_label_set_text(GTK_LABEL(label), buf);
	g_free(buf);

	if (ud->type == UTILITY_TYPE_RENAME || ud->type == UTILITY_TYPE_COPY || ud->type == UTILITY_TYPE_MOVE)
		{
		imd = (ImageWindow *)g_object_get_data(G_OBJECT(ud->gd->dialog), "img_image2");
		label = (GtkWidget *)g_object_get_data(G_OBJECT(ud->gd->dialog), "img_label2");

		if (imd)
			{
			if (isfile(fd->change->dest))
				{
				fd2 = file_data_new_group(fd->change->dest);
				image_change_fd(imd, fd2, 0.0);
				buf = g_strjoin("\n", text_from_time(fd2->date), text_from_size(fd2->size), NULL);
				if (label && fd->change->dest) gtk_label_set_text(GTK_LABEL(label), buf);
				file_data_unref(fd2);
				g_free(buf);
				}
			else
				{
				image_change_fd(imd, NULL, 0.0);
				if (label) gtk_label_set_text(GTK_LABEL(label), "");
				}
			}
		}
}

static gboolean file_util_write_metadata_first(UtilityType type, UtilityPhase phase, GList *flist, const gchar *dest_path, const gchar *editor_key, GtkWidget *parent);

#define UTILITY_LIST_MIN_WIDTH  250
#define UTILITY_LIST_MIN_HEIGHT 150

/* thumbnail spec has a max depth of 4 (.thumb??/fail/appname/??.png) */
#define UTILITY_DELETE_MAX_DEPTH 5

static UtilityData *file_util_data_new(UtilityType type)
{
	UtilityData *ud;

	ud = g_new0(UtilityData, 1);

	ud->type = type;
	ud->phase = UTILITY_PHASE_START;

	return ud;
}

static void file_util_data_free(UtilityData *ud)
{
	if (!ud) return;

	if (ud->update_idle_id) g_source_remove(ud->update_idle_id);
	if (ud->perform_idle_id) g_source_remove(ud->perform_idle_id);

	file_data_unref(ud->dir_fd);
	filelist_free(ud->content_list);
	filelist_free(ud->flist);

	if (ud->gd) generic_dialog_close(ud->gd);

	g_free(ud->dest_path);
	g_free(ud->external_command);

	g_free(ud);
}

static GtkTreeViewColumn *file_util_dialog_add_list_column(GtkWidget *view, const gchar *text, gboolean image, gint n)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, text);
	gtk_tree_view_column_set_min_width(column, 4);
	if (image)
		{
		gtk_tree_view_column_set_min_width(column, 20);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
		renderer = gtk_cell_renderer_pixbuf_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", n);
		}
	else
		{
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", n);
		}
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

	return column;
}

static void file_util_dialog_list_select(GtkWidget *view, gint n)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	if (gtk_tree_model_iter_nth_child(store, &iter, NULL, n))
		{
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
		gtk_tree_selection_select_iter(selection, &iter);
		}
}

static GtkWidget *file_util_dialog_add_list(GtkWidget *box, GList *list, gboolean full_paths, gboolean with_sidecars)
{
	GtkWidget *scrolled;
	GtkWidget *view;
	GtkListStore *store;

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);
	gtk_widget_show(scrolled);

	store = gtk_list_store_new(UTILITY_COLUMN_COUNT, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), TRUE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), FALSE);

	file_util_dialog_add_list_column(view, "", TRUE, UTILITY_COLUMN_PIXBUF);

	if (full_paths)
		{
		file_util_dialog_add_list_column(view, _("Path"), FALSE, UTILITY_COLUMN_PATH);
		}
	else
		{
		file_util_dialog_add_list_column(view, _("Name"), FALSE, UTILITY_COLUMN_NAME);
		}

	gtk_widget_set_size_request(view, UTILITY_LIST_MIN_WIDTH, UTILITY_LIST_MIN_HEIGHT);
	gtk_container_add(GTK_CONTAINER(scrolled), view);
	gtk_widget_show(view);

	while (list)
		{
		FileData *fd = (FileData *)list->data;
		GtkTreeIter iter;
		gchar *sidecars;

		sidecars = with_sidecars ? file_data_sc_list_to_string(fd) : NULL;
		GdkPixbuf *icon = file_util_get_error_icon(fd, list, view);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				   UTILITY_COLUMN_FD, fd,
				   UTILITY_COLUMN_PIXBUF, icon,
				   UTILITY_COLUMN_PATH, fd->path,
				   UTILITY_COLUMN_NAME, fd->name,
				   UTILITY_COLUMN_SIDECARS, sidecars,
				   UTILITY_COLUMN_DEST_PATH, fd->change ? fd->change->dest : "error",
				   UTILITY_COLUMN_DEST_NAME, fd->change ? filename_from_path(fd->change->dest) : "error",
				   -1);
		g_free(sidecars);

		list = list->next;
		}

	return view;
}


static gboolean file_util_perform_ci_internal(gpointer data);
void file_util_dialog_run(UtilityData *ud);
static gint file_util_perform_ci_cb(gpointer resume_data, EditorFlags flags, GList *list, gpointer data);

/* call file_util_perform_ci_internal or start_editor_from_filelist_full */


static void file_util_resume_cb(GenericDialog *UNUSED(gd), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	if (ud->external)
		editor_resume(ud->resume_data);
	else
		file_util_perform_ci_internal(ud);
}

static void file_util_abort_cb(GenericDialog *UNUSED(gd), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	if (ud->external)
		editor_skip(ud->resume_data);
	else
		file_util_perform_ci_cb(NULL, EDITOR_ERROR_SKIPPED, ud->flist, ud);

}


static gint file_util_perform_ci_cb(gpointer resume_data, EditorFlags flags, GList *list, gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	gint ret = EDITOR_CB_CONTINUE;

	ud->resume_data = resume_data;

	if (EDITOR_ERRORS_BUT_SKIPPED(flags))
		{
		GString *msg = g_string_new(editor_get_error_str(flags));
		GenericDialog *d;
		g_string_append(msg, "\n");
		g_string_append(msg, ud->messages.fail);
		g_string_append(msg, "\n");
		while (list)
			{
			FileData *fd = (FileData *)list->data;

			g_string_append(msg, fd->path);
			g_string_append(msg, "\n");
			list = list->next;
			}
		if (resume_data)
			{
			g_string_append(msg, _("\n Continue multiple file operation?"));
			d = file_util_gen_dlg(ud->messages.fail, "dlg_confirm",
					      NULL, TRUE,
					      file_util_abort_cb, ud);

			generic_dialog_add_message(d, GTK_STOCK_DIALOG_WARNING, NULL, msg->str, TRUE);

			generic_dialog_add_button(d, GTK_STOCK_GO_FORWARD, _("Co_ntinue"),
						  file_util_resume_cb, TRUE);
			gtk_widget_show(d->dialog);
			ret = EDITOR_CB_SUSPEND;
			}
		else
			{
			file_util_warning_dialog(ud->messages.fail, msg->str, GTK_STOCK_DIALOG_ERROR, NULL);
			}
		g_string_free(msg, TRUE);
		}


	while (list)  /* be careful, file_util_perform_ci_internal can pass ud->flist as list */
		{
		FileData *fd = (FileData *)list->data;
		list = list->next;

		if (!EDITOR_ERRORS(flags)) /* files were successfully deleted, call the maint functions */
			{
			if (ud->with_sidecars)
				file_data_sc_apply_ci(fd);
			else
				file_data_apply_ci(fd);
			}

		ud->flist = g_list_remove(ud->flist, fd);

		if (ud->finalize_func)
			{
			ud->finalize_func(fd);
			}

		if (ud->with_sidecars)
			file_data_sc_free_ci(fd);
		else
			file_data_free_ci(fd);
		file_data_unref(fd);
		}

	if (!resume_data) /* end of the list */
		{
		ud->phase = UTILITY_PHASE_DONE;
		file_util_dialog_run(ud);
		}

	return ret;
}


/*
 * Perform the operation described by FileDataChangeInfo on all files in the list
 * it is an alternative to start_editor_from_filelist_full, it should use similar interface
 */


static gboolean file_util_perform_ci_internal(gpointer data)
{
	UtilityData *ud = (UtilityData*)data;

	if (!ud->perform_idle_id)
		{
		/* this function was called directly
		   just setup idle callback and wait until we are called again
		*/

		/* this is removed when ud is destroyed */
		ud->perform_idle_id = g_idle_add(file_util_perform_ci_internal, ud);
		return TRUE;
		}

	g_assert(ud->flist);

	if (ud->flist)
		{
		gint ret;

		/* take a single entry each time, this allows better control over the operation */
		GList *single_entry = g_list_append(NULL, ud->flist->data);
		gboolean last = !ud->flist->next;
		EditorFlags status = EDITOR_ERROR_STATUS;

		if (ud->with_sidecars ? file_data_sc_perform_ci(single_entry->data)
		                      : file_data_perform_ci(single_entry->data))
			status = 0; /* OK */

		ret = file_util_perform_ci_cb(GINT_TO_POINTER(!last), status, single_entry, ud);
		g_list_free(single_entry);

		if (ret == EDITOR_CB_SUSPEND || last) return FALSE;

		if (ret == EDITOR_CB_SKIP)
			{
			file_util_perform_ci_cb(NULL, EDITOR_ERROR_SKIPPED, ud->flist, ud);
			return FALSE;
			}
		}

	return TRUE;
}

static void file_util_perform_ci_dir(UtilityData *ud, gboolean internal, gboolean ext_result)
{
	switch (ud->type)
		{
		case UTILITY_TYPE_DELETE_LINK:
			{
			g_assert(ud->dir_fd->sidecar_files == NULL); // directories should not have sidecars
			if ((internal && file_data_perform_ci(ud->dir_fd)) ||
			    (!internal && ext_result))
				{
				file_data_apply_ci(ud->dir_fd);
				}
			else
				{
				gchar *text;

				text = g_strdup_printf("%s:\n\n%s", ud->messages.fail, ud->dir_fd->path);
				file_util_warning_dialog(ud->messages.fail, text, GTK_STOCK_DIALOG_ERROR, NULL);
				g_free(text);
				}
			file_data_free_ci(ud->dir_fd);
			break;
			}
		case UTILITY_TYPE_DELETE_FOLDER:
			{
			FileData *fail = NULL;
			GList *work;
			work = ud->content_list;
			while (work)
				{
				FileData *fd;

				fd = (FileData *)work->data;
				work = work->next;

				if (!fail)
					{
					if ((internal && file_data_sc_perform_ci(fd)) ||
					    (!internal && ext_result))
						{
						file_data_sc_apply_ci(fd);
						}
					else
						{
						if (internal) fail = file_data_ref(fd);
						}
					}
				file_data_sc_free_ci(fd);
				}

			if (!fail)
				{
				g_assert(ud->dir_fd->sidecar_files == NULL); // directories should not have sidecars
				if ((internal && file_data_sc_perform_ci(ud->dir_fd)) ||
				    (!internal && ext_result))
					{
					file_data_apply_ci(ud->dir_fd);
					}
				else
					{
					fail = file_data_ref(ud->dir_fd);
					}
				}

			if (fail)
				{
				gchar *text;
				GenericDialog *gd;

				text = g_strdup_printf("%s:\n\n%s", ud->messages.fail, ud->dir_fd->path);
				gd = file_util_warning_dialog(ud->messages.fail, text, GTK_STOCK_DIALOG_ERROR, NULL);
				g_free(text);

				if (fail != ud->dir_fd)
					{
					pref_spacer(gd->vbox, PREF_PAD_GROUP);
					text = g_strdup_printf(_("Removal of folder contents failed at this file:\n\n%s"),
								fail->path);
					pref_label_new(gd->vbox, text);
					g_free(text);
					}

				file_data_unref(fail);
				}
			break;
			}
		case UTILITY_TYPE_RENAME_FOLDER:
			{
			FileData *fail = NULL;
			GList *work;
			g_assert(ud->dir_fd->sidecar_files == NULL); // directories should not have sidecars

			if ((internal && file_data_sc_perform_ci(ud->dir_fd)) ||
			    (!internal && ext_result))
				{
				file_data_sc_apply_ci(ud->dir_fd);
				}
			else
				{
				fail = file_data_ref(ud->dir_fd);
				}


			work = ud->content_list;
			while (work)
				{
				FileData *fd;

				fd = (FileData *)work->data;
				work = work->next;

				if (!fail)
					{
					file_data_sc_apply_ci(fd);
					}
				file_data_sc_free_ci(fd);
				}

			if (fail)
				{
				gchar *text;

				text = g_strdup_printf("%s:\n\n%s", ud->messages.fail, ud->dir_fd->path);
				(void) file_util_warning_dialog(ud->messages.fail, text, GTK_STOCK_DIALOG_ERROR, NULL);
				g_free(text);

				file_data_unref(fail);
				}
			break;
			}
		case UTILITY_TYPE_CREATE_FOLDER:
			{
			if ((internal && mkdir_utf8(ud->dir_fd->path, 0755)) ||
			    (!internal && ext_result))
				{
				file_data_check_changed_files(ud->dir_fd); /* this will update the FileData and send notification */
				}
			else
				{
				gchar *text;

				text = g_strdup_printf("%s:\n\n%s", ud->messages.fail, ud->dir_fd->path);
				(void) file_util_warning_dialog(ud->messages.fail, text, GTK_STOCK_DIALOG_ERROR, NULL);
				g_free(text);
				}

			break;
			}
		default:
			g_warning("unhandled operation");
		}
	ud->phase = UTILITY_PHASE_DONE;
	file_util_dialog_run(ud);
}

static gint file_util_perform_ci_dir_cb(gpointer UNUSED(resume_data), EditorFlags flags, GList *UNUSED(list), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	file_util_perform_ci_dir(ud, FALSE, !EDITOR_ERRORS_BUT_SKIPPED(flags));
	return EDITOR_CB_CONTINUE; /* does not matter, there was just single directory */
}

void file_util_perform_ci(UtilityData *ud)
{
	switch (ud->type)
		{
		case UTILITY_TYPE_COPY:
			ud->external_command = g_strdup(CMD_COPY);
			break;
		case UTILITY_TYPE_MOVE:
			ud->external_command = g_strdup(CMD_MOVE);
			break;
		case UTILITY_TYPE_RENAME:
		case UTILITY_TYPE_RENAME_FOLDER:
			ud->external_command = g_strdup(CMD_RENAME);
			break;
		case UTILITY_TYPE_DELETE:
		case UTILITY_TYPE_DELETE_LINK:
		case UTILITY_TYPE_DELETE_FOLDER:
			ud->external_command = g_strdup(CMD_DELETE);
			break;
		case UTILITY_TYPE_CREATE_FOLDER:
			ud->external_command = g_strdup(CMD_FOLDER);
			break;
		case UTILITY_TYPE_FILTER:
		case UTILITY_TYPE_EDITOR:
			g_assert(ud->external_command != NULL); /* it should be already set */
			break;
		case UTILITY_TYPE_WRITE_METADATA:
			ud->external_command = NULL;
		}

	if (is_valid_editor_command(ud->external_command))
		{
		EditorFlags flags;

		ud->external = TRUE;

		if (ud->dir_fd)
			{
			flags = start_editor_from_file_full(ud->external_command, ud->dir_fd, file_util_perform_ci_dir_cb, ud);
			}
		else
			{
			if (editor_blocks_file(ud->external_command))
				{
				DEBUG_1("Starting %s and waiting for results", ud->external_command);
				flags = start_editor_from_filelist_full(ud->external_command, ud->flist, NULL, file_util_perform_ci_cb, ud);
				}
			else
				{
				/* start the editor without callback and finish the operation internally */
				DEBUG_1("Starting %s and finishing the operation", ud->external_command);
				flags = start_editor_from_filelist(ud->external_command, ud->flist);
				file_util_perform_ci_internal(ud);
				}
			}

		if (EDITOR_ERRORS(flags))
			{
			gchar *text = g_strdup_printf(_("%s\nUnable to start external command.\n"), editor_get_error_str(flags));
			file_util_warning_dialog(ud->messages.fail, text, GTK_STOCK_DIALOG_ERROR, NULL);
			g_free(text);

			ud->gd = NULL;
			ud->phase = UTILITY_PHASE_CANCEL;
			file_util_dialog_run(ud);
			}
		}
	else
		{
		ud->external = FALSE;
		if (ud->dir_fd)
			{
			file_util_perform_ci_dir(ud, TRUE, FALSE);
			}
		else
			{
			file_util_perform_ci_internal(ud);
			}
		}
}

static GdkPixbuf *file_util_get_error_icon(FileData *fd, GList *list, GtkWidget *widget)
{
	static GdkPixbuf *pb_warning;
	static GdkPixbuf *pb_error;
	static GdkPixbuf *pb_apply;
	gint error;

	if (!pb_warning)
		{
		pb_warning = gtk_widget_render_icon(widget, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU, NULL);
		}

	if (!pb_error)
		{
		pb_error = gtk_widget_render_icon(widget, GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU, NULL);
		}

	if (!pb_apply)
		{
		pb_apply = gtk_widget_render_icon(widget, GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU, NULL);
		}

	error = file_data_sc_verify_ci(fd, list);

	if (!error) return pb_apply;

	if (error & CHANGE_ERROR_MASK)
		{
		return pb_error;
		}
	else
		{
		return pb_warning;
		}
}

static void file_util_check_resume_cb(GenericDialog *UNUSED(gd), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	ud->phase = UTILITY_PHASE_CHECKED;
	file_util_dialog_run(ud);
}

static void file_util_check_abort_cb(GenericDialog *UNUSED(gd), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	ud->phase = UTILITY_PHASE_START;
	file_util_dialog_run(ud);
}

void file_util_check_ci(UtilityData *ud)
{
	gint error = CHANGE_OK;
	gchar *desc = NULL;

	if (ud->type != UTILITY_TYPE_CREATE_FOLDER &&
	    ud->type != UTILITY_TYPE_RENAME_FOLDER)
		{
		if (ud->dest_path && !isdir(ud->dest_path))
			{
			error = CHANGE_GENERIC_ERROR;
			desc = g_strdup_printf(_("%s is not a directory"), ud->dest_path);
			}
		else if (ud->dir_fd)
			{
			g_assert(ud->dir_fd->sidecar_files == NULL); // directories should not have sidecars
			error = file_data_verify_ci(ud->dir_fd, ud->flist);
			if (error) desc = file_data_get_error_string(error);
			}
		else
			{
			error = file_data_verify_ci_list(ud->flist, &desc, ud->with_sidecars);
			}
		}
	else
		{
		if (ud->type == UTILITY_TYPE_CREATE_FOLDER || ud->type == UTILITY_TYPE_RENAME_FOLDER)
			{
			if (isdir(ud->dest_path) || isfile(ud->dest_path))
				{
				error = CHANGE_DEST_EXISTS;
				desc = g_strdup_printf(_("%s already exists"), ud->dest_path);
				}
			}
		}

	if (!error)
		{
		ud->phase = UTILITY_PHASE_CHECKED;
		file_util_dialog_run(ud);
		return;
		}

	if (!(error & CHANGE_ERROR_MASK))
		{
		/* just a warning */
		GenericDialog *d;

		d = file_util_gen_dlg(ud->messages.title, "dlg_confirm",
					ud->parent, TRUE,
					file_util_check_abort_cb, ud);

		generic_dialog_add_message(d, GTK_STOCK_DIALOG_WARNING, _("Really continue?"), desc, TRUE);

		generic_dialog_add_button(d, GTK_STOCK_GO_FORWARD, _("Co_ntinue"),
					  file_util_check_resume_cb, TRUE);
		gtk_widget_show(d->dialog);
		}
	else
		{
		/* fatal error */
		GenericDialog *d;

		d = file_util_gen_dlg(ud->messages.title, "dlg_confirm",
					ud->parent, TRUE,
					file_util_check_abort_cb, ud);
		generic_dialog_add_message(d, GTK_STOCK_DIALOG_WARNING, _("This operation can't continue:"), desc, TRUE);

		gtk_widget_show(d->dialog);
		}
	g_free(desc);
}





static void file_util_cancel_cb(GenericDialog *gd, gpointer data)
{
	UtilityData *ud = (UtilityData*)data;

	generic_dialog_close(gd);

	ud->gd = NULL;

	ud->phase = UTILITY_PHASE_CANCEL;
	file_util_dialog_run(ud);
}

static void file_util_discard_cb(GenericDialog *gd, gpointer data)
{
	UtilityData *ud = (UtilityData*)data;

	generic_dialog_close(gd);

	ud->gd = NULL;

	ud->phase = UTILITY_PHASE_DISCARD;
	file_util_dialog_run(ud);
}

static void file_util_ok_cb(GenericDialog *gd, gpointer data)
{
	UtilityData *ud = (UtilityData*)data;

	generic_dialog_close(gd);

	ud->gd = NULL;

	file_util_dialog_run(ud);
}

static void file_util_fdlg_cancel_cb(FileDialog *fdlg, gpointer data)
{
	UtilityData *ud = (UtilityData*)data;

	file_dialog_close(fdlg);

	ud->fdlg = NULL;

	ud->phase = UTILITY_PHASE_CANCEL;
	file_util_dialog_run(ud);
}

static void file_util_dest_folder_update_path(UtilityData *ud)
{
	g_free(ud->dest_path);
	ud->dest_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(ud->fdlg->entry)));

	switch (ud->type)
		{
		case UTILITY_TYPE_COPY:
			file_data_sc_update_ci_copy_list(ud->flist, ud->dest_path);
			break;
		case UTILITY_TYPE_MOVE:
			file_data_sc_update_ci_move_list(ud->flist, ud->dest_path);
			break;
		case UTILITY_TYPE_FILTER:
		case UTILITY_TYPE_EDITOR:
			file_data_sc_update_ci_unspecified_list(ud->flist, ud->dest_path);
			break;
		case UTILITY_TYPE_CREATE_FOLDER:
			file_data_unref(ud->dir_fd);
			ud->dir_fd = file_data_new_dir(ud->dest_path);
			break;
		case UTILITY_TYPE_DELETE:
		case UTILITY_TYPE_DELETE_LINK:
		case UTILITY_TYPE_DELETE_FOLDER:
		case UTILITY_TYPE_RENAME:
		case UTILITY_TYPE_RENAME_FOLDER:
		case UTILITY_TYPE_WRITE_METADATA:
			g_warning("unhandled operation");
		}
}

static void file_util_fdlg_rename_cb(FileDialog *fdlg, gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	gchar *desc = NULL;
	GenericDialog *d = NULL;

	file_util_dest_folder_update_path(ud);
	if (isdir(ud->dest_path))
		{
		file_dialog_sync_history(fdlg, TRUE);
		file_dialog_close(fdlg);
		ud->fdlg = NULL;
		file_util_dialog_run(ud);
		}
	else
		{
		/* During copy/move operations it is necessary to ensure that the
		 * target directory exists before continuing with the next step.
		 * If not revert to the select directory dialog
		 */
		desc = g_strdup_printf(_("%s is not a directory"), ud->dest_path);

		d = file_util_gen_dlg(ud->messages.title, "dlg_confirm",
					ud->parent, TRUE,
					file_util_check_abort_cb, ud);
		generic_dialog_add_message(d, GTK_STOCK_DIALOG_WARNING, _("This operation can't continue:"), desc, TRUE);

		gtk_widget_show(d->dialog);
		ud->phase = UTILITY_PHASE_START;

		file_dialog_close(fdlg);
		ud->fdlg = NULL;
		g_free(desc);
		}
}

static void file_util_fdlg_ok_cb(FileDialog *fdlg, gpointer data)
{
	UtilityData *ud = (UtilityData*)data;

	file_util_dest_folder_update_path(ud);
	if (isdir(ud->dest_path)) file_dialog_sync_history(fdlg, TRUE);
	file_dialog_close(fdlg);

	ud->fdlg = NULL;
	ud->phase = UTILITY_PHASE_ENTERING;

	file_util_dialog_run(ud);

	return;
}

static void file_util_dest_folder_entry_cb(GtkWidget *UNUSED(entry), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	file_util_dest_folder_update_path(ud);
}

/* format: * = filename without extension, ## = number position, extension is kept */
static gchar *file_util_rename_multiple_auto_format_name(const gchar *format, const gchar *name, gint n)
{
	gchar *new_name;
	gchar *parsed;
	const gchar *ext;
	gchar *middle;
	gchar *tmp;
	gchar *pad_start;
	gchar *pad_end;
	gint padding;

	if (!format || !name) return NULL;

	tmp = g_strdup(format);
	pad_start = strchr(tmp, '#');
	if (pad_start)
		{
		pad_end = pad_start;
		padding = 0;
		while (*pad_end == '#')
			{
			pad_end++;
			padding++;
			}
		*pad_start = '\0';

		parsed = g_strdup_printf("%s%0*d%s", tmp, padding, n, pad_end);
		g_free(tmp);
		}
	else
		{
		parsed = tmp;
		}

	ext = registered_extension_from_path(name);

	middle = strchr(parsed, '*');
	if (middle)
		{
		gchar *base;

		*middle = '\0';
		middle++;

		base = remove_extension_from_path(name);
		new_name = g_strconcat(parsed, base, middle, ext, NULL);
		g_free(base);
		}
	else
		{
		new_name = g_strconcat(parsed, ext, NULL);
		}

	g_free(parsed);

	return new_name;
}


static void file_util_rename_preview_update(UtilityData *ud)
{
	GtkTreeModel *store;
	GtkTreeSelection *selection;
	GtkTreeIter iter, iter_selected;
	GtkTreePath *path_iter, *path_selected;
	const gchar *front;
	const gchar *end;
	const gchar *format;
	gboolean valid;
	gint start_n;
	gint padding;
	gint n;
	gint mode;
	gchar *dirname;
	gchar *destname;

	mode = gtk_notebook_get_current_page(GTK_NOTEBOOK(ud->notebook));

	if (mode == UTILITY_RENAME)
		{
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ud->listview));
		if (gtk_tree_selection_get_selected(selection, &store, &iter))
			{
			FileData *fd;
			const gchar *dest = gtk_entry_get_text(GTK_ENTRY(ud->rename_entry));

			gtk_tree_model_get(store, &iter, UTILITY_COLUMN_FD, &fd, -1);
			g_assert(ud->with_sidecars); /* sidecars must be renamed too, it would break the pairing otherwise */

			dirname = g_path_get_dirname(fd->change->dest);
			destname = g_build_filename(dirname, dest, NULL);
			switch (ud->type)
				{
				case UTILITY_TYPE_RENAME:
					file_data_sc_update_ci_rename(fd, dest);
					break;
				case UTILITY_TYPE_COPY:
					file_data_sc_update_ci_copy(fd, destname);
					break;
				case UTILITY_TYPE_MOVE:
					file_data_sc_update_ci_move(fd, destname);
					break;
				default:;
				}
			generic_dialog_image_set(ud, fd);

			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
				   UTILITY_COLUMN_DEST_PATH, fd->change->dest,
				   UTILITY_COLUMN_DEST_NAME, filename_from_path(fd->change->dest),
				   -1);
			}
		}
	else
		{
		front = gtk_entry_get_text(GTK_ENTRY(ud->auto_entry_front));
		end = gtk_entry_get_text(GTK_ENTRY(ud->auto_entry_end));
		padding = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ud->auto_spin_pad));

		format = gtk_entry_get_text(GTK_ENTRY(ud->format_entry));

		g_free(options->cp_mv_rn.auto_end);
		options->cp_mv_rn.auto_end = g_strdup(end);
		options->cp_mv_rn.auto_padding = padding;

		if (mode == UTILITY_RENAME_FORMATTED)
			{
			start_n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ud->format_spin));
			options->cp_mv_rn.formatted_start = start_n;
			}
		else
			{
			start_n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ud->auto_spin_start));
			options->cp_mv_rn.auto_start = start_n;
			}

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(ud->listview));
		n = start_n;
		valid = gtk_tree_model_get_iter_first(store, &iter);
		while (valid)
			{
			gchar *dest;
			FileData *fd;
			gtk_tree_model_get(store, &iter, UTILITY_COLUMN_FD, &fd, -1);

			if (mode == UTILITY_RENAME_FORMATTED)
				{
				dest = file_util_rename_multiple_auto_format_name(format, fd->name, n);
				}
			else
				{
				dest = g_strdup_printf("%s%0*d%s", front, padding, n, end);
				}

			g_assert(ud->with_sidecars); /* sidecars must be renamed too, it would break the pairing otherwise */

			dirname = g_path_get_dirname(fd->change->dest);
			destname = g_build_filename(dirname, dest, NULL);

			switch (ud->type)
				{
				case UTILITY_TYPE_RENAME:
					file_data_sc_update_ci_rename(fd, dest);
					break;
				case UTILITY_TYPE_COPY:
					file_data_sc_update_ci_copy(fd, destname);
					break;
				case UTILITY_TYPE_MOVE:
					file_data_sc_update_ci_move(fd, destname);
					break;
				default:;
				}

			g_free(dirname);
			g_free(destname);
			g_free(dest);

			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ud->listview));
			gtk_tree_selection_get_selected(selection, &store, &iter_selected);
			path_iter=gtk_tree_model_get_path(store,&iter);
			path_selected=gtk_tree_model_get_path(store,&iter_selected);
			if (!gtk_tree_path_compare(path_iter,path_selected))
				{
				generic_dialog_image_set(ud, fd);
				}
			gtk_tree_path_free(path_iter);
			gtk_tree_path_free(path_selected);

			gtk_list_store_set(GTK_LIST_STORE(store), &iter,
					   UTILITY_COLUMN_DEST_PATH, fd->change->dest,
					   UTILITY_COLUMN_DEST_NAME, filename_from_path(fd->change->dest),
					   -1);
			n++;
			valid = gtk_tree_model_iter_next(store, &iter);
			}
		}

	/* Check the other entries in the list - if there are
	 * multiple destination filenames with the same name the
	 * error icons must be updated
	 */
	valid = gtk_tree_model_get_iter_first(store, &iter);
	while (valid)
		{
		FileData *fd;
		gtk_tree_model_get(store, &iter, UTILITY_COLUMN_FD, &fd, -1);

		gtk_list_store_set(GTK_LIST_STORE(store), &iter,
			   UTILITY_COLUMN_PIXBUF, file_util_get_error_icon(fd, ud->flist, ud->listview),
			   -1);
		valid = gtk_tree_model_iter_next(store, &iter);
		}

}

static void file_util_rename_preview_entry_cb(GtkWidget *UNUSED(entry), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	file_util_rename_preview_update(ud);
}

static void file_util_rename_preview_adj_cb(GtkWidget *UNUSED(spin), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	file_util_rename_preview_update(ud);
}

static gboolean file_util_rename_idle_cb(gpointer data)
{
	UtilityData *ud = (UtilityData*)data;

	file_util_rename_preview_update(ud);

	ud->update_idle_id = 0;
	return FALSE;
}

static void file_util_rename_preview_order_cb(GtkTreeModel *UNUSED(treemodel), GtkTreePath *UNUSED(tpath),
					      GtkTreeIter *UNUSED(iter), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;

	if (ud->update_idle_id) return;

	ud->update_idle_id = g_idle_add(file_util_rename_idle_cb, ud);
}


static gboolean file_util_preview_cb(GtkTreeSelection *UNUSED(selection), GtkTreeModel *store,
				     GtkTreePath *tpath, gboolean path_currently_selected,
				     gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	GtkTreeIter iter;
	FileData *fd = NULL;

	if (path_currently_selected ||
	    !gtk_tree_model_get_iter(store, &iter, tpath)) return TRUE;

	gtk_tree_model_get(store, &iter, UTILITY_COLUMN_FD, &fd, -1);
	generic_dialog_image_set(ud, fd);

	ud->sel_fd = fd;

	if (ud->type == UTILITY_TYPE_RENAME || ud->type == UTILITY_TYPE_COPY || ud->type == UTILITY_TYPE_MOVE)
		{
		const gchar *name = filename_from_path(fd->change->dest);

		gtk_widget_grab_focus(ud->rename_entry);
		gtk_label_set_text(GTK_LABEL(ud->rename_label), fd->name);
		g_signal_handlers_block_by_func(ud->rename_entry, (gpointer)(file_util_rename_preview_entry_cb), ud);
		gtk_entry_set_text(GTK_ENTRY(ud->rename_entry), name);
		gtk_editable_select_region(GTK_EDITABLE(ud->rename_entry), 0, filename_base_length(name));
		g_signal_handlers_unblock_by_func(ud->rename_entry, (gpointer)file_util_rename_preview_entry_cb, ud);
		}

	return TRUE;
}



static void box_append_safe_delete_status(GenericDialog *gd)
{
	GtkWidget *label;
	gchar *buf;

	buf = file_util_safe_delete_status();
	label = pref_label_new(gd->vbox, buf);
	g_free(buf);

	gtk_label_set_xalign(GTK_LABEL(label), 1.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);
	gtk_widget_set_sensitive(label, FALSE);
}

static void file_util_details_cb(GenericDialog *UNUSED(gd), gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	if (ud->details_func && ud->sel_fd)
		{
		ud->details_func(ud, ud->sel_fd);
		}
}

static void file_util_dialog_init_simple_list(UtilityData *ud)
{
	GtkWidget *box;
	GtkTreeSelection *selection;
	gchar *dir_msg;

	const gchar *stock_id;

	/** @FIXME use ud->stock_id */
	if (ud->type == UTILITY_TYPE_DELETE ||
	    ud->type == UTILITY_TYPE_DELETE_LINK ||
	    ud->type == UTILITY_TYPE_DELETE_FOLDER)
		{
		stock_id = GTK_STOCK_DELETE;
		}
	else
		{
		stock_id = GTK_STOCK_OK;
		}

	ud->gd = file_util_gen_dlg(ud->messages.title, "dlg_confirm",
				   ud->parent, FALSE,  file_util_cancel_cb, ud);
	if (ud->discard_func) generic_dialog_add_button(ud->gd, GTK_STOCK_REVERT_TO_SAVED, _("Discard changes"), file_util_discard_cb, FALSE);
	if (ud->details_func) generic_dialog_add_button(ud->gd, GTK_STOCK_INFO, _("File details"), file_util_details_cb, FALSE);

	generic_dialog_add_button(ud->gd, stock_id, NULL, file_util_ok_cb, TRUE);

	if (ud->dir_fd)
		{
		dir_msg = g_strdup_printf("%s\n\n%s\n", ud->messages.desc_source_fd, ud->dir_fd->path);
		}
	else
		{
		dir_msg = g_strdup("");
		}

	box = generic_dialog_add_message(ud->gd, GTK_STOCK_DIALOG_QUESTION,
					 ud->messages.question,
					 dir_msg, TRUE);

	g_free(dir_msg);

	box = pref_group_new(box, TRUE, ud->messages.desc_flist, GTK_ORIENTATION_HORIZONTAL);

	ud->listview = file_util_dialog_add_list(box, ud->flist, FALSE, ud->with_sidecars);
	if (ud->with_sidecars) file_util_dialog_add_list_column(ud->listview, _("Sidecars"), FALSE, UTILITY_COLUMN_SIDECARS);

	if (ud->type == UTILITY_TYPE_WRITE_METADATA) file_util_dialog_add_list_column(ud->listview, _("Write to file"), FALSE, UTILITY_COLUMN_DEST_NAME);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ud->listview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	gtk_tree_selection_set_select_function(selection, file_util_preview_cb, ud, NULL);

	generic_dialog_add_image(ud->gd, box, NULL, NULL, FALSE, NULL, NULL, FALSE);

	if (ud->type == UTILITY_TYPE_DELETE ||
	    ud->type == UTILITY_TYPE_DELETE_LINK ||
	    ud->type == UTILITY_TYPE_DELETE_FOLDER)
		box_append_safe_delete_status(ud->gd);

	gtk_widget_show(ud->gd->dialog);

	file_util_dialog_list_select(ud->listview, 0);
}

static void file_util_dialog_init_dest_folder(UtilityData *ud)
{
	FileDialog *fdlg;
	GtkWidget *label;
	const gchar *stock_id;

	if (ud->type == UTILITY_TYPE_COPY)
		{
		stock_id = GTK_STOCK_COPY;
		}
	else
		{
		stock_id = GTK_STOCK_OK;
		}

	fdlg = file_util_file_dlg(ud->messages.title, "dlg_dest_folder", ud->parent,
				  file_util_fdlg_cancel_cb, ud);

	ud->fdlg = fdlg;

	generic_dialog_add_message(GENERIC_DIALOG(fdlg), NULL, ud->messages.question, NULL, FALSE);

	label = pref_label_new(GENERIC_DIALOG(fdlg)->vbox, _("Choose the destination folder."));
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	gtk_label_set_yalign(GTK_LABEL(label), 0.5);

	pref_spacer(GENERIC_DIALOG(fdlg)->vbox, 0);

	if (options->with_rename)
		{
		file_dialog_add_button(fdlg, stock_id, ud->messages.title, file_util_fdlg_ok_cb, TRUE);
		file_dialog_add_button(fdlg, GTK_STOCK_EDIT, "With Rename", file_util_fdlg_rename_cb, TRUE);
		}
	else
		{
		file_dialog_add_button(fdlg, GTK_STOCK_EDIT, "With Rename", file_util_fdlg_rename_cb, TRUE);
		file_dialog_add_button(fdlg, stock_id, ud->messages.title, file_util_fdlg_ok_cb, TRUE);
		}

	file_dialog_add_path_widgets(fdlg, NULL, ud->dest_path, "move_copy", NULL, NULL);

	g_signal_connect(G_OBJECT(fdlg->entry), "changed",
			 G_CALLBACK(file_util_dest_folder_entry_cb), ud);

	gtk_widget_show(GENERIC_DIALOG(fdlg)->dialog);
}


static GtkWidget *furm_simple_vlabel(GtkWidget *box, const gchar *text, gboolean expand)
{
	GtkWidget *vbox;
	GtkWidget *label;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(box), vbox, expand, expand, 0);
	gtk_widget_show(vbox);

	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	return vbox;
}


static void file_util_dialog_init_source_dest(UtilityData *ud, gboolean second_image)
{
	GtkTreeModel *store;
	GtkTreeSelection *selection;
	GtkWidget *box;
	GtkWidget *hbox;
	GtkWidget *box2;
	GtkWidget *table;
	GtkWidget *combo;
	GtkWidget *page;
	gchar *destination_message;

	ud->gd = file_util_gen_dlg(ud->messages.title, "dlg_confirm",
				   ud->parent, FALSE,  file_util_cancel_cb, ud);

	box = generic_dialog_add_message(ud->gd, NULL, ud->messages.question, NULL, TRUE);

	if (ud->discard_func) generic_dialog_add_button(ud->gd, GTK_STOCK_REVERT_TO_SAVED, _("Discard changes"), file_util_discard_cb, FALSE);
	if (ud->details_func) generic_dialog_add_button(ud->gd, GTK_STOCK_INFO, _("File details"), file_util_details_cb, FALSE);

	generic_dialog_add_button(ud->gd, GTK_STOCK_OK, ud->messages.title, file_util_ok_cb, TRUE);

	if (ud->type == UTILITY_TYPE_COPY || ud->type == UTILITY_TYPE_MOVE)
		{
		destination_message = g_strconcat(ud->messages.desc_flist," to: ", ud->dest_path, NULL);
		}
	else
		{
		destination_message = g_strdup(ud->messages.desc_flist);
		}

	box = pref_group_new(box, TRUE, destination_message, GTK_ORIENTATION_HORIZONTAL);
	g_free(destination_message);

	ud->listview = file_util_dialog_add_list(box, ud->flist, FALSE, ud->with_sidecars);
	file_util_dialog_add_list_column(ud->listview, _("Sidecars"), FALSE, UTILITY_COLUMN_SIDECARS);

	file_util_dialog_add_list_column(ud->listview, _("New name"), FALSE, UTILITY_COLUMN_DEST_NAME);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ud->listview));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_SINGLE);
	gtk_tree_selection_set_select_function(selection, file_util_preview_cb, ud, NULL);


//	column = file_util_rename_multiple_add_column(rd, _("Preview"), RENAME_COLUMN_PREVIEW);
//	gtk_tree_view_column_set_visible(column, FALSE);

	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(ud->listview), TRUE);

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ud->listview));
	g_signal_connect(G_OBJECT(store), "row_changed",
			 G_CALLBACK(file_util_rename_preview_order_cb), ud);
	gtk_widget_set_size_request(ud->listview, 300, 150);

	if (second_image)
		{
		generic_dialog_add_image(ud->gd, box, NULL, "Source", TRUE, NULL, "Destination", TRUE);
		}
	else
		{
		generic_dialog_add_image(ud->gd, box, NULL, NULL, FALSE, NULL, NULL, FALSE);
		}

//	gtk_container_add(GTK_CONTAINER(scrolled), view);
	gtk_widget_show(ud->gd->dialog);


	ud->notebook = gtk_notebook_new();

	gtk_box_pack_start(GTK_BOX(ud->gd->vbox), ud->notebook, FALSE, FALSE, 0);
	gtk_widget_show(ud->notebook);


	page = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);
	gtk_notebook_append_page(GTK_NOTEBOOK(ud->notebook), page, gtk_label_new(_("Manual rename")));
	gtk_widget_show(page);

	table = pref_table_new(page, 2, 2, FALSE, FALSE);

	pref_table_label(table, 0, 0, _("Original name:"), 1.0);
	ud->rename_label = pref_table_label(table, 1, 0, "", 0.0);

	pref_table_label(table, 0, 1, _("New name:"), 1.0);

	ud->rename_entry = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), ud->rename_entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	generic_dialog_attach_default(GENERIC_DIALOG(ud->gd), ud->rename_entry);
	gtk_widget_grab_focus(ud->rename_entry);

	g_signal_connect(G_OBJECT(ud->rename_entry), "changed",
			 G_CALLBACK(file_util_rename_preview_entry_cb), ud);

	gtk_widget_show(ud->rename_entry);

	page = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);
	gtk_notebook_append_page(GTK_NOTEBOOK(ud->notebook), page, gtk_label_new(_("Auto rename")));
	gtk_widget_show(page);


	hbox = pref_box_new(page, FALSE, GTK_ORIENTATION_HORIZONTAL, PREF_PAD_GAP);

	box2 = furm_simple_vlabel(hbox, _("Begin text"), TRUE);

	combo = history_combo_new(&ud->auto_entry_front, "", "numerical_rename_prefix", -1);
	g_signal_connect(G_OBJECT(ud->auto_entry_front), "changed",
			 G_CALLBACK(file_util_rename_preview_entry_cb), ud);
	gtk_box_pack_start(GTK_BOX(box2), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);

	box2 = furm_simple_vlabel(hbox, _("Start #"), FALSE);

	ud->auto_spin_start = pref_spin_new(box2, NULL, NULL,
					    0.0, 1000000.0, 1.0, 0, options->cp_mv_rn.auto_start,
					    G_CALLBACK(file_util_rename_preview_adj_cb), ud);

	box2 = furm_simple_vlabel(hbox, _("End text"), TRUE);

	combo = history_combo_new(&ud->auto_entry_end, options->cp_mv_rn.auto_end, "numerical_rename_suffix", -1);
	g_signal_connect(G_OBJECT(ud->auto_entry_end), "changed",
			 G_CALLBACK(file_util_rename_preview_entry_cb), ud);
	gtk_box_pack_start(GTK_BOX(box2), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);

	ud->auto_spin_pad = pref_spin_new(page, _("Padding:"), NULL,
					  1.0, 8.0, 1.0, 0, options->cp_mv_rn.auto_padding,
					  G_CALLBACK(file_util_rename_preview_adj_cb), ud);

	page = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);
	gtk_notebook_append_page(GTK_NOTEBOOK(ud->notebook), page, gtk_label_new(_("Formatted rename")));
	gtk_widget_show(page);

	hbox = pref_box_new(page, FALSE, GTK_ORIENTATION_HORIZONTAL, PREF_PAD_GAP);

	box2 = furm_simple_vlabel(hbox, _("Format (* = original name, ## = numbers)"), TRUE);

	combo = history_combo_new(&ud->format_entry, "", "auto_rename_format", -1);
	g_signal_connect(G_OBJECT(ud->format_entry), "changed",
			 G_CALLBACK(file_util_rename_preview_entry_cb), ud);
	gtk_box_pack_start(GTK_BOX(box2), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);

	box2 = furm_simple_vlabel(hbox, _("Start #"), FALSE);

	ud->format_spin = pref_spin_new(box2, NULL, NULL,
					0.0, 1000000.0, 1.0, 0, options->cp_mv_rn.formatted_start,
					G_CALLBACK(file_util_rename_preview_adj_cb), ud);

//	gtk_combo_box_set_active(GTK_COMBO_BOX(ud->combo_type), 0); /* callback will take care of the rest */

	file_util_dialog_list_select(ud->listview, 0);
}

static void file_util_finalize_all(UtilityData *ud)
{
	GList *work = ud->flist;

	if (ud->phase == UTILITY_PHASE_CANCEL) return;
	if (ud->phase == UTILITY_PHASE_DONE && !ud->finalize_func) return;
	if (ud->phase == UTILITY_PHASE_DISCARD && !ud->discard_func) return;

	while (work)
		{
		FileData *fd = (FileData *)work->data;
		work = work->next;
		if (ud->phase == UTILITY_PHASE_DONE) ud->finalize_func(fd);
		else if (ud->phase == UTILITY_PHASE_DISCARD) ud->discard_func(fd);
		}
}

static gboolean file_util_exclude_fd(UtilityData *ud, FileData *fd)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	gboolean valid;

	if (!g_list_find(ud->flist, fd)) return FALSE;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ud->listview));
	valid = gtk_tree_model_get_iter_first(store, &iter);
	while (valid)
		{
		FileData *store_fd;
		gtk_tree_model_get(store, &iter, UTILITY_COLUMN_FD, &store_fd, -1);

		if (store_fd == fd)
			{
			gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			break;
			}
		valid = gtk_tree_model_iter_next(store, &iter);
		}

	ud->flist = g_list_remove(ud->flist, fd);

	if (ud->with_sidecars)
		file_data_sc_free_ci(fd);
	else
		file_data_free_ci(fd);

	file_data_unref(fd);
	return TRUE;
}

void file_util_dialog_run(UtilityData *ud)
{
	switch (ud->phase)
		{
		case UTILITY_PHASE_START:
			/* create the dialogs */
			switch (ud->type)
				{
				case UTILITY_TYPE_DELETE:
				case UTILITY_TYPE_DELETE_LINK:
				case UTILITY_TYPE_DELETE_FOLDER:
				case UTILITY_TYPE_EDITOR:
				case UTILITY_TYPE_WRITE_METADATA:
					file_util_dialog_init_simple_list(ud);
					ud->phase = UTILITY_PHASE_ENTERING;
					break;
				case UTILITY_TYPE_RENAME:
					file_util_dialog_init_source_dest(ud, TRUE);
					ud->phase = UTILITY_PHASE_ENTERING;
					break;
				case UTILITY_TYPE_COPY:
				case UTILITY_TYPE_MOVE:
					file_util_dialog_init_dest_folder(ud);
					ud->phase = UTILITY_PHASE_INTERMEDIATE;
					break;
				case UTILITY_TYPE_FILTER:
				case UTILITY_TYPE_CREATE_FOLDER:
					file_util_dialog_init_dest_folder(ud);
					ud->phase = UTILITY_PHASE_ENTERING;
					break;
				case UTILITY_TYPE_RENAME_FOLDER:
					ud->phase = UTILITY_PHASE_CANCEL; /**< @FIXME not handled for now */
					file_util_dialog_run(ud);
					return;
				}
			break;
		case UTILITY_PHASE_INTERMEDIATE:
			switch (ud->type)
				{
				case UTILITY_TYPE_COPY:
				case UTILITY_TYPE_MOVE:
					file_util_dialog_init_source_dest(ud, TRUE);
					break;
				default:;
				}
			ud->phase = UTILITY_PHASE_ENTERING;
			break;
		case UTILITY_PHASE_ENTERING:
			file_util_check_ci(ud);
			break;
		case UTILITY_PHASE_CHECKED:
			file_util_perform_ci(ud);
			break;
		case UTILITY_PHASE_CANCEL:
		case UTILITY_PHASE_DONE:
		case UTILITY_PHASE_DISCARD:

			file_util_finalize_all(ud);

			/* both DISCARD and DONE finishes the operation for good */
			if (ud->done_func)
				ud->done_func((ud->phase != UTILITY_PHASE_CANCEL), ud->dest_path, ud->done_data);

			if (ud->with_sidecars)
				file_data_sc_free_ci_list(ud->flist);
			else
				file_data_free_ci_list(ud->flist);

			/* directory content is always handled including sidecars */
			file_data_sc_free_ci_list(ud->content_list);

			if (ud->dir_fd) file_data_free_ci(ud->dir_fd);
			file_util_data_free(ud);
			break;
		}
}




static void file_util_warn_op_in_progress(const gchar *title)
{
	file_util_warning_dialog(title, _("Another operation in progress.\n"), GTK_STOCK_DIALOG_ERROR, NULL);
}

static void file_util_details_dialog_close_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	gtk_widget_destroy(data);

}

static void file_util_details_dialog_destroy_cb(GtkWidget *widget, gpointer data)
{
	UtilityData *ud = (UtilityData*)data;
	g_signal_handlers_disconnect_by_func(ud->gd->dialog, (gpointer)(file_util_details_dialog_close_cb), widget);
}


static void file_util_details_dialog_ok_cb(GenericDialog *UNUSED(gd), gpointer UNUSED(data))
{
	/* no op */
}

static void file_util_details_dialog_exclude(GenericDialog *gd, gpointer data, gboolean discard)
{
	UtilityData *ud = (UtilityData*)data;
	FileData *fd = (FileData *)g_object_get_data(G_OBJECT(gd->dialog), "file_data");

	if (!fd) return;
	file_util_exclude_fd(ud, fd);

	if (discard && ud->discard_func) ud->discard_func(fd);

	/* all files were excluded, this has the same effect as pressing the cancel button in the confirmation dialog*/
	if (!ud->flist)
		{
		/* both dialogs will be closed anyway, the signals would cause duplicate calls */
		g_signal_handlers_disconnect_by_func(ud->gd->dialog, (gpointer)(file_util_details_dialog_close_cb), gd->dialog);
		g_signal_handlers_disconnect_by_func(gd->dialog, (gpointer)(file_util_details_dialog_destroy_cb), ud);

		file_util_cancel_cb(ud->gd, ud);
		}
}

static void file_util_details_dialog_exclude_cb(GenericDialog *gd, gpointer data)
{
	file_util_details_dialog_exclude(gd, data, FALSE);
}

static void file_util_details_dialog_discard_cb(GenericDialog *gd, gpointer data)
{
	file_util_details_dialog_exclude(gd, data, TRUE);
}

static gchar *file_util_details_get_message(UtilityData *ud, FileData *fd, const gchar **stock_id)
{
	GString *message = g_string_new("");
	gint error;
	g_string_append_printf(message, _("File: '%s'\n"), fd->path);

	if (ud->with_sidecars && fd->sidecar_files)
		{
		GList *work = fd->sidecar_files;
		g_string_append(message, _("with sidecar files:\n"));

		while (work)
			{
			FileData *sfd = (FileData *)work->data;
			work =work->next;
			g_string_append_printf(message, _(" '%s'\n"), sfd->path);
			}
		}

	g_string_append(message, _("\nStatus: "));

	error = ud->with_sidecars ? file_data_sc_verify_ci(fd, ud->flist) : file_data_verify_ci(fd, ud->flist);

	if (error)
		{
		gchar *err_msg = (gchar*)file_data_get_error_string(error);
		g_string_append(message, err_msg);
		if (stock_id) *stock_id = (error & CHANGE_ERROR_MASK) ? GTK_STOCK_DIALOG_ERROR : GTK_STOCK_DIALOG_WARNING;
		}
	else
		{
		g_string_append(message, _("no problem detected"));
		if (stock_id) *stock_id = GTK_STOCK_DIALOG_INFO;
		}

	return g_string_free(message, FALSE);;
}

static void file_util_details_dialog(UtilityData *ud, FileData *fd)
{
	GenericDialog *gd;
	GtkWidget *box;
	gchar *message;
	const gchar *stock_id;

	gd = file_util_gen_dlg(_("File details"), "details", ud->gd->dialog, TRUE, NULL, ud);
	generic_dialog_add_button(gd, GTK_STOCK_CLOSE, NULL, file_util_details_dialog_ok_cb, TRUE);
	generic_dialog_add_button(gd, GTK_STOCK_REMOVE, _("Exclude file"), file_util_details_dialog_exclude_cb, FALSE);

	g_object_set_data(G_OBJECT(gd->dialog), "file_data", fd);

	g_signal_connect(G_OBJECT(gd->dialog), "destroy",
			 G_CALLBACK(file_util_details_dialog_destroy_cb), ud);

	/* in case the ud->gd->dialog is closed during editing */
	g_signal_connect(G_OBJECT(ud->gd->dialog), "destroy",
			 G_CALLBACK(file_util_details_dialog_close_cb), gd->dialog);


	message = file_util_details_get_message(ud, fd, &stock_id);

	box = generic_dialog_add_message(gd, stock_id, _("File details"), message, TRUE);

	generic_dialog_add_image(gd, box, fd, NULL, FALSE, NULL, NULL, FALSE);

	gtk_widget_show(gd->dialog);

	g_free(message);
}

static void file_util_write_metadata_details_dialog(UtilityData *ud, FileData *fd)
{
	GenericDialog *gd;
	GtkWidget *box;
	GtkWidget *table;
	GtkWidget *frame;
	GtkWidget *label;
	GList *keys = NULL;
	GList *work;
	gchar *message1;
	gchar *message2;
	gint i;
	const gchar *stock_id;

	if (fd && fd->modified_xmp)
		{
		keys = g_hash_table_get_keys(fd->modified_xmp);
		}

	g_assert(keys);


	gd = file_util_gen_dlg(_("Overview of changed metadata"), "details", ud->gd->dialog, TRUE, NULL, ud);
	generic_dialog_add_button(gd, GTK_STOCK_CLOSE, NULL, file_util_details_dialog_ok_cb, TRUE);
	generic_dialog_add_button(gd, GTK_STOCK_REMOVE, _("Exclude file"), file_util_details_dialog_exclude_cb, FALSE);
	generic_dialog_add_button(gd, GTK_STOCK_REVERT_TO_SAVED, _("Discard changes"), file_util_details_dialog_discard_cb, FALSE);

	g_object_set_data(G_OBJECT(gd->dialog), "file_data", fd);

	g_signal_connect(G_OBJECT(gd->dialog), "destroy",
			 G_CALLBACK(file_util_details_dialog_destroy_cb), ud);

	/* in case the ud->gd->dialog is closed during editing */
	g_signal_connect(G_OBJECT(ud->gd->dialog), "destroy",
			 G_CALLBACK(file_util_details_dialog_close_cb), gd->dialog);

	message1 = file_util_details_get_message(ud, fd, &stock_id);

	if (fd->change && fd->change->dest)
		{
		message2 = g_strdup_printf(_("The following metadata tags will be written to\n'%s'."), fd->change->dest);
		}
	else
		{
		message2 = g_strdup_printf(_("The following metadata tags will be written to the image file itself."));
		}

	box = generic_dialog_add_message(gd, stock_id, _("Overview of changed metadata"), message1, TRUE);

	box = pref_group_new(box, TRUE, message2, GTK_ORIENTATION_HORIZONTAL);

	frame = pref_frame_new(box, TRUE, NULL, GTK_ORIENTATION_HORIZONTAL, 2);
	table = pref_table_new(frame, 2, g_list_length(keys), FALSE, TRUE);

	work = keys;
	i = 0;
	while (work)
		{
		const gchar *key = (const gchar *)work->data;
		gchar *title = exif_get_description_by_key(key);
		gchar *title_f = g_strdup_printf("%s:", title);
		gchar *value = metadata_read_string(fd, key, METADATA_FORMATTED);
		work = work->next;


		label = gtk_label_new(title_f);
		gtk_label_set_xalign(GTK_LABEL(label), 1.0);
		gtk_label_set_yalign(GTK_LABEL(label), 0.0);

		pref_label_bold(label, TRUE, FALSE);
		gtk_table_attach(GTK_TABLE(table), label,
				 0, 1, i, i + 1,
				 GTK_FILL, GTK_FILL,
				 2, 2);
		gtk_widget_show(label);

		label = gtk_label_new(value);

		gtk_label_set_xalign(GTK_LABEL(label), 0.0);
		gtk_label_set_yalign(GTK_LABEL(label), 0.0);

		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_table_attach(GTK_TABLE(table), label,
				 1, 2, i, i + 1,
				 GTK_FILL, GTK_FILL,
				 2, 2);
		gtk_widget_show(label);

		g_free(title);
		g_free(title_f);
		g_free(value);
		i++;
		}

	generic_dialog_add_image(gd, box, fd, NULL, FALSE, NULL, NULL, FALSE);

	gtk_widget_set_size_request(gd->dialog, DIALOG_WIDTH, -1);
	gtk_widget_show(gd->dialog);

	g_list_free(keys);
	g_free(message1);
	g_free(message2);
}


static void file_util_mark_ungrouped_files(GList *work)
{
	while (work)
		{
		FileData *fd = (FileData *)work->data;
		file_data_set_regroup_when_finished(fd, TRUE);
		work = work->next;
		}
}

static void file_util_delete_full(FileData *source_fd, GList *flist, GtkWidget *parent, UtilityPhase phase, FileUtilDoneFunc done_func, gpointer done_data)
{
	UtilityData *ud;
	GList *ungrouped = NULL;
	gchar *message;

	if (source_fd)
		flist = g_list_append(flist, file_data_ref(source_fd));

	if (!flist) return;

	flist = file_data_process_groups_in_selection(flist, TRUE, &ungrouped);

	if (!file_data_sc_add_ci_delete_list(flist))
		{
		file_util_warn_op_in_progress(_("File deletion failed"));
		file_data_disable_grouping_list(ungrouped, FALSE);
		filelist_free(flist);
		filelist_free(ungrouped);
		return;
		}

	file_util_mark_ungrouped_files(ungrouped);
	filelist_free(ungrouped);

	ud = file_util_data_new(UTILITY_TYPE_DELETE);

	ud->phase = phase;

	ud->with_sidecars = TRUE;

	ud->dir_fd = NULL;
	ud->flist = flist;
	ud->content_list = NULL;
	ud->parent = parent;
	ud->done_data = done_data;
	ud->done_func = done_func;

	ud->details_func = file_util_details_dialog;
	if(options->file_ops.safe_delete_enable)
		{
		message = _("This will move the following files to the Trash bin");
		}
	else
		{
		message = _("This will permanently delete the following files");
		}
	ud->messages.title = _("Delete");
	ud->messages.question = _("Delete files?");
	ud->messages.desc_flist = message;
	ud->messages.desc_source_fd = "";
	ud->messages.fail = _("File deletion failed");

	file_util_dialog_run(ud);
}


static void file_util_write_metadata_full(FileData *source_fd, GList *flist, GtkWidget *parent, UtilityPhase phase, FileUtilDoneFunc done_func, gpointer done_data)
{
	UtilityData *ud;

	if (source_fd)
		flist = g_list_append(flist, file_data_ref(source_fd));

	if (!flist) return;

	if (!file_data_add_ci_write_metadata_list(flist))
		{
		file_util_warn_op_in_progress(_("Can't write metadata"));
		filelist_free(flist);
		return;
		}

	ud = file_util_data_new(UTILITY_TYPE_WRITE_METADATA);

	ud->phase = phase;

	ud->with_sidecars = FALSE; /* operate on individual files, not groups */

	ud->dir_fd = NULL;
	ud->flist = flist;
	ud->content_list = NULL;
	ud->parent = parent;

	ud->done_func = done_func;
	ud->done_data = done_data;

	ud->details_func = file_util_write_metadata_details_dialog;
	ud->finalize_func = metadata_write_queue_remove;
	ud->discard_func = metadata_write_queue_remove;

	ud->messages.title = _("Write metadata");
	ud->messages.question = _("Write metadata?");
	ud->messages.desc_flist = _("This will write the changed metadata into the following files");
	ud->messages.desc_source_fd = "";
	ud->messages.fail = _("Metadata writing failed");

	file_util_dialog_run(ud);
}

static void file_util_move_full(FileData *source_fd, GList *flist, const gchar *dest_path, GtkWidget *parent, UtilityPhase phase)
{
	UtilityData *ud;
	GList *ungrouped = NULL;

	if (source_fd)
		flist = g_list_append(flist, file_data_ref(source_fd));

	if (!flist) return;

	flist = file_data_process_groups_in_selection(flist, TRUE, &ungrouped);

	if (!file_data_sc_add_ci_move_list(flist, dest_path))
		{
		file_util_warn_op_in_progress(_("Move failed"));
		file_data_disable_grouping_list(ungrouped, FALSE);
		filelist_free(flist);
		filelist_free(ungrouped);
		return;
		}

	file_util_mark_ungrouped_files(ungrouped);
	filelist_free(ungrouped);

	ud = file_util_data_new(UTILITY_TYPE_MOVE);

	ud->phase = phase;

	ud->with_sidecars = TRUE;

	ud->dir_fd = NULL;
	ud->flist = flist;
	ud->content_list = NULL;
	ud->parent = parent;
	ud->details_func = file_util_details_dialog;

	if (dest_path) ud->dest_path = g_strdup(dest_path);

	ud->messages.title = _("Move");
	ud->messages.question = _("Move files?");
	ud->messages.desc_flist = _("This will move the following files");
	ud->messages.desc_source_fd = "";
	ud->messages.fail = _("Move failed");

	file_util_dialog_run(ud);
}

static void file_util_copy_full(FileData *source_fd, GList *flist, const gchar *dest_path, GtkWidget *parent, UtilityPhase phase)
{
	UtilityData *ud;
	GList *ungrouped = NULL;

	if (source_fd)
		flist = g_list_append(flist, file_data_ref(source_fd));

	if (!flist) return;

	if (file_util_write_metadata_first(UTILITY_TYPE_COPY, phase, flist, dest_path, NULL, parent))
		return;

	flist = file_data_process_groups_in_selection(flist, TRUE, &ungrouped);

	if (!file_data_sc_add_ci_copy_list(flist, dest_path))
		{
		file_util_warn_op_in_progress(_("Copy failed"));
		file_data_disable_grouping_list(ungrouped, FALSE);
		filelist_free(flist);
		filelist_free(ungrouped);
		return;
		}

	file_util_mark_ungrouped_files(ungrouped);
	filelist_free(ungrouped);

	ud = file_util_data_new(UTILITY_TYPE_COPY);

	ud->phase = phase;

	ud->with_sidecars = TRUE;

	ud->dir_fd = NULL;
	ud->flist = flist;
	ud->content_list = NULL;
	ud->parent = parent;
	ud->details_func = file_util_details_dialog;

	if (dest_path) ud->dest_path = g_strdup(dest_path);

	ud->messages.title = _("Copy");
	ud->messages.question = _("Copy files?");
	ud->messages.desc_flist = _("This will copy the following files");
	ud->messages.desc_source_fd = "";
	ud->messages.fail = _("Copy failed");

	file_util_dialog_run(ud);
}

static void file_util_rename_full(FileData *source_fd, GList *flist, const gchar *dest_path, GtkWidget *parent, UtilityPhase phase)
{
	UtilityData *ud;
	GList *ungrouped = NULL;

	if (source_fd)
		flist = g_list_append(flist, file_data_ref(source_fd));

	if (!flist) return;

	flist = file_data_process_groups_in_selection(flist, TRUE, &ungrouped);

	if (!file_data_sc_add_ci_rename_list(flist, dest_path))
		{
		file_util_warn_op_in_progress(_("Rename failed"));
		file_data_disable_grouping_list(ungrouped, FALSE);
		filelist_free(flist);
		filelist_free(ungrouped);
		return;
		}

	file_util_mark_ungrouped_files(ungrouped);
	filelist_free(ungrouped);

	ud = file_util_data_new(UTILITY_TYPE_RENAME);

	ud->phase = phase;

	ud->with_sidecars = TRUE;

	ud->dir_fd = NULL;
	ud->flist = flist;
	ud->content_list = NULL;
	ud->parent = parent;

	ud->details_func = file_util_details_dialog;

	ud->messages.title = _("Rename");
	ud->messages.question = _("Rename files?");
	ud->messages.desc_flist = _("This will rename the following files");
	ud->messages.desc_source_fd = "";
	ud->messages.fail = _("Rename failed");

	file_util_dialog_run(ud);
}

static void file_util_start_editor_full(const gchar *key, FileData *source_fd, GList *flist, const gchar *dest_path, const gchar *working_directory, GtkWidget *parent, UtilityPhase phase)
{
	UtilityData *ud;
	GList *ungrouped = NULL;

	if (editor_no_param(key))
		{
		gchar *file_directory = NULL;
		if (!working_directory)
			{
			/* working directory was not specified, try to extract it from the files */
			if (source_fd)
				file_directory = remove_level_from_path(source_fd->path);

			if (!file_directory && flist)
				file_directory = remove_level_from_path(((FileData *)flist->data)->path);
			working_directory = file_directory;
			}

		/* just start the editor, don't care about files */
		start_editor(key, working_directory);
		g_free(file_directory);
		filelist_free(flist);
		return;
		}


	if (source_fd)
		{
		/* flist is most probably NULL
		   operate on source_fd and it's sidecars
		*/
		flist = g_list_concat(filelist_copy(source_fd->sidecar_files), flist);
		flist = g_list_append(flist, file_data_ref(source_fd));
		}

	if (!flist) return;

	if (file_util_write_metadata_first(UTILITY_TYPE_FILTER, phase, flist, dest_path, key, parent))
		return;

	flist = file_data_process_groups_in_selection(flist, TRUE, &ungrouped);

	if (!file_data_sc_add_ci_unspecified_list(flist, dest_path))
		{
		file_util_warn_op_in_progress(_("Can't run external editor"));
		file_data_disable_grouping_list(ungrouped, FALSE);
		filelist_free(flist);
		filelist_free(ungrouped);
		return;
		}

	file_util_mark_ungrouped_files(ungrouped);
	filelist_free(ungrouped);

	if (editor_is_filter(key))
		ud = file_util_data_new(UTILITY_TYPE_FILTER);
	else
		ud = file_util_data_new(UTILITY_TYPE_EDITOR);


	/* ask for destination if we don't have it */
	if (ud->type == UTILITY_TYPE_FILTER && dest_path == NULL) phase = UTILITY_PHASE_START;

	ud->phase = phase;

	ud->with_sidecars = TRUE;

	ud->external_command = g_strdup(key);

	ud->dir_fd = NULL;
	ud->flist = flist;
	ud->content_list = NULL;
	ud->parent = parent;

	ud->details_func = file_util_details_dialog;

	if (dest_path) ud->dest_path = g_strdup(dest_path);

	ud->messages.title = _("Editor");
	ud->messages.question = _("Run editor?");
	ud->messages.desc_flist = _("This will copy the following files");
	ud->messages.desc_source_fd = "";
	ud->messages.fail = _("External command failed");

	file_util_dialog_run(ud);
}

static GList *file_util_delete_dir_remaining_folders(GList *dlist)
{
	GList *rlist = NULL;

	while (dlist)
		{
		FileData *fd;

		fd = (FileData *)dlist->data;
		dlist = dlist->next;

		if (!fd->name ||
		    (strcmp(fd->name, THUMB_FOLDER_GLOBAL) != 0 &&
		     strcmp(fd->name, THUMB_FOLDER_LOCAL) != 0 &&
		     strcmp(fd->name, GQ_CACHE_LOCAL_METADATA) != 0) )
			{
			rlist = g_list_prepend(rlist, fd);
			}
		}

	return g_list_reverse(rlist);
}

static gboolean file_util_delete_dir_empty_path(UtilityData *ud, FileData *fd, gint level)
{
	GList *dlist;
	GList *flist;
	GList *work;

	gboolean ok = TRUE;

	DEBUG_1("deltree into: %s", fd->path);

	level++;
	if (level > UTILITY_DELETE_MAX_DEPTH)
		{
		log_printf("folder recursion depth past %d, giving up\n", UTILITY_DELETE_MAX_DEPTH);
		// ud->fail_fd = fd
		return 0;
		}

	if (!filelist_read_lstat(fd, &flist, &dlist))
		{
		// ud->fail_fd = fd
		return 0;
		}

	if (ok)
		{
		ok = file_data_sc_add_ci_delete(fd);
		if (ok)
			{
			ud->content_list = g_list_prepend(ud->content_list, fd);
			}
		// ud->fail_fd = fd
		}

	work = dlist;
	while (work && ok)
		{
		FileData *lfd;

		lfd = (FileData *)work->data;
		work = work->next;

		ok = file_util_delete_dir_empty_path(ud, lfd, level);
		}

	work = flist;
	while (work && ok)
		{
		FileData *lfd;

		lfd = (FileData *)work->data;
		work = work->next;

		DEBUG_1("deltree child: %s", lfd->path);

		ok = file_data_sc_add_ci_delete(lfd);
		if (ok)
			{
			ud->content_list = g_list_prepend(ud->content_list, lfd);
			}
		// ud->fail_fd = fd
		}

	filelist_free(dlist);
	filelist_free(flist);


	DEBUG_1("deltree done: %s", fd->path);

	return ok;
}

static gboolean file_util_delete_dir_prepare(UtilityData *ud, GList *flist, GList *dlist)
{
	gboolean ok = TRUE;
	GList *work;


	work = dlist;
	while (work && ok)
		{
		FileData *fd;

		fd = (FileData *)work->data;
		work = work->next;

		ok = file_util_delete_dir_empty_path(ud, fd, 0);
		}

	work = flist;
	if (ok && file_data_sc_add_ci_delete_list(flist))
		{
		ud->content_list = g_list_concat(filelist_copy(flist), ud->content_list);
		}
	else
		{
		ok = FALSE;
		}

	if (ok)
		{
		ok = file_data_sc_add_ci_delete(ud->dir_fd);
		}

	if (!ok)
		{
		work = ud->content_list;
		while (work)
			{
			FileData *fd;

			fd = (FileData *)work->data;
			work = work->next;
			file_data_sc_free_ci(fd);
			}
		}

	return ok;
}

static void file_util_delete_dir_full(FileData *fd, GtkWidget *parent, UtilityPhase phase)
{
	GList *dlist;
	GList *flist;
	GList *rlist;

	if (!isdir(fd->path)) return;

	if (islink(fd->path))
		{
		UtilityData *ud;
		ud = file_util_data_new(UTILITY_TYPE_DELETE_LINK);

		ud->phase = phase;
		ud->with_sidecars = TRUE;
		ud->dir_fd = file_data_ref(fd);
		ud->content_list = NULL;
		ud->flist = NULL;

		ud->parent = parent;

		ud->messages.title = _("Delete folder");
		ud->messages.question = _("Delete symbolic link?");
		ud->messages.desc_flist = "";
		ud->messages.desc_source_fd = _("This will delete the symbolic link.\n"
						"The folder this link points to will not be deleted.");
		ud->messages.fail = _("Link deletion failed");

		file_util_dialog_run(ud);
		return;
		}

	if (!access_file(fd->path, W_OK | X_OK))
		{
		gchar *text;

		text = g_strdup_printf(_("Unable to remove folder %s\n"
					 "Permissions do not allow writing to the folder."), fd->path);
		file_util_warning_dialog(_("Delete failed"), text, GTK_STOCK_DIALOG_ERROR, parent);
		g_free(text);

		return;
		}

	if (!filelist_read_lstat(fd, &flist, &dlist))
		{
		gchar *text;

		text = g_strdup_printf(_("Unable to list contents of folder %s"), fd->path);
		file_util_warning_dialog(_("Delete failed"), text, GTK_STOCK_DIALOG_ERROR, parent);
		g_free(text);

		return;
		}

	rlist = file_util_delete_dir_remaining_folders(dlist);
	if (rlist)
		{
		GenericDialog *gd;
		GtkWidget *box;
		gchar *text;

		gd = file_util_gen_dlg(_("Folder contains subfolders"), "dlg_warning",
					parent, TRUE, NULL, NULL);
		generic_dialog_add_button(gd, GTK_STOCK_CLOSE, NULL, NULL, TRUE);

		text = g_strdup_printf(_("Unable to delete the folder:\n\n%s\n\n"
					 "This folder contains subfolders which must be moved before it can be deleted."),
					fd->path);
		box = generic_dialog_add_message(gd, GTK_STOCK_DIALOG_WARNING,
						 _("Folder contains subfolders"),
						 text, TRUE);
		g_free(text);

		box = pref_group_new(box, TRUE, _("Subfolders:"), GTK_ORIENTATION_VERTICAL);

		rlist = filelist_sort_path(rlist);
		file_util_dialog_add_list(box, rlist, FALSE, FALSE);

		gtk_widget_show(gd->dialog);
		}
	else
		{
		UtilityData *ud;
		ud = file_util_data_new(UTILITY_TYPE_DELETE_FOLDER);

		ud->phase = phase;
		ud->with_sidecars = TRUE;
		ud->dir_fd = file_data_ref(fd);
		ud->content_list = NULL; /* will be filled by file_util_delete_dir_prepare */
		ud->flist = flist = filelist_sort_path(flist);

		ud->parent = parent;

		ud->messages.title = _("Delete folder");
		ud->messages.question = _("Delete folder?");
		ud->messages.desc_flist = _("The folder contains these files:");
		ud->messages.desc_source_fd = _("This will delete the folder.\n"
						"The contents of this folder will also be deleted.");
		ud->messages.fail = _("File deletion failed");

		if (!file_util_delete_dir_prepare(ud, flist, dlist))
			{
			gchar *text;

			text = g_strdup_printf(_("Unable to list contents of folder %s"), fd->path);
			file_util_warning_dialog(_("Delete failed"), text, GTK_STOCK_DIALOG_ERROR, parent);
			g_free(text);
			file_data_unref(ud->dir_fd);
			file_util_data_free(ud);
			}
		else
			{
			filelist_free(dlist);
			file_util_dialog_run(ud);
			return;
			}
		}

	g_list_free(rlist);
	filelist_free(dlist);
	filelist_free(flist);
}

static gboolean file_util_rename_dir_scan(UtilityData *ud, FileData *fd)
{
	GList *dlist;
	GList *flist;
	GList *work;

	gboolean ok = TRUE;

	if (!filelist_read_lstat(fd, &flist, &dlist))
		{
		// ud->fail_fd = fd
		return 0;
		}

	ud->content_list = g_list_concat(flist, ud->content_list);

	work = dlist;
	while (work && ok)
		{
		FileData *lfd;

		lfd = (FileData *)work->data;
		work = work->next;

		ud->content_list = g_list_prepend(ud->content_list, file_data_ref(lfd));
		ok = file_util_rename_dir_scan(ud, lfd);
		}

	filelist_free(dlist);

	return ok;
}

static gboolean file_util_rename_dir_prepare(UtilityData *ud, const gchar *new_path)
{
	gboolean ok;
	GList *work;
	gint orig_len = strlen(ud->dir_fd->path);

	ok = file_util_rename_dir_scan(ud, ud->dir_fd);

	work = ud->content_list;

	while (ok && work)
		{
		gchar *np;
		FileData *fd;

		fd = (FileData *)work->data;
		work = work->next;

		g_assert(strncmp(fd->path, ud->dir_fd->path, orig_len) == 0);

		np = g_strconcat(new_path, fd->path + orig_len, NULL);

		ok = file_data_sc_add_ci_rename(fd, np);

		DEBUG_1("Dir rename: %s -> %s", fd->path, np);
		g_free(np);
		}

	if (ok)
		{
		ok = file_data_sc_add_ci_rename(ud->dir_fd, new_path);
		}

	if (!ok)
		{
		work = ud->content_list;
		while (work)
			{
			FileData *fd;

			fd = (FileData *)work->data;
			work = work->next;
			file_data_sc_free_ci(fd);
			}
		}

	return ok;
}


static void file_util_rename_dir_full(FileData *fd, const gchar *new_path, GtkWidget *parent, UtilityPhase phase, FileUtilDoneFunc done_func, gpointer done_data)
{
	UtilityData *ud;

	ud = file_util_data_new(UTILITY_TYPE_RENAME_FOLDER);

	ud->phase = phase;
	ud->with_sidecars = TRUE; /* does not matter, the directory should not have sidecars
	                            and the content must be handled including sidecars */

	ud->dir_fd = file_data_ref(fd);
	ud->flist = NULL;
	ud->content_list = NULL;
	ud->parent = parent;

	ud->done_func = done_func;
	ud->done_data = done_data;
	ud->dest_path = g_strdup(new_path);

	ud->messages.title = _("Rename");
	ud->messages.question = _("Rename folder?");
	ud->messages.desc_flist = _("The folder contains the following files");
	ud->messages.desc_source_fd = "";
	ud->messages.fail = _("Rename failed");

	if (!file_util_rename_dir_prepare(ud, new_path))
		{
		file_util_warn_op_in_progress(ud->messages.fail);
		file_util_data_free(ud);
		return;
		}

//	ud->flist = filelist_recursive(fd);

	file_util_dialog_run(ud);
}

static void file_util_create_dir_full(FileData *fd, const gchar *dest_path, GtkWidget *parent, UtilityPhase phase, FileUtilDoneFunc done_func, gpointer done_data)
{
	UtilityData *ud;

	ud = file_util_data_new(UTILITY_TYPE_CREATE_FOLDER);

	ud->phase = phase;
	ud->with_sidecars = TRUE;

	ud->dir_fd = NULL;
	ud->flist = NULL;
	ud->content_list = NULL;
	ud->parent = parent;

	if (dest_path)
		{
		g_assert_not_reached(); // not used in current design
		ud->dest_path = g_strdup(dest_path);
		}
	else
		{
		gchar *buf = new_folder(GTK_WINDOW(parent), fd->path);
		if (!buf)
			{
			ud->phase = UTILITY_PHASE_CANCEL;
			ud->dir_fd = NULL;
			}
		else
			{
			ud->dest_path = buf;
			ud->dir_fd = file_data_new_dir(ud->dest_path);
			}
		}

	ud->done_func = done_func;
	ud->done_data = done_data;

	ud->messages.title = _("Create Folder");
	ud->messages.question = _("Create folder?");
	ud->messages.desc_flist = "";
	ud->messages.desc_source_fd = "";
	ud->messages.fail = _("Can't create folder");

	file_util_dialog_run(ud);
}


static gboolean file_util_write_metadata_first_after_done(gpointer data)
{
	UtilityDelayData *dd = (UtilityDelayData*)data;

	/* start the delayed operation with original arguments */
	switch (dd->type)
		{
		case UTILITY_TYPE_FILTER:
		case UTILITY_TYPE_EDITOR:
			file_util_start_editor_full(dd->editor_key, NULL, dd->flist, dd->dest_path, NULL, dd->parent, dd->phase);
			break;
		case UTILITY_TYPE_COPY:
			file_util_copy_full(NULL, dd->flist, dd->dest_path, dd->parent, dd->phase);
			break;
		default:
			g_warning("unsupported type");
		}
	g_free(dd->dest_path);
	g_free(dd->editor_key);
	g_free(dd);
	return FALSE;
}

static void file_util_write_metadata_first_done(gboolean success, const gchar *UNUSED(done_path), gpointer data)
{
	UtilityDelayData *dd = (UtilityDelayData*)data;

	if (success)
		{
		dd->idle_id = g_idle_add(file_util_write_metadata_first_after_done, dd);
		return;
		}

	/* the operation was cancelled */
	filelist_free(dd->flist);
	g_free(dd->dest_path);
	g_free(dd->editor_key);
	g_free(dd);
}

static gboolean file_util_write_metadata_first(UtilityType type, UtilityPhase phase, GList *flist, const gchar *dest_path, const gchar *editor_key, GtkWidget *parent)
{
	GList *unsaved = NULL;
	UtilityDelayData *dd;

	GList *work;

	work = flist;
	while (work)
		{
		FileData *fd = (FileData *)work->data;
		work = work->next;

		if (fd->change)
			{
			filelist_free(unsaved);
			return FALSE; /* another op. in progress, let the caller handle it */
			}

		if (fd->modified_xmp) /* has unsaved metadata */
			{
			unsaved = g_list_prepend(unsaved, file_data_ref(fd));
			}
		}

	if (!unsaved) return FALSE;

	/* save arguments of the original operation */

	dd = g_new0(UtilityDelayData, 1);

	dd->type = type;
	dd->phase = phase;
	dd->flist = flist;
	dd->dest_path = g_strdup(dest_path);
	dd->editor_key = g_strdup(editor_key);
	dd->parent = parent;

	file_util_write_metadata(NULL, unsaved, parent, FALSE, file_util_write_metadata_first_done, dd);
	return TRUE;
}


/* full-featured entry points
*/

void file_util_delete(FileData *source_fd, GList *source_list, GtkWidget *parent)
{
	file_util_delete_full(source_fd, source_list, parent, options->file_ops.confirm_delete ? UTILITY_PHASE_START : UTILITY_PHASE_ENTERING, NULL, NULL);
}

void file_util_delete_notify_done(FileData *source_fd, GList *source_list, GtkWidget *parent, FileUtilDoneFunc done_func, gpointer done_data)
{
	file_util_delete_full(source_fd, source_list, parent, options->file_ops.confirm_delete ? UTILITY_PHASE_START : UTILITY_PHASE_ENTERING, done_func, done_data);
}

void file_util_write_metadata(FileData *source_fd, GList *source_list, GtkWidget *parent, gboolean force_dialog, FileUtilDoneFunc done_func, gpointer done_data)
{
	file_util_write_metadata_full(source_fd, source_list, parent,
	                              ((options->metadata.save_in_image_file && options->metadata.confirm_write) || force_dialog) ? UTILITY_PHASE_START : UTILITY_PHASE_ENTERING,
	                              done_func, done_data);
}

void file_util_copy(FileData *source_fd, GList *source_list, const gchar *dest_path, GtkWidget *parent)
{
	file_util_copy_full(source_fd, source_list, dest_path, parent, UTILITY_PHASE_START);
}

void file_util_move(FileData *source_fd, GList *source_list, const gchar *dest_path, GtkWidget *parent)
{
	file_util_move_full(source_fd, source_list, dest_path, parent, UTILITY_PHASE_START);
}

void file_util_rename(FileData *source_fd, GList *source_list, GtkWidget *parent)
{
	file_util_rename_full(source_fd, source_list, NULL, parent, UTILITY_PHASE_START);
}

/* these avoid the location entry dialog unless there is an error, list must be files only and
 * dest_path must be a valid directory path
 */
void file_util_move_simple(GList *list, const gchar *dest_path, GtkWidget *parent)
{
	file_util_move_full(NULL, list, dest_path, parent, UTILITY_PHASE_ENTERING);
}

void file_util_copy_simple(GList *list, const gchar *dest_path, GtkWidget *parent)
{
	file_util_copy_full(NULL, list, dest_path, parent, UTILITY_PHASE_ENTERING);
}

void file_util_rename_simple(FileData *fd, const gchar *dest_path, GtkWidget *parent)
{
	file_util_rename_full(fd, NULL, dest_path, parent, UTILITY_PHASE_ENTERING);
}


void file_util_start_editor_from_file(const gchar *key, FileData *fd, GtkWidget *parent)
{
	file_util_start_editor_full(key, fd, NULL, NULL, NULL, parent, UTILITY_PHASE_ENTERING);
}

void file_util_start_editor_from_filelist(const gchar *key, GList *list, const gchar *working_directory, GtkWidget *parent)
{
	file_util_start_editor_full(key, NULL, list, NULL, working_directory, parent, UTILITY_PHASE_ENTERING);
}

void file_util_start_filter_from_file(const gchar *key, FileData *fd, const gchar *dest_path, GtkWidget *parent)
{
	file_util_start_editor_full(key, fd, NULL, dest_path, NULL, parent, UTILITY_PHASE_ENTERING);
}

void file_util_start_filter_from_filelist(const gchar *key, GList *list, const gchar *dest_path, GtkWidget *parent)
{
	file_util_start_editor_full(key, NULL, list, dest_path, NULL, parent, UTILITY_PHASE_ENTERING);
}

void file_util_delete_dir(FileData *fd, GtkWidget *parent)
{
	file_util_delete_dir_full(fd, parent, UTILITY_PHASE_START);
}

void file_util_create_dir(FileData *dir_fd, GtkWidget *parent, FileUtilDoneFunc done_func, gpointer done_data)
{
	file_util_create_dir_full(dir_fd, NULL, parent, UTILITY_PHASE_ENTERING, done_func, done_data);
}

void file_util_rename_dir(FileData *source_fd, const gchar *new_path, GtkWidget *parent, FileUtilDoneFunc done_func, gpointer done_data)
{
	file_util_rename_dir_full(source_fd, new_path, parent, UTILITY_PHASE_ENTERING, done_func, done_data);
}


void file_util_copy_path_to_clipboard(FileData *fd, gboolean quoted)
{
	if (!fd || !*fd->path) return;

	if (options->clipboard_selection == CLIPBOARD_PRIMARY)
		{
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), quoted ? g_shell_quote(fd->path) :  fd->path , -1);
		}
	else if (options->clipboard_selection == CLIPBOARD_CLIPBOARD)
		{
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), quoted ? g_shell_quote(fd->path) :  fd->path , -1);
		}
	else
		{
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), quoted ? g_shell_quote(fd->path) :  fd->path , -1);
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), quoted ? g_shell_quote(fd->path) :  fd->path , -1);
		}
}

void file_util_copy_path_list_to_clipboard(GList *list, gboolean quoted)
{
	GList *work;
	GString *path_list_str;

	path_list_str = g_string_new("");
	work = list;
	while (work) {
		FileData *fd = (FileData *)work->data;
		work = work->next;

		if (!fd || !*fd->path) continue;

		if (quoted)
			{
			g_string_append(path_list_str, g_shell_quote(fd->path));
			}
		else
			{
			g_string_append(path_list_str, fd->path);
			}
		if (work) g_string_append_c(path_list_str, ' ');
		}

	if (options->clipboard_selection == CLIPBOARD_PRIMARY)
		{
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), path_list_str->str, path_list_str->len);
		}
	else if  (options->clipboard_selection == CLIPBOARD_CLIPBOARD)
		{
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), path_list_str->str, path_list_str->len);
		}
	else
		{
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), path_list_str->str, path_list_str->len);
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), path_list_str->str, path_list_str->len);
		}

	g_string_free(path_list_str, TRUE);
	filelist_free(list);
}

static void new_folder_entry_activate_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	GtkDialog *dialog = (GtkDialog*)data;

	gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
}

gchar *new_folder(GtkWindow *window , gchar *path)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;
	gchar *buf;
	gchar *folder_name;
	gchar *folder_path;
	GtkWidget *image;
	GtkWidget *folder_name_entry;
	GtkWidget *dialog;
	GtkWidget *dialog_warning;
	GtkWidget *content_area;
	gboolean ok_or_cancel = FALSE;
	gint result;
	gchar *new_folder_name;
	gchar *new_folder_path_full = NULL;
	gchar *window_title;
	PangoLayout *layout;
	gint width, height;

	buf = g_build_filename(path, _("New folder"), NULL);
	folder_path = unique_filename(buf, NULL, " ", FALSE);
	folder_name = g_path_get_basename(folder_path);
	window_title = g_strconcat(_("Create Folder - "), GQ_APPNAME, NULL);

	dialog = gtk_dialog_new_with_buttons(window_title,
										GTK_WINDOW(window),
										GTK_DIALOG_MODAL,
										GTK_STOCK_CANCEL,
										GTK_RESPONSE_REJECT,
										GTK_STOCK_OK,
										GTK_RESPONSE_ACCEPT,
										NULL);

	layout = gtk_widget_create_pango_layout(GTK_WIDGET(dialog), window_title);
	pango_layout_get_pixel_size(layout, &width, &height);
	gtk_window_set_default_size(GTK_WINDOW(dialog), width * 2, -1);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);
	gtk_box_pack_start(GTK_BOX(content_area), vbox, FALSE, FALSE, 0);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, PREF_PAD_GAP);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), PREF_PAD_GAP);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	image = gtk_image_new_from_icon_name("dialog-question", GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	label = gtk_label_new(_("Create new folder"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	folder_name_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(folder_name_entry), folder_name);
	gtk_box_pack_start(GTK_BOX(vbox), folder_name_entry, FALSE, FALSE, PREF_PAD_SPACE);
	g_signal_connect(G_OBJECT(folder_name_entry), "activate", G_CALLBACK(new_folder_entry_activate_cb), dialog);

	gtk_widget_show_all(dialog);

	while (ok_or_cancel == FALSE)
		{
		result = gtk_dialog_run(GTK_DIALOG(dialog));

		switch (result)
			{
			case GTK_RESPONSE_ACCEPT:
				new_folder_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(folder_name_entry)));
				new_folder_path_full = g_build_filename(path, new_folder_name, NULL);
				if (isname(new_folder_path_full))
					{
					dialog_warning = gtk_message_dialog_new(GTK_WINDOW(window),
											GTK_DIALOG_MODAL,
											GTK_MESSAGE_WARNING,
											GTK_BUTTONS_OK,
											_("Cannot create folder:"));
					gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog_warning), "%s", new_folder_name);
					gtk_window_set_title(GTK_WINDOW(dialog_warning), GQ_APPNAME);

					gtk_dialog_run(GTK_DIALOG(dialog_warning));

					gtk_widget_destroy(dialog_warning);
					g_free(new_folder_path_full);
					new_folder_path_full = NULL;
					g_free(new_folder_name);
					ok_or_cancel = FALSE;
					}
				else
					{
					ok_or_cancel = TRUE;
					}
				break;
			case GTK_RESPONSE_REJECT:
				ok_or_cancel = TRUE;
				break;
			default:
				ok_or_cancel = TRUE;
				break;
		  }
		}

	g_free(buf);
	g_free(folder_path);
	g_free(folder_name);
	g_object_unref(layout);
	g_free(window_title);
	gtk_widget_destroy(dialog);

	return new_folder_path_full;
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
