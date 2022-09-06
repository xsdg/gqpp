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

#include "main.h"
#include "img-view.h"

#include "collect.h"
#include "collect-io.h"
#include "dnd.h"
#include "editors.h"
#include "filedata.h"
#include "fullscreen.h"
#include "image.h"
#include "image-load.h"
#include "image-overlay.h"
#include "layout.h"
#include "layout-image.h"
#include "layout-util.h"
#include "menu.h"
#include "misc.h"
#include "pixbuf-util.h"
#include "pixbuf-renderer.h"
#include "print.h"
#include "slideshow.h"
#include "ui-fileops.h"
#include "ui-menu.h"
#include "uri-utils.h"
#include "utilops.h"
#include "window.h"

#include <gdk/gdkkeysyms.h> /* for keyboard values */


typedef struct _ViewWindow ViewWindow;
struct _ViewWindow
{
	GtkWidget *window;
	ImageWindow *imd;
	FullScreenData *fs;
	SlideShowData *ss;

	GList *list;
	GList *list_pointer;
};


static GList *view_window_list = NULL;


static GtkWidget *view_popup_menu(ViewWindow *vw);
static void view_fullscreen_toggle(ViewWindow *vw, gboolean force_off);
static void view_overlay_toggle(ViewWindow *vw);

static void view_slideshow_next(ViewWindow *vw);
static void view_slideshow_prev(ViewWindow *vw);
static void view_slideshow_start(ViewWindow *vw);
static void view_slideshow_stop(ViewWindow *vw);

static void view_window_close(ViewWindow *vw);

static void view_window_dnd_init(ViewWindow *vw);

static void view_window_notify_cb(FileData *fd, NotifyType type, gpointer data);


/**
 * This array must be kept in sync with the contents of:\n
 *  @link view_popup_menu() @endlink \n
 *  @link view_window_key_press_cb() @endlink
 * 
 * See also @link hard_coded_window_keys @endlink
 **/
hard_coded_window_keys image_window_keys[] = {
	{GDK_CONTROL_MASK, 'C', N_("Copy")},
	{GDK_CONTROL_MASK, 'M', N_("Move")},
	{GDK_CONTROL_MASK, 'R', N_("Rename")},
	{GDK_CONTROL_MASK, 'D', N_("Move to Trash")},
	{0, GDK_KEY_Delete, N_("Move to Trash")},
	{GDK_SHIFT_MASK, GDK_KEY_Delete, N_("Delete")},
	{GDK_CONTROL_MASK, 'W', N_("Close window")},
	{GDK_SHIFT_MASK, 'R', N_("Rotate 180°")},
	{GDK_SHIFT_MASK, 'M', N_("Rotate mirror")},
	{GDK_SHIFT_MASK, 'F', N_("Rotate flip")},
	{0, ']', N_(" Rotate counterclockwise 90°")},
	{0, '[', N_(" Rotate clockwise 90°")},
	{0, GDK_KEY_Page_Up, N_("Previous")},
	{0, GDK_KEY_KP_Page_Up, N_("Previous")},
	{0, GDK_KEY_BackSpace, N_("Previous")},
	{0, 'B', N_("Previous")},
	{0, GDK_KEY_Page_Down, N_("Next")},
	{0, GDK_KEY_KP_Page_Down, N_("Next")},
	{0, GDK_KEY_space, N_("Next")},
	{0, 'N', N_("Next")},
	{0, GDK_KEY_equal, N_("Zoom in")},
	{0, GDK_KEY_plus, N_("Zoom in")},
	{0, GDK_KEY_minus, N_("Zoom out")},
	{0, 'X', N_("Zoom to fit")},
	{0, GDK_KEY_KP_Multiply, N_("Zoom to fit")},
	{0, 'Z', N_("Zoom 1:1")},
	{0, GDK_KEY_KP_Divide, N_("Zoom 1:1")},
	{0, GDK_KEY_1, N_("Zoom 1:1")},
	{0, '2', N_("Zoom 2:1")},
	{0, '3', N_("Zoom 3:1")},
	{0, '4', N_("Zoom 4:1")},
	{0, '7', N_("Zoom 1:4")},
	{0, '8', N_("Zoom 1:3")},
	{0, '9', N_("Zoom 1:2")},
	{0, 'W', N_("Zoom fit window width")},
	{0, 'H', N_("Zoom fit window height")},
	{0, 'S', N_("Toggle slideshow")},
	{0, 'P', N_("Pause slideshow")},
	{0, 'R', N_("Reload image")},
	{0, 'F', N_("Full screen")},
	{0, 'V', N_("Fullscreen")},
	{0, GDK_KEY_F11, N_("Fullscreen")},
	{0, 'I', N_("Image overlay")},
	{0, GDK_KEY_Escape, N_("Exit fullscreen")},
	{0, GDK_KEY_Escape, N_("Close window")},
	{GDK_SHIFT_MASK, 'G', N_("Desaturate")},
	{GDK_SHIFT_MASK, 'P', N_("Print")},
	{0, 0, NULL}
};


/*
 *-----------------------------------------------------------------------------
 * misc
 *-----------------------------------------------------------------------------
 */

static ImageWindow *view_window_active_image(ViewWindow *vw)
{
	if (vw->fs) return vw->fs->imd;

	return vw->imd;
}

static void view_window_set_list(ViewWindow *vw, GList *list)
{

	filelist_free(vw->list);
	vw->list = NULL;
	vw->list_pointer = NULL;

	vw->list = filelist_copy(list);
}

static gboolean view_window_contains_collection(ViewWindow *vw)
{
	CollectionData *cd;
	CollectInfo *info;

	cd = image_get_collection(view_window_active_image(vw), &info);

	return (cd && info);
}

static void view_collection_step(ViewWindow *vw, gboolean next)
{
	ImageWindow *imd = view_window_active_image(vw);
	CollectionData *cd;
	CollectInfo *info;
	CollectInfo *read_ahead_info = NULL;

	cd = image_get_collection(imd, &info);

	if (!cd || !info) return;

	if (next)
		{
		info = collection_next_by_info(cd, info);
		if (options->image.enable_read_ahead)
			{
			read_ahead_info = collection_next_by_info(cd, info);
			if (!read_ahead_info) read_ahead_info = collection_prev_by_info(cd, info);
			}
		}
	else
		{
		info = collection_prev_by_info(cd, info);
		if (options->image.enable_read_ahead)
			{
			read_ahead_info = collection_prev_by_info(cd, info);
			if (!read_ahead_info) read_ahead_info = collection_next_by_info(cd, info);
			}
		}

	if (info)
		{
		image_change_from_collection(imd, cd, info, image_zoom_get_default(imd));

		if (read_ahead_info) image_prebuffer_set(imd, read_ahead_info->fd);
		}

}

