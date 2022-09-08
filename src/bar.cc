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
#include "bar.h"

#include "filedata.h"
#include "history-list.h"
#include "metadata.h"
#include "misc.h"
#include "ui-fileops.h"
#include "ui-misc.h"
#include "ui-utildlg.h"

#include "ui-menu.h"
#include "bar-comment.h"
#include "bar-keywords.h"
#include "bar-exif.h"
#include "bar-histogram.h"
#include "histogram.h"
#include "rcfile.h"
#include "bar-gps.h"

#include <gdk/gdkkeysyms.h>

typedef struct _KnownPanes KnownPanes;
struct _KnownPanes
{
	PaneType type;
	gchar *id;
	gchar *title;
	const gchar *config;
};

static const gchar default_config_histogram[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_histogram id = 'histogram' expanded = 'true' histogram_channel = '4' histogram_mode = '0' />"
"        </bar>"
"    </layout>"
"</gq>";

static const gchar default_config_title[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_comment id = 'title' expanded = 'true' key = 'Xmp.dc.title' height = '40' />"
"        </bar>"
"    </layout>"
"</gq>";

static const gchar default_config_headline[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_comment id = 'headline' expanded = 'true' key = 'Xmp.photoshop.Headline'  height = '40' />"
"        </bar>"
"    </layout>"
"</gq>";

static const gchar default_config_keywords[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_keywords id = 'keywords' expanded = 'true' key = '" KEYWORD_KEY "' />"
"        </bar>"
"    </layout>"
"</gq>";

static const gchar default_config_comment[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_comment id = 'comment' expanded = 'true' key = '" COMMENT_KEY "' height = '150' />"
"        </bar>"
"    </layout>"
"</gq>";
static const gchar default_config_rating[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_rating id = 'rating' expanded = 'true' />"
"        </bar>"
"    </layout>"
"</gq>";

static const gchar default_config_exif[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_exif id = 'exif' expanded = 'true' >"
"                <entry key = 'formatted.Camera' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.DateTime' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.localtime' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.ShutterSpeed' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.Aperture' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.ExposureBias' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.ISOSpeedRating' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.FocalLength' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.FocalLength35mmFilm' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.Flash' if_set = 'true' editable = 'false' />"
"                <entry key = 'Exif.Photo.ExposureProgram' if_set = 'true' editable = 'false' />"
"                <entry key = 'Exif.Photo.MeteringMode' if_set = 'true' editable = 'false' />"
"                <entry key = 'Exif.Photo.LightSource' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.ColorProfile' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.SubjectDistance' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.Resolution' if_set = 'true' editable = 'false' />"
"                <entry key = '" ORIENTATION_KEY "' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.star_rating' if_set = 'true' editable = 'false' />"
"            </pane_exif>"
"        </bar>"
"    </layout>"
"</gq>";

static const gchar default_config_file_info[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_exif id = 'file_info' expanded = 'true' >"
"                <entry key = 'file.mode' if_set = 'false' editable = 'false' />"
"                <entry key = 'file.date' if_set = 'false' editable = 'false' />"
"                <entry key = 'file.size' if_set = 'false' editable = 'false' />"
"                <entry key = 'file.owner' if_set = 'false' editable = 'false' />"
"                <entry key = 'file.group' if_set = 'false' editable = 'false' />"
"                <entry key = 'file.class' if_set = 'false' editable = 'false' />"
"                <entry key = 'file.link' if_set = 'false' editable = 'false' />"
"            </pane_exif>"
"        </bar>"
"    </layout>"
"</gq>";

static const gchar default_config_location[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_exif id = 'location' expanded = 'true' >"
"                <entry key = 'formatted.GPSPosition' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.GPSAltitude' if_set = 'true' editable = 'false' />"
"                <entry key = 'formatted.timezone' if_set = 'true' editable = 'false' />"
"                <entry key = 'Xmp.photoshop.Country' if_set = 'false' editable = 'true' />"
"                <entry key = 'Xmp.iptc.CountryCode' if_set = 'false' editable = 'true' />"
"                <entry key = 'Xmp.photoshop.State' if_set = 'false' editable = 'true' />"
"                <entry key = 'Xmp.photoshop.City' if_set = 'false' editable = 'true' />"
"                <entry key = 'Xmp.iptc.Location' if_set = 'false' editable = 'true' />"
"            </pane_exif>"
"        </bar>"
"    </layout>"
"</gq>";

static const gchar default_config_copyright[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_exif id = 'copyright' expanded = 'true' >"
"                <entry key = 'Xmp.dc.creator' if_set = 'true' editable = 'false' />"
"                <entry key = 'Xmp.dc.contributor' if_set = 'true' editable = 'false' />"
"                <entry key = 'Xmp.dc.rights' if_set = 'false' editable = 'false' />"
"            </pane_exif>"
"        </bar>"
"    </layout>"
"</gq>";

#ifdef HAVE_LIBCHAMPLAIN
#ifdef HAVE_LIBCHAMPLAIN_GTK
static const gchar default_config_gps[] =
"<gq>"
"    <layout id = '_current_'>"
"        <bar>"
"            <pane_gps id = 'gps' expanded = 'true'"
"                      map-id = 'osm-mapnik'"
"                      zoom-level = '8'"
"                      latitude = '50116666'"
"                      longitude = '8683333' />"
"        </bar>"
"    </layout>"
"</gq>";
#endif
#endif

static const KnownPanes known_panes[] = {
/* default sidebar */
	{PANE_HISTOGRAM,	"histogram",	N_("Histogram"),	default_config_histogram},
	{PANE_COMMENT,		"title",	N_("Title"),		default_config_title},
	{PANE_KEYWORDS,		"keywords",	N_("Keywords"),		default_config_keywords},
	{PANE_COMMENT,		"comment",	N_("Comment"),		default_config_comment},
	{PANE_RATING,		"rating",	N_("Star Rating"),	default_config_rating},
	{PANE_COMMENT,		"headline",	N_("Headline"),		default_config_headline},
	{PANE_EXIF,		"exif",		N_("Exif"),		default_config_exif},
/* other pre-configured panes */
	{PANE_EXIF,		"file_info",	N_("File info"),	default_config_file_info},
	{PANE_EXIF,		"location",	N_("Location and GPS"),	default_config_location},
	{PANE_EXIF,		"copyright",	N_("Copyright"),	default_config_copyright},
#ifdef HAVE_LIBCHAMPLAIN
#ifdef HAVE_LIBCHAMPLAIN_GTK
	{PANE_GPS,		"gps",	N_("GPS Map"),	default_config_gps},
#endif
#endif
	{PANE_UNDEF,		NULL,		NULL,			NULL}
};

typedef struct _BarData BarData;
struct _BarData
{
	GtkWidget *widget;
	GtkWidget *vbox;
	FileData *fd;
	GtkWidget *label_file_name;
	GtkWidget *add_button;

	LayoutWindow *lw;
	gint width;
};

static void bar_expander_move(GtkWidget *UNUSED(widget), gpointer data, gboolean up, gboolean single_step)
{
	GtkWidget *expander = (GtkWidget*)data;
	GtkWidget *box;
	gint pos;

	if (!expander) return;
	box = gtk_widget_get_ancestor(expander, GTK_TYPE_BOX);
	if (!box) return;

	gtk_container_child_get(GTK_CONTAINER(box), expander, "position", &pos, NULL);

	if (single_step)
		{
		pos = up ? (pos - 1) : (pos + 1);
		if (pos < 0) pos = 0;
		}
	else
		{
		pos = up ? 0 : -1;
		}

	gtk_box_reorder_child(GTK_BOX(box), expander, pos);
}


static void bar_expander_move_up_cb(GtkWidget *widget, gpointer data)
{
	bar_expander_move(widget, data, TRUE, TRUE);
}

static void bar_expander_move_down_cb(GtkWidget *widget, gpointer data)
{
	bar_expander_move(widget, data, FALSE, TRUE);
}

static void bar_expander_move_top_cb(GtkWidget *widget, gpointer data)
{
	bar_expander_move(widget, data, TRUE, FALSE);
}

static void bar_expander_move_bottom_cb(GtkWidget *widget, gpointer data)
{
	bar_expander_move(widget, data, FALSE, FALSE);
}

static void height_spin_changed_cb(GtkSpinButton *spin, gpointer data)
{

	gtk_widget_set_size_request(GTK_WIDGET(data), -1, gtk_spin_button_get_value_as_int(spin));
}

static gboolean height_spin_key_press_cb(GtkWidget *UNUSED(widget), GdkEventKey *event, gpointer data)
{
	if ((event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_Escape))
		{
		gtk_widget_destroy(GTK_WIDGET(data));
		}

	return TRUE;
}

static void bar_expander_height_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	GtkWidget *expander = (GtkWidget*)data;
	GtkWidget *spin;
	GtkWidget *window;
	GtkWidget *data_box;
	GList *list;
	gint x, y;
	gint w, h;
	GdkDisplay *display;
	GdkDeviceManager *device_manager;
	GdkDevice *device;

	display = gdk_display_get_default();
	device_manager = gdk_display_get_device_manager(display);
	device = gdk_device_manager_get_client_pointer(device_manager);
	gdk_device_get_position(device, NULL, &x, &y);

	list = gtk_container_get_children(GTK_CONTAINER(expander));
	data_box = list->data;

	window = gtk_window_new(GTK_WINDOW_POPUP);

	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(window), 50, 30); //** @FIXME set these values in a more sensible way */

	gtk_window_move(GTK_WINDOW(window), x, y);
	gtk_widget_show(window);

	gtk_widget_get_size_request(GTK_WIDGET(data_box), &w, &h);

	spin = gtk_spin_button_new_with_range(1, 1000, 1);
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(height_spin_changed_cb), data_box);
	g_signal_connect(G_OBJECT(spin), "key-press-event", G_CALLBACK(height_spin_key_press_cb), window);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), h);
	gtk_container_add(GTK_CONTAINER(window), spin);
	gtk_widget_show(spin);
	gtk_widget_grab_focus(GTK_WIDGET(spin));

	g_list_free(list);
}

static void bar_expander_delete_cb(GtkWidget *UNUSED(widget), gpointer data)
{
	GtkWidget *expander = (GtkWidget*)data;
	gtk_widget_destroy(expander);
}

static void bar_expander_add_cb(GtkWidget *widget, gpointer UNUSED(data))
{
	//GtkWidget *bar = (GtkWidget*)data;
	const KnownPanes *pane = known_panes;
	const gchar *id = g_object_get_data(G_OBJECT(widget), "pane_add_id");
	const gchar *config;

	if (!id) return;

	while (pane->id)
		{
		if (strcmp(pane->id, id) == 0) break;
		pane++;
		}
	if (!pane->id) return;

	config = bar_pane_get_default_config(id);
	if (config) load_config_from_buf(config, strlen(config), FALSE);

}


static void bar_menu_popup(GtkWidget *widget)
{
	GtkWidget *menu;
	GtkWidget *bar;
	GtkWidget *expander;
	BarData *bd;
	gboolean display_height_option = FALSE;
	gchar const *label;

	label = gtk_expander_get_label(GTK_EXPANDER(widget));
	display_height_option =	(g_strcmp0(label, "Comment") == 0) ||
							(g_strcmp0(label, "Rating") == 0) ||
							(g_strcmp0(label, "Title") == 0) ||
							(g_strcmp0(label, "Headline") == 0) ||
							(g_strcmp0(label, "Keywords") == 0) ||
							(g_strcmp0(label, "GPS Map") == 0);

	bd = g_object_get_data(G_OBJECT(widget), "bar_data");
	if (bd)
		{
		expander = NULL;
		bar = widget;
		}
	else
		{
		expander = widget;
		bar = gtk_widget_get_parent(widget);
		while (bar && !g_object_get_data(G_OBJECT(bar), "bar_data"))
			bar = gtk_widget_get_parent(bar);
		if (!bar) return;
		}

	menu = popup_menu_short_lived();

	if (expander)
		{
		menu_item_add_stock(menu, _("Move to _top"), GTK_STOCK_GOTO_TOP, G_CALLBACK(bar_expander_move_top_cb), expander);
		menu_item_add_stock(menu, _("Move _up"), GTK_STOCK_GO_UP, G_CALLBACK(bar_expander_move_up_cb), expander);
		menu_item_add_stock(menu, _("Move _down"), GTK_STOCK_GO_DOWN, G_CALLBACK(bar_expander_move_down_cb), expander);
		menu_item_add_stock(menu, _("Move to _bottom"), GTK_STOCK_GOTO_BOTTOM, G_CALLBACK(bar_expander_move_bottom_cb), expander);
		menu_item_add_divider(menu);

		if (gtk_expander_get_expanded(GTK_EXPANDER(expander)) && display_height_option)
			{
			menu_item_add_stock(menu, _("Height..."), GTK_STOCK_PREFERENCES, G_CALLBACK(bar_expander_height_cb), expander);
			menu_item_add_divider(menu);
			}

		menu_item_add_stock(menu, _("Remove"), GTK_STOCK_DELETE, G_CALLBACK(bar_expander_delete_cb), expander);
		menu_item_add_divider(menu);
		}

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, bar, 0, GDK_CURRENT_TIME);
}

static void bar_menu_add_popup(GtkWidget *widget)
{
	GtkWidget *menu;
	GtkWidget *bar;
	const KnownPanes *pane = known_panes;

	bar = widget;

	menu = popup_menu_short_lived();

	while (pane->id)
		{
		GtkWidget *item;
		item = menu_item_add_stock(menu, _(pane->title), GTK_STOCK_ADD, G_CALLBACK(bar_expander_add_cb), bar);
		g_object_set_data(G_OBJECT(item), "pane_add_id", pane->id);
		pane++;
		}

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, bar, 0, GDK_CURRENT_TIME);
}


static gboolean bar_menu_cb(GtkWidget *widget, GdkEventButton *bevent, gpointer UNUSED(data))
{
	if (bevent->button == MOUSE_BUTTON_RIGHT)
		{
		bar_menu_popup(widget);
		return TRUE;
		}
	return FALSE;
}

static void bar_expander_cb(GObject *object, GParamSpec *UNUSED(param_spec), gpointer UNUSED(data))
{
	GtkExpander *expander;
	GtkWidget *child;

	expander = GTK_EXPANDER(object);
	child = gtk_bin_get_child(GTK_BIN(expander));

	if (gtk_expander_get_expanded(expander))
		{
		gtk_widget_show_all(child);
		}
	else
		{
		gtk_widget_hide(child);
		}
}

static gboolean bar_menu_add_cb(GtkWidget *widget, GdkEventButton *UNUSED(bevent), gpointer UNUSED(data))
{
	bar_menu_add_popup(widget);
	return TRUE;
}


static void bar_pane_set_fd_cb(GtkWidget *expander, gpointer data)
{
	GtkWidget *widget = gtk_bin_get_child(GTK_BIN(expander));
	PaneData *pd = g_object_get_data(G_OBJECT(widget), "pane_data");
	if (!pd) return;
	if (pd->pane_set_fd) pd->pane_set_fd(widget, data);
}

void bar_set_fd(GtkWidget *bar, FileData *fd)
{
	BarData *bd;
	bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	if (!bd) return;

	file_data_unref(bd->fd);
	bd->fd = file_data_ref(fd);

	gtk_container_foreach(GTK_CONTAINER(bd->vbox), bar_pane_set_fd_cb, fd);

	gtk_label_set_text(GTK_LABEL(bd->label_file_name), (bd->fd) ? bd->fd->name : "");

}

static void bar_pane_notify_selection_cb(GtkWidget *expander, gpointer data)
{
	GtkWidget *widget = gtk_bin_get_child(GTK_BIN(expander));
	PaneData *pd = g_object_get_data(G_OBJECT(widget), "pane_data");
	if (!pd) return;
	if (pd->pane_notify_selection) pd->pane_notify_selection(widget, GPOINTER_TO_INT(data));
}

void bar_notify_selection(GtkWidget *bar, gint count)
{
	BarData *bd;
	bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	if (!bd) return;

	gtk_container_foreach(GTK_CONTAINER(bd->vbox), bar_pane_notify_selection_cb, GINT_TO_POINTER(count));
}

gboolean bar_event(GtkWidget *bar, GdkEvent *event)
{
	BarData *bd;
	GList *list, *work;
	gboolean ret = FALSE;

	bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	if (!bd) return FALSE;

	list = gtk_container_get_children(GTK_CONTAINER(bd->vbox));

	work = list;
	while (work)
		{
		GtkWidget *widget = gtk_bin_get_child(GTK_BIN(work->data));
		PaneData *pd = g_object_get_data(G_OBJECT(widget), "pane_data");
		if (!pd) continue;

		if (pd->pane_event && pd->pane_event(widget, event))
			{
			ret = TRUE;
			break;
			}
		work = work->next;
		}
	g_list_free(list);
	return ret;
}

GtkWidget *bar_find_pane_by_id(GtkWidget *bar, PaneType type, const gchar *id)
{
	BarData *bd;
	GList *list, *work;
	GtkWidget *ret = NULL;

	if (!id || !id[0]) return NULL;

	bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	if (!bd) return NULL;

	list = gtk_container_get_children(GTK_CONTAINER(bd->vbox));

	work = list;
	while (work)
		{
		GtkWidget *widget = gtk_bin_get_child(GTK_BIN(work->data));
		PaneData *pd = g_object_get_data(G_OBJECT(widget), "pane_data");
		if (!pd) continue;

		if (type == pd->type && strcmp(id, pd->id) == 0)
			{
			ret = widget;
			break;
			}
		work = work->next;
		}
	g_list_free(list);
	return ret;
}

void bar_clear(GtkWidget *bar)
{
	BarData *bd;
	GList *list, *work;

	bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	if (!bd) return;

	list = gtk_container_get_children(GTK_CONTAINER(bd->vbox));

	work = list;
	while (work)
		{
		GtkWidget *widget = work->data;
		gtk_widget_destroy(widget);
		work = work->next;
		}
	g_list_free(list);
}

void bar_write_config(GtkWidget *bar, GString *outstr, gint indent)
{
	BarData *bd;
	GList *list, *work;

	if (!bar) return;

	bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	if (!bd) return;

	WRITE_NL(); WRITE_STRING("<bar ");
	write_bool_option(outstr, indent, "enabled", gtk_widget_get_visible(bar));
	write_uint_option(outstr, indent, "width", bd->width);
	WRITE_STRING(">");

	indent++;
	WRITE_NL(); WRITE_STRING("<clear/>");

	list = gtk_container_get_children(GTK_CONTAINER(bd->vbox));
	work = list;
	while (work)
		{
		GtkWidget *expander = work->data;
		GtkWidget *widget = gtk_bin_get_child(GTK_BIN(expander));
		PaneData *pd = g_object_get_data(G_OBJECT(widget), "pane_data");
		if (!pd) continue;

		pd->expanded = gtk_expander_get_expanded(GTK_EXPANDER(expander));

		if (pd->pane_write_config)
			pd->pane_write_config(widget, outstr, indent);

		work = work->next;
		}
	g_list_free(list);
	indent--;
	WRITE_NL(); WRITE_STRING("</bar>");
}

void bar_update_expander(GtkWidget *pane)
{
	PaneData *pd = g_object_get_data(G_OBJECT(pane), "pane_data");
	GtkWidget *expander;

	if (!pd) return;

	expander = gtk_widget_get_parent(pane);

	gtk_expander_set_expanded(GTK_EXPANDER(expander), pd->expanded);
}

void bar_add(GtkWidget *bar, GtkWidget *pane)
{
	GtkWidget *expander;
	BarData *bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	PaneData *pd = g_object_get_data(G_OBJECT(pane), "pane_data");

	if (!bd) return;

	pd->lw = bd->lw;
	pd->bar = bar;

	expander = gtk_expander_new(NULL);
	DEBUG_NAME(expander);
	if (pd && pd->title)
		{
		gtk_expander_set_label_widget(GTK_EXPANDER(expander), pd->title);
		gtk_widget_show(pd->title);
		}

	gtk_box_pack_start(GTK_BOX(bd->vbox), expander, FALSE, TRUE, 0);

	g_signal_connect(expander, "button_release_event", G_CALLBACK(bar_menu_cb), bd);
	g_signal_connect(expander, "notify::expanded", G_CALLBACK(bar_expander_cb), pd);

	gtk_container_add(GTK_CONTAINER(expander), pane);

	gtk_expander_set_expanded(GTK_EXPANDER(expander), pd->expanded);

	gtk_widget_show(expander);

	if (bd->fd && pd && pd->pane_set_fd) pd->pane_set_fd(pane, bd->fd);

}

void bar_populate_default(GtkWidget *UNUSED(bar))
{
	const gchar *populate_id[] = {"histogram", "title", "keywords", "comment", "rating", "exif", NULL};
	const gchar **id = populate_id;

	while (*id)
		{
		const gchar *config = bar_pane_get_default_config(*id);
		if (config) load_config_from_buf(config, strlen(config), FALSE);
		id++;
		}
}

static void bar_size_allocate(GtkWidget *UNUSED(widget), GtkAllocation *UNUSED(allocation), gpointer data)
{
	BarData *bd = (BarData*)data;

	bd->width = gtk_paned_get_position(GTK_PANED(bd->lw->utility_paned));
}

gint bar_get_width(GtkWidget *bar)
{
	BarData *bd;

	bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	if (!bd) return 0;

	return bd->width;
}

void bar_close(GtkWidget *bar)
{
	BarData *bd;

	bd = g_object_get_data(G_OBJECT(bar), "bar_data");
	if (!bd) return;

	gtk_widget_destroy(bd->widget);
}

static void bar_destroy(GtkWidget *UNUSED(widget), gpointer data)
{
	BarData *bd = (BarData*)data;

	file_data_unref(bd->fd);
	g_free(bd);
}

#ifdef HAVE_LIBCHAMPLAIN_GTK
/**
   @FIXME this is an ugly hack that works around this bug:
   https://bugzilla.gnome.org/show_bug.cgi?id=590692
   http://bugzilla.openedhand.com/show_bug.cgi?id=1751
   it should be removed as soon as a better solution exists
*/

static void bar_unrealize_clutter_fix_cb(GtkWidget *widget, gpointer UNUSED(data))
{
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(widget));
	if (child) gtk_widget_unrealize(child);
}
#endif

GtkWidget *bar_new(LayoutWindow *lw)
{
	BarData *bd;
	GtkWidget *box;
	GtkWidget *scrolled;
	GtkWidget *tbar;
	GtkWidget *add_box;

	bd = g_new0(BarData, 1);

	bd->lw = lw;

	bd->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, PREF_PAD_GAP);
	DEBUG_NAME(bd->widget);
	g_object_set_data(G_OBJECT(bd->widget), "bar_data", bd);
	g_signal_connect(G_OBJECT(bd->widget), "destroy",
			 G_CALLBACK(bar_destroy), bd);

	g_signal_connect(G_OBJECT(bd->widget), "size-allocate",
			 G_CALLBACK(bar_size_allocate), bd);

	g_signal_connect(G_OBJECT(bd->widget), "button_release_event", G_CALLBACK(bar_menu_cb), bd);

	bd->width = SIDEBAR_DEFAULT_WIDTH;

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	DEBUG_NAME(box);

	bd->label_file_name = gtk_label_new("");
	gtk_label_set_ellipsize(GTK_LABEL(bd->label_file_name), PANGO_ELLIPSIZE_END);
	gtk_label_set_selectable(GTK_LABEL(bd->label_file_name), TRUE);
	gtk_label_set_xalign(GTK_LABEL(bd->label_file_name), 0.5);
	gtk_label_set_yalign(GTK_LABEL(bd->label_file_name), 0.5);

	gtk_box_pack_start(GTK_BOX(box), bd->label_file_name, TRUE, TRUE, 0);
	gtk_widget_show(bd->label_file_name);

	gtk_box_pack_start(GTK_BOX(bd->widget), box, FALSE, FALSE, 0);
	gtk_widget_show(box);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	DEBUG_NAME(scrolled);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(bd->widget), scrolled, TRUE, TRUE, 0);
	gtk_widget_show(scrolled);


	bd->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(scrolled), bd->vbox);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(gtk_bin_get_child(GTK_BIN(scrolled))), GTK_SHADOW_NONE);

	add_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	DEBUG_NAME(add_box);
	gtk_box_pack_end(GTK_BOX(bd->widget), add_box, FALSE, FALSE, 0);
	tbar = pref_toolbar_new(add_box, GTK_TOOLBAR_ICONS);
	bd->add_button = pref_toolbar_button(tbar, GTK_STOCK_ADD, NULL, FALSE,
					     _("Add Pane"),
					     G_CALLBACK(bar_menu_add_cb), bd);
	gtk_widget_show(add_box);

