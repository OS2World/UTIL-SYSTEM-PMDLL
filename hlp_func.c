
/***********************************************************************

hlp_func.c - Helper functions

Copyright (c) 2001, 2012 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) hlp_func.c - Ver 9, 13-May-12,19:41:44 $

1995-04-07 AVB Baseline
2001-05-14 SHL FDateToText add
2001-05-14 SHL FTimeToText add
2001-05-30 SHL ComposeTreeDetailLine: Rework to support long names
2001-05-31 SHL Drop casts
2001-06-01 SHL ComposeTreeDetailLine: correct width logic
2001-06-26 SHL Add generic message box routines
2001-08-28 SHL More generic message box routines
2006-03-15 YD gcc compatibility mods
2012-04-22 SHL Avoid OpenWatcom warnings
2012-05-12 SHL Support extended LIBPATH

***********************************************************************/

#include "pmdll.h"

//=== String utilities ===

BOOL EmptyField(PSZ Field)
{
    int i;

    for (i = 0; i < (int) strlen(Field); ++i)
    {
	if (Field[i] != ' ')
	    return (FALSE);
    }

    return (TRUE);
}

//=== FDateToString ===

VOID FDateToString(PSZ psz, FDATE *pFDATE)
{
    switch (CountryInfo.fsDateFmt)
    {
    case 1: // dd/mm/yy
	sprintf(psz, "%0.2u%s%0.2u%s%0.2u",
		(USHORT)pFDATE -> day,
		CountryInfo.szDateSeparator,
		(USHORT)pFDATE -> month,
		CountryInfo.szDateSeparator,
		((USHORT)pFDATE -> year + 80) % 100);
	break;
    case 2: // yy/mm/dd
	sprintf(psz, "%0.2u%s%0.2u%s%0.2u",
		(USHORT)pFDATE -> day,
		CountryInfo.szDateSeparator,
		(USHORT)pFDATE -> month,
		CountryInfo.szDateSeparator,
		((USHORT)pFDATE -> year + 80) % 100);
	break;
    default: // mm/dd/yy
	sprintf(psz, "%0.2u%s%0.2u%s%0.2u",
		(USHORT)pFDATE -> month,
		CountryInfo.szDateSeparator,
		(USHORT)pFDATE -> day,
		CountryInfo.szDateSeparator,
		((USHORT)pFDATE -> year + 80) % 100);
    }

} // FDateToString

//=== FDateTimeToString ===

VOID FDateTimeToString(PSZ psz, FDATE *pFDATE, FTIME *pFTIME)
{
    FDateToString(psz, pFDATE);

    psz += strlen(psz);
    *psz++ = ' ';

    FTimeToString(psz, pFTIME);

} // FDateTimeToString

//=== FTimeToString ===

VOID FTimeToString(PSZ psz, FTIME *pFTIME)
{
    switch (CountryInfo.fsDateFmt)
    {
    case 1: // 24 hour
	sprintf(psz, "%0.2u%s%0.2u%s%0.2u",
		(USHORT)pFTIME -> hours,
		CountryInfo.szTimeSeparator,
		(USHORT)pFTIME -> minutes,
		CountryInfo.szTimeSeparator,
		(USHORT)pFTIME -> twosecs * 2);
	break;
    default: // 12 hour
	sprintf(psz, "%0.2u%s%0.2u%s%0.2u %cm",
		(USHORT)(pFTIME -> hours) % 12,
		CountryInfo.szTimeSeparator,
		(USHORT)pFTIME -> minutes,
		CountryInfo.szTimeSeparator,
		(USHORT)pFTIME -> twosecs * 2,
		pFTIME -> hours >= 12 ? 'p' : 'a');
    }

} // FDateToString

VOID StripSpaces(PSZ Field)
{
    CHAR Hulpveld[256];
    INT i, j;

    if (EmptyField(Field))
    {
	Field[0] = '\0';
	return;
    }

    i = strlen(Field) - 1;

    while (i > 0 && Field[i] == ' ')
    {
	Field[i] = '\0';
	--i;
    }

    while (Field[0] == ' ')
    {
	for (i = 0; i < (int) strlen(Field); ++i)
	    Field[i] = Field[i + 1];
    }

    i = 0;
    j = 0;
    while (Field[i])
    {
	if (Field[i] == ' ')
	{
	    if (Field[i - 1] != ' ')
	    {
		Hulpveld[j] = Field[i];
		++j;
	    }
	}
	else
	{
	    Hulpveld[j] = Field[i];
	    ++j;
	}

	++i;
    }

    Hulpveld[j] = '\0';
    strcpy(Field, Hulpveld);

}