static void view_collection_step_to_end(ViewWindow *vw, gboolean last)
{
	ImageWindow *imd = view_window_active_image(vw);
	CollectionData *cd;
	CollectInfo *info;
	CollectInfo *read_ahead_info = NULL;

	cd = image_get_collection(imd, &info);

	if (!cd || !info) return;

	if (last)
		{
		info = collection_get_last(cd);
		if (options->image.enable_read_ahead) read_ahead_info = collection_prev_by_info(cd, info);
		}
	else
		{
		info = collection_get_first(cd);
		if (options->image.enable_read_ahead) read_ahead_info = collection_next_by_info(cd, info);
		}

	if (info)
		{
		image_change_from_collection(imd, cd, info, image_zoom_get_default(imd));
		if (read_ahead_info) image_prebuffer_set(imd, read_ahead_info->fd);
		}
}

static void view_list_step(ViewWindow *vw, gboolean next)
{
	ImageWindow *imd = view_window_active_image(vw);
	FileData *fd;
	GList *work;
	GList *work_ahead;

	if (!vw->list) return;

	fd = image_get_fd(imd);
	if (!fd) return;

	if (g_list_position(vw->list, vw->list_pointer) >= 0)
		{
		work = vw->list_pointer;
		}
	else
		{
		gboolean found = FALSE;

		work = vw->list;
		while (work && !found)
			{
			FileData *temp;

			temp = work->data;

			if (fd == temp)
				{
				found = TRUE;
				}
			else
				{
				work = work->next;
				}
			}
		}
	if (!work) return;

	work_ahead = NULL;
	if (next)
		{
		work = work->next;
		if (work) work_ahead = work->next;
		}
	else
		{
		work = work->prev;
		if (work) work_ahead = work->prev;
		}

	if (!work) return;

	vw->list_pointer = work;
	fd = work->data;
	image_change_fd(imd, fd, image_zoom_get_default(imd));

	if (options->image.enable_read_ahead && work_ahead)
		{
		FileData *next_fd = work_ahead->data;
		image_prebuffer_set(imd, next_fd);
		}
}

static void view_list_step_to_end(ViewWindow *vw, gboolean last)
{
	ImageWindow *imd = view_window_active_image(vw);
	FileData *fd;
	GList *work;
	GList *work_ahead;

	if (!vw->list) return;

	if (last)
		{
		work = g_list_last(vw->list);
		work_ahead = work->prev;
		}
	else
		{
		work = vw->list;
		work_ahead = work->next;
		}

	vw->list_pointer = work;
	fd = work->data;
	image_change_fd(imd, fd, image_zoom_get_default(imd));

	if (options->image.enable_read_ahead && work_ahead)
		{
		FileData *next_fd = work_ahead->data;
		image_prebuffer_set(imd, next_fd);
		}
}

static void view_step_next(ViewWindow *vw)
{
	if (vw->ss)
		{
		view_slideshow_next(vw);
		}
	else if (vw->list)
		{
		view_list_step(vw, TRUE);
		}
	else
		{
		view_collection_step(vw, TRUE);
		}
}

static void view_step_prev(ViewWindow *vw)
{
	if (vw->ss)
		{
		view_slideshow_prev(vw);
		}
	else if (vw->list)
		{
		view_list_step(vw, FALSE);
		}
	else
		{
		view_collection_step(vw, FALSE);
		}
}

static void view_step_to_end(ViewWindow *vw, gboolean last)
{
	if (vw->list)
		{
		view_list_step_to_end(vw, last);
		}
	else
		{
		view_collection_step_to_end(vw, last);
		}
}

/*
 *-----------------------------------------------------------------------------
 * view window keyboard
 *-----------------------------------------------------------------------------
 */

static void view_window_menu_pos_cb(GtkMenu *menu, gint *x, gint *y, gboolean *UNUSED(push_in), gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	imd = view_window_active_image(vw);
	gdk_window_get_origin(gtk_widget_get_window(imd->pr), x, y);
	popup_menu_position_clamp(menu, x, y, 0);
}

