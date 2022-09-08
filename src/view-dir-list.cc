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
#include "view-dir-list.h"

#include "dnd.h"
#include "dupe.h"
#include "filedata.h"
#include "layout.h"
#include "layout-image.h"
#include "layout-util.h"
#include "utilops.h"
#include "ui-fileops.h"
#include "ui-menu.h"
#include "ui-tree-edit.h"
#include "view-dir.h"

#include <gdk/gdkkeysyms.h> /* for keyboard values */


#define VDLIST(_vd_) ((ViewDirInfoList *)(_vd_->info))


/*
 *-----------------------------------------------------------------------------
 * misc
 *-----------------------------------------------------------------------------
 */

gboolean vdlist_find_row(ViewDir *vd, FileData *fd, GtkTreeIter *iter)
{
	GtkTreeModel *store;
	gboolean valid;

	store = gtk_tree_view_get_model(GTK_TREE_VIEW(vd->view));
	valid = gtk_tree_model_get_iter_first(store, iter);
	while (valid)
		{
		FileData *fd_n;
		gtk_tree_model_get(GTK_TREE_MODEL(store), iter, DIR_COLUMN_POINTER, &fd_n, -1);
		if (fd_n == fd) return TRUE;

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter);
		}

	return FALSE;
}


FileData *vdlist_row_by_path(ViewDir *vd, const gchar *path, gint *row)
{
	GList *work;
	gint n;

	if (!path)
		{
		if (row) *row = -1;
		return NULL;
		}

	n = 0;
	work = VDLIST(vd)->list;
	while (work)
		{
		FileData *fd = work->data;
		if (strcmp(fd->path, path) == 0)
			{
			if (row) *row = n;
			return fd;
			}
		work = work->next;
		n++;
		}

	if (row) *row = -1;
	return NULL;
}

/*
 *-----------------------------------------------------------------------------
 * dnd
 *-----------------------------------------------------------------------------
 */

static void vdlist_scroll_to_row(ViewDir *vd, FileData *fd, gfloat y_align)
{
	GtkTreeIter iter;

	if (gtk_widget_get_realized(vd->view) && vd_find_row(vd, fd, &iter))
		{
		GtkTreeModel *store;
		GtkTreePath *tpath;

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(vd->view));
		tpath = gtk_tree_model_get_path(store, &iter);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(vd->view), tpath, NULL, TRUE, y_align, 0.0);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(vd->view), tpath, NULL, FALSE);
		gtk_tree_path_free(tpath);

		if (!gtk_widget_has_focus(vd->view)) gtk_widget_grab_focus(vd->view);
		}
}

/*
 *-----------------------------------------------------------------------------
 * main
 *-----------------------------------------------------------------------------
 */

const gchar *vdlist_row_get_path(ViewDir *vd, gint row)
{
	FileData *fd;

	fd = g_list_nth_data(VDLIST(vd)->list, row);

	if (fd) return fd->path;

	return NULL;
}

