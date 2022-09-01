/*
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

#ifndef __FORMAT_RAW_H
#define __FORMAT_RAW_H

#include "exif-int.h"


typedef enum {
	FORMAT_RAW_MATCH_MAGIC,
	FORMAT_RAW_MATCH_TIFF_MAKE
} FormatRawMatchType;

typedef enum {
	FORMAT_RAW_EXIF_NONE,
	FORMAT_RAW_EXIF_TIFF,
	FORMAT_RAW_EXIF_JPEG,
	FORMAT_RAW_EXIF_IFD_II,
	FORMAT_RAW_EXIF_IFD_MM,
	FORMAT_RAW_EXIF_PROPRIETARY
} FormatRawExifType;

typedef gboolean (* FormatRawParseFunc)(guchar *data, const guint len,
				        guint *image_offset, guint *exif_offset);

typedef gboolean (* FormatRawExifParseFunc)(guchar *data, const guint len,
					    ExifData *exif);

gboolean format_raw_img_exif_offsets(guchar *data, const guint len,
				     guint *image_offset, guint *exif_offset);
gboolean format_raw_img_exif_offsets_fd(gint fd, const gchar *path,
				        guchar *header_data, const guint header_len,
				        guint *image_offset, guint *exif_offset);

FormatRawExifType format_raw_exif_offset(guchar *data, const guint len, guint *exif_offset,
					 FormatRawExifParseFunc *exif_parse_func);


typedef enum {
	FORMAT_EXIF_MATCH_MAKE,
	FORMAT_EXIF_MATCH_MAKERNOTE
} FormatExifMatchType;

typedef gint (* FormatExifParseFunc)(ExifData *exif, guchar *tiff, guint offset,
				     guint size, ExifByteOrder bo);

gboolean format_exif_makernote_parse(ExifData *exif, guchar *tiff, guint offset,
				     guint size, ExifByteOrder bo);


#define DEBUG_RAW_TIFF 0

#if DEBUG_RAW_TIFF

#define FORMAT_RAW_DEBUG_TIFF { "", \
				FORMAT_RAW_MATCH_MAGIC,     0, "II", 2, \
				FORMAT_RAW_EXIF_NONE, NULL, \
				"Tiff debugger II", format_debug_tiff_raw }, \
			      { "", \
				FORMAT_RAW_MATCH_MAGIC,     0, "MM", 2, \
				FORMAT_RAW_EXIF_NONE, NULL, \
				"Tiff debugger MM", format_debug_tiff_raw }

/**
 * @headerfile format_debug_tiff_raw
 * used for debugging only
 */
gint format_debug_tiff_raw(guchar *data, const guint len,
			   guint *image_offset, guint *exif_offset);
#endif

#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