static gboolean view_window_key_press_cb(GtkWidget *UNUSED(widget), GdkEventKey *event, gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;
	gint stop_signal;
	GtkWidget *menu;
	gint x = 0;
	gint y = 0;

	imd = view_window_active_image(vw);

	stop_signal = TRUE;
	switch (event->keyval)
		{
		case GDK_KEY_Left: case GDK_KEY_KP_Left:
			x -= 1;
			break;
		case GDK_KEY_Right: case GDK_KEY_KP_Right:
			x += 1;
			break;
		case GDK_KEY_Up: case GDK_KEY_KP_Up:
			y -= 1;
			break;
		case GDK_KEY_Down: case GDK_KEY_KP_Down:
			y += 1;
			break;
		default:
			stop_signal = FALSE;
			break;
		}

	if (x != 0 || y!= 0)
		{
		if (event->state & GDK_SHIFT_MASK)
			{
			x *= 3;
			y *= 3;
			}

		keyboard_scroll_calc(&x, &y, event);
		image_scroll(imd, x, y);
		}

	if (stop_signal) return stop_signal;

	if (event->state & GDK_CONTROL_MASK)
		{
		stop_signal = TRUE;
		switch (event->keyval)
			{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '0':
				break;
			case 'C': case 'c':
				file_util_copy(image_get_fd(imd), NULL, NULL, imd->widget);
				break;
			case 'M': case 'm':
				file_util_move(image_get_fd(imd), NULL, NULL, imd->widget);
				break;
			case 'R': case 'r':
				file_util_rename(image_get_fd(imd), NULL, imd->widget);
				break;
			case 'D': case 'd':
				options->file_ops.safe_delete_enable = TRUE;
				file_util_delete(image_get_fd(imd), NULL, imd->widget);
				break;
			case 'W': case 'w':
				view_window_close(vw);
				break;
			default:
				stop_signal = FALSE;
				break;
			}
		}
	else if (event->state & GDK_SHIFT_MASK)
		{
		stop_signal = TRUE;
		switch (event->keyval)
			{
			case 'R': case 'r':
				image_alter_orientation(imd, imd->image_fd, ALTER_ROTATE_180);
				break;
			case 'M': case 'm':
				image_alter_orientation(imd, imd->image_fd, ALTER_MIRROR);
				break;
			case 'F': case 'f':
				image_alter_orientation(imd, imd->image_fd, ALTER_FLIP);
				break;
			case 'G': case 'g':
				image_set_desaturate(imd, !image_get_desaturate(imd));
				break;
			case 'P': case 'p':
				{
				FileData *fd;

				view_fullscreen_toggle(vw, TRUE);
				imd = view_window_active_image(vw);
				fd = image_get_fd(imd);
				print_window_new(fd,
						 fd ? g_list_append(NULL, file_data_ref(fd)) : NULL,
						 filelist_copy(vw->list), vw->window);
				}
				break;
			case GDK_KEY_Delete: case GDK_KEY_KP_Delete:
				if (options->file_ops.enable_delete_key)
					{
					options->file_ops.safe_delete_enable = FALSE;
					file_util_delete(image_get_fd(imd), NULL, imd->widget);
					}
				break;
			default:
				stop_signal = FALSE;
				break;
			}
		}
	else
		{
		stop_signal = TRUE;
		switch (event->keyval)
			{
			case GDK_KEY_Page_Up: case GDK_KEY_KP_Page_Up:
			case GDK_KEY_BackSpace:
			case 'B': case 'b':
				view_step_prev(vw);
				break;
			case GDK_KEY_Page_Down: case GDK_KEY_KP_Page_Down:
			case GDK_KEY_space:
			case 'N': case 'n':
				view_step_next(vw);
				break;
			case GDK_KEY_Home: case GDK_KEY_KP_Home:
				view_step_to_end(vw, FALSE);
				break;
			case GDK_KEY_End: case GDK_KEY_KP_End:
				view_step_to_end(vw, TRUE);
				break;
			case '+': case '=': case GDK_KEY_KP_Add:
				image_zoom_adjust(imd, get_zoom_increment());
				break;
			case '-': case GDK_KEY_KP_Subtract:
				image_zoom_adjust(imd, -get_zoom_increment());
				break;
			case 'X': case 'x': case GDK_KEY_KP_Multiply:
				image_zoom_set(imd, 0.0);
				break;
			case 'Z': case 'z': case GDK_KEY_KP_Divide: case '1':
				image_zoom_set(imd, 1.0);
				break;
			case '2':
				image_zoom_set(imd, 2.0);
				break;
			case '3':
				image_zoom_set(imd, 3.0);
				break;
			case '4':
				image_zoom_set(imd, 4.0);
				break;
			case '7':
				image_zoom_set(imd, -4.0);
				break;
			case '8':
				image_zoom_set(imd, -3.0);
				break;
			case '9':
				image_zoom_set(imd, -2.0);
				break;
			case 'W': case 'w':
				image_zoom_set_fill_geometry(imd, FALSE);
				break;
			case 'H': case 'h':
				image_zoom_set_fill_geometry(imd, TRUE);
				break;
			case 'R': case 'r':
				image_reload(imd);
				break;
			case 'S': case 's':
				if (vw->ss)
					{
					view_slideshow_stop(vw);
					}
				else
					{
					view_slideshow_start(vw);
					}
				break;
			case 'P': case 'p':
				slideshow_pause_toggle(vw->ss);
				break;
			case 'F': case 'f':
			case 'V': case 'v':
			case GDK_KEY_F11:
				view_fullscreen_toggle(vw, FALSE);
				break;
			case 'I': case 'i':
				view_overlay_toggle(vw);
				break;
			case ']':
				image_alter_orientation(imd, imd->image_fd, ALTER_ROTATE_90);
				break;
			case '[':
				image_alter_orientation(imd, imd->image_fd, ALTER_ROTATE_90_CC);
				break;
			case GDK_KEY_Delete: case GDK_KEY_KP_Delete:
				if (options->file_ops.enable_delete_key)
					{
					options->file_ops.safe_delete_enable = TRUE;
					file_util_delete(image_get_fd(imd), NULL, imd->widget);
					}
				break;
			case GDK_KEY_Escape:
				if (vw->fs)
					{
					view_fullscreen_toggle(vw, TRUE);
					}
				else
					{
					view_window_close(vw);
					}
				break;
			case GDK_KEY_Menu:
			case GDK_KEY_F10:
				menu = view_popup_menu(vw);
				gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
					       view_window_menu_pos_cb, vw, 0, GDK_CURRENT_TIME);
				break;
			default:
				stop_signal = FALSE;
				break;
			}
		}
	if (!stop_signal && is_help_key(event))
		{
		help_window_show("GuideOtherWindowsImageWindow.html");
		stop_signal = TRUE;
		}

	return stop_signal;
}

/*
 *-----------------------------------------------------------------------------
 * view window main routines
 *-----------------------------------------------------------------------------
 */
static void button_cb(ImageWindow *imd, GdkEventButton *event, gpointer data)
{
	ViewWindow *vw = data;
	GtkWidget *menu;
	gchar *dest_dir;
	LayoutWindow *lw_new;

	switch (event->button)
		{
		case MOUSE_BUTTON_LEFT:
	 		if (options->image_l_click_archive && imd->image_fd->format_class == FORMAT_CLASS_ARCHIVE)
				{
				dest_dir = open_archive(imd->image_fd);
				if (dest_dir)
					{
					lw_new = layout_new_from_default();
					layout_set_path(lw_new, dest_dir);
					g_free(dest_dir);
					}
				else
					{
					warning_dialog(_("Cannot open archive file"), _("See the Log Window"), GTK_STOCK_DIALOG_WARNING, NULL);
					}
				}
	 		else if (options->image_l_click_video && options->image_l_click_video_editor && imd->image_fd->format_class == FORMAT_CLASS_VIDEO)
				{
				start_editor_from_file(options->image_l_click_video_editor, imd->image_fd);
				}
			else if (options->image_lm_click_nav)
				view_step_next(vw);
			break;
		case MOUSE_BUTTON_MIDDLE:
			if (options->image_lm_click_nav)
				view_step_prev(vw);
			break;
		case MOUSE_BUTTON_RIGHT:
			menu = view_popup_menu(vw);
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, event->time);
			break;
		default:
			break;
		}
}

