/*
 * Copyright (C) 2008 - 2016 The Geeqie Team
 *
 * Author: Laurent Monin
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
#include "view_file.h"

#include "dupe.h"
#include "collect.h"
#include "collect-table.h"
#include "editors.h"
#include "history_list.h"
#include "layout.h"
#include "menu.h"
#include "misc.h"
#include "pixbuf_util.h"
#include "thumb.h"
#include "ui_menu.h"
#include "ui_fileops.h"
#include "ui_misc.h"
#include "utilops.h"
#include "view_file/view_file_list.h"
#include "view_file/view_file_icon.h"
#include "window.h"

/*
 *-----------------------------------------------------------------------------
 * signals
 *-----------------------------------------------------------------------------
 */

void vf_send_update(ViewFile *vf)
{
	if (vf->func_status) vf->func_status(vf, vf->data_status);
}

/*
 *-----------------------------------------------------------------------------
 * misc
 *-----------------------------------------------------------------------------
 */

void vf_sort_set(ViewFile *vf, SortType type, gboolean ascend)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_sort_set(vf, type, ascend); break;
	case FILEVIEW_ICON: vficon_sort_set(vf, type, ascend); break;
	}
}

/*
 *-----------------------------------------------------------------------------
 * row stuff
 *-----------------------------------------------------------------------------
 */

FileData *vf_index_get_data(ViewFile *vf, gint row)
{
	return g_list_nth_data(vf->list, row);
}

gint vf_index_by_fd(ViewFile *vf, FileData *fd)
{
	gint ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_index_by_fd(vf, fd); break;
	case FILEVIEW_ICON: ret = vficon_index_by_fd(vf, fd); break;
	default: ret = 0;
	}

	return ret;
}

guint vf_count(ViewFile *vf, gint64 *bytes)
{
	if (bytes)
		{
		gint64 b = 0;
		GList *work;

		work = vf->list;
		while (work)
			{
			FileData *fd = work->data;
			work = work->next;

			b += fd->size;
			}

		*bytes = b;
		}

	return g_list_length(vf->list);
}

GList *vf_get_list(ViewFile *vf)
{
	GList *list = NULL;
	GList *work;
	for (work = vf->list; work; work = work->next)
		{
		FileData *fd = work->data;
		list = g_list_prepend(list, file_data_ref(fd));
		}

	return g_list_reverse(list);
}

/*
 *-------------------------------------------------------------------
 * keyboard
 *-------------------------------------------------------------------
 */

static gboolean vf_press_key_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	ViewFile *vf = data;
	gboolean ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_press_key_cb(widget, event, data); break;
	case FILEVIEW_ICON: ret = vficon_press_key_cb(widget, event, data); break;
	default: ret = FALSE;
	}

	return ret;
}

/*
 *-------------------------------------------------------------------
 * mouse
 *-------------------------------------------------------------------
 */

static gboolean vf_press_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	ViewFile *vf = data;
	gboolean ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_press_cb(widget, bevent, data); break;
	case FILEVIEW_ICON: ret = vficon_press_cb(widget, bevent, data); break;
	default: ret = FALSE;
	}

	return ret;
}

static gboolean vf_release_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	ViewFile *vf = data;
	gboolean ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_release_cb(widget, bevent, data); break;
	case FILEVIEW_ICON: ret = vficon_release_cb(widget, bevent, data); break;
	default: ret = FALSE;
	}

	return ret;
}


/*
 *-----------------------------------------------------------------------------
 * selections
 *-----------------------------------------------------------------------------
 */

guint vf_selection_count(ViewFile *vf, gint64 *bytes)
{
	guint ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_selection_count(vf, bytes); break;
	case FILEVIEW_ICON: ret = vficon_selection_count(vf, bytes); break;
	default: ret = 0;
	}

	return ret;
}

GList *vf_selection_get_list(ViewFile *vf)
{
	GList *ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_selection_get_list(vf); break;
	case FILEVIEW_ICON: ret = vficon_selection_get_list(vf); break;
	default: ret = NULL;
	}

	return ret;
}

GList *vf_selection_get_list_by_index(ViewFile *vf)
{
	GList *ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_selection_get_list_by_index(vf); break;
	case FILEVIEW_ICON: ret = vficon_selection_get_list_by_index(vf); break;
	default: ret = NULL;
	}

	return ret;
}

void vf_select_all(ViewFile *vf)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_select_all(vf); break;
	case FILEVIEW_ICON: vficon_select_all(vf); break;
	}
}

void vf_select_none(ViewFile *vf)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_select_none(vf); break;
	case FILEVIEW_ICON: vficon_select_none(vf); break;
	}
}

void vf_select_invert(ViewFile *vf)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_select_invert(vf); break;
	case FILEVIEW_ICON: vficon_select_invert(vf); break;
	}
}

void vf_select_by_fd(ViewFile *vf, FileData *fd)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_select_by_fd(vf, fd); break;
	case FILEVIEW_ICON: vficon_select_by_fd(vf, fd); break;
	}
}

void vf_select_list(ViewFile *vf, GList *list)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_select_list(vf, list); break;
	case FILEVIEW_ICON: vficon_select_list(vf, list); break;
	}
}

void vf_mark_to_selection(ViewFile *vf, gint mark, MarkToSelectionMode mode)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_mark_to_selection(vf, mark, mode); break;
	case FILEVIEW_ICON: vficon_mark_to_selection(vf, mark, mode); break;
	}
}

void vf_selection_to_mark(ViewFile *vf, gint mark, SelectionToMarkMode mode)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_selection_to_mark(vf, mark, mode); break;
	case FILEVIEW_ICON: vficon_selection_to_mark(vf, mark, mode); break;
	}
}

/*
 *-----------------------------------------------------------------------------
 * dnd
 *-----------------------------------------------------------------------------
 */


static void vf_dnd_init(ViewFile *vf)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_dnd_init(vf); break;
	case FILEVIEW_ICON: vficon_dnd_init(vf); break;
	}
}

/*
 *-----------------------------------------------------------------------------
 * pop-up menu
 *-----------------------------------------------------------------------------
 */

GList *vf_pop_menu_file_list(ViewFile *vf)
{
	GList *ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_pop_menu_file_list(vf); break;
	case FILEVIEW_ICON: ret = vficon_pop_menu_file_list(vf); break;
	default: ret = NULL;
	}

	return ret;
}

GList *vf_selection_get_one(ViewFile *vf, FileData *fd)
{
	GList *ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_selection_get_one(vf, fd); break;
	case FILEVIEW_ICON: ret = vficon_selection_get_one(vf, fd); break;
	default: ret = NULL;
	}

	return ret;
}

