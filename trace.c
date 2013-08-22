
/***********************************************************************

trace.c	- Trace DLL

Copyright (c) 2001, 2012 Steven Levine and Associates, Inc.
Copyright (c) 1996 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) trace.c - Ver 5, 04-Aug-12,00:18:50 $

1996-04-26 AVB Baseline
2001-05-30 SHL Rename vars
2001-05-31 SHL Correct window procedure arguments drop casts
2006-03-15 YD gcc compatibility mods
2012-04-22 SHL Rework exceptq support
2012-04-22 SHL Avoid OpenWatcom warnings
2012-08-04 SHL Sync with exceptq 7.x

***********************************************************************/

#define EXTERN extern

#define INCL_DOSPROCESS
#define INCL_DOSSESMGR
#define INCL_WINBUTTONS

#if defined(EXCEPTQ)
#define INCL_DOSEXCEPTIONS
#endif

#include "pmdll.h"

#if defined(EXCEPTQ)
#include "exceptq.h"
#endif

#ifdef __EMX__
#define XCPT_CONTINUE_SEARCH    0x00000000      /* exception not handled */
#else
#include <bsexcpt.h>
#endif
#include <process.h>

uDB_t DebugBuf;

#define MAX_EXE_PARM_LEN  100

static PID ProcessId;

static BOOL TraceRunning;
static CHAR ServerError[81];
static CHAR ExeParm[MAX_EXE_PARM_LEN + 1];
static BOOL ExclSysDlls;
static BOOL FirstTime = TRUE;

static VOID _ServerThread(PVOID Parm);
static APIRET _StartSession(PSZ ExeFile, PSZ ExeParm);

