
/***********************************************************************

chkdir.c - Change directory
$Id: chgdir.c,v 1.4 2013/04/03 00:11:07 Steven Exp $

Copyright (c) 2001, 2013 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

1994-06-19 AVB Baseline
2001-05-27 SHL Clean warnings
2001-05-31 SHL Correct window procedure arguments
2013-03-24 SHL Update for OpenWatcom compatibility

***********************************************************************/

#define EXTERN extern

#include "pmdll.h"

static ULONG SaveCurDisk;
static CHAR SaveCurDir[CCHMAXPATH];
static CHAR TfText[CCHMAXPATH];
static ULONG SelectedDrive;

static USHORT _FillDriveLb(HWND hDlg);
static USHORT _FillDirLb(HWND hDlg, ULONG CurDrive);
static VOID _ShowTextField(HWND hDlg);

MRESULT EXPENTRY ChangeCurDirWndProc(HWND hDlg, ULONG Msg,
				     MPARAM mp1, MPARAM mp2)

{
    SHORT DriveId, DirId;
    CHAR HlpStr[CCHMAXPATH];
    ULONG DriveMap;
    ULONG PathLen;
    APIRET rc;

    switch (Msg)
    {
    case WM_INITDLG:
/*--- save current drive/dir ---*/

	PathLen = CCHMAXPATH;

	if (DosQCurDisk(&SaveCurDisk, &DriveMap) ||
		DosQCurDir(SaveCurDisk, (PUCHAR)SaveCurDir, &PathLen))
	{
	    DosBeep(1000, 1000);
	    WinDismissDlg(hDlg, FALSE);
	    break;
	}

	if (PathLen == 0)
	    strcpy(SaveCurDir, "\\");
	else
	{
	    memmove(&SaveCurDir[1], SaveCurDir, strlen(SaveCurDir) + 1);
	    SaveCurDir[0] = '\\';
	}

	WinSendDlgItemMsg(hDlg, TID_CD_PATH, EM_SETTEXTLIMIT,
			  MPFROMSHORT(CCHMAXPATH), 0L);

	if (_FillDriveLb(hDlg))
	{
	    DosBeep(1000, 1000);
	    WinDismissDlg(hDlg, FALSE);
	    break;
	}

	CenterDialog(hWndFrame, hDlg);

	WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, LID_CD_DIRS));
	return ((MRESULT) TRUE);
	break;

    case WM_CONTROL:
	switch (SHORT1FROMMP(mp1))
	{
	case LID_CD_DRIVES:
	    if (SHORT2FROMMP(mp1) == LN_ENTER)
	    {
		DriveId =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_CD_DRIVES),
					    LM_QUERYSELECTION,
					    MPFROMSHORT(LIT_FIRST), NULL));

		WinSendMsg(WinWindowFromID(hDlg, LID_CD_DRIVES), LM_QUERYITEMTEXT,
			   MPFROM2SHORT(DriveId, CCHMAXPATH),
			   MPFROMP(HlpStr));

		SelectedDrive = (USHORT) HlpStr[1] - 64;
		DosSelectDisk(SelectedDrive);

		_FillDirLb(hDlg, SelectedDrive);
	    }
	    break;

	case LID_CD_DIRS:
	    if (SHORT2FROMMP(mp1) == LN_ENTER)
	    {
		DirId =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_CD_DIRS),
					    LM_QUERYSELECTION,
					    MPFROMSHORT(LIT_FIRST), NULL));

		WinSendMsg(WinWindowFromID(hDlg, LID_CD_DIRS), LM_QUERYITEMTEXT,
			   MPFROM2SHORT(DirId, CCHMAXPATH),
			   MPFROMP(HlpStr));

		if (strncmp(HlpStr, " <root", 6) == 0)
		    strcpy(HlpStr, "\\");

		if (DosChDir(HlpStr))
		    DosBeep(1000, 1000);

		_FillDirLb(hDlg, SelectedDrive);
	    }
	    break;
	}
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case DID_OK:
	    if (FileSelected)
	    {
		rc = WinMessageBox(HWND_DESKTOP, hDlg, "Refresh tree window?",
				   ApplTitle, 0, MB_YESNO |
				   MB_ICONQUESTION | MB_APPLMODAL);

		if (rc == MBID_YES)
		    WinPostMsg(hWndClient, WUM_START_REBUILD, 0L, 0L);
		else
		    WinInvalidateRect(hWndClient, NULL, FALSE);
	    }
	    else
		WinInvalidateRect(hWndClient, NULL, FALSE);

	    WinDismissDlg(hDlg, TRUE);
	    break;

	case DID_CANCEL:
	    if (DosSelectDisk(SaveCurDisk) ||
		    DosChDir(SaveCurDir))
		DosBeep(1000, 1000);

	    WinInvalidateRect(hWndClient, NULL, FALSE);
	    WinDismissDlg(hDlg, FALSE);
	    break;
	}
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
	break;
    }

    return NULL;
}

