/*
 * Copyright (C) 2022 The Geeqie Team
 *
 * Author: Omari Stephens
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

/*
 *-----------------------------------------------------------------------------
 * text conversion utils
 *-----------------------------------------------------------------------------
 */

gchar *text_from_size(gint64 size)
{
	gchar *a, *b;
	gchar *s, *d;
	gint l, n, i;

	/* what I would like to use is printf("%'d", size)
	 * BUT: not supported on every libc :(
	 */
	if (size > G_MAXINT)
		{
		/* the %lld conversion is not valid in all libcs, so use a simple work-around */
		a = g_strdup_printf("%d%09d", (guint)(size / 1000000000), (guint)(size % 1000000000));
		}
	else
		{
		a = g_strdup_printf("%d", (guint)size);
		}
	l = strlen(a);
	n = (l - 1)/ 3;
	if (n < 1) return a;

	b = g_new(gchar, l + n + 1);

	s = a;
	d = b;
	i = l - n * 3;
	while (*s != '\0')
		{
		if (i < 1)
			{
			i = 3;
			*d = ',';
			d++;
			}

		*d = *s;
		s++;
		d++;
		i--;
		}
	*d = '\0';

	g_free(a);
	return b;
}

gchar *text_from_size_abrev(gint64 size)
{
	if (size < (gint64)1024)
		{
		return g_strdup_printf(_("%d bytes"), (gint)size);
		}
	if (size < (gint64)1048576)
		{
		return g_strdup_printf(_("%.1f KiB"), (gdouble)size / 1024.0);
		}
	if (size < (gint64)1073741824)
		{
		return g_strdup_printf(_("%.1f MiB"), (gdouble)size / 1048576.0);
		}

	/* to avoid overflowing the gdouble, do division in two steps */
	size /= 1048576;
	return g_strdup_printf(_("%.1f GiB"), (gdouble)size / 1024.0);
}

/* note: returned string is valid until next call to text_from_time() */
const gchar *text_from_time(time_t t)
{
	static gchar *ret = NULL;
	gchar buf[128];
	gint buflen;
	struct tm *btime;
	GError *error = NULL;

	btime = localtime(&t);

	/* the %x warning about 2 digit years is not an error */
	buflen = strftime(buf, sizeof(buf), "%x %X", btime);
	if (buflen < 1) return "";

	g_free(ret);
	ret = g_locale_to_utf8(buf, buflen, NULL, NULL, &error);
	if (error)
		{
		log_printf("Error converting locale strftime to UTF-8: %s\n", error->message);
		g_error_free(error);
		return "";
		}

	return ret;
}

static gint file_data_sort_by_ext(gconstpointer a, gconstpointer b)
{
	const FileData *fda = a;
	const FileData *fdb = b;

	if (fda->sidecar_priority < fdb->sidecar_priority) return -1;
	if (fda->sidecar_priority > fdb->sidecar_priority) return 1;

	return strcmp(fdb->extension, fda->extension);
}

/*
 *-----------------------------------------------------------------------------
 * basename hash - grouping of sidecars in filelist
 *-----------------------------------------------------------------------------
 */