static void scroll_cb(ImageWindow *imd, GdkEventScroll *event, gpointer data)
{
	ViewWindow *vw = data;

	if ((event->state & GDK_CONTROL_MASK) ||
				(imd->mouse_wheel_mode && !options->image_lm_click_nav))
		{
		switch (event->direction)
			{
			case GDK_SCROLL_UP:
				image_zoom_adjust_at_point(imd, get_zoom_increment(), event->x, event->y);
				break;
			case GDK_SCROLL_DOWN:
				image_zoom_adjust_at_point(imd, -get_zoom_increment(), event->x, event->y);
				break;
			default:
				break;
			}
		}
	else if ( (event->state & GDK_SHIFT_MASK) != (guint) (options->mousewheel_scrolls))
		{
		switch (event->direction)
			{
			case GDK_SCROLL_UP:
				image_scroll(imd, 0, -MOUSEWHEEL_SCROLL_SIZE);
				break;
			case GDK_SCROLL_DOWN:
				image_scroll(imd, 0, MOUSEWHEEL_SCROLL_SIZE);
				break;
			case GDK_SCROLL_LEFT:
				image_scroll(imd, -MOUSEWHEEL_SCROLL_SIZE, 0);
				break;
			case GDK_SCROLL_RIGHT:
				image_scroll(imd, MOUSEWHEEL_SCROLL_SIZE, 0);
				break;
			default:
				break;
			}
		}
	else
		{
		switch (event->direction)
			{
			case GDK_SCROLL_UP:
				view_step_prev(vw);
				break;
			case GDK_SCROLL_DOWN:
				view_step_next(vw);
				break;
			default:
				break;
			}
		}
}

static void view_image_set_buttons(ViewWindow *vw, ImageWindow *imd)
{
	image_set_button_func(imd, button_cb, vw);
	image_set_scroll_func(imd, scroll_cb, vw);
}

static void view_fullscreen_stop_func(FullScreenData *UNUSED(fs), gpointer data)
{
	ViewWindow *vw = data;

	vw->fs = NULL;

	if (vw->ss) vw->ss->imd = vw->imd;
}

static void view_fullscreen_toggle(ViewWindow *vw, gboolean force_off)
{
	if (force_off && !vw->fs) return;

	if (vw->fs)
		{
		if (image_osd_get(vw->imd) & OSD_SHOW_INFO)
			image_osd_set(vw->imd, image_osd_get(vw->fs->imd));

		fullscreen_stop(vw->fs);
		}
	else
		{
		vw->fs = fullscreen_start(vw->window, vw->imd, view_fullscreen_stop_func, vw);

		view_image_set_buttons(vw, vw->fs->imd);
		g_signal_connect(G_OBJECT(vw->fs->window), "key_press_event",
				 G_CALLBACK(view_window_key_press_cb), vw);

		if (vw->ss) vw->ss->imd = vw->fs->imd;

		if (image_osd_get(vw->imd) & OSD_SHOW_INFO)
			{
			image_osd_set(vw->fs->imd, image_osd_get(vw->imd));
			image_osd_set(vw->imd, OSD_SHOW_NOTHING);
			}
		}
}

static void view_overlay_toggle(ViewWindow *vw)
{
	ImageWindow *imd;

	imd = view_window_active_image(vw);

	image_osd_toggle(imd);
}

static void view_slideshow_next(ViewWindow *vw)
{
	if (vw->ss) slideshow_next(vw->ss);
}

static void view_slideshow_prev(ViewWindow *vw)
{
	if (vw->ss) slideshow_prev(vw->ss);
}

static void view_slideshow_stop_func(SlideShowData *UNUSED(fs), gpointer data)
{
	ViewWindow *vw = data;
	GList *work;
	FileData *fd;

	vw->ss = NULL;

	work = vw->list;
	fd = image_get_fd(view_window_active_image(vw));
	while (work)
		{
		FileData *temp;

		temp = work->data;
		if (fd == temp)
			{
			vw->list_pointer = work;
			work = NULL;
			}
		else
			{
			work = work->next;
			}
		}
}

static void view_slideshow_start(ViewWindow *vw)
{
	if (!vw->ss)
		{
		CollectionData *cd;
		CollectInfo *info;

		if (vw->list)
			{
			vw->ss = slideshow_start_from_filelist(NULL, view_window_active_image(vw),
								filelist_copy(vw->list),
								view_slideshow_stop_func, vw);
			vw->list_pointer = NULL;
			return;
			}

		cd = image_get_collection(view_window_active_image(vw), &info);
		if (cd && info)
			{
			vw->ss = slideshow_start_from_collection(NULL, view_window_active_image(vw), cd,
								 view_slideshow_stop_func, vw, info);
			}
		}
}

static void view_slideshow_stop(ViewWindow *vw)
{
	if (vw->ss) slideshow_free(vw->ss);
}

static void view_window_destroy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	view_window_list = g_list_remove(view_window_list, vw);

	view_slideshow_stop(vw);
	fullscreen_stop(vw->fs);

	filelist_free(vw->list);

	file_data_unregister_notify_func(view_window_notify_cb, vw);

	g_free(vw);
}

static void view_window_close(ViewWindow *vw)
{
	view_slideshow_stop(vw);
	view_fullscreen_toggle(vw, TRUE);
	gtk_widget_destroy(vw->window);
}

static gboolean view_window_delete_cb(GtkWidget *UNUSED(w), GdkEventAny *UNUSED(event), gpointer data)
{
	ViewWindow *vw = data;

	view_window_close(vw);
	return TRUE;
}

