/*
 * Copyright (C) 2004 John Ellis
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

#include "main.h"
#include "desktop_file.h"

#include "editors.h"
#include "filedata.h"
#include "misc.h"
#include "ui_misc.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"
#include "pixbuf_util.h"
#include "window.h"
#include "utilops.h"
#include "layout_util.h"

#define CONFIG_WINDOW_DEF_WIDTH		700
#define CONFIG_WINDOW_DEF_HEIGHT	400



typedef struct _EditorWindow EditorWindow;
struct _EditorWindow
{
	GtkWidget *window;
	GtkWidget *entry;
	GtkWidget *save_button;
	GtkTextBuffer *buffer;
	gchar *desktop_name;
	gboolean modified;
};

typedef struct _EditorListWindow EditorListWindow;
struct _EditorListWindow
{
	GtkWidget *window;
	GtkWidget *view;
	GenericDialog *gd;	/* any open confirm dialogs ? */
	GtkWidget *delete_button;
	GtkWidget *edit_button;
};

typedef struct _EditorWindowDel_Data EditorWindowDel_Data;
struct _EditorWindowDel_Data
{
	EditorListWindow *ewl;
	gchar *path;
};

static EditorListWindow *editor_list_window = NULL;

static gboolean editor_window_save(EditorWindow *ew)
{
	gchar *dir;
	gchar *path;
	gchar *text;
	GtkTextIter start, end;
	GError *error = NULL;
	gboolean ret = TRUE;
	const gchar *name = gtk_entry_get_text(GTK_ENTRY(ew->entry));

	if (!name || !name[0])
		{
		file_util_warning_dialog(_("Can't save"), _("Please specify file name."), GTK_STOCK_DIALOG_ERROR, NULL);
		return FALSE;
		}

	gtk_text_buffer_get_bounds(ew->buffer, &start, &end);
	text = gtk_text_buffer_get_text(ew->buffer, &start, &end, FALSE);

	dir = g_build_filename(get_rc_dir(), "applications", NULL);
	path = g_build_filename(dir, name, NULL);

	if (!recursive_mkdir_if_not_exists(dir, 0755))
		{
		file_util_warning_dialog(_("Can't save"), _("Could not create directory"), GTK_STOCK_DIALOG_ERROR, NULL);
		ret = FALSE;
		}

	if (ret && !g_file_set_contents(path, text, -1, &error))
		{
		file_util_warning_dialog(_("Can't save"), error->message, GTK_STOCK_DIALOG_ERROR, NULL);
		g_error_free(error);
		ret = FALSE;
		}

	g_free(path);
	g_free(dir);
	g_free(text);
	layout_editors_reload_start();
	/* idle function is not needed, everything should be cached */
	layout_editors_reload_finish();
	return ret;
}

static void editor_window_close_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	EditorWindow *ew = data;

	g_free(ew->desktop_name);
	gtk_widget_destroy(ew->window);
	g_free(ew);
}

static gint editor_window_delete_cb(GtkWidget *w, GdkEventAny *UNUSED(event), gpointer data)
{
	editor_window_close_cb(w, data);
	return TRUE;
}

static void editor_window_save_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	EditorWindow *ew = data;

	if (ew->modified)
		{
		editor_window_save(ew);
		}

	gtk_widget_set_sensitive(ew->save_button, FALSE);
	gtk_text_buffer_set_modified(ew->buffer, FALSE);
	ew->modified = FALSE;
}

static void editor_window_text_modified_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	EditorWindow *ew = data;

	if (gtk_text_buffer_get_modified(ew->buffer))
		{
		gtk_widget_set_sensitive(ew->save_button, TRUE);
		ew->modified = TRUE;
		}
}

static void editor_window_entry_changed_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	EditorWindow *ew = data;
	const gchar *content = gtk_entry_get_text(GTK_ENTRY(ew->entry));
	gboolean modified = ew->modified;

	if (!modified)
		{
		modified = (!ew->desktop_name && *content);
		}

	if (!modified)
		{
		modified = strcmp(ew->desktop_name, content);
		}

	gtk_widget_set_sensitive(ew->save_button, modified);
	ew->modified = modified;
}

