# Uses libtool to create a googletest convenience library, so that we don't need to recompile it
# every single time we recompile our tests.
check_LTLIBRARIES = libgtest.la
libgtest_la_SOURCES = ../googletest/googletest/src/gtest-all.cc
libgtest_la_CPPFLAGS = -I$(top_srcdir)/googletest/googletest/include -I$(top_srcdir)/googletest/googletest
libgtest_la_LDFLAGS = -pthread

check_PROGRAMS = gtest

gtest_SOURCES = \
	gtest.cc \
	mock_debug.c \
	$(top_srcdir)/src/filedata/util.c

gtest_LDADD = libgtest.la $(GTK_LIBS) $(GLIB_LIBS)

gtest_LDFLAGS = -pthread

gtest_CPPFLAGS = \
	-I$(top_srcdir)/googletest/googletest/include \
	-I$(top_srcdir)/googletest/googletest -pthread \
	-I$(top_srcdir)/src $(GLIB_CFLAGS) $(GTK_CFLAGS)

TESTS = gtest
