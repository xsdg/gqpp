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

#include <gdk/gdkkeysyms.h> /* for keyboard values */

#include <signal.h>
#include <sys/mman.h>

#include <math.h>
#ifdef G_OS_UNIX
#include <pwd.h>
#endif
#include <locale.h>

#include "main.h"

#include "cache.h"
#include "collect.h"
#include "collect-io.h"
#include "filedata.h"
#include "filefilter.h"
#include "history_list.h"
#include "image.h"
#include "image-overlay.h"
#include "img-view.h"
#include "layout.h"
#include "layout_image.h"
#include "layout_util.h"
#include "misc.h"
#include "options.h"
#include "rcfile.h"
#include "remote.h"
#include "secure_save.h"
#include "similar.h"
#include "ui_fileops.h"
#include "ui_utildlg.h"
#include "cache_maint.h"
#include "thumb.h"
#include "metadata.h"
#include "editors.h"
#include "exif.h"
#include "histogram.h"
#include "pixbuf_util.h"
#include "glua.h"
#include "whereami.h"

#ifdef HAVE_CLUTTER
#include <clutter-gtk/clutter-gtk.h>
#endif

gboolean thumb_format_changed = FALSE;
static RemoteConnection *remote_connection = NULL;

gchar *gq_prefix;
gchar *gq_localedir;
gchar *gq_helpdir;
gchar *gq_htmldir;
gchar *gq_appdir;
gchar *gq_bindir;
gchar *gq_executable_path;
gchar *desktop_file_template;
gchar *instance_identifier;

/*
 *-----------------------------------------------------------------------------
 * keyboard functions
 *-----------------------------------------------------------------------------
 */

void keyboard_scroll_calc(gint *x, gint *y, GdkEventKey *event)
{
	static gint delta = 0;
	static guint32 time_old = 0;
	static guint keyval_old = 0;

	if (event->state & GDK_CONTROL_MASK)
		{
		if (*x < 0) *x = G_MININT / 2;
		if (*x > 0) *x = G_MAXINT / 2;
		if (*y < 0) *y = G_MININT / 2;
		if (*y > 0) *y = G_MAXINT / 2;

		return;
		}

	if (options->progressive_key_scrolling)
		{
		guint32 time_diff;

		time_diff = event->time - time_old;

		/* key pressed within 125ms ? (1/8 second) */
		if (time_diff > 125 || event->keyval != keyval_old) delta = 0;

		time_old = event->time;
		keyval_old = event->keyval;

		delta += 2;
		}
	else
		{
		delta = 8;
		}

	*x = *x * delta * options->keyboard_scroll_step;
	*y = *y * delta * options->keyboard_scroll_step;
}



/*
 *-----------------------------------------------------------------------------
 * command line parser (private) hehe, who needs popt anyway?
 *-----------------------------------------------------------------------------
 */

static void parse_command_line_add_file(const gchar *file_path, gchar **path, gchar **file,
					GList **list, GList **collection_list)
{
	gchar *path_parsed;

	path_parsed = g_strdup(file_path);
	parse_out_relatives(path_parsed);

	if (file_extension_match(path_parsed, GQ_COLLECTION_EXT))
		{
		*collection_list = g_list_append(*collection_list, path_parsed);
		}
	else
		{
		if (!*path) *path = remove_level_from_path(path_parsed);
		if (!*file) *file = g_strdup(path_parsed);
		*list = g_list_prepend(*list, path_parsed);
		}
}

static void parse_command_line_add_dir(const gchar *dir, gchar **UNUSED(path), gchar **UNUSED(file),
				       GList **UNUSED(list))
{
#if 0
	/* This is broken because file filter is not initialized yet.
	*/
	GList *files;
	gchar *path_parsed;
	FileData *dir_fd;

	path_parsed = g_strdup(dir);
	parse_out_relatives(path_parsed);
	dir_fd = file_data_new_dir(path_parsed);


	if (filelist_read(dir_fd, &files, NULL))
		{
		GList *work;

		files = filelist_filter(files, FALSE);
		files = filelist_sort_path(files);

		work = files;
		while (work)
			{
			FileData *fd = work->data;
			if (!*path) *path = remove_level_from_path(fd->path);
			if (!*file) *file = g_strdup(fd->path);
			*list = g_list_prepend(*list, fd);

			work = work->next;
			}

		g_list_free(files);
		}

	g_free(path_parsed);
	file_data_unref(dir_fd);
#else
	DEBUG_1("multiple directories specified, ignoring: %s", dir);
#endif
}

static void parse_command_line_process_dir(const gchar *dir, gchar **path, gchar **file,
					   GList **list, gchar **first_dir)
{

	if (!*list && !*first_dir)
		{
		*first_dir = g_strdup(dir);
		}
	else
		{
		if (*first_dir)
			{
			parse_command_line_add_dir(*first_dir, path, file, list);
			g_free(*first_dir);
			*first_dir = NULL;
			}
		parse_command_line_add_dir(dir, path, file, list);
		}
}

static void parse_command_line_process_file(const gchar *file_path, gchar **path, gchar **file,
					    GList **list, GList **collection_list, gchar **first_dir)
{

	if (*first_dir)
		{
		parse_command_line_add_dir(*first_dir, path, file, list);
		g_free(*first_dir);
		*first_dir = NULL;
		}
	parse_command_line_add_file(file_path, path, file, list, collection_list);
}