static void vf_pop_menu_edit_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf;
	const gchar *key = data;

	vf = submenu_item_get_data(widget);

	if (!vf) return;

	file_util_start_editor_from_filelist(key, vf_pop_menu_file_list(vf), vf->dir_fd->path, vf->listview);
}

static void vf_pop_menu_view_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;

	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_pop_menu_view_cb(widget, data); break;
	case FILEVIEW_ICON: vficon_pop_menu_view_cb(widget, data); break;
	}
}

static void vf_pop_menu_open_archive_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	LayoutWindow *lw_new;
	FileData *fd;
	gchar *dest_dir;

	switch (vf->type)
	{
	case FILEVIEW_LIST:
		fd = (VFLIST(vf)->click_fd);
		break;
	case FILEVIEW_ICON:
		fd = (VFICON(vf)->click_fd);
		break;
	}

	dest_dir = open_archive(fd);
	if (dest_dir)
		{
		lw_new = layout_new_from_default();
		layout_set_path(lw_new, dest_dir);
		g_free(dest_dir);
		}
	else
		{
		warning_dialog(_("Cannot open archive file"), _("See the Log Window"), GTK_STOCK_DIALOG_WARNING, NULL);
		}
}

static void vf_pop_menu_copy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	file_util_copy(NULL, vf_pop_menu_file_list(vf), NULL, vf->listview);
}

static void vf_pop_menu_move_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	file_util_move(NULL, vf_pop_menu_file_list(vf), NULL, vf->listview);
}

static void vf_pop_menu_rename_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;

	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_pop_menu_rename_cb(widget, data); break;
	case FILEVIEW_ICON: vficon_pop_menu_rename_cb(widget, data); break;
	}
}

static void vf_pop_menu_delete_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	options->file_ops.safe_delete_enable = FALSE;
	file_util_delete(NULL, vf_pop_menu_file_list(vf), vf->listview);
}

static void vf_pop_menu_move_to_trash_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	options->file_ops.safe_delete_enable = TRUE;
	file_util_delete(NULL, vf_pop_menu_file_list(vf), vf->listview);
}

static void vf_pop_menu_copy_path_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	file_util_copy_path_list_to_clipboard(vf_pop_menu_file_list(vf), TRUE);
}

static void vf_pop_menu_copy_path_unquoted_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	file_util_copy_path_list_to_clipboard(vf_pop_menu_file_list(vf), FALSE);
}

static void vf_pop_menu_enable_grouping_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	file_data_disable_grouping_list(vf_pop_menu_file_list(vf), FALSE);
}

static void vf_pop_menu_duplicates_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	DupeWindow *dw;

	dw = dupe_window_new();
	dupe_window_add_files(dw, vf_pop_menu_file_list(vf), FALSE);
}

static void vf_pop_menu_disable_grouping_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	file_data_disable_grouping_list(vf_pop_menu_file_list(vf), TRUE);
}

static void vf_pop_menu_sort_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf;
	SortType type;

	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) return;

	vf = submenu_item_get_data(widget);
	if (!vf) return;

	type = (SortType)GPOINTER_TO_INT(data);

	if (type == SORT_EXIFTIME || type == SORT_EXIFTIMEDIGITIZED || type == SORT_RATING)
		{
		vf_read_metadata_in_idle(vf);
		}

	if (vf->layout)
		{
		layout_sort_set(vf->layout, type, vf->sort_ascend);
		}
	else
		{
		vf_sort_set(vf, type, vf->sort_ascend);
		}
}

static void vf_pop_menu_sort_ascend_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	if (vf->layout)
		{
		layout_sort_set(vf->layout, vf->sort_method, !vf->sort_ascend);
		}
	else
		{
		vf_sort_set(vf, vf->sort_method, !vf->sort_ascend);
		}
}

static void vf_pop_menu_sel_mark_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	vf_mark_to_selection(vf, vf->active_mark, MTS_MODE_SET);
}

static void vf_pop_menu_sel_mark_and_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	vf_mark_to_selection(vf, vf->active_mark, MTS_MODE_AND);
}

static void vf_pop_menu_sel_mark_or_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	vf_mark_to_selection(vf, vf->active_mark, MTS_MODE_OR);
}

static void vf_pop_menu_sel_mark_minus_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	vf_mark_to_selection(vf, vf->active_mark, MTS_MODE_MINUS);
}

static void vf_pop_menu_set_mark_sel_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	vf_selection_to_mark(vf, vf->active_mark, STM_MODE_SET);
}

static void vf_pop_menu_res_mark_sel_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	vf_selection_to_mark(vf, vf->active_mark, STM_MODE_RESET);
}

static void vf_pop_menu_toggle_mark_sel_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	vf_selection_to_mark(vf, vf->active_mark, STM_MODE_TOGGLE);
}

static void vf_pop_menu_toggle_view_type_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;
	FileViewType new_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "menu_item_radio_data"));
	if (!vf->layout) return;

	layout_views_set(vf->layout, vf->layout->options.dir_view_type, new_type);
}

static void vf_pop_menu_refresh_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;

	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_pop_menu_refresh_cb(widget, data); break;
	case FILEVIEW_ICON: vficon_pop_menu_refresh_cb(widget, data); break;
	}
}

static void vf_popup_destroy_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;

	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_popup_destroy_cb(widget, data); break;
	case FILEVIEW_ICON: vficon_popup_destroy_cb(widget, data); break;
	}

	filelist_free(vf->editmenu_fd_list);
	vf->editmenu_fd_list = NULL;
}

/**
 * @brief Add file selection list to a collection
 * @param[in] widget 
 * @param[in] data Index to the collection list menu item selected, or -1 for new collection
 * 
 * 
 */
static void vf_pop_menu_collections_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf;
	GList *selection_list;

	vf = submenu_item_get_data(widget);
	selection_list = vf_selection_get_list(vf);
	pop_menu_collections(selection_list, data);

	filelist_free(selection_list);
}

