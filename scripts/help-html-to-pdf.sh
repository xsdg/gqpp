#!/bin/sh
#**********************************************************************
# Copyright (C) 2024 - The Geeqie Team
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
## @brief Create a pdf version of the help file from html
##
## $1 Source html dir
## $2 Path to build root
## $3 Relative path to output file
##
## wkhtmltopdf v12.6 with patched qt is required.
##
## wget https://github.com/wkhtmltopdf/packaging/releases/download/0.12.6.1-2/wkhtmltox_0.12.6.1-2.jammy_amd64.deb
## sudo apt install -f ./wkhtmltox_0.12.6.1-2.jammy_amd64.deb
##

if [ -z "$(command -v wkhtmltopdf)" ]
then
	printf "ERROR: wkhtmltopdf is not installed"
	exit 1
fi

cd "$1" || exit 1

echo "GuideIndex.html Features.html GuideIntroduction.html GuideMainWindow.html GuideMainWindowFilePane.html GuideMainWindowFolderPane.html GuideMainWindowImagePane.html GuideMainWindowNavigation.html GuideMainWindowMenus.html GuideMainWindowStatusBar.html GuideMainWindowLayout.html GuideSidebars.html GuideSidebarsInfo.html GuideSidebarsSortManager.html GuideOtherWindows.html GuideOtherWindowsImageWindow.html GuideOtherWindowsExif.html GuideOtherWindowsPanView.html GuideCollections.html GuideImageSearch.html GuideImageSearchCache.html GuideImageSearchSearch.html GuideImageSearchFindingDuplicates.html GuideImageMarks.html GuideImageManagementPlugins.html GuideImageManagement.html GuideImageManagementCopyMove.html GuideImageManagementRename.html GuideImageManagementDelete.html GuideReferenceManagement.html GuideColorManagement.html GuideImagePresentation.html GuideImagePresentationSlideshow.html GuideImagePresentationFullscreen.html GuidePrinting.html Optionstab.html GuideOptionsMain.html GuideOptionsGeneral.html GuideOptionsImage.html GuideOptionsOSD.html GuideOptionsWindow.html GuideOptionsKeyboard.html GuideOptionsFiltering.html GuideOptionsMetadata.html GuideOptionsKeywords.html GuideOptionsColor.html GuideOptionsStereo.html GuideOptionsBehavior.html GuideOptionsToolbar.html GuideOptionsAdvanced.html GuideOptionsAdditional.html GuideOptionsLayout.html GuidePluginsConfig.html GuideOptionsHidden.html GuideReference.html GuideReferenceCommandLine.html GuideReferenceRemoteKeyboardActions.html GuideReferenceKeyboardShortcuts.html GuideReferenceThumbnails.html GuideReferenceMetadata.html GuideReferenceLua.html GuideReferenceLuaAPI.html ./lua-api/html/lua_8cc.html GuideReferenceConfig.html GuideReferenceFileDates.html GuideReferenceXmpExif.html GuideReferenceSupportedFormats.html GuideReferenceSimilarityAlgorithms.html GuideReferencePixbufLoaders.html GuideReferenceStandardPlugins.html GuideReferenceUTC.html GuideReferenceDecodeLatLong.html GuideReferenceStandards.html GuideReferencePCRE.html GuideOtherSoftware.html OtherSoftware.html GuideFaq.html TrashFailed.html Imageviewing.html Commandline.html Miscellaneous.html ExifRotation.html GuideLicence.html GuideCredits.html GuideGnuFdl.html $2/$3" | wkhtmltopdf --load-error-handling ignore --enable-local-file-access --read-args-from-stdin 2>/dev/null

