/*
 * Copyright (C) 2004 John Ellis
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
#include "collect-table.h"

#include "cellrenderericon.h"
#include "collect-dlg.h"
#include "collect-io.h"
#include "dnd.h"
#include "dupe.h"
#include "editors.h"
#include "filedata.h"
#include "img-view.h"
#include "layout.h"
#include "layout-image.h"
#include "menu.h"
#include "metadata.h"
#include "pixbuf-util.h"
#include "print.h"
#include "utilops.h"
#include "ui-fileops.h"
#include "ui-menu.h"
#include "ui-tree-edit.h"
#include "uri-utils.h"
#include "view-file.h"

#include "icons/marker.xpm"
#define MARKER_WIDTH 26
#define MARKER_HEIGHT 32

#include <gdk/gdkkeysyms.h> /* for keyboard values */

/* between these, the icon width is increased by thumb_max_width / 2 */
#define THUMB_MIN_ICON_WIDTH 128
#define THUMB_MAX_ICON_WIDTH 150

#define COLLECT_TABLE_MAX_COLUMNS 32
#define THUMB_BORDER_PADDING 2

#define COLLECT_TABLE_TIP_DELAY 500
#define COLLECT_TABLE_TIP_DELAY_PATH (COLLECT_TABLE_TIP_DELAY * 1.7)


enum {
	CTABLE_COLUMN_POINTER = 0,
	CTABLE_COLUMN_COUNT
};

#define INFO_SELECTED(x) (x->flag_mask & SELECTION_SELECTED)


static void collection_table_populate_at_new_size(CollectTable *ct, gint w, gint h, gboolean force);

/**
 * This array must be kept in sync with the contents of:\n
 * @link collection_table_press_key_cb @endlink \n
 * @link collection_window_keypress @endlink \n
 * @link collection_table_popup_menu @endlink
 *
 * See also @link hard_coded_window_keys @endlink
 **/
hard_coded_window_keys collection_window_keys[] = {
	{GDK_CONTROL_MASK, 'C', N_("Copy")},
	{GDK_CONTROL_MASK, 'M', N_("Move")},
	{GDK_CONTROL_MASK, 'R', N_("Rename")},
	{GDK_CONTROL_MASK, 'D', N_("Move to Trash")},
	{GDK_CONTROL_MASK, 'W', N_("Close window")},
	{0, GDK_KEY_Delete, N_("Remove")},
	{0, GDK_KEY_Return, N_("View")},
	{0, 'V', N_("View in new window")},
	{GDK_CONTROL_MASK, 'A', N_("Select all")},
	{GDK_CONTROL_MASK + GDK_SHIFT_MASK, 'A', N_("Select none")},
	{GDK_MOD1_MASK, 'R', N_("Rectangular selection")},
	{0, GDK_KEY_space, N_("Select single file")},
	{GDK_CONTROL_MASK, GDK_KEY_space, N_("Toggle select image")},
	{GDK_CONTROL_MASK, 'L', N_("Append from file selection")},
	{0, 'A', N_("Append from collection")},
	{0, 'S', N_("Save collection")},
	{GDK_CONTROL_MASK, 'S', N_("Save collection as")},
	{GDK_CONTROL_MASK, 'T', N_("Show filename text")},
	{0, 'N', N_("Sort by name")},
	{0, 'I', N_("Sort by number")},
	{0, 'D', N_("Sort by date")},
	{0, 'B', N_("Sort by size")},
	{0, 'P', N_("Sort by path")},
	{GDK_SHIFT_MASK, 'P', N_("Print")},
	{0, 0, NULL}
};

/*
 *-------------------------------------------------------------------
 * more misc
 *-------------------------------------------------------------------
 */

static gboolean collection_table_find_position(CollectTable *ct, CollectInfo *info, gint *row, gint *col)
{
	gint n;

	n = g_list_index(ct->cd->list, info);

	if (n < 0) return FALSE;

	*row = n / ct->columns;
	*col = n - (*row * ct->columns);

	return TRUE;
}

static gboolean collection_table_find_iter(CollectTable *ct, CollectInfo *info, GtkTreeIter *iter, gint *column)
{
	GtkTreeModel *store;
	gint row, col;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));
	if (!collection_table_find_position(ct, info, &row, &col)) return FALSE;
	if (!gtk_tree_model_iter_nth_child(store, iter, NULL, row)) return FALSE;
	if (column) *column = col;

	return TRUE;
}

static CollectInfo *collection_table_find_data(CollectTable *ct, gint row, gint col, GtkTreeIter *iter)
{
	GtkTreeModel *store;
	GtkTreeIter p;

	if (row < 0 || col < 0) return NULL;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));
	if (gtk_tree_model_iter_nth_child(store, &p, NULL, row))
		{
		GList *list;

		gtk_tree_model_get(store, &p, CTABLE_COLUMN_POINTER, &list, -1);
		if (!list) return NULL;

		if (iter) *iter = p;

		return g_list_nth_data(list, col);
		}

	return NULL;
}

static CollectInfo *collection_table_find_data_by_coord(CollectTable *ct, gint x, gint y, GtkTreeIter *iter)
{
	GtkTreePath *tpath;
	GtkTreeViewColumn *column;
	GtkTreeModel *store;
	GtkTreeIter row;
	GList *list;
	gint n;

	if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(ct->listview), x, y,
					   &tpath, &column, NULL, NULL))
		return NULL;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));
	gtk_tree_model_get_iter(store, &row, tpath);
	gtk_tree_path_free(tpath);

	gtk_tree_model_get(store, &row, CTABLE_COLUMN_POINTER, &list, -1);
	if (!list) return NULL;

	n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), "column_number"));
	if (iter) *iter = row;
	return g_list_nth_data(list, n);
}

static guint collection_table_list_count(CollectTable *ct, gint64 *bytes)
{
	if (bytes)
		{
		gint64 b = 0;
		GList *work;

		work = ct->cd->list;
		while (work)
			{
			CollectInfo *ci = work->data;
			work = work->next;
			b += ci->fd->size;
			}

		*bytes = b;
		}

	return g_list_length(ct->cd->list);
}

static guint collection_table_selection_count(CollectTable *ct, gint64 *bytes)
{
	if (bytes)
		{
		gint64 b = 0;
		GList *work;

		work = ct->selection;
		while (work)
			{
			CollectInfo *ci = work->data;
			work = work->next;
			b += ci->fd->size;
			}

		*bytes = b;
		}

	return g_list_length(ct->selection);
}

static void collection_table_update_status(CollectTable *ct)
{
	gchar *buf;
	guint n;
	gint64 n_bytes = 0;
	guint s;
	gint64 s_bytes = 0;

	if (!ct->status_label) return;

	n = collection_table_list_count(ct, &n_bytes);
	s = collection_table_selection_count(ct, &s_bytes);

	if (s > 0)
		{
		gchar *b = text_from_size_abrev(n_bytes);
		gchar *sb = text_from_size_abrev(s_bytes);
		buf = g_strdup_printf(_("%s, %d images (%s, %d)"), b, n, sb, s);
		g_free(b);
		g_free(sb);
		}
	else if (n > 0)
		{
		gchar *b = text_from_size_abrev(n_bytes);
		buf = g_strdup_printf(_("%s, %d images"), b, n);
		g_free(b);
		}
	else
		{
		buf = g_strdup(_("Empty"));
		}

	gtk_label_set_text(GTK_LABEL(ct->status_label), buf);
	g_free(buf);
}

static void collection_table_update_extras(CollectTable *ct, gboolean loading, gdouble value)
{
	gchar *text;

	if (!ct->extra_label) return;

	if (loading)
		text = _("Loading thumbs...");
	else
		text = " ";

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ct->extra_label), value);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ct->extra_label), text);
}

static void collection_table_toggle_filenames(CollectTable *ct)
{
	GtkAllocation allocation;
	ct->show_text = !ct->show_text;
	options->show_icon_names = ct->show_text;

	gtk_widget_get_allocation(ct->listview, &allocation);
	collection_table_populate_at_new_size(ct, allocation.width, allocation.height, TRUE);
}

static void collection_table_toggle_stars(CollectTable *ct)
{
	GtkAllocation allocation;
	ct->show_stars = !ct->show_stars;
	options->show_star_rating = ct->show_stars;

	gtk_widget_get_allocation(ct->listview, &allocation);
	collection_table_populate_at_new_size(ct, allocation.width, allocation.height, TRUE);
}

static gint collection_table_get_icon_width(CollectTable *ct)
{
	gint width;

	if (!ct->show_text) return options->thumbnails.max_width;

	width = options->thumbnails.max_width + options->thumbnails.max_width / 2;
	if (width < THUMB_MIN_ICON_WIDTH) width = THUMB_MIN_ICON_WIDTH;
	if (width > THUMB_MAX_ICON_WIDTH) width = options->thumbnails.max_width;

	return width;
}

/*
 *-------------------------------------------------------------------
 * cell updates
 *-------------------------------------------------------------------
 */

