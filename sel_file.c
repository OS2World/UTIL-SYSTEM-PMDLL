
/***********************************************************************

sel_file.c - Select file
$Id: sel_file.c,v 1.6 2016/02/27 04:43:00 Steven Exp $

Copyright (c) 2001, 2013 Steven Levine and Associates, Inc.
Copyright (c) 1994 Arthur Van Beek
All rights reserved.

1994-06-18 AVB Baseline
2001-05-31 SHL Correct window proc arguments, drop casts
2012-04-22 SHL Avoid OpenWatcom warnings
2013-03-24 SHL Enhance file open to include all executables

***********************************************************************/

#define EXTERN extern

#include "pmdll.h"

static ULONG SaveCurDisk;
static CHAR SaveCurDir[CCHMAXPATH];
static ULONG CurDisk;
static CHAR CurDir[CCHMAXPATH];
static CHAR TfText[CCHMAXPATH];		// Used by DosFind...
static ULONG SelectedDrive;

static USHORT _FillDriveLb(HWND hDlg);
static USHORT _FillDirLb(HWND hDlg, ULONG CurDrive);
static USHORT _FillFileLb(HWND hDlg);
static VOID _ShowTextField(HWND hDlg);

static CHAR FileNameLoaded[CCHMAXPATH];

MRESULT EXPENTRY SelectFileWndProc(HWND hDlg, ULONG Msg,
				   MPARAM mp1, MPARAM mp2)
{
    SHORT DriveId, DirId, FileId;
    CHAR HlpStr[CCHMAXPATH], *pStr, Drive[_MAX_DRIVE];
    ULONG DriveMap;
    ULONG PathLen;

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

	if (strlen(CurDir) > 0)
	{
	    DosSelectDisk(CurDisk);
	    DosChDir(CurDir);
	}
	else if (strlen(CurrentFileName) > 0)
	{
	    strupr(CurrentFileName);
	    _splitpath(CurrentFileName, Drive, CurDir, HlpStr, HlpStr);
	    if (strlen(CurDir) > 1)
		CurDir[strlen(CurDir) - 1] = '\0';
	    CurDisk = (USHORT) Drive[0] - 64;

	    DosSelectDisk(CurDisk);
	    DosChDir(CurDir);
	}

	strcpy(FileNameLoaded, CurrentFileName);
	WinSendDlgItemMsg(hDlg, TID_SELECT_FILE_PATH, EM_SETTEXTLIMIT,
			  MPFROMSHORT(CCHMAXPATH), 0L);

	if (_FillDriveLb(hDlg))
	{
	    DosBeep(1000, 1000);
	    WinDismissDlg(hDlg, FALSE);
	    break;
	}

	CenterDialog(hWndFrame, hDlg);

	WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, LID_SELECT_FILE_FILES));
	return ((MRESULT) TRUE);
	break;

    case WM_CONTROL:
	switch (SHORT1FROMMP(mp1))
	{
	case LID_SELECT_FILE_DRIVES:
	    if (SHORT2FROMMP(mp1) == LN_ENTER)
	    {
		DriveId =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SELECT_FILE_DRIVES),
					    LM_QUERYSELECTION,
					    MPFROMSHORT(LIT_FIRST), NULL));

		WinSendMsg(WinWindowFromID(hDlg, LID_SELECT_FILE_DRIVES), LM_QUERYITEMTEXT,
			   MPFROM2SHORT(DriveId, CCHMAXPATH),
			   MPFROMP(HlpStr));

		SelectedDrive = (USHORT) HlpStr[1] - 64;
		DosSelectDisk(SelectedDrive);

		_FillDirLb(hDlg, SelectedDrive);
	    }
	    break;

	case LID_SELECT_FILE_DIRS:
	    if (SHORT2FROMMP(mp1) == LN_ENTER)
	    {
		DirId =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SELECT_FILE_DIRS),
					    LM_QUERYSELECTION,
					    MPFROMSHORT(LIT_FIRST), NULL));

		WinSendMsg(WinWindowFromID(hDlg, LID_SELECT_FILE_DIRS), LM_QUERYITEMTEXT,
			   MPFROM2SHORT(DirId, CCHMAXPATH),
			   MPFROMP(HlpStr));

		if (strncmp(HlpStr, " <root", 6) == 0)
		    strcpy(HlpStr, "\\");

		if (DosChDir(HlpStr))
		    DosBeep(1000, 1000);

		_FillDirLb(hDlg, SelectedDrive);
	    }
	    break;

	case LID_SELECT_FILE_FILES:
	    if (SHORT2FROMMP(mp1) == LN_ENTER)
		WinPostMsg(hDlg, WM_COMMAND, MPFROMSHORT(DID_OK), 0L);
	    break;
	}
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case DID_OK:
	    // Get selection
	    FileId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SELECT_FILE_FILES),
					LM_QUERYSELECTION,
					MPFROMSHORT(LIT_FIRST), NULL));

	    if (FileId == LIT_NONE)
		break;			// Oops - nothing selected

	    // Build full path
	    DosQCurDisk(&CurDisk, &DriveMap);
	    PathLen = CCHMAXPATH;
	    DosQCurDir(CurDisk, (PUCHAR)CurDir, &PathLen);
	    HlpStr[0] = (CHAR) (64 + CurDisk);
	    HlpStr[1] = ':';
	    HlpStr[2] = '\\';
	    if (strlen(CurDir) > 0)
	    {
		strcpy(&HlpStr[3], CurDir);
		strcat(HlpStr, "\\");
	    }
	    else
		HlpStr[3] = '\0';

	    if ((pStr = strrchr(HlpStr, '\\')) == NULL)
		break;

	    ++pStr;

	    // Append filename to path
	    WinSendMsg(WinWindowFromID(hDlg, LID_SELECT_FILE_FILES), LM_QUERYITEMTEXT,
		       MPFROM2SHORT(FileId, CCHMAXPATH), MPFROMP(pStr));

	    strcpy(CurrentFileName, HlpStr);

	    PathLen = CCHMAXPATH;
	    DosQCurDisk(&CurDisk, &DriveMap);
	    DosQCurDir(CurDisk, (PUCHAR)CurDir, &PathLen);

	    if (PathLen == 0)
		strcpy(CurDir, "\\");
	    else
	    {
		memmove(&CurDir[1], CurDir, strlen(CurDir) + 1);
		CurDir[0] = '\\';
	    }

	    // Restore orginal current drive/directory
	    if (DosSelectDisk(SaveCurDisk) ||
		    DosChDir(SaveCurDir))
		DosBeep(1000, 1000);

	    WinDismissDlg(hDlg, TRUE);
	    break;

	case DID_CANCEL:
	    PathLen = CCHMAXPATH;
	    DosQCurDisk(&CurDisk, &DriveMap);
	    DosQCurDir(CurDisk, (PUCHAR)CurDir, &PathLen);

	    if (PathLen == 0)
		strcpy(CurDir, "\\");
	    else
	    {
		memmove(&CurDir[1], CurDir, strlen(CurDir) + 1);
		CurDir[0] = '\\';
	    }

	    if (DosSelectDisk(SaveCurDisk) ||
		    DosChDir(SaveCurDir))
		DosBeep(1000, 1000);

	    WinDismissDlg(hDlg, FALSE);
	    break;
	}
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
	break;
    }

    return (NULL);
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
	    WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_DRIVES, LM_INSERTITEM,
			      MPFROM2SHORT(LIT_END, 0),
			      MPFROMP(DisplayTekst));

	    if (i == CurrentDrive - 1)
		CurDriveId = j;

	    ++j;
	}

	Mask <<= 1;
    }

    WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_DRIVES, LM_SELECTITEM,
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

    WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_DIRS, LM_DELETEALL, 0L, 0L);
    WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_FILES, LM_DELETEALL, 0L, 0L);
    WinEnableWindow(WinWindowFromID(hDlg, DID_OK), FALSE);

    PathLen = CCHMAXPATH;

    if (DosQCurDir(CurDrive, (PUCHAR)Path, &PathLen))
	return (1);

    // Build wildcard
    if (strlen(Path) == 0)
	sprintf(TfText, "%c:\\*", (CHAR) (CurDrive + 'A' - 1));
    else
	sprintf(TfText, "%c:\\%s\\*", (CHAR) (CurDrive + 'A' - 1), Path);

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
		WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_DIRS, LM_INSERTITEM,
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

    if (_FillFileLb(hDlg))
	return (1);

    return (0);
}