static ViewWindow *real_view_window_new(FileData *fd, GList *list, CollectionData *cd, CollectInfo *info)
{
	ViewWindow *vw;
	GtkAllocation req_size;
	GdkGeometry geometry;
	gint w, h;

	if (!fd && !list && (!cd || !info)) return NULL;

	vw = g_new0(ViewWindow, 1);

	vw->window = window_new(GTK_WINDOW_TOPLEVEL, "view", PIXBUF_INLINE_ICON_VIEW, NULL, NULL);
	DEBUG_NAME(vw->window);

	geometry.min_width = DEFAULT_MINIMAL_WINDOW_SIZE;
	geometry.min_height = DEFAULT_MINIMAL_WINDOW_SIZE;
	gtk_window_set_geometry_hints(GTK_WINDOW(vw->window), NULL, &geometry, GDK_HINT_MIN_SIZE);

	gtk_window_set_resizable(GTK_WINDOW(vw->window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(vw->window), 0);

	vw->imd = image_new(FALSE);
	image_color_profile_set(vw->imd,
				options->color_profile.input_type,
				options->color_profile.use_image);
	image_color_profile_set_use(vw->imd, options->color_profile.enabled);

	image_background_set_color_from_options(vw->imd, FALSE);

	image_attach_window(vw->imd, vw->window, NULL, GQ_APPNAME, TRUE);

	image_auto_refresh_enable(vw->imd, TRUE);
	image_top_window_set_sync(vw->imd, TRUE);

	gtk_container_add(GTK_CONTAINER(vw->window), vw->imd->widget);
	gtk_widget_show(vw->imd->widget);

	view_window_dnd_init(vw);

	view_image_set_buttons(vw, vw->imd);

	g_signal_connect(G_OBJECT(vw->window), "destroy",
			 G_CALLBACK(view_window_destroy_cb), vw);
	g_signal_connect(G_OBJECT(vw->window), "delete_event",
			 G_CALLBACK(view_window_delete_cb), vw);
	g_signal_connect(G_OBJECT(vw->window), "key_press_event",
			 G_CALLBACK(view_window_key_press_cb), vw);
	if (cd && info)
		{
		image_change_from_collection(vw->imd, cd, info, image_zoom_get_default(NULL));
		/* Grab the fd so we can correctly size the window in
		   the call to image_load_dimensions() below. */
		fd = info->fd;
		if (options->image.enable_read_ahead)
			{
			CollectInfo * r_info = collection_next_by_info(cd, info);
			if (!r_info) r_info = collection_prev_by_info(cd, info);
			if (r_info) image_prebuffer_set(vw->imd, r_info->fd);
			}
		}
	else if (list)
		{
		view_window_set_list(vw, list);
		vw->list_pointer = vw->list;
		image_change_fd(vw->imd, (FileData *)vw->list->data, image_zoom_get_default(NULL));
		/* Set fd to first in list */
		fd = vw->list->data;

		if (options->image.enable_read_ahead)
			{
			GList *work = vw->list->next;
			if (work) image_prebuffer_set(vw->imd, (FileData *)work->data);
			}
		}
	else
		{
		image_change_fd(vw->imd, fd, image_zoom_get_default(NULL));
		}

	/* Wait until image is loaded otherwise size is not defined */
	image_load_dimensions(fd, &w, &h);

	if (options->image.limit_window_size)
		{
		gint mw = gdk_screen_width() * options->image.max_window_size / 100;
		gint mh = gdk_screen_height() * options->image.max_window_size / 100;

		if (w > mw) w = mw;
		if (h > mh) h = mh;
		}

	gtk_window_set_default_size(GTK_WINDOW(vw->window), w, h);
	req_size.x = req_size.y = 0;
	req_size.width = w;
	req_size.height = h;
	gtk_widget_size_allocate(GTK_WIDGET(vw->window), &req_size);

	gtk_window_set_focus_on_map(GTK_WINDOW(vw->window), FALSE);
	gtk_widget_show(vw->window);

	view_window_list = g_list_append(view_window_list, vw);

	file_data_register_notify_func(view_window_notify_cb, vw, NOTIFY_PRIORITY_LOW);

	/** @FIXME This is a hack to fix #965 View in new window - blank image
	 * The problem occurs when zoom is set to Original Size and Preload
	 * Next Image is set.
	 * An extra reload is required to force the image to be displayed.
	 * This is probably not the correct solution.
	 **/
	image_reload(vw->imd);

	return vw;
}

static void view_window_collection_unref_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	CollectionData *cd = data;

	collection_unref(cd);
}

void view_window_new(FileData *fd)
{
	GList *list;

	if (fd)
		{
		if (file_extension_match(fd->path, GQ_COLLECTION_EXT))
			{
			ViewWindow *vw;
			CollectionData *cd;
			CollectInfo *info;

			cd = collection_new(fd->path);
			if (collection_load(cd, fd->path, COLLECTION_LOAD_NONE))
				{
				info = collection_get_first(cd);
				}
			else
				{
				collection_unref(cd);
				cd = NULL;
				info = NULL;
				}
			vw = real_view_window_new(NULL, NULL, cd, info);
			if (vw && cd)
				{
				g_signal_connect(G_OBJECT(vw->window), "destroy",
						 G_CALLBACK(view_window_collection_unref_cb), cd);
				}
			}
		else if (isdir(fd->path) && filelist_read(fd, &list, NULL))
			{
			list = filelist_sort_path(list);
			list = filelist_filter(list, FALSE);
			real_view_window_new(NULL, list, NULL, NULL);
			filelist_free(list);
			}
		else
			{
			real_view_window_new(fd, NULL, NULL, NULL);
			}
		}
}

void view_window_new_from_list(GList *list)
{
	real_view_window_new(NULL, list, NULL, NULL);
}

void view_window_new_from_collection(CollectionData *cd, CollectInfo *info)
{
	real_view_window_new(NULL, NULL, cd, info);
}

/*
 *-----------------------------------------------------------------------------
 * public
 *-----------------------------------------------------------------------------
 */

void view_window_colors_update(void)
{
	GList *work;

	work = view_window_list;
	while (work)
		{
		ViewWindow *vw = work->data;
		work = work->next;

		image_background_set_color_from_options(vw->imd, !!vw->fs);
		}
}

gboolean view_window_find_image(ImageWindow *imd, gint *index, gint *total)
{
	GList *work;

	work = view_window_list;
	while (work)
		{
		ViewWindow *vw = work->data;
		work = work->next;

		if (vw->imd == imd ||
		    (vw->fs && vw->fs->imd == imd))
			{
			if (vw->ss)
				{
				gint n;
				gint t;

				n = g_list_length(vw->ss->list_done);
				t = n + g_list_length(vw->ss->list);
				if (n == 0) n = t;
				if (index) *index = n - 1;
				if (total) *total = t;
				}
			else
				{
				if (index) *index = g_list_position(vw->list, vw->list_pointer);
				if (total) *total = g_list_length(vw->list);
				}
			return TRUE;
			}
		}

	return FALSE;
}