static void collection_table_selection_set(CollectTable *ct, CollectInfo *info, SelectionType value, GtkTreeIter *iter)
{
	GtkTreeModel *store;
	GList *list;

	if (!info) return;

	if (info->flag_mask == value) return;
	info->flag_mask = value;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));
	if (iter)
		{
		gtk_tree_model_get(store, iter, CTABLE_COLUMN_POINTER, &list, -1);
		if (list) gtk_list_store_set(GTK_LIST_STORE(store), iter, CTABLE_COLUMN_POINTER, list, -1);
		}
	else
		{
		GtkTreeIter row;

		if (collection_table_find_iter(ct, info, &row, NULL))
			{
			gtk_tree_model_get(store, &row, CTABLE_COLUMN_POINTER, &list, -1);
			if (list) gtk_list_store_set(GTK_LIST_STORE(store), &row, CTABLE_COLUMN_POINTER, list, -1);
			}
		}
}

static void collection_table_selection_add(CollectTable *ct, CollectInfo *info, SelectionType mask, GtkTreeIter *iter)
{
	if (!info) return;

	collection_table_selection_set(ct, info, info->flag_mask | mask, iter);
}

static void collection_table_selection_remove(CollectTable *ct, CollectInfo *info, SelectionType mask, GtkTreeIter *iter)
{
	if (!info) return;

	collection_table_selection_set(ct, info, info->flag_mask & ~mask, iter);
}
/*
 *-------------------------------------------------------------------
 * selections
 *-------------------------------------------------------------------
 */

static void collection_table_verify_selections(CollectTable *ct)
{
	GList *work;

	work = ct->selection;
	while (work)
		{
		CollectInfo *info = work->data;
		work = work->next;
		if (!g_list_find(ct->cd->list, info))
			{
			ct->selection = g_list_remove(ct->selection, info);
			}
		}
}

void collection_table_select_all(CollectTable *ct)
{
	GList *work;

	g_list_free(ct->selection);
	ct->selection = NULL;

	work = ct->cd->list;
	while (work)
		{
		ct->selection = g_list_append(ct->selection, work->data);
		collection_table_selection_add(ct, work->data, SELECTION_SELECTED, NULL);
		work = work->next;
		}

	collection_table_update_status(ct);
}

void collection_table_unselect_all(CollectTable *ct)
{
	GList *work;

	work = ct->selection;
	while (work)
		{
		collection_table_selection_remove(ct, work->data, SELECTION_SELECTED, NULL);
		work = work->next;
		}

	g_list_free(ct->selection);
	ct->selection = NULL;

	collection_table_update_status(ct);
}

/* Invert the current collection's selection */
static void collection_table_select_invert_all(CollectTable *ct)
{
	GList *work;
	GList *new_selection = NULL;

	work = ct->cd->list;
	while (work)
		{
		CollectInfo *info = work->data;

		if (INFO_SELECTED(info))
			{
			collection_table_selection_remove(ct, info, SELECTION_SELECTED, NULL);
			}
		else
			{
			new_selection = g_list_append(new_selection, info);
			collection_table_selection_add(ct, info, SELECTION_SELECTED, NULL);

			}

		work = work->next;
		}

	g_list_free(ct->selection);
	ct->selection = new_selection;

	collection_table_update_status(ct);
}

void collection_table_select(CollectTable *ct, CollectInfo *info)
{
	ct->prev_selection = info;

	if (!info || INFO_SELECTED(info)) return;

	ct->selection = g_list_append(ct->selection, info);
	collection_table_selection_add(ct, info, SELECTION_SELECTED, NULL);

	collection_table_update_status(ct);
}

static void collection_table_unselect(CollectTable *ct, CollectInfo *info)
{
	ct->prev_selection = info;

	if (!info || !INFO_SELECTED(info) ) return;

	ct->selection = g_list_remove(ct->selection, info);
	collection_table_selection_remove(ct, info, SELECTION_SELECTED, NULL);

	collection_table_update_status(ct);
}

static void collection_table_select_util(CollectTable *ct, CollectInfo *info, gboolean select)
{
	if (select)
		{
		collection_table_select(ct, info);
		}
	else
		{
		collection_table_unselect(ct, info);
		}
}

static void collection_table_select_region_util(CollectTable *ct, CollectInfo *start, CollectInfo *end, gboolean select)
{
	gint row1, col1;
	gint row2, col2;
	gint t;
	gint i, j;

	if (!collection_table_find_position(ct, start, &row1, &col1) ||
	    !collection_table_find_position(ct, end, &row2, &col2) ) return;

	ct->prev_selection = end;

	if (!options->collections.rectangular_selection)
		{
		GList *work;
		CollectInfo *info;

		if (g_list_index(ct->cd->list, start) > g_list_index(ct->cd->list, end))
			{
			info = start;
			start = end;
			end = info;
			}

		work = g_list_find(ct->cd->list, start);
		while (work)
			{
			info = work->data;
			collection_table_select_util(ct, info, select);

			if (work->data != end)
				work = work->next;
			else
				work = NULL;
			}
		return;
		}

	if (row2 < row1)
		{
		t = row1;
		row1 = row2;
		row2 = t;
		}
	if (col2 < col1)
		{
		t = col1;
		col1 = col2;
		col2 = t;
		}

	DEBUG_1("table: %d x %d to %d x %d", row1, col1, row2, col2);

	for (i = row1; i <= row2; i++)
		{
		for (j = col1; j <= col2; j++)
			{
			CollectInfo *info = collection_table_find_data(ct, i, j, NULL);
			if (info) collection_table_select_util(ct, info, select);
			}
		}
}

GList *collection_table_selection_get_list(CollectTable *ct)
{
	return collection_list_to_filelist(ct->selection);
}

static GList *collection_table_get_list(CollectTable *ct)
{
	return collection_list_to_filelist(ct->cd->list);
}

/*
 *-------------------------------------------------------------------
 * tooltip type window
 *-------------------------------------------------------------------
 */

static void tip_show(CollectTable *ct)
{
	GtkWidget *label;
	gint x, y;
	GdkDisplay *display;
	GdkDeviceManager *device_manager;
	GdkDevice *device;

	if (ct->tip_window) return;

	device_manager = gdk_display_get_device_manager(gdk_window_get_display(
								gtk_widget_get_window(ct->listview)));
	device = gdk_device_manager_get_client_pointer(device_manager);
	gdk_window_get_device_position(gtk_widget_get_window(ct->listview),
								device, &x, &y, NULL);

	ct->tip_info = collection_table_find_data_by_coord(ct, x, y, NULL);
	if (!ct->tip_info) return;

	ct->tip_window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_resizable(GTK_WINDOW(ct->tip_window), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(ct->tip_window), 2);

	label = gtk_label_new(ct->show_text ? ct->tip_info->fd->path : ct->tip_info->fd->name);

	g_object_set_data(G_OBJECT(ct->tip_window), "tip_label", label);
	gtk_container_add(GTK_CONTAINER(ct->tip_window), label);
	gtk_widget_show(label);

	display = gdk_display_get_default();
	device_manager = gdk_display_get_device_manager(display);
	device = gdk_device_manager_get_client_pointer(device_manager);
	gdk_device_get_position(device, NULL, &x, &y);

	if (!gtk_widget_get_realized(ct->tip_window)) gtk_widget_realize(ct->tip_window);
	gtk_window_move(GTK_WINDOW(ct->tip_window), x + 16, y + 16);
	gtk_widget_show(ct->tip_window);
}

static void tip_hide(CollectTable *ct)
{
	if (ct->tip_window) gtk_widget_destroy(ct->tip_window);
	ct->tip_window = NULL;
}

static gboolean tip_schedule_cb(gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	if (!ct->tip_delay_id) return FALSE;

	tip_show(ct);

	ct->tip_delay_id = 0;
	return FALSE;
}

static void tip_schedule(CollectTable *ct)
{
	tip_hide(ct);

	if (ct->tip_delay_id)
		{
		g_source_remove(ct->tip_delay_id);
		ct->tip_delay_id = 0;
		}

	ct->tip_delay_id = g_timeout_add(ct->show_text ? COLLECT_TABLE_TIP_DELAY_PATH : COLLECT_TABLE_TIP_DELAY, tip_schedule_cb, ct);
}

static void tip_unschedule(CollectTable *ct)
{
	tip_hide(ct);

	if (ct->tip_delay_id)
		{
		g_source_remove(ct->tip_delay_id);
		ct->tip_delay_id = 0;
		}
}

static void tip_update(CollectTable *ct, CollectInfo *info)
{
	GdkDisplay *display = gdk_display_get_default();
	GdkDeviceManager *device_manager = gdk_display_get_device_manager(display);
	GdkDevice *device = gdk_device_manager_get_client_pointer(device_manager);

	tip_schedule(ct);

	if (ct->tip_window)
		{
		gint x, y;
		gdk_device_get_position(device, NULL, &x, &y);

		gtk_window_move(GTK_WINDOW(ct->tip_window), x + 16, y + 16);

		if (info != ct->tip_info)
			{
			GtkWidget *label;

			ct->tip_info = info;

			if (!ct->tip_info)
				{
				return;
				}

			label = g_object_get_data(G_OBJECT(ct->tip_window), "tip_label");
			gtk_label_set_text(GTK_LABEL(label), ct->show_text ? ct->tip_info->fd->path : ct->tip_info->fd->name);
			}
		}
}

/*
 *-------------------------------------------------------------------
 * popup menus
 *-------------------------------------------------------------------
 */

static void collection_table_popup_save_as_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_dialog_save_as(NULL, ct->cd);
}

static void collection_table_popup_save_cb(GtkWidget *widget, gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	if (!ct->cd->path)
		{
		collection_table_popup_save_as_cb(widget, data);
		return;
		}

	if (!collection_save(ct->cd, ct->cd->path))
		{
		log_printf("failed saving to collection path: %s\n", ct->cd->path);
		}
}