GtkWidget *vf_pop_menu(ViewFile *vf)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *submenu;
	gboolean active = FALSE;
	gboolean class_archive = FALSE;
	GtkAccelGroup *accel_group;

	switch (vf->type)
	{
	case FILEVIEW_LIST:
		vflist_color_set(vf, VFLIST(vf)->click_fd, TRUE);
		active = (VFLIST(vf)->click_fd != NULL);
		class_archive = (VFLIST(vf)->click_fd != NULL && VFLIST(vf)->click_fd->format_class == FORMAT_CLASS_ARCHIVE);
		break;
	case FILEVIEW_ICON:
		active = (VFICON(vf)->click_fd != NULL);
		class_archive = (VFICON(vf)->click_fd != NULL && VFICON(vf)->click_fd->format_class == FORMAT_CLASS_ARCHIVE);
		break;
	}

	menu = popup_menu_short_lived();

	accel_group = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);

	g_object_set_data(G_OBJECT(menu), "window_keys", NULL);
	g_object_set_data(G_OBJECT(menu), "accel_group", accel_group);

	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(vf_popup_destroy_cb), vf);

	if (vf->clicked_mark > 0)
		{
		gint mark = vf->clicked_mark;
		gchar *str_set_mark = g_strdup_printf(_("_Set mark %d"), mark);
		gchar *str_res_mark = g_strdup_printf(_("_Reset mark %d"), mark);
		gchar *str_toggle_mark = g_strdup_printf(_("_Toggle mark %d"), mark);
		gchar *str_sel_mark = g_strdup_printf(_("_Select mark %d"), mark);
		gchar *str_sel_mark_or = g_strdup_printf(_("_Add mark %d"), mark);
		gchar *str_sel_mark_and = g_strdup_printf(_("_Intersection with mark %d"), mark);
		gchar *str_sel_mark_minus = g_strdup_printf(_("_Unselect mark %d"), mark);

		g_assert(mark >= 1 && mark <= FILEDATA_MARKS_SIZE);

		vf->active_mark = mark;
		vf->clicked_mark = 0;

		menu_item_add_sensitive(menu, str_set_mark, active,
					G_CALLBACK(vf_pop_menu_set_mark_sel_cb), vf);

		menu_item_add_sensitive(menu, str_res_mark, active,
					G_CALLBACK(vf_pop_menu_res_mark_sel_cb), vf);

		menu_item_add_sensitive(menu, str_toggle_mark, active,
					G_CALLBACK(vf_pop_menu_toggle_mark_sel_cb), vf);

		menu_item_add_divider(menu);

		menu_item_add_sensitive(menu, str_sel_mark, active,
					G_CALLBACK(vf_pop_menu_sel_mark_cb), vf);
		menu_item_add_sensitive(menu, str_sel_mark_or, active,
					G_CALLBACK(vf_pop_menu_sel_mark_or_cb), vf);
		menu_item_add_sensitive(menu, str_sel_mark_and, active,
					G_CALLBACK(vf_pop_menu_sel_mark_and_cb), vf);
		menu_item_add_sensitive(menu, str_sel_mark_minus, active,
					G_CALLBACK(vf_pop_menu_sel_mark_minus_cb), vf);

		menu_item_add_divider(menu);

		g_free(str_set_mark);
		g_free(str_res_mark);
		g_free(str_toggle_mark);
		g_free(str_sel_mark);
		g_free(str_sel_mark_and);
		g_free(str_sel_mark_or);
		g_free(str_sel_mark_minus);
		}

	vf->editmenu_fd_list = vf_pop_menu_file_list(vf);
	submenu_add_edit(menu, &item, G_CALLBACK(vf_pop_menu_edit_cb), vf, vf->editmenu_fd_list);
	gtk_widget_set_sensitive(item, active);

	menu_item_add_stock_sensitive(menu, _("View in _new window"), GTK_STOCK_NEW, active,
				      G_CALLBACK(vf_pop_menu_view_cb), vf);

	menu_item_add_stock_sensitive(menu, _("Open archive"), GTK_STOCK_OPEN, active & class_archive, G_CALLBACK(vf_pop_menu_open_archive_cb), vf);

	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu, _("_Copy..."), GTK_STOCK_COPY, active,
				      G_CALLBACK(vf_pop_menu_copy_cb), vf);
	menu_item_add_sensitive(menu, _("_Move..."), active,
				G_CALLBACK(vf_pop_menu_move_cb), vf);
	menu_item_add_sensitive(menu, _("_Rename..."), active,
				G_CALLBACK(vf_pop_menu_rename_cb), vf);
	menu_item_add_sensitive(menu, _("_Copy path to clipboard"), active,
				G_CALLBACK(vf_pop_menu_copy_path_cb), vf);
	menu_item_add_sensitive(menu, _("_Copy path unquoted to clipboard"), active,
				G_CALLBACK(vf_pop_menu_copy_path_unquoted_cb), vf);
	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu,
				options->file_ops.confirm_move_to_trash ? _("Move to Trash...") :
					_("Move to Trash"), PIXBUF_INLINE_ICON_TRASH, active,
				G_CALLBACK(vf_pop_menu_move_to_trash_cb), vf);
	menu_item_add_stock_sensitive(menu,
				options->file_ops.confirm_delete ? _("_Delete...") :
					_("_Delete"), GTK_STOCK_DELETE, active,
				G_CALLBACK(vf_pop_menu_delete_cb), vf);
	menu_item_add_divider(menu);

	menu_item_add_sensitive(menu, _("Enable file _grouping"), active,
				G_CALLBACK(vf_pop_menu_enable_grouping_cb), vf);
	menu_item_add_sensitive(menu, _("Disable file groupi_ng"), active,
				G_CALLBACK(vf_pop_menu_disable_grouping_cb), vf);

	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu, _("_Find duplicates..."), GTK_STOCK_FIND, active,
				G_CALLBACK(vf_pop_menu_duplicates_cb), vf);
	menu_item_add_divider(menu);

	submenu = submenu_add_collections(menu, &item,
				G_CALLBACK(vf_pop_menu_collections_cb), vf);
	gtk_widget_set_sensitive(item, active);
	menu_item_add_divider(menu);

	submenu = submenu_add_sort(NULL, G_CALLBACK(vf_pop_menu_sort_cb), vf,
				   FALSE, FALSE, TRUE, vf->sort_method);
	menu_item_add_divider(submenu);
	menu_item_add_check(submenu, _("Ascending"), vf->sort_ascend,
			    G_CALLBACK(vf_pop_menu_sort_ascend_cb), vf);

	item = menu_item_add(menu, _("_Sort"), NULL, NULL);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	item = menu_item_add_radio(menu, _("Images as List"), GINT_TO_POINTER(FILEVIEW_LIST), vf->type == FILEVIEW_LIST,
                                           G_CALLBACK(vf_pop_menu_toggle_view_type_cb), vf);

	item = menu_item_add_radio(menu, _("Images as Icons"), GINT_TO_POINTER(FILEVIEW_ICON), vf->type == FILEVIEW_ICON,
                                           G_CALLBACK(vf_pop_menu_toggle_view_type_cb), vf);

	switch (vf->type)
	{
	case FILEVIEW_LIST:
		menu_item_add_check(menu, _("Show _thumbnails"), VFLIST(vf)->thumbs_enabled,
				    G_CALLBACK(vflist_pop_menu_thumbs_cb), vf);
		break;
	case FILEVIEW_ICON:
		menu_item_add_check(menu, _("Show filename _text"), VFICON(vf)->show_text,
				    G_CALLBACK(vficon_pop_menu_show_names_cb), vf);
		break;
	}

	switch (vf->type)
	{
	case FILEVIEW_LIST:
		menu_item_add_check(menu, _("Show star rating"), options->show_star_rating,
				    G_CALLBACK(vflist_pop_menu_show_star_rating_cb), vf);
		break;
	case FILEVIEW_ICON:
		menu_item_add_check(menu, _("Show star rating"), options->show_star_rating,
				    G_CALLBACK(vficon_pop_menu_show_star_rating_cb), vf);
		break;
	}

	menu_item_add_stock(menu, _("Re_fresh"), GTK_STOCK_REFRESH, G_CALLBACK(vf_pop_menu_refresh_cb), vf);

	return menu;
}

