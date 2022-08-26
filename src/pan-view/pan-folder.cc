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

#include "pan-folder.h"

#include <math.h>

#include "pan-item.h"
#include "pan-util.h"
#include "pan-view-filter.h"

static void pan_flower_size(PanWindow *pw, gint *width, gint *height)
{
	GList *work;
	gint x1, y1, x2, y2;

	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;

	work = pw->list;
	while (work)
		{
		PanItem *pi;

		pi = work->data;
		work = work->next;

		if (x1 > pi->x) x1 = pi->x;
		if (y1 > pi->y) y1 = pi->y;
		if (x2 < pi->x + pi->width) x2 = pi->x + pi->width;
		if (y2 < pi->y + pi->height) y2 = pi->y + pi->height;
		}

	x1 -= PAN_BOX_BORDER;
	y1 -= PAN_BOX_BORDER;
	x2 += PAN_BOX_BORDER;
	y2 += PAN_BOX_BORDER;

	work = pw->list;
	while (work)
		{
		PanItem *pi;

		pi = work->data;
		work = work->next;

		pi->x -= x1;
		pi->y -= y1;

		if (pi->type == PAN_ITEM_TRIANGLE && pi->data)
			{
			gint *coord;

			coord = pi->data;
			coord[0] -= x1;
			coord[1] -= y1;
			coord[2] -= x1;
			coord[3] -= y1;
			coord[4] -= x1;
			coord[5] -= y1;
			}
		}

	if (width) *width = x2 - x1;
	if (height) *height = y2 - y1;
}

typedef struct _FlowerGroup FlowerGroup;
struct _FlowerGroup {
	GList *items;
	GList *children;
	gint x;
	gint y;
	gint width;
	gint height;

	gdouble angle;
	gint circumference;
	gint diameter;
};

static void pan_flower_move(FlowerGroup *group, gint x, gint y)
{
	GList *work;

	work = group->items;
	while (work)
		{
		PanItem *pi;

		pi = work->data;
		work = work->next;

		pi->x += x;
		pi->y += y;
		}

	group->x += x;
	group->y += y;
}

#define PI 3.14159265

static void pan_flower_position(FlowerGroup *group, FlowerGroup *parent,
							     gint *result_x, gint *result_y)
{
	gint x, y;
	gint radius;
	gdouble a;

	radius = parent->circumference / (2*PI);
	radius = MAX(radius, parent->diameter / 2 + group->diameter / 2);

	a = 2*PI * group->diameter / parent->circumference;

	x = (gint)((gdouble)radius * cos(parent->angle + a / 2));
	y = (gint)((gdouble)radius * sin(parent->angle + a / 2));

	parent->angle += a;

	x += parent->x;
	y += parent->y;

	x += parent->width / 2;
	y += parent->height / 2;

	x -= group->width / 2;
	y -= group->height / 2;

	*result_x = x;
	*result_y = y;
}

static void pan_flower_build(PanWindow *pw, FlowerGroup *group, FlowerGroup *parent)
{
	GList *work;
	gint x, y;

	if (!group) return;

	if (parent && parent->children)
		{
		pan_flower_position(group, parent, &x, &y);
		}
	else
		{
		x = 0;
		y = 0;
		}

	pan_flower_move(group, x, y);

	if (parent)
		{
		PanItem *pi;
		gint px, py, gx, gy;
		gint x1, y1, x2, y2;

		px = parent->x + parent->width / 2;
		py = parent->y + parent->height / 2;

		gx = group->x + group->width / 2;
		gy = group->y + group->height / 2;

		x1 = MIN(px, gx);
		y1 = MIN(py, gy);

		x2 = MAX(px, gx + 5);
		y2 = MAX(py, gy + 5);

		pi = pan_item_tri_new(pw, NULL, x1, y1, x2 - x1, y2 - y1,
				      px, py, gx, gy, gx + 5, gy + 5,
				      255, 40, 40, 128);
		pan_item_tri_border(pi, PAN_BORDER_1 | PAN_BORDER_3,
				    255, 0, 0, 128);
		}

	pw->list = g_list_concat(group->items, pw->list);
	group->items = NULL;

	group->circumference = 0;
	work = group->children;
	while (work)
		{
		FlowerGroup *child;

		child = work->data;
		work = work->next;

		group->circumference += child->diameter;
		}

	work = g_list_last(group->children);
	while (work)
		{
		FlowerGroup *child;

		child = work->data;
		work = work->prev;

		pan_flower_build(pw, child, group);
		}

	g_list_free(group->children);
	g_free(group);
}