static GList *collection_table_popup_file_list(CollectTable *ct)
{
	if (!ct->click_info) return NULL;

	if (INFO_SELECTED(ct->click_info))
		{
		return collection_table_selection_get_list(ct);
		}

	return g_list_append(NULL, file_data_ref(ct->click_info->fd));
}

static void collection_table_popup_edit_cb(GtkWidget *widget, gpointer data)
{
	CollectTable *ct;
	const gchar *key = (gchar*)data;

	ct = submenu_item_get_data(widget);

	if (!ct) return;

	file_util_start_editor_from_filelist(key, collection_table_popup_file_list(ct), NULL, ct->listview);
}

static void collection_table_popup_copy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	file_util_copy(NULL, collection_table_popup_file_list(ct), NULL, ct->listview);
}

static void collection_table_popup_move_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	file_util_move(NULL, collection_table_popup_file_list(ct), NULL, ct->listview);
}

static void collection_table_popup_rename_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	file_util_rename(NULL, collection_table_popup_file_list(ct), ct->listview);
}

static void collection_table_popup_delete_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	options->file_ops.safe_delete_enable = FALSE;
	file_util_delete(NULL, collection_table_popup_file_list(ct), ct->listview);
}

static void collection_table_popup_move_to_trash_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	options->file_ops.safe_delete_enable = TRUE;
	file_util_delete(NULL, collection_table_popup_file_list(ct), ct->listview);
}

static void collection_table_popup_copy_path_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	file_util_copy_path_list_to_clipboard(collection_table_popup_file_list(ct), TRUE);
}

static void collection_table_popup_copy_path_unquoted_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	file_util_copy_path_list_to_clipboard(collection_table_popup_file_list(ct), FALSE);
}

static void collection_table_popup_sort_cb(GtkWidget *widget, gpointer data)
{
	CollectTable *ct;
	SortType type;

	ct = submenu_item_get_data(widget);

	if (!ct) return;

	type = (SortType)GPOINTER_TO_INT(data);

	collection_set_sort_method(ct->cd, type);
}

static void collection_table_popup_randomize_cb(GtkWidget *widget, gpointer UNUSED(data))
{
	CollectTable *ct;

	ct = submenu_item_get_data(widget);

	if (!ct) return;

	collection_randomize(ct->cd);
}

static void collection_table_popup_view_new_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	if (ct->click_info && g_list_find(ct->cd->list, ct->click_info))
		{
		view_window_new_from_collection(ct->cd, ct->click_info);
		}
}

static void collection_table_popup_view_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	if (ct->click_info && g_list_find(ct->cd->list, ct->click_info))
		{
		layout_image_set_collection(NULL, ct->cd, ct->click_info);
		}
}

static void collection_table_popup_selectall_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_select_all(ct);
	ct->prev_selection= ct->click_info;
}

static void collection_table_popup_unselectall_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_unselect_all(ct);
	ct->prev_selection= ct->click_info;
}

static void collection_table_popup_select_invert_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_select_invert_all(ct);
	ct->prev_selection= ct->click_info;
}

static void collection_table_popup_rectangular_selection_cb(GtkWidget *UNUSED(widget), gpointer UNUSED(data))
{
	options->collections.rectangular_selection = !(options->collections.rectangular_selection);
}

static void collection_table_popup_remove_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	GList *list;

	if (!ct->click_info) return;

	if (INFO_SELECTED(ct->click_info))
		{
		list = g_list_copy(ct->selection);
		}
	else
		{
		list = g_list_append(NULL, ct->click_info);
		}

	collection_remove_by_info_list(ct->cd, list);
	g_list_free(list);
}

static void collection_table_popup_add_file_selection_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	GList *list;
	LayoutWindow *lw = NULL;

	if (!layout_valid(&lw)) return;

	list = vf_selection_get_list(lw->vf);

	if (list)
		{
		collection_table_add_filelist(ct, list);
		filelist_free(list);
		}
}

static void collection_table_popup_add_collection_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_dialog_append(NULL, ct->cd);
}

static void collection_table_popup_goto_original_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	GList *list;
	LayoutWindow *lw = NULL;
	FileData *fd;

	if (!layout_valid(&lw)) return;
	list = collection_table_selection_get_list(ct);
	if (list)
		{
		fd = list->data;
		if (fd)
			{
			layout_set_fd(lw, fd);
			}
		}
	g_list_free(list);
}

static void collection_table_popup_find_dupes_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	DupeWindow *dw;

	dw = dupe_window_new();
	dupe_window_add_collection(dw, ct->cd);
}

static void collection_table_popup_print_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	FileData *fd;

	fd = (ct->click_info) ? ct->click_info->fd : NULL;

	print_window_new(fd, collection_table_selection_get_list(ct), collection_table_get_list(ct), gtk_widget_get_toplevel(ct->listview));
}

static void collection_table_popup_show_names_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_toggle_filenames(ct);
}

static void collection_table_popup_show_stars_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_toggle_stars(ct);
}

static void collection_table_popup_destroy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_selection_remove(ct, ct->click_info, SELECTION_PRELIGHT, NULL);
	ct->click_info = NULL;
	ct->popup = NULL;

	filelist_free(ct->drop_list);
	ct->drop_list = NULL;
	ct->drop_info = NULL;

	filelist_free(ct->editmenu_fd_list);
	ct->editmenu_fd_list = NULL;
}

static GtkWidget *collection_table_popup_menu(CollectTable *ct, gboolean over_icon)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *submenu;
 	GtkAccelGroup *accel_group;

	menu = popup_menu_short_lived();

	accel_group = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);

	g_object_set_data(G_OBJECT(menu), "window_keys", collection_window_keys);
	g_object_set_data(G_OBJECT(menu), "accel_group", accel_group);

	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(collection_table_popup_destroy_cb), ct);

	menu_item_add_sensitive(menu, _("_View"), over_icon,
			G_CALLBACK(collection_table_popup_view_cb), ct);
	menu_item_add_stock_sensitive(menu, _("View in _new window"), GTK_STOCK_NEW, over_icon,
			G_CALLBACK(collection_table_popup_view_new_cb), ct);
	menu_item_add_stock(menu, _("Go to original"), GTK_STOCK_FIND,
			G_CALLBACK(collection_table_popup_goto_original_cb), ct);
	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu, _("Rem_ove"), GTK_STOCK_REMOVE, over_icon,
			G_CALLBACK(collection_table_popup_remove_cb), ct);

	menu_item_add_stock(menu, _("Append from file selection"), GTK_STOCK_ADD,
			G_CALLBACK(collection_table_popup_add_file_selection_cb), ct);
	menu_item_add_stock(menu, _("Append from collection..."), GTK_STOCK_OPEN,
			G_CALLBACK(collection_table_popup_add_collection_cb), ct);
	menu_item_add_divider(menu);

	item = menu_item_add(menu, _("_Selection"), NULL, NULL);
	submenu = gtk_menu_new();
	menu_item_add(submenu, _("Select all"),
			G_CALLBACK(collection_table_popup_selectall_cb), ct);
	menu_item_add(submenu, _("Select none"),
			G_CALLBACK(collection_table_popup_unselectall_cb), ct);
	menu_item_add(submenu, _("Invert selection"),
			G_CALLBACK(collection_table_popup_select_invert_cb), ct);
	menu_item_add_check(submenu, _("Rectangular selection"), (options->collections.rectangular_selection != FALSE),
			G_CALLBACK(collection_table_popup_rectangular_selection_cb), ct);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
	menu_item_add_divider(menu);


	ct->editmenu_fd_list = collection_table_selection_get_list(ct);
	submenu_add_edit(menu, &item,
			G_CALLBACK(collection_table_popup_edit_cb), ct, ct->editmenu_fd_list);
	gtk_widget_set_sensitive(item, over_icon);

	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu, _("_Copy..."), GTK_STOCK_COPY, over_icon,
			G_CALLBACK(collection_table_popup_copy_cb), ct);
	menu_item_add_sensitive(menu, _("_Move..."), over_icon,
			G_CALLBACK(collection_table_popup_move_cb), ct);
	menu_item_add_sensitive(menu, _("_Rename..."), over_icon,
			G_CALLBACK(collection_table_popup_rename_cb), ct);
	menu_item_add_sensitive(menu, _("_Copy path"), over_icon,
				G_CALLBACK(collection_table_popup_copy_path_cb), ct);
	menu_item_add_sensitive(menu, _("_Copy path unquoted"), over_icon,
				G_CALLBACK(collection_table_popup_copy_path_unquoted_cb), ct);

	menu_item_add_divider(menu);
	menu_item_add_stock_sensitive(menu,
				options->file_ops.confirm_move_to_trash ? _("Move to Trash...") :
					_("Move to Trash"), PIXBUF_INLINE_ICON_TRASH, over_icon,
				G_CALLBACK(collection_table_popup_move_to_trash_cb), ct);
	menu_item_add_stock_sensitive(menu,
				options->file_ops.confirm_delete ? _("_Delete...") :
					_("_Delete"), GTK_STOCK_DELETE, over_icon,
				G_CALLBACK(collection_table_popup_delete_cb), ct);

	menu_item_add_divider(menu);
	submenu = submenu_add_sort(NULL, G_CALLBACK(collection_table_popup_sort_cb), ct, FALSE, TRUE, FALSE, 0);
	menu_item_add_divider(submenu);
	menu_item_add(submenu, _("Randomize"),
			G_CALLBACK(collection_table_popup_randomize_cb), ct);
	item = menu_item_add(menu, _("_Sort"), NULL, NULL);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

	menu_item_add_check(menu, _("Show filename _text"), ct->show_text,
			G_CALLBACK(collection_table_popup_show_names_cb), ct);
	menu_item_add_check(menu, _("Show star rating"), ct->show_stars,
				G_CALLBACK(collection_table_popup_show_stars_cb), ct);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("_Save collection"), GTK_STOCK_SAVE,
			G_CALLBACK(collection_table_popup_save_cb), ct);
	menu_item_add_stock(menu, _("Save collection _as..."), GTK_STOCK_SAVE_AS,
			G_CALLBACK(collection_table_popup_save_as_cb), ct);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("_Find duplicates..."), GTK_STOCK_FIND,
			G_CALLBACK(collection_table_popup_find_dupes_cb), ct);
	menu_item_add_stock_sensitive(menu, _("Print..."), GTK_STOCK_PRINT, over_icon,
			G_CALLBACK(collection_table_popup_print_cb), ct);

	return menu;
}
/*
 *-------------------------------------------------------------------
 * keyboard callbacks
 *-------------------------------------------------------------------
 */

