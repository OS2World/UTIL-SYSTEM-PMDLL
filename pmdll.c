
/***********************************************************************

pmdll.c - Startup and main window
$Id: pmdll.c,v 1.10 2013/04/03 00:14:19 Steven Exp $

Copyright (c) 2001, 2013 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

1999-11-06 AVB Baseline
2001-05-12 SHL #define VERSION
2001-05-22 SHL Release v2.7
2001-05-31 SHL Correct window proc arguments, drop casts
2001-06-02 SHL Drop unused window classes
2001-06-11 SHL AboutWndProc: drop extra code to make keys work
2001-06-11 SHL Release v2.8
2001-06-18 SHL Catch WinLoadDlg failures
2001-06-18 SHL Get rid of HWND casts
2001-06-26 SHL Add Load System DLLs support
2001-06-26 SHL Release v2.9beta2
2001-07-06 SHL Add exceptq support
2001-08-28 SHL Rework error messaging and fatal shutdowns
2001-08-28 SHL Release v2.9beta3
2006-03-15 SHL Release v2.9beta4
2012-01-02 SHL Avoid spurious display rebuild after Load SysDlls toggle
2012-04-22 SHL Rework exceptq support
2012-04-22 SHL Release v2.9
2012-05-13 SHL Release v2.10
2013-03-24 SHL Enhance file open to include all executables
2013-04-02 SHL Release v2.11

***********************************************************************/

#define INCL_WINHOOKS

#if defined(EXCEPTQ)
#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#endif

#define GEN_GLOBALS

#include "pmdll.h"

#include <io.h>

#if defined(EXCEPTQ)
#define INCL_LOADEXCEPTQ
#include "exceptq.h"
#endif

#define PGM_VERSION "2.11"

#define NR_OF_DET_LINES		5
#define FRAME_CX_MIN		400
#define FRAME_CY_MIN		200
#define BORDER_WIDTH		4
#define SHADOW_WIDTH		2

#define PRF_MAX_IMPORTS_DEF	"128"
#define PRF_MAX_DLLS_DEF	"512"
#define PRF_MAX_TREE_DEPTH_DEF	"10"

static USHORT CyCommentbar;
static PFNWP MainFrameProc;

static VOID _DrawDetailLines(HPS hPS, SHORT DetailEntry);
static VOID _DrawDepthBorder(HPS hPS, USHORT Depth, LONG xPos, LONG yPos,
			     LONG cx, LONG cy);
static VOID _AdjustTreeWindowPos(VOID);
static VOID _SetInitialWindowPos(VOID);
static VOID _SaveWindowPos(LONG x, LONG y, LONG cx, LONG cy);

static CHAR MyName[15];

//=== Local functions ===

MRESULT EXPENTRY AboutWndProc (HWND, ULONG, MPARAM, MPARAM);
// MRESULT EXPENTRY DetailsDllWndProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY MainWndProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY MainWndSubProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY ResourcesWndProc (HWND, ULONG, MPARAM, MPARAM);

//=== main ===