static void parse_command_line(gint argc, gchar *argv[])
{
	GList *list = NULL;
	GList *remote_list = NULL;
	GList *remote_errors = NULL;
	gboolean remote_do = FALSE;
	gchar *first_dir = NULL;
	gchar *app_lock;
	gchar *pwd;
	gchar *current_dir;
	gchar *geometry = NULL;
	GtkWidget *dialog_warning;
	GString *command_line_errors = g_string_new(NULL);

	command_line = g_new0(CommandLine, 1);

	command_line->argc = argc;
	command_line->argv = argv;
	command_line->regexp = NULL;

	if (argc > 1)
		{
		gint i;
		gchar *base_dir = get_current_dir();
		i = 1;
		while (i < argc)
			{
			gchar *cmd_line = path_to_utf8(argv[i]);
			gchar *cmd_all = g_build_filename(base_dir, cmd_line, NULL);

			if (cmd_line[0] == G_DIR_SEPARATOR && isdir(cmd_line))
				{
				parse_command_line_process_dir(cmd_line, &command_line->path, &command_line->file, &list, &first_dir);
				}
			else if (isdir(cmd_all))
				{
				parse_command_line_process_dir(cmd_all, &command_line->path, &command_line->file, &list, &first_dir);
				}
			else if (cmd_line[0] == G_DIR_SEPARATOR && isfile(cmd_line))
				{
				parse_command_line_process_file(cmd_line, &command_line->path, &command_line->file,
								&list, &command_line->collection_list, &first_dir);
				}
			else if (isfile(cmd_all))
				{
				parse_command_line_process_file(cmd_all, &command_line->path, &command_line->file,
								&list, &command_line->collection_list, &first_dir);
				}
			else if (download_web_file(cmd_line, FALSE, NULL))
				{
				}
			else if (is_collection(cmd_line))
				{
				gchar *path = NULL;

				path = collection_path(cmd_line);
				parse_command_line_process_file(path, &command_line->path, &command_line->file,
								&list, &command_line->collection_list, &first_dir);
				g_free(path);
				}
			else if (strncmp(cmd_line, "--debug", 7) == 0 && (cmd_line[7] == '\0' || cmd_line[7] == '='))
				{
				/* do nothing but do not produce warnings */
				}
			else if (strncmp(cmd_line, "--disable-clutter", 17) == 0 && (cmd_line[17] == '\0'))
				{
				/* do nothing but do not produce warnings */
				}
			else if (strcmp(cmd_line, "+t") == 0 ||
				 strcmp(cmd_line, "--with-tools") == 0)
				{
				command_line->tools_show = TRUE;

				remote_list = g_list_append(remote_list, "+t");
				}
			else if (strcmp(cmd_line, "-t") == 0 ||
				 strcmp(cmd_line, "--without-tools") == 0)
				{
				command_line->tools_hide = TRUE;

				remote_list = g_list_append(remote_list, "-t");
				}
			else if (strcmp(cmd_line, "-f") == 0 ||
				 strcmp(cmd_line, "--fullscreen") == 0)
				{
				command_line->startup_full_screen = TRUE;
				}
			else if (strcmp(cmd_line, "-s") == 0 ||
				 strcmp(cmd_line, "--slideshow") == 0)
				{
				command_line->startup_in_slideshow = TRUE;
				}
			else if (strcmp(cmd_line, "-l") == 0 ||
				 strcmp(cmd_line, "--list") == 0)
				{
				command_line->startup_command_line_collection = TRUE;
				}
			else if (strncmp(cmd_line, "--geometry=", 11) == 0)
				{
				if (!command_line->geometry) command_line->geometry = g_strdup(cmd_line + 11);
				}
			else if (strcmp(cmd_line, "-r") == 0 ||
				 strcmp(cmd_line, "--remote") == 0)
				{
				if (!remote_do)
					{
					remote_do = TRUE;
					remote_list = remote_build_list(remote_list, argc - i, &argv[i], &remote_errors);
					}
				}
			else if ((strcmp(cmd_line, "+w") == 0) ||
						strcmp(cmd_line, "--show-log-window") == 0)
				{
				command_line->log_window_show = TRUE;
				}
			else if (strncmp(cmd_line, "-o:", 3) == 0)
				{
				command_line->log_file = g_strdup(cmd_line + 3);
				}
			else if (strncmp(cmd_line, "--log-file:", 11) == 0)
				{
				command_line->log_file = g_strdup(cmd_line + 11);
				}
			else if (strncmp(cmd_line, "-g:", 3) == 0)
				{
				set_regexp(g_strdup(cmd_line+3));
				}
			else if (strncmp(cmd_line, "-grep:", 6) == 0)
				{
				set_regexp(g_strdup(cmd_line+3));
				}
			else if (strncmp(cmd_line, "-n", 2) == 0)
				{
				command_line->new_instance = TRUE;
				}
			else if (strncmp(cmd_line, "--new-instance", 14) == 0)
				{
				command_line->new_instance = TRUE;
				}
			else if (strcmp(cmd_line, "-rh") == 0 ||
				 strcmp(cmd_line, "--remote-help") == 0)
				{
				remote_help();
				exit(0);
				}
			else if (strcmp(cmd_line, "--blank") == 0)
				{
				command_line->startup_blank = TRUE;
				}
			else if (strcmp(cmd_line, "-v") == 0 ||
				 strcmp(cmd_line, "--version") == 0)
				{
				printf_term(FALSE, "%s %s GTK%d\n", GQ_APPNAME, VERSION, gtk_major_version);
				exit(0);
				}
			else if (strcmp(cmd_line, "--alternate") == 0)
				{
				/* enable faster experimental algorithm */
				log_printf("Alternate similarity algorithm enabled\n");
				image_sim_alternate_set(TRUE);
				}
			else if (strcmp(cmd_line, "-h") == 0 ||
				 strcmp(cmd_line, "--help") == 0)
				{
				printf_term(FALSE, "%s %s\n", GQ_APPNAME, VERSION);
				printf_term(FALSE, _("Usage: %s [options] [path]\n\n"), GQ_APPNAME_LC);
				print_term(FALSE, _("Valid options:\n"));
				print_term(FALSE, _("      --blank                      start with blank file list\n"));
				print_term(FALSE, _("      --cache-maintenance <path>   run cache maintenance in non-GUI mode\n"));
				print_term(FALSE, _("      --disable-clutter            disable use of Clutter library (i.e. GPU accel.)\n"));
				print_term(FALSE, _("  -f, --fullscreen                 start in full screen mode\n"));
				print_term(FALSE, _("      --geometry=WxH+XOFF+YOFF     set main window location\n"));
				print_term(FALSE, _("  -h, --help                       show this message\n"));
				print_term(FALSE, _("  -l, --list [files] [collections] open collection window for command line\n"));
				print_term(FALSE, _("  -n, --new-instance               open a new instance of Geeqie\n"));
				print_term(FALSE, _("  -o:, --log-file:<file>     save log data to file\n"));
				print_term(FALSE, _("  -r, --remote                     send following commands to open window\n"));
				print_term(FALSE, _("  -rh, --remote-help               print remote command list\n"));
				print_term(FALSE, _("  -s, --slideshow                  start in slideshow mode\n"));
				print_term(FALSE, _("  +t, --with-tools                 force show of tools\n"));
				print_term(FALSE, _("  -t, --without-tools              force hide of tools\n"));
				print_term(FALSE, _("  -v, --version                    print version info\n"));
				print_term(FALSE, _("  +w, --show-log-window            show log window\n"));
#ifdef DEBUG
				print_term(FALSE, _("      --debug[=level]              turn on debug output\n"));
				print_term(FALSE, _("  -g:, --grep:<regexp>     filter debug output\n"));
#endif

#if 0
				/* these options are not officially supported!
				 * only for testing new features, no need to translate them */
				print_term(FALSE, "  --alternate                use alternate similarity algorithm\n");
#endif
				print_term(FALSE, "\n");

				remote_help();


				exit(0);
				}
			else if (!remote_do)
				{
				command_line_errors = g_string_append(command_line_errors, cmd_line);
				command_line_errors = g_string_append(command_line_errors, "\n");
				}

			g_free(cmd_all);
			g_free(cmd_line);
			i++;
			}

		if (command_line_errors->len > 0)
			{
			dialog_warning = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", "Invalid parameter(s):");
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog_warning), "%s", command_line_errors->str);
			gtk_window_set_title(GTK_WINDOW(dialog_warning), GQ_APPNAME);
			gtk_window_set_keep_above(GTK_WINDOW(dialog_warning), TRUE);
			gtk_dialog_run(GTK_DIALOG(dialog_warning));
			gtk_widget_destroy(dialog_warning);
			g_string_free(command_line_errors, TRUE);

			exit(EXIT_FAILURE);
			}

		g_free(base_dir);
		parse_out_relatives(command_line->path);
		parse_out_relatives(command_line->file);
		}

	list = g_list_reverse(list);

	if (!command_line->path && first_dir)
		{
		command_line->path = first_dir;
		first_dir = NULL;

		parse_out_relatives(command_line->path);
		}
	g_free(first_dir);

	if (!command_line->new_instance)
		{
		/* If Geeqie is already running, prevent a second instance
		 * from being started. Open a new window instead.
		 */
		app_lock = g_build_filename(get_rc_dir(), ".command", NULL);
		if (remote_server_exists(app_lock) && !remote_do)
			{
			remote_do = TRUE;
			if (command_line->geometry)
				{
				geometry = g_strdup_printf("--geometry=%s", command_line->geometry);
				remote_list = g_list_prepend(remote_list, geometry);
				}
			remote_list = g_list_prepend(remote_list, "--new-window");
			}
		g_free(app_lock);
		}

	if (remote_do)
		{
		if (remote_errors)
			{
			GList *work = remote_errors;

			while (work)
				{
				gchar *opt = work->data;

				command_line_errors = g_string_append(command_line_errors, opt);
				command_line_errors = g_string_append(command_line_errors, "\n");
				work = work->next;
				}

			dialog_warning = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", "Invalid parameter(s):");
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog_warning), "%s", command_line_errors->str);
			gtk_window_set_title(GTK_WINDOW(dialog_warning), GQ_APPNAME);
			gtk_window_set_keep_above(GTK_WINDOW(dialog_warning), TRUE);
			gtk_dialog_run(GTK_DIALOG(dialog_warning));
			gtk_widget_destroy(dialog_warning);
			g_string_free(command_line_errors, TRUE);

			exit(EXIT_FAILURE);
			}

		/* prepend the current dir the remote command was made from,
		 * for use by any remote command that needs it
		 */
		current_dir = g_get_current_dir();
		pwd = g_strconcat("--PWD:", current_dir, NULL);
		remote_list = g_list_prepend(remote_list, pwd);

		remote_control(argv[0], remote_list, command_line->path, list, command_line->collection_list);
		/* There is no return to this point
		 */
		g_free(pwd);
		g_free(current_dir);
		}
	g_free(geometry);
	g_list_free(remote_list);

	if (list && list->next)
		{
		command_line->cmd_list = list;
		}
	else
		{
		string_list_free(list);
		command_line->cmd_list = NULL;
		}

	if (command_line->startup_blank)
		{
		g_free(command_line->path);
		command_line->path = NULL;
		g_free(command_line->file);
		command_line->file = NULL;
		filelist_free(command_line->cmd_list);
		command_line->cmd_list = NULL;
		string_list_free(command_line->collection_list);
		command_line->collection_list = NULL;
		}
}