void collection_table_set_focus(CollectTable *ct, CollectInfo *info)
{
	GtkTreeIter iter;
	gint row, col;

	if (g_list_find(ct->cd->list, ct->focus_info))
		{
		if (info == ct->focus_info)
			{
			/* ensure focus row col are correct */
			collection_table_find_position(ct, ct->focus_info,
						       &ct->focus_row, &ct->focus_column);
			return;
			}
		collection_table_selection_remove(ct, ct->focus_info, SELECTION_FOCUS, NULL);
		}

	if (!collection_table_find_position(ct, info, &row, &col))
		{
		ct->focus_info = NULL;
		ct->focus_row = -1;
		ct->focus_column = -1;
		return;
		}

	ct->focus_info = info;
	ct->focus_row = row;
	ct->focus_column = col;
	collection_table_selection_add(ct, ct->focus_info, SELECTION_FOCUS, NULL);

	if (collection_table_find_iter(ct, ct->focus_info, &iter, NULL))
		{
		GtkTreePath *tpath;
		GtkTreeViewColumn *column;
		GtkTreeModel *store;

		tree_view_row_make_visible(GTK_TREE_VIEW(ct->listview), &iter, FALSE);

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));
		tpath = gtk_tree_model_get_path(store, &iter);
		/* focus is set to an extra column with 0 width to hide focus, we draw it ourself */
		column = gtk_tree_view_get_column(GTK_TREE_VIEW(ct->listview), COLLECT_TABLE_MAX_COLUMNS);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(ct->listview), tpath, column, FALSE);
		gtk_tree_path_free(tpath);
		}
}

static void collection_table_move_focus(CollectTable *ct, gint row, gint col, gboolean relative)
{
	gint new_row;
	gint new_col;

	if (relative)
		{
		new_row = ct->focus_row;
		new_col = ct->focus_column;

		new_row += row;
		if (new_row < 0) new_row = 0;
		if (new_row >= ct->rows) new_row = ct->rows - 1;

		while (col != 0)
			{
			if (col < 0)
				{
				new_col--;
				col++;
				}
			else
				{
				new_col++;
				col--;
				}

			if (new_col < 0)
				{
				if (new_row > 0)
					{
					new_row--;
					new_col = ct->columns - 1;
					}
				else
					{
					new_col = 0;
					}
				}
			if (new_col >= ct->columns)
				{
				if (new_row < ct->rows - 1)
					{
					new_row++;
					new_col = 0;
					}
				else
					{
					new_col = ct->columns - 1;
					}
				}
			}
		}
	else
		{
		new_row = row;
		new_col = col;

		if (new_row >= ct->rows)
			{
			if (ct->rows > 0)
				new_row = ct->rows - 1;
			else
				new_row = 0;
			new_col = ct->columns - 1;
			}
		if (new_col >= ct->columns) new_col = ct->columns - 1;
		}

	if (new_row == ct->rows - 1)
		{
		gint l;

		/* if we moved beyond the last image, go to the last image */

		l = g_list_length(ct->cd->list);
		if (ct->rows > 1) l -= (ct->rows - 1) * ct->columns;
		if (new_col >= l) new_col = l - 1;
		}

	if (new_row == -1 || new_col == -1)
		{
		if (!ct->cd->list) return;
		new_row = new_col = 0;
		}

	collection_table_set_focus(ct, collection_table_find_data(ct, new_row, new_col, NULL));
}

static void collection_table_update_focus(CollectTable *ct)
{
	gint new_row = 0;
	gint new_col = 0;

	if (ct->focus_info && collection_table_find_position(ct, ct->focus_info, &new_row, &new_col))
		{
		/* first find the old focus, if it exists and is valid */
		}
	else
		{
		/* (try to) stay where we were */
		new_row = ct->focus_row;
		new_col = ct->focus_column;
		}

	collection_table_move_focus(ct, new_row, new_col, FALSE);
}

/* used to figure the page up/down distances */
static gint page_height(CollectTable *ct)
{
	GtkAdjustment *adj;
	gint page_size;
	gint row_height;
	gint ret;

	adj = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ct->listview));
	page_size = (gint)gtk_adjustment_get_page_increment(adj);

	row_height = options->thumbnails.max_height + THUMB_BORDER_PADDING * 2;
	if (ct->show_text) row_height += options->thumbnails.max_height / 3;

	ret = page_size / row_height;
	if (ret < 1) ret = 1;

	return ret;
}

static void collection_table_menu_pos_cb(GtkMenu *menu, gint *x, gint *y, gboolean *UNUSED(push_in), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	GtkTreeModel *store;
	GtkTreeIter iter;
	gint column;
	GtkTreePath *tpath;
	gint cw, ch;

	if (!collection_table_find_iter(ct, ct->click_info, &iter, &column)) return;
	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));
	tpath = gtk_tree_model_get_path(store, &iter);
	tree_view_get_cell_clamped(GTK_TREE_VIEW(ct->listview), tpath, column, FALSE, x, y, &cw, &ch);
	gtk_tree_path_free(tpath);
	*y += ch;
	popup_menu_position_clamp(menu, x, y, 0);
}

static gboolean collection_table_press_key_cb(GtkWidget *UNUSED(widget), GdkEventKey *event, gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	gint focus_row = 0;
	gint focus_col = 0;
	CollectInfo *info;
	gboolean stop_signal = TRUE;

	switch (event->keyval)
		{
		case GDK_KEY_Left: case GDK_KEY_KP_Left:
			focus_col = -1;
			break;
		case GDK_KEY_Right: case GDK_KEY_KP_Right:
			focus_col = 1;
			break;
		case GDK_KEY_Up: case GDK_KEY_KP_Up:
			focus_row = -1;
			break;
		case GDK_KEY_Down: case GDK_KEY_KP_Down:
			focus_row = 1;
			break;
		case GDK_KEY_Page_Up: case GDK_KEY_KP_Page_Up:
			focus_row = -page_height(ct);
			break;
		case GDK_KEY_Page_Down: case GDK_KEY_KP_Page_Down:
			focus_row = page_height(ct);
			break;
		case GDK_KEY_Home: case GDK_KEY_KP_Home:
			focus_row = -ct->focus_row;
			focus_col = -ct->focus_column;
			break;
		case GDK_KEY_End: case GDK_KEY_KP_End:
			focus_row = ct->rows - 1 - ct->focus_row;
			focus_col = ct->columns - 1 - ct->focus_column;
			break;
		case GDK_KEY_space:
			info = collection_table_find_data(ct, ct->focus_row, ct->focus_column, NULL);
			if (info)
				{
				ct->click_info = info;
				if (event->state & GDK_CONTROL_MASK)
					{
					collection_table_select_util(ct, info, !INFO_SELECTED(info));
					}
				else
					{
					collection_table_unselect_all(ct);
					collection_table_select(ct, info);
					}
				}
			break;
		case 'T': case 't':
			if (event->state & GDK_CONTROL_MASK) collection_table_toggle_filenames(ct);
			break;
		case GDK_KEY_Menu:
		case GDK_KEY_F10:
			info = collection_table_find_data(ct, ct->focus_row, ct->focus_column, NULL);
			ct->click_info = info;

			collection_table_selection_add(ct, ct->click_info, SELECTION_PRELIGHT, NULL);
			tip_unschedule(ct);

			ct->popup = collection_table_popup_menu(ct, (info != NULL));
			gtk_menu_popup(GTK_MENU(ct->popup), NULL, NULL, collection_table_menu_pos_cb, ct, 0, GDK_CURRENT_TIME);
			break;
		default:
			stop_signal = FALSE;
			break;
		}

	if (focus_row != 0 || focus_col != 0)
		{
		CollectInfo *new_info;
		CollectInfo *old_info;

		old_info = collection_table_find_data(ct, ct->focus_row, ct->focus_column, NULL);
		collection_table_move_focus(ct, focus_row, focus_col, TRUE);
		new_info = collection_table_find_data(ct, ct->focus_row, ct->focus_column, NULL);

		if (new_info != old_info)
			{
			if (event->state & GDK_SHIFT_MASK)
				{
				if (!options->collections.rectangular_selection)
					{
					collection_table_select_region_util(ct, old_info, new_info, FALSE);
					}
				else
					{
					collection_table_select_region_util(ct, ct->click_info, old_info, FALSE);
					}
				collection_table_select_region_util(ct, ct->click_info, new_info, TRUE);
				}
			else if (event->state & GDK_CONTROL_MASK)
				{
				ct->click_info = new_info;
				}
			else
				{
				ct->click_info = new_info;
				collection_table_unselect_all(ct);
				collection_table_select(ct, new_info);
				}
			}
		}

	if (stop_signal)
		{
		tip_unschedule(ct);
		}

	return stop_signal;
}