VOID main(INT Argc,
	  PCHAR * Argv)
{
#ifdef EXCEPTQ
    EXCEPTIONREGISTRATIONRECORD exRegRec;
#endif
    ULONG ulFrameStyle;
    ULONG DriveMap;
    FILESTATUS3 FileStatus;
    QMSG Msg;
    ULONG CurDisk;
    ULONG PathLen;
    ULONG cbActual;
    ULONG cb;
    APIRET rc;
    CHAR *pStr;

#ifdef EXCEPTQ
    LoadExceptq(&exRegRec, "I", "pmdll v" PGM_VERSION);
#endif

    // Isolate program name
    pStr = strrchr(Argv[0], '\\');
    if (pStr && strlen(pStr) < sizeof(MyName))
    {
	++pStr;
	strcpy(MyName, pStr);
    }
    else
	ShutWithMessage(1, "MyName buffer overflow");

    DosError(FERR_DISABLEHARDERR);		// fixme to be smarter

    rc = DosQueryCtryInfo(sizeof(CountryInfo), &CountryCode, &CountryInfo, &cbActual);
    if (rc)
	ShutWithMessage(rc, "DosQueryCtryInfo failed (%lu)", rc);

    strcpy(ApplTitle, "PM DLL tree view " PGM_VERSION);
    strcpy(FrameTitle, ApplTitle);

    hAB = WinInitialize(0);
    if (hAB == NULLHANDLE)
	ShutWithMessage(1, "WinInitialize failed");

    hMsgQ = WinCreateMsgQueue(hAB, 0);
    if (hMsgQ == NULLHANDLE)
    {
	rc = WinGetLastError(hAB);
	ShutWithMessage(rc, "WinCreateMsgQueue failed (0x%lx)",	rc);
    }

    /*--- check command line parameter ---*/

    if (Argc == 2)
    {
	strcpy(CurrentFileName, Argv[1]);
	strupr(CurrentFileName);

	if (strstr(CurrentFileName, ".EXE") == NULL &&
	    strstr(CurrentFileName, ".DLL") == NULL)
	{
	    // Extension missing or odd - warn
	    AskShutWithMessage(1, "%s may not be an .EXE or .DLL file.  Click OK to open anyway", Argv[1]);
	}

	if (strchr(CurrentFileName, '*') != NULL ||
		 strchr(CurrentFileName, '?') != NULL)
	{
	    // Name contains wildcards
	    AskShutWithMessage(1, "%s contains wildcards", Argv[1]);

	    // 2013-03-24 SHL FIXME to pass to file open dialog
	    *CurrentFileName = 0;	// Forget
	}
	else if (strchr(CurrentFileName, '\\') == NULL &&
		 strchr(CurrentFileName, ':') == NULL)
	{
	    // No path specified, check current directory
	    if (access(CurrentFileName, 0))
	    {
		// Check PATH
		if (DosSearchPath(SEARCH_ENVIRONMENT | SEARCH_IGNORENETERRS,
				  "PATH",
				  Argv[1],
				  (PUCHAR)CurrentFileName,
				  CCHMAXPATH))
		{
		    AskShutWithMessage(1, "%s not found", Argv[1]);

		    *CurrentFileName = 0;
		}
	    }
	    else
	    {
		// File exists - build full pathname using current drive and directory
		DosQCurDisk(&CurDisk, &DriveMap);
		CurrentFileName[0] = (CHAR) (CurDisk + ('A' - 1));
		CurrentFileName[1] = ':';
		CurrentFileName[2] = '\\';
		PathLen = CCHMAXPATH;
		DosQCurDir(CurDisk, (PUCHAR)CurrentFileName + 3, &PathLen);
		if (CurrentFileName[strlen(CurrentFileName) - 1] != '\\')
		    strcat(CurrentFileName, "\\");
		strcat(CurrentFileName, Argv[1]);

		strupr(CurrentFileName);
	    }
	}
	else
	{
	    // Explicit path
	    rc = DosQueryPathInfo(CurrentFileName, FIL_STANDARD,
				  &FileStatus, sizeof(FILESTATUS3));

	    if (rc)
	    {
		switch (rc)
		{
		case ERROR_FILE_NOT_FOUND:
		    AskShutWithMessage(rc, "%s not found", CurrentFileName);
		    break;
		case ERROR_PATH_NOT_FOUND:
		    AskShutWithMessage(rc, "%s path not found", CurrentFileName);
		    break;
		case ERROR_INVALID_NAME:
		case ERROR_FILENAME_EXCED_RANGE:
		    AskShutWithMessage(rc, "%s is an invalid path string", CurrentFileName);
		    break;
		case ERROR_INVALID_DRIVE:
		    AskShutWithMessage(rc, "%s contains an invalid drive in path", CurrentFileName);
		    break;
		default:
		    AskShutWithMessage(rc, "%s is an invalid file/path name (%lu)", rc, CurrentFileName);
		}

		*CurrentFileName = 0;
	    }
	    else
	    {
		if ((FileStatus.attrFile & FILE_DIRECTORY) == FILE_DIRECTORY)
		{
		    AskShutWithMessage(ERROR_FILE_NOT_FOUND,
				       "%s is a directory name", CurrentFileName);

		    *CurrentFileName = 0;
		}
	    }
	}

	strupr(CurrentFileName);
    }

    /*--- get profile data - initialize as needed ---*/

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_MAX_IMPORTS,
			  "NONE", ProfileData, 80);

    if (strcmp(ProfileData, "NONE"))
	PrfMaxNrOfImports = (USHORT) atoi(ProfileData);
    else
    {
	PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_MAX_IMPORTS,
			      PRF_MAX_IMPORTS_DEF);
	PrfMaxNrOfImports = (USHORT) atoi(PRF_MAX_IMPORTS_DEF);
    }

    PrfMaxNrOfImports = max((USHORT) 5, PrfMaxNrOfImports);

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_MAX_DLLS,
			  "NONE", ProfileData, 80);

    if (strcmp(ProfileData, "NONE"))
	PrfMaxNrOfDlls = (USHORT) atoi(ProfileData);
    else
    {
	PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_MAX_DLLS,
			      PRF_MAX_DLLS_DEF);
	PrfMaxNrOfDlls = (USHORT) atoi(PRF_MAX_DLLS_DEF);
    }

    PrfMaxNrOfDlls = max((USHORT) 5, PrfMaxNrOfDlls);
    MaxNrOfDlls = PrfMaxNrOfDlls;

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_MAX_TREE_DEPTH,
			  "NONE", ProfileData, 80);

    if (strcmp(ProfileData, "NONE"))
	PrfMaxTreeDepth = (USHORT) atoi(ProfileData);
    else
    {
	PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_MAX_TREE_DEPTH,
			      PRF_MAX_TREE_DEPTH_DEF);
	PrfMaxTreeDepth = (USHORT) atoi(PRF_MAX_TREE_DEPTH_DEF);
    }

    PrfMaxTreeDepth = max((USHORT) 5, PrfMaxTreeDepth);
    MaxTreeDepth = PrfMaxTreeDepth;

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_FONT_KEY,
			  "NONE", ProfileData, 80);

    if (strcmp(ProfileData, "NONE"))
	PrfFontNr = (USHORT) atoi(ProfileData);
    else
    {
	PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_FONT_KEY,
			      "1");
	PrfFontNr = 1;
    }

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_USE_SYSDLLS_KEY,
			  "NONE", ProfileData, 80);

    if (strcmp(ProfileData, "NONE"))
	fLoadSysDlls = (USHORT)atoi(ProfileData);
    else
    {
	PrfWriteProfileString(HINI_USERPROFILE,
			      PRF_APPL_NAME,
			      PRF_USE_SYSDLLS_KEY,
			      "1");
	fLoadSysDlls = 0;
    }

    memset(PrfSysDllPath, 0x00, sizeof(PrfSysDllPath));
    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_SYSDLL_KEY,
			  "NONE", PrfSysDllPath, sizeof(PrfSysDllPath));

    if (strcmp(PrfSysDllPath, "NONE") == 0)
    {
	strcpy(PrfSysDllPath, "C:\\OS2\\DLL;");
	PrfSysDllPath[0] = GetBootDrive();
	PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_SYSDLL_KEY,
			      PrfSysDllPath);
    }

    strcpy(SysDllPath, PrfSysDllPath);

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_PAGELEN_KEY,
			  "NONE", ProfileData, 80);

    if (strcmp(ProfileData, "NONE"))
	PrfPageLen = (USHORT) atoi(ProfileData);
    else
    {
	PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_PAGELEN_KEY,
			      "60");
	PrfPageLen = 60;
    }

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_PRINTER_KEY,
			  "NOPRINTER", ProfileData, 80);

    if (strcmp(ProfileData, "NOPRINTER"))
	strcpy(PrfPrinter, ProfileData);
    else
    {
	PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_PRINTER_KEY,
			      "LPT1");
	strcpy(PrfPrinter, "LPT1");
    }

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_PAGEWIDTH_KEY,
			  "NONE", ProfileData, 80);

    if (strcmp(ProfileData, "NONE"))
	PrfPageWidth = (USHORT) atoi(ProfileData);
    else
    {
	PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_PAGEWIDTH_KEY,
			      "80");
	PrfPageWidth = 80;
    }

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_TEST_LOAD,
			  "1", ProfileData, 80);

    TestLoadModule = atoi(ProfileData);

    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_TREE_VIEW,
			  "1", ProfileData, 80);

    TreeViewMode = atoi(ProfileData);

    /*--- allocate memory ---*/

    cb = MaxNrOfDlls * sizeof(SHOW_TABLE_DATA);
    rc = DosAllocMem((PPVOID) &pShowTableData,
		     cb,
		     PAG_READ | PAG_WRITE | PAG_COMMIT);

    if (rc)
	ShutWithMessage(rc, "Can not allocate %lu bytes (%lu)", cb, rc);

    LoadFonts();

    if (WinRegisterClass(hAB, "WC_MAIN", MainWndProc,
			 CS_SIZEREDRAW, 0) == FALSE)
    {
	rc = WinGetLastError(hAB);
	ShutWithMessage(rc, "Can not register class WC_MAIN (0x%lx)", rc);
    }

    ulFrameStyle = FCF_ACCELTABLE |
		   FCF_TITLEBAR |
		   FCF_MINBUTTON |
		   FCF_MAXBUTTON |
		   FCF_SIZEBORDER |
		   FCF_MENU |
		   FCF_ICON |
		   FCF_SHELLPOSITION |
		   FCF_SYSMENU |
		   FCF_TASKLIST;

    hWndFrame = WinCreateStdWindow(HWND_DESKTOP,
				   0L,
				   &ulFrameStyle,
				   "WC_MAIN",
				   FrameTitle,
				   0L,
				   NULLHANDLE,
				   FID_PMDLL,
				   &hWndClient);

    if (hWndFrame == NULLHANDLE)
    {
	rc = WinGetLastError(hAB);
	ShutWithMessage(rc, "Can not create framewindow (0x%lx)", rc);
    }

    if (TestLoadModule)
	CheckMenuItem(hWndFrame, MIT_TEST_LOAD);
    else
	UncheckMenuItem(hWndFrame, MIT_TEST_LOAD);

    if (TreeViewMode)
	CheckMenuItem(hWndFrame, MIT_TREE_VIEW);
    else
	UncheckMenuItem(hWndFrame, MIT_TREE_VIEW);

    if (fLoadSysDlls)
	CheckMenuItem(hWndFrame, MIT_LOAD_SYSTEM_DLLS);
    else
	UncheckMenuItem(hWndFrame, MIT_LOAD_SYSTEM_DLLS);

    MainFrameProc = WinSubclassWindow(hWndFrame, MainWndSubProc);

    _SetInitialWindowPos();

    if (!WinRegisterClass(hAB, "WC_TREE", TreeWndProc, 0L, 0))
    {
	rc = WinGetLastError(hAB);
	ShutWithMessage(rc, "Can not register WC_TREE class (0x%x)", rc);
    }

    ulFrameStyle = FCF_BORDER |
		   FCF_NOBYTEALIGN |
		   FCF_VERTSCROLL |
		   FCF_HORZSCROLL;

    hWndFrameTree = WinCreateStdWindow(hWndClient,
				       WS_VISIBLE,
				       &ulFrameStyle,
				       "WC_TREE",
				       "",
				       0L,
				       NULLHANDLE,
				       0,
				       &hWndClientTree);

    if (hWndFrameTree == NULLHANDLE)
    {
	rc = WinGetLastError(hAB);
	ShutWithMessage(rc, "Can not create tree frame window (0x%lx)",	rc);
    }

    TreeFrameProc = WinSubclassWindow(hWndFrameTree, TreeWndSubProc);

    _AdjustTreeWindowPos();

    hSwitch = WinQuerySwitchHandle(hWndFrame, 0);

