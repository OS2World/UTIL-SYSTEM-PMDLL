
/***********************************************************************

pmdll.h		Definitions

Copyright (c) 2001, 2006 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) pmdll.h - Ver 7, 13-May-12,19:39:54 $

06 Nov 99 AVB Baseline
14 May 01 SHL FDateToText add
14 May 01 SHL FTimeToText add
27 May 01 SHL ComposeTreeDetailLine: add buffer size
27 May 01 SHL STD_NAME_LEN: add
30 May 01 SHL Rename vars
31 May 01 SHL Correct window procedure prototypes
26 Jun 01 SHL Add Load System DLLs support
28 Aug 01 SHL More generic message box routines
15 Mar 06 YD Support full pathnames.  Support gcc
2012-05-12 SHL Support extended LIBPATH

***********************************************************************/

#define INCL_PM
#define INCL_DOSMISC
#define INCL_DOSERRORS
#define INCL_DOSMODULEMGR
#define INCL_DOSNLS

#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pmdllid.h"		// Resource id's

//=== Menu id's ===

#define WUM_START_REBUILD	(WM_USER + 1)
#define WUM_NEW_TREE		(WM_USER + 2)
#define WUM_FONT_CHANGE		(WM_USER + 3)
#define WUM_SHOW_DETAILS	(WM_USER + 4)
#define WUM_BUSY	        (WM_USER + 5)
#define WUM_CHANGE_MODE	        (WM_USER + 6)
#define WUM_SERVERERROR		(WM_USER + 7)
#define WUM_NEW_ENTRY           (WM_USER + 8)
#define WUM_SERVEREND		(WM_USER + 9)

//=== Menu id's ===

//=== Internal limits ===

#define MAX_FONTS		200
#define PROCESS_STACK_SIZE	32000
#define MAX_LIBPATH_SIZE	8192
#define MAX_EXPORT_NAME_LENGTH  127
#define MAX_DLL_ENTRY_LENGTH	255		// Name only, no terminator
#define MAX_NAME_LEN		32		// Name.ext and terminator


#define FRAME_TITLE_HEADER	 "DLL tree - "

typedef struct
{
  CHAR   Signature[2];
  BYTE   ByteOrdening;
  BYTE   WordOrdening;
  ULONG  FormatLevel;
  USHORT CpuType;
  USHORT OsType;
  ULONG  ModuleVersion;
  ULONG  ModuleFlags;
  ULONG  ModuleNrOfPages;
  ULONG  EipObject;
  ULONG  Eip;
  ULONG  EspObject;
  ULONG  Esp;
  ULONG  PageSize;
  ULONG  PageOffsetShift;
  ULONG  FixupSectSize;
  ULONG  FixupSectChcksum;
  ULONG  LoaderSectSize;
  ULONG  LoaderSectChcksum;
  ULONG  ObjectTableOffset;
  ULONG  NrOfObjects;
  ULONG  ObjectPageTableOffset;
  ULONG  ObjectIterPageOffset;
  ULONG  ResourceTableOfset;
  ULONG  NrOfResTableEntries;
  ULONG  ResNameTableOffset;
  ULONG  EntryTableOffset;
  ULONG  ModuleDirOffset;
  ULONG  NrOfModDirectives;
  ULONG  FixupPageTableOffset;
  ULONG  FixupRecordTableOffset;
  ULONG  ImportModTableOffset;
  ULONG  NrOfImportModEntries;
  ULONG  ImportProcTableOffset;
  ULONG  PrePageChcksumOffset;
  ULONG  DataPagesOffset;
  ULONG  NrOfPreloadPages;
  ULONG  NonResNameTableOffset;
  ULONG  NonResNameTableLen;
  ULONG  NonResNameTableChcksum;
  ULONG  NrOfAutoDsObjects;
  ULONG  DebugInfoOffset;
  ULONG  DebugInfoLen;
  ULONG  NrOfPreloadInstances;
  ULONG  NrOfDemandInstances;
  ULONG  HeapSize;
} EXE_HEADER32;

