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

#include "pan-view-filter.h"

#include "image.h"
#include "metadata.h"
#include "pan-item.h"
#include "pan-util.h"
#include "pan-view.h"
#include "ui-fileops.h"
#include "ui-tabcomp.h"
#include "ui-misc.h"

PanViewFilterUi *pan_filter_ui_new(PanWindow *pw)
{
	PanViewFilterUi *ui = g_new0(PanViewFilterUi, 1);
	GtkWidget *combo;
	GtkWidget *hbox;
	gint i;

	/* Since we're using the GHashTable as a HashSet (in which key and value pointers
	 * are always identical), specifying key _and_ value destructor callbacks will
	 * cause a double-free.
	 */
	{
		GtkTreeIter iter;
		ui->filter_mode_model = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
		gtk_list_store_append(ui->filter_mode_model, &iter);
		gtk_list_store_set(ui->filter_mode_model, &iter,
				   0, PAN_VIEW_FILTER_REQUIRE, 1, _("Require"), 2, _("R"), -1);
		gtk_list_store_append(ui->filter_mode_model, &iter);
		gtk_list_store_set(ui->filter_mode_model, &iter,
				   0, PAN_VIEW_FILTER_EXCLUDE, 1, _("Exclude"), 2, _("E"), -1);
		gtk_list_store_append(ui->filter_mode_model, &iter);
		gtk_list_store_set(ui->filter_mode_model, &iter,
				   0, PAN_VIEW_FILTER_INCLUDE, 1, _("Include"), 2, _("I"), -1);
		gtk_list_store_append(ui->filter_mode_model, &iter);
		gtk_list_store_set(ui->filter_mode_model, &iter,
				   0, PAN_VIEW_FILTER_GROUP, 1, _("Group"), 2, _("G"), -1);

		ui->filter_mode_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ui->filter_mode_model));
		gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(ui->filter_mode_combo), FALSE);
		gtk_combo_box_set_active(GTK_COMBO_BOX(ui->filter_mode_combo), 0);

		GtkCellRenderer *render = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ui->filter_mode_combo), render, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(ui->filter_mode_combo), render, "text", 1, NULL);
	}

	// Build the actual filter UI.
	ui->filter_box = gtk_hbox_new(FALSE, PREF_PAD_SPACE);
	pref_spacer(ui->filter_box, 0);
	pref_label_new(ui->filter_box, _("Keyword Filter:"));

	gtk_box_pack_start(GTK_BOX(ui->filter_box), ui->filter_mode_combo, FALSE, FALSE, 0);
	gtk_widget_show(ui->filter_mode_combo);

	hbox = gtk_hbox_new(TRUE, PREF_PAD_SPACE);
	gtk_box_pack_start(GTK_BOX(ui->filter_box), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	combo = tab_completion_new_with_history(&ui->filter_entry, "", "pan_view_filter", -1,
						pan_filter_activate_cb, pw);
	gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
	gtk_widget_show(combo);

	ui->filter_label = gtk_label_new("");/** @todo (xsdg): Figure out whether it's useful to keep this label around. */
	//gtk_box_pack_start(GTK_BOX(hbox), ui->filter_label, FALSE, FALSE, 0);
	//gtk_widget_show(ui->filter_label);

	ui->filter_kw_hbox = gtk_hbox_new(FALSE, PREF_PAD_SPACE);
	gtk_box_pack_start(GTK_BOX(hbox), ui->filter_kw_hbox, TRUE, TRUE, 0);
	gtk_widget_show(ui->filter_kw_hbox);

	// Build the spin-button to show/hide the filter UI.
	ui->filter_button = gtk_toggle_button_new();
	gtk_button_set_relief(GTK_BUTTON(ui->filter_button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(ui->filter_button), FALSE);
	hbox = gtk_hbox_new(FALSE, PREF_PAD_GAP);
	gtk_container_add(GTK_CONTAINER(ui->filter_button), hbox);
	gtk_widget_show(hbox);
	ui->filter_button_arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_NONE);
	gtk_box_pack_start(GTK_BOX(hbox), ui->filter_button_arrow, FALSE, FALSE, 0);
	gtk_widget_show(ui->filter_button_arrow);
	pref_label_new(hbox, _("Filter"));

	g_signal_connect(G_OBJECT(ui->filter_button), "clicked",
			 G_CALLBACK(pan_filter_toggle_cb), pw);

	// Add check buttons for filtering by image class
	for (i = 0; i < FILE_FORMAT_CLASSES; i++)
	{
		ui->filter_check_buttons[i] = gtk_check_button_new_with_label(_(format_class_list[i]));
		gtk_box_pack_start(GTK_BOX(ui->filter_box), ui->filter_check_buttons[i], FALSE, FALSE, 0);
		gtk_widget_show(ui->filter_check_buttons[i]);
	}

	gtk_toggle_button_set_active((GtkToggleButton *)ui->filter_check_buttons[FORMAT_CLASS_IMAGE], TRUE);
	gtk_toggle_button_set_active((GtkToggleButton *)ui->filter_check_buttons[FORMAT_CLASS_RAWIMAGE], TRUE);
	gtk_toggle_button_set_active((GtkToggleButton *)ui->filter_check_buttons[FORMAT_CLASS_VIDEO], TRUE);
	ui->filter_classes = (1 << FORMAT_CLASS_IMAGE) | (1 << FORMAT_CLASS_RAWIMAGE) | (1 << FORMAT_CLASS_VIDEO);

	// Connecting the signal before setting the state causes segfault as pw is not yet prepared
	for (i = 0; i < FILE_FORMAT_CLASSES; i++)
		g_signal_connect((GtkToggleButton *)(ui->filter_check_buttons[i]), "toggled", G_CALLBACK(pan_filter_toggle_button_cb), pw);

	return ui;
}

