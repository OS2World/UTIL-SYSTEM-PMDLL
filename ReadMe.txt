
PMDLL.EXE version 2.11                          	02 Apr 2013
======================

PMDLL is an OS/2 presentation manager program that can show you the of DLLs
loaded by an OS/2 executable file or by a DLL.  Both 16 and 32 bit formats
are supported. Sometimes it is not obvious why a program will not start
(correctly) or behaves strangely. When a program uses DLLs one of the causes
of incorrect behavior can be that a DLL cannot be found or that a DLL is
loaded from an unexpected directory that contains an incorrect version of
the DLL. PMDLL gives you a tool to detect and analyze these kind of
problems.

  This utility is placed in Public Domain and can be considered
  freeware, this utility may not be sold, hired, leased or modified,
  it may be freely copied as a complete package.

Warranty
========

EXCEPT AS OTHERWISE RESTRICTED BY LAW, THIS WORK IS PROVIDED WITHOUT ANY
EXPRESSED OR IMPLIED WARRANTIES OF ANY KIND, INCLUDING BUT NOT LIMITED TO,
ANY IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABILITY
OR TITLE.  EXCEPT AS OTHERWISE PROVIDED BY LAW, NO AUTHOR, COPYRIGHT HOLDER
OR LICENSOR SHALL BE LIABLE TO YOU FOR DAMAGES OF ANY KIND, EVEN IF THEY
HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

About DLLs
==========

Most OS/2 programs require one or more DLLs which are loaded by OS/2 when the
program is started. Many of these DLLs can be found in the \OS2\DLL
directory on your boot volume.  However, many programs distributed with DLLs
of their own.  Others use runtime DLLs associated with a particular
compiler, such as gcc. Problems start when a DLL required by a program is
missing or can not be loaded or when more than one version of a DLL is
present on your system. To find DLLs, OS/2 uses the search path
defined in the CONFIG.SYS LIBPATH statement along with the current settings
of the extended libpath (BEGINLIBPATH and ENDPATH and LIBPATHSTRICT) for the
session.

A minimal installation of OS/2 might have a LIBPATH that looks like:

  LIBPATH=.;C:\OS2\DLL;C:\OS2\MDOS;C:\;

This LIBPATH statement contains 4 entries:

  .             = Search current directory (some older versions of OS/2 did
                  not add this entry after installation which can be very
                  handy, check your config.sys).
  C:\OS2\DLL    = The OS/2 system DLL directory.
  C:\OS2\MDOS   = The OS/2 DOS system DLL directory.
  C:\           = The root of your OS/2 system disk.

Consider the following example. Suppose you have installed a program called
APROG.EXE together with a DLL APROG.DLL in the C:\APROG directory and you
are using the standard settings of the LIBPATH command described above. When
you try to start MYPROG.EXE from C:\ (with the 'C:\APROG\APROG' command) it
will not start because APROG.DLL can not be found by OS/2 in the LIBPATH
search path. If you start APROG.EXE from the C:\APROG directory itself it
will start because of the '.' entry in LIBPATH. So the current directory can
be important for locating DLLs. That's why PMDLL gives you the option to
change its current directory (see below).

Another example. Suppose you have installed the program called APROG.EXE in
the C:\APROG directory and APRG.DLL in the C:\APROG\DLL directory. You
changed the settings of the LIBPATH command to:

  LIBPATH=.;C:\OS2\DLL;C:\OS2\MDOS;C:\;C:\APROG\DLL;

When there is only one instance of APROG.DLL on your system, the program
APROG.EXE will certainly use the DLL in C:\APROG\DLL. However if the program
behaves unexpectedly, there is the possibility that there is another version
of the DLL somewhere in the directories of your LIBPATH. When examining
APROG.EXE in PMDLL you can determine which DLLs are loaded by APROG.EXE and
where they are loaded from.

Besides the DLLs statically linked to an EXE or DLL, a program can also
load a DLL during run time. Because no program can ever predict which DLLs
an executable will load during run time, PMDLL can run the program you want
to investigate in a trace mode. Every time PMDLL detects that the program
loads a DLL, PMDLL will report the event.


Using PMDLL
===========

To run PMDLL from the OS/2 command line, enter PMDLL followed by an optional
argument specifying the name of the executable or DLL to be analyzed.  You
can also create a Program object on your Desktop.

If a program name argument is not supplied on the command line or the object
parameters, PMDLL will display an Open file dialog where you can select the
EXE or DLL file you want to test.

If you created a PMDLL Program Object, you can drop EXE or DLL Program or
File objects on the PMDLL Object.

After an executable or DLL is selected, PMDLL will analyze the selected
executable or DLL and and the DLLs used by the selected executable or DLL.
The menu options control which DLLs are shown and how the information is
shown.

The information can be shown as in a tree view or as one line per item.
Since DLLs can reference other DLLs, the tree view is a natural way to
visualize of the relationship between the DLLs.  The line view is more
compact but does not show the relationships between DLLs.

To see how PMDLL can display results, select the PMDLL.EXE program itself.
The result is maybe a little bit boring because PMDLL only uses a small
number of OS/2 DLLs. To see more DLLs on the display, select System DLL
paths from the Options menu. Press the delete button on the '?:\OS2\DLL'
entry (where ? will be your OS/2 boot drive letter) and press the Use
button. The tree will now be more interesting.