#if 0 // Enable to test EXCEPTQ
    *(PCHAR)0 = 0;
#endif

    while (WinGetMsg(hAB, &Msg, NULLHANDLE, 0, 0))
	WinDispatchMsg(hAB, &Msg);

#   if defined(EXCEPTQ)
    UninstallExceptq(&exRegRec);
#   endif
    Shut(0);

} // main

MRESULT EXPENTRY AboutWndProc(HWND hDlg,
			      ULONG Msg,
			      MPARAM mp1,
			      MPARAM mp2)
{
    switch (Msg)
    {
    case WM_INITDLG:
	WinSetDlgItemText(hDlg, TID_PROGRAM_NAME, ApplTitle);
	CenterDialog(hWndFrame, hDlg);
	break;
    }

    return WinDefDlgProc(hDlg, Msg, mp1, mp2);

} // AboutWndProc

static VOID _AdjustTreeWindowPos(VOID)
{
    LONG xPos, yPos, cx, cy;
    SWP Swp;

    WinQueryWindowPos(hWndClient, &Swp);

    xPos = 0;
    yPos = CyCommentbar;
    cx = Swp.cx;
    cy = Swp.cy - (SHORT) CyCommentbar;

    WinSetWindowPos(hWndFrameTree, NULLHANDLE, xPos, yPos, cx, cy,
		    SWP_SIZE | SWP_MOVE);

} // _AdjustTreeWindowPos

//=== AskShutWithMessage: Show message in app modal box and die ===

VOID AskShutWithMessage(APIRET rc, CHAR *pszFmt, ...)
{
    va_list pVA;

    va_start(pVA, pszFmt);

    if (hMsgQ != NULLHANDLE)
    {
	rc = VShwMsgBox(HWND_DESKTOP,
			MB_OKCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL,
			pszFmt,
			pVA);

	if (rc == MBID_CANCEL)
	    Shut(rc);
    }
    else
    {
	// Can't ask if not initialized
	vfprintf(stderr, pszFmt, pVA);
	fprintf(stderr, "\n");
	Shut(rc);
    }

} // AskShutWithMessage

