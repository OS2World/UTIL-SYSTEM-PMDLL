# makefile.wat - make pmdll with OpenWatcom
# contributed by Michal N

# Copyright (c) 2001, 2012 Steven Levine and Associates, Inc.
# All rights reserved.

# $TLIB$: $ &(#) %n - Ver %v, %f $
# TLIB: $ &(#) makefile.wat - Ver 4, 12-May-12,20:11:38 $

# 2005-08-02 MN Adapt from makefile.vac
# 2006-03-15 SHL Add debug, exceptq and lxlite support
# 2012-04-22 SHL Allow build options from environment or command line
# 2012-04-22 SHL Switch to dynamic exceptq support
# 2012-05-12 SHL Comments

# Requires OpenWatcom 1.4 wmake or better
# Requires INCLUDE and LIB like
# set INCLUDE=.;D:\WATCOM\H;D:\TOOLKIT\H
# set LIB=.;D:\TOOLKIT\LIB;;D:\WATCOM\LIB386\OS2;D:\WATCOM\LIB386
# Requires lxlite and mapxql

# Define DEBUG for debug build
# Define EXCEPTQ to build with exceptq support

# Requires
#  _VENDOR defined in environment
#  _VERSION defined in environment
#  BUILDTIME defined in environment

!ifndef %WATCOM
!error WATCOM not defined
!endif

!ifndef %_VENDOR
!error _VENDOR not defined
!endif

!ifndef %_VERSION
!error _VERSION not defined
!endif

!ifndef %BUILDTIME
!error BUILDTIME not defined (YYYY/MM/YY HH:MM:SS)
!endif

!ifdef %DEBUG			# if defined in environment
DEBUG = $(%DEBUG)		# use value from environment
!endif

!ifdef %EXCEPTQ			# if defined in environment
EXCEPTQ = $(%EXCEPTQ)		# use value from environment
!endif

.SUFFIXES: .res .rc

# -bm		multithread libs
# -bt=os2	target
# -d2		full debug
# -d3		full debug w/unref
# -hd		dwarf
# -j		signed char
# -mf		flat
# -olinars	optimze loops, inline, e(n)able fp recip, relax (a)lias, reordering, space
# -s		disable stack checks
# -sg		generate calls to grow the stack
# -st		touch stack through SS first
# -wx		max warnings
# -zfp		disable fs use
# -zgp		disable gs use
# -zp4		align 4
# -zq		quiet

NAME = pmdll

CC=wcc386

!ifdef DEBUG
CFLAGS = -bt=os2 -mf -bm -d2 -olirs -s -j -wx -zfp -zgp -zq -hd
!else
CFLAGS = -bt=os2 -mf -bm -olirs -s -j -wx -zfp -zgp -zq -hd
!endif

!ifdef EXCEPTQ
CFLAGS += -dEXCEPTQ
!endif

RC=wrc
RCFLAGS = -ad -r -bt=os2

LINK = wlink

!ifdef DEBUG
# LFLAGS += ,debug all
!endif

.c.obj : .autodepend
    $(CC) $(CFLAGS) $<

.rc.res: .autodepend
    $(RC) $(RCFLAGS) $<

OBJS = trace.obj tree_wnd.obj chgdir.obj exehdr.obj finddll.obj &
       hlp_func.obj libpath.obj pmdll.obj print.obj sel_file.obj &
       sel_font.obj sysdll.obj

all: pmdll.exe .symbolic

$(NAME).lrf: makefile.wat
    @%write $^@ system os2v2_pm
    @%write $^@ option quiet
    @%write $^@ option cache
    @%write $^@ option caseexact
!ifdef DEBUG
    @%write $^@ debug all
!endif
    @%write $^@ option map
    @%write $^@ name $(NAME)
    @for %f in ($(OBJS)) do @%append $^@ file %f
    @%write $^@ op desc '@$#$(%_VENDOR):$(%_VERSION)$#@$#$#1$#$# $(%BUILDTIME)      SLAMain::EN:US:0:U:@@PM DLL analyzer'

$(NAME).exe $(NAME).sym $(NAME).xqs: $(OBJS) makefile.wat $(NAME).lrf $(NAME).res
    $(LINK) @$(NAME).lrf
    $(RC) -q $(NAME).res $@
    if exist d:\cmd\mapsymw.pl perl d:\cmd\mapsymw.pl $(NAME).map
    mapxqs $(NAME).map
    bldlevel $@

lxlite: pmdll.exe .symbolic
    dir $<
    lxlite -X- -B- $<
    bldlevel $<
    dir $<

clean : .symbolic
    @for %f in ( "*.obj" *.exe *.res *.lrf *.sym *.map *.xqs ) do if exist %f del %f

# The end