MRESULT EXPENTRY TraceDllWndProc(HWND hDlg, ULONG Msg, MPARAM mp1, MPARAM mp2)
{
    LONG Tid;
    CHAR DispStr[CCHMAXPATH];
    APIRET rc, MbRc;
    USHORT i, NrOfItems, NewNrOfDlls;
    HPOINTER pIcon;
    PSWP WindowInfo;
    CHAR FailName[CCHMAXPATH], *pStr, NoSys[2];
    ULONG DriveMap, CurDisk, PathLen;

    switch (Msg)
    {
    case WM_INITDLG:
	TraceRunning = FALSE;
	pIcon = WinLoadPointer(HWND_DESKTOP, NULLHANDLE, IID_PMDLL);
	WinSendMsg(hDlg, WM_SETICON, (MPARAM) pIcon, NULL);

	WinSetDlgItemText(hDlg, TID_TRACE_EXE, pShowTableData[0].Path);

	DosQueryCurrentDisk(&CurDisk, &DriveMap);
	FailName[0] = (UCHAR) (CurDisk + 64);
	FailName[1] = ':';
	FailName[2] = '\\';
	PathLen = CCHMAXPATH - 3;
	DosQCurDir(CurDisk, (PUCHAR)&FailName[3], &PathLen);
	strupr(FailName);
	WinSetDlgItemText(hDlg, TID_TRACE_DLL_STARTDIR, FailName);

	WinSendDlgItemMsg(hDlg, EID_TRACE_EXE_PARM, EM_SETTEXTLIMIT,
			  MPFROMSHORT(MAX_EXE_PARM_LEN), 0L);
	if (FirstTime)
	{
	    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
			    PRF_EXE_PARM_KEY, "", ExeParm, sizeof(ExeParm));
	    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
			       PRF_TRACE_NO_SYS, "1", NoSys, sizeof(NoSys));
	    ExclSysDlls = atoi(NoSys);
	    FirstTime = FALSE;
	}

	WinSetDlgItemText(hDlg, EID_TRACE_EXE_PARM, ExeParm);

	if (ExclSysDlls)
	{
	    WinSendDlgItemMsg(hDlg, RID_TRACE_DLL_SKIP_SYS, BM_SETCHECK,
			      MPFROMSHORT(TRUE), 0L);
	}

	WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_STOP), FALSE);
	WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_OK), FALSE);

	CenterDialog(hWndFrame, hDlg);

	WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, LID_TRACE_DLL));
	return ((MRESULT) TRUE);

    case WM_MINMAXFRAME:
	WindowInfo = PVOIDFROMMP(mp1);

	if (WindowInfo -> fl & SWP_RESTORE)
	{
	    WinShowWindow(WinWindowFromID(hDlg, BID_TRACE_START), TRUE);
	    WinShowWindow(WinWindowFromID(hDlg, GID_TRACE), TRUE);
	    WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, LID_TRACE_DLL));
	}
	else if (WindowInfo -> fl & SWP_MINIMIZE)
	{
	    WinShowWindow(WinWindowFromID(hDlg, GID_TRACE), FALSE);
	    WinShowWindow(WinWindowFromID(hDlg, BID_TRACE_START), FALSE);
	}
	break;

    case WUM_SERVERERROR:
	WinSendMsg(hDlg, WM_COMMAND, MPFROMSHORT(BID_TRACE_STOP), 0L);

	WinMessageBox(HWND_DESKTOP, hDlg, ServerError,
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
	break;

    case WUM_NEW_ENTRY:
/*--- get name ---*/

	rc = DosQueryModuleName(LONGFROMMP(mp1), sizeof(DispStr), DispStr);

	if (!rc)
	    strupr(DispStr);

	if (rc || strstr(DispStr, ".DLL") == NULL)
	    break;

/*--- already statically loaded ? ---*/

	for (i = 1; i < cShowTableData; ++i)
	{
	    if (stricmp(pShowTableData[i].Path, DispStr) == 0)
		break;
	}

	if (i < cShowTableData)
	    break;

	if (ExclSysDlls)
	{
/*--- path in sys dll path  ? ---*/

	    pStr = strrchr(DispStr, '\\');

	    if (pStr)
	    {
		++pStr;
		rc = DosSearchPath(SEARCH_PATH | SEARCH_IGNORENETERRS,
				   SysDllPath,
				   pStr,
				   (PUCHAR)FailName,
				   CCHMAXPATH);

		if (rc == NO_ERROR)
		    break;
	    }
	}

/*--- already in listbox ? ---*/

	if (StrInListbox(hDlg, LID_TRACE_DLL, DispStr))
	    break;

/*--- add to listbox ---*/

	WinSendDlgItemMsg(hDlg, LID_TRACE_DLL, LM_INSERTITEM,
			  MPFROMSHORT(LIT_END), MPFROMP(DispStr));
	break;

    case WUM_SERVEREND:
	TraceRunning = FALSE;
	WinSendMsg(hDlg, WM_COMMAND, MPFROMSHORT(BID_TRACE_STOP), 0L);
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case DID_CANCEL:
	    WinShowWindow(hWndFrame, TRUE);
	    WinDestroyWindow(hWndTrace);
	    hWndTrace = NULLHANDLE;
	    SetSwitchWnd(hWndFrame);
	    break;

	case BID_TRACE_OK:
	    NrOfItems =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_TRACE_DLL),
					LM_QUERYITEMCOUNT, NULL, NULL));

	    if (NrOfItems == 0)
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, "Internal error",
			      ApplTitle, 0, MB_OK |
			      MB_ICONQUESTION | MB_APPLMODAL);
		break;
	    }

	    NrOfDynItems = NrOfItems;

	    rc = DosAllocMem((PPVOID) & pDynTable,
			     NrOfItems * sizeof(DYN_TABLE_DATA),
			     PAG_READ | PAG_WRITE | PAG_COMMIT);
	    if (rc)
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, "Out of memory",
			      ApplTitle, 0, MB_OK |
			      MB_ICONQUESTION | MB_APPLMODAL);
		break;
	    }

	    for (i = 0; i < NrOfItems; ++i)
	    {
		WinSendMsg(WinWindowFromID(hDlg, LID_TRACE_DLL),
			   LM_QUERYITEMTEXT, MPFROM2SHORT(i, CCHMAXPATH),
			   MPFROMP(pDynTable[i].Path));
	    }

	    WinShowWindow(hWndFrame, TRUE);
	    WinDestroyWindow(hWndTrace);
	    hWndTrace = NULLHANDLE;
	    SetSwitchWnd(hWndFrame);

	    WinPostMsg(hWndFrame, WUM_START_REBUILD, 0L, 0L);
	    break;

	case BID_TRACE_START:
/*--- delete old trace data ---*/

	    for (i = 0; i < cShowTableData; ++i)
	    {
		if (pShowTableData[i].Dynamic)
		    break;
	    }

	    NewNrOfDlls = i;

	    for (; i < cShowTableData; ++i)
		memset(&pShowTableData[i], 0x00, sizeof(SHOW_TABLE_DATA));

	    cShowTableData = NewNrOfDlls;