static void editor_window_new(const gchar *src_path, const gchar *desktop_name)
{
	EditorWindow *ew;
	GtkWidget *win_vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *ct_button;
	GtkWidget *button_hbox;
	GtkWidget *scrolled;
	GtkWidget *text_view;
	gchar *text;
	gsize size;

	ew = g_new0(EditorWindow, 1);


	ew->window = window_new(GTK_WINDOW_TOPLEVEL, "Desktop", PIXBUF_INLINE_ICON_CONFIG, NULL, _("Desktop file"));
	DEBUG_NAME(ew->window);
	gtk_window_set_type_hint(GTK_WINDOW(ew->window), GDK_WINDOW_TYPE_HINT_DIALOG);

	g_signal_connect(G_OBJECT(ew->window), "delete_event",
			 G_CALLBACK(editor_window_delete_cb), ew);

	gtk_window_set_default_size(GTK_WINDOW(ew->window), CONFIG_WINDOW_DEF_WIDTH, CONFIG_WINDOW_DEF_HEIGHT);
	gtk_window_set_resizable(GTK_WINDOW(ew->window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(ew->window), PREF_PAD_BORDER);

	win_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_SPACE);
	gtk_container_add(GTK_CONTAINER(ew->window), win_vbox);
	gtk_widget_show(win_vbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, PREF_PAD_SPACE);
	gtk_box_pack_end(GTK_BOX(win_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	ew->entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), ew->entry, TRUE, TRUE, 0);
	ew->desktop_name = NULL;
	if (desktop_name)
		{
		gtk_entry_set_text(GTK_ENTRY(ew->entry), desktop_name);
		ew->desktop_name = g_strdup(desktop_name);
		}
	gtk_widget_show(ew->entry);
	g_signal_connect(G_OBJECT(ew->entry), "changed", G_CALLBACK(editor_window_entry_changed_cb), ew);

	button_hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(button_hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(button_hbox), PREF_PAD_BUTTON_GAP);
	gtk_box_pack_end(GTK_BOX(hbox), button_hbox, FALSE, FALSE, 0);
	gtk_widget_show(button_hbox);

	ew->save_button = pref_button_new(NULL, GTK_STOCK_SAVE, NULL, FALSE,
				 G_CALLBACK(editor_window_save_cb), ew);
	gtk_container_add(GTK_CONTAINER(button_hbox), ew->save_button);
	gtk_widget_set_can_default(ew->save_button, TRUE);
	gtk_widget_set_sensitive(ew->save_button, FALSE);
	gtk_widget_show(ew->save_button);
	ct_button = ew->save_button;

	button = pref_button_new(NULL, GTK_STOCK_CLOSE, NULL, FALSE,
				 G_CALLBACK(editor_window_close_cb), ew);
	gtk_container_add(GTK_CONTAINER(button_hbox), button);
	gtk_widget_set_can_default(button, TRUE);
	gtk_widget_show(button);

	if (!generic_dialog_get_alternative_button_order(ew->window))
		{
		gtk_box_reorder_child(GTK_BOX(button_hbox), ct_button, -1);
		}


	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(win_vbox), scrolled, TRUE, TRUE, 5);
	gtk_widget_show(scrolled);

	text_view = gtk_text_view_new();
	gtk_container_add(GTK_CONTAINER(scrolled), text_view);
	gtk_widget_show(text_view);

	ew->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	if (g_file_get_contents(src_path, &text, &size, NULL))
		{
		gtk_text_buffer_set_text(ew->buffer, text, size);
		}
	gtk_text_buffer_set_modified(ew->buffer, FALSE);
	g_signal_connect(G_OBJECT(ew->buffer), "modified-changed",
			 G_CALLBACK(editor_window_text_modified_cb), ew);

	gtk_widget_show(ew->window);
}


static void editor_list_window_close_cb(GtkWidget *UNUSED(widget), gpointer UNUSED(data))
{
	gtk_widget_destroy(editor_list_window->window);
	g_free(editor_list_window);
	editor_list_window = NULL;
}

static gboolean editor_list_window_delete(GtkWidget *UNUSED(widget), GdkEventAny *UNUSED(event), gpointer UNUSED(data))
{
	editor_list_window_close_cb(NULL, NULL);
	return TRUE;
}

static void editor_list_window_delete_dlg_cancel(GenericDialog *gd, gpointer data);

static void editor_list_window_delete_dlg_cancel(GenericDialog *UNUSED(gd), gpointer data)
{
	EditorWindowDel_Data *ewdl = data;

	ewdl->ewl->gd = NULL;
	g_free(ewdl->path);
	g_free(ewdl);
}