//=== Cursor control ===

VOID CursorWait(VOID)
{
    WinSetPointer(HWND_DESKTOP, WinQuerySysPointer(HWND_DESKTOP,
						   SPTR_WAIT, FALSE));
}

VOID CursorNormal(VOID)
{
    WinSetPointer(HWND_DESKTOP, WinQuerySysPointer(HWND_DESKTOP,
						   SPTR_ARROW, FALSE));
}

//=== Menu control ===

VOID DisableMenuItem(HWND hWndFrame, SHORT MenuItemId)
{
    HWND hWndActionBar;

    hWndActionBar = WinWindowFromID(hWndFrame, FID_MENU);

    WinSendMsg(hWndActionBar, MM_SETITEMATTR,
	       MPFROM2SHORT(MenuItemId, TRUE),
	       MPFROM2SHORT(MIA_DISABLED, MIA_DISABLED));
}

VOID EnableMenuItem(HWND hWndFrame, SHORT MenuItemId)
{
    HWND hWndActionBar;

    hWndActionBar = WinWindowFromID(hWndFrame, FID_MENU);

    WinSendMsg(hWndActionBar, MM_SETITEMATTR,
	       MPFROM2SHORT(MenuItemId, TRUE),
	       MPFROM2SHORT(MIA_DISABLED, ~MIA_DISABLED));
}