static void parse_command_line_for_debug_option(gint argc, gchar *argv[])
{
#ifdef DEBUG
	const gchar *debug_option = "--debug";
	gint len = strlen(debug_option);

	if (argc > 1)
		{
		gint i;

		for (i = 1; i < argc; i++)
			{
			const gchar *cmd_line = argv[i];
			if (strncmp(cmd_line, debug_option, len) == 0)
				{
				gint cmd_line_len = strlen(cmd_line);

				/* we now increment the debug state for verbosity */
				if (cmd_line_len == len)
					debug_level_add(1);
				else if (cmd_line[len] == '=' && g_ascii_isdigit(cmd_line[len+1]))
					{
					gint n = atoi(cmd_line + len + 1);
					if (n < 0) n = 1;
					debug_level_add(n);
					}
				}
			}
		}

	DEBUG_1("debugging output enabled (level %d)", get_debug_level());
#endif
}

#ifdef HAVE_CLUTTER
static gboolean parse_command_line_for_clutter_option(gint argc, gchar *argv[])
{
	const gchar *clutter_option = "--disable-clutter";
	gint len = strlen(clutter_option);
	gboolean ret = FALSE;

	if (argc > 1)
		{
		gint i;

		for (i = 1; i < argc; i++)
			{
			const gchar *cmd_line = argv[i];
			if (strncmp(cmd_line, clutter_option, len) == 0)
				{
				ret = TRUE;
				}
			}
		}

	return ret;
}
#endif

