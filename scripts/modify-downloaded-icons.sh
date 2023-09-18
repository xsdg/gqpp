#!/bin/sh
#**********************************************************************
# Copyright (C) 2023 - The Geeqie Team
#
# Author: Colin Clark
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#**********************************************************************

## @file
## @brief Convert downloaded icons
##
## Rename downloaded icons to Geeqie name
## Invert colors for dark theme and save with "-dark" filename
##
##


input_array="
arrows.png  gq-icon-zoomfillhor https://www.flaticon.com/free-icon/arrows_9847398
black-and-white.png gq-icon-grayscale https://www.flaticon.com/free-icon/black-and-white_3099713
bookmark.png gq-icon-marks https://www.flaticon.com/free-icon/bookmark_2099170
checkbox.png gq-icon-select-invert https://www.flaticon.com/free-icon/checkbox_6948194
database-management.png gq-icon-maintenance https://www.flaticon.com/free-icon/database-management_9292424
data-synchronization.png gq-icon-split-pane-sync https://www.flaticon.com/free-icon/data-synchronization_4882652
double-arrow-vertical-symbol.png  gq-icon-zoomfillvert https://www.flaticon.com/free-icon/double-arrow-vertical-symbol_54668
edit.png gq-icon-rename https://www.flaticon.com/free-icon/edit_1159633
error.png  gq-icon-broken https://www.flaticon.com/free-icon/error_3152157
exif.png gq-icon-exif https://www.flaticon.com/free-icon/exif_6393981
exposure.png gq-icon-exposure https://www.flaticon.com/free-icon/exposure_2214025
frame.png gq-icon-select-rectangle https://www.flaticon.com/free-icon/frame_4864813
heic.png gq-icon-heic https://www.flaticon.com/free-icon/heic_6393991
move-right.png gq-icon-move https://www.flaticon.com/free-icon/move-right_10515829
panorama.png gq-icon-panorama https://www.flaticon.com/free-icon/panorama_8207268
paper-pin.png gq-icon-float https://www.flaticon.com/free-icon/paper-pin_3378283
paper.png gq-icon-hidetools https://www.flaticon.com/free-icon/paper_11028332
pdf.png gq-icon-pdf https://www.flaticon.com/free-icon/pdf_201153
restore-down.png gq-icon-select-none https://www.flaticon.com/free-icon/restore-down_4903563
rotate.png gq-icon-original https://www.flaticon.com/free-icon/rotate_764623
select.png  gq-icon-select-all https://www.flaticon.com/free-icon/select_7043937
thumbnails-1.png gq-icon-collection https://www.flaticon.com/free-icon/thumbnails_204593
thumbnails.png gq-icon-thumb https://www.flaticon.com/free-icon/thumbnails_204592
transform.png gq-icon-draw-rectangle https://www.flaticon.com/free-icon/rectangle_3496559
two-clockwise-circular-rotating-arrows-circle.png gq-icon-rotate-180 https://www.flaticon.com/free-icon/two-clockwise-circular-rotating-arrows-circle_54529
unknown.png gq-icon-unknown https://www.flaticon.com/free-icon/unknown_9166172
video.png gq-icon-video https://www.flaticon.com/free-icon/video_10260807
workflow.png gq-icon-sort https://www.flaticon.com/free-icon/workflow_3748469
xmp.png gq-icon-metadata https://www.flaticon.com/free-icon/xmp_10260892
zip.png gq-icon-archive-file https://www.flaticon.com/free-icon/zip_201199
"

i=0
for file in $input_array
do
	if [ "$i" -eq 0 ]
	then
		input="$HOME/Downloads/$file"
		i=$((i + 1))
	else
		if [ "$i" -eq 1 ]
		then
			output="$file"
			cp "$input" "./src/icons/$output.png"
			convert "$input" -fill '#ffffff' -opaque black "./src/icons/$output-dark.png"
			i=$((i + 1))
		else
			i=0
		fi
	fi
done