VOID CheckMenuItem(HWND hWndFrame, SHORT MenuItemId)
{
    HWND hWndActionBar;

    hWndActionBar = WinWindowFromID(hWndFrame, FID_MENU);

    WinSendMsg(hWndActionBar, MM_SETITEMATTR,
	       MPFROM2SHORT(MenuItemId, TRUE),
	       MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
}

VOID UncheckMenuItem(HWND hWndFrame, SHORT MenuItemId)
{
    HWND hWndActionBar;

    hWndActionBar = WinWindowFromID(hWndFrame, FID_MENU);

    WinSendMsg(hWndActionBar, MM_SETITEMATTR,
	       MPFROM2SHORT(MenuItemId, TRUE),
	       MPFROM2SHORT(MIA_CHECKED, ~MIA_CHECKED));
}

//=== Dialog control ===

VOID CenterDialog(HWND hWnd, HWND hDlg)
{
    SWP Swp, SwpFrame, SwpDt;
    LONG x, y;

    WinQueryWindowPos(hDlg, &Swp);
    WinQueryWindowPos(hWnd, &SwpFrame);
    WinQueryWindowPos(HWND_DESKTOP, &SwpDt);

    if ((SwpFrame.fl & SWP_MINIMIZE) == SWP_MINIMIZE)
	SwpFrame = SwpDt;

    x = SwpFrame.x + (SwpFrame.cx - Swp.cx) / 2;
    y = SwpFrame.y + (SwpFrame.cy - Swp.cy) / 2;

    x = max(0, x);
    x = min(x, SwpDt.cx - Swp.cx);

    y = max(0, y);
    y = min(y, SwpDt.cy - Swp.cy);

    WinSetWindowPos(hDlg, HWND_TOP, x, y, 0, 0, SWP_MOVE | SWP_SHOW);
}

//=== Misc ===

VOID CompressFilePath(PSZ RawText, PSZ Text)
{
    USHORT i;

    i = 3;
    while (i < 15 && RawText[i] != '\\')
	++i;

    if (RawText[i] == '\\')
	++i;

    strncpy(Text, RawText, i);
    Text[i] = '\0';

    strcat(Text, " ... ");

    i = (USHORT) (strlen(RawText) - 1);
    while (i > 0 && RawText[i] != '\\')
	--i;

    if (i > 0)
	--i;

    while (i > 0 && RawText[i] != '\\')
	--i;

    if (strlen(RawText) - i > 20)
	i = (USHORT) (strlen(RawText) - 20);

    strcat(Text, &RawText[i]);
}

//=== CreateInterpStr: convert long to string with separators ===

PCHAR CreateInterpStr(ULONG Number, PCHAR pszOut)
{
    SHORT i, j, cSep;
    CHAR szTmp[15];

    ultoa(Number, szTmp, 10);

    // Calc number of separators needed
    cSep = (SHORT) (strlen(szTmp) / 3);

    if (strlen(szTmp) >= 3 &&	strlen(szTmp) % 3 == 0)
	--cSep;

    j = (SHORT) (strlen(szTmp) + cSep - 1);
    memset(pszOut, 0x00, j + 2);

    for (i = (SHORT)(strlen(szTmp) - 1); i >= 0 && j >= 0; --i)
    {
	pszOut[j] = szTmp[i];
	--j;

	if ((strlen(szTmp) - i) % 3 == 0 && cSep > 0)
	{
	    pszOut[j] = *CountryInfo.szThousandsSeparator;
	    --j;
	    --cSep;
	}
    } // for

    return pszOut;
}

VOID ComposeTreeDetailLine(USHORT iTableData, PSZ pszDisplayLine,
			   USHORT usSizeofDisplayLine, PUSHORT pusNameOffset)
{
    USHORT iLevel;
    USHORT iTable;
    USHORT iLen;
    USHORT cWidth;
    CHAR szDLLName[MAX_NAME_LEN];

    // This will be widest entry because dll's are always 8.3
    cWidth = strlen(pShowTableData[0].Name);

    // Draw lines
    for (iLevel = 0; iLevel <= pShowTableData[iTableData].Level; ++iLevel)
    {
	// Calculate width for this level
	cWidth = 0;
	for (iTable = 0; iTable < cShowTableData; ++iTable)
	{
	    if (pShowTableData[iTable].Level == iLevel)
	    {
		iLen = strlen(pShowTableData[iTable].Name);
		if (iLen > cWidth)
		    cWidth = iLen;
	    }
	}

	if (iLevel == pShowTableData[iTableData].Level)
	    break;			// Got width

	// Blank fill
	sprintf(pszDisplayLine + strlen(pszDisplayLine), "%*s", cWidth, "");

	// Draw veriticals as needed
	if (pShowTableData[iTableData].Level > 0)
	{
	    if (pShowTableData[iTableData].Level > 1 &&
		iLevel <= pShowTableData[iTableData].Level - 2)
	    {
		// Find next level
		for (iTable = iTableData + 1; iTable < cShowTableData; ++iTable)
		{
		    if (pShowTableData[iTable].Level <= iLevel + 1)
			break;		// Got it
		}

		if (iTable >= cShowTableData ||
		    pShowTableData[iTable].Level < iLevel + 1)
		{
		    strcat(pszDisplayLine, " ");	// No more
		}
		else
		    strcat(pszDisplayLine, "³");
	    }
	    else
	    {
		// Find next level
		for (iTable = iTableData + 1; iTable < cShowTableData; ++iTable)
		{
		    if (pShowTableData[iTable].Level <= pShowTableData[iTableData].Level)
			break;
		} // for

		if (iTable >= cShowTableData ||
		      pShowTableData[iTable].Level < pShowTableData[iTableData].Level)
		    strcat(pszDisplayLine, "À");	// Last at this level
		else
		    strcat(pszDisplayLine, "Ã");	// More at this level
	    }
	} // if Level > 0
    } // for

    *pusNameOffset = (USHORT)strlen(pszDisplayLine);

    if (pShowTableData[iTableData].Dynamic)
    {
	strcpy(szDLLName, pShowTableData[iTableData].Name);
	strlwr(szDLLName);
	strcat(pszDisplayLine, szDLLName);
    }
    else
	strcat(pszDisplayLine, pShowTableData[iTableData].Name);

    if (iTableData + 1 < cShowTableData &&
	    pShowTableData[iTableData + 1].Level > pShowTableData[iTableData].Level)
    {
	iLen = strlen(pShowTableData[iTableData].Name);

	for (;iLen < cWidth; ++iLen)
	    strcat(pszDisplayLine, "Ä");
	strcat(pszDisplayLine, "¿");
    }

} // ComposeTreeDetailLine

VOID ComposeDllDetailLine(USHORT iTableData, PSZ pszDisplayLine)
{
    USHORT j;
    CHAR HlpStr[41];

    /*--- dynamic or static loaded ---*/
    if (pShowTableData[iTableData].Dynamic)
	strcpy(pszDisplayLine, "DYN  ");
    else
	strcpy(pszDisplayLine, "STAT ");

    /*--- name ---*/
    strcat(pszDisplayLine, pShowTableData[iTableData].Name);
    j = (USHORT) strlen(pShowTableData[iTableData].Name);
    while (j < 14)
    {
	strcat(pszDisplayLine, " ");
	++j;
    }

    /*--- size ---*/
    CreateInterpStr(pShowTableData[iTableData].FileSize, HlpStr);
    j = (USHORT) strlen(HlpStr);
    while (j < 8)
    {
	strcat(pszDisplayLine, " ");
	++j;
    }

    strcat(HlpStr, " ");
    strcat(pszDisplayLine, HlpStr);

    /*--- date/time ---*/
    FDateTimeToString(HlpStr,
		      &pShowTableData[iTableData].fdateLastWrite,
		      &pShowTableData[iTableData].ftimeLastWrite);

    strcat(pszDisplayLine, HlpStr);

    /*--- path ---*/
    strcat(pszDisplayLine, pShowTableData[iTableData].Path);
}

CHAR GetBootDrive(VOID)
{
    CHAR BootDrive;
    ULONG ulBootDrive;
    APIRET rc;

    rc = DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE, &ulBootDrive, sizeof(ULONG));
    if (rc)
    {
	AskShutWithMessage(rc, "DosQuerySysInfo failed (%lu)", rc);
	ulBootDrive = 3;		// Assume C:
    }

    BootDrive = (CHAR)(ulBootDrive + ('A' - 1));

    return (BootDrive);
}