static gboolean parse_command_line_for_cache_maintenance_option(gint argc, gchar *argv[])
{
	const gchar *cache_maintenance_option = "--cache-maintenance";
	gint len = strlen(cache_maintenance_option);
	gboolean ret = FALSE;

	if (argc >= 2)
		{
		const gchar *cmd_line = argv[1];
		if (strncmp(cmd_line, cache_maintenance_option, len) == 0)
			{
			ret = TRUE;
			}
		}

	return ret;
}

static void process_command_line_for_cache_maintenance_option(gint argc, gchar *argv[])
{
	gchar *rc_path;
	gchar *folder_path = NULL;
	gsize size;
	gsize i = 0;
	gchar *buf_config_file;
	gint diff_count;

	if (argc >= 3)
		{
		folder_path = expand_tilde(argv[2]);

		if (isdir(folder_path))
			{
			rc_path = g_build_filename(get_rc_dir(), RC_FILE_NAME, NULL);

			if (isfile(rc_path))
				{
				if (g_file_get_contents(rc_path, &buf_config_file, &size, NULL))
					{
					while (i < size)
						{
						diff_count = strncmp("</global>", &buf_config_file[i], 9);
						if (diff_count == 0)
							{
							break;
							}
						i++;
						}
					/* Load only the <global> section */
					load_config_from_buf(buf_config_file, i + 9, FALSE);

					if (options->thumbnails.enable_caching)
						{
						cache_maintenance(folder_path);
						}
					else
						{
						print_term(TRUE, "Caching not enabled\n");
						exit(EXIT_FAILURE);
						}
					g_free(buf_config_file);
					}
				else
					{
					print_term(TRUE, g_strconcat(_("Cannot load "), rc_path, "\n", NULL));
					exit(EXIT_FAILURE);
					}
				}
			else
				{
				print_term(TRUE, g_strconcat(_("Configuration file path "), rc_path, _(" is not a file\n"), NULL));
				exit(EXIT_FAILURE);
				}
			g_free(rc_path);
			}
		else
			{
			print_term(TRUE, g_strconcat(argv[2], _(" is not a folder\n"), NULL));
			exit(EXIT_FAILURE);
			}
		g_free(folder_path);
		}
	else
		{
		print_term(TRUE, _("No path parameter given\n"));
		exit(EXIT_FAILURE);
		}
}

/*
 *-----------------------------------------------------------------------------
 * startup, init, and exit
 *-----------------------------------------------------------------------------
 */

#define RC_HISTORY_NAME "history"
#define RC_MARKS_NAME "marks"

static void setup_env_path(void)
{
	const gchar *old_path = g_getenv("PATH");
	gchar *path = g_strconcat(gq_bindir, ":", old_path, NULL);
        g_setenv("PATH", path, TRUE);
	g_free(path);
}

static void keys_load(void)
{
	gchar *path;

	path = g_build_filename(get_rc_dir(), RC_HISTORY_NAME, NULL);
	history_list_load(path);
	g_free(path);
}

static void keys_save(void)
{
	gchar *path;

	path = g_build_filename(get_rc_dir(), RC_HISTORY_NAME, NULL);
	history_list_save(path);
	g_free(path);
}