static void editor_list_window_delete_dlg_ok_cb(GenericDialog *gd, gpointer data)
{
	EditorWindowDel_Data *ewdl = data;

	if (!unlink_file(ewdl->path))
		{
		gchar *text = g_strdup_printf(_("Unable to delete file:\n%s"), ewdl->path);
		warning_dialog(_("File deletion failed"), text, GTK_STOCK_DIALOG_WARNING, NULL);
		g_free(text);
		}
	else
		{
		/* refresh list */
		layout_editors_reload_start();
		/* idle function is not needed, everything should be cached */
		layout_editors_reload_finish();
		}

	editor_list_window_delete_dlg_cancel(gd, data);
}

static void editor_list_window_delete_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	EditorListWindow *ewl = data;
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ewl->view));
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(sel, NULL, &iter))
		{
		GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(ewl->view));
		gchar *path;
		gchar *key;
		gchar *text;
		EditorWindowDel_Data *ewdl;

		gtk_tree_model_get(store, &iter,
				   DESKTOP_FILE_COLUMN_PATH, &path,
				   DESKTOP_FILE_COLUMN_KEY, &key, -1);


		ewdl = g_new(EditorWindowDel_Data, 1);
		ewdl->ewl = ewl;
		ewdl->path = path;

		if (ewl->gd)
			{
			GenericDialog *gd = ewl->gd;
			editor_list_window_delete_dlg_cancel(ewl->gd, ewl->gd->data);
			generic_dialog_close(gd);
			}

		ewl->gd = generic_dialog_new(_("Delete file"), "dlg_confirm",
					    NULL, TRUE,
					    editor_list_window_delete_dlg_cancel, ewdl);

		generic_dialog_add_button(ewl->gd, GTK_STOCK_DELETE, NULL, editor_list_window_delete_dlg_ok_cb, TRUE);

		text = g_strdup_printf(_("About to delete the file:\n %s"), path);
		generic_dialog_add_message(ewl->gd, GTK_STOCK_DIALOG_QUESTION,
					   _("Delete file"), text, TRUE);
		g_free(text);

		gtk_widget_show(ewl->gd->dialog);
		}
}

static void editor_list_window_edit_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	EditorListWindow *ewl = data;
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ewl->view));
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(sel, NULL, &iter))
		{
		GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(ewl->view));
		gchar *path;
		gchar *key;

		gtk_tree_model_get(store, &iter,
				   DESKTOP_FILE_COLUMN_PATH, &path,
				   DESKTOP_FILE_COLUMN_KEY, &key, -1);
		editor_window_new(path, key);
		g_free(key);
		g_free(path);
		}
}

static void editor_list_window_new_cb(GtkWidget *UNUSED(widget), gpointer UNUSED(data))
{
	editor_window_new(desktop_file_template, _("new.desktop"));
}

static void editor_list_window_help_cb(GtkWidget *UNUSED(widget), gpointer UNUSED(data))
{
	help_window_show("GuidePluginsConfig.html");
}

static void editor_list_window_selection_changed_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	EditorListWindow *ewl = data;
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(ewl->view));
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(sel, NULL, &iter))
		{
		GtkTreeModel *store = gtk_tree_view_get_model(GTK_TREE_VIEW(ewl->view));
		gchar *path;

		gtk_tree_model_get(store, &iter,
						   DESKTOP_FILE_COLUMN_PATH, &path,
						   -1);

		gtk_widget_set_sensitive(ewl->delete_button, access_file(path, W_OK));
		gtk_widget_set_sensitive(ewl->edit_button, TRUE);
		g_free(path);
		}

}

static gint editor_list_window_sort_cb(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
	gint n = GPOINTER_TO_INT(data);
	gint ret = 0;
	gboolean bool1, bool2;

	switch (n)
		{
		case DESKTOP_FILE_COLUMN_KEY:
		case DESKTOP_FILE_COLUMN_NAME:
		case DESKTOP_FILE_COLUMN_PATH:
		case DESKTOP_FILE_COLUMN_HIDDEN:
			{
			gchar *s1, *s2;

			gtk_tree_model_get(model, a, n, &s1, -1);
			gtk_tree_model_get(model, b, n, &s2, -1);

			if (!s1 || !s2)
				{
			  	if (!s1 && !s2) break;
			  	ret = s1 ? 1 : -1;
				}
			else
				{
			  	ret = g_utf8_collate(s1, s2);
				}

			g_free(s1);
			g_free(s2);
			}
			break;
		case DESKTOP_FILE_COLUMN_DISABLED:
			{
			gtk_tree_model_get(model, a, n, &bool1, -1);
			gtk_tree_model_get(model, b, n, &bool2, -1);

			if (bool1 == bool2)
				{
				ret = 0;
				}
			else if (bool1 > bool2)
				{
				ret = -1;
				}
			else
				{
				ret = 1;
				}
			break;
			}

    		default:
       			g_return_val_if_reached(0);
		}

	return ret;
}