typedef EXE_HEADER32 *PEXE_HEADER32;

typedef struct
{
  CHAR	 Signature[2];
  CHAR	 LinkerVersion[2];
  USHORT EntryTableOffset;
  USHORT EntryTableSize;
  ULONG  FileChcksum;
  USHORT Flags;
  USHORT SegNrAutoDataSeg;
  USHORT HeapSize;
  USHORT StackSize;
  USHORT InitialIpValue;
  USHORT SegNrEntryPoint;
  USHORT InitialSpValue;
  USHORT SegNrStackSegment;
  USHORT NrOfSegments;
  USHORT NrOfModRefTableEntries;
  USHORT NonResNameTableLen;
  USHORT SegmentTableOffset;
  USHORT ResourceTableOffset;
  USHORT ResNameTableOffset;
  USHORT ModRefTableOffset;
  USHORT ImpNamesTableOffset;
  ULONG  NonResNameTableOffset;
  USHORT NrOfMovableSegm;
  USHORT SegmAlignmentShiftCount;
  USHORT ResTableNrOfEntries;
  BYTE	 Reserved[10];
} EXE_HEADER16;

typedef EXE_HEADER16 *PEXE_HEADER16;

typedef struct
{
  CHAR	 Name[MAX_EXPORT_NAME_LENGTH + 1];
  USHORT Ordinal;
  BOOL	 Resident;
} EXPORT_TABLE_DATA;

typedef EXPORT_TABLE_DATA *PEXPORT_TABLE_DATA;

typedef struct
{
  CHAR	 Name[MAX_NAME_LEN];		// room for .ext and terminator
  BOOL	 PathFound;
  CHAR	 Path[CCHMAXPATH];
  BOOL	 SystemDll;
  BOOL	 Loadable;
  APIRET LoadError;
  CHAR	 LoadErrorCause[CCHMAXPATH];
  APIRET FreeError;
  ULONG  FileSize;
  FDATE  fdateLastWrite;
  FTIME  ftimeLastWrite;
  BOOL   Dynamic;
} IMPORT_TABLE_DATA;

typedef IMPORT_TABLE_DATA *PIMPORT_TABLE_DATA;

typedef struct
{
  USHORT Level;
  USHORT SeqNr;
  CHAR	 Name[MAX_NAME_LEN];
  BOOL	 PathFound;
  CHAR	 Path[CCHMAXPATH];
  BOOL	 SystemDll;
  BOOL	 Loadable;
  APIRET LoadError;
  CHAR	 LoadErrorCause[CCHMAXPATH];
  APIRET FreeError;
  ULONG  FileSize;
  FDATE  fdateLastWrite;
  FTIME  ftimeLastWrite;
  BOOL   Dynamic;
} SHOW_TABLE_DATA;

typedef SHOW_TABLE_DATA *PSHOW_TABLE_DATA;

typedef struct
{
  CHAR Path[CCHMAXPATH];
} DYN_TABLE_DATA;

typedef DYN_TABLE_DATA *PDYN_TABLE_DATA;

//=== Window procedures invoked from pmdll.c ===

// chkdir.c
MRESULT EXPENTRY ChangeCurDirWndProc (HWND, ULONG, MPARAM, MPARAM);

// finddll.c
MRESULT EXPENTRY FindDllWndProc (HWND, ULONG, MPARAM, MPARAM);

// libpath.c
MRESULT EXPENTRY LibpathWndProc (HWND, ULONG, MPARAM, MPARAM);

// print.c
MRESULT EXPENTRY PrintWndProc (HWND, ULONG, MPARAM, MPARAM);

// sel_file.c
MRESULT EXPENTRY SelectFileWndProc (HWND, ULONG, MPARAM, MPARAM);

// sel_font.c
MRESULT EXPENTRY SelectFontWndProc (HWND, ULONG, MPARAM, MPARAM);