static void marks_load(void)
{
	gchar *path;

	path = g_build_filename(get_rc_dir(), RC_MARKS_NAME, NULL);
	marks_list_load(path);
	g_free(path);
}

static void marks_save(gboolean save)
{
	gchar *path;

	path = g_build_filename(get_rc_dir(), RC_MARKS_NAME, NULL);
	marks_list_save(path, save);
	g_free(path);
}

static void mkdir_if_not_exists(const gchar *path)
{
	if (isdir(path)) return;

	log_printf(_("Creating %s dir:%s\n"), GQ_APPNAME, path);

	if (!recursive_mkdir_if_not_exists(path, 0755))
		{
		log_printf(_("Could not create dir:%s\n"), path);
		}
}


/* We add to duplicate and modify  gtk_accel_map_print() and gtk_accel_map_save()
 * to improve the reliability in special cases (especially when disk is full)
 * These functions are now using secure saving stuff.
 */
static void gq_accel_map_print(
		    gpointer 	data,
		    const gchar	*accel_path,
		    guint	accel_key,
		    GdkModifierType accel_mods,
		    gboolean	changed)
{
	GString *gstring = g_string_new(changed ? NULL : "; ");
	SecureSaveInfo *ssi = data;
	gchar *tmp, *name;

	g_string_append(gstring, "(gtk_accel_path \"");

	tmp = g_strescape(accel_path, NULL);
	g_string_append(gstring, tmp);
	g_free(tmp);

	g_string_append(gstring, "\" \"");

	name = gtk_accelerator_name(accel_key, accel_mods);
	tmp = g_strescape(name, NULL);
	g_free(name);
	g_string_append(gstring, tmp);
	g_free(tmp);

	g_string_append(gstring, "\")\n");

	secure_fwrite(gstring->str, sizeof(*gstring->str), gstring->len, ssi);

	g_string_free(gstring, TRUE);
}

static gboolean gq_accel_map_save(const gchar *path)
{
	gchar *pathl;
	SecureSaveInfo *ssi;
	GString *gstring;

	pathl = path_from_utf8(path);
	ssi = secure_open(pathl);
	g_free(pathl);
	if (!ssi)
		{
		log_printf(_("error saving file: %s\n"), path);
		return FALSE;
		}

	gstring = g_string_new("; ");
	if (g_get_prgname())
		g_string_append(gstring, g_get_prgname());
	g_string_append(gstring, " GtkAccelMap rc-file         -*- scheme -*-\n");
	g_string_append(gstring, "; this file is an automated accelerator map dump\n");
	g_string_append(gstring, ";\n");

	secure_fwrite(gstring->str, sizeof(*gstring->str), gstring->len, ssi);

	g_string_free(gstring, TRUE);

	gtk_accel_map_foreach((gpointer) ssi, gq_accel_map_print);

	if (secure_close(ssi))
		{
		log_printf(_("error saving file: %s\nerror: %s\n"), path,
			   secsave_strerror(secsave_errno));
		return FALSE;
		}

	return TRUE;
}

static gchar *accep_map_filename(void)
{
	return g_build_filename(get_rc_dir(), "accels", NULL);
}

static void accel_map_save(void)
{
	gchar *path;

	path = accep_map_filename();
	gq_accel_map_save(path);
	g_free(path);
}

static void accel_map_load(void)
{
	gchar *path;
	gchar *pathl;

	path = accep_map_filename();
	pathl = path_from_utf8(path);
	gtk_accel_map_load(pathl);
	g_free(pathl);
	g_free(path);
}

static void gtkrc_load(void)
{
	gchar *path;
	gchar *pathl;

	/* If a gtkrc file exists in the rc directory, add it to the
	 * list of files to be parsed at the end of gtk_init() */
	path = g_build_filename(get_rc_dir(), "gtkrc", NULL);
	pathl = path_from_utf8(path);
	if (access(pathl, R_OK) == 0)
		gtk_rc_add_default_file(pathl);
	g_free(pathl);
	g_free(path);
}

static void exit_program_final(void)
{
	LayoutWindow *lw = NULL;
	GList *list;
	LayoutWindow *tmp_lw;
	gchar *archive_dir;
	GFile *archive_file;

	 /* make sure that external editors are loaded, we would save incomplete configuration otherwise */
	layout_editors_reload_finish();

	remote_close(remote_connection);

	collect_manager_flush();

	/* Save the named windows */
	if (layout_window_list && layout_window_list->next)
		{
		list = layout_window_list;
		while (list)
			{
			tmp_lw = list->data;
			if (!g_str_has_prefix(tmp_lw->options.id, "lw"))
				{
				save_layout(list->data);
				}
			list = list->next;
			}
		}

	save_options(options);
	keys_save();
	accel_map_save();

	if (layout_valid(&lw))
		{
		layout_free(lw);
		}

	/* Delete any files/folders in /tmp that have been created by the open archive function */
	archive_dir = g_build_filename(g_get_tmp_dir(), GQ_ARCHIVE_DIR, instance_identifier, NULL);
	if (isdir(archive_dir))
		{
		archive_file = g_file_new_for_path(archive_dir);
		rmdir_recursive(archive_file, NULL, NULL);
		g_free(archive_dir);
		g_object_unref(archive_file);
		}

	/* If there are still sub-dirs created by another instance, this will fail
	 * but that does not matter */
	archive_dir = g_build_filename(g_get_tmp_dir(), GQ_ARCHIVE_DIR, NULL);
	if (isdir(archive_dir))
		{
		archive_file = g_file_new_for_path(archive_dir);
		g_file_delete(archive_file, NULL, NULL);
		g_free(archive_dir);
		g_object_unref(archive_file);
		}

	secure_close(command_line->ssi);

	gtk_main_quit();
}