/*--- start new trace ---*/

	    WinSendMsg(WinWindowFromID(hDlg, LID_TRACE_DLL),
		       LM_DELETEALL, NULL, NULL);

	    WinQueryDlgItemText(hDlg, EID_TRACE_EXE_PARM,
				MAX_EXE_PARM_LEN + 1, ExeParm);
	    StripSpaces(ExeParm);
	    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				  PRF_EXE_PARM_KEY, ExeParm);

	    ExclSysDlls = SHORT1FROMMR
		(WinSendDlgItemMsg(hDlg, RID_TRACE_DLL_SKIP_SYS,
				   BM_QUERYCHECK, 0L, 0L));

	    if (ExclSysDlls)
	    {
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_TRACE_NO_SYS, "1");
	    }
	    else
	    {
		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_TRACE_NO_SYS, "0");
	    }

	    rc = _StartSession(pShowTableData[0].Path, ExeParm);
	    if (rc)
	    {
		sprintf(ErrorStr, "Start session failed (%lu)", rc);
		WinMessageBox(HWND_DESKTOP, hDlg, ErrorStr,
			      ApplTitle, 0, MB_OK |
			      MB_ICONQUESTION | MB_APPLMODAL);
		break;
	    }

	    Tid = _beginthread(_ServerThread,
			       NULL,
			       PROCESS_STACK_SIZE,
			       (PVOID) hDlg);

	    if (Tid == -1)
	    {
		WinMessageBox(HWND_DESKTOP, hWndFrame, "Could not start thread",
		      ApplTitle, 0, MB_OK | MB_ICONQUESTION | MB_APPLMODAL);
		DosKillProcess(DKP_PROCESS, ProcessId);
		break;
	    }

	    TraceRunning = TRUE;
	    WinEnableWindow(WinWindowFromID(hDlg, EID_TRACE_EXE_PARM), FALSE);
	    WinEnableWindow(WinWindowFromID(hDlg, RID_TRACE_DLL_SKIP_SYS), FALSE);
	    WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_STOP), TRUE);
	    WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_START), FALSE);
	    WinEnableWindow(WinWindowFromID(hDlg, DID_CANCEL), FALSE);
	    WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_OK), FALSE);
	    break;

	case BID_TRACE_STOP:
	    if (TraceRunning)
	    {
		MbRc = WinMessageBox(HWND_DESKTOP, hDlg,
				     "Kill the application ?",
				     ApplTitle, 0, MB_OKCANCEL |
				     MB_ICONQUESTION | MB_APPLMODAL);

		if (MbRc != MBID_OK)
		    break;

		DosKillProcess(DKP_PROCESS, ProcessId);
	    }

	    TraceRunning = FALSE;
	    WinEnableWindow(WinWindowFromID(hDlg, EID_TRACE_EXE_PARM), TRUE);
	    WinEnableWindow(WinWindowFromID(hDlg, RID_TRACE_DLL_SKIP_SYS), TRUE);
	    WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_STOP), FALSE);
	    WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_START), TRUE);
	    WinEnableWindow(WinWindowFromID(hDlg, DID_CANCEL), TRUE);

	    NrOfItems =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_TRACE_DLL),
					LM_QUERYITEMCOUNT, NULL, NULL));
	    if (NrOfItems > 0)
		WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_OK), TRUE);
	    else
		WinEnableWindow(WinWindowFromID(hDlg, BID_TRACE_OK), FALSE);
	    break;
	}
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
    }

    return NULL;
}