void pan_filter_ui_destroy(PanViewFilterUi **ui_ptr)
{
	if (ui_ptr == NULL || *ui_ptr == NULL) return;

	// Note that g_clear_pointer handles already-NULL pointers.
	//g_clear_pointer(&(*ui_ptr)->filter_kw_table, g_hash_table_destroy);

	g_free(*ui_ptr);
	*ui_ptr = NULL;
}

static void pan_filter_status(PanWindow *pw, const gchar *text)
{
	gtk_label_set_text(GTK_LABEL(pw->filter_ui->filter_label), (text) ? text : "");
}

static void pan_filter_kw_button_cb(GtkButton *widget, gpointer data)
{
	PanFilterCallbackState *cb_state = data;
	PanWindow *pw = cb_state->pw;
	PanViewFilterUi *ui = pw->filter_ui;

	/** @todo (xsdg): Fix filter element pointed object memory leak. */
	ui->filter_elements = g_list_delete_link(ui->filter_elements, cb_state->filter_element);
	gtk_widget_destroy(GTK_WIDGET(widget));
	g_free(cb_state);

	pan_filter_status(pw, _("Removed keyword…"));
	pan_layout_update(pw);
}

void pan_filter_activate_cb(const gchar *text, gpointer data)
{
	GtkWidget *kw_button;
	PanWindow *pw = data;
	PanViewFilterUi *ui = pw->filter_ui;
	GtkTreeIter iter;

	if (!text) return;

	// Get all relevant state and reset UI.
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(ui->filter_mode_combo), &iter);
	gtk_entry_set_text(GTK_ENTRY(ui->filter_entry), "");
	tab_completion_append_to_history(ui->filter_entry, text);

	// Add new filter element.
	PanViewFilterElement *element = g_new0(PanViewFilterElement, 1);
	gtk_tree_model_get(GTK_TREE_MODEL(ui->filter_mode_model), &iter, 0, &element->mode, -1);
	element->keyword = g_strdup(text);
	if (g_strcmp0(text, g_regex_escape_string(text, -1)))
		{
		// It's an actual regex, so compile
		element->kw_regex = g_regex_new(text, G_REGEX_ANCHORED | G_REGEX_OPTIMIZE, G_REGEX_MATCH_ANCHORED, NULL);
		}
	ui->filter_elements = g_list_append(ui->filter_elements, element);

	// Get the short version of the mode value.
	gchar *short_mode;
	gtk_tree_model_get(GTK_TREE_MODEL(ui->filter_mode_model), &iter, 2, &short_mode, -1);

	// Create the button.
	/** @todo (xsdg): Use MVC so that the button list is an actual representation of the GList */
	gchar *label = g_strdup_printf("(%s) %s", short_mode, text);
	kw_button = gtk_button_new_with_label(label);
	g_clear_pointer(&label, g_free);

	gtk_box_pack_start(GTK_BOX(ui->filter_kw_hbox), kw_button, FALSE, FALSE, 0);
	gtk_widget_show(kw_button);

	PanFilterCallbackState *cb_state = g_new0(PanFilterCallbackState, 1);
	cb_state->pw = pw;
	cb_state->filter_element = g_list_last(ui->filter_elements);

	g_signal_connect(G_OBJECT(kw_button), "clicked",
			 G_CALLBACK(pan_filter_kw_button_cb), cb_state);

	pan_layout_update(pw);
}

void pan_filter_activate(PanWindow *pw)
{
	gchar *text;

	text = g_strdup(gtk_entry_get_text(GTK_ENTRY(pw->filter_ui->filter_entry)));
	pan_filter_activate_cb(text, pw);
	g_free(text);
}

void pan_filter_toggle_cb(GtkWidget *button, gpointer data)
{
	PanWindow *pw = data;
	PanViewFilterUi *ui = pw->filter_ui;
	gboolean visible;

	visible = gtk_widget_get_visible(ui->filter_box);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == visible) return;

	if (visible)
		{
		gtk_widget_hide(ui->filter_box);
		gtk_arrow_set(GTK_ARROW(ui->filter_button_arrow), GTK_ARROW_UP, GTK_SHADOW_NONE);
		}
	else
		{
		gtk_widget_show(ui->filter_box);
		gtk_arrow_set(GTK_ARROW(ui->filter_button_arrow), GTK_ARROW_DOWN, GTK_SHADOW_NONE);
		gtk_widget_grab_focus(ui->filter_entry);
		}
}

