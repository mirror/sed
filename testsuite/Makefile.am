CLEANFILES = tmp* core *.core $(EXTRA_PROGRAMS) *.*out *.log

TESTS = $(check_PROGRAMS) $(SEDTESTS)

pcretest_LDADD = ../pcre/libpcre.a ../lib/libsed.a @INTLLIBS@
pcretest_DEPENDENCIES = ../pcre/libpcre.a ../lib/libsed.a
check_PROGRAMS = pcretest
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/pcre \
	-I$(top_builddir)/intl -I$(top_srcdir)/intl

SEDTESTS = \
        space appquit enable sep inclib 8bit newjis xabcx dollar noeol noeolw \
  	numsub numsub2 numsub3 numsub4 numsub5 0range bkslashes \
  	head madding mac-mf empty xbxcx xbxcx3 recall recall2 xemacs \
  	fasts uniq manis khadafy linecnt eval distrib 8to7 \
	allsub cv-vars classes middle bsd stdin flipcase \
	insens subwrite writeout readin \
	help version file quiet \
	factor binary3 binary2 binary dc

TESTS_ENVIRONMENT = MAKE="$(MAKE)" VERSION="$(VERSION)" $(srcdir)/runtest

EXTRA_DIST = runtest Makefile.tests \
        pcre1.good pcre1.inp pcre1p.good \
        pcre2.good pcre2.inp pcre2p.good \
        pcre3.good pcre3.inp pcre3p.good \
	0range.good 0range.inp 0range.sed \
	8bit.good 8bit.inp 8bit.sed \
	8to7.good 8to7.inp 8to7.sed \
	allsub.good allsub.inp allsub.sed \
	appquit.good appquit.inp appquit.sed \
	binary.good binary.inp binary.sed binary2.sed binary3.sed \
	bkslashes.good bkslashes.inp bkslashes.sed \
	bsd.good bsd.sh \
	cv-vars.good cv-vars.inp cv-vars.sed \
	classes.good classes.inp classes.sed \
	dc.good dc.inp dc.sed \
	distrib.good distrib.inp distrib.sed distrib.sh \
	dollar.good dollar.inp dollar.sed \
	empty.good empty.inp empty.sed \
	enable.good enable.inp enable.sed \
	eval.good eval.inp eval.sed \
	factor.good factor.inp factor.sed \
	fasts.good fasts.inp fasts.sed \
	flipcase.good flipcase.inp flipcase.sed \
	head.good head.inp head.sed \
	inclib.good inclib.inp inclib.sed \
	insens.good insens.inp insens.sed \
	khadafy.good khadafy.inp khadafy.sed \
	linecnt.good linecnt.inp linecnt.sed \
	space.good space.inp space.sed \
	mac-mf.good mac-mf.inp mac-mf.sed \
	madding.good madding.inp madding.sed \
	manis.good manis.inp manis.sed \
	middle.good middle.sed middle.inp \
	newjis.good newjis.inp  newjis.sed \
	noeol.good noeol.inp noeol.sed \
	noeolw.good noeolw.1good noeolw.2good noeolw.sed \
	numsub.good numsub.inp numsub.sed \
	numsub2.good numsub2.inp numsub2.sed \
	numsub3.good numsub3.inp numsub3.sed \
	numsub4.good numsub4.inp numsub4.sed \
	numsub5.good numsub5.inp numsub5.sed \
	readin.good readin.in2 readin.inp readin.sed \
	recall.good recall.inp recall.sed \
	recall2.good recall2.inp recall2.sed \
	sep.good sep.inp sep.sed \
	subwrite.inp subwrite.sed  subwrt1.good subwrt2.good \
	uniq.good uniq.inp uniq.sed \
	version.gin \
	writeout.inp writeout.sed wrtout1.good wrtout2.good \
	xabcx.good xabcx.inp xabcx.sed \
	xbxcx.good xbxcx.inp xbxcx.sed \
	xbxcx3.good xbxcx3.inp xbxcx3.sed \
	xemacs.good xemacs.inp xemacs.sed

clean-local:
	test x$(srcdir) = x. || rm -f readin.in2 eval.in2

# automake makes `check' depend on $(TESTS).  Declare
# dummy targets for $(TESTS) so that make does not complain.

.PHONY: $(SEDTESTS)
$(SEDTESTS):