/*
 *-------------------------------------------------------------------
 * insert marker
 *-------------------------------------------------------------------
 */

static CollectInfo *collection_table_insert_find(CollectTable *ct, CollectInfo *source, gboolean *after, GdkRectangle *cell,
						 gboolean use_coord, gint x, gint y)
{
	CollectInfo *info = NULL;
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkTreePath *tpath;
	GtkTreeViewColumn *column;
	GdkDeviceManager *device_manager;
	GdkDevice *device;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));

	if (!use_coord)
		{
		device_manager = gdk_display_get_device_manager(gdk_window_get_display(
									gtk_widget_get_window(ct->listview)));
		device = gdk_device_manager_get_client_pointer(device_manager);
		gdk_window_get_device_position(gtk_widget_get_window(ct->listview),
									device, &x, &y, NULL);
		}

	if (source)
		{
		gint col;
		if (collection_table_find_iter(ct, source, &iter, &col))
			{
			tpath = gtk_tree_model_get_path(store, &iter);
			column = gtk_tree_view_get_column(GTK_TREE_VIEW(ct->listview), col);
			gtk_tree_view_get_background_area(GTK_TREE_VIEW(ct->listview), tpath, column, cell);
			gtk_tree_path_free(tpath);

			info = source;
			*after = !!(x > cell->x + (cell->width / 2));
			}
		return info;
		}

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(ct->listview), x, y,
					  &tpath, &column, NULL, NULL))
		{
		GList *list;
		gint n;

		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, CTABLE_COLUMN_POINTER, &list, -1);

		n = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), "column_number"));
		info = g_list_nth_data(list, n);

		if (info)
			{
			gtk_tree_view_get_background_area(GTK_TREE_VIEW(ct->listview), tpath, column, cell);
			*after = !!(x > cell->x + (cell->width / 2));
			}

		gtk_tree_path_free(tpath);
		}

	if (info == NULL)
		{
		GList *work;

		work = g_list_last(ct->cd->list);
		if (work)
			{
			gint col;

			info = work->data;
			*after = TRUE;

			if (collection_table_find_iter(ct, info, &iter, &col))
				{
				tpath = gtk_tree_model_get_path(store, &iter);
				column = gtk_tree_view_get_column(GTK_TREE_VIEW(ct->listview), col);
				gtk_tree_view_get_background_area(GTK_TREE_VIEW(ct->listview), tpath, column, cell);
				gtk_tree_path_free(tpath);
				}
			}
		}

	return info;
}

static CollectInfo *collection_table_insert_point(CollectTable *ct, gint x, gint y)
{
	CollectInfo *info;
	GdkRectangle cell;
	gboolean after = FALSE;

	info = collection_table_insert_find(ct, NULL, &after, &cell, TRUE, x, y);

	if (info && after)
		{
		GList *work;

		work = g_list_find(ct->cd->list, info);
		if (work && work->next)
			{
			info = work->next->data;
			}
		else
			{
			info = NULL;
			}
		}

	return info;
}

static void collection_table_insert_marker(CollectTable *ct, CollectInfo *info, gboolean enable)
{
	gboolean after = FALSE;
	GdkRectangle cell;
	GdkWindow *parent;
	gint x_parent, y_parent;

	parent = gtk_widget_get_window(gtk_widget_get_toplevel(ct->listview));
	gdk_window_get_position(parent, &x_parent, &y_parent);

	if (!enable)
		{
		if (ct->marker_window) gdk_window_destroy(ct->marker_window);
		ct->marker_window = NULL;

		return;
		}

	info = collection_table_insert_find(ct, info, &after, &cell, FALSE, 0, 0);

	/* this setting does not take into account (after), but since it is not really used... */
	ct->marker_info = info;

	if (!ct->marker_window)
		{
		GdkWindowAttr attributes;
		gint attributes_mask;
		GdkPixbuf *pb = gdk_pixbuf_new_from_xpm_data((const gchar **)marker_xpm);
		gint w = gdk_pixbuf_get_width(pb);
		gint h = gdk_pixbuf_get_height(pb);

		attributes.window_type = GDK_WINDOW_CHILD;
		attributes.wclass = GDK_INPUT_OUTPUT;
		attributes.width = w;
		attributes.height = h;
		attributes.event_mask = gtk_widget_get_events(ct->listview);
		attributes_mask = 0;

		ct->marker_window = gdk_window_new(NULL, &attributes, attributes_mask);

		cairo_region_t *mask;
		cairo_pattern_t *pattern;
		cairo_surface_t *img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
		cairo_t *cr = cairo_create(img);
		gdk_cairo_set_source_pixbuf(cr, pb, 0, 0);
		cairo_paint(cr);
		/** @FIXME this is a hack to get the background color */
		//~ pattern = cairo_pattern_create_for_surface(img);
		pattern = cairo_pattern_create_rgb (1.0, 0.0, 0.0);
		mask = gdk_cairo_region_create_from_surface(img);
		gdk_window_shape_combine_region(ct->marker_window, mask, 0, 0);
		gdk_window_set_background_pattern(ct->marker_window, pattern);
		cairo_region_destroy(mask);
		cairo_pattern_destroy(pattern);
		cairo_destroy(cr);
		cairo_surface_destroy(img);
		g_object_unref(pb);
		}

	if (info)
		{
		gint x, y;
		gint w, h;

		w = gdk_window_get_width(ct->marker_window);
		h = gdk_window_get_height(ct->marker_window);

		if (!after)
			{
			x = cell.x;
			}
		else
			{
			x = cell.x + cell.width;
			}
		x -= (w / 2);
		y = cell.y + (cell.height / 2) - (h / 2);

		x = x + x_parent;
		y = y + y_parent;

		gdk_window_move(ct->marker_window, x, y);

		if (!gdk_window_is_visible(ct->marker_window)) gdk_window_show(ct->marker_window);
		}
	else
		{
		if (gdk_window_is_visible(ct->marker_window)) gdk_window_hide(ct->marker_window);
		}
}

/*
 *-------------------------------------------------------------------
 * mouse drag auto-scroll
 *-------------------------------------------------------------------
 */

static void collection_table_motion_update(CollectTable *ct, gint x, gint y, gboolean drop_event)
{
	CollectInfo *info;

	info = collection_table_find_data_by_coord(ct, x, y, NULL);

	if (drop_event)
		{
		tip_unschedule(ct);
		collection_table_insert_marker(ct, info, TRUE);
		}
	else
		{
		tip_update(ct, info);
		}
}

static gboolean collection_table_auto_scroll_idle_cb(gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	GdkWindow *window;
	gint x, y;
	gint w, h;
	GdkDeviceManager *device_manager;
	GdkDevice *device;

	if (!ct->drop_idle_id) return FALSE;

	window = gtk_widget_get_window(ct->listview);
	device_manager = gdk_display_get_device_manager(gdk_window_get_display(window));
	device = gdk_device_manager_get_client_pointer(device_manager);
	gdk_window_get_device_position(window, device, &x, &y, NULL);

	w = gdk_window_get_width(window);
	h = gdk_window_get_height(window);
	if (x >= 0 && x < w && y >= 0 && y < h)
		{
		collection_table_motion_update(ct, x, y, TRUE);
		}

	ct->drop_idle_id = 0;
	return FALSE;
}

static gboolean collection_table_auto_scroll_notify_cb(GtkWidget *UNUSED(widget), gint UNUSED(x), gint UNUSED(y), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	if (!ct->drop_idle_id)
		{
		ct->drop_idle_id = g_idle_add(collection_table_auto_scroll_idle_cb, ct);
		}

	return TRUE;
}

static void collection_table_scroll(CollectTable *ct, gboolean scroll)
{
	if (!scroll)
		{
		if (ct->drop_idle_id)
			{
			g_source_remove(ct->drop_idle_id);
			ct->drop_idle_id = 0;
			}
		widget_auto_scroll_stop(ct->listview);
		collection_table_insert_marker(ct, NULL, FALSE);
		}
	else
		{
		GtkAdjustment *adj = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(ct->listview));
		widget_auto_scroll_start(ct->listview, adj, -1, options->thumbnails.max_height / 2,
					 collection_table_auto_scroll_notify_cb, ct);
		}
}

/*
 *-------------------------------------------------------------------
 * mouse callbacks
 *-------------------------------------------------------------------
 */

static gboolean collection_table_motion_cb(GtkWidget *UNUSED(widget), GdkEventMotion *event, gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_motion_update(ct, (gint)event->x, (gint)event->y, FALSE);

	return FALSE;
}

