/*
 * Copyright (C) 2005 Lars Ellenberg
 * Copyright (C) 2005 John Ellis
 * Copyright (C) 2008 - 2016 The Geeqie Team
 *
 * Author: Lars Ellenberg
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

#ifndef __FORMAT_FUJI_H
#define __FORMAT_FUJI_H


#include "exif-int.h"


gboolean format_fuji_raw(guchar *data, const guint len,
		         guint *image_offset, guint *exif_offset);


#define FORMAT_RAW_FUJI { "raf", \
			  FORMAT_RAW_MATCH_MAGIC, 0, "FUJIFILM", 8, \
			  FORMAT_RAW_EXIF_JPEG, NULL, \
			  "Fuji raw", format_fuji_raw }


gboolean format_fuji_makernote(ExifData *exif, guchar *tiff, guint offset,
			       guint size, ExifByteOrder bo);

#define FORMAT_EXIF_FUJI { FORMAT_EXIF_MATCH_MAKERNOTE, "FUJIFILM", 8, "Fujifilm", format_fuji_makernote }



#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