/*
 *-----------------------------------------------------------------------------
 * view window menu routines and callbacks
 *-----------------------------------------------------------------------------
 */

static void view_new_window_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	CollectionData *cd;
	CollectInfo *info;

	cd = image_get_collection(vw->imd, &info);

	if (cd && info)
		{
		view_window_new_from_collection(cd, info);
		}
	else
		{
		view_window_new(image_get_fd(vw->imd));
		}
}

static void view_edit_cb(GtkWidget *widget, gpointer data)
{
	ViewWindow *vw;
	ImageWindow *imd;
	const gchar *key = data;

	vw = submenu_item_get_data(widget);
	if (!vw) return;

	if (!editor_window_flag_set(key))
		{
		view_fullscreen_toggle(vw, TRUE);
		}

	imd = view_window_active_image(vw);
	file_util_start_editor_from_file(key, image_get_fd(imd), imd->widget);
}

static void view_alter_cb(GtkWidget *widget, gpointer data)
{
	ViewWindow *vw;
	AlterType type;

	vw = submenu_item_get_data(widget);
	type = GPOINTER_TO_INT(data);

	if (!vw) return;
	image_alter_orientation(vw->imd, vw->imd->image_fd, type);
}

static void view_zoom_in_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	image_zoom_adjust(view_window_active_image(vw), get_zoom_increment());
}

static void view_zoom_out_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	image_zoom_adjust(view_window_active_image(vw), -get_zoom_increment());
}

static void view_zoom_1_1_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	image_zoom_set(view_window_active_image(vw), 1.0);
}

static void view_zoom_fit_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	image_zoom_set(view_window_active_image(vw), 0.0);
}

static void view_copy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	imd = view_window_active_image(vw);
	file_util_copy(image_get_fd(imd), NULL, NULL, imd->widget);
}

static void view_move_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	imd = view_window_active_image(vw);
	file_util_move(image_get_fd(imd), NULL, NULL, imd->widget);
}

static void view_rename_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	imd = view_window_active_image(vw);
	file_util_rename(image_get_fd(imd), NULL, imd->widget);
}

static void view_delete_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	imd = view_window_active_image(vw);
	options->file_ops.safe_delete_enable = FALSE;
	file_util_delete(image_get_fd(imd), NULL, imd->widget);
}

static void view_move_to_trash_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	imd = view_window_active_image(vw);
	options->file_ops.safe_delete_enable = TRUE;
	file_util_delete(image_get_fd(imd), NULL, imd->widget);
}

static void view_copy_path_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	imd = view_window_active_image(vw);
	file_util_copy_path_to_clipboard(image_get_fd(imd), TRUE);
}

static void view_copy_path_unquoted_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	imd = view_window_active_image(vw);
	file_util_copy_path_to_clipboard(image_get_fd(imd), FALSE);
}

static void view_fullscreen_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	view_fullscreen_toggle(vw, FALSE);
}

static void view_slideshow_start_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	view_slideshow_start(vw);
}

static void view_slideshow_stop_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	view_slideshow_stop(vw);
}

static void view_slideshow_pause_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	slideshow_pause_toggle(vw->ss);
}

static void view_close_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;

	view_window_close(vw);
}

static LayoutWindow *view_new_layout_with_fd(FileData *fd)
{
	LayoutWindow *nw;

	nw = layout_new(NULL, NULL);
	layout_sort_set(nw, options->file_sort.method, options->file_sort.ascending);
	layout_set_fd(nw, fd);
	return nw;
}


static void view_set_layout_path_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	ViewWindow *vw = data;
	LayoutWindow *lw;
	ImageWindow *imd;

	imd = view_window_active_image(vw);

	if (!imd || !imd->image_fd) return;

	lw = layout_find_by_image_fd(imd);
	if (lw)
		layout_set_fd(lw, imd->image_fd);
	else
		view_new_layout_with_fd(imd->image_fd);
	view_window_close(vw);
}

static void view_popup_menu_destroy_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	GList *editmenu_fd_list = data;

	filelist_free(editmenu_fd_list);
}

static GList *view_window_get_fd_list(ViewWindow *vw)
{
	GList *list = NULL;
	ImageWindow *imd = view_window_active_image(vw);

	if (imd)
		{
		FileData *fd = image_get_fd(imd);
		if (fd) list = g_list_append(NULL, file_data_ref(fd));
		}

	return list;
}

/**
 * @brief Add file selection list to a collection
 * @param[in] widget 
 * @param[in] data Index to the collection list menu item selected, or -1 for new collection
 * 
 * 
 */
static void image_pop_menu_collections_cb(GtkWidget *widget, gpointer data)
{
	ViewWindow *vw;
	ImageWindow *imd;
	FileData *fd;
	GList *selection_list = NULL;

	vw = submenu_item_get_data(widget);
	imd = view_window_active_image(vw);
	fd = image_get_fd(imd);
	selection_list = g_list_append(selection_list, fd);
	pop_menu_collections(selection_list, data);

	filelist_free(selection_list);
}