static VOID _DrawDetailLines(HPS hPS,
			     SHORT Entry)
{
    POINTL Ptl;
    CHAR DisplayStr[CCHMAXPATH];
    ULONG DriveMap, PathLen, CurDisk;
    RECTL rClient;
    USHORT i;

    WinQueryWindowRect(hWndClient, &rClient);

    _DrawDepthBorder(hPS, SHADOW_WIDTH, 0L, 0L, rClient.xRight,
		     CyCommentbar - 1);	// + 2 * SHADOW_WIDTH);

    Ptl.x = BORDER_WIDTH;
    Ptl.y = CyCommentbar - FontHeight + FontOffset - BORDER_WIDTH;

    GpiSetColor(hPS, CLR_BLACK);

    /*--- line 1 ---*/

    strcpy(DisplayStr, "Current directory: ");
    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);

    Ptl.x = strlen(DisplayStr) * FontWidth;

    DosQueryCurrentDisk(&CurDisk, &DriveMap);
    DisplayStr[0] = (UCHAR) (CurDisk + 64);
    DisplayStr[1] = ':';
    DisplayStr[2] = '\\';
    PathLen = CCHMAXPATH - 3;
    DosQCurDir(CurDisk, (PUCHAR)&DisplayStr[3], &PathLen);
    strupr(DisplayStr);

    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);

    Ptl.y -= FontHeight;
    Ptl.x = BORDER_WIDTH;

    /*--- line 2 ---*/

    if (Entry < 0)
    {
	strcpy(DisplayStr, "File path        : ");
	GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	Ptl.x = strlen(DisplayStr) * FontWidth;
	GpiCharStringAt(hPS, &Ptl, 1L, "-");
    }
    else
    {
	if (pShowTableData[Entry].PathFound)
	{
	    strcpy(DisplayStr, "File path        : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);

	    Ptl.x = strlen(DisplayStr) * FontWidth;
	    GpiCharStringAt(hPS, &Ptl,
			    strlen(pShowTableData[Entry].Path),
			    pShowTableData[Entry].Path);
	}
	else
	{
	    GpiSetColor(hPS, CLR_RED);

	    strcpy(DisplayStr, "File path        : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	    Ptl.x = strlen(DisplayStr) * FontWidth;

	    GpiCharStringAt(hPS, &Ptl, strlen(pShowTableData[Entry].Path),
			    pShowTableData[Entry].Path);
	}
    }

    Ptl.y -= FontHeight;
    Ptl.x = BORDER_WIDTH;
    GpiSetColor(hPS, CLR_BLACK);

    /*--- line 3 ---*/

    if (Entry < 0 ||
	    !pShowTableData[Entry].PathFound)
    {
	strcpy(DisplayStr, "DLL loadable     : ");
	GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	Ptl.x = strlen(DisplayStr) * FontWidth;
	GpiCharStringAt(hPS, &Ptl, 1L, "-");
    }
    else
    {
	if (strstr(pShowTableData[Entry].Name, ".EXE"))
	{
	    strcpy(DisplayStr, "EXE startable    : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	    Ptl.x = strlen(DisplayStr) * FontWidth;

	    for (i = 1; i < cShowTableData; ++i)
	    {
		if (!pShowTableData[i].PathFound ||
			!pShowTableData[i].Loadable)
		    break;
	    }

	    if (i < cShowTableData)
		strcpy(DisplayStr, "No");
	    else
	    {
		if (TestLoadModuleUsed)
		    strcpy(DisplayStr, "Yes");
		else
		    strcpy(DisplayStr, "Yes, but test load DLL option was disabled");
	    }

	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	}
	else if (Entry == 0 && pShowTableData[0].Loadable)
	{
	    strcpy(DisplayStr, "DLL loadable     : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	    Ptl.x = strlen(DisplayStr) * FontWidth;

	    for (i = 1; i < cShowTableData; ++i)
	    {
		if (!pShowTableData[i].PathFound ||
			!pShowTableData[i].Loadable)
		    break;
	    }

	    if (i < cShowTableData)
		strcpy(DisplayStr, "No, an error occured deeper down the tree");
	    else
	    {
		if (TestLoadModuleUsed)
		    strcpy(DisplayStr, "Yes");
		else
		    strcpy(DisplayStr, "Yes, but test load DLL option was disabled");
	    }

	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	}
	else if (pShowTableData[Entry].Dynamic)
	{
	    strcpy(DisplayStr, "DLL loadable     : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	    Ptl.x = strlen(DisplayStr) * FontWidth;
	    strcpy(DisplayStr, "Yes, this DLL was dynamically loaded by the executable");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	}
	else if (pShowTableData[Entry].SystemDll)
	{
	    strcpy(DisplayStr, "DLL loadable     : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	    Ptl.x = strlen(DisplayStr) * FontWidth;
	    strcpy(DisplayStr, "System DLLs are not test loaded");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	}
	else if (!TestLoadModuleUsed)
	{
	    strcpy(DisplayStr, "DLL loadable     : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	    Ptl.x = strlen(DisplayStr) * FontWidth;
	    strcpy(DisplayStr, "Not tested, test load DLL option is disabled");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	}
	else if (pShowTableData[Entry].Loadable)
	{
	    strcpy(DisplayStr, "DLL loadable     : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	    Ptl.x = strlen(DisplayStr) * FontWidth;

	    if (pShowTableData[Entry].FreeError)
	    {
		sprintf(DisplayStr, "Yes, however error %lu freeing module after test load",
			pShowTableData[Entry].FreeError);
	    }
	    else
		strcpy(DisplayStr, "Yes");

	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	}
	else
	{
	    GpiSetColor(hPS, CLR_RED);

	    strcpy(DisplayStr, "DLL loadable     : ");
	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	    Ptl.x = strlen(DisplayStr) * FontWidth;

	    if (pShowTableData[Entry].LoadError == ERROR_INIT_ROUTINE_FAILED)
		strcpy(DisplayStr, "DLL initialization routine failed");
	    else
	    {
		sprintf(DisplayStr, "DLL could not be loaded, error %lu, reason is '%s'",
			pShowTableData[Entry].LoadError, pShowTableData[Entry].LoadErrorCause);
	    }

	    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
	}
    }

    Ptl.y -= FontHeight;
    Ptl.x = BORDER_WIDTH;
    GpiSetColor(hPS, CLR_BLACK);

    /*--- line 4 ---*/

    strcpy(DisplayStr, "File size        : ");
    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);

    Ptl.x = strlen(DisplayStr) * FontWidth;

    if (Entry >= 0)
    {
	if (pShowTableData[Entry].PathFound)
	{
	    CreateInterpStr(pShowTableData[Entry].FileSize, DisplayStr);
	    strcat(DisplayStr, " bytes");
	}
	else
	    strcpy(DisplayStr, "?");

	GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
    }
    else
	GpiCharStringAt(hPS, &Ptl, 1L, "-");

    Ptl.y -= FontHeight;
    Ptl.x = BORDER_WIDTH;

    /*--- line 5 ---*/

    strcpy(DisplayStr, "File date/time   : ");
    GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);

    Ptl.x = strlen(DisplayStr) * FontWidth;

    if (Entry >= 0)
    {
	if (pShowTableData[Entry].PathFound)
	{
	    FDateTimeToString(DisplayStr,
			      &pShowTableData[Entry].fdateLastWrite,
			      &pShowTableData[Entry].ftimeLastWrite);

	}
	else
	    strcpy(DisplayStr, "?");

	GpiCharStringAt(hPS, &Ptl, strlen(DisplayStr), DisplayStr);
    }
    else
	GpiCharStringAt(hPS, &Ptl, 1L, "-");

    Ptl.y -= FontHeight;
    Ptl.x = BORDER_WIDTH;

} // _DrawDetailLines

static VOID _DrawDepthBorder(HPS hPS,
			     USHORT Depth,
			     LONG xPos,
			     LONG yPos,
			     LONG cx,
			     LONG cy)
{
    POINTL EndPoint;
    POINTL StartPoint;
    USHORT i;

    for (i = 0; i < Depth; i++)
    {
	StartPoint.x = xPos + i;
	StartPoint.y = yPos + i;
	EndPoint.x = xPos + i;
	EndPoint.y = yPos + cy - 1 - i;
	GpiMove(hPS, &StartPoint);
	GpiSetColor(hPS, CLR_DARKGRAY);
	GpiLine(hPS, &EndPoint);

	EndPoint.x = xPos + cx - 1 - i;
	GpiLine(hPS, &EndPoint);

	GpiSetColor(hPS, CLR_WHITE);
	EndPoint.y--;
	GpiMove(hPS, &EndPoint);
	EndPoint.y = yPos + i;
	GpiLine(hPS, &EndPoint);

	EndPoint.x = xPos + 1 + i;
	GpiLine(hPS, &EndPoint);
    }
} // _DrawDepthBorder

MRESULT EXPENTRY MainWndProc(HWND hWnd,
			     ULONG Msg,
			     MPARAM mp1,
			     MPARAM mp2)
{
    static HPS hPSInit;
    static SHORT DetailEntry;
    HPS hPS;
    RECTL rClient;
    USHORT FontEntryOld;
    APIRET rc;
    SHORT Entry;
    SWP Swp;
    USHORT i;
    USHORT cmd;
    USHORT cmdSrc;
    HWND hWndT;
    SWP swp;

    switch (Msg)
    {
    case WM_CREATE:
	hPSInit = NULLHANDLE;
	DetailEntry = -1;

	rc = ChangeFont(hWnd, &hPSInit, 850);
	// Just warn
	if (rc)
	    ShwOKDeskMsgBox("Can not change font: %lu, 0x%lx",
			    rc,
			    WinGetLastError(hAB));

	CyCommentbar = (USHORT) (NR_OF_DET_LINES * FontHeight +
				 2 * BORDER_WIDTH);

	if (*CurrentFileName == 0)
	    WinPostMsg(hWnd, WM_COMMAND, MPFROMSHORT(MIT_FILE_OPEN), 0L);
	else
	    WinPostMsg(hWnd, WUM_START_REBUILD, 0L, 0L);
	break;

    case WUM_START_REBUILD:
	DetailEntry = -1;
	WinQueryWindowRect(hWnd, &rClient);
	rClient.yTop = CyCommentbar;
	WinInvalidateRect(hWnd, &rClient, FALSE);

	StartBuildDllTree(CurrentFileName);
	break;

    case WM_SIZE:
	_AdjustTreeWindowPos();

	WinQueryWindowPos(hWndFrame, &Swp);

	if ((Swp.fl & SWP_MINIMIZE) == SWP_MINIMIZE ||
		(Swp.fl & SWP_MAXIMIZE) == SWP_MAXIMIZE)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	_SaveWindowPos(Swp.x, Swp.y, Swp.cx, Swp.cy);
	break;

    case WM_INITMENU:
	if (!FileSelected)
	{
	    DisableMenuItem(hWndFrame, MIT_FILE_REFRESH);
	    DisableMenuItem(hWndFrame, MIT_PRINT);
	    DisableMenuItem(hWndFrame, MIT_FILE_TRACE);
	}
	else
	{
	    EnableMenuItem(hWndFrame, MIT_FILE_REFRESH);
	    EnableMenuItem(hWndFrame, MIT_PRINT);
	    if (strstr(pShowTableData[0].Path, ".EXE") == NULL)
		DisableMenuItem(hWndFrame, MIT_FILE_TRACE);
	    else
	    {
		for (i = 1; i < cShowTableData; ++i)
		{
		    if (!pShowTableData[i].PathFound ||
			    !pShowTableData[i].Loadable)
			break;
		}

		if (i < cShowTableData)
		    DisableMenuItem(hWndFrame, MIT_FILE_TRACE);
		else
		    EnableMenuItem(hWndFrame, MIT_FILE_TRACE);
	    }
	}
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case MIT_ABOUT:
	    WinDlgBox(HWND_DESKTOP, hWnd, AboutWndProc,
		      NULLHANDLE, DID_ABOUT, NULL);
	    break;

	case MIT_FILE_OPEN:
	    rc = WinDlgBox(HWND_DESKTOP, hWnd, SelectFileWndProc,
			   NULLHANDLE, DID_SELECT_FILE, NULL);

	    if (rc == FALSE)
		break;

	    WinPostMsg(hWnd, WUM_START_REBUILD, 0L, 0L);
	    break;

	case MIT_FILE_REFRESH:
	    WinPostMsg(hWnd, WUM_START_REBUILD, 0L, 0L);
	    break;

	// case MIT_FILE_DETAIL:
	//           WinDlgBox (HWND_DESKTOP, hWnd, DetailsDllWndProc,
	//                     NULLHANDLE, DID_DETAILS_DLL, &DetailEntry);
	//           break;

	case MIT_FILE_TRACE:
	    if (stricmp(MyName, pShowTableData[0].Name) == 0)
	    {
		ShwOKDeskMsgBox("Cannot trace myself");	// Just warn
		break;
	    }

	    if (hWndTrace == NULLHANDLE)
	    {
		hWndTrace = WinLoadDlg(HWND_DESKTOP, hWnd,
				       TraceDllWndProc,
				       0, DID_TRACE_DLL, NULL);

		if (hWndTrace == NULLHANDLE)
		{
		    ShwOKDeskMsgBox("Can not load Trace dialog");
		}
		else
		{
		    WinShowWindow(hWndFrame, FALSE);
		    SetSwitchWnd(hWndTrace);
		}
	    }
	    break;

	case MIT_FIND_DLL:
	    rc = WinDlgBox(HWND_DESKTOP, hWnd, FindDllWndProc,
			   NULLHANDLE, DID_FIND_DLL, NULL);

	    if (rc == TRUE)
		WinPostMsg(hWnd, WUM_START_REBUILD, 0L, 0L);
	    break;

	case MIT_PRINT:
	    WinDlgBox(HWND_DESKTOP, hWnd, PrintWndProc,
		      NULLHANDLE, DID_PRINT, NULL);
	    break;

	case MIT_CUR_DIR:
	    WinDlgBox(HWND_DESKTOP, hWnd, ChangeCurDirWndProc,
		      NULLHANDLE, DID_CHANGE_CUR_DIR, NULL);
	    break;

	case MIT_SYSTEM_DLLS:
	    WinDlgBox(HWND_DESKTOP, hWnd, SysDllWndProc,
		      NULLHANDLE, DID_EDIT_SYSPATH, NULL);
	    break;

	case MIT_LIBPATH:
	    WinDlgBox(HWND_DESKTOP, hWnd, LibpathWndProc,
		      NULLHANDLE, DID_EDIT_LIBPATH, NULL);
	    break;

	case MIT_FONT:
	    FontEntryOld = FontEntry;

	    rc = WinDlgBox(HWND_DESKTOP, hWnd, SelectFontWndProc,
			   NULLHANDLE, DID_SELECT_FONT, NULL);

	    if (rc == TRUE)
	    {
		rc = ChangeFont(hWnd, &hPSInit, 850);
		if (rc)
		{
		    FontEntry = FontEntryOld;

		    ShwOKDeskMsgBox("Error changing font: %lu", rc);
		    break;
		}

		CyCommentbar = (USHORT) (NR_OF_DET_LINES * FontHeight +
					 2 * BORDER_WIDTH);

		WinQueryWindowRect(hWnd, &rClient);
		rClient.yTop = CyCommentbar;
		WinInvalidateRect(hWnd, &rClient, FALSE);

		_AdjustTreeWindowPos();

		WinPostMsg(hWndClientTree, WUM_FONT_CHANGE, 0L, 0L);

		itoa(FontEntry, ProfileData, 10);
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_FONT_KEY, ProfileData);
	    }
	    break;

	case MIT_TEST_LOAD:
	    if (TestLoadModule)
	    {
		UncheckMenuItem(hWndFrame, MIT_TEST_LOAD);
		TestLoadModule = FALSE;
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_TEST_LOAD, "0");
	    }
	    else
	    {
		CheckMenuItem(hWndFrame, MIT_TEST_LOAD);
		TestLoadModule = TRUE;
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_TEST_LOAD, "1");
	    }

	    if (FileSelected)
	    {
		rc = ShwYesNoMsgBox(hWnd, "Refresh tree window?");

		if (rc == MBID_YES)
		    WinPostMsg(hWnd, WUM_START_REBUILD, 0L, 0L);
	    }
	    break;

	case MIT_TREE_VIEW:
	    if (TreeViewMode)
	    {
		UncheckMenuItem(hWndFrame, MIT_TREE_VIEW);
		TreeViewMode = FALSE;
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_TREE_VIEW, "0");
	    }
	    else
	    {
		CheckMenuItem(hWndFrame, MIT_TREE_VIEW);
		TreeViewMode = TRUE;
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_TREE_VIEW, "1");
	    }

	    WinPostMsg(hWndClientTree, WUM_CHANGE_MODE, 0L, 0L);
	    break;

	case MIT_RESOURCES:
	    WinDlgBox(HWND_DESKTOP, hWnd, ResourcesWndProc,
		      NULLHANDLE, DID_RESOURCE, NULL);
	    break;

	case MIT_LOAD_SYSTEM_DLLS:
	    if (fLoadSysDlls)
	    {
		UncheckMenuItem(hWndFrame, MIT_LOAD_SYSTEM_DLLS);
		fLoadSysDlls = FALSE;
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_USE_SYSDLLS_KEY, "0");
	    }
	    else
	    {
		CheckMenuItem(hWndFrame, MIT_LOAD_SYSTEM_DLLS);
		fLoadSysDlls = TRUE;
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_USE_SYSDLLS_KEY, "1");
	    }

	    if (FileSelected)
	      WinPostMsg(hWnd, WUM_START_REBUILD, 0L, 0L);
	    break;

	case MIT_EXIT:
	    WinSendMsg(hWndFrame, WM_CLOSE, 0L, 0L);
	    break;
	}
	break;

    case WM_SYSCOMMAND:
	// If the mouse has been captured, ignore the system command.
	if (WinQueryCapture (HWND_DESKTOP) != NULLHANDLE)
	    return MPVOID;

	cmd = LOUSHORT(mp1);
	cmdSrc = LOUSHORT(mp2);

	switch (cmd)
	{
	case SC_RESTORE:
	case SC_MAXIMIZE:
	    /*
	     * Get the restore position that SetMultWindowPos will use.
	     */
	    WinQueryWindowPos(hWnd, &swp);

	    switch(cmd)
	    {
	    case SC_RESTORE:
		swp.fl = SWP_RESTORE;
		break;

	    case SC_MAXIMIZE:
		swp.fl = SWP_MAXIMIZE;
		break;
	    }

	    /*
	     * If the control key is down, don't activate, or deactivate.
	     * Don't do this if this came from an accelerator.
	     */
	    if (cmdSrc != CMDSRC_ACCELERATOR)
	    {
		if (WinGetKeyState(HWND_DESKTOP, VK_CTRL) & 0x8000)
		    swp.fl &= ~(SWP_DEACTIVATE | SWP_ACTIVATE);
	    }

	    WinSetMultWindowPos(NULLHANDLE, (PSWP)&swp, 1);
	    break;

	case SC_MOVE:
	case SC_SIZE:
	case SC_CLOSE:
	    switch (cmd)
	    {
	    case SC_SIZE:
		/*
		 * Start keyboard sizing interface by sending a WM_TRACKFRAME
		 * message back to ourselves (the frame window).
		 */
		WinSendMsg(hWnd, WM_TRACKFRAME,
			MPFROMSHORT(TF_SETPOINTERPOS), MPVOID );
		break;

	    case SC_MOVE:
		/*
		 * Start keyboard moving interface by sending a WM_TRACKFRAME
		 * message back to ourselves (the frame window).
		 */
		WinSendMsg (hWnd, WM_TRACKFRAME,
			MPFROMSHORT(TF_SETPOINTERPOS | TF_MOVE), MPVOID );
		break;

	    case SC_CLOSE:
		if ((hWndT = WinWindowFromID (hWnd, FID_CLIENT)) == NULLHANDLE)
		    hWndT = hWnd;
		WinPostMsg (hWndT, WM_CLOSE, MPVOID, MPVOID);
		break;
	    } // switch cmd
	    break;

	} // switch cmd
	break;

    case WM_PAINT:
	hPS = WinBeginPaint(hWnd, hPSInit, 0);
	WinQueryWindowRect(hWnd, &rClient);
	WinFillRect(hPS, &rClient, CLR_PALEGRAY);

	_DrawDetailLines(hPS, DetailEntry);

	WinEndPaint(hPS);
	break;

    case WUM_SHOW_DETAILS:
	if (!FileSelected)
	    DetailEntry = -1;
	else
	{
	    Entry = SHORT1FROMMP(mp1);

	    if (Entry >= (SHORT) cShowTableData)
		DetailEntry = -1;
	    else
		DetailEntry = Entry;
	}

	WinQueryWindowRect(hWnd, &rClient);
	rClient.yTop = CyCommentbar;
	WinInvalidateRect(hWnd, &rClient, FALSE);
	break;

    case WM_CLOSE:
#	if 0
	rc = ShwYesNoMsgBox(hWndFrame, "Exit program?");

	if (rc == MBID_NO)
	    break;
#	endif

	if (CsChanged)
	{
	    WinShowWindow(hWndFrame, FALSE);

	    WinMessageBox(HWND_DESKTOP,
			  HWND_DESKTOP,
			  "The LIBPATH statement in your config.sys "
			  "has been changed, in order to make the changes "
			  "active please reboot your PC",
			  ApplTitle,
			  0,
			  MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
	}

	return (WinDefWindowProc(hWnd, Msg, mp1, mp2));
	break;

    case DM_DRAGOVER:
	WinQueryWindowPos(hWndFrame, &Swp);

	if ((Swp.fl & SWP_MINIMIZE) == SWP_MINIMIZE)
	    return (ProcessDragOver(mp1));
	else
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));
	break;

    case DM_DROP:
	WinSetWindowPos(hWndFrame, NULLHANDLE, 0, 0, 0, 0, SWP_RESTORE);
	return (ProcessDragDrop(mp1));
	break;

    default:
	return (WinDefWindowProc(hWnd, Msg, mp1, mp2));
	break;
    }

    return WinDefWindowProc(hWnd, Msg, mp1, mp2);

} // MainWndProc

MRESULT EXPENTRY MainWndSubProc(HWND hWnd,
				ULONG Msg,
				MPARAM mp1,
				MPARAM mp2)
{
    PTRACKINFO pTrack;
    POINTL PtlMin;

    switch (Msg)
    {
    case WM_QUERYTRACKINFO:
	(*MainFrameProc) (hWnd, Msg, mp1, mp2);
	pTrack = (PTRACKINFO) mp2;
	if (((pTrack -> fs & TF_MOVE) != TF_MOVE) &&
		((pTrack -> fs & TF_LEFT) ||
		 (pTrack -> fs & TF_TOP) ||
		 (pTrack -> fs & TF_RIGHT) ||
		 (pTrack -> fs & TF_BOTTOM) ||
		 (pTrack -> fs & TF_SETPOINTERPOS)))
	{
	    PtlMin.x = FRAME_CX_MIN;
	    PtlMin.y = FRAME_CY_MIN;
	    pTrack -> ptlMinTrackSize = PtlMin;
	}

	return ((MRESULT) TRUE);
	break;

    default:
	return ((*MainFrameProc) (hWnd, Msg, mp1, mp2));
	break;
    }

    return NULL;

} // MainWndSubProc