static GHashTable *file_data_basename_hash_new(void)
{
	return g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

static GList * file_data_basename_hash_insert(GHashTable *basename_hash, FileData *fd)
{
	GList *list;
	gchar *basename = g_strndup(fd->path, fd->extension - fd->path);

	list = g_hash_table_lookup(basename_hash, basename);

	if (!list)
		{
		DEBUG_1("TG: basename_hash not found for %s",fd->path);
		const gchar *parent_extension = registered_extension_from_path(basename);

		if (parent_extension)
			{
			DEBUG_1("TG: parent extension %s",parent_extension);
			gchar *parent_basename = g_strndup(basename, parent_extension - basename);
			DEBUG_1("TG: parent basename %s",parent_basename);
			FileData *parent_fd = g_hash_table_lookup(file_data_pool, basename);
			if (parent_fd)
				{
				DEBUG_1("TG: parent fd found");
				list = g_hash_table_lookup(basename_hash, parent_basename);
				if (!g_list_find(list, parent_fd))
					{
					DEBUG_1("TG: parent fd doesn't fit");
					g_free(parent_basename);
					list = NULL;
					}
				else
					{
					g_free(basename);
					basename = parent_basename;
					fd->extended_extension = g_strconcat(parent_extension, fd->extension, NULL);
					}
				}
			}
		}

	if (!g_list_find(list, fd))
		{
		list = g_list_insert_sorted(list, file_data_ref(fd), file_data_sort_by_ext);
		g_hash_table_insert(basename_hash, basename, list);
		}
	else
		{
		g_free(basename);
		}
	return list;
}

static void file_data_basename_hash_insert_cb(gpointer fd, gpointer basename_hash)
{
	file_data_basename_hash_insert((GHashTable *)basename_hash, (FileData *)fd);
}

static void file_data_basename_hash_remove_list(gpointer key, gpointer value, gpointer data)
{
	filelist_free((GList *)value);
}

static void file_data_basename_hash_free(GHashTable *basename_hash)
{
	g_hash_table_foreach(basename_hash, file_data_basename_hash_remove_list, NULL);
	g_hash_table_destroy(basename_hash);
}

static void file_data_basename_hash_to_sidecars(gpointer key, gpointer value, gpointer data)
{
	GList *basename_list = (GList *)value;
	file_data_check_sidecars(basename_list);
}


static gboolean is_hidden_file(const gchar *name)
{
	if (name[0] != '.') return FALSE;
	if (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')) return FALSE;
	return TRUE;
}

static gboolean file_data_can_write_directly(FileData *fd)
{
	return filter_name_is_writable(fd->extension);
}

static gboolean file_data_can_write_sidecar(FileData *fd)
{
	return filter_name_allow_sidecar(fd->extension) && !filter_name_is_writable(fd->extension);
}

gint file_data_get_user_orientation(FileData *fd)
{
	return fd->user_orientation;
}

void file_data_set_user_orientation(FileData *fd, gint value)
{
	if (fd->user_orientation == value) return;

	fd->user_orientation = value;
	file_data_increment_version(fd);
	file_data_send_notification(fd, NOTIFY_ORIENTATION);
}

gchar *file_data_get_error_string(gint error)
{
	GString *result = g_string_new("");

	if (error & CHANGE_NO_SRC)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("file or directory does not exist"));
		}

	if (error & CHANGE_DEST_EXISTS)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination already exists"));
		}

	if (error & CHANGE_NO_WRITE_PERM_DEST)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination can't be overwritten"));
		}

	if (error & CHANGE_WARN_NO_WRITE_PERM_DEST_DIR)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination directory is not writable"));
		}

	if (error & CHANGE_NO_DEST_DIR)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination directory does not exist"));
		}

	if (error & CHANGE_NO_WRITE_PERM_DIR)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("source directory is not writable"));
		}

	if (error & CHANGE_NO_READ_PERM)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("no read permission"));
		}

	if (error & CHANGE_WARN_NO_WRITE_PERM)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("file is readonly"));
		}

	if (error & CHANGE_WARN_DEST_EXISTS)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("destination already exists and will be overwritten"));
		}

	if (error & CHANGE_WARN_SAME)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("source and destination are the same"));
		}

	if (error & CHANGE_WARN_CHANGED_EXT)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("source and destination have different extension"));
		}

	if (error & CHANGE_WARN_UNSAVED_META)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("there are unsaved metadata changes for the file"));
		}

	if (error & CHANGE_DUPLICATE_DEST)
		{
		if (result->len > 0) g_string_append(result, ", ");
		g_string_append(result, _("another destination file has the same filename"));
		}

	return g_string_free(result, FALSE);
}

void file_data_set_page_num(FileData *fd, gint page_num)
{
	if (fd->page_total > 1 && page_num < 0)
		{
		fd->page_num = fd->page_total - 1;
		}
	else if (fd->page_total > 1 && page_num <= fd->page_total)
		{
		fd->page_num = page_num - 1;
		}
	else
		{
		fd->page_num = 0;
		}
	file_data_send_notification(fd, NOTIFY_REREAD);
}

void file_data_inc_page_num(FileData *fd)
{
	if (fd->page_total > 0 && fd->page_num < fd->page_total - 1)
		{
		fd->page_num = fd->page_num + 1;
		}
	else if (fd->page_total == 0)
		{
		fd->page_num = fd->page_num + 1;
		}
	file_data_send_notification(fd, NOTIFY_REREAD);
}

void file_data_dec_page_num(FileData *fd)
{
	if (fd->page_num > 0)
		{
		fd->page_num = fd->page_num - 1;
		}
	file_data_send_notification(fd, NOTIFY_REREAD);
}

void file_data_set_page_total(FileData *fd, gint page_total)
{
	fd->page_total = page_total;
}
