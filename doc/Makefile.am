## Process this file with automake to produce Makefile.in
info_TEXINFOS = sed.texi
sed_TEXINFOS = config.texi version.texi
dist_man_MANS = sed.1
dist_noinst_DATA = sed.x sed-in.texi
dist_noinst_SCRIPTS = groupify.sed
CLEANFILES = sed.html
TEXI2DVI = $(top_srcdir)/config/texi2dvi --expand
HELP2MAN = $(top_srcdir)/config/help2man
SED = $(top_builddir)/sed/sed

# To produce better quality output, in the example sed
# scripts we group comments with lines following them;
# since mantaining the "@group...@end group" manually
# is a burden, we do this automatically
$(srcdir)/sed.texi: sed-in.texi $(srcdir)/groupify.sed
	sed -nf $(srcdir)/groupify.sed \
	  < $(srcdir)/sed-in.texi > $(srcdir)/sed.texi

sed.1: $(top_srcdir)/sed/sed.c $(top_srcdir)/configure.ac $(srcdir)/sed.x
	$(HELP2MAN) --name "stream editor for filtering and transforming text" \
	  -p sed --include $(srcdir)/sed.x $(SED) > $(srcdir)/sed.1

dist-hook:
	touch $(distdir)/sed.1

# This rule is used if --enable-html is passed
if BUILD_HTML
docdir = $(datadir)/doc
pkgdocdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)
pkgdoc_DATA = sed.html

all: html

html: sed.html

.PHONY: html
endif

# Having a dependancy on sed.info automatically makes
# sed.html dependant on sed.texi and all the included
# sources
if MAKEINFO_HTML
sed.html: sed.texi sed.info
	builddir=`pwd` && cd $(srcdir) && \
	  $(MAKEINFO) --html --no-split -o $$builddir/sed.html sed.texi
endif

# These rules are used together with TEXI2HTML
if TEXI2HTML_HTML
sed.html: sed.texi sed.info
	cp $(srcdir)/*.texi . && \
	  $(TEXI2HTML) sed.texi && \
	  rm *.texi
endif