MRESULT EXPENTRY ResourcesWndProc(HWND hDlg,
				  ULONG Msg,
				  MPARAM mp1,
				  MPARAM mp2)
{
    CHAR TmpStr[21];
    LONG TmpVal;

    switch (Msg)
    {
    case WM_INITDLG:
	WinSendDlgItemMsg(hDlg, EID_RES_MAX_IMPORTS, SPBM_SETLIMITS,
			  (MPARAM) 5000, (MPARAM) 5);
	WinSendDlgItemMsg(hDlg, EID_RES_MAX_DLLS, SPBM_SETLIMITS,
			  (MPARAM) 5000, (MPARAM) 5);
	WinSendDlgItemMsg(hDlg, EID_RES_MAX_DEPTH, SPBM_SETLIMITS,
			  (MPARAM) 99, (MPARAM) 5);

	WinSendDlgItemMsg(hDlg, EID_RES_MAX_IMPORTS, SPBM_SETCURRENTVALUE,
			  (MPARAM) PrfMaxNrOfImports, 0L);

	WinSendDlgItemMsg(hDlg, EID_RES_MAX_DLLS, SPBM_SETCURRENTVALUE,
			  (MPARAM) PrfMaxNrOfDlls, 0L);

	WinSendDlgItemMsg(hDlg, EID_RES_MAX_DEPTH, SPBM_SETCURRENTVALUE,
			  (MPARAM) PrfMaxTreeDepth, 0L);

	CenterDialog(hWndFrame, hDlg);
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case BID_RES_DEFAULTS:
	    WinSendDlgItemMsg(hDlg, EID_RES_MAX_IMPORTS, SPBM_SETCURRENTVALUE,
			      (MPARAM) atoi(PRF_MAX_IMPORTS_DEF), 0L);

	    WinSendDlgItemMsg(hDlg, EID_RES_MAX_DLLS, SPBM_SETCURRENTVALUE,
			      (MPARAM) atoi(PRF_MAX_DLLS_DEF), 0L);

	    WinSendDlgItemMsg(hDlg, EID_RES_MAX_DEPTH, SPBM_SETCURRENTVALUE,
			      (MPARAM) atoi(PRF_MAX_TREE_DEPTH_DEF), 0L);
	    break;

	case DID_OK:
	    WinSendDlgItemMsg(hDlg, EID_RES_MAX_IMPORTS, SPBM_QUERYVALUE,
		       MPFROMP(&TmpVal), MPFROM2SHORT(0, SPBQ_DONOTUPDATE));
	    PrfMaxNrOfImports = (USHORT) TmpVal;
	    itoa(PrfMaxNrOfImports, TmpStr, 10);
	    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				  PRF_MAX_IMPORTS, TmpStr);

	    WinSendDlgItemMsg(hDlg, EID_RES_MAX_DLLS, SPBM_QUERYVALUE,
		       MPFROMP(&TmpVal), MPFROM2SHORT(0, SPBQ_DONOTUPDATE));
	    PrfMaxNrOfDlls = (USHORT) TmpVal;
	    itoa(PrfMaxNrOfDlls, TmpStr, 10);
	    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				  PRF_MAX_DLLS, TmpStr);

	    WinSendDlgItemMsg(hDlg, EID_RES_MAX_DEPTH, SPBM_QUERYVALUE,
		       MPFROMP(&TmpVal), MPFROM2SHORT(0, SPBQ_DONOTUPDATE));
	    PrfMaxTreeDepth = (USHORT) TmpVal;
	    itoa(PrfMaxTreeDepth, TmpStr, 10);
	    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				  PRF_MAX_TREE_DEPTH, TmpStr);

	    if (PrfMaxNrOfDlls != MaxNrOfDlls ||
		    PrfMaxTreeDepth != MaxTreeDepth)
	    {
		ShwOKMsgBox(hDlg,
		    "Restart application to use new resource limit values");
	    }

	    WinDismissDlg(hDlg, TRUE);
	    break;

	case DID_CANCEL:
	    WinDismissDlg(hDlg, FALSE);
	    break;
	}
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
	break;
    }

    return NULL;

} // ResourcesWndProc