What do these colors mean?

    Black:  Names of DLLs in black are OK (normal situation)
    Gray:   These DLLs are found in one of the System DLL paths you
            specified in the Options menu. These DLLs are considered to
	    be OK and are not recursively scanned further nor test loaded (see
            the Options menu).
    Blue:   These DLLs are OK but are not recursively scanned because they appeared
            earlier in the tree of DLLs and were scanned there. In our
            example PMDLL.EXE is calling amongst others PMGPI.DLL. PMGPI.DLL
            is calling (besides DOSCALL1.DLL) PMMERGE.DLL which in turn calls
            PMGPI.DLL again. PMGPI.DLL was already being scanned so it is
            painted in blue now. To jump to the decomposition of a blue entry
            double click the MB1 (left) mouse button on this entry.
    Red:    This DLL cannot be found or cannot be loaded.


What does the uppercase/lowercase mean?

    Uppercase means the DLL was statically loaded.

    Lowercase means the DLL was dynamically loaded
    (see the Trace option in File menu).

To see more information about an executable or a DLL, click the left (MB1)
mouse button to select the entry. The information window at the bottom of the
main window  will display detailed information about the selected item along
with the directory where the DLL is loaded from.


The menu items
==============

The File menu.

   About     : The About dialog display PMDLL version information and my
               address in case you have comments or suggestions.
   Select    : The Select DLL/executable dialog allows an EXE/DLL
               to be selected for scanning.
   Refresh   : If you have deleted/moved/copied some DLLs in another
               session, you can use Refresh menu option to rebuild the tree.
   Trace     : After the DLL scan completes, you can try tracing
               dynamically loaded DLLs. The Trace dialog will display. Fill
               in any required program parameters and click the Start
               button to start tracing To stop tracing, either click the
               Stop button (the program being traced is killed after a
               confirmation) or exit the program being traced in the normal
               way. If one or more dynamically loaded DLLs have been found,
               click the  Merge  button to add them to the DLL tree window.
               Remember that statically loaded DLLs are shown in uppercase,
               dynamically loaded DLLs in lowercase. The dynamically loaded
               DLLs found, can depend on the way you navigate through the
               program being traced.
   Find DLL  : You can enter the name of a single DLL to see where it would
               be loaded from.
   Print     : Send the information shown on your screen to a printer.
   Exit      : Stop PMDLL.

The Options menu:

   Load System DLLs    : If this menu option is checked, PMDLL will ignore
                         the System DLLs path and all DLLs with be
                         processed identically.
   Current directory   : You can change the current directory of PMDLL. This
                         can have influence on the way DLLs are loaded.
   System DLL paths    : Use this option to enter the directories to be
  			 considered as members of the System DLLs path.
			 'C:\OS2\DLL' (if C: is your OS/2 drive) is a good
                         candidate.  Other possibilities are 'C:\TCPIP\DLL'
			 and 'C:\SQLLIB\DLL'. DLLs found in these
                         directories are not scanned for DLL usage and are
                         displayed in gray.
   Change OS/2 libpath : This option displays the Edit LIBPATH statement
                         dialog. You can change the CONFIG.SYS LIBPATH
                         statement from this dialog. PMDLL will use the
                         modified LIBPATH immediately.  However OS/2 will
                         use the modified LIBPATH only after rebooting.
   Test load DLLs      : When this menu item is checked, PMDLL will try to
                         load each DLL with DosLoadModule. DLLs found in
                         the System DLL paths will not be loaded unless the
                         Load System DLLs menu option is toggled on.
                         Loading a DLL can sometimes cause problems because
                         some DLLs contain startup code which will be
                         executed by OS/2 the moment the DLL is loaded by
                         the system. This piece of code can report a
                         problem back to OS/2 and OS/2 will stop loading
                         your program. With this option checked the
                         creation of the tree will take some more time.
                         PMDLL may even abort because this startup code
                         contains a serious error.  So, be aware.
   Tree view           : This menu option toggles the DLLs display
                         between tree view and line view (just try it).
   Select font         : Change the font used for drawing the tree.
   Resource limits     : Change the maximum size of tables used by PMDLL.


Settings
========

PMDLL saves its settings in the OS2.INI file under the Application name
PM_DllTree. If you suspect these settings may corrupted, use an INI file
editor to delete the suspect keys.  They will be rebuilt the next time you
restart PMDLL.

Known problems/shortcomings
===========================

 - The OpenWatcom build of PMDLL can not test load gcc libc
   DLLs.  The test load fails with error 12, an invalid access failure.
   The libc DLL InitTerm routines assume they are loaded by a gcc based
   application which is not the case when PMDLL is built with
   OpenWatcom.  The gcc build of PMDLL can test load the gcc libc
   DLLs.

 - The Trace DLLs option sometimes traps.

 - Some test loaded DLLs refuse to unload.  The DosFreeModule() call
   fails.  These DLLs will stay loaded until PMDLL terminates.

 - Keyboard accelerators not fully implemented.

 - DLL entry points are not validated.

 - Windows EXE/DLLs are not supported.

About PMDLL
===========

PMDLL was originally written by:

  Arthur van Beek
  avbeek@allshare.nl

He has kindly allowed me to take over maintenance and support of PMDLL.  I'm
sure he would appreciate a Thank You note for his generosity.

Building
========

PMDLL is an OS/2 PM application written in C.  Currently, it can be built
with OpenWatcom 1.x, Visualage C (v3.08 or 3.65) and gcc (3.3.5 or newer).

See the makefiles for build options usage notes.

The OpenWatcom makefile supports the most features.

The gcc and VAC makefiles do not yet support exceptq.

Support
=======

Please address support questions and enhancement requests to:

  Steven H. Levine
  steve53@earthlink.net

I also monitor most of the comp.os.os2.programmer.* newsgroups.

Thanks and enjoy.

$Id: ReadMe.txt,v 1.7 2013/04/03 00:13:54 Steven Exp $