static FlowerGroup *pan_flower_group(PanWindow *pw, FileData *dir_fd, gint x, gint y)
{
	FlowerGroup *group;
	GList *f;
	GList *d;
	GList *work;
	PanItem *pi_box;
	gint x_start;
	gint y_height;
	gint grid_size;
	gint grid_count;

	if (!filelist_read(dir_fd, &f, &d)) return NULL;
	if (!f && !d) return NULL;

	f = filelist_sort(f, SORT_NAME, TRUE);
	d = filelist_sort(d, SORT_NAME, TRUE);

	pan_filter_fd_list(&f, pw->filter_ui->filter_elements, pw->filter_ui->filter_classes);

	pi_box = pan_item_text_new(pw, x, y, dir_fd->path, PAN_TEXT_ATTR_NONE,
				   PAN_TEXT_BORDER_SIZE,
				   PAN_TEXT_COLOR, 255);

	y += pi_box->height;

	pi_box = pan_item_box_new(pw, file_data_ref(dir_fd),
				  x, y,
				  PAN_BOX_BORDER * 2, PAN_BOX_BORDER * 2,
				  PAN_BOX_OUTLINE_THICKNESS,
				  PAN_BOX_COLOR, PAN_BOX_ALPHA,
				  PAN_BOX_OUTLINE_COLOR, PAN_BOX_OUTLINE_ALPHA);

	x += PAN_BOX_BORDER;
	y += PAN_BOX_BORDER;

	grid_size = (gint)(sqrt(g_list_length(f)) + 0.9);
	grid_count = 0;
	x_start = x;
	y_height = y;

	work = f;
	while (work)
		{
		FileData *fd;
		PanItem *pi;

		fd = work->data;
		work = work->next;

		if (pw->size > PAN_IMAGE_SIZE_THUMB_LARGE)
			{
			pi = pan_item_image_new(pw, fd, x, y, 10, 10);
			x += pi->width + PAN_THUMB_GAP;
			if (pi->height > y_height) y_height = pi->height;
			}
		else
			{
			pi = pan_item_thumb_new(pw, fd, x, y);
			x += PAN_THUMB_SIZE + PAN_THUMB_GAP;
			y_height = PAN_THUMB_SIZE;
			}

		grid_count++;
		if (grid_count >= grid_size)
			{
			grid_count = 0;
			x = x_start;
			y += y_height + PAN_THUMB_GAP;
			y_height = 0;
			}

		pan_item_size_by_item(pi_box, pi, PAN_BOX_BORDER);
		}

	group = g_new0(FlowerGroup, 1);
	group->items = pw->list;
	pw->list = NULL;

	group->width = pi_box->width;
	group->height = pi_box->y + pi_box->height;
	group->diameter = (gint)sqrt(group->width * group->width + group->height * group->height);

	group->children = NULL;

	work = d;
	while (work)
		{
		FileData *fd;
		FlowerGroup *child;

		fd = work->data;
		work = work->next;

		if (!pan_is_ignored(fd->path, pw->ignore_symlinks))
			{
			child = pan_flower_group(pw, fd, 0, 0);
			if (child) group->children = g_list_prepend(group->children, child);
			}
		}

	if (!f && !group->children)
		{
		work = group->items;
		while (work)
			{
			PanItem *pi;

			pi = work->data;
			work = work->next;

			pan_item_free(pi);
			}

		g_list_free(group->items);
		g_free(group);
		group = NULL;
		}

	g_list_free(f);
	filelist_free(d);

	return group;
}