static USHORT _FillFileLb(HWND hDlg)
{
    APIRET rc;
    FILEFINDBUF3 FileInfo;
    ULONG NrOfFiles;
    SHORT Pos;
    HDIR hDir;
    MRESULT MsgRes;
    BOOL FilesFound;
    BOOL FileSelected;
    BOOL addToListbox;
    PCHAR pFileNameLoaded;
    PSZ pTfTextName;

    pFileNameLoaded = strrchr(FileNameLoaded, '\\');
    if (pFileNameLoaded == NULL)
	pFileNameLoaded = FileNameLoaded;
    else
	++pFileNameLoaded;

    WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_FILES, LM_DELETEALL, 0L, 0L);

    FilesFound = FALSE;
    FileSelected = FALSE;

    // Find last backslash in current wildcard
    pTfTextName = strrchr(TfText, '\\');
    if (pTfTextName)
      pTfTextName++;
    else
      pTfTextName = TfText;

    NrOfFiles = 1;
    hDir = HDIR_CREATE;
    strcpy(pTfTextName, "*");

    rc = DosFindFirst2(TfText, &hDir, FILE_NORMAL | FILE_HIDDEN |
		       FILE_SYSTEM | FILE_READONLY | FILE_ARCHIVED,
		       &FileInfo, sizeof(FILEFINDBUF3),
		       &NrOfFiles, FIL_STANDARD);

    while (!rc)
    {
	// 2013-03-24 SHL FIXME to be finished
	// If not DLL or EXE check signature
#	define OFFSET_EXE_HEADER 60		// offset to NE/LX header in MZ header
	CHAR Buffer[256];
	ULONG BytesRead;
	HFILE hFile;
	ULONG ul;
	ULONG Action;
	PSZ psz;
	psz = strrchr(FileInfo.achName, '.');
	// Assume OK if known extension - otherwise check header
	addToListbox = psz && (stricmp(psz, ".EXE") == 0 || stricmp(psz, ".DLL") == 0);
	if (!addToListbox) {
	    // Need to check content
	    strcpy(pTfTextName, FileInfo.achName);
	    rc = DosOpen(TfText, &hFile, &Action, 0L,
			 FILE_NORMAL, FILE_OPEN,
			 OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE, 0L);
	    if (!rc) {
		rc = DosRead(hFile, Buffer, sizeof(Buffer), &BytesRead);
		DosClose(hFile);
		if (!rc && BytesRead == sizeof(Buffer)) {
		    ul = strncmp(Buffer, "MZ", 2) ? 0 : *(ULONG*)(Buffer + OFFSET_EXE_HEADER);
		    // 2016-02-26 SHL Avoid going beyond end of buffer
		    addToListbox = ul < sizeof(Buffer) - 4 && (strncmp(&Buffer[ul], "LX", 2) == 0 || strncmp(&Buffer[ul], "NE", 2) == 0);
		}
	    }
	}

	if (addToListbox) {
	    FilesFound = TRUE;

	    MsgRes = WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_FILES, LM_INSERTITEM,
				       MPFROM2SHORT(LIT_SORTASCENDING, 0),
				       MPFROMP(FileInfo.achName));

	    if (strcmp(pFileNameLoaded, FileInfo.achName) == 0)
	    {
		Pos = SHORT1FROMMR(MsgRes);

		WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_FILES, LM_SELECTITEM,
				  MPFROMSHORT(Pos), MPFROMSHORT(TRUE));
		FileSelected = TRUE;
	    }
	}

	NrOfFiles = 1;
	rc = DosFindNext(hDir, &FileInfo, sizeof(FILEFINDBUF3), &NrOfFiles);
    } // while

    DosFindClose(hDir);

    if (rc != ERROR_NO_MORE_FILES)
	return (1);

    if (FileSelected)
	WinEnableWindow(WinWindowFromID(hDlg, DID_OK), TRUE);
    else if (FilesFound)
    {
	WinSendDlgItemMsg(hDlg, LID_SELECT_FILE_FILES, LM_SELECTITEM,
			  MPFROMSHORT(0), MPFROMSHORT(TRUE));
	WinEnableWindow(WinWindowFromID(hDlg, DID_OK), TRUE);
    }
    else
	WinEnableWindow(WinWindowFromID(hDlg, DID_OK), FALSE);

    return (0);
}

static VOID _ShowTextField(HWND hDlg)
{
    CHAR HlpStr[CCHMAXPATH], ShowStr[81], *pStr;

    strcpy(HlpStr, TfText);
    strupr(HlpStr);
    pStr = strrchr(HlpStr, '\\');
    if (pStr)
	pStr[0] = '\0';

    if (strlen(HlpStr) == 2)
	strcat(HlpStr, "\\");

    if (strlen(HlpStr) > 35)
    {
	CompressFilePath(HlpStr, ShowStr);
	WinSetDlgItemText(hDlg, TID_SELECT_FILE_PATH, ShowStr);
    }
    else
	WinSetDlgItemText(hDlg, TID_SELECT_FILE_PATH, HlpStr);

    return;
}
