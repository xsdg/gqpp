/*
 * Copyright (C) 2008 - 2025 The Geeqie Team
 *
 * Author: Colin Clark
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

#include "secure-save.h"

#include <gio/gio.h>
#include <sys/stat.h>

#include "intl.h"

/**
 * @brief Save file in a safe manner
 * @param file_name 
 * @param contents 
 * @param length Length of contents or -1 if null-terminated string
 * @returns 
 * 
 * The operation is atomic in the sense that it is first written to a
 * temporary file which is then renamed to the final name.
 * After a failure, either the old version of the file or the
 * new version of the file will be available, but not a mixture.
 *
 * If the file already exists, mode and group are preserved.
 */
gboolean secure_save(const gchar *file_name, const gchar *contents, gsize length)
{
	GError *error = nullptr;
	gboolean ret = TRUE;
	guint permissions = 0;
	guint32 gid = 0;
	guint32 uid = 0;
	struct stat st;

	GApplication *app = g_application_get_default();

	mode_t saved_mask;
	const mode_t mask = S_IXUSR | S_IRWXG | S_IRWXO;
	saved_mask = umask(mask);

	if (stat(file_name, &st) == 0)
		{
		permissions = st.st_mode & 0777;
		gid = st.st_gid;
		uid = st.st_uid;
		}
	else
		{
		permissions = 0600;
		gid = getuid();
		uid = getuid();
		}

	g_autoptr(GNotification) notification = g_notification_new("Geeqie");
	g_notification_set_priority(notification, G_NOTIFICATION_PRIORITY_URGENT);
	g_notification_set_default_action(notification, "app.null");
	g_notification_set_title(notification, _("File was not saved"));

	if (!g_file_set_contents_full(file_name, contents, length, (GFileSetContentsFlags)(G_FILE_SET_CONTENTS_CONSISTENT | G_FILE_SET_CONTENTS_DURABLE), 0600, &error))
		{
		if (error)
			{
			g_notification_set_body(notification, error->message);
			g_application_send_notification(G_APPLICATION(app), "save-file-notification", notification);

			log_printf("Error: Failed to save file: %s\n%s", file_name, error->message);

			g_error_free(error);
			error=nullptr;

			ret = FALSE;
			}
		else
			{
			g_notification_set_body(notification, file_name);
			g_application_send_notification(G_APPLICATION(app), "save-file-notification", notification);
			log_printf("Error: Failed to save file: %s", file_name);

			ret = FALSE;
			}
		}

	if (ret)
		{
		if (chown(file_name, uid, gid) == -1)
			{
			log_printf("secure_save: chown failed");
			}

		if (chmod(file_name, permissions) == -1)
			{
			log_printf("secure_save: chmod failed");
			}
		}

	umask(saved_mask);

	return ret;
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