static GtkWidget *view_popup_menu(ViewWindow *vw)
{
	GtkWidget *menu;
	GtkWidget *item;
	GList *editmenu_fd_list;
	GtkAccelGroup *accel_group;

	menu = popup_menu_short_lived();

	accel_group = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);

	g_object_set_data(G_OBJECT(menu), "window_keys", image_window_keys);
	g_object_set_data(G_OBJECT(menu), "accel_group", accel_group);

	menu_item_add_stock(menu, _("Zoom _in"), GTK_STOCK_ZOOM_IN, G_CALLBACK(view_zoom_in_cb), vw);
	menu_item_add_stock(menu, _("Zoom _out"), GTK_STOCK_ZOOM_OUT, G_CALLBACK(view_zoom_out_cb), vw);
	menu_item_add_stock(menu, _("Zoom _1:1"), GTK_STOCK_ZOOM_100, G_CALLBACK(view_zoom_1_1_cb), vw);
	menu_item_add_stock(menu, _("Zoom to fit"), GTK_STOCK_ZOOM_FIT, G_CALLBACK(view_zoom_fit_cb), vw);
	menu_item_add_divider(menu);

 	editmenu_fd_list = view_window_get_fd_list(vw);
	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(view_popup_menu_destroy_cb), editmenu_fd_list);
	item = submenu_add_edit(menu, NULL, G_CALLBACK(view_edit_cb), vw, editmenu_fd_list);
	menu_item_add_divider(item);

	submenu_add_alter(menu, G_CALLBACK(view_alter_cb), vw);

	menu_item_add_stock(menu, _("View in _new window"), GTK_STOCK_NEW, G_CALLBACK(view_new_window_cb), vw);
	item = menu_item_add(menu, _("_Go to directory view"), G_CALLBACK(view_set_layout_path_cb), vw);

	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("_Copy..."), GTK_STOCK_COPY, G_CALLBACK(view_copy_cb), vw);
	menu_item_add(menu, _("_Move..."), G_CALLBACK(view_move_cb), vw);
	menu_item_add(menu, _("_Rename..."), G_CALLBACK(view_rename_cb), vw);
	menu_item_add(menu, _("_Copy path"), G_CALLBACK(view_copy_path_cb), vw);
	menu_item_add(menu, _("_Copy path unquoted"), G_CALLBACK(view_copy_path_unquoted_cb), vw);

	menu_item_add_divider(menu);
	menu_item_add_stock(menu,
				options->file_ops.confirm_move_to_trash ? _("Move to Trash...") :
					_("Move to Trash"), PIXBUF_INLINE_ICON_TRASH,
				G_CALLBACK(view_move_to_trash_cb), vw);
	menu_item_add_stock(menu,
				options->file_ops.confirm_delete ? _("_Delete...") :
					_("_Delete"), GTK_STOCK_DELETE,
				G_CALLBACK(view_delete_cb), vw);

	menu_item_add_divider(menu);

	submenu_add_collections(menu, &item,
				G_CALLBACK(image_pop_menu_collections_cb), vw);
	gtk_widget_set_sensitive(item, TRUE);
	menu_item_add_divider(menu);

	if (vw->ss)
		{
		menu_item_add(menu, _("Toggle _slideshow"), G_CALLBACK(view_slideshow_stop_cb), vw);
		if (slideshow_paused(vw->ss))
			{
			item = menu_item_add(menu, _("Continue slides_how"),
					     G_CALLBACK(view_slideshow_pause_cb), vw);
			}
		else
			{
			item = menu_item_add(menu, _("Pause slides_how"),
					     G_CALLBACK(view_slideshow_pause_cb), vw);
			}
		}
	else
		{
		item = menu_item_add(menu, _("Toggle _slideshow"), G_CALLBACK(view_slideshow_start_cb), vw);
		gtk_widget_set_sensitive(item, (vw->list != NULL) || view_window_contains_collection(vw));
		item = menu_item_add(menu, _("Pause slides_how"), G_CALLBACK(view_slideshow_pause_cb), vw);
		gtk_widget_set_sensitive(item, FALSE);
		}

	if (vw->fs)
		{
		menu_item_add(menu, _("Exit _full screen"), G_CALLBACK(view_fullscreen_cb), vw);
		}
	else
		{
		menu_item_add(menu, _("_Full screen"), G_CALLBACK(view_fullscreen_cb), vw);
		}

	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("C_lose window"), GTK_STOCK_CLOSE, G_CALLBACK(view_close_cb), vw);

	return menu;
}

/*
 *-------------------------------------------------------------------
 * dnd confirm dir
 *-------------------------------------------------------------------
 */

typedef struct {
	ViewWindow *vw;
	GList *list;
} CViewConfirmD;

static void view_dir_list_cancel(GtkWidget *UNUSED(widget), gpointer UNUSED(data))
{
	/* do nothing */
}

static void view_dir_list_do(ViewWindow *vw, GList *list, gboolean skip, gboolean recurse)
{
	GList *work;

	view_window_set_list(vw, NULL);

	work = list;
	while (work)
		{
		FileData *fd = work->data;
		work = work->next;

		if (isdir(fd->path))
			{
			if (!skip)
				{
				GList *list = NULL;

				if (recurse)
					{
					list = filelist_recursive(fd);
					}
				else
					{ /** @FIXME ?? */
					filelist_read(fd, &list, NULL);
					list = filelist_sort_path(list);
					list = filelist_filter(list, FALSE);
					}
				if (list) vw->list = g_list_concat(vw->list, list);
				}
			}
		else
			{
			/** @FIXME no filtering here */
			vw->list = g_list_append(vw->list, file_data_ref(fd));
			}
		}

	if (vw->list)
		{
		FileData *fd;

		vw->list_pointer = vw->list;
		fd = vw->list->data;
		image_change_fd(vw->imd, fd, image_zoom_get_default(vw->imd));

		work = vw->list->next;
		if (options->image.enable_read_ahead && work)
			{
			fd = work->data;
			image_prebuffer_set(vw->imd, fd);
			}
		}
	else
		{
		image_change_fd(vw->imd, NULL, image_zoom_get_default(vw->imd));
		}
}

static void view_dir_list_add(GtkWidget *UNUSED(widget), gpointer data)
{
	CViewConfirmD *d = data;
	view_dir_list_do(d->vw, d->list, FALSE, FALSE);
}

static void view_dir_list_recurse(GtkWidget *UNUSED(widget), gpointer data)
{
	CViewConfirmD *d = data;
	view_dir_list_do(d->vw, d->list, FALSE, TRUE);
}

static void view_dir_list_skip(GtkWidget *UNUSED(widget), gpointer data)
{
	CViewConfirmD *d = data;
	view_dir_list_do(d->vw, d->list, TRUE, FALSE);
}

static void view_dir_list_destroy(GtkWidget *UNUSED(widget), gpointer data)
{
	CViewConfirmD *d = data;
	filelist_free(d->list);
	g_free(d);
}

static GtkWidget *view_confirm_dir_list(ViewWindow *vw, GList *list)
{
	GtkWidget *menu;
	CViewConfirmD *d;

	d = g_new(CViewConfirmD, 1);
	d->vw = vw;
	d->list = list;

	menu = popup_menu_short_lived();
	g_signal_connect(G_OBJECT(menu), "destroy",
			 G_CALLBACK(view_dir_list_destroy), d);

	menu_item_add_stock(menu, _("Dropped list includes folders."), GTK_STOCK_DND_MULTIPLE, NULL, NULL);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("_Add contents"), GTK_STOCK_OK, G_CALLBACK(view_dir_list_add), d);
	menu_item_add_stock(menu, _("Add contents _recursive"), GTK_STOCK_ADD, G_CALLBACK(view_dir_list_recurse), d);
	menu_item_add_stock(menu, _("_Skip folders"), GTK_STOCK_REMOVE, G_CALLBACK(view_dir_list_skip), d);
	menu_item_add_divider(menu);
	menu_item_add_stock(menu, _("Cancel"), GTK_STOCK_CANCEL, G_CALLBACK(view_dir_list_cancel), d);

	return menu;
}