gboolean vf_refresh(ViewFile *vf)
{
	gboolean ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_refresh(vf); break;
	case FILEVIEW_ICON: ret = vficon_refresh(vf); break;
	default: ret = FALSE;
	}

	return ret;
}

gboolean vf_set_fd(ViewFile *vf, FileData *dir_fd)
{
	gboolean ret;

	switch (vf->type)
	{
	case FILEVIEW_LIST: ret = vflist_set_fd(vf, dir_fd); break;
	case FILEVIEW_ICON: ret = vficon_set_fd(vf, dir_fd); break;
	default: ret = FALSE;
	}

	return ret;
}

static void vf_destroy_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;

	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_destroy_cb(widget, data); break;
	case FILEVIEW_ICON: vficon_destroy_cb(widget, data); break;
	}

	if (vf->popup)
		{
		g_signal_handlers_disconnect_matched(G_OBJECT(vf->popup), G_SIGNAL_MATCH_DATA,
						     0, 0, 0, NULL, vf);
		gtk_widget_destroy(vf->popup);
		}

	if (vf->read_metadata_in_idle_id)
		{
		g_idle_remove_by_data(vf);
		}
	file_data_unref(vf->dir_fd);
	g_free(vf->info);
	g_free(vf);
}

static void vf_marks_filter_toggle_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	vf_refresh_idle(vf);
}

typedef struct _MarksTextEntry MarksTextEntry;
struct _MarksTextEntry {
	GenericDialog *gd;
	gint mark_no;
	GtkWidget *edit_widget;
	gchar *text_entry;
	GtkWidget *parent;
};

static void vf_marks_tooltip_cancel_cb(GenericDialog *gd, gpointer data)
{
	MarksTextEntry *mte = data;

	g_free(mte->text_entry);
	generic_dialog_close(gd);
}

static void vf_marks_tooltip_ok_cb(GenericDialog *gd, gpointer data)
{
	MarksTextEntry *mte = data;

	g_free(options->marks_tooltips[mte->mark_no]);
	options->marks_tooltips[mte->mark_no] = g_strdup(gtk_entry_get_text(GTK_ENTRY(mte->edit_widget)));

	gtk_widget_set_tooltip_text(mte->parent, options->marks_tooltips[mte->mark_no]);

	g_free(mte->text_entry);
	generic_dialog_close(gd);
}

void vf_marks_filter_on_icon_press(GtkEntry *UNUSED(entry), GtkEntryIconPosition UNUSED(pos),
									GdkEvent *UNUSED(event), gpointer userdata)
{
	MarksTextEntry *mte = userdata;

	g_free(mte->text_entry);
	mte->text_entry = g_strdup("");
	gtk_entry_set_text(GTK_ENTRY(mte->edit_widget), "");
}

static void vf_marks_tooltip_help_cb(GenericDialog *UNUSED(gd), gpointer UNUSED(data))
{
	help_window_show("GuideImageMarks.html");
}

static gboolean vf_marks_tooltip_cb(GtkWidget *widget,
										GdkEventButton *event,
										gpointer user_data)
{
	GtkWidget *table;
	gint i = GPOINTER_TO_INT(user_data);
	MarksTextEntry *mte;

	if (event->button == MOUSE_BUTTON_RIGHT)
		{
		mte = g_new0(MarksTextEntry, 1);
		mte->mark_no = i;
		mte->text_entry = g_strdup(options->marks_tooltips[i]);
		mte->parent = widget;

		mte->gd = generic_dialog_new(_("Mark text"), "mark_text",
						widget, FALSE,
						vf_marks_tooltip_cancel_cb, mte);
		generic_dialog_add_message(mte->gd, GTK_STOCK_DIALOG_QUESTION, _("Set mark text"),
					    _("This will set or clear the mark text."), FALSE);
		generic_dialog_add_button(mte->gd, GTK_STOCK_OK, NULL,
							vf_marks_tooltip_ok_cb, TRUE);
		generic_dialog_add_button(mte->gd, GTK_STOCK_HELP, NULL,
						vf_marks_tooltip_help_cb, FALSE);

		table = pref_table_new(mte->gd->vbox, 3, 1, FALSE, TRUE);
		pref_table_label(table, 0, 0, g_strdup_printf("%s%d", _("Mark "), mte->mark_no + 1), 1.0);
		mte->edit_widget = gtk_entry_new();
		gtk_widget_set_size_request(mte->edit_widget, 300, -1);
		if (mte->text_entry)
			{
			gtk_entry_set_text(GTK_ENTRY(mte->edit_widget), mte->text_entry);
			}
		gtk_table_attach_defaults(GTK_TABLE(table), mte->edit_widget, 1, 2, 0, 1);
		generic_dialog_attach_default(mte->gd, mte->edit_widget);

		gtk_entry_set_icon_from_stock(GTK_ENTRY(mte->edit_widget),
							GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
		gtk_entry_set_icon_tooltip_text (GTK_ENTRY(mte->edit_widget),
							GTK_ENTRY_ICON_SECONDARY, "Clear");
		g_signal_connect(GTK_ENTRY(mte->edit_widget), "icon-press",
							G_CALLBACK(vf_marks_filter_on_icon_press), mte);

		gtk_widget_show(mte->edit_widget);
		gtk_widget_grab_focus(mte->edit_widget);
		gtk_widget_show(GTK_WIDGET(mte->gd->dialog));

		return TRUE;
		}

	return FALSE;
}

static void vf_file_filter_save_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;
	gchar *entry_text;
	gchar *remove_text = NULL;
	gchar *index_text = NULL;
	gboolean text_found = FALSE;
	gint i;

	entry_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(vf->file_filter.combo)))));

	if (entry_text[0] == '\0' && vf->file_filter.last_selected >= 0)
		{
		gtk_combo_box_set_active(GTK_COMBO_BOX(vf->file_filter.combo), vf->file_filter.last_selected);
		remove_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(vf->file_filter.combo));
		history_list_item_remove("file_filter", remove_text);
		gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(vf->file_filter.combo), vf->file_filter.last_selected);
		g_free(remove_text);

		gtk_combo_box_set_active(GTK_COMBO_BOX(vf->file_filter.combo), -1);
		vf->file_filter.last_selected = - 1;
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(vf->file_filter.combo))), "");
		vf->file_filter.count--;
		}
	else
		{
		if (entry_text[0] != '\0')
			{
			for (i = 0; i < vf->file_filter.count; i++)
				{
				if (index_text)
					{
					g_free(index_text);
					}
				gtk_combo_box_set_active(GTK_COMBO_BOX(vf->file_filter.combo), i);
				index_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(vf->file_filter.combo));

				if (g_strcmp0(index_text, entry_text) == 0)
					{
					text_found = TRUE;
					break;
					}
				}

			g_free(index_text);
			if (!text_found)
				{
				history_list_add_to_key("file_filter", entry_text, 10);
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vf->file_filter.combo), entry_text);
				vf->file_filter.count++;
				gtk_combo_box_set_active(GTK_COMBO_BOX(vf->file_filter.combo), vf->file_filter.count - 1);
				}
			}
		}
	vf_refresh(vf);

	g_free(entry_text);
}