static gboolean collection_table_press_cb(GtkWidget *UNUSED(widget), GdkEventButton *bevent, gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	GtkTreeIter iter;
	CollectInfo *info;

	tip_unschedule(ct);

	info = collection_table_find_data_by_coord(ct, (gint)bevent->x, (gint)bevent->y, &iter);

	ct->click_info = info;
	collection_table_selection_add(ct, ct->click_info, SELECTION_PRELIGHT, &iter);

	switch (bevent->button)
		{
		case MOUSE_BUTTON_LEFT:
			if (bevent->type == GDK_2BUTTON_PRESS)
				{
				if (info)
					{
					layout_image_set_collection(NULL, ct->cd, info);
					}
				}
			else if (!gtk_widget_has_focus(ct->listview))
				{
				gtk_widget_grab_focus(ct->listview);
				}
			break;
		case MOUSE_BUTTON_RIGHT:
			ct->popup = collection_table_popup_menu(ct, (info != NULL));
			gtk_menu_popup(GTK_MENU(ct->popup), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
			break;
		default:
			break;
		}

	return TRUE;
}

static gboolean collection_table_release_cb(GtkWidget *UNUSED(widget), GdkEventButton *bevent, gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	GtkTreeIter iter;
	CollectInfo *info = NULL;

	tip_schedule(ct);

	if ((gint)bevent->x != 0 || (gint)bevent->y != 0)
		{
		info = collection_table_find_data_by_coord(ct, (gint)bevent->x, (gint)bevent->y, &iter);
		}

	if (ct->click_info)
		{
		collection_table_selection_remove(ct, ct->click_info, SELECTION_PRELIGHT, NULL);
		}

	if (bevent->button == MOUSE_BUTTON_LEFT &&
	    info && ct->click_info == info)
		{
		collection_table_set_focus(ct, info);

		if (bevent->state & GDK_CONTROL_MASK)
			{
			gboolean select = !INFO_SELECTED(info);

			if ((bevent->state & GDK_SHIFT_MASK) && ct->prev_selection)
				{
				collection_table_select_region_util(ct, ct->prev_selection, info, select);
				}
			else
				{
				collection_table_select_util(ct, info, select);
				}
			}
		else
			{
			collection_table_unselect_all(ct);

			if ((bevent->state & GDK_SHIFT_MASK) &&
			    ct->prev_selection)
				{
				collection_table_select_region_util(ct, ct->prev_selection, info, TRUE);
				}
			else
				{
				collection_table_select_util(ct, info, TRUE);
				}
			}
		}
	else if (bevent->button == MOUSE_BUTTON_MIDDLE &&
		 info && ct->click_info == info)
		{
		collection_table_select_util(ct, info, !INFO_SELECTED(info));
		}

	return TRUE;
}

static gboolean collection_table_leave_cb(GtkWidget *UNUSED(widget), GdkEventCrossing *UNUSED(event), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	tip_unschedule(ct);
	return FALSE;
}

/*
 *-------------------------------------------------------------------
 * populate, add, insert, etc.
 *-------------------------------------------------------------------
 */

static gboolean collection_table_destroy_node_cb(GtkTreeModel *store, GtkTreePath *UNUSED(tpath), GtkTreeIter *iter, gpointer UNUSED(data))
{
	GList *list;

	gtk_tree_model_get(store, iter, CTABLE_COLUMN_POINTER, &list, -1);
	g_list_free(list);

	return FALSE;
}

static void collection_table_clear_store(CollectTable *ct)
{
	GtkTreeModel *store;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));
	gtk_tree_model_foreach(store, collection_table_destroy_node_cb, NULL);

	gtk_list_store_clear(GTK_LIST_STORE(store));
}

static GList *collection_table_add_row(CollectTable *ct, GtkTreeIter *iter)
{
	GtkListStore *store;
	GList *list = NULL;
	gint i;

	for (i = 0; i < ct->columns; i++) list = g_list_prepend(list, NULL);

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview)));
	gtk_list_store_append(store, iter);
	gtk_list_store_set(store, iter, CTABLE_COLUMN_POINTER, list, -1);

	return list;
}

static void collection_table_populate(CollectTable *ct, gboolean resize)
{
	gint row;
	GList *work;

	collection_table_verify_selections(ct);

	collection_table_clear_store(ct);

	if (resize)
		{
		gint i;
		gint thumb_width;

		thumb_width = collection_table_get_icon_width(ct);

		for (i = 0; i < COLLECT_TABLE_MAX_COLUMNS; i++)
			{
			GtkTreeViewColumn *column;
			GtkCellRenderer *cell;
			GList *list;

			column = gtk_tree_view_get_column(GTK_TREE_VIEW(ct->listview), i);
			gtk_tree_view_column_set_visible(column, (i < ct->columns));
			gtk_tree_view_column_set_fixed_width(column, thumb_width + (THUMB_BORDER_PADDING * 6));

			list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(column));
			cell = (list) ? list->data : NULL;
			g_list_free(list);

			if (cell && GQV_IS_CELL_RENDERER_ICON(cell))
				{
				g_object_set(G_OBJECT(cell), "fixed_width", thumb_width,
							     "fixed_height", options->thumbnails.max_height,
							     "show_text", ct->show_text || ct->show_stars, NULL);
				}
			}
		if (gtk_widget_get_realized(ct->listview)) gtk_tree_view_columns_autosize(GTK_TREE_VIEW(ct->listview));
		}

	row = -1;
	work = ct->cd->list;
	while (work)
		{
		GList *list;
		GtkTreeIter iter;

		row++;

		list = collection_table_add_row(ct, &iter);
		while (work && list)
			{
			list->data = work->data;
			list = list->next;
			work = work->next;
			}
		}

	ct->rows = row + 1;

	collection_table_update_focus(ct);
	collection_table_update_status(ct);
}

static void collection_table_populate_at_new_size(CollectTable *ct, gint w, gint UNUSED(h), gboolean force)
{
	gint new_cols;
	gint thumb_width;

	thumb_width = collection_table_get_icon_width(ct);

	new_cols = w / (thumb_width + (THUMB_BORDER_PADDING * 6));
	if (new_cols < 1) new_cols = 1;

	if (!force && new_cols == ct->columns) return;

	ct->columns = new_cols;

	collection_table_populate(ct, TRUE);

	DEBUG_1("col tab pop cols=%d rows=%d", ct->columns, ct->rows);
}

static void collection_table_sync(CollectTable *ct)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	GList *work;
	gint r, c;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));

	r = -1;
	c = 0;

	work = ct->cd->list;
	while (work)
		{
		GList *list;
		r++;
		c = 0;
		if (gtk_tree_model_iter_nth_child(store, &iter, NULL, r))
			{
			gtk_tree_model_get(store, &iter, CTABLE_COLUMN_POINTER, &list, -1);
			gtk_list_store_set(GTK_LIST_STORE(store), &iter, CTABLE_COLUMN_POINTER, list, -1);
			}
		else
			{
			list = collection_table_add_row(ct, &iter);
			}

		while (list)
			{
			CollectInfo *info;
			if (work)
				{
				info = work->data;
				work = work->next;
				c++;
				}
			else
				{
				info = NULL;
				}
			if (list)
				{
				list->data = info;
				list = list->next;
				}
			}
		}

	r++;
	while (gtk_tree_model_iter_nth_child(store, &iter, NULL, r))
		{
		GList *list;

		gtk_tree_model_get(store, &iter, CTABLE_COLUMN_POINTER, &list, -1);
		gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
		g_list_free(list);
		}

	ct->rows = r;

	collection_table_update_focus(ct);
	collection_table_update_status(ct);
}

static gboolean collection_table_sync_idle_cb(gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	if (!ct->sync_idle_id) return FALSE;
	g_source_remove(ct->sync_idle_id);
	ct->sync_idle_id = 0;

	collection_table_sync(ct);
	return FALSE;
}

static void collection_table_sync_idle(CollectTable *ct)
{
	if (!ct->sync_idle_id)
		{
		/* high priority, the view needs to be resynced before a redraw
		 * may contain invalid pointers at this time
		 */
		ct->sync_idle_id = g_idle_add_full(G_PRIORITY_HIGH, collection_table_sync_idle_cb, ct, NULL);
		}
}

void collection_table_add_filelist(CollectTable *ct, GList *list)
{
	GList *work;

	if (!list) return;

	work = list;
	while (work)
		{
		collection_add(ct->cd, (FileData *)work->data, FALSE);
		work = work->next;
		}
}

static void collection_table_insert_filelist(CollectTable *ct, GList *list, CollectInfo *insert_info)
{
	GList *work;

	if (!list) return;

	work = list;
	while (work)
		{
		collection_insert(ct->cd, (FileData *)work->data, insert_info, FALSE);
		work = work->next;
		}

	collection_table_sync_idle(ct);
}

static void collection_table_move_by_info_list(CollectTable *ct, GList *info_list, gint row, gint col)
{
	GList *work;
	GList *insert_pos = NULL;
	GList *temp;
	CollectInfo *info;

	if (!info_list) return;

	info = collection_table_find_data(ct, row, col, NULL);

	if (!info_list->next && info_list->data == info) return;

	if (info) insert_pos = g_list_find(ct->cd->list, info);

	/** @FIXME this may get slow for large lists */
	work = info_list;
	while (insert_pos && work)
		{
		if (insert_pos->data == work->data)
			{
			insert_pos = insert_pos->next;
			work = info_list;
			}
		else
			{
			work = work->next;
			}
		}

	work = info_list;
	while (work)
		{
		ct->cd->list = g_list_remove(ct->cd->list, work->data);
		work = work->next;
		}

	/* place them back in */
	temp = g_list_copy(info_list);

	if (insert_pos)
		{
		ct->cd->list = uig_list_insert_list(ct->cd->list, insert_pos, temp);
		}
	else if (info)
		{
		ct->cd->list = g_list_concat(temp, ct->cd->list);
		}
	else
		{
		ct->cd->list = g_list_concat(ct->cd->list, temp);
		}

	ct->cd->changed = TRUE;

	collection_table_sync_idle(ct);
}


