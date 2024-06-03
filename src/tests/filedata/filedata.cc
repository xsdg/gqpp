/*
 * Copyright (C) 2024 The Geeqie Team
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
 *
 *
 * Unit tests for pixbuf-util.cc
 *
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <string>
#include <utility>
#include <vector>

#include <glib.h>

#include "filedata.h"

namespace {

// For convenience.
namespace t = ::testing;

class FileDataTest : public t::Test
{
    protected:
	void TearDown() override
	{
		if (fd != nullptr)
		{
			g_free(fd);
			fd = nullptr;
		}
		if (parent_fd != nullptr)
		{
			g_free(parent_fd);
			parent_fd = nullptr;
		}
	}

	FileData *fd = nullptr;
	FileData *parent_fd = nullptr;
};

TEST_F(FileDataTest, text_from_size_test)
{
	std::vector<std::pair<gint64, std::string>> test_cases = {
		{0, "0"},
		{1, "1"},
		{999, "999"},
		{1000, "1,000"},
		{1'000'000, "1,000,000"},
		{-1000, "-1,000"},

		// The following test fails.  The right solution is probably to alter
		// text_from_size to accept a guint64 instead of gint64.
		// {-100'000, "-100,000"},
	};

	for (const auto &test_case : test_cases)
	{
		gchar *generated = FileData::text_from_size(test_case.first);
		ASSERT_EQ(test_case.second, std::string(generated));
		g_free(generated);
	}
}

TEST_F(FileDataTest, text_from_size_abrev_test)
{
	constexpr gint64 kib_threshold = 1024;
	constexpr gint64 mib_threshold = 1024 * 1024;
	constexpr gint64 gib_threshold = 1024 * 1024 * 1024;
	std::vector<std::pair<gint64, std::string>> test_cases = {
		{0, "0 bytes"},
		{1, "1 bytes"},
		{kib_threshold - 1, "1023 bytes"},
		{kib_threshold, "1.0 KiB"},
		{kib_threshold * 1.5, "1.5 KiB"},
		{kib_threshold * 2, "2.0 KiB"},

		{mib_threshold - 1, "1024.0 KiB"},
		{mib_threshold, "1.0 MiB"},
		{mib_threshold * 1.5, "1.5 MiB"},
		{mib_threshold * 2, "2.0 MiB"},

		{gib_threshold - 1, "1024.0 MiB"},
		{gib_threshold, "1.0 GiB"},
		{gib_threshold * 1.5, "1.5 GiB"},
		{gib_threshold * 2, "2.0 GiB"},
		{gib_threshold * 2048, "2048.0 GiB"},
	};

	for (const auto &test_case : test_cases)
	{
		gchar* generated = FileData::text_from_size_abrev(test_case.first);
		ASSERT_EQ(test_case.second, std::string(generated));
		g_free(generated);
	}
}

TEST_F(FileDataTest, BasicIncrementVersion)
{
	fd = g_new0(FileData, 1);
	fd->valid_marks = 0x4;

	file_data_increment_version(fd);

	ASSERT_EQ(1, fd->version);
	ASSERT_EQ(0x0, fd->valid_marks);
}

TEST_F(FileDataTest, BasicIncrementVersionWithParent)
{
	parent_fd = g_new0(FileData, 1);
	parent_fd->valid_marks = 0x8;
	fd = g_new0(FileData, 1);
	fd->valid_marks = 0x4;
	fd->parent = parent_fd;

	file_data_increment_version(fd);

	ASSERT_EQ(1, fd->version);
	ASSERT_EQ(0x0, fd->valid_marks);
	ASSERT_EQ(1, parent_fd->version);
	ASSERT_EQ(0x0, parent_fd->valid_marks);
}

}  // anonymous namespace

/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