static VOID _ServerThread(PVOID Parm)
{
#ifdef EXCEPTQ
    EXCEPTIONREGISTRATIONRECORD exRegRec;
#endif
    HAB hThrAB;
    HMQ hThrMsgQ;
    APIRET rc;
    HWND hDlg;

#ifdef EXCEPTQ
    LoadExceptq(&exRegRec, NULL, NULL);
#endif
    hDlg = (HWND) Parm;

    hThrAB = WinInitialize(0);
    hThrMsgQ = WinCreateMsgQueue(hThrAB, 0);

    memset(&DebugBuf, 0x00, sizeof(DebugBuf));
    DebugBuf.Cmd = DBG_C_Connect;
    DebugBuf.Pid = ProcessId;
    DebugBuf.Tid = 0;
    DebugBuf.Value = 1;

    rc = DosDebug(&DebugBuf);

    if (DebugBuf.Cmd != DBG_N_Success)
    {
	sprintf(ServerError, "Trace failed (%lu, %ld, %ld)",
		rc, (LONG) DebugBuf.Cmd, (LONG) DebugBuf.Value);
	WinPostMsg(hDlg, WUM_SERVERERROR, 0L, 0L);
	WinDestroyMsgQueue(hThrMsgQ);
	WinTerminate(hThrAB);
#	if defined(EXCEPTQ)
	UninstallExceptq(&exRegRec);
#	endif
	_endthread();
    }

    memset(&DebugBuf, 0x00, sizeof(DebugBuf));

    DebugBuf.Cmd = DBG_C_Go;
    DebugBuf.Pid = ProcessId;

    do
    {
	rc = DosDebug(&DebugBuf);

	if (rc || DebugBuf.Cmd == DBG_N_Error)
	{
	    if (DebugBuf.Value == ERROR_INVALID_PROCID)
		WinPostMsg(hDlg, WUM_SERVEREND, 0L, 0L);
	    else
	    {
		sprintf(ServerError, "Trace error (%lu, %ld, %ld, %ld)",
			rc, (LONG) DebugBuf.Cmd, (LONG) DebugBuf.Value, (LONG) DebugBuf.EAX);
		WinPostMsg(hDlg, WUM_SERVERERROR, 0L, 0L);

		DebugBuf.Cmd = DBG_C_Term;
		DebugBuf.Pid = ProcessId;
		DosDebug(&DebugBuf);
	    }

	    WinDestroyMsgQueue(hThrMsgQ);
	    WinTerminate(hThrAB);
#	    if defined(EXCEPTQ)
	    UninstallExceptq(&exRegRec);
#	    endif
	    _endthread();
	}

	if (DebugBuf.Cmd == DBG_N_ModuleLoad)
	{
	    WinSendMsg(hDlg, WUM_NEW_ENTRY, MPFROMLONG(DebugBuf.Value), 0L);

	    memset(&DebugBuf, 0x00, sizeof(DebugBuf));
	    DebugBuf.Cmd = DBG_C_Go;
	    DebugBuf.Pid = ProcessId;
	}
	else if (DebugBuf.Cmd == DBG_N_Exception)
	{
	    DebugBuf.Cmd = DBG_C_Continue;
	    DebugBuf.Pid = ProcessId;
	    DebugBuf.Value = XCPT_CONTINUE_SEARCH;
	}
	else
	{
	    memset(&DebugBuf, 0x00, sizeof(DebugBuf));
	    DebugBuf.Cmd = DBG_C_Go;
	    DebugBuf.Pid = ProcessId;
	    // DebugPrintf ("Notification: %ld", (LONG) DebugBuf.Cmd);
	}
    }
    while (DebugBuf.Cmd != DBG_N_ProcTerm);

    WinPostMsg(hDlg, WUM_SERVEREND, 0L, 0L);

    DebugBuf.Cmd = DBG_C_Term;
    DebugBuf.Pid = ProcessId;
    DosDebug(&DebugBuf);

    WinDestroyMsgQueue(hThrMsgQ);
    WinTerminate(hThrAB);

#   if defined(EXCEPTQ)
    UninstallExceptq(&exRegRec);
#   endif
    _endthread();
}

static APIRET _StartSession(PSZ ExeFile, PSZ ExeParm)
{
    STARTDATA StartData;
    APIRET rc;
    CHAR ObjectBuffer[100];
    ULONG SessionId;

    memset(&StartData, 0x00, sizeof(StartData));

    StartData.Length = sizeof(STARTDATA);
    StartData.Related = SSF_RELATED_CHILD;
    StartData.FgBg = SSF_FGBG_FORE;
    StartData.TraceOpt = SSF_TRACEOPT_TRACE;
    StartData.PgmTitle = NULL;
    StartData.PgmName = ExeFile;
    if (strlen(ExeParm))
	StartData.PgmInputs = (PUCHAR)ExeParm;
    else
	StartData.PgmInputs = NULL;
    StartData.TermQ = NULL;
    StartData.Environment = NULL;
    StartData.InheritOpt = SSF_INHERTOPT_PARENT;
    StartData.IconFile = NULL;
    StartData.PgmHandle = NULLHANDLE;
    StartData.InitXPos = 100;
    StartData.InitYPos = 100;
    StartData.InitXSize = 450;
    StartData.InitYSize = 300;
    StartData.SessionType = SSF_TYPE_DEFAULT;
    StartData.PgmControl = SSF_CONTROL_SETPOS;
    StartData.ObjectBuffer = ObjectBuffer;
    StartData.ObjectBuffLen = sizeof(ObjectBuffer);

    CursorWait();

    rc = DosStartSession(&StartData, &SessionId, &ProcessId);

    CursorNormal();

    return (rc);
}
