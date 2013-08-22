
/***********************************************************************

tree_wnd.c - Tree window builder and EXE/DLL scanners

Copyright (c) 2001, 2012 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) tree_wnd.c - Ver 9, 04-Aug-12,00:18:26 $

1995-07-16 AVB Baseline
2001-05-27 SHL _BldTreeThread: avoid Name[] overflow
2001-05-31 SHL Correct window procedure arguments drop casts
2001-06-18 SHL StartBuildDllTree: avoid race, catch errors
2001-06-18 SHL Get rid of HWND casts
2001-08-28 SHL Localize code
2006-03-15 YD Show load fail reason
2012-04-22 SHL Rework exceptq support
2012-04-22 SHL Avoid OpenWatcom warnings
2012-05-12 SHL Sync with extended LIBPATH mods
2012-08-04 SHL Sync with exceptq 7.x

***********************************************************************/

#define INCL_DOSSESMGR

#if defined(EXCEPTQ)
#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#endif

#include "pmdll.h"

#include <process.h>

#if defined(EXCEPTQ)
#include "exceptq.h"
#endif

typedef struct
{
    CHAR Name[MAX_NAME_LEN];
} DLL_TREE_TABLE;

typedef DLL_TREE_TABLE *PDLL_TREE_TABLE;

static VOID _SetSliders(HWND hWnd, SHORT MaxLineWidth,
			SHORT sVPos, SHORT sHPos);
static VOID _DisplayTree(HPS hPS, SHORT sVPos, SHORT sHPos, RECTL rClient,
			 SHORT DetailLine);
static VOID _DisplayDlls(HPS hPS, SHORT sVPos, SHORT sHPos, RECTL rClient,
			 SHORT DetailLine);
static VOID _RepaintLine(HWND hWnd, SHORT sVPos, SHORT DetailLine);
static VOID _BldTreeThread(PVOID ThreadNr);
static BOOL _WalkPath(PSZ PgmName, PSZ ErrorStr);
static BOOL _GetImportsTable(PSZ pPgmName, PIMPORT_TABLE_DATA pImportTable,
			     PUSHORT pNrOfEntries, PSZ pErrorStr);

static USHORT Level;
static PDLL_TREE_TABLE pDllTreeTable;
static BOOL DynDlls;

MRESULT EXPENTRY BuildTreeWndProc (HWND, ULONG, MPARAM, MPARAM);

//== TreeWndProc: Tree Window Procedure ===