/**
 * Extract LIBPATH from config.sys and extended libpath
 * @return TRUE if OK
 */

BOOL GetLibPath(PSZ LibPath, USHORT MaxLibPathSize, PSZ ErrorStr, BOOL includeExtLIBPATH)
{
#   define CCHMAXEXTLIBPATH 1024	// For extended libpath
    CHAR *pLibPath;
    CHAR *pch;
    HFILE hFile;
    UINT cb;
    APIRET rc;
    ULONG BytesRead;
    ULONG Action;
    FILESTATUS FileStatus;
    PCHAR pBuffer;
    CHAR ch;
    static CHAR CsPath[] = "?:\\CONFIG.SYS";

    LibPath[0] = 0;

    CsPath[0] = GetBootDrive();

    // Read config.sys into buffer
    rc = DosOpen(CsPath, &hFile, &Action, 0L,
		 FILE_NORMAL, FILE_OPEN,
		 OPEN_ACCESS_READONLY | OPEN_SHARE_DENYWRITE, 0L);

    if (rc) {
	sprintf(ErrorStr, "Can not open %s (%lu)", CsPath, rc);
	return (FALSE);
    }

    rc = DosQFileInfo(hFile, FIL_STANDARD, &FileStatus, sizeof(FILESTATUS));
    if (rc) {
	sprintf(ErrorStr, "Can not get file info for %s (%lu)", CsPath, rc);
	DosClose(hFile);
	return (FALSE);
    }

    if (FileStatus.cbFile > (ULONG) 0xFFFE) {
	sprintf(ErrorStr, "%s too big to handle (%lu)", CsPath, FileStatus.cbFile);
	DosClose(hFile);
	return (FALSE);
    }

    // Ensure big enough for config.sys and extended LIBPATH
    cb = FileStatus.cbFile + 1;
    if (includeExtLIBPATH && cb < CCHMAXEXTLIBPATH)
	cb = CCHMAXEXTLIBPATH;

    rc = DosAllocMem((PPVOID) & pBuffer, cb,
		     PAG_READ | PAG_WRITE | PAG_COMMIT);
    if (rc) {
	sprintf(ErrorStr, "Out of memory (%lu)", rc);
	DosClose(hFile);
	return (FALSE);
    }

    cb = 0;

    if (includeExtLIBPATH) {
	// Copy BEGIN_LIBPATH to to buffer
	rc = DosQueryExtLIBPATH(pBuffer, BEGIN_LIBPATH);
	if (rc) {
	    sprintf(ErrorStr, "DosQueryExtLIBPATH failed (%lu)", rc);
	    return (FALSE);
	}
	for (pch = pBuffer; *pch && cb < MaxLibPathSize - 1; pch++, cb++) {
	  LibPath[cb] = *pch;
	}
	// Append trailing semi-colon if needed
	if (cb > 0 && LibPath[cb - 1] != ';' && cb < MaxLibPathSize - 1)
	  LibPath[cb++] = ';';
    }

    rc = DosRead(hFile, pBuffer, FileStatus.cbFile, &BytesRead);
    if (rc) {
	sprintf(ErrorStr, "Can not read %s (%lu)", CsPath, rc);
	DosClose(hFile);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    if (BytesRead != (USHORT) FileStatus.cbFile) {
	sprintf(ErrorStr, "Can not read %lu bytes from %s", BytesRead, CsPath);
	DosClose(hFile);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    pBuffer[BytesRead] = '\0';

    DosClose(hFile);

    // Find LIBPATH statement in config.sys

    for (pLibPath = pBuffer; *pLibPath ; pLibPath++) {
	// Find LIBPATH keyword
	if (strnicmp(pLibPath, "LIBPATH", 7) != 0)
	    continue;

	// Ensure at start of line
	pch = pLibPath;

	// Back up over whitespace
	while (pch != pBuffer) {
	    pch--;
	    ch = *pch;
	    if (ch != ' ' && ch != '\t')
		break;
	} // while

	if (pch == pBuffer ||
	    ch == '\r' ||
	    ch == '\n')
	    break;		// Found it

    } // for config.sys

    if (*pLibPath == 0)	{
	sprintf(ErrorStr, "Can not find LIBPATH in %s", CsPath);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    pch = pLibPath + strlen("LIBPATH");

    // Find start of path list
    while (*pch &&
	    (*pch == ' ' ||
	     *pch == '\t' ||
	     *pch == '='))
	pch++;

    // Copy path list
    while (*pch &&
	   cb < MaxLibPathSize - 1 &&
	   *pch != 0x0D &&
	   *pch != 0x0A)
    {
	LibPath[cb] = *pch;
	cb++;
	pch++;
    }

    // Trim trailing whitespace
    while (cb && (LibPath[cb - 1] == ' ' || LibPath[cb - 1] == '\t'))
	cb--;

    if (includeExtLIBPATH) {
	rc = DosQueryExtLIBPATH(pBuffer, END_LIBPATH);
	if (rc) {
	    sprintf(ErrorStr, "DosQueryExtLIBPATH failed (%lu)", rc);
	    DosFreeMem(pBuffer);
	    return (FALSE);
	}

	// Append trailing semi-colon if needed
	if (*pBuffer && cb > 0 && LibPath[cb - 1] != ';' && cb < MaxLibPathSize - 1)
	  LibPath[cb++] = ';';

	for (pch = pBuffer; *pch && cb < MaxLibPathSize - 1; pch++, cb++) {
	  LibPath[cb] = *pch;
	}
    }

    DosFreeMem(pBuffer);
    pBuffer = NULL;

    if (cb == MaxLibPathSize - 1) {
	sprintf(ErrorStr, "LIBPATH in %s and extended LIBPATHs exceeds %lu",
		CsPath, MaxLibPathSize - 1);
	return (FALSE);
    }

    LibPath[cb] = 0;

    if (cb == 0)	{
	sprintf(ErrorStr, "Empty LIBPATH in %s", CsPath);
	return (FALSE);
    }

    return (TRUE);

} // GetLibPath

BOOL UpdateLibPath(PSZ LibPath, PSZ ErrorStr)
{
    CHAR *pLibPath, CsPath[21], CsTempPath[21], *pSearchStart;
    HFILE hFile, hTempFile;
    USHORT i, Offset;
    ULONG Action, BytesRead, BytesWritten;
    APIRET rc;
    FILESTATUS FileStatus;
    PCHAR pBuffer;

    CsPath[0] = GetBootDrive();
    strcpy(CsPath +1, ":\\CONFIG.SYS");
    CsTempPath[0] = GetBootDrive();
    strcpy(&CsTempPath[1], ":\\CONFIG.$$$");

    DosDelete(CsTempPath);

    rc = DosOpen(CsTempPath, &hTempFile, &Action, 0L,
		 FILE_NORMAL, FILE_CREATE,
		 OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYWRITE, 0L);

    if (rc)
    {
	sprintf(ErrorStr, "Could not open %s (%lu)", CsTempPath, rc);
	return (FALSE);
    }

    rc = DosOpen(CsPath, &hFile, &Action, 0L,
		 FILE_NORMAL, FILE_OPEN,
		 OPEN_ACCESS_READONLY | OPEN_SHARE_DENYWRITE, 0L);

    if (rc)
    {
	sprintf(ErrorStr, "Could not open %s (%lu)", CsPath, rc);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	return (FALSE);
    }

    rc = DosQFileInfo(hFile, FIL_STANDARD, &FileStatus, sizeof(FILESTATUS));
    if (rc)
    {
	sprintf(ErrorStr, "Could not get info on %s (%lu)", CsPath, rc);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosClose(hFile);
	return (FALSE);
    }

    if (FileStatus.cbFile > 0xFFFEL)
    {
	sprintf(ErrorStr, "%s too big to handle (%lu)", CsPath, FileStatus.cbFile);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosClose(hFile);
	return (FALSE);
    }

    rc = DosAllocMem((PPVOID) & pBuffer, FileStatus.cbFile + 1,
		     PAG_READ | PAG_WRITE | PAG_COMMIT);
    if (rc)
    {
	sprintf(ErrorStr, "Out of memory (%lu)", rc);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosClose(hFile);
	return (FALSE);
    }

    rc = DosRead(hFile, pBuffer, FileStatus.cbFile, &BytesRead);
    if (rc)
    {
	sprintf(ErrorStr, "Could not read %s (%lu)", CsPath, rc);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosClose(hFile);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    if (BytesRead != FileStatus.cbFile)
    {
	sprintf(ErrorStr, "Could not read %s (%lu)", CsPath, BytesRead);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosClose(hFile);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    pBuffer[BytesRead] = '\0';

    DosClose(hFile);

/*--- search LIBPATH ---*/

    pSearchStart = pBuffer;

    for (;;)
    {
	pLibPath = strstr(pSearchStart, "LIBPATH");

	if (pLibPath == NULL)
	{
	    sprintf(ErrorStr, "Could not find LIBPATH in %s", CsPath);
	    DosFreeMem(pBuffer);
	    return FALSE;
	}

	Offset = (USHORT) ((char*)pLibPath - (char*)pBuffer);

	if (Offset == 0 ||
		pBuffer[Offset - 1] == 0x0D ||
		pBuffer[Offset - 1] == 0x0A)
	    break;

	++pLibPath;
	pSearchStart = pLibPath;
    }

    i = (USHORT) (((char*)pLibPath - (char*)pBuffer) + strlen("LIBPATH"));

    while (i < (USHORT) FileStatus.cbFile &&
	    (pBuffer[i] == ' ' ||
	     pBuffer[i] == '='))
	++i;

    rc = DosWrite(hTempFile, pBuffer, i, &BytesWritten);
    if (rc)
    {
	sprintf(ErrorStr, "Could not write to %s (%lu)", CsTempPath, rc);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    if (BytesWritten != i)
    {
	sprintf(ErrorStr, "Could not write to %s (%lu)", CsTempPath, BytesWritten);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    rc = DosWrite(hTempFile, LibPath, strlen(LibPath), &BytesWritten);
    if (rc)
    {
	sprintf(ErrorStr, "Could not write to %s (%lu)", CsTempPath, rc);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    if (BytesWritten != strlen(LibPath))
    {
	sprintf(ErrorStr, "Could not write to %s (%lu)", CsTempPath, BytesWritten);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    while (i < (USHORT) FileStatus.cbFile &&
	    pBuffer[i] != 0x0D &&
	    pBuffer[i] != 0x0A)
    {
	++i;
    }

    rc = DosWrite(hTempFile, &pBuffer[i], (USHORT) FileStatus.cbFile - i, &BytesWritten);
    if (rc)
    {
	sprintf(ErrorStr, "Could not write to %s (%lu)", CsTempPath, rc);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    if (BytesWritten != FileStatus.cbFile - i)
    {
	sprintf(ErrorStr, "Could not write to %s (%lu)", CsTempPath, BytesWritten);
	DosClose(hTempFile);
	DosDelete(CsTempPath);
	DosFreeMem(pBuffer);
	return (FALSE);
    }

    DosFreeMem(pBuffer);
    DosClose(hTempFile);

    rc = DosCopy(CsTempPath, CsPath, DCPY_EXISTING);
    if (rc)
    {
	sprintf(ErrorStr, "Could not copy temporary file %s (%lu)", CsTempPath, rc);
	DosDelete(CsTempPath);
	return (FALSE);
    }

    DosDelete(CsTempPath);

    return (TRUE);
}

BOOL CheckDirPath(PSZ DirName, PSZ ErrorStr)
{
    APIRET rc;
    FILESTATUS3 FileStatus;

    rc = DosQueryPathInfo(DirName, FIL_STANDARD,
			  &FileStatus, sizeof(FILESTATUS3));

    if (rc)
    {
	if (rc == ERROR_FILE_NOT_FOUND)
	    strcpy(ErrorStr, "Directory does not exist");
	else if (rc == ERROR_PATH_NOT_FOUND)
	    strcpy(ErrorStr, "Path does not exist");
	else if (rc == ERROR_INVALID_NAME ||
		 rc == ERROR_FILENAME_EXCED_RANGE)
	    strcpy(ErrorStr, "Invalid directory path string");
	else if (rc == ERROR_INVALID_DRIVE)
	    strcpy(ErrorStr, "Invalid drive in directory path string");
	else
	    sprintf(ErrorStr, "Invalid directory name/path (%lu)", rc);

	return (FALSE);
    }

    if ((FileStatus.attrFile & FILE_DIRECTORY) != FILE_DIRECTORY)
    {
	strcpy(ErrorStr, "Directory path is a file name");
	return (FALSE);
    }

    return (TRUE);
}

//=== Drag and drop control ===

MRESULT ProcessDragOver(MPARAM mp1)
{
    PDRAGINFO pDragInfo;
    PDRAGITEM pDragItem;
    ULONG NameLen;

    if (hWndProcess != NULLHANDLE)
	return (MPFROM2SHORT(DOR_NEVERDROP, DO_UNKNOWN));

    strcpy(DragPath, "");
    pDragInfo = (PDRAGINFO) mp1;

    if (!DrgAccessDraginfo(pDragInfo))
	return (MPFROM2SHORT(DOR_NEVERDROP, DO_UNKNOWN));

    if (DrgQueryDragitemCount(pDragInfo) != 1)
    {
	DrgFreeDraginfo(pDragInfo);
	return (MPFROM2SHORT(DOR_NEVERDROP, DO_UNKNOWN));
    }

    pDragItem = DrgQueryDragitemPtr(pDragInfo, 0);

//if (!DrgVerifyRMF (pDragItem, "DRM_OBJECT", "DRF_OBJECT"))
    //  {
    //  DrgFreeDraginfo (pDragInfo);
    //  return (MPFROM2SHORT (DOR_NEVERDROP, DO_UNKNOWN));
    //  }

//DrgQueryStrName (pDragItem->hstrRMF,
    //               sizeof (DebugStr), DebugStr);

//DrgQueryTrueType (pDragItem, sizeof (DebugStr), DebugStr);

    NameLen = DrgQueryStrName(pDragItem -> hstrSourceName,
			      sizeof(DragFile), DragFile);

    if (NameLen == 0)
    {
	DrgFreeDraginfo(pDragInfo);
	return (MPFROM2SHORT(DOR_NEVERDROP, DO_UNKNOWN));
    }

    NameLen = DrgQueryStrName(pDragItem -> hstrContainerName,
			      sizeof(DragPath), DragPath);
    if (NameLen == 0)
    {
	DrgFreeDraginfo(pDragInfo);
	return (MPFROM2SHORT(DOR_NEVERDROP, DO_UNKNOWN));
    }

//DebugPrintf ("|%s|, |%s|", DragFile, DragPath);

    DrgFreeDraginfo(pDragInfo);

    return (MPFROM2SHORT(DOR_DROP, DO_UNKNOWN));
}

MRESULT ProcessDragDrop(MPARAM mp1)
{
    PDRAGINFO pDragInfo;

    pDragInfo = (PDRAGINFO) mp1;

    if (!DrgAccessDraginfo(pDragInfo))
	return (MPFROM2SHORT(DOR_NEVERDROP, DO_UNKNOWN));

    DrgFreeDraginfo(pDragInfo);

    strcpy(CurrentFileName, DragPath);
    WinPostMsg(hWndClient, WUM_START_REBUILD, 0L, 0L);

    return (0L);
}

//=== Switch list control ===

VOID SetSwitchWnd(HWND hWnd)
{
    SWCNTRL SwEntry;

    if (!WinQuerySwitchEntry(hSwitch, &SwEntry))
    {
	SwEntry.hwnd = hWnd;
	WinChangeSwitchEntry(hSwitch, &SwEntry);
    }
}

//=== List box control ===

BOOL StrInListbox(HWND hDlg, SHORT LbId, PSZ LbStr)
{
    SHORT i, NrOfItems;
    CHAR TestStr[256];

    NrOfItems =
	SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LbId),
				LM_QUERYITEMCOUNT, NULL, NULL));

    for (i = 0; i < NrOfItems; ++i)
    {
	WinSendMsg(WinWindowFromID(hDlg, LbId),
		   LM_QUERYITEMTEXT, MPFROM2SHORT(i, sizeof(TestStr)),
		   MPFROMP(TestStr));

	if (stricmp(TestStr, LbStr) == 0)
	    break;
    }

    if (i < NrOfItems)
	return (TRUE);

    return (FALSE);
}

//=== Message boxes ===

//=== ShwOKCanMsgBox: Show error message in app modal box ===

APIRET ShwOKCanMsgBox(CHAR *pszFmt, ...)		/* i : Format string or message */
{
    va_list	pVA;
    APIRET	rc;

    va_start(pVA, pszFmt);

    rc = VShwMsgBox(HWND_DESKTOP,
		    MB_OKCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL,
		    pszFmt,
		    pVA);
    return rc;

} // ShwOKCanMsgBox

//=== ShwOKDeskMsgBox: Show message in app modal box ===

VOID ShwOKDeskMsgBox(CHAR *pszFmt, ...)
{
    va_list	pVA;

    va_start(pVA, pszFmt);

    VShwMsgBox(HWND_DESKTOP,
	       0,
	       pszFmt,
	       pVA);

} // ShwOKDeskMsgBox

//=== ShwOKMsgBox: Show OK message in app modal box ===

VOID ShwOKMsgBox(HWND hwndOwner, CHAR *pszFmt, ...)	/* i : Format string or message */
{
    va_list	pVA;

    va_start(pVA, pszFmt);

    VShwMsgBox(hwndOwner,
	       0,
	       pszFmt,
	       pVA);

} // ShwOKMsgBox

//=== ShwYesNoMsgBox: Show error message in app modal box ===

APIRET ShwYesNoMsgBox(HWND hwndOwner, CHAR *pszFmt, ...)	/* i : Format string or message */
{
    va_list	pVA;
    APIRET	rc;

    va_start(pVA, pszFmt);

    rc = VShwMsgBox(hwndOwner,
		    MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL,
		    pszFmt,
		    pVA);
    return rc;

} // ShwYesNoMsgBox

//=== VShwMsgBox: Show OK message in app modal box ===

APIRET VShwMsgBox(HWND hwndOwner,
		  ULONG flStyle,
		  CHAR *pszFmt,
		  va_list pVA)
{
    APIRET	rc;
    CHAR	szMsg[CCHMAXPATH + 160];

    /* Format */
    vsprintf(szMsg, pszFmt, pVA);

    /* Default */
    if (flStyle == 0)
	flStyle = MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL;

    rc = WinMessageBox(HWND_DESKTOP,
		       hwndOwner,
		       szMsg,
		       ApplTitle,
		       0,
		       flStyle);

    return rc;

} // VShwMsgBox

// The end