static gboolean vdlist_populate(ViewDir *vd, gboolean clear)
{
	GtkListStore *store;
	GList *work;
	GtkTreeIter iter;
	gboolean valid;
	gchar *filepath;
	GList *old_list;
	gboolean ret;
	FileData *fd;
	SortType sort_type = SORT_NAME;
	gboolean sort_ascend = TRUE;
	gchar *link = NULL;

	if (vd->layout)
		{
		sort_type = vd->layout->options.dir_view_list_sort.method;
		sort_ascend = vd->layout->options.dir_view_list_sort.ascend;
		}

	old_list = VDLIST(vd)->list;

	ret = filelist_read(vd->dir_fd, NULL, &VDLIST(vd)->list);
	VDLIST(vd)->list = filelist_sort(VDLIST(vd)->list, sort_type, sort_ascend);

	/* add . and .. */

	if (options->file_filter.show_parent_directory && strcmp(vd->dir_fd->path, G_DIR_SEPARATOR_S) != 0)
		{
		filepath = g_build_filename(vd->dir_fd->path, "..", NULL);
		fd = file_data_new_dir(filepath);
		VDLIST(vd)->list = g_list_prepend(VDLIST(vd)->list, fd);
		g_free(filepath);
		}

	if (options->file_filter.show_dot_directory)
		{
		filepath = g_build_filename(vd->dir_fd->path, ".", NULL);
		fd = file_data_new_dir(filepath);
		VDLIST(vd)->list = g_list_prepend(VDLIST(vd)->list, fd);
		g_free(filepath);
	}

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(vd->view)));
	if (clear) gtk_list_store_clear(store);

	valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &iter, NULL);

	work = VDLIST(vd)->list;
	while (work)
		{
		gint match;
		GdkPixbuf *pixbuf;
		const gchar *date = "";
		gboolean done = FALSE;

		fd = work->data;

		if (access_file(fd->path, R_OK | X_OK) && fd->name)
			{
			if (islink(fd->path))
				{
				pixbuf = vd->pf->link;
				}
			else if (fd->name[0] == '.' && fd->name[1] == '\0')
				{
				pixbuf = vd->pf->open;
				}
			else if (fd->name[0] == '.' && fd->name[1] == '.' && fd->name[2] == '\0')
				{
				pixbuf = vd->pf->parent;
				}
			else
				{
				pixbuf = vd->pf->close;
				if (vd->layout && vd->layout->options.show_directory_date)
					date = text_from_time(fd->date);
				}
			}
		else
			{
			pixbuf = vd->pf->deny;
			}

		while (!done)
			{
			FileData *old_fd = NULL;

			if (valid)
				{
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
						   DIR_COLUMN_POINTER, &old_fd,
						   -1);

				if (fd == old_fd)
					{
					match = 0;
					}
				else
					{
					match = filelist_sort_compare_filedata_full(fd, old_fd, sort_type, sort_ascend);

					if (match == 0) g_warning("multiple fd for the same path");
					}

				}
			else
				{
				match = -1;
				}

			if (islink(fd->path))
				{
				link = realpath(fd->path, NULL);
				}
			else
				{
				link = NULL;
				}

			if (match < 0)
				{
				GtkTreeIter new_iter;

				if (valid)
					{
					gtk_list_store_insert_before(store, &new_iter, &iter);
					}
				else
					{
					gtk_list_store_append(store, &new_iter);
					}

				gtk_list_store_set(store, &new_iter,
						   DIR_COLUMN_POINTER, fd,
						   DIR_COLUMN_ICON, pixbuf,
						   DIR_COLUMN_NAME, fd->name,
						   DIR_COLUMN_LINK, link,
						   DIR_COLUMN_DATE, date,
						   -1);

				done = TRUE;
				}
			else if (match > 0)
				{
				valid = gtk_list_store_remove(store, &iter);
				}
			else
				{
				gtk_list_store_set(store, &iter,
						   DIR_COLUMN_ICON, pixbuf,
						   DIR_COLUMN_NAME, fd->name,
						   DIR_COLUMN_LINK, link,
						   DIR_COLUMN_DATE, date,
						   -1);

				if (valid) valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);

				done = TRUE;
				}
			}
		work = work->next;
		}

	while (valid)
		{
		FileData *old_fd;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, DIR_COLUMN_POINTER, &old_fd, -1);

		valid = gtk_list_store_remove(store, &iter);
		}


	vd->click_fd = NULL;
	vd->drop_fd = NULL;

	filelist_free(old_list);
	g_free(link);
	return ret;
}