MRESULT EXPENTRY TreeWndProc(HWND hWnd,
			     ULONG Msg,
			     MPARAM mp1,
			     MPARAM mp2)
{
    static HPS hPSInit;
    static SHORT sVPos, sHPos, MaxLineWidth;
    static SHORT DetailLine;
    static BOOL ButtonDown;
    HPS hPS;
    RECTL rClient;
    USHORT rc;
    USHORT i;
    USHORT MaxLevel;
    SHORT CharsPerLine, LinesPerPage, sHPosOld, sVPosOld, NewDetailLine,
	KeyCode, HyperlinkLine;
    USHORT fs;

    switch (Msg)
    {
    case WM_CREATE:
	hPSInit = NULLHANDLE;
	sVPos = 0;
	sHPos = 0;
	DetailLine = -1;
	ButtonDown = FALSE;

	rc = ChangeFont(hWnd, &hPSInit, 850);
	if (rc)
	    ShwOKDeskMsgBox("Error changing font: %u", rc);

	hVscroll = WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT),
				   FID_VERTSCROLL);
	hHscroll = WinWindowFromID(WinQueryWindow(hWnd, QW_PARENT),
				   FID_HORZSCROLL);
	WinSetFocus(HWND_DESKTOP, hWnd);
	break;

    case WUM_NEW_TREE:
	sVPos = 0;
	sHPos = 0;
	MaxLevel = 0;
	DetailLine = 0;
	WinPostMsg(hWndFrame, WUM_SHOW_DETAILS, MPFROMSHORT(DetailLine), 0L);

	for (i = 0; i < cShowTableData; ++i)
	{
	    if (pShowTableData[i].Level > MaxLevel)
		MaxLevel = pShowTableData[i].Level;
	}

	MaxLineWidth = (SHORT) (13 * MaxLevel + 13);
	WinInvalidateRect(hWnd, NULL, FALSE);

	if (TreeViewMode)
	    _SetSliders(hWnd, MaxLineWidth, sVPos, sHPos);
	else
	    _SetSliders(hWnd, CCHMAXPATH, sVPos, sHPos);
	break;

    case WUM_FONT_CHANGE:
	rc = ChangeFont(hWnd, &hPSInit, 850);
	if (rc)
	    ShwOKDeskMsgBox("Error changing font: %u", rc);

	WinInvalidateRect(hWnd, NULL, FALSE);
	if (TreeViewMode)
	    _SetSliders(hWnd, MaxLineWidth, sVPos, sHPos);
	else
	    _SetSliders(hWnd, CCHMAXPATH, sVPos, sHPos);
	break;

    case WUM_CHANGE_MODE:
	WinInvalidateRect(hWnd, NULL, FALSE);
	if (TreeViewMode)
	    _SetSliders(hWnd, MaxLineWidth, sVPos, sHPos);
	else
	    _SetSliders(hWnd, CCHMAXPATH, sVPos, sHPos);
	break;

    case WM_SIZE:
	if (TreeViewMode)
	    _SetSliders(hWnd, MaxLineWidth, sVPos, sHPos);
	else
	    _SetSliders(hWnd, CCHMAXPATH, sVPos, sHPos);
	break;

    case WM_CHAR:

	// Ignore messages while busy
	if (!FileSelected)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	// Ignore unexpected keys
	fs = SHORT1FROMMP(mp1);

	if (fs & KC_KEYUP || ~fs & KC_VIRTUALKEY)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	KeyCode = SHORT2FROMMP(mp2);

	switch (KeyCode)
	{
	case VK_HOME:
	    WinSendMsg(hWnd, WM_VSCROLL, 0L, MPFROM2SHORT(0, SB_SLIDERPOSITION));
	    return (MRFROMSHORT(TRUE));
	    break;

	case VK_END:
	    WinSendMsg(hWnd, WM_VSCROLL,
		       0L,
		       MPFROM2SHORT(cShowTableData, SB_SLIDERPOSITION));
	    return (MRFROMSHORT(TRUE));
	    break;

	case VK_UP:
	    WinSendMsg(hWnd, WM_VSCROLL, 0L, MPFROM2SHORT(0, SB_LINEUP));
	    return (MRFROMSHORT(TRUE));
	    break;

	case VK_PAGEUP:
	    WinSendMsg(hWnd, WM_VSCROLL, 0L, MPFROM2SHORT(0, SB_PAGEUP));
	    return (MRFROMSHORT(TRUE));
	    break;

	case VK_DOWN:
	    WinSendMsg(hWnd, WM_VSCROLL, 0L, MPFROM2SHORT(0, SB_LINEDOWN));
	    return (MRFROMSHORT(TRUE));
	    break;

	case VK_PAGEDOWN:
	    WinSendMsg(hWnd, WM_VSCROLL, 0L, MPFROM2SHORT(0, SB_PAGEDOWN));
	    return (MRFROMSHORT(TRUE));
	    break;

	default:
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));
	}
	break;

    case WM_BUTTON1DOWN:
	if (WinQueryFocus(HWND_DESKTOP) != hWndClientTree)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	WinSetCapture(HWND_DESKTOP, hWndClientTree);
	ButtonDown = TRUE;

	WinQueryWindowRect(hWnd, &rClient);
	LinesPerPage = (SHORT) (rClient.yTop / FontHeight);
	NewDetailLine = (SHORT) (sVPos + (rClient.yTop - SHORT2FROMMP(mp1))
				 / FontHeight);

	if (NewDetailLine >= (SHORT) cShowTableData)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	_RepaintLine(hWnd, sVPos, DetailLine);
	DetailLine = NewDetailLine;
	_RepaintLine(hWnd, sVPos, DetailLine);
	WinPostMsg(hWndFrame, WUM_SHOW_DETAILS, MPFROMSHORT(DetailLine), 0L);

	return (WinDefWindowProc(hWnd, Msg, mp1, mp2));
	break;

    case WM_BUTTON1UP:
	if (!ButtonDown)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	WinSetCapture(HWND_DESKTOP, NULLHANDLE);
	ButtonDown = FALSE;
	break;

    case WM_BUTTON1DBLCLK:
	WinQueryWindowRect(hWnd, &rClient);
	LinesPerPage = (SHORT) (rClient.yTop / FontHeight);
	HyperlinkLine = (SHORT) (sVPos + (rClient.yTop - SHORT2FROMMP(mp1))
				 / FontHeight);

	if (HyperlinkLine >= (SHORT) cShowTableData)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	for (i = 0; i < cShowTableData; ++i)
	{
	    if (stricmp(pShowTableData[i].Path,
			pShowTableData[HyperlinkLine].Path) == 0)
		break;
	}

	if (i < HyperlinkLine)
	{
	    _RepaintLine(hWnd, sVPos, DetailLine);
	    DetailLine = i;
	    _RepaintLine(hWnd, sVPos, DetailLine);

	    sVPos = i;
	    sVPos = (SHORT) max(0, sVPos - 2);

	    WinSendMsg(hVscroll, SBM_SETPOS, MPFROM2SHORT(sVPos, 0), 0);
	    WinInvalidateRect(hWnd, NULL, TRUE);

	    WinPostMsg(hWndFrame, WUM_SHOW_DETAILS, MPFROMSHORT(DetailLine), 0L);
	}
	break;

    case WM_BUTTON2DBLCLK:
	WinQueryWindowRect(hWnd, &rClient);
	LinesPerPage = (SHORT) (rClient.yTop / FontHeight);
	NewDetailLine = (SHORT) (sVPos + (rClient.yTop - SHORT2FROMMP(mp1))
				 / FontHeight);

	if (NewDetailLine >= (SHORT) cShowTableData)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	_RepaintLine(hWnd, sVPos, DetailLine);
	DetailLine = NewDetailLine;
	_RepaintLine(hWnd, sVPos, DetailLine);

	WinPostMsg(hWndFrame, WUM_SHOW_DETAILS, MPFROMSHORT(DetailLine), 0L);

	if (!pShowTableData[DetailLine].PathFound)
	{
	    DosBeep(1000, 100);
	    break;
	}

	break;

    case WM_MOUSEMOVE:
	if (WinQueryFocus(HWND_DESKTOP) != hWndClientTree)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	if (!ButtonDown)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	if (!FileSelected)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	WinQueryWindowRect(hWnd, &rClient);
	LinesPerPage = (SHORT) (rClient.yTop / FontHeight);
	NewDetailLine = (SHORT) (sVPos + (rClient.yTop - SHORT2FROMMP(mp1))
				 / FontHeight);

	if (NewDetailLine >= (SHORT) cShowTableData)
	    return (WinDefWindowProc(hWnd, Msg, mp1, mp2));

	if (NewDetailLine != DetailLine)
	{
	    _RepaintLine(hWnd, sVPos, DetailLine);
	    DetailLine = NewDetailLine;
	    _RepaintLine(hWnd, sVPos, DetailLine);

	    WinPostMsg(hWndFrame, WUM_SHOW_DETAILS, MPFROMSHORT(DetailLine), 0L);
	}

	return (WinDefWindowProc(hWnd, Msg, mp1, mp2));
	break;

    case WM_PAINT:
	hPS = WinBeginPaint(hWnd, hPSInit, 0);
	WinQueryWindowRect(hWnd, &rClient);
	WinFillRect(hPS, &rClient, CLR_WHITE);

	if (FileSelected)
	{
	    if (TreeViewMode)
		_DisplayTree(hPS, sVPos, sHPos, rClient, DetailLine);
	    else
		_DisplayDlls(hPS, sVPos, sHPos, rClient, DetailLine);
	}

	WinEndPaint(hPS);
	break;

    case WM_HSCROLL:
	sHPosOld = sHPos;
	WinQueryWindowRect(hWnd, &rClient);
	CharsPerLine = (SHORT) (rClient.xRight / FontWidth);

	switch (SHORT2FROMMP(mp2))
	{
	case SB_LINEDOWN:
	    if (TreeViewMode)
		sHPos = (SHORT) min(MaxLineWidth, sHPos + 1);
	    else
		sHPos = (SHORT) min(CCHMAXPATH, sHPos + 1);
	    break;

	case SB_PAGEDOWN:
	    if (TreeViewMode)
		sHPos = (SHORT) min(MaxLineWidth - CharsPerLine, sHPos + 13);
	    else
		sHPos = (SHORT) min(CCHMAXPATH - CharsPerLine, sHPos + 10);
	    break;

	case SB_LINEUP:
	    sHPos = (SHORT) max(0, sHPos - 1);
	    break;

	case SB_PAGEUP:
	    if (TreeViewMode)
		sHPos = (SHORT) max(0, sHPos - 13);
	    else
		sHPos = (SHORT) max(0, sHPos - 10);
	    break;

	case SB_SLIDERTRACK:
	    sHPos = SHORT1FROMMP(mp2);
	    break;

	default:
	    return 0;
	}

	WinSendMsg(hHscroll, SBM_SETPOS, MPFROM2SHORT(sHPos, 0), 0);

	WinScrollWindow(hWnd, (sHPosOld - sHPos) * FontWidth, 0,
			NULL, NULL, NULLHANDLE,
			NULL, SW_INVALIDATERGN);
	break;

    case WM_VSCROLL:
	sVPosOld = sVPos;
	WinQueryWindowRect(hWnd, &rClient);
	LinesPerPage = (SHORT)(rClient.yTop / FontHeight);

	switch (SHORT2FROMMP(mp2))
	{
	case SB_SLIDERPOSITION:
	    sVPos = SHORT1FROMMP(mp2);
	    break;

	case SB_LINEDOWN:
	    sVPos = sVPos + 1;
	    break;

	case SB_PAGEDOWN:
	    sVPos = sVPos + LinesPerPage - 1;
	    break;

	case SB_LINEUP:
	    sVPos = sVPos - 1;
	    break;

	case SB_PAGEUP:
	    sVPos = sVPos - LinesPerPage + 1;
	    break;

	case SB_SLIDERTRACK:
	    sVPos = SHORT1FROMMP(mp2);
	    break;

	default:
	    return (0);
	}

	if (sVPos > 0)
	    sVPos = (SHORT)min(cShowTableData - LinesPerPage, sVPos);

	if (sVPos < 0)
	    sVPos = 0;

	if (sVPos != sVPosOld)
	{
	    WinSendMsg(hVscroll, SBM_SETPOS, MPFROM2SHORT(sVPos, 0), 0);

	    WinScrollWindow(hWnd, 0, (sVPos - sVPosOld) * FontHeight,
			    NULL, NULL, NULLHANDLE,
			    NULL, SW_INVALIDATERGN);
	}
	break;

