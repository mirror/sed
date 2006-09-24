## Process this file with automake to produce Makefile.in

lib_SOURCES = 
EXTRA_DIST =
BUILT_SOURCES =
MOSTLYCLEANFILES =
MOSTLYCLEANDIRS =
include gnulib.mk

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)/intl -I$(top_srcdir) \
	-I$(top_builddir)/lib -I$(top_builddir)/intl

noinst_LIBRARIES = libsed.a
libsed_a_SOURCES = $(lib_SOURCES)
libsed_a_LIBADD = @LIBOBJS@ @ALLOCA@
libsed_a_DEPENDENCIES = $(libsed_a_LIBADD)

DISTCLEANFILES = regex.h stdbool.h
