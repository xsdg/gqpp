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

#ifndef CACHE_MAINT_H
#define CACHE_MAINT_H


void cache_maintain_home(gboolean metadata, gboolean clear, GtkWidget *parent);
void cache_notify_cb(FileData *fd, NotifyType type, gpointer data);
void cache_manager_show(void);

void cache_maintain_home_remote(gboolean metadata, gboolean clear, GDestroyNotify *func);
void cache_manager_standard_process_remote(gboolean clear);
void cache_manager_render_remote(const gchar *path, gboolean recurse, gboolean local, GDestroyNotify *func);
void cache_manager_sim_remote(const gchar *path, gboolean recurse, GDestroyNotify *func);
void cache_maintenance(const gchar *path);
#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