static GenericDialog *exit_dialog = NULL;

static void exit_confirm_cancel_cb(GenericDialog *gd, gpointer UNUSED(data))
{
	exit_dialog = NULL;
	generic_dialog_close(gd);
}

static void exit_confirm_exit_cb(GenericDialog *gd, gpointer UNUSED(data))
{
	exit_dialog = NULL;
	generic_dialog_close(gd);
	exit_program_final();
}

static gint exit_confirm_dlg(void)
{
	GtkWidget *parent;
	LayoutWindow *lw;
	gchar *msg;

	if (exit_dialog)
		{
		gtk_window_present(GTK_WINDOW(exit_dialog->dialog));
		return TRUE;
		}

	if (!collection_window_modified_exists()) return FALSE;

	parent = NULL;
	lw = NULL;
	if (layout_valid(&lw))
		{
		parent = lw->window;
		}

	msg = g_strdup_printf("%s - %s", GQ_APPNAME, _("exit"));
	exit_dialog = generic_dialog_new(msg,
				"exit", parent, FALSE,
				exit_confirm_cancel_cb, NULL);
	g_free(msg);
	msg = g_strdup_printf(_("Quit %s"), GQ_APPNAME);
	generic_dialog_add_message(exit_dialog, GTK_STOCK_DIALOG_QUESTION,
				   msg, _("Collections have been modified. Quit anyway?"), TRUE);
	g_free(msg);
	generic_dialog_add_button(exit_dialog, GTK_STOCK_QUIT, NULL, exit_confirm_exit_cb, TRUE);

	gtk_widget_show(exit_dialog->dialog);

	return TRUE;
}

static void exit_program_write_metadata_cb(gint success, const gchar *UNUSED(dest_path), gpointer UNUSED(data))
{
	if (success) exit_program();
}

void exit_program(void)
{
	layout_image_full_screen_stop(NULL);

	if (metadata_write_queue_confirm(FALSE, exit_program_write_metadata_cb, NULL)) return;

	options->marks_save ? marks_save(TRUE) : marks_save(FALSE);

	if (exit_confirm_dlg()) return;

	exit_program_final();
}

/* This code is supposed to handle situation when a file mmaped by image_loader
 * or by exif loader is truncated by some other process.
 * This is probably not completely correct according to posix, because
 * mmap is not in the list of calls that can be used safely in signal handler,
 * but anyway, the handler is called in situation when the application would
 * crash otherwise.
 * Ideas for improvement are welcome ;)
 */
/** @FIXME this probably needs some better ifdefs. Please report any compilation problems */

