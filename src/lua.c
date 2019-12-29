/*
 * Copyright (C) 2008 - 2016 The Geeqie Team
 *
 * Author: Klaus Ethgen
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

#include "config.h"

#ifdef HAVE_LUA

#define _XOPEN_SOURCE

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <time.h>

#include "main.h"
#include "glua.h"
#include "ui_fileops.h"
#include "exif.h"

static lua_State *L; /** The LUA object needed for all operations (NOTE: That is
		       * a upper-case variable to match the documentation!) */

/* Taking that definition from lua 5.1 source */
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM >= 502
int luaL_typerror(lua_State *L, int narg, const char *tname)
{
	const char *msg = lua_pushfstring(L, "%s expected, got %s", tname, luaL_typename(L, narg));
	return luaL_argerror(L, narg, msg);
}

# define LUA_register_meta(L, meta) luaL_setfuncs(L, meta, 0);
# define LUA_register_global(L, string, func) \
	lua_newtable(L); \
	luaL_setfuncs(L, func, 0); \
	lua_pushvalue(L, -1); \
	lua_setglobal(L, string)
#else
# define LUA_register_meta(L, meta) luaL_register(L, NULL, meta)
# define LUA_register_global(L, string, func) luaL_register(L, string, func)
#endif

static FileData *lua_check_image(lua_State *L, int index)
{
	FileData **fd;
	luaL_checktype(L, index, LUA_TUSERDATA);
	fd = (FileData **)luaL_checkudata(L, index, "Image");
	if (fd == NULL) luaL_typerror(L, index, "Image");
	return *fd;
}

static int lua_image_get_exif(lua_State *L)
{
	FileData *fd;
	ExifData *exif;
	ExifData **exif_data;

	fd = lua_check_image(L, 1);
	exif = exif_read_fd(fd);

	exif_data = (ExifData **)lua_newuserdata(L, sizeof(ExifData *));
	luaL_getmetatable(L, "Exif");
	lua_setmetatable(L, -2);

	*exif_data = exif;

	return 1;
}

static int lua_image_get_path(lua_State *L)
{
	FileData *fd;

	fd = lua_check_image(L, 1);
	lua_pushstring(L, fd->path);
	return 1;
}

static int lua_image_get_name(lua_State *L)
{
	FileData *fd;

	fd = lua_check_image(L, 1);
	lua_pushstring(L, fd->name);
	return 1;
}

static int lua_image_get_extension(lua_State *L)
{
	FileData *fd;

	fd = lua_check_image(L, 1);
	lua_pushstring(L, fd->extension);
	return 1;
}

static int lua_image_get_date(lua_State *L)
{
	FileData *fd;

	fd = lua_check_image(L, 1);
	lua_pushnumber(L, fd->date);
	return 1;
}

static int lua_image_get_size(lua_State *L)
{
	FileData *fd;

	fd = lua_check_image(L, 1);
	lua_pushnumber(L, fd->size);
	return 1;
}

static int lua_image_get_marks(lua_State *L)
{
	FileData *fd;

	fd = lua_check_image(L, 1);
	lua_pushnumber(L, fd->marks);
	return 1;
}

static ExifData *lua_check_exif(lua_State *L, int index)
{
	ExifData **exif;
	luaL_checktype(L, index, LUA_TUSERDATA);
	exif = (ExifData **)luaL_checkudata(L, index, "Exif");
	if (exif == NULL) luaL_typerror(L, index, "Exif");
	return *exif;
}