#if 0
    case DM_DRAGOVER:
	return (ProcessDragOver(mp1));
	break;

    case DM_DROP:
	return (ProcessDragDrop(mp1));
	break;
#endif

    default:
	return (WinDefWindowProc(hWnd, Msg, mp1, mp2));
	break;
    }

    return (0L);
}

VOID _SetSliders(HWND hWnd,
		 SHORT MaxLineWidth,
		 SHORT sVPos,
		 SHORT sHPos)
{
    RECTL rClient;
    SHORT LinesPerPage, CharsPerLine;

    WinQueryWindowRect(hWnd, &rClient);

    LinesPerPage = (SHORT) (rClient.yTop / FontHeight);
    CharsPerLine = (SHORT) (rClient.xRight / FontWidth);

    WinSendMsg(hHscroll, SBM_SETTHUMBSIZE,
	       MPFROM2SHORT(CharsPerLine, MaxLineWidth), 0L);
    WinSendMsg(hVscroll, SBM_SETTHUMBSIZE,
	       MPFROM2SHORT(LinesPerPage, cShowTableData), 0L);

    WinSendMsg(hHscroll, SBM_SETSCROLLBAR, 0L,
	       MPFROM2SHORT(0, MaxLineWidth - CharsPerLine));
    WinSendMsg(hVscroll, SBM_SETSCROLLBAR, 0L,
	       MPFROM2SHORT(0, cShowTableData - LinesPerPage));

    WinSendMsg(hHscroll, SBM_SETPOS, MPFROM2SHORT(sHPos, 0), 0);
    WinSendMsg(hVscroll, SBM_SETPOS, MPFROM2SHORT(sVPos, 0), 0);

}