static void vf_file_filter_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewFile *vf = data;

	vf_refresh(vf);
}

static gboolean vf_file_filter_press_cb(GtkWidget *widget, GdkEventButton *UNUSED(bevent), gpointer data)
{
	ViewFile *vf = data;
	vf->file_filter.last_selected = gtk_combo_box_get_active(GTK_COMBO_BOX(vf->file_filter.combo));

	gtk_widget_grab_focus(widget);

	return TRUE;
}

static GtkWidget *vf_marks_filter_init(ViewFile *vf)
{
	GtkWidget *frame = gtk_frame_new(NULL);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	gint i;

	for (i = 0; i < FILEDATA_MARKS_SIZE ; i++)
		{
		GtkWidget *check = gtk_check_button_new();
		gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(check), "toggled",
			 G_CALLBACK(vf_marks_filter_toggle_cb), vf);
		g_signal_connect(G_OBJECT(check), "button_press_event",
			 G_CALLBACK(vf_marks_tooltip_cb), GINT_TO_POINTER(i));
		gtk_widget_set_tooltip_text(check, options->marks_tooltips[i]);

		gtk_widget_show(check);
		vf->filter_check[i] = check;
		}
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	gtk_widget_show(hbox);
	return frame;
}

void vf_file_filter_set(ViewFile *vf, gboolean enable)
{
	if (enable)
		{
		gtk_widget_show(vf->file_filter.combo);
		gtk_widget_show(vf->file_filter.frame);
		}
	else
		{
		gtk_widget_hide(vf->file_filter.combo);
		gtk_widget_hide(vf->file_filter.frame);
		}

	vf_refresh(vf);
}

static gboolean vf_file_filter_class_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;
	gint i;

	gboolean state = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));

	for (i = 0; i < FILE_FORMAT_CLASSES; i++)
		{
		if (g_strcmp0(format_class_list[i], gtk_menu_item_get_label(GTK_MENU_ITEM(widget))) == 0)
			{
			options->class_filter[i] = state;
			}
		}
	vf_refresh(vf);

	return TRUE;
}

static gboolean vf_file_filter_class_set_all_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;
	GtkWidget *parent;
	GList *children;
	GtkWidget *child;
	gint i;
	gboolean state;

	if (g_strcmp0(_("Select all"), gtk_menu_item_get_label(GTK_MENU_ITEM(widget))) == 0)
		{
		state = TRUE;
		}
	else
		{
		state = FALSE;
		}

	for (i = 0; i < FILE_FORMAT_CLASSES; i++)
		{
		options->class_filter[i] = state;
		}

	i = 0;
	parent = gtk_widget_get_parent(widget);
	children = gtk_container_get_children(GTK_CONTAINER(parent));
	while (children)
		{
		child = children->data;
		if (i < FILE_FORMAT_CLASSES)
			{
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(child), state);
			}
		i++;
		children = children->next;
		}
	g_list_free(children);
	vf_refresh(vf);

	return TRUE;
}

static GtkWidget *class_filter_menu (ViewFile *vf)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	int i;

	menu = gtk_menu_new();

	for (i = 0; i < FILE_FORMAT_CLASSES; i++)
	    {
		menu_item = gtk_check_menu_item_new_with_label(format_class_list[i]);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), options->class_filter[i]);
		g_signal_connect(G_OBJECT(menu_item), "toggled", G_CALLBACK(vf_file_filter_class_cb), vf);
		gtk_menu_shell_append(GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show(menu_item);
		}

	menu_item = gtk_menu_item_new_with_label(_("Select all"));
	gtk_menu_shell_append(GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(vf_file_filter_class_set_all_cb), vf);

	menu_item = gtk_menu_item_new_with_label(_("Select none"));
	gtk_menu_shell_append(GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(vf_file_filter_class_set_all_cb), vf);

	return menu;
}

static void case_sensitive_cb(GtkWidget *widget, gpointer data)
{
	ViewFile *vf = data;

	vf->file_filter.case_sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	vf_refresh(vf);
}

static void file_filter_clear_cb(GtkEntry *UNUSED(entry), GtkEntryIconPosition pos, GdkEvent *UNUSED(event), gpointer userdata)
{
	if (pos == GTK_ENTRY_ICON_SECONDARY)
		{
		gtk_entry_set_text(GTK_ENTRY(userdata), "");
		gtk_widget_grab_focus(GTK_WIDGET(userdata));
		}
}

