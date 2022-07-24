/*
 * Copyright (C) 2008 - 2016 The Geeqie Team
 *
 * Authors: Vladimir Nadvornik, Laurent Monin
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
#include "debug.h"

#include "ifiledata.h"  // TODO(xsdg): should not be here.

#include <glib/gprintf.h>

/*
 * Logging functions
 */

static gboolean log_msg_cb(gpointer data)
{
	gchar *buf = data;
    puts(buf);
	g_free(buf);
	return FALSE;
}

/**
 * @brief Appends a user information message to the log window queue
 * @param data The message
 * @returns FALSE
 * 
 * If the first word of the message is either "error" or "warning"
 * (case insensitive) the message is color-coded appropriately
 */
static gboolean log_normal_cb(gpointer data)
{
	gchar *buf = data;
    puts(buf);

	g_free(buf);
	return FALSE;
}

void log_domain_print_message(const gchar *domain, gchar *buf)
{
	gchar *buf_nl;

	buf_nl = g_strconcat(buf, "\n", NULL);

    puts(buf_nl);
    if (strcmp(domain, DOMAIN_INFO) == 0)
        g_idle_add(log_normal_cb, buf_nl);
    else
        g_idle_add(log_msg_cb, buf_nl);

    g_free(buf_nl);
	g_free(buf);
}

void log_domain_print_debug(const gchar *domain, const gchar *file_name, const gchar *function_name,
									int line_number, const gchar *format, ...)
{
	va_list ap;
	gchar *message;
	gchar *location;
	gchar *buf;

	va_start(ap, format);
	message = g_strdup_vprintf(format, ap);
	va_end(ap);

    location = g_strdup_printf("%s:%s:%d:", file_name, function_name, line_number);

	buf = g_strconcat(location, message, NULL);
	log_domain_print_message(domain,buf);
	g_free(location);
	g_free(message);
}

void log_domain_printf(const gchar *domain, const gchar *format, ...)
{
	va_list ap;
	gchar *buf;

	va_start(ap, format);
	buf = g_strdup_vprintf(format, ap);
	va_end(ap);

	log_domain_print_message(domain, buf);
}
GHashTable *FileData::file_data_pool = NULL;

/*
 * Debugging only functions
 */

#ifdef DEBUG

static gint debug_level = DEBUG_LEVEL_MIN;


gint get_debug_level(void)
{
	return debug_level;
}

void set_debug_level(gint new_level)
{
	debug_level = CLAMP(new_level, DEBUG_LEVEL_MIN, DEBUG_LEVEL_MAX);
}

void debug_level_add(gint delta)
{
	set_debug_level(debug_level + delta);
}

gint required_debug_level(gint level)
{
	return (debug_level >= level);
}

static gint timeval_delta(struct timeval *result, struct timeval *x, struct timeval *y)
{
	if (x->tv_usec < y->tv_usec)
		{
		gint nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
		}

	if (x->tv_usec - y->tv_usec > 1000000)
		{
		gint nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	return x->tv_sec < y->tv_sec;
}

const gchar *get_exec_time(void)
{
	static gchar timestr[30];
	static struct timeval start_tv = {0, 0};
	static struct timeval previous = {0, 0};
	static gint started = 0;

	struct timeval tv = {0, 0};
	static struct timeval delta = {0, 0};

	gettimeofday(&tv, NULL);

	if (start_tv.tv_sec == 0) start_tv = tv;

	tv.tv_sec -= start_tv.tv_sec;
	if (tv.tv_usec >= start_tv.tv_usec)
		tv.tv_usec -= start_tv.tv_usec;
	else
		{
		tv.tv_usec += 1000000 - start_tv.tv_usec;
		tv.tv_sec -= 1;
		}

	if (started) timeval_delta(&delta, &tv, &previous);

	previous = tv;
	started = 1;

	g_snprintf(timestr, sizeof(timestr), "%5d.%06d (+%05d.%06d)", (gint)tv.tv_sec, (gint)tv.tv_usec, (gint)delta.tv_sec, (gint)delta.tv_usec);

	return timestr;
}

void init_exec_time(void)
{
	get_exec_time();
}

void set_regexp(gchar *cmd_regexp) {}

gchar *get_regexp(void)
{
    return NULL;
}




// TODO(xsdg): Move these mocks to somewhere more reasonable.
const gchar *registered_extension_from_path(const gchar *name)
{
    gchar *dot = strrchr(name, '.');
    if (!dot) return NULL;
    return (dot + 1);
}

// Memory leak!
void filelist_free(GList *list) {}
void file_data_send_notification(FileData *fd, NotifyType type) {}
gboolean filter_name_is_writable(const gchar *name)
{
    return TRUE;
}
gboolean filter_name_allow_sidecar(const gchar *name)
{
    return TRUE;
}

FileData *FileData::file_data_ref_debug(const gchar *file, gint line, FileData *fd)
{
    return fd;
}

void FileData::file_data_increment_version(FileData *fd) {}
void FileData::file_data_send_notification(FileData *fd, NotifyType type) {}


#endif /* DEBUG */
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
