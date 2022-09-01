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

#ifndef UI_HELP_H
#define UI_HELP_H


GtkWidget *help_window_new(const gchar *title,
			   const gchar *subclass,
			   const gchar *path, const gchar *key);
void help_window_set_file(GtkWidget *window, const gchar *path, const gchar *key);
void help_window_set_key(GtkWidget *window, const gchar *key);

GtkWidget *help_window_get_box(GtkWidget *window);


#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