gboolean vdlist_set_fd(ViewDir *vd, FileData *dir_fd)
{
	gboolean ret;
	gchar *old_path = NULL; /* Used to store directory for walking up */

	if (!dir_fd) return FALSE;
	if (vd->dir_fd == dir_fd) return TRUE;

	if (vd->dir_fd)
		{
		gchar *base;

		base = remove_level_from_path(vd->dir_fd->path);
		if (strcmp(base, dir_fd->path) == 0)
			{
			old_path = g_strdup(filename_from_path(vd->dir_fd->path));
			}
		g_free(base);
		}

	file_data_unref(vd->dir_fd);
	vd->dir_fd = file_data_ref(dir_fd);

	ret = vdlist_populate(vd, TRUE);

	/* scroll to make last path visible */
	FileData *found = NULL;
	GList *work;

	work = VDLIST(vd)->list;
	while (work && !found)
		{
		FileData *fd = work->data;
		if (!old_path || strcmp(old_path, fd->name) == 0) found = fd;
		work = work->next;
		}

	if (found) vdlist_scroll_to_row(vd, found, 0.5);

	if (old_path) g_free(old_path);

	return ret;
}

void vdlist_refresh(ViewDir *vd)
{
	vdlist_populate(vd, FALSE);
}

gboolean vdlist_press_key_cb(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	ViewDir *vd = (ViewDir*)data;
	GtkTreePath *tpath;

	if (event->keyval != GDK_KEY_Menu) return FALSE;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(vd->view), &tpath, NULL);
	if (tpath)
		{
		GtkTreeModel *store;
		GtkTreeIter iter;

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &vd->click_fd, -1);

		gtk_tree_path_free(tpath);
		}
	else
		{
		vd->click_fd = NULL;
		}

	vd_color_set(vd, vd->click_fd, TRUE);

	vd->popup = vd_pop_menu(vd, vd->click_fd);

	gtk_menu_popup(GTK_MENU(vd->popup), NULL, NULL, vd_menu_position_cb, vd, 0, GDK_CURRENT_TIME);

	return TRUE;
}

gboolean vdlist_press_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer data)
{
	ViewDir *vd = (ViewDir*)data;
	GtkTreePath *tpath;
	GtkTreeIter iter;
	FileData *fd = NULL;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bevent->x, bevent->y,
					  &tpath, NULL, NULL, NULL))
		{
		GtkTreeModel *store;

		store = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
		gtk_tree_model_get_iter(store, &iter, tpath);
		gtk_tree_model_get(store, &iter, DIR_COLUMN_POINTER, &fd, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), tpath, NULL, FALSE);
		gtk_tree_path_free(tpath);
		}

	vd->click_fd = fd;

	if (options->view_dir_list_single_click_enter)
		vd_color_set(vd, vd->click_fd, TRUE);

	if (bevent->button == MOUSE_BUTTON_RIGHT)
		{
		vd->popup = vd_pop_menu(vd, vd->click_fd);
		gtk_menu_popup(GTK_MENU(vd->popup), NULL, NULL, NULL, NULL,
			       bevent->button, bevent->time);
		return TRUE;
		}

	return options->view_dir_list_single_click_enter;
}

void vdlist_destroy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewDir *vd = (ViewDir*)data;

	vd_dnd_drop_scroll_cancel(vd);
	widget_auto_scroll_stop(vd->view);

	filelist_free(VDLIST(vd)->list);
}

ViewDir *vdlist_new(ViewDir *vd, FileData *UNUSED(dir_fd))
{
	GtkListStore *store;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	vd->info = g_new0(ViewDirInfoList, 1);

	vd->type = DIRVIEW_LIST;

	store = gtk_list_store_new(6, G_TYPE_POINTER, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	vd->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(vd->view), FALSE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(vd->view), FALSE);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(vd->view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", DIR_COLUMN_ICON);
	gtk_tree_view_column_set_cell_data_func(column, renderer, vd_color_cb, vd, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", DIR_COLUMN_NAME);
	gtk_tree_view_column_set_cell_data_func(column, renderer, vd_color_cb, vd, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", DIR_COLUMN_DATE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, vd_color_cb, vd, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(vd->view), column);

	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(vd->view), DIR_COLUMN_LINK);

	return vd;
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