static void plugin_disable_cb(GtkCellRendererToggle *UNUSED(renderer), gchar *path_str, gpointer data)
{
	EditorListWindow *ewl = data;
	GtkTreePath *tpath;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean disabled;
	gchar *path;
	GList *list;
	gchar *haystack;

	tpath = gtk_tree_path_new_from_string(path_str);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(ewl->view));
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, tpath);
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, DESKTOP_FILE_COLUMN_DISABLED, &disabled, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, DESKTOP_FILE_COLUMN_PATH, &path, -1);

	gtk_list_store_set(GTK_LIST_STORE(desktop_file_list), &iter, DESKTOP_FILE_COLUMN_DISABLED, !disabled, -1);

	if (!disabled)
		{
		options->disabled_plugins = g_list_append((options->disabled_plugins), g_strdup(path));
		}
	else
		{
		list = options->disabled_plugins;
		while (list)
			{
			haystack = list->data;

			if (haystack && strcmp(haystack, path) == 0)
				{
				g_free(haystack);
				options->disabled_plugins = g_list_remove(options->disabled_plugins, haystack);
				}

			list = list->next;
			}
		}

	layout_editors_reload_start();
	layout_editors_reload_finish();
}

static void plugin_disable_set_func(GtkTreeViewColumn *UNUSED(tree_column), GtkCellRenderer *cell,
							GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer UNUSED(data))
{
	gboolean disabled;

	gtk_tree_model_get(tree_model, iter, DESKTOP_FILE_COLUMN_DISABLED, &disabled, -1);

	if (disabled)
		{
		g_object_set(GTK_CELL_RENDERER(cell), "active", TRUE, NULL);
		}
	else
		{
		g_object_set(GTK_CELL_RENDERER(cell), "active", FALSE, NULL);
		}
}