static GtkWidget *vf_file_filter_init(ViewFile *vf)
{
	GtkWidget *frame = gtk_frame_new(NULL);
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GList *work;
	gint n = 0;
	GtkWidget *combo_entry;
	GtkWidget *menubar;
	GtkWidget *menuitem;
	GtkWidget *case_sensitive;
	GtkWidget *box;
	GtkWidget *icon;
	GtkWidget *label;

	vf->file_filter.combo = gtk_combo_box_text_new_with_entry();
	combo_entry = gtk_bin_get_child(GTK_BIN(vf->file_filter.combo));
	gtk_widget_show(gtk_bin_get_child(GTK_BIN(vf->file_filter.combo)));
	gtk_widget_show((GTK_WIDGET(vf->file_filter.combo)));
	gtk_widget_set_tooltip_text(GTK_WIDGET(vf->file_filter.combo), _("Use regular expressions"));

	gtk_entry_set_icon_from_stock(GTK_ENTRY(combo_entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
	gtk_entry_set_icon_tooltip_text (GTK_ENTRY(combo_entry), GTK_ENTRY_ICON_SECONDARY, _("Clear"));
	g_signal_connect(GTK_ENTRY(combo_entry), "icon-press", G_CALLBACK(file_filter_clear_cb), combo_entry);

	work = history_list_get_by_key("file_filter");
	while (work)
		{
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(vf->file_filter.combo), (gchar *)work->data);
		work = work->next;
		n++;
		vf->file_filter.count = n;
		}
	gtk_combo_box_set_active(GTK_COMBO_BOX(vf->file_filter.combo), 0);

	g_signal_connect(G_OBJECT(combo_entry), "activate",
		G_CALLBACK(vf_file_filter_save_cb), vf);
		
	g_signal_connect(G_OBJECT(vf->file_filter.combo), "changed",
		G_CALLBACK(vf_file_filter_cb), vf);

	g_signal_connect(G_OBJECT(combo_entry), "button_press_event",
			 G_CALLBACK(vf_file_filter_press_cb), vf);

	gtk_box_pack_start(GTK_BOX(hbox), vf->file_filter.combo, FALSE, FALSE, 0);
	gtk_widget_show(vf->file_filter.combo);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	gtk_widget_show(hbox);

	case_sensitive = gtk_check_button_new_with_label(_("Case"));
	gtk_box_pack_start(GTK_BOX(hbox), case_sensitive, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(GTK_WIDGET(case_sensitive), _("Case sensitive"));
	g_signal_connect(G_OBJECT(case_sensitive), "clicked", G_CALLBACK(case_sensitive_cb), vf);
	gtk_widget_show(case_sensitive);

	menubar = gtk_menu_bar_new();
	gtk_box_pack_start(GTK_BOX(hbox), menubar, FALSE, TRUE, 0);
	gtk_widget_show(menubar);

	box = gtk_hbox_new(GTK_ORIENTATION_HORIZONTAL, PREF_PAD_GAP);
	icon = gtk_image_new_from_icon_name("pan-down", GTK_ICON_SIZE_MENU);
	label = gtk_label_new(_("Class"));

	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(box), icon, FALSE, FALSE, 0);

	menuitem = gtk_menu_item_new();

	gtk_widget_set_tooltip_text(GTK_WIDGET(menuitem), _("Select Class filter"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), class_filter_menu(vf));
	gtk_menu_shell_append(GTK_MENU_SHELL(menubar), menuitem);
	gtk_container_add(GTK_CONTAINER(menuitem), box);
	gtk_widget_show_all(menuitem);

	return frame;
}

void vf_mark_filter_toggle(ViewFile *vf, gint mark)
{
	gint n = mark - 1;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vf->filter_check[n]),
				     !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vf->filter_check[n])));
}

ViewFile *vf_new(FileViewType type, FileData *dir_fd)
{
	ViewFile *vf;

	vf = g_new0(ViewFile, 1);

	vf->type = type;
	vf->sort_method = SORT_NAME;
	vf->sort_ascend = TRUE;
	vf->read_metadata_in_idle_id = 0;

	vf->scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(vf->scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(vf->scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	vf->filter = vf_marks_filter_init(vf);
	vf->file_filter.frame = vf_file_filter_init(vf);

	vf->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(vf->widget), vf->filter, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vf->widget), vf->file_filter.frame, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vf->widget), vf->scrolled, TRUE, TRUE, 0);
	gtk_widget_show(vf->scrolled);

	g_signal_connect(G_OBJECT(vf->widget), "destroy",
			 G_CALLBACK(vf_destroy_cb), vf);

	switch (type)
	{
	case FILEVIEW_LIST: vf = vflist_new(vf, dir_fd); break;
	case FILEVIEW_ICON: vf = vficon_new(vf, dir_fd); break;
	}

	vf_dnd_init(vf);

	g_signal_connect(G_OBJECT(vf->listview), "key_press_event",
			 G_CALLBACK(vf_press_key_cb), vf);
	g_signal_connect(G_OBJECT(vf->listview), "button_press_event",
			 G_CALLBACK(vf_press_cb), vf);
	g_signal_connect(G_OBJECT(vf->listview), "button_release_event",
			 G_CALLBACK(vf_release_cb), vf);

	gtk_container_add(GTK_CONTAINER(vf->scrolled), vf->listview);
	gtk_widget_show(vf->listview);

	if (dir_fd) vf_set_fd(vf, dir_fd);

	return vf;
}

void vf_set_status_func(ViewFile *vf, void (*func)(ViewFile *vf, gpointer data), gpointer data)
{
	vf->func_status = func;
	vf->data_status = data;
}

void vf_set_thumb_status_func(ViewFile *vf, void (*func)(ViewFile *vf, gdouble val, const gchar *text, gpointer data), gpointer data)
{
	vf->func_thumb_status = func;
	vf->data_thumb_status = data;
}

void vf_thumb_set(ViewFile *vf, gboolean enable)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_thumb_set(vf, enable); break;
	case FILEVIEW_ICON: /*vficon_thumb_set(vf, enable);*/ break;
	}
}


static gboolean vf_thumb_next(ViewFile *vf);

static gdouble vf_thumb_progress(ViewFile *vf)
{
	gint count = 0;
	gint done = 0;

	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_thumb_progress_count(vf->list, &count, &done); break;
	case FILEVIEW_ICON: vficon_thumb_progress_count(vf->list, &count, &done); break;
	}

	DEBUG_1("thumb progress: %d of %d", done, count);
	return (gdouble)done / count;
}

static gdouble vf_read_metadata_in_idle_progress(ViewFile *vf)
{
	gint count = 0;
	gint done = 0;

	switch (vf->type)
		{
		case FILEVIEW_LIST: vflist_read_metadata_progress_count(vf->list, &count, &done); break;
		case FILEVIEW_ICON: vficon_read_metadata_progress_count(vf->list, &count, &done); break;
		}

	return (gdouble)done / count;
}