/*
 *-------------------------------------------------------------------
 * updating
 *-------------------------------------------------------------------
 */

void collection_table_file_update(CollectTable *ct, CollectInfo *info)
{
	GtkTreeIter iter;
	gint row, col;
	gdouble value;

	if (!info)
		{
		collection_table_update_extras(ct, FALSE, 0.0);
		return;
		}

	if (!collection_table_find_position(ct, info, &row, &col)) return;

	if (ct->columns != 0 && ct->rows != 0)
		{
		value = (gdouble)(row * ct->columns + col) / (ct->columns * ct->rows);
		}
	else
		{
		value = 0.0;
		}

	collection_table_update_extras(ct, TRUE, value);

	if (collection_table_find_iter(ct, info, &iter, NULL))
		{
		GtkTreeModel *store;
		GList *list;

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(ct->listview));
		gtk_tree_model_get(store, &iter, CTABLE_COLUMN_POINTER, &list, -1);
		gtk_list_store_set(GTK_LIST_STORE(store), &iter, CTABLE_COLUMN_POINTER, list, -1);
		}
}

void collection_table_file_add(CollectTable *ct, CollectInfo *UNUSED(info))
{
	collection_table_sync_idle(ct);
}

void collection_table_file_insert(CollectTable *ct, CollectInfo *UNUSED(ci))
{
	collection_table_sync_idle(ct);
}

void collection_table_file_remove(CollectTable *ct, CollectInfo *ci)
{
	if (ci && INFO_SELECTED(ci))
		{
		ct->selection = g_list_remove(ct->selection, ci);
		}

	collection_table_sync_idle(ct);
}

void collection_table_refresh(CollectTable *ct)
{
	collection_table_populate(ct, FALSE);
}

/*
 *-------------------------------------------------------------------
 * dnd
 *-------------------------------------------------------------------
 */

static void collection_table_add_dir_recursive(CollectTable *ct, FileData *dir_fd, gboolean recursive)
{
	GList *d;
	GList *f;
	GList *work;

	if (!filelist_read(dir_fd, &f, recursive ? &d : NULL))
		return;

	f = filelist_filter(f, FALSE);
	d = filelist_filter(d, TRUE);

	f = filelist_sort_path(f);
	d = filelist_sort_path(d);

	collection_table_insert_filelist(ct, f, ct->marker_info);

	work = g_list_last(d);
	while (work)
		{
		collection_table_add_dir_recursive(ct, (FileData *)work->data, TRUE);
		work = work->prev;
		}

	filelist_free(f);
	filelist_free(d);
}

static void confirm_dir_list_do(CollectTable *ct, GList *list, gboolean recursive)
{
	GList *work = list;
	while (work)
		{
		FileData *fd = work->data;
		work = work->next;
		if (isdir(fd->path)) collection_table_add_dir_recursive(ct, fd, recursive);
		}
	collection_table_insert_filelist(ct, list, ct->marker_info);
}


static void confirm_dir_list_add(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	confirm_dir_list_do(ct, ct->drop_list, FALSE);
}

static void confirm_dir_list_recurse(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	confirm_dir_list_do(ct, ct->drop_list, TRUE);
}

static void confirm_dir_list_skip(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_insert_filelist(ct, ct->drop_list, ct->marker_info);
}

static GtkWidget *collection_table_drop_menu(CollectTable *ct)
{
	GtkWidget *menu;

	menu = popup_menu_short_lived();
	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(collection_table_popup_destroy_cb), ct);

	menu_item_add_stock(menu, _("Dropped list includes folders."), GTK_STOCK_DND_MULTIPLE, NULL, NULL);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("_Add contents"), GTK_STOCK_OK,
			    G_CALLBACK(confirm_dir_list_add), ct);
	menu_item_add_stock(menu, _("Add contents _recursive"), GTK_STOCK_ADD,
			    G_CALLBACK(confirm_dir_list_recurse), ct);
	menu_item_add_stock(menu, _("_Skip folders"), GTK_STOCK_REMOVE,
			    G_CALLBACK(confirm_dir_list_skip), ct);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("Cancel"), GTK_STOCK_CANCEL, NULL, ct);

	return menu;
}

/*
 *-------------------------------------------------------------------
 * dnd
 *-------------------------------------------------------------------
 */

static GtkTargetEntry collection_drag_types[] = {
	{ TARGET_APP_COLLECTION_MEMBER_STRING, 0, TARGET_APP_COLLECTION_MEMBER },
	{ "text/uri-list", 0, TARGET_URI_LIST },
	{ "text/plain", 0, TARGET_TEXT_PLAIN }
};
static gint n_collection_drag_types = 3;

static GtkTargetEntry collection_drop_types[] = {
	{ TARGET_APP_COLLECTION_MEMBER_STRING, 0, TARGET_APP_COLLECTION_MEMBER },
	{ "text/uri-list", 0, TARGET_URI_LIST }
};
static gint n_collection_drop_types = 2;


static void collection_table_dnd_get(GtkWidget *UNUSED(widget), GdkDragContext *UNUSED(context),
				     GtkSelectionData *selection_data, guint info,
				     guint UNUSED(time), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	gboolean selected;
	GList *list = NULL;
	gchar *uri_text = NULL;
	gint total;

	if (!ct->click_info) return;

	selected = INFO_SELECTED(ct->click_info);

	switch (info)
		{
		case TARGET_APP_COLLECTION_MEMBER:
			if (selected)
				{
				uri_text = collection_info_list_to_dnd_data(ct->cd, ct->selection, &total);
				}
			else
				{
				list = g_list_append(NULL, ct->click_info);
				uri_text = collection_info_list_to_dnd_data(ct->cd, list, &total);
				g_list_free(list);
				}
			gtk_selection_data_set(selection_data, gtk_selection_data_get_target(selection_data),
						8, (guchar *)uri_text, total);
			g_free(uri_text);
			break;
		case TARGET_URI_LIST:
		case TARGET_TEXT_PLAIN:
		default:
			if (selected)
				{
				list = collection_table_selection_get_list(ct);
				}
			else
				{
				list = g_list_append(NULL, file_data_ref(ct->click_info->fd));
				}
			if (!list) return;

			uri_selection_data_set_uris_from_filelist(selection_data, list);
			filelist_free(list);
			break;
		}
}


static void collection_table_dnd_receive(GtkWidget *UNUSED(widget), GdkDragContext *context,
					  gint x, gint y,
					  GtkSelectionData *selection_data, guint info,
					  guint time, gpointer data)
{
	CollectTable *ct = (CollectTable*)data;
	GList *list = NULL;
	GList *info_list = NULL;
	CollectionData *source;
	CollectInfo *drop_info;
	GList *work;

	DEBUG_1("%s", gtk_selection_data_get_data(selection_data));

	collection_table_scroll(ct, FALSE);
	collection_table_insert_marker(ct, NULL, FALSE);

	drop_info = collection_table_insert_point(ct, x, y);

	switch (info)
		{
		case TARGET_APP_COLLECTION_MEMBER:
			source = collection_from_dnd_data((gchar *)gtk_selection_data_get_data(selection_data), &list, &info_list);
			if (source)
				{
				if (source == ct->cd)
					{
					gint row = -1;
					gint col = -1;

					/* it is a move within a collection */
					filelist_free(list);
					list = NULL;

					if (!drop_info)
						{
						collection_table_move_by_info_list(ct, info_list, -1, -1);
						}
					else if (collection_table_find_position(ct, drop_info, &row, &col))
						{
						collection_table_move_by_info_list(ct, info_list, row, col);
						}
					}
				else
					{
					/* it is a move/copy across collections */
					if (gdk_drag_context_get_selected_action(context) == GDK_ACTION_MOVE)
						{
						collection_remove_by_info_list(source, info_list);
						}
					}
				g_list_free(info_list);
				}
			break;
		case TARGET_URI_LIST:
			list = uri_filelist_from_gtk_selection_data(selection_data);
			work = list;
			while (work)
				{
				FileData *fd = work->data;
				if (isdir(fd->path))
					{
					GtkWidget *menu;

					ct->drop_list = list;
					ct->drop_info = drop_info;
					menu = collection_table_drop_menu(ct);
					gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, time);
					return;
					}
				work = work->next;
				}
			break;
		default:
			list = NULL;
			break;
		}

	if (list)
		{
		collection_table_insert_filelist(ct, list, drop_info);
		filelist_free(list);
		}
}

static void collection_table_dnd_begin(GtkWidget *widget, GdkDragContext *context, gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	if (ct->click_info && ct->click_info->pixbuf)
		{
		gint items;

		if (INFO_SELECTED(ct->click_info))
			items = g_list_length(ct->selection);
		else
			items = 1;
		dnd_set_drag_icon(widget, context, ct->click_info->pixbuf, items);
		}
}

static void collection_table_dnd_end(GtkWidget *UNUSED(widget), GdkDragContext *UNUSED(context), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	/* apparently a leave event is not generated on a drop */
	tip_unschedule(ct);

	collection_table_scroll(ct, FALSE);
}