static VOID _RepaintLine(HWND hWnd,
			 SHORT sVPos,
			 SHORT Line)
{
    RECTL Rectl;
    SHORT LinesPerPage;

    WinQueryWindowRect(hWnd, &Rectl);

    LinesPerPage = (SHORT) (Rectl.yTop / FontHeight);

    if (Line < sVPos ||
	    Line > sVPos + LinesPerPage)
	return;

    Rectl.yTop = Rectl.yTop - (Line - sVPos) * FontHeight;
    Rectl.yBottom = Rectl.yTop - FontHeight;

    WinInvalidateRect(hWnd, &Rectl, FALSE);

}

//=== _DisplayTree: display tree on screen ===

static VOID _DisplayTree(HPS hPS,
			 SHORT sVPos,
			 SHORT sHPos,
			 RECTL rClient,
			 SHORT iDetailLine)
{
    USHORT iLine;
    SHORT j;
    USHORT cLinesPerPage;
    POINTL Ptl;
    CHAR szDisplayLine[1024];
    USHORT iTableData;
    USHORT CharsToPrint;
    USHORT iNameOffset;
    ULONG Color;
    RECTL rEntry;

    cLinesPerPage = rClient.yTop / FontHeight;

    Ptl.x = 0;
    Ptl.y = rClient.yTop - FontHeight + FontOffset;

    for (iLine = 0; iLine <= cLinesPerPage; iLine++)
    {
	iTableData = iLine + sVPos;
	memset(szDisplayLine, 0, sizeof(szDisplayLine));

	if (iTableData < cShowTableData)
	    ComposeTreeDetailLine(iTableData, szDisplayLine, sizeof(szDisplayLine), &iNameOffset);

	if ((size_t)sHPos < strlen(szDisplayLine))
	{
	    GpiSetColor(hPS, CLR_BLACK);
	    GpiCharStringAt(hPS, &Ptl, strlen(szDisplayLine + sHPos),
			    szDisplayLine + sHPos);

	    if (sHPos < iNameOffset + strlen(pShowTableData[iTableData].Name))
	    {
		if (iTableData == 0)
		{
		    for (j = 1; j < cShowTableData; ++j)
		    {
			if (!pShowTableData[j].PathFound ||
				!pShowTableData[j].Loadable)
			    break;
		    }

		    if (j < cShowTableData)
			Color = CLR_RED;
		    else
			Color = CLR_BLACK;
		}
		else
		{
		    if (!pShowTableData[iTableData].PathFound ||
			!pShowTableData[iTableData].Loadable)
		    {
			Color = CLR_RED;	// A bad boy
		    }
		    else if (pShowTableData[iTableData].SystemDll)
			Color = CLR_PALEGRAY;
		    else
		    {
			for (j = 0; j < iTableData; ++j)
			{
			    if (strcmp(pShowTableData[j].Name, pShowTableData[iTableData].Name) == 0)
				break;		// Found it
			}

			if (j == iTableData)
			    Color = CLR_BLACK;		// New
			else
			    Color = CLR_DARKBLUE;	// Already seen
		    }
		}

		if (sHPos < iNameOffset)
		{
		    CharsToPrint = strlen(pShowTableData[iTableData].Name);
		    Ptl.x += (iNameOffset - sHPos) * FontWidth;
		}
		else
		    CharsToPrint = iNameOffset +
				   strlen(pShowTableData[iTableData].Name) -
				   sHPos;

		if (iTableData == iDetailLine)
		{
		    rEntry.xLeft = Ptl.x;
		    rEntry.xRight = Ptl.x + CharsToPrint * FontWidth;
		    rEntry.yBottom = Ptl.y - FontOffset;
		    rEntry.yTop = Ptl.y + FontHeight - FontOffset;

		    WinFillRect(hPS, &rEntry, Color);

		    GpiSetColor(hPS, CLR_WHITE);
		}
		else
		    GpiSetColor(hPS, Color);

		GpiCharStringAt(hPS, &Ptl, CharsToPrint,
				szDisplayLine +
				(iNameOffset + strlen(pShowTableData[iTableData].Name) - CharsToPrint));

		Ptl.x = 0;
	    }
	}

	Ptl.y -= FontHeight;

    } // for cLinesPerPage

} // _DisplayTree

static VOID _DisplayDlls(HPS hPS,
			 SHORT sVPos,
			 SHORT sHPos,
			 RECTL rClient,
			 SHORT DetailLine)
{
    USHORT iLine;
    USHORT cLinesPerPage;
    POINTL Ptl;
    CHAR szDisplayLine[CCHMAXPATH + 50];
    USHORT j;
    USHORT iTableData;
    ULONG Color;
    RECTL rEntry;

    cLinesPerPage = rClient.yTop / FontHeight;

    Ptl.x = 0;
    Ptl.y = rClient.yTop - FontHeight + FontOffset;

    for (iLine = 0; iLine <= cLinesPerPage; iLine++)
    {
	iTableData = iLine + sVPos;
	memset(szDisplayLine, 0, sizeof(szDisplayLine));

	if (iTableData < cShowTableData)
	{
	    ComposeDllDetailLine(iTableData, szDisplayLine);

	    if (iTableData == 0)
	    {
		for (j = 1; j < cShowTableData; ++j)
		{
		    if (!pShowTableData[j].PathFound ||
			    !pShowTableData[j].Loadable)
			break;
		}

		if (j < cShowTableData)
		    Color = CLR_RED;
		else
		    Color = CLR_BLACK;
	    }
	    else
	    {
		if (!pShowTableData[iTableData].PathFound ||
			!pShowTableData[iTableData].Loadable)
		    Color = CLR_RED;
		else if (pShowTableData[iTableData].SystemDll)
		    Color = CLR_PALEGRAY;
		else
		{
		    for (j = 0; j < iTableData; ++j)
		    {
			if (strcmp(pShowTableData[j].Name, pShowTableData[iTableData].Name) == 0)
			    break;	// Found it
		    }

		    if (j == iTableData)
			Color = CLR_BLACK;
		    else
			Color = CLR_DARKBLUE;
		}
	    }

	    if ((SHORT) iTableData == DetailLine)
	    {
		rEntry.xLeft = Ptl.x;
		rEntry.xRight = Ptl.x + rClient.xRight;
		rEntry.yBottom = Ptl.y - FontOffset;
		rEntry.yTop = Ptl.y + FontHeight - FontOffset;

		WinFillRect(hPS, &rEntry, Color);

		GpiSetColor(hPS, CLR_WHITE);
	    }
	    else
		GpiSetColor(hPS, Color);

	    if (strlen(szDisplayLine) > sHPos)
	    {
		GpiCharStringAt(hPS, &Ptl, strlen(szDisplayLine + sHPos),
				szDisplayLine + sHPos);
	    }
	} // if in table range

	Ptl.y -= FontHeight;

    } // for iLine

} // _DisplayDlls