static void vf_set_thumb_fd(ViewFile *vf, FileData *fd)
{
	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_set_thumb_fd(vf, fd); break;
	case FILEVIEW_ICON: vficon_set_thumb_fd(vf, fd); break;
	}
}

static void vf_thumb_status(ViewFile *vf, gdouble val, const gchar *text)
{
	if (vf->func_thumb_status)
		{
		vf->func_thumb_status(vf, val, text, vf->data_thumb_status);
		}
}

static void vf_thumb_do(ViewFile *vf, FileData *fd)
{
	if (!fd) return;

	vf_set_thumb_fd(vf, fd);
	vf_thumb_status(vf, vf_thumb_progress(vf), _("Loading thumbs..."));
}

void vf_thumb_cleanup(ViewFile *vf)
{
	vf_thumb_status(vf, 0.0, NULL);

	vf->thumbs_running = FALSE;

	thumb_loader_free(vf->thumbs_loader);
	vf->thumbs_loader = NULL;

	vf->thumbs_filedata = NULL;
}

void vf_thumb_stop(ViewFile *vf)
{
	if (vf->thumbs_running) vf_thumb_cleanup(vf);
}

static void vf_thumb_common_cb(ThumbLoader *tl, gpointer data)
{
	ViewFile *vf = data;

	if (vf->thumbs_filedata && vf->thumbs_loader == tl)
		{
		vf_thumb_do(vf, vf->thumbs_filedata);
		}

	while (vf_thumb_next(vf));
}

static void vf_thumb_error_cb(ThumbLoader *tl, gpointer data)
{
	vf_thumb_common_cb(tl, data);
}

static void vf_thumb_done_cb(ThumbLoader *tl, gpointer data)
{
	vf_thumb_common_cb(tl, data);
}

static gboolean vf_thumb_next(ViewFile *vf)
{
	FileData *fd = NULL;

	if (!gtk_widget_get_realized(vf->listview))
		{
		vf_thumb_status(vf, 0.0, NULL);
		return FALSE;
		}

	switch (vf->type)
	{
	case FILEVIEW_LIST: fd = vflist_thumb_next_fd(vf); break;
	case FILEVIEW_ICON: fd = vficon_thumb_next_fd(vf); break;
	}

	if (!fd)
		{
		/* done */
		vf_thumb_cleanup(vf);
		return FALSE;
		}

	vf->thumbs_filedata = fd;

	thumb_loader_free(vf->thumbs_loader);

	vf->thumbs_loader = thumb_loader_new(options->thumbnails.max_width, options->thumbnails.max_height);
	thumb_loader_set_callbacks(vf->thumbs_loader,
				   vf_thumb_done_cb,
				   vf_thumb_error_cb,
				   NULL,
				   vf);

	if (!thumb_loader_start(vf->thumbs_loader, fd))
		{
		/* set icon to unknown, continue */
		DEBUG_1("thumb loader start failed %s", fd->path);
		vf_thumb_do(vf, fd);

		return TRUE;
		}

	return FALSE;
}

static void vf_thumb_reset_all(ViewFile *vf)
{
	GList *work;

	for (work = vf->list; work; work = work->next)
		{
		FileData *fd = work->data;
		if (fd->thumb_pixbuf)
			{
			g_object_unref(fd->thumb_pixbuf);
			fd->thumb_pixbuf = NULL;
			}
		}
}

void vf_thumb_update(ViewFile *vf)
{
	vf_thumb_stop(vf);

	if (vf->type == FILEVIEW_LIST && !VFLIST(vf)->thumbs_enabled) return;

	vf_thumb_status(vf, 0.0, _("Loading thumbs..."));
	vf->thumbs_running = TRUE;

	if (thumb_format_changed)
		{
		vf_thumb_reset_all(vf);
		thumb_format_changed = FALSE;
		}

	while (vf_thumb_next(vf));
}

void vf_star_cleanup(ViewFile *vf)
{
	if (vf->stars_id != 0)
		{
		g_source_remove(vf->stars_id);
		}

	vf->stars_id = 0;
	vf->stars_filedata = NULL;
}

void vf_star_stop(ViewFile *vf)
{
	 vf_star_cleanup(vf);
}

static void vf_set_star_fd(ViewFile *vf, FileData *fd)
{
	switch (vf->type)
		{
		case FILEVIEW_LIST: vflist_set_star_fd(vf, fd); break;
		case FILEVIEW_ICON: vficon_set_star_fd(vf, fd); break;
		default: break;
		}
}

static void vf_star_do(ViewFile *vf, FileData *fd)
{
	if (!fd) return;

	vf_set_star_fd(vf, fd);
}

static gboolean vf_star_next(ViewFile *vf)
{
	FileData *fd = NULL;

	switch (vf->type)
		{
		case FILEVIEW_LIST: fd = vflist_star_next_fd(vf); break;
		case FILEVIEW_ICON: fd = vficon_star_next_fd(vf); break;
		default: break;
		}

	if (!fd)
		{
		/* done */
		vf_star_cleanup(vf);
		return FALSE;
		}

	return TRUE;
}

gboolean vf_stars_cb(gpointer data)
{
	ViewFile *vf = data;
	FileData *fd = vf->stars_filedata;

	if (fd)
		{
		read_rating_data(fd);

		vf_star_do(vf, fd);

		if (vf_star_next(vf))
			{
			return TRUE;
			}
		else
			{
			vf->stars_filedata = NULL;
			vf->stars_id = 0;
			return FALSE;
			}
		}

	return FALSE;
}

void vf_star_update(ViewFile *vf)
{
	vf_star_stop(vf);

	if (!options->show_star_rating)
		{
		return;
		}

	vf_star_next(vf);
}

void vf_marks_set(ViewFile *vf, gboolean enable)
{
	if (vf->marks_enabled == enable) return;

	vf->marks_enabled = enable;

	switch (vf->type)
	{
	case FILEVIEW_LIST: vflist_marks_set(vf, enable); break;
	case FILEVIEW_ICON: vficon_marks_set(vf, enable); break;
	}
	if (enable)
		gtk_widget_show(vf->filter);
	else
		gtk_widget_hide(vf->filter);

	vf_refresh_idle(vf);
}