#if defined(SIGBUS) && defined(SA_SIGINFO)
static void sigbus_handler_cb(int UNUSED(signum), siginfo_t *info, void *UNUSED(context))
{
	unsigned long pagesize = sysconf(_SC_PAGE_SIZE);
	DEBUG_1("SIGBUS %p", info->si_addr);
	mmap((void *)(((unsigned long)info->si_addr / pagesize) * pagesize), pagesize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}
#endif

static void setup_sigbus_handler(void)
{
#if defined(SIGBUS) && defined(SA_SIGINFO)
	struct sigaction sigbus_action;
	sigfillset(&sigbus_action.sa_mask);
	sigbus_action.sa_sigaction = sigbus_handler_cb;
	sigbus_action.sa_flags = SA_SIGINFO;

	sigaction(SIGBUS, &sigbus_action, NULL);
#endif
}

static void set_theme_bg_color()
{
	GdkRGBA bg_color;
	GdkColor theme_color;
	GtkStyleContext *style_context;
	GList *work;
	LayoutWindow *lw;

	if (!options->image.use_custom_border_color)
		{
		work = layout_window_list;
		lw = work->data;

		style_context = gtk_widget_get_style_context(lw->window);
		gtk_style_context_get_background_color(style_context, GTK_STATE_FLAG_NORMAL, &bg_color);

		theme_color.red = bg_color.red * 65535;
		theme_color.green = bg_color.green * 65535;
		theme_color.blue = bg_color.blue * 65535;

		while (work)
			{
			lw = work->data;
			image_background_set_color(lw->image, &theme_color);
			work = work->next;
			}
		}

	view_window_colors_update();
}

static gboolean theme_change_cb(GObject *UNUSED(gobject), GParamSpec *UNUSED(pspec), gpointer UNUSED(data))
{
	set_theme_bg_color();

	return FALSE;
}

/**
 * @brief Set up the application paths
 * 
 * This function is required for use of AppImages. AppImages are
 * relocatable, and therefore cannot use fixed paths to various components.
 * These paths were originally #defines created during compilation.
 * They are now variables, all defined relative to one level above the
 * directory that the executable is run from.
 */
static void create_application_paths()
{
	gchar *dirname;
	gint length;
	gchar *path;

	length = wai_getExecutablePath(NULL, 0, NULL);
	path = (gchar *)malloc(length + 1);
	wai_getExecutablePath(path, length, NULL);
	path[length] = '\0';

	gq_executable_path = g_strdup(path);
	dirname = g_path_get_dirname(gq_executable_path);
	gq_prefix = g_path_get_dirname(dirname);

	gq_localedir = g_build_filename(gq_prefix, GQ_LOCALEDIR, NULL);
	gq_helpdir = g_build_filename(gq_prefix, GQ_HELPDIR, NULL);
	gq_htmldir = g_build_filename(gq_prefix, GQ_HTMLDIR, NULL);
	gq_appdir = g_build_filename(gq_prefix, GQ_APPDIR, NULL);
	gq_bindir = g_build_filename(gq_prefix, GQ_BINDIR, NULL);
	desktop_file_template = g_build_filename(gq_appdir, "template.desktop", NULL);

	g_free(dirname);
	g_free(path);
}

gboolean stderr_channel_cb(GIOChannel *source, GIOCondition UNUSED(condition), gpointer UNUSED(data))
{
	static GString *message_str = NULL;
	gchar buf[10] = {0};
	gsize count;

	if (!message_str)
		{
		message_str = g_string_new(NULL);
		}

	g_io_channel_read_chars(source, buf, 1, &count, NULL);

	if (count > 0)
		{
		if (buf[0] == '\n')
			{
			log_printf("%s", message_str->str);
			g_string_free(message_str, TRUE);
			message_str = NULL;
			}
		else
			{
			message_str = g_string_append_c(message_str, buf[0]);
			}
		return TRUE;
		}
	else
		{
		return FALSE;
		}
}

gint main(gint argc, gchar *argv[])
{
	CollectionData *first_collection = NULL;
	gchar *buf;
	CollectionData *cd = NULL;
	gboolean disable_clutter = FALSE;
	gboolean single_dir = TRUE;
	LayoutWindow *lw;
	GtkSettings *default_settings;
	gint fd_stderr[2];
	GIOChannel *stderr_channel;

	gdk_set_allowed_backends("x11,*");

	gdk_threads_init();
	gdk_threads_enter();

	/* init execution time counter (debug only) */
	init_exec_time();

	create_application_paths();

	/* setup locale, i18n */
	setlocale(LC_ALL, "");

#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE, gq_localedir);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
#endif

	/* Tee stderr to log window */
	if (pipe(fd_stderr) == 0)
		{
		if (dup2(fd_stderr[1], fileno(stderr)) != -1)
			{
			close(fd_stderr[1]);
			stderr_channel = g_io_channel_unix_new(fd_stderr[0]);
			g_io_add_watch(stderr_channel, G_IO_IN, (GIOFunc)stderr_channel_cb, NULL);
			}
		}

	exif_init();

#ifdef HAVE_LUA
	lua_init();
#endif

	/* setup random seed for random slideshow */
	srand(time(NULL));

	setup_sigbus_handler();

	/* register global notify functions */
	file_data_register_notify_func(cache_notify_cb, NULL, NOTIFY_PRIORITY_HIGH);
	file_data_register_notify_func(thumb_notify_cb, NULL, NOTIFY_PRIORITY_HIGH);
	file_data_register_notify_func(histogram_notify_cb, NULL, NOTIFY_PRIORITY_HIGH);
	file_data_register_notify_func(collect_manager_notify_cb, NULL, NOTIFY_PRIORITY_LOW);
	file_data_register_notify_func(metadata_notify_cb, NULL, NOTIFY_PRIORITY_LOW);


	gtkrc_load();

	parse_command_line_for_debug_option(argc, argv);
	DEBUG_1("%s main: gtk_init", get_exec_time());
#ifdef HAVE_CLUTTER
	if (parse_command_line_for_clutter_option(argc, argv))
		{
		disable_clutter	= TRUE;
		gtk_init(&argc, &argv);
		}
	else
		{
		if (gtk_clutter_init(&argc, &argv) != CLUTTER_INIT_SUCCESS)
			{
			log_printf("Can't initialize clutter-gtk.\nStart Geeqie with the option \"geeqie --disable-clutter\"");
			runcmd("zenity --error --title=\"Geeqie\" --text \"Can't initialize clutter-gtk.\n\nStart Geeqie with the option:\n geeqie --disable-clutter\" --width=300");
			exit(1);
			}
		}
#else
	gtk_init(&argc, &argv);
#endif

	if (gtk_major_version < GTK_MAJOR_VERSION ||
	    (gtk_major_version == GTK_MAJOR_VERSION && gtk_minor_version < GTK_MINOR_VERSION) )
		{
		log_printf("!!! This is a friendly warning.\n");
		log_printf("!!! The version of GTK+ in use now is older than when %s was compiled.\n", GQ_APPNAME);
		log_printf("!!!  compiled with GTK+-%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION);
		log_printf("!!!   running with GTK+-%d.%d\n", gtk_major_version, gtk_minor_version);
		log_printf("!!! %s may quit unexpectedly with a relocation error.\n", GQ_APPNAME);
		}

	DEBUG_1("%s main: pixbuf_inline_register_stock_icons", get_exec_time());
	pixbuf_inline_register_stock_icons();

	DEBUG_1("%s main: setting default options before commandline handling", get_exec_time());
	options = init_options(NULL);
	setup_default_options(options);
	if (disable_clutter)
		{
		options->disable_gpu = TRUE;
		}

	/* Generate a unique identifier used by the open archive function */
	instance_identifier = g_strdup_printf("%x", g_random_int());

	DEBUG_1("%s main: mkdir_if_not_exists", get_exec_time());
	/* these functions don't depend on config file */
	mkdir_if_not_exists(get_rc_dir());
	mkdir_if_not_exists(get_collections_dir());
	mkdir_if_not_exists(get_thumbnails_cache_dir());
	mkdir_if_not_exists(get_metadata_cache_dir());
	mkdir_if_not_exists(get_window_layouts_dir());

	setup_env_path();

	if (parse_command_line_for_cache_maintenance_option(argc, argv))
		{
		process_command_line_for_cache_maintenance_option(argc, argv);
		}
	else
		{
		DEBUG_1("%s main: parse_command_line", get_exec_time());
		parse_command_line(argc, argv);

		keys_load();
		accel_map_load();

		/* restore session from the config file */


		DEBUG_1("%s main: load_options", get_exec_time());
		if (!load_options(options))
			{
			/* load_options calls these functions after it parses global options, we have to call it here if it fails */
			filter_add_defaults();
			filter_rebuild();
			}

	#ifdef HAVE_CLUTTER
	/** @FIXME For the background of this see:
	 * https://github.com/BestImageViewer/geeqie/issues/397
	 * The feature CLUTTER_FEATURE_SWAP_EVENTS indictates if the
	 * system is liable to exhibit this problem.
	 * The user is provided with an override in Preferences/Behavior
	 */
		if (!options->override_disable_gpu && !options->disable_gpu)
			{
			DEBUG_1("CLUTTER_FEATURE_SWAP_EVENTS %d",clutter_feature_available(CLUTTER_FEATURE_SWAP_EVENTS));
			if (clutter_feature_available(CLUTTER_FEATURE_SWAP_EVENTS) != 0)
				{
				options->disable_gpu = TRUE;
				}
			}
	#endif

		/* handle missing config file and commandline additions*/
		if (!layout_window_list)
			{
			/* broken or no config file or no <layout> section */
			layout_new_from_default();
			}

		layout_editors_reload_start();

		/* If no --list option, open a separate collection window for each
		 * .gqv file on the command line
		 */
		if (command_line->collection_list && !command_line->startup_command_line_collection)
			{
			GList *work;

			work = command_line->collection_list;
			while (work)
				{
				CollectWindow *cw;
				const gchar *path;

				path = work->data;
				work = work->next;

				cw = collection_window_new(path);
				if (!first_collection && cw) first_collection = cw->cd;
				}
			}

		if (command_line->log_file)
			{
			gchar *pathl;
			gchar *path = g_strdup(command_line->log_file);

			pathl = path_from_utf8(path);
			command_line->ssi = secure_open(pathl);
			}

		/* If there is a files list on the command line and no --list option,
		 * check if they are all in the same folder
		 */
		if (command_line->cmd_list && !(command_line->startup_command_line_collection))
			{
			GList *work;
			gchar *path = NULL;

			work = command_line->cmd_list;

			while (work && single_dir)
				{
				gchar *dirname;

				dirname = g_path_get_dirname(work->data);
				if (!path)
					{
					path = g_strdup(dirname);
					}
				else
					{
					if (g_strcmp0(path, dirname) != 0)
						{
						single_dir = FALSE;
						}
					}
				g_free(dirname);
				work = work->next;
				}
			g_free(path);
			}

		/* Files from multiple folders, or --list option given
		 * then open an unnamed collection and insert all files
		 */
		if ((command_line->cmd_list && !single_dir) || (command_line->startup_command_line_collection && command_line->cmd_list))
			{
			GList *work;
			CollectWindow *cw;

			cw = collection_window_new(NULL);
			cd = cw->cd;

			collection_path_changed(cd);

			work = command_line->cmd_list;
			while (work)
				{
				FileData *fd;

				fd = file_data_new_simple(work->data);
				collection_add(cd, fd, FALSE);
				file_data_unref(fd);
				work = work->next;
				}

			work = command_line->collection_list;
			while (work)
				{
				collection_load(cd, (gchar *)work->data, COLLECTION_LOAD_APPEND);
				work = work->next;
				}

			if (cd->list) layout_image_set_collection(NULL, cd, cd->list->data);

			/* mem leak, we never unref this collection when !startup_command_line_collection
			 * (the image view of the main window does not hold a ref to the collection)
			 * this is sort of unavoidable, for if it did hold a ref, next/back
			 * may not work as expected when closing collection windows.
			 *
			 * collection_unref(cd);
			 */

			}
		else if (first_collection)
			{
			layout_image_set_collection(NULL, first_collection,
						    collection_get_first(first_collection));
			}

		/* If the files on the command line are from one folder, select those files
		 * unless it is a command line collection - then leave focus on collection window
		 */
		lw = NULL;
		layout_valid(&lw);

		if (single_dir && command_line->cmd_list && !command_line->startup_command_line_collection)
			{
			GList *work;
			GList *selected;
			FileData *fd;

			selected = NULL;
			work = command_line->cmd_list;
			while (work)
				{
				fd = file_data_new_simple((gchar *)work->data);
				selected = g_list_append(selected, fd);
				file_data_unref(fd);
				work = work->next;
				}
			layout_select_list(lw, selected);
			}

		buf = g_build_filename(get_rc_dir(), ".command", NULL);
		remote_connection = remote_server_init(buf, cd);
		g_free(buf);

		marks_load();

		default_settings = gtk_settings_get_default();
		g_signal_connect(default_settings, "notify::gtk-theme-name", G_CALLBACK(theme_change_cb), NULL);
		set_theme_bg_color();
		}

	DEBUG_1("%s main: gtk_main", get_exec_time());
	gtk_main();

	gdk_threads_leave();
	return 0;
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