//=== TreeWndSubProc: Tree Window Frame Procedure ===

MRESULT EXPENTRY TreeWndSubProc(HWND hWnd,
				ULONG Msg,
				MPARAM mp1,
				MPARAM mp2)
{
    PTRACKINFO pTrack;
    USHORT cmdsrc;

    switch (Msg)
    {
    case WM_SYSCOMMAND:
    case WM_COMMAND:
	// If from accelerator pass to main frame
	cmdsrc = SHORT1FROMMP(mp2);
	if (cmdsrc == CMDSRC_ACCELERATOR)
	{
	    WinPostMsg(hWndFrame, Msg, mp1, mp2);
	    return MRFROMSHORT(0);
	}
	break;

    case WM_QUERYTRACKINFO:
	(*TreeFrameProc) (hWnd, Msg, mp1, mp2);
	pTrack = (PTRACKINFO) mp2;
	if ((pTrack -> fs & TF_MOVE) == TF_MOVE)
	    WinSetActiveWindow(HWND_DESKTOP, hWndFrameTree);
	return (MRESULT) FALSE;
	break;
    }

    return (*TreeFrameProc)(hWnd, Msg, mp1, mp2);
}

BOOL StartBuildDllTree(PSZ FileName)
{
    LONG ProcessTid;

    FileSelected = FALSE;

    hWndProcess = WinLoadDlg(HWND_DESKTOP, hWndFrame, BuildTreeWndProc,
			     0, DID_BUILDING_TREE, NULL);

    if (hWndProcess == NULLHANDLE)
    {
	ShwOKDeskMsgBox("Can not load Building Tree dialog");
    }
    else
    {
	ProcessTid = _beginthread(_BldTreeThread,
				  NULL,
				  PROCESS_STACK_SIZE,
				  FileName);
	if (ProcessTid == -1)
	{
	    ShwOKDeskMsgBox("Can not start Build Tree thread");
	    WinPostMsg(hWndProcess, WM_CLOSE, 0L, 0L);
	    return FALSE;
	}
    }

    return TRUE;

} // StartBuildDllTree

MRESULT EXPENTRY BuildTreeWndProc(HWND hDlg,
				  ULONG Msg,
				  MPARAM mp1,
				  MPARAM mp2)
{
    static USHORT Count;

    USHORT i;
    CHAR BusyStr[51];

    switch (Msg)
    {
    case WM_INITDLG:
	Count = 0;
	DisableMenuItem(hWndFrame, MID_FILE);
	DisableMenuItem(hWndFrame, MID_OPTIONS);

	WinPostMsg(hDlg, WUM_BUSY, 0L, 0L);

	CenterDialog(hWndFrame, hDlg);
	WinSetFocus(HWND_DESKTOP, hDlg);
	return ((MRESULT) TRUE);
	break;

    case WUM_BUSY:
	strcpy(BusyStr, "Building tree");
	for (i = 0; i < Count; ++i)
	    strcat(BusyStr, ".");

	if (Count > 30)
	    Count = 1;
	else
	    ++Count;

	WinSetDlgItemText(hDlg, TID_BUILDING_TREE, BusyStr);
	break;

    case WM_CLOSE:
	WinDestroyWindow(hDlg);
	hWndProcess = NULLHANDLE;

	EnableMenuItem(hWndFrame, MID_FILE);
	EnableMenuItem(hWndFrame, MID_OPTIONS);

	if (FileSelected)
	{
	    strcpy(FrameTitle, FRAME_TITLE_HEADER);
	    if (strlen(CurrentFileName) > 35)
	    {
		CompressFilePath(CurrentFileName, ErrorStr);
		strcat(FrameTitle, ErrorStr);
	    }
	    else
		strcat(FrameTitle, CurrentFileName);
	}
	else
	    strcpy(FrameTitle, ApplTitle);

	WinSetWindowText(hWndFrame, FrameTitle);

	WinPostMsg(hWndFrameTree, WUM_NEW_TREE, 0L, 0L);
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
    }

    return NULL;
}

//== _BldTreeThread: Build DLL tree thread ===