static void editor_list_window_create(void)
{
	GtkWidget *win_vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *scrolled;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkTreeModel *store;
	GtkTreeSortable *sortable;
	EditorListWindow *ewl;

	editor_list_window = ewl = g_new0(EditorListWindow, 1);

	ewl->window = window_new(GTK_WINDOW_TOPLEVEL, "editors", PIXBUF_INLINE_ICON_CONFIG, NULL, _("Plugins"));
	DEBUG_NAME(ewl->window);
	gtk_window_set_type_hint(GTK_WINDOW(ewl->window), GDK_WINDOW_TYPE_HINT_DIALOG);
	g_signal_connect(G_OBJECT(ewl->window), "delete_event",
			 G_CALLBACK(editor_list_window_delete), NULL);
	gtk_window_set_default_size(GTK_WINDOW(ewl->window), CONFIG_WINDOW_DEF_WIDTH, CONFIG_WINDOW_DEF_HEIGHT);
	gtk_window_set_resizable(GTK_WINDOW(ewl->window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(ewl->window), PREF_PAD_BORDER);

	win_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_SPACE);
	gtk_container_add(GTK_CONTAINER(ewl->window), win_vbox);
	gtk_widget_show(win_vbox);

	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), PREF_PAD_BUTTON_GAP);
	gtk_box_pack_end(GTK_BOX(win_vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);


	button = pref_button_new(NULL, GTK_STOCK_HELP, NULL, FALSE,
				 G_CALLBACK(editor_list_window_help_cb), ewl);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	gtk_widget_set_can_default(button, TRUE);
	gtk_widget_show(button);

	button = pref_button_new(NULL, GTK_STOCK_NEW, NULL, FALSE,
				 G_CALLBACK(editor_list_window_new_cb), ewl);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	gtk_widget_set_can_default(button, TRUE);
	gtk_widget_show(button);

	button = pref_button_new(NULL, GTK_STOCK_EDIT, NULL, FALSE,
				 G_CALLBACK(editor_list_window_edit_cb), ewl);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	gtk_widget_set_can_default(button, TRUE);
	gtk_widget_set_sensitive(button, FALSE);
	gtk_widget_show(button);
	ewl->edit_button = button;

	button = pref_button_new(NULL, GTK_STOCK_DELETE, NULL, FALSE,
				 G_CALLBACK(editor_list_window_delete_cb), ewl);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	gtk_widget_set_can_default(button, TRUE);
	gtk_widget_set_sensitive(button, FALSE);
	gtk_widget_show(button);
	ewl->delete_button = button;

	button = pref_button_new(NULL, GTK_STOCK_CLOSE, NULL, FALSE,
				 G_CALLBACK(editor_list_window_close_cb), ewl);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	gtk_widget_set_can_default(button, TRUE);
	gtk_widget_show(button);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(win_vbox), scrolled, TRUE, TRUE, 5);
	gtk_widget_show(scrolled);

	ewl->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(desktop_file_list));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ewl->view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_SINGLE);
 	g_signal_connect(selection, "changed", G_CALLBACK(editor_list_window_selection_changed_cb), ewl);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ewl->view), FALSE);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Disabled"));
	gtk_tree_view_column_set_resizable(column, TRUE);

	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(G_OBJECT(renderer), "toggled",
			 G_CALLBACK(plugin_disable_cb), ewl);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, plugin_disable_set_func,
						NULL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ewl->view), column);
	gtk_tree_view_column_set_sort_column_id(column, DESKTOP_FILE_COLUMN_DISABLED);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Name"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", DESKTOP_FILE_COLUMN_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ewl->view), column);
	gtk_tree_view_column_set_sort_column_id(column, DESKTOP_FILE_COLUMN_NAME);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Hidden"));
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", DESKTOP_FILE_COLUMN_HIDDEN);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ewl->view), column);
	gtk_tree_view_column_set_sort_column_id(column, DESKTOP_FILE_COLUMN_HIDDEN);
	gtk_tree_view_column_set_alignment(column, 0.5);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Desktop file"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", DESKTOP_FILE_COLUMN_KEY);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ewl->view), column);
	gtk_tree_view_column_set_sort_column_id(column, DESKTOP_FILE_COLUMN_KEY);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Path"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", DESKTOP_FILE_COLUMN_PATH);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ewl->view), column);
	gtk_tree_view_column_set_sort_column_id(column, DESKTOP_FILE_COLUMN_PATH);

	/* set up sorting */
	store = gtk_tree_view_get_model(GTK_TREE_VIEW(ewl->view));
	sortable = GTK_TREE_SORTABLE(store);
	gtk_tree_sortable_set_sort_func(sortable, DESKTOP_FILE_COLUMN_KEY, editor_list_window_sort_cb,
					GINT_TO_POINTER(DESKTOP_FILE_COLUMN_KEY), NULL);
	gtk_tree_sortable_set_sort_func(sortable, DESKTOP_FILE_COLUMN_HIDDEN, editor_list_window_sort_cb,
					GINT_TO_POINTER(DESKTOP_FILE_COLUMN_HIDDEN), NULL);
	gtk_tree_sortable_set_sort_func(sortable, DESKTOP_FILE_COLUMN_NAME, editor_list_window_sort_cb,
					GINT_TO_POINTER(DESKTOP_FILE_COLUMN_NAME), NULL);
	gtk_tree_sortable_set_sort_func(sortable, DESKTOP_FILE_COLUMN_PATH, editor_list_window_sort_cb,
					GINT_TO_POINTER(DESKTOP_FILE_COLUMN_PATH), NULL);
	gtk_tree_sortable_set_sort_func(sortable, DESKTOP_FILE_COLUMN_DISABLED, editor_list_window_sort_cb,
					GINT_TO_POINTER(DESKTOP_FILE_COLUMN_DISABLED), NULL);

	/* set initial sort order */
    gtk_tree_sortable_set_sort_column_id(sortable, DESKTOP_FILE_COLUMN_NAME, GTK_SORT_ASCENDING);

	gtk_container_add(GTK_CONTAINER(scrolled), ewl->view);
	gtk_widget_show(ewl->view);

	gtk_widget_show(ewl->window);
}

/*
 *-----------------------------------------------------------------------------
 * config window show (public)
 *-----------------------------------------------------------------------------
 */

void show_editor_list_window(void)
{
	if (editor_list_window)
		{
		gtk_window_present(GTK_WINDOW(editor_list_window));
		return;
		}

	editor_list_window_create();
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