// sysdll.c
MRESULT EXPENTRY SysDllWndProc (HWND, ULONG, MPARAM, MPARAM);

// trace.c
MRESULT EXPENTRY TraceDllWndProc (HWND, ULONG, MPARAM, MPARAM);

// tree_wnd.c
MRESULT EXPENTRY TreeWndProc (HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY TreeWndSubProc (HWND, ULONG, MPARAM, MPARAM);


//=== Functions ===

// pmdll.c

VOID    main (INT, PCHAR *);

VOID	AskShutWithMessage(APIRET rc, CHAR *pszFmt, ...);
USHORT  InitProgram (INT, PCHAR *);
VOID	LoadFonts (VOID);
USHORT  ChangeFont (HWND, HPS *, USHORT);
VOID	Shut(APIRET rc);
VOID	ShutWithMessage(APIRET rc, CHAR *pszFmt, ...);

// hlp_func.c

BOOL    EmptyField (PSZ Field);
VOID	FDateToString(PSZ psz, FDATE *pFDATE);
VOID	FDateTimeToString(PSZ psz, FDATE *pFDATE, FTIME *pFTime);
VOID	FTimeToString(PSZ psz, FTIME *pFTIME);
VOID    StripSpaces (PSZ Field);

VOID    CursorWait (VOID);
VOID    CursorNormal (VOID);

VOID    DisableMenuItem (HWND hWndFrame, SHORT MenuItemId);
VOID    EnableMenuItem (HWND hWndFrame, SHORT MenuItemId);
VOID    CheckMenuItem (HWND hWndFrame, SHORT MenuItemId);
VOID    UncheckMenuItem (HWND hWndFrame, SHORT MenuItemId);

VOID    CenterDialog (HWND hWnd, HWND hDlg);

VOID    CompressFilePath (PSZ RawText, PSZ Text);
BOOL    StartBuildDllTree (PSZ FileName);
PCHAR   CreateInterpStr (ULONG Number, PCHAR HulpStr);
VOID    ComposeTreeDetailLine (USHORT usIndex, PSZ pszDisplayLine,
			       USHORT usSizeofDisplayLine, PUSHORT pusNameOffset);
VOID    ComposeDllDetailLine (USHORT Index, PSZ DisplayLine);
CHAR    GetBootDrive (VOID);
BOOL    GetLibPath (PSZ LibPath, USHORT MaxLibPathSize, PSZ ErrorStr, BOOL includeExtLIBPATH);
BOOL    UpdateLibPath (PSZ LibPath, PSZ ErrorStr);
BOOL    CheckDirPath (PSZ NewEntry, PSZ ErrorStr);

//=== help_func.c ===

MRESULT ProcessDragOver (MPARAM mp1);
MRESULT ProcessDragDrop (MPARAM mp1);

VOID    SetSwitchWnd (HWND hWnd);

BOOL    StrInListbox (HWND hDlg, SHORT LbId, PSZ LbStr);

BOOL    ExeHdrGetImports (PSZ PgmName, BOOL TestLoadModuleParm, PSZ LibPath,
			  PBOOL Bit32, PUSHORT NrOfEntries,
			  PIMPORT_TABLE_DATA ImportTable, PSZ ErrorStr);
BOOL	ExeHdrGetExports (PSZ PgmName, PBOOL Bit32, PUSHORT NrOfEntries,
			  PEXPORT_TABLE_DATA ExportTable, PSZ ModuleName,
			  PSZ ModDescr, PSZ ErrorStr);

APIRET	ShwOKCanMsgBox(CHAR *pszFmt, ...);
VOID	ShwOKDeskMsgBox(CHAR *pszFmt, ...);
VOID	ShwOKMsgBox(HWND hwndOwner, CHAR *pszFmt, ...);
APIRET	ShwYesNoMsgBox(HWND hwndOwner, CHAR *pszFmt, ...);
APIRET	VShwMsgBox(HWND hwndOwner, ULONG flStyle, CHAR *pszFmt, va_list pVA);

//=== Globals ===

#ifdef GEN_GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN HAB    hAB;
EXTERN HMQ    hMsgQ;
EXTERN HWND   hVscroll, hHscroll, hWndTrace;
EXTERN CHAR   ApplTitle[81];
EXTERN CHAR   FrameTitle[81];
EXTERN CHAR   ErrorStr[128];
EXTERN CHAR   Libpath[MAX_LIBPATH_SIZE];
EXTERN HWND   hWndFrame, hWndClient, hWndFrameTree, hWndClientTree;
EXTERN PFNWP  TreeFrameProc;
EXTERN BOOL   FileSelected;
EXTERN CHAR   CurrentFileName[CCHMAXPATH];
EXTERN HWND   hWndProcess;
EXTERN PSHOW_TABLE_DATA pShowTableData;
EXTERN USHORT cShowTableData;
EXTERN CHAR   SysDllPath[CCHMAXPATH];
EXTERN BOOL   CsChanged;
EXTERN CHAR   DragPath[CCHMAXPATH];
EXTERN CHAR   DragFile[CCHMAXPATH];
EXTERN BOOL   TestLoadModule;
EXTERN BOOL   TestLoadModuleUsed;
EXTERN BOOL   TreeViewMode;
EXTERN USHORT NrOfDlls;
EXTERN HSWITCH hSwitch;
EXTERN PDYN_TABLE_DATA pDynTable;
EXTERN USHORT NrOfDynItems;
EXTERN FONTMETRICS  fm[MAX_FONTS];
EXTERN SHORT  FontEntry;
EXTERN SHORT  FontHeight;
EXTERN SHORT  FontWidth;
EXTERN SHORT  FontOffset;
EXTERN BOOL   FontsLoaded;
EXTERN SHORT  NrLoadedFonts;
EXTERN USHORT MaxNrOfDlls, MaxTreeDepth;
EXTERN COUNTRYINFO CountryInfo;
EXTERN COUNTRYCODE CountryCode;

// Profile data

#define PRF_APPL_NAME	     "PM_DllTree"
EXTERN CHAR		     ProfileData[81];

#define PRF_MAX_DLLS	     "MaxDlls"
EXTERN USHORT                PrfMaxNrOfDlls;

#define PRF_MAX_IMPORTS      "MaxImports"
EXTERN USHORT                PrfMaxNrOfImports;

#define PRF_MAX_TREE_DEPTH   "MaxTreeDepth"
EXTERN USHORT                PrfMaxTreeDepth;

#define PRF_FONT_KEY	     "FontNr"
EXTERN USHORT		     PrfFontNr;

#define PRF_USE_SYSDLLS_KEY  "LoadSysDlls"
EXTERN USHORT		     fLoadSysDlls;

#define PRF_SYSDLL_KEY	     "SysDlls"
EXTERN CHAR		     PrfSysDllPath[MAX_LIBPATH_SIZE];

EXTERN USHORT                PrfPageLen;
#define PRF_PAGELEN_KEY      "Pagelength"

EXTERN USHORT		     PrfPageWidth;
#define PRF_PAGEWIDTH_KEY    "Pagewidth"

EXTERN CHAR		     PrfPrinter[51];
#define PRF_PRINTER_KEY      "Printer"

#define PRF_EXE_PARM_KEY     "Exe parm"

#define PRF_TRACE_NO_SYS     "Trace no sys"

#define PRF_TEST_LOAD        "Test load"

#define PRF_TREE_VIEW        "Tree view"

#define PRF_X_KEY	     "X"
#define PRF_CX_KEY	     "CX"
#define PRF_Y_KEY	     "Y"
#define PRF_CY_KEY	     "CY"

#define DEF_X_POS	     70
#define DEF_Y_POS	     50

#ifdef __EMX__
#define min(a,b) (a<b ? a : b)
#define max(a,b) (a>b ? a : b)
#endif

// The end