static VOID _BldTreeThread(PVOID ThreadNr)
{
#ifdef EXCEPTQ
    EXCEPTIONREGISTRATIONRECORD exRegRec;
#endif
    HAB hThrAB;
    HMQ hThrMsgQ;
    APIRET rc;
    PCHAR pStr;
    ULONG ApplType;
    FILESTATUS FileStatus;
    USHORT i;
    USHORT j;
    USHORT StatDllOffset;
    USHORT DynDllOffset;
    USHORT RemoveLevel;
    USHORT InsertLevel;
    BOOL NewLevel;

#ifdef EXCEPTQ
    LoadExceptq(&exRegRec, NULL, NULL);
#endif

    hThrAB = WinInitialize(0);
    hThrMsgQ = WinCreateMsgQueue(hThrAB, 0);

    DynDlls = FALSE;
    TestLoadModuleUsed = TestLoadModule;
    cShowTableData = 0;
    Level = 0;
    memset(pShowTableData, 0x00, MaxNrOfDlls * sizeof(SHOW_TABLE_DATA));
    strupr(CurrentFileName);

    rc = DosQueryAppType(CurrentFileName, &ApplType);
    if (rc)
    {
	WinPostMsg(hWndProcess, WM_CLOSE, 0L, 0L);
	switch (rc)
	{
	case ERROR_BAD_FORMAT:
	    strcpy(ErrorStr, "EXE/DLL has a bad format");
	    break;

	case ERROR_SHARING_VIOLATION:
	    strcpy(ErrorStr, "EXE/DLL is read locked by another process");
	    break;

	case ERROR_INVALID_EXE_SIGNATURE:
	    strcpy(ErrorStr, "EXE/DLL has an invalid EXE signature");
	    break;

	case ERROR_EXE_MARKED_INVALID:
	    strcpy(ErrorStr, "EXE/DLL has been marked as invalid");
	    break;

	case ERROR_BAD_EXE_FORMAT:
	    strcpy(ErrorStr, "File has an invalid EXE/DLL format");
	    break;

	default:
	    sprintf(ErrorStr, "Could not get application type (%lu)", rc);
	    break;
	}

	ShwOKMsgBox(hWndFrame, ErrorStr);

	WinDestroyMsgQueue(hThrMsgQ);
	WinTerminate(hThrAB);
#       if defined(EXCEPTQ)
	UninstallExceptq(&exRegRec);
#       endif
	_endthread();
    }

    if ((ApplType & FAPPTYP_NOTWINDOWCOMPAT) != FAPPTYP_NOTWINDOWCOMPAT &&
	    (ApplType & FAPPTYP_WINDOWCOMPAT) != FAPPTYP_WINDOWCOMPAT &&
	    (ApplType & FAPPTYP_WINDOWAPI) != FAPPTYP_WINDOWAPI &&
	    (ApplType & FAPPTYP_DLL) != FAPPTYP_DLL)
    {
	WinPostMsg(hWndProcess, WM_CLOSE, 0L, 0L);

	switch (ApplType & 0xFFFFFFF8)
	{
	case FAPPTYP_BOUND:
	    strcpy(ErrorStr, "File is a bound application and is not supported");
	    break;

	case FAPPTYP_DOS:
	    strcpy(ErrorStr, "File is a DOS application and does not use DLLs");
	    break;

	case FAPPTYP_PHYSDRV:
	case FAPPTYP_VIRTDRV:
	    strcpy(ErrorStr, "File is a device driver and is not supported");
	    break;

	case FAPPTYP_WINDOWSREAL:
	case FAPPTYP_WINDOWSPROT:
	case 4096:			// undocumented !?

	    strcpy(ErrorStr, "File is a Windows program/DLL and is not supported");
	    break;

	case FAPPTYP_PROTDLL:
	    strcpy(ErrorStr, "File is a Windows DLL and is not supported");
	    break;

	default:
	    sprintf(ErrorStr, "Unsupported EXE/DLL type (%lu)", ApplType);
	    break;
	}

	ShwOKMsgBox(hWndFrame, ErrorStr);

	WinDestroyMsgQueue(hThrMsgQ);
	WinTerminate(hThrAB);
#       if defined(EXCEPTQ)
	UninstallExceptq(&exRegRec);
#       endif
	_endthread();
    }

    if (!GetLibPath(Libpath, MAX_LIBPATH_SIZE, ErrorStr, TRUE))	{
	WinPostMsg(hWndProcess, WM_CLOSE, 0L, 0L);
	ShwOKMsgBox(hWndFrame, ErrorStr);

	WinDestroyMsgQueue(hThrMsgQ);
	WinTerminate(hThrAB);
#       if defined(EXCEPTQ)
	UninstallExceptq(&exRegRec);
#       endif
	_endthread();
    }

    rc = DosAllocMem((PPVOID) & pDllTreeTable,
		     MaxNrOfDlls * sizeof(DLL_TREE_TABLE),
		     PAG_READ | PAG_WRITE | PAG_COMMIT);
    if (rc)
    {
	WinPostMsg(hWndProcess, WM_CLOSE, 0L, 0L);
	ShwOKMsgBox(hWndFrame,
		    "Out of memory (%lu), thread %u",
		    rc,
		    (UINT)ThreadNr);

	WinDestroyMsgQueue(hThrMsgQ);
	WinTerminate(hThrAB);
#       if defined(EXCEPTQ)
	UninstallExceptq(&exRegRec);
#       endif
	_endthread();
    }

    memset(pDllTreeTable, 0x00, MaxNrOfDlls * sizeof(DLL_TREE_TABLE));

    pShowTableData[cShowTableData].Level = 0;
    pShowTableData[cShowTableData].SeqNr = 0;
    pStr = strrchr(CurrentFileName, '\\');

    if (pStr == NULL)
	*pShowTableData[cShowTableData].Name = 0;
    else
    {
	++pStr;
	strncpy(pShowTableData[cShowTableData].Name, pStr, MAX_NAME_LEN - 1);
	pShowTableData[cShowTableData].Name[MAX_NAME_LEN - 1] = 0;
    }

    pShowTableData[cShowTableData].PathFound = TRUE;
    strcpy(pShowTableData[cShowTableData].Path, CurrentFileName);
    pShowTableData[cShowTableData].SystemDll = FALSE;
    pShowTableData[cShowTableData].Loadable = TRUE;
    pShowTableData[cShowTableData].LoadError = 0;
    pShowTableData[cShowTableData].FreeError = 0;
    DosQPathInfo(CurrentFileName, FIL_STANDARD, &FileStatus,
		 sizeof(FILESTATUS));
    pShowTableData[cShowTableData].FileSize = FileStatus.cbFile;
    pShowTableData[cShowTableData].fdateLastWrite = FileStatus.fdateLastWrite;
    pShowTableData[cShowTableData].ftimeLastWrite = FileStatus.ftimeLastWrite;
    pShowTableData[cShowTableData].Dynamic = FALSE;

    ++cShowTableData;

    if (!_WalkPath(CurrentFileName, ErrorStr))
    {
	cShowTableData = 0;
	FileSelected = FALSE;
	cShowTableData = 0;
	memset(pShowTableData, 0x00, MaxNrOfDlls * sizeof(SHOW_TABLE_DATA));
	WinPostMsg(hWndProcess, WM_CLOSE, 0L, 0L);
	ShwOKMsgBox(hWndFrame, ErrorStr);

	DosFreeMem(pDllTreeTable);
	WinDestroyMsgQueue(hThrMsgQ);
	WinTerminate(hThrAB);
#       if defined(EXCEPTQ)
	UninstallExceptq(&exRegRec);
#       endif
	_endthread();
    }

    NrOfDlls = 0;

    while (pDllTreeTable[NrOfDlls].Name[0] != '\0')
	++NrOfDlls;

    if (strstr(CurrentFileName, ".EXE"))
	++NrOfDlls;

    DosFreeMem(pDllTreeTable);

#   if 0
    for (i = 0; i < cShowTableData; ++i)
	DebugPrintf ("%0.2u: %u %s %ld", i, pShowTableData[i].Level,
		     pShowTableData[i].Name,
		     pShowTableData[i].Dynamic);
#   endif

    /*--- clean up dynamic DLLs ---*/

    if (DynDlls)
    {
	DynDllOffset = 0;

	for (;;)
	{
	    /* Find next dynamic DLL --- */

	    while (DynDllOffset < cShowTableData &&
		    pShowTableData[DynDllOffset].Dynamic == FALSE)
		++DynDllOffset;

	    if (DynDllOffset < cShowTableData)
	    {
		/* Dynamic DLL found ! --- */

		RemoveLevel = pShowTableData[DynDllOffset].Level;
		NewLevel = FALSE;

		for (i = (USHORT) (DynDllOffset + 1); i < cShowTableData; ++i)
		{
		    if (pShowTableData[i].Level <= RemoveLevel)
			NewLevel = TRUE;

		    if (NewLevel &&
			    pShowTableData[i].Dynamic == FALSE &&
			    stricmp(pShowTableData[i].Path,
				    pShowTableData[DynDllOffset].Path) == 0)
			break;
		}

		if (i < cShowTableData)
		{
		    /* Dynamic DLL also statically loaded, move it --- */

		    StatDllOffset = i;
		    RemoveLevel = pShowTableData[DynDllOffset].Level;
		    InsertLevel = pShowTableData[StatDllOffset].Level;

		    /* insert new stat levels */

		    i = (USHORT) (DynDllOffset + 1);
		    j = (USHORT) (StatDllOffset + 1);

		    while (i < cShowTableData &&
			    pShowTableData[i].Level > RemoveLevel)
		    {
			memmove(&pShowTableData[j + 1],
				&pShowTableData[j],
			      (cShowTableData - j + 1) * sizeof(SHOW_TABLE_DATA));

			pShowTableData[j] = pShowTableData[i];
			pShowTableData[j].Level = (USHORT) (InsertLevel - RemoveLevel +
						   pShowTableData[i].Level);
			++cShowTableData;

			if (cShowTableData >= MaxNrOfDlls)
			{
			    WinPostMsg(hWndProcess, WM_CLOSE, 0L, 0L);
			    ShwOKMsgBox(hWndFrame,
					"Too many DLLs in tree, increase "
					"the 'Maximum number of DLLs' setting");

			    WinDestroyMsgQueue(hThrMsgQ);
			    WinTerminate(hThrAB);
#			    if defined(EXCEPTQ)
			    UninstallExceptq(&exRegRec);
#			    endif
			    _endthread();
			}

			++i;
			++j;
		    }

		    /* Remove old dyn levels --- */

		    do
		    {
			memmove(&pShowTableData[DynDllOffset],
				&pShowTableData[DynDllOffset + 1],
			(cShowTableData - DynDllOffset) * sizeof(SHOW_TABLE_DATA));

			--cShowTableData;
		    }
		    while (DynDllOffset < cShowTableData &&
			  pShowTableData[DynDllOffset].Level > RemoveLevel);
		}
		else
		    ++DynDllOffset;
	    }
	    else
		break;
	} // for

	DynDlls = FALSE;
    }

    FileSelected = TRUE;

    WinPostMsg(hWndProcess, WM_CLOSE, 0L, 0L);

    WinDestroyMsgQueue(hThrMsgQ);
    WinTerminate(hThrAB);

#   if defined(EXCEPTQ)
    UninstallExceptq(&exRegRec);
#   endif
    _endthread();
}