/*
 *-----------------------------------------------------------------------------
 * image drag and drop routines
 *-----------------------------------------------------------------------------
 */

static void view_window_get_dnd_data(GtkWidget *UNUSED(widget), GdkDragContext *context,
				     gint UNUSED(x), gint UNUSED(y),
				     GtkSelectionData *selection_data, guint info,
				     guint time, gpointer data)
{
	ViewWindow *vw = data;
	ImageWindow *imd;

	if (gtk_drag_get_source_widget(context) == vw->imd->pr) return;

	imd = vw->imd;

	if (info == TARGET_URI_LIST || info == TARGET_APP_COLLECTION_MEMBER)
		{
		CollectionData *source;
		GList *list;
		GList *info_list;

		if (info == TARGET_URI_LIST)
			{
			GList *work;

			list = uri_filelist_from_gtk_selection_data(selection_data);

			work = list;
			while (work)
				{
				FileData *fd = work->data;
				if (isdir(fd->path))
					{
					GtkWidget *menu;
					menu = view_confirm_dir_list(vw, list);
					gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, time);
					return;
					}
				work = work->next;
				}

			list = filelist_filter(list, FALSE);

			source = NULL;
			info_list = NULL;
			}
		else
			{
			source = collection_from_dnd_data((gchar *)gtk_selection_data_get_data(selection_data), &list, &info_list);
			}

		if (list)
			{
			FileData *fd;

			fd = list->data;
			if (isfile(fd->path))
				{
				view_slideshow_stop(vw);
				view_window_set_list(vw, NULL);

				if (source && info_list)
					{
					image_change_from_collection(imd, source, info_list->data, image_zoom_get_default(imd));
					}
				else
					{
					if (list->next)
						{
						vw->list = list;
						list = NULL;

						vw->list_pointer = vw->list;
						}
					image_change_fd(imd, fd, image_zoom_get_default(imd));
					}
				}
			}
		filelist_free(list);
		g_list_free(info_list);
		}
}

static void view_window_set_dnd_data(GtkWidget *UNUSED(widget), GdkDragContext *UNUSED(context),
				     GtkSelectionData *selection_data, guint UNUSED(info),
				     guint UNUSED(time), gpointer data)
{
	ViewWindow *vw = data;
	FileData *fd;

	fd = image_get_fd(vw->imd);

	if (fd)
		{
		GList *list;

		list = g_list_append(NULL, fd);
		uri_selection_data_set_uris_from_filelist(selection_data, list);
		g_list_free(list);
		}
	else
		{
		gtk_selection_data_set(selection_data, gtk_selection_data_get_target(selection_data),
				       8, NULL, 0);
		}
}

static void view_window_dnd_init(ViewWindow *vw)
{
	ImageWindow *imd;

	imd = vw->imd;

	gtk_drag_source_set(imd->pr, GDK_BUTTON2_MASK,
			    dnd_file_drag_types, dnd_file_drag_types_count,
			    GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
	g_signal_connect(G_OBJECT(imd->pr), "drag_data_get",
			 G_CALLBACK(view_window_set_dnd_data), vw);

	gtk_drag_dest_set(imd->pr,
			  GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
			  dnd_file_drop_types, dnd_file_drop_types_count,
			  GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
	g_signal_connect(G_OBJECT(imd->pr), "drag_data_received",
			 G_CALLBACK(view_window_get_dnd_data), vw);
}

/*
 *-----------------------------------------------------------------------------
 * maintenance (for rename, move, remove)
 *-----------------------------------------------------------------------------
 */

static void view_real_removed(ViewWindow *vw, FileData *fd)
{
	ImageWindow *imd;
	FileData *image_fd;

	imd = view_window_active_image(vw);
	image_fd = image_get_fd(imd);

	if (image_fd && image_fd == fd)
		{
		if (vw->list)
			{
			view_list_step(vw, TRUE);
			if (image_get_fd(imd) == image_fd)
				{
				view_list_step(vw, FALSE);
				}
			}
		else if (view_window_contains_collection(vw))
			{
			view_collection_step(vw, TRUE);
			if (image_get_fd(imd) == image_fd)
				{
				view_collection_step(vw, FALSE);
				}
			}
		if (image_get_fd(imd) == image_fd)
			{
			image_change_fd(imd, NULL, image_zoom_get_default(imd));
			}
		}

	if (vw->list)
		{
		GList *work;
		GList *old;

		old = vw->list_pointer;

		work = vw->list;
		while (work)
			{
			FileData *chk_fd;
			GList *chk_link;

			chk_fd = work->data;
			chk_link = work;
			work = work->next;

			if (chk_fd == fd)
				{
				if (vw->list_pointer == chk_link)
					{
					vw->list_pointer = (chk_link->next) ? chk_link->next : chk_link->prev;
					}
				vw->list = g_list_remove(vw->list, chk_fd);
				file_data_unref(chk_fd);
				}
			}

		/* handles stepping correctly when same image is in the list more than once */
		if (old && old != vw->list_pointer)
			{
			FileData *fd;

			if (vw->list_pointer)
				{
				fd = vw->list_pointer->data;
				}
			else
				{
				fd = NULL;
				}

			image_change_fd(imd, fd, image_zoom_get_default(imd));
			}
		}

	image_osd_update(imd);
}

static void view_window_notify_cb(FileData *fd, NotifyType type, gpointer data)
{
	ViewWindow *vw = data;

	if (!(type & NOTIFY_CHANGE) || !fd->change) return;

	DEBUG_1("Notify view_window: %s %04x", fd->path, type);

	switch (fd->change->type)
		{
		case FILEDATA_CHANGE_MOVE:
		case FILEDATA_CHANGE_RENAME:
			break;
		case FILEDATA_CHANGE_COPY:
			break;
		case FILEDATA_CHANGE_DELETE:
			view_real_removed(vw, fd);
			break;
		case FILEDATA_CHANGE_UNSPECIFIED:
		case FILEDATA_CHANGE_WRITE_METADATA:
			break;
		}
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