static gint collection_table_dnd_motion(GtkWidget *UNUSED(widget), GdkDragContext *UNUSED(context),
					gint x, gint y, guint UNUSED(time), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_motion_update(ct, x, y, TRUE);
	collection_table_scroll(ct, TRUE);

	return FALSE;
}

static void collection_table_dnd_leave(GtkWidget *UNUSED(widget), GdkDragContext *UNUSED(context), guint UNUSED(time), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_scroll(ct, FALSE);
}

static void collection_table_dnd_init(CollectTable *ct)
{
	gtk_drag_source_set(ct->listview, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
			    collection_drag_types, n_collection_drag_types,
			    GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
	g_signal_connect(G_OBJECT(ct->listview), "drag_data_get",
			 G_CALLBACK(collection_table_dnd_get), ct);
	g_signal_connect(G_OBJECT(ct->listview), "drag_begin",
			 G_CALLBACK(collection_table_dnd_begin), ct);
	g_signal_connect(G_OBJECT(ct->listview), "drag_end",
			 G_CALLBACK(collection_table_dnd_end), ct);

	gtk_drag_dest_set(ct->listview,
			  GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP,
			  collection_drop_types, n_collection_drop_types,
			  GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK);
	g_signal_connect(G_OBJECT(ct->listview), "drag_motion",
			 G_CALLBACK(collection_table_dnd_motion), ct);
	g_signal_connect(G_OBJECT(ct->listview), "drag_leave",
			 G_CALLBACK(collection_table_dnd_leave), ct);
	g_signal_connect(G_OBJECT(ct->listview), "drag_data_received",
			 G_CALLBACK(collection_table_dnd_receive), ct);
}

/*
 *-----------------------------------------------------------------------------
 * draw, etc.
 *-----------------------------------------------------------------------------
 */

typedef struct _ColumnData ColumnData;
struct _ColumnData
{
	CollectTable *ct;
	gint number;
};

static void collection_table_cell_data_cb(GtkTreeViewColumn *UNUSED(tree_column), GtkCellRenderer *cell,
					  GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	ColumnData *cd = (ColumnData*)data;
	CollectTable *ct;
	GtkStyle *style;
	GList *list;
	CollectInfo *info;
	GdkColor color_fg;
	GdkColor color_bg;
	gchar *star_rating = NULL;
	gchar *display_text = NULL;

	ct = cd->ct;

	gtk_tree_model_get(tree_model, iter, CTABLE_COLUMN_POINTER, &list, -1);

	/** @FIXME this is a primitive hack to stop a crash.
	 * When compiled with GTK3, if a Collection window containing
	 * say, 50 or so, images has its width changed, there is a segfault
	 * https://github.com/BestImageViewer/geeqie/issues/531
	 */
	if (cd->number == COLLECT_TABLE_MAX_COLUMNS) return;

	info = g_list_nth_data(list, cd->number);

	style = gtk_widget_get_style(ct->listview);
	if (info && (info->flag_mask & SELECTION_SELECTED) )
		{
		memcpy(&color_fg, &style->text[GTK_STATE_SELECTED], sizeof(color_fg));
		memcpy(&color_bg, &style->base[GTK_STATE_SELECTED], sizeof(color_bg));
		}
	else
		{
		memcpy(&color_fg, &style->text[GTK_STATE_NORMAL], sizeof(color_fg));
		memcpy(&color_bg, &style->base[GTK_STATE_NORMAL], sizeof(color_bg));
		}

	if (info && (info->flag_mask & SELECTION_PRELIGHT))
		{
		shift_color(&color_bg, -1, 0);
		}

	if (ct->show_stars && info && info->fd)
		{
		star_rating = metadata_read_rating_stars(info->fd);
		}
	else
		{
		star_rating = g_strdup("");
		}

	if (info && info->fd)
		{
		if (ct->show_text && ct->show_stars)
			{
			display_text = g_strconcat(info->fd->name, "\n", star_rating, NULL);
			}
		else if (ct->show_text)
			{
			display_text = g_strdup(info->fd->name);
			}
		else if (ct->show_stars)
			{
			display_text = g_strdup(star_rating);
			}
		else
			{
			display_text = g_strdup("");
			}
		}
	else
		{
		display_text = g_strdup("");
		}

	if (GQV_IS_CELL_RENDERER_ICON(cell))
		{
		if (info)
			{
			g_object_set(cell,	"pixbuf", info->pixbuf,
						"text",  display_text,
						"cell-background-gdk", &color_bg,
						"cell-background-set", TRUE,
						"foreground-gdk", &color_fg,
						"foreground-set", TRUE,
						"has-focus", (ct->focus_info == info), NULL);
			}
		else
			{
			g_object_set(cell,	"pixbuf", NULL,
						"text", NULL,
						"cell-background-set", FALSE,
						"foreground-set", FALSE,
						"has-focus", FALSE,  NULL);
			}
		}

	g_free(display_text);
	g_free(star_rating);
}

static void collection_table_append_column(CollectTable *ct, gint n)
{
	ColumnData *cd;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_min_width(column, 0);

	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment(column, 0.5);

	renderer = gqv_cell_renderer_icon_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	g_object_set(G_OBJECT(renderer), "xpad", THUMB_BORDER_PADDING * 2,
					 "ypad", THUMB_BORDER_PADDING,
					 "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);

	g_object_set_data(G_OBJECT(column), "column_number", GINT_TO_POINTER(n));

	cd = g_new0(ColumnData, 1);
	cd->ct = ct;
	cd->number = n;
	gtk_tree_view_column_set_cell_data_func(column, renderer, collection_table_cell_data_cb, cd, g_free);

	gtk_tree_view_append_column(GTK_TREE_VIEW(ct->listview), column);
}

/*
 *-------------------------------------------------------------------
 * init, destruction
 *-------------------------------------------------------------------
 */

static void collection_table_destroy(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	/* If there is no unsaved data, save the window geometry
	 */
	if (!ct->cd->changed)
		{
		if (!collection_save(ct->cd, ct->cd->path))
			{
			log_printf("failed saving to collection path: %s\n", ct->cd->path);
			}
		}

	if (ct->popup)
		{
		g_signal_handlers_disconnect_matched(G_OBJECT(ct->popup), G_SIGNAL_MATCH_DATA,
						     0, 0, 0, NULL, ct);
		gtk_widget_destroy(ct->popup);
		}

	if (ct->sync_idle_id) g_source_remove(ct->sync_idle_id);

	tip_unschedule(ct);
	collection_table_scroll(ct, FALSE);

	g_free(ct);
}

static void collection_table_sized(GtkWidget *UNUSED(widget), GtkAllocation *allocation, gpointer data)
{
	CollectTable *ct = (CollectTable*)data;

	collection_table_populate_at_new_size(ct, allocation->width, allocation->height, FALSE);
}

CollectTable *collection_table_new(CollectionData *cd)
{
	CollectTable *ct;
	GtkListStore *store;
	GtkTreeSelection *selection;
	gint i;

	ct = g_new0(CollectTable, 1);

	ct->cd = cd;
	ct->show_text = options->show_icon_names;
	ct->show_stars = options->show_star_rating;

	ct->scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(ct->scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ct->scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	store = gtk_list_store_new(1, G_TYPE_POINTER);
	ct->listview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ct->listview));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_NONE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ct->listview), FALSE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ct->listview), FALSE);

	for (i = 0; i < COLLECT_TABLE_MAX_COLUMNS; i++)
		{
		collection_table_append_column(ct, i);
		}

	/* zero width column to hide tree view focus, we draw it ourselves */
	collection_table_append_column(ct, i);
	/* end column to fill white space */
	collection_table_append_column(ct, i);

	g_signal_connect(G_OBJECT(ct->listview), "destroy",
			 G_CALLBACK(collection_table_destroy), ct);
	g_signal_connect(G_OBJECT(ct->listview), "size_allocate",
			 G_CALLBACK(collection_table_sized), ct);
	g_signal_connect(G_OBJECT(ct->listview), "key_press_event",
			 G_CALLBACK(collection_table_press_key_cb), ct);

	gtk_container_add(GTK_CONTAINER(ct->scrolled), ct->listview);
	gtk_widget_show(ct->listview);

	collection_table_dnd_init(ct);

	gtk_widget_set_events(ct->listview, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK |
			      GDK_BUTTON_PRESS_MASK | GDK_LEAVE_NOTIFY_MASK);
	g_signal_connect(G_OBJECT(ct->listview),"button_press_event",
			 G_CALLBACK(collection_table_press_cb), ct);
	g_signal_connect(G_OBJECT(ct->listview),"button_release_event",
			 G_CALLBACK(collection_table_release_cb), ct);
	g_signal_connect(G_OBJECT(ct->listview),"motion_notify_event",
			 G_CALLBACK(collection_table_motion_cb), ct);
	g_signal_connect(G_OBJECT(ct->listview), "leave_notify_event",
			 G_CALLBACK(collection_table_leave_cb), ct);

	return ct;
}

void collection_table_set_labels(CollectTable *ct, GtkWidget *status, GtkWidget *extra)
{
	ct->status_label = status;
	ct->extra_label = extra;
	collection_table_update_status(ct);
	collection_table_update_extras(ct, FALSE, 0.0);
}

CollectInfo *collection_table_get_focus_info(CollectTable *ct)
{
	return collection_table_find_data(ct, ct->focus_row, ct->focus_column, NULL);
}

/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