void pan_filter_toggle_visible(PanWindow *pw, gboolean enable)
{
	PanViewFilterUi *ui = pw->filter_ui;
	if (pw->fs) return;

	if (enable)
		{
		if (gtk_widget_get_visible(ui->filter_box))
			{
			gtk_widget_grab_focus(ui->filter_entry);
			}
		else
			{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->filter_button), TRUE);
			}
		}
	else
		{
		if (gtk_widget_get_visible(ui->filter_entry))
			{
			if (gtk_widget_has_focus(ui->filter_entry))
				{
				gtk_widget_grab_focus(GTK_WIDGET(pw->imd->widget));
				}
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui->filter_button), FALSE);
			}
		}
}

void pan_filter_toggle_button_cb(GtkWidget *UNUSED(button), gpointer data)
{
	PanWindow *pw = data;
	PanViewFilterUi *ui = pw->filter_ui;

	gint old_classes = ui->filter_classes;
	ui->filter_classes = 0;

	for (gint i = 0; i < FILE_FORMAT_CLASSES; i++)
	{
		ui->filter_classes |= gtk_toggle_button_get_active((GtkToggleButton *)ui->filter_check_buttons[i]) ? 1 << i : 0;
	}

	if (ui->filter_classes != old_classes) 
		pan_layout_update(pw);
}

static gboolean pan_view_list_contains_kw_pattern(GList *haystack, PanViewFilterElement *filter, gchar **found_kw)
{
	if (filter->kw_regex)
		{
		// regex compile succeeded; attempt regex match.
		GList *work = g_list_first(haystack);
		while (work)
			{
			gchar *keyword = work->data;
			work = work->next;
			if (g_regex_match(filter->kw_regex, keyword, 0x0, NULL))
				{
				if (found_kw) *found_kw = keyword;
				return TRUE;
				}
			}
		return FALSE;
		}
	else
		{
		// regex compile failed; fall back to exact string match.
		GList *found_elem = g_list_find_custom(haystack, filter->keyword, (GCompareFunc)g_strcmp0);
		if (found_elem && found_kw) *found_kw = found_elem->data;
		return !!found_elem;
		}
}

gboolean pan_filter_fd_list(GList **fd_list, GList *filter_elements, gint filter_classes)
{
	GList *work;
	gboolean modified = FALSE;
	GHashTable *seen_kw_table = NULL;

	if (!fd_list || !*fd_list) return modified;

	// seen_kw_table is only valid in this scope, so don't take ownership of any strings.
	if (filter_elements)
		seen_kw_table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);

	work = *fd_list;
	while (work)
		{
		FileData *fd = work->data;
		GList *last_work = work;
		work = work->next;

		gboolean should_reject = FALSE;
		gchar *group_kw = NULL;

		if (!((1 << fd -> format_class) & filter_classes))
			{
			should_reject = TRUE;
			}
		else if (filter_elements)
			{
			/** @todo (xsdg): OPTIMIZATION Do the search inside of metadata.c to avoid a bunch of string list copies. */
			GList *img_keywords = metadata_read_list(fd, KEYWORD_KEY, METADATA_PLAIN);

			/** @todo (xsdg): OPTIMIZATION Determine a heuristic for when to linear-search the keywords list, and when to build a hash table for the image's keywords. */
			GList *filter_element = filter_elements;

			while (filter_element)
				{
				PanViewFilterElement *filter = filter_element->data;
				filter_element = filter_element->next;
				gchar *found_kw = NULL;
				gboolean has_kw = pan_view_list_contains_kw_pattern(img_keywords, filter, &found_kw);

				switch (filter->mode)
					{
					case PAN_VIEW_FILTER_REQUIRE:
						should_reject |= !has_kw;
						break;
					case PAN_VIEW_FILTER_EXCLUDE:
						should_reject |= has_kw;
						break;
					case PAN_VIEW_FILTER_INCLUDE:
						if (has_kw) should_reject = FALSE;
						break;
					case PAN_VIEW_FILTER_GROUP:
						if (has_kw)
							{
							if (g_hash_table_contains(seen_kw_table, found_kw))
								{
								should_reject = TRUE;
								}
							else if (group_kw == NULL)
								{
								group_kw = found_kw;
								}
							}
						break;
					}
				}
			string_list_free(img_keywords);
			if (!should_reject && group_kw != NULL) g_hash_table_add(seen_kw_table, group_kw);
			group_kw = NULL;  // group_kw references an item from img_keywords.
			}

		if (should_reject)
			{
			*fd_list = g_list_delete_link(*fd_list, last_work);
			modified = TRUE;
			}
		}

	if (filter_elements)
		g_hash_table_destroy(seen_kw_table);

	return modified;
}