/* Interface for EXIF data */
static int lua_exif_get_datum(lua_State *L)
{
	const gchar *key;
	gchar *value = NULL;
	ExifData *exif;
	struct tm tm;
	time_t datetime;

	exif = lua_check_exif(L, 1);
	key = luaL_checkstring(L, 2);
	if (key == (gchar*)NULL || key[0] == '\0')
		{
		lua_pushnil(L);
		return 1;
		}
	if (!exif)
		{
		lua_pushnil(L);
		return 1;
		}
	value = exif_get_data_as_text(exif, key);
	if (strcmp(key, "Exif.Photo.DateTimeOriginal") == 0)
		{
		memset(&tm, 0, sizeof(tm));
		if (value && strptime(value, "%Y:%m:%d %H:%M:%S", &tm))
			{
			datetime = mktime(&tm);
			lua_pushnumber(L, datetime);
			return 1;
			}
		else
			{
			lua_pushnil(L);
			return 1;
			}
		}
	else if (strcmp(key, "Exif.Photo.DateTimeDigitized") == 0)
		{
		memset(&tm, 0, sizeof(tm));
		if (value && strptime(value, "%Y:%m:%d %H:%M:%S", &tm))
			{
			datetime = mktime(&tm);
			lua_pushnumber(L, datetime);
			return 1;
			}
		else
			{
			lua_pushnil(L);
			return 1;
			}
		}
	lua_pushstring(L, value);
	return 1;
}

/**
 * \brief Initialize the lua interpreter.
 */
void lua_init(void)
{
	L = luaL_newstate();
	luaL_openlibs(L); /* Open all libraries for lua programms */

	/* Now create custom methodes to do something */
	static const luaL_Reg meta_methods[] = {
			{NULL, NULL}
	};

	/* The Image metatable and methodes */
	static const luaL_Reg image_methods[] = {
			{"get_path", lua_image_get_path},
			{"get_name", lua_image_get_name},
			{"get_extension", lua_image_get_extension},
			{"get_date", lua_image_get_date},
			{"get_size", lua_image_get_size},
			{"get_exif", lua_image_get_exif},
			{"get_marks", lua_image_get_marks},
			{NULL, NULL}
	};
	LUA_register_global(L, "Image", image_methods);
	luaL_newmetatable(L, "Image");
	LUA_register_meta(L, meta_methods);
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 1);
	lua_pop(L, 1);

	/* The Exif table and methodes */
	static const luaL_Reg exif_methods[] = {
			{"get_datum", lua_exif_get_datum},
			{NULL, NULL}
	};
	LUA_register_global(L, "Exif", exif_methods);
	luaL_newmetatable(L, "Exif");
	LUA_register_meta(L, meta_methods);
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 1);
	lua_pop(L, 1);
}

/**
 * \brief Call a lua function to get a single value.
 */
gchar *lua_callvalue(FileData *fd, const gchar *file, const gchar *function)
{
	gint result;
	gchar *data = NULL;
	gchar *path;
	FileData **image_data;
	gchar *tmp;
	GError *error = NULL;
	gboolean ok;

	ok = access(g_build_filename(get_rc_dir(), "lua", file, NULL), R_OK);
	if (ok == 0)
		{
		path = g_build_filename(get_rc_dir(), "lua", file, NULL);
		}
	else
		{
		/* FIXME: what is the correct way to find the scripts folder? */
		ok = access(g_build_filename("/usr/local/lib", GQ_APPNAME_LC, file, NULL), R_OK);
		if (ok == 0)
			{
			path = g_build_filename("/usr/local/lib", GQ_APPNAME_LC, file, NULL);
			}
		else
			{
			return g_strdup("");
			}
		}

	/* Collection Table (Dummy at the moment) */
	lua_newtable(L);
	lua_setglobal(L, "Collection");

	/* Current Image */
	image_data = (FileData **)lua_newuserdata(L, sizeof(FileData *));
	luaL_getmetatable(L, "Image");
	lua_setmetatable(L, -2);
	lua_setglobal(L, "Image");

	*image_data = fd;
	if (file[0] == '\0')
		{
		result = luaL_dostring(L, function);
		}
	else
		{
		result = luaL_dofile(L, path);
		g_free(path);
		}

	if (result)
		{
		data = g_strdup_printf("Error running lua script: %s", lua_tostring(L, -1));
		return data;
		}
	data = g_strdup(lua_tostring(L, -1));
	tmp = g_locale_to_utf8(data, strlen(data), NULL, NULL, &error);
	if (error)
		{
		log_printf("Error converting lua output from locale to UTF-8: %s\n", error->message);
		g_error_free(error);
		}
	else
		{
		g_free(data);
		data = g_strdup(tmp);
		} // if (error) { ... } else
	return data;
}

#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