void pan_flower_compute(PanWindow *pw, FileData *dir_fd,
			gint *width, gint *height,
			gint *scroll_x, gint *scroll_y)
{
	FlowerGroup *group;
	GList *list;

	group = pan_flower_group(pw, dir_fd, 0, 0);
	pan_flower_build(pw, group, NULL);

	pan_flower_size(pw, width, height);

	list = pan_item_find_by_fd(pw, PAN_ITEM_BOX, dir_fd, FALSE, FALSE);
	if (list)
		{
		PanItem *pi = list->data;
		*scroll_x = pi->x + pi->width / 2;
		*scroll_y = pi->y + pi->height / 2;
		}
	g_list_free(list);
}

static void pan_folder_tree_path(PanWindow *pw, FileData *dir_fd,
				 gint *x, gint *y, gint *level,
				 PanItem *parent,
				 gint *width, gint *height)
{
	GList *f;
	GList *d;
	GList *work;
	PanItem *pi_box;
	gint y_height = 0;

	if (!filelist_read(dir_fd, &f, &d)) return;
	if (!f && !d) return;

	f = filelist_sort(f, SORT_NAME, TRUE);
	d = filelist_sort(d, SORT_NAME, TRUE);

	pan_filter_fd_list(&f, pw->filter_ui->filter_elements, pw->filter_ui->filter_classes);

	*x = PAN_BOX_BORDER + ((*level) * MAX(PAN_BOX_BORDER, PAN_THUMB_GAP));

	pi_box = pan_item_text_new(pw, *x, *y, dir_fd->path, PAN_TEXT_ATTR_NONE,
				   PAN_TEXT_BORDER_SIZE,
				   PAN_TEXT_COLOR, 255);

	*y += pi_box->height;

	pi_box = pan_item_box_new(pw, file_data_ref(dir_fd),
				  *x, *y,
				  PAN_BOX_BORDER, PAN_BOX_BORDER,
				  PAN_BOX_OUTLINE_THICKNESS,
				  PAN_BOX_COLOR, PAN_BOX_ALPHA,
				  PAN_BOX_OUTLINE_COLOR, PAN_BOX_OUTLINE_ALPHA);

	*x += PAN_BOX_BORDER;
	*y += PAN_BOX_BORDER;

	work = f;
	while (work)
		{
		FileData *fd;
		PanItem *pi;

		fd = work->data;
		work = work->next;

		if (pw->size > PAN_IMAGE_SIZE_THUMB_LARGE)
			{
			pi = pan_item_image_new(pw, fd, *x, *y, 10, 10);
			*x += pi->width + PAN_THUMB_GAP;
			if (pi->height > y_height) y_height = pi->height;
			}
		else
			{
			pi = pan_item_thumb_new(pw, fd, *x, *y);
			*x += PAN_THUMB_SIZE + PAN_THUMB_GAP;
			y_height = PAN_THUMB_SIZE;
			}

		pan_item_size_by_item(pi_box, pi, PAN_BOX_BORDER);
		}

	if (f) *y = pi_box->y + pi_box->height;

	g_list_free(f);

	work = d;
	while (work)
		{
		FileData *fd;

		fd = work->data;
		work = work->next;

		if (!pan_is_ignored(fd->path, pw->ignore_symlinks))
			{
			*level = *level + 1;
			pan_folder_tree_path(pw, fd, x, y, level, pi_box, width, height);
			*level = *level - 1;
			}
		}

	filelist_free(d);

	pan_item_size_by_item(parent, pi_box, PAN_BOX_BORDER);

	if (*y < pi_box->y + pi_box->height + PAN_BOX_BORDER)
		*y = pi_box->y + pi_box->height + PAN_BOX_BORDER;

	pan_item_size_coordinates(pi_box, PAN_BOX_BORDER, width, height);
}

void pan_folder_tree_compute(PanWindow *pw, FileData *dir_fd, gint *width, gint *height)
{
	gint x, y;
	gint level;
	gint w, h;

	level = 0;
	x = PAN_BOX_BORDER;
	y = PAN_BOX_BORDER;
	w = PAN_BOX_BORDER * 2;
	h = PAN_BOX_BORDER * 2;

	pan_folder_tree_path(pw, dir_fd, &x, &y, &level, NULL, &w, &h);

	if (width) *width = w;
	if (height) *height = h;
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