static USHORT _FillDriveLb(HWND hDlg)

{
    USHORT i, j;
    SHORT CurDriveId;
    ULONG LogDrives, Mask, CurrentDrive;
    CHAR DisplayTekst[5];

    if (DosQCurDisk(&CurrentDrive, &LogDrives))
	return (1);

    Mask = 0x0001;
    CurDriveId = 0;

    for (i = 0, j = 0; i < 26; ++i)
    {
	if (LogDrives & Mask)
	{
	    sprintf(DisplayTekst, "[%c:]", (char) (65 + i));
	    WinSendDlgItemMsg(hDlg, LID_CD_DRIVES, LM_INSERTITEM,
			      MPFROM2SHORT(LIT_END, 0),
			      MPFROMP(DisplayTekst));

	    if (i == CurrentDrive - 1)
		CurDriveId = j;

	    ++j;
	}

	Mask <<= 1;
    }

    WinSendDlgItemMsg(hDlg, LID_CD_DRIVES, LM_SELECTITEM,
		      MPFROMSHORT(CurDriveId),
		      MPFROMSHORT((BOOL) TRUE));
    SelectedDrive = CurrentDrive;

    if (_FillDirLb(hDlg, CurrentDrive))
	return (1);

    return (0);
}

static USHORT _FillDirLb(HWND hDlg, ULONG CurDrive)

{
    CHAR Path[CCHMAXPATH];
    ULONG PathLen, NrOfFiles;
    APIRET rc;
    FILEFINDBUF3 FileInfo;
    HDIR hDir;

    WinSendDlgItemMsg(hDlg, LID_CD_DIRS, LM_DELETEALL, 0L, 0L);
    WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_FILES, LM_DELETEALL, 0L, 0L);

    PathLen = CCHMAXPATH;

    if (DosQCurDir(CurDrive, (PUCHAR)Path, &PathLen))
	return (1);

    if (strlen(Path) == 0)
	sprintf(TfText, "%c:\\*.*", (CHAR) (CurDrive + 64));
    else
	sprintf(TfText, "%c:\\%s\\*.*", (CHAR) (CurDrive + 64), Path);

    _ShowTextField(hDlg);

    NrOfFiles = 1;
    hDir = HDIR_CREATE;

    rc = DosFindFirst2(TfText, &hDir, FILE_DIRECTORY | FILE_HIDDEN,
		       &FileInfo, sizeof(FILEFINDBUF3),
		       &NrOfFiles, FIL_STANDARD);

    while (!rc)
    {
	if (FileInfo.attrFile & FILE_DIRECTORY)
	{
	    if (strlen(TfText) != 6 &&
		    strcmp(FileInfo.achName, ".") == 0)
		sprintf(FileInfo.achName, " <root %c:>", (CHAR) (CurDrive + 64));

	    if (strlen(TfText) != 6 ||
		    FileInfo.achName[0] != '.')
	    {
		WinSendDlgItemMsg(hDlg, LID_CD_DIRS, LM_INSERTITEM,
				  MPFROM2SHORT(LIT_SORTASCENDING, 0),
				  MPFROMP(FileInfo.achName));
	    }
	}

	NrOfFiles = 1;
	rc = DosFindNext(hDir, &FileInfo, sizeof(FILEFINDBUF3), &NrOfFiles);
    }

    DosFindClose(hDir);

    if (rc != ERROR_NO_MORE_FILES)
	return (1);

    return (0);
}

static VOID _ShowTextField(HWND hDlg)

{
    CHAR HlpStr[CCHMAXPATH], ShowStr[81], *pStr;

    strcpy(HlpStr, TfText);
    strupr(HlpStr);
    if ((pStr = strrchr(HlpStr, '\\')) != NULL)
	pStr[0] = '\0';

    if (strlen(HlpStr) == 2)
	strcat(HlpStr, "\\");

    if (strlen(HlpStr) > 35)
    {
	CompressFilePath(HlpStr, ShowStr);
	WinSetDlgItemText(hDlg, TID_CD_PATH, ShowStr);
    }
    else
	WinSetDlgItemText(hDlg, TID_CD_PATH, HlpStr);

    return;
}