void vf_star_rating_set(ViewFile *vf, gboolean enable)
{
	if (options->show_star_rating == enable) return;
	options->show_star_rating = enable;

	switch (vf->type)
		{
		case FILEVIEW_LIST: vflist_star_rating_set(vf, enable); break;
		case FILEVIEW_ICON: vficon_star_rating_set(vf, enable); break;
		}
	vf_refresh_idle(vf);
}

guint vf_marks_get_filter(ViewFile *vf)
{
	guint ret = 0;
	gint i;
	if (!vf->marks_enabled) return 0;

	for (i = 0; i < FILEDATA_MARKS_SIZE ; i++)
		{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vf->filter_check[i])))
			{
			ret |= 1 << i;
			}
		}
	return ret;
}

GRegex *vf_file_filter_get_filter(ViewFile *vf)
{
	GRegex *ret = NULL;
	GError *error = NULL;
	gchar *file_filter_text = NULL;

	if (!gtk_widget_get_visible(vf->file_filter.combo))
		{
		return g_regex_new("", 0, 0, NULL);
		}

	file_filter_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(vf->file_filter.combo));

	if (file_filter_text[0] != '\0')
		{
		ret = g_regex_new(file_filter_text, vf->file_filter.case_sensitive ? 0 : G_REGEX_CASELESS, 0, &error);
		if (error)
			{
			log_printf("Error: could not compile regular expression %s\n%s\n", file_filter_text, error->message);
			g_error_free(error);
			error = NULL;
			ret = g_regex_new("", 0, 0, NULL);
			}
		g_free(file_filter_text);
		}
	else
		{
		ret = g_regex_new("", 0, 0, NULL);
		}

	return ret;
}

guint vf_class_get_filter(ViewFile *vf)
{
	guint ret = 0;
	gint i;

	if (!gtk_widget_get_visible(vf->file_filter.combo))
		{
		return G_MAXUINT;
		}

	for ( i = 0; i < FILE_FORMAT_CLASSES; i++)
		{
		if (options->class_filter[i])
			{
			ret |= 1 << i;
			}
		}

	return ret;
}

void vf_set_layout(ViewFile *vf, LayoutWindow *layout)
{
	vf->layout = layout;
}


/*
 *-----------------------------------------------------------------------------
 * maintenance (for rename, move, remove)
 *-----------------------------------------------------------------------------
 */

static gboolean vf_refresh_idle_cb(gpointer data)
{
	ViewFile *vf = data;

	vf_refresh(vf);
	vf->refresh_idle_id = 0;
	return FALSE;
}

void vf_refresh_idle_cancel(ViewFile *vf)
{
	if (vf->refresh_idle_id)
		{
		g_source_remove(vf->refresh_idle_id);
		vf->refresh_idle_id = 0;
		}
}


void vf_refresh_idle(ViewFile *vf)
{
	if (!vf->refresh_idle_id)
		{
		vf->time_refresh_set = time(NULL);
		/* file operations run with G_PRIORITY_DEFAULT_IDLE */
		vf->refresh_idle_id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE + 50, vf_refresh_idle_cb, vf, NULL);
		}
	else if (time(NULL) - vf->time_refresh_set > 1)
		{
		/* more than 1 sec since last update - increase priority */
		vf_refresh_idle_cancel(vf);
		vf->time_refresh_set = time(NULL);
		vf->refresh_idle_id = g_idle_add_full(G_PRIORITY_DEFAULT_IDLE - 50, vf_refresh_idle_cb, vf, NULL);
		}
}

void vf_notify_cb(FileData *fd, NotifyType type, gpointer data)
{
	ViewFile *vf = data;
	gboolean refresh;

	NotifyType interested = NOTIFY_CHANGE | NOTIFY_REREAD | NOTIFY_GROUPING;
	if (vf->marks_enabled) interested |= NOTIFY_MARKS | NOTIFY_METADATA;
	/** @FIXME NOTIFY_METADATA should be checked by the keyword-to-mark functions and converted to NOTIFY_MARKS only if there was a change */

	if (!(type & interested) || vf->refresh_idle_id || !vf->dir_fd) return;

	refresh = (fd == vf->dir_fd);

	if (!refresh)
		{
		gchar *base = remove_level_from_path(fd->path);
		refresh = (g_strcmp0(base, vf->dir_fd->path) == 0);
		g_free(base);
		}

	if ((type & NOTIFY_CHANGE) && fd->change)
		{
		if (!refresh && fd->change->dest)
			{
			gchar *dest_base = remove_level_from_path(fd->change->dest);
			refresh = (g_strcmp0(dest_base, vf->dir_fd->path) == 0);
			g_free(dest_base);
			}

		if (!refresh && fd->change->source)
			{
			gchar *source_base = remove_level_from_path(fd->change->source);
			refresh = (g_strcmp0(source_base, vf->dir_fd->path) == 0);
			g_free(source_base);
			}
		}

	if (refresh)
		{
		DEBUG_1("Notify vf: %s %04x", fd->path, type);
		vf_refresh_idle(vf);
		}
}

static gboolean vf_read_metadata_in_idle_cb(gpointer data)
{
	FileData *fd;
	ViewFile *vf = data;
	GList *work;

	vf_thumb_status(vf, vf_read_metadata_in_idle_progress(vf), _("Loading meta..."));

	work = vf->list;

	while (work)
		{
		fd = work->data;

		if (fd && !fd->metadata_in_idle_loaded)
			{
			if (!fd->exifdate)
				{
				read_exif_time_data(fd);
				}
			if (!fd->exifdate_digitized)
				{
				read_exif_time_digitized_data(fd);
				}
			if (fd->rating == STAR_RATING_NOT_READ)
				{
				read_rating_data(fd);
				}
			fd->metadata_in_idle_loaded = TRUE;
			return TRUE;
			}
		work = work->next;
		}

	vf_thumb_status(vf, 0.0, NULL);
	vf->read_metadata_in_idle_id = 0;
	vf_refresh(vf);
	return FALSE;
}

static void vf_read_metadata_in_idle_finished_cb(gpointer data)
{
	ViewFile *vf = data;

	vf_thumb_status(vf, 0.0, "Loading meta...");
	vf->read_metadata_in_idle_id = 0;
}

void vf_read_metadata_in_idle(ViewFile *vf)
{
	if (!vf) return;

	if (vf->read_metadata_in_idle_id)
		{
		g_idle_remove_by_data(vf);
		}
	vf->read_metadata_in_idle_id = 0;

	if (vf->list)
		{
		vf->read_metadata_in_idle_id = g_idle_add_full(G_PRIORITY_LOW, vf_read_metadata_in_idle_cb, vf, vf_read_metadata_in_idle_finished_cb);
		}

	return;
}

/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
