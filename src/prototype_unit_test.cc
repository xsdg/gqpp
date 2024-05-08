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
 * Prototype unit test entrypoint.  To be replaced by something more formal
 * like GoogleTest.
 *
 */

#include "prototype_unit_test.h"

#include <cstdio>

#include "pixbuf-util.h"

namespace {

void assert_true(const gboolean condition, const gchar* assert_description)
{
	if(condition)
	{
		printf("A condition passed: %s\n", assert_description);
	} else {
		printf("A condition failed: %s\n", assert_description);
	}
}

void assert_eq(const int a, const int b, const gchar* assert_description)
{
	assert_true(a == b, assert_description);
}

}  // anonymous namespace

int run_tests()
{
	printf("Unit test run function\n");

	gint rx, ry, rw, rh;
	assert_true(util_clip_region(0, 0, 100, 100,
				     50, 50, 100, 100,
				     &rx, &ry, &rw, &rh),
		    "util_clip_region found overlap");

	assert_eq(50, rx, "rx as expected");
	assert_eq(50, ry, "ry as expected");
	assert_eq(50, rw, "rw as expected");
	assert_eq(50, rh, "rh as expected");

	assert_eq(0xabc, 0xdef, "expected to fail");

	return 0;
}

/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