BOOL _WalkPath(PSZ pPgmName,
	       PSZ pErrorStr)
{
    USHORT NrOfEntries, i, j;
    APIRET rc;
    PIMPORT_TABLE_DATA pImportTable;

    WinSendMsg(hWndProcess, WUM_BUSY, 0L, 0L);

    ++Level;

    rc = DosAllocMem((PPVOID)&pImportTable,
		     PrfMaxNrOfImports * sizeof(IMPORT_TABLE_DATA),
		     PAG_READ | PAG_WRITE | PAG_COMMIT);
    if (rc)
    {
	sprintf(pErrorStr, "Error allocating memory (%lu)", rc);
	return (FALSE);
    }

    NrOfEntries = PrfMaxNrOfImports;

    if (!_GetImportsTable(pPgmName, pImportTable, &NrOfEntries, pErrorStr))
    {
	DosFreeMem(pImportTable);
	return (FALSE);
    }

    if (NrOfEntries > 0)
    {
	for (i = 0; i < NrOfEntries; ++i)
	{
	    pShowTableData[cShowTableData].Level = Level;
	    pShowTableData[cShowTableData].SeqNr = i;
	    strcpy(pShowTableData[cShowTableData].Name, pImportTable[i].Name);
	    pShowTableData[cShowTableData].PathFound = pImportTable[i].PathFound;
	    strcpy(pShowTableData[cShowTableData].Path, pImportTable[i].Path);
	    pShowTableData[cShowTableData].SystemDll = pImportTable[i].SystemDll;
	    pShowTableData[cShowTableData].Loadable = pImportTable[i].Loadable;
	    pShowTableData[cShowTableData].LoadError = pImportTable[i].LoadError;
	    strcpy(pShowTableData[cShowTableData].LoadErrorCause, pImportTable[i].LoadErrorCause);
	    pShowTableData[cShowTableData].FreeError = pImportTable[i].FreeError;
	    pShowTableData[cShowTableData].FileSize = pImportTable[i].FileSize;
	    pShowTableData[cShowTableData].fdateLastWrite = pImportTable[i].fdateLastWrite;
	    pShowTableData[cShowTableData].ftimeLastWrite = pImportTable[i].ftimeLastWrite;
	    pShowTableData[cShowTableData].Dynamic = pImportTable[i].Dynamic;

	    ++cShowTableData;

	    if (cShowTableData > MaxNrOfDlls)
	    {
		strcpy(pErrorStr, "Too many DLLs in tree, increase the 'Maximum number of DLLs' setting");
		DosFreeMem(pImportTable);
		return (FALSE);
	    }

	    j = 0;

	    while (pDllTreeTable[j].Name[0] != '\0')
	    {
		if (strcmp(pDllTreeTable[j].Name, pImportTable[i].Name) == 0)
		    break;

		++j;
	    }

	    if (pDllTreeTable[j].Name[0] == '\0')
	    {
		strcpy(pDllTreeTable[j].Name, pImportTable[i].Name);

		if (pImportTable[i].PathFound &&
			!pImportTable[i].SystemDll)
		{
		    if (!_WalkPath(pImportTable[i].Path, pErrorStr))
		    {
			DosFreeMem(pImportTable);
			return (FALSE);
		    }
		}
	    }
	}
    }

    DosFreeMem(pImportTable);

    printf("\n");
    --Level;

    return (TRUE);
}