static VOID _SetInitialWindowPos(VOID)
{
    LONG x, y, cx, cy;

    x = PrfQueryProfileInt(HINI_USERPROFILE, PRF_APPL_NAME,
			   PRF_X_KEY, DEF_X_POS);
    y = PrfQueryProfileInt(HINI_USERPROFILE, PRF_APPL_NAME,
			   PRF_Y_KEY, DEF_Y_POS);
    cx = PrfQueryProfileInt(HINI_USERPROFILE, PRF_APPL_NAME,
		      PRF_CX_KEY, DEF_X_POS + (SHORT) (FRAME_CX_MIN * 1.2));
    cy = PrfQueryProfileInt(HINI_USERPROFILE, PRF_APPL_NAME,
		      PRF_CY_KEY, DEF_Y_POS + (SHORT) (FRAME_CY_MIN * 1.5));

    WinSetWindowPos(hWndFrame, NULLHANDLE, x, y, cx, cy,
		    SWP_SIZE | SWP_MOVE | SWP_SHOW);

} // _SetInitialWindowPos

static VOID _SaveWindowPos(LONG x,
			   LONG y,
			   LONG cx,
			   LONG cy)
{
    CHAR HulpStr[11];

    itoa(x, HulpStr, 10);
    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
			  PRF_X_KEY, HulpStr);

    itoa(y, HulpStr, 10);
    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
			  PRF_Y_KEY, HulpStr);

    itoa(cx, HulpStr, 10);
    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
			  PRF_CX_KEY, HulpStr);

    itoa(cy, HulpStr, 10);
    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
			  PRF_CY_KEY, HulpStr);

} // _SaveWindowPos

VOID Shut(APIRET rc)
{
    if (pShowTableData != NULL)
    {
	DosFreeMem(pShowTableData);
	pShowTableData = NULL;
    }
    if (hMsgQ != NULLHANDLE)
    {
	WinDestroyMsgQueue(hMsgQ);
	hMsgQ = NULLHANDLE;
    }
    if (hAB != NULLHANDLE)
    {
	WinTerminate(hAB);
	hAB = NULLHANDLE;
    }

    exit(rc);

} // Shut

//=== ShutWithMessage: Show message in app modal box and die ===

VOID ShutWithMessage(APIRET rc, CHAR *pszFmt, ...)
{
    va_list	pVA;

    /* Point at arguments */
    va_start(pVA, pszFmt);

    if (hMsgQ != NULLHANDLE)
    {
	VShwMsgBox(HWND_DESKTOP,
		   0,
		   pszFmt,
		   pVA);
    }
    else
    {
	vfprintf(stderr, pszFmt, pVA);
	fprintf(stderr, "\n");
    }

    Shut(rc);

} // ShutWithMessage

// The end