#ifdef HAVE_LIBCHAMPLAIN_GTK
	g_signal_connect(G_OBJECT(gtk_bin_get_child(GTK_BIN(scrolled))), "unrealize", G_CALLBACK(bar_unrealize_clutter_fix_cb), NULL);
#endif

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled), GTK_SHADOW_NONE);
	gtk_widget_show(bd->vbox);
	return bd->widget;
}


GtkWidget *bar_update_from_config(GtkWidget *bar, const gchar **attribute_names, const gchar **attribute_values, LayoutWindow *lw, gboolean startup)
{
	gboolean enabled = TRUE;
	gint width = SIDEBAR_DEFAULT_WIDTH;

	while (*attribute_names)
		{
		const gchar *option = *attribute_names++;
		const gchar *value = *attribute_values++;

		if (READ_BOOL_FULL("enabled", enabled)) continue;
		if (READ_INT_FULL("width", width)) continue;


		log_printf("unknown attribute %s = %s\n", option, value);
		}

	if (startup)
		{
		gtk_paned_set_position(GTK_PANED(lw->utility_paned), width);
		}

	if (enabled)
		{
		gtk_widget_show(bar);
		}
	else
		{
		gtk_widget_hide(bar);
		}
	return bar;
}

GtkWidget *bar_new_from_config(LayoutWindow *lw, const gchar **attribute_names, const gchar **attribute_values)
{
	GtkWidget *bar = bar_new(lw);
	return bar_update_from_config(bar, attribute_names, attribute_values, lw, TRUE);
}

GtkWidget *bar_pane_expander_title(const gchar *title)
{
	GtkWidget *widget = gtk_label_new(title);

	pref_label_bold(widget, TRUE, FALSE);
	//gtk_label_set_ellipsize(GTK_LABEL(widget), PANGO_ELLIPSIZE_END);
	/** @FIXME do not work */

	return widget;
}

gboolean bar_pane_translate_title(PaneType type, const gchar *id, gchar **title)
{
	const KnownPanes *pane = known_panes;

	if (!title) return FALSE;
	while (pane->id)
		{
		if (pane->type == type && strcmp(pane->id, id) == 0) break;
		pane++;
		}
	if (!pane->id) return FALSE;

	if (*title && **title && strcmp(pane->title, *title) != 0) return FALSE;

	g_free(*title);
	*title = g_strdup(_(pane->title));
	return TRUE;
}

const gchar *bar_pane_get_default_config(const gchar *id)
{
	const KnownPanes *pane = known_panes;

	while (pane->id)
		{
		if (strcmp(pane->id, id) == 0) break;
		pane++;
		}
	if (!pane->id) return NULL;
	return pane->config;
}

/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