static BOOL _GetImportsTable(PSZ pPgmName,
			     PIMPORT_TABLE_DATA pImportTable,
			     PUSHORT pNrOfEntries,
			     PSZ pErrorStr)
{
    FILESTATUS FileStatus;
    CHAR *pStr, FailName[CCHMAXPATH];
    APIRET rc;
    USHORT i;
    BOOL Bit32;

    WinSendMsg(hWndProcess, WUM_BUSY, 0L, 0L);

    if (!ExeHdrGetImports(pPgmName, TestLoadModuleUsed, Libpath,
			  &Bit32, pNrOfEntries, pImportTable, pErrorStr))
	return (FALSE);

    if (pDynTable)
    {
	for (i = 0; i < NrOfDynItems; ++i)
	{
	    pStr = strrchr(pDynTable[i].Path, '\\');

	    if (pStr == NULL)
		continue;

	    ++pStr;

	    strcpy(pImportTable[*pNrOfEntries].Name, pStr);
	    pImportTable[*pNrOfEntries].PathFound = TRUE;
	    strcpy(pImportTable[*pNrOfEntries].Path, pDynTable[i].Path);

	    rc = DosSearchPath(SEARCH_PATH | SEARCH_IGNORENETERRS,
			       SysDllPath,
			       pImportTable[*pNrOfEntries].Name,
			       (PUCHAR)FailName,
			       CCHMAXPATH);
	    if (rc)
		pImportTable[*pNrOfEntries].SystemDll = FALSE;
	    else
		pImportTable[*pNrOfEntries].SystemDll = TRUE;

	    pImportTable[*pNrOfEntries].Loadable = TRUE;
	    pImportTable[*pNrOfEntries].LoadError = 0L;
	    pImportTable[*pNrOfEntries].FreeError = 0L;

	    DosQPathInfo(pImportTable[*pNrOfEntries].Path, FIL_STANDARD,
			 &FileStatus, sizeof(FILESTATUS));
	    pImportTable[*pNrOfEntries].FileSize = FileStatus.cbFile;
	    pImportTable[*pNrOfEntries].fdateLastWrite = FileStatus.fdateLastWrite;
	    pImportTable[*pNrOfEntries].ftimeLastWrite = FileStatus.ftimeLastWrite;
	    pImportTable[*pNrOfEntries].Dynamic = TRUE;

	    ++(*pNrOfEntries);
	}

	DosFreeMem(pDynTable);
	pDynTable = (PDYN_TABLE_DATA)NULL;
	DynDlls = TRUE;
    }

    return (TRUE);
}

// The end
