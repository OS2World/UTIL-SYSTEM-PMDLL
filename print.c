
/***********************************************************************

print.c	Helper Print report

Copyright (c) 2001 Steven Levine and Associates, Inc.
Copyright (c) 1994 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) print.c - Ver 3, 31-May-01,13:23:28 $

Revisions	25 Jun 94 AVB - Release
		14 May 01 SHL - Use F....ToText to correct Y2K errors
		31 May 01 SHL - Correct window proc arguments, drop casts

***********************************************************************/


#define EXTERN extern

#include "pmdll.h"

static VOID _PrintPageHeader(HFILE hFilePrn, USHORT PageWidth, PSZ PrnHeader,
			     PUSHORT PrnPageNr, PUSHORT PrnLineNr);
static BOOL _TreeInfoToPrinter(HWND hDlg, PCHAR Printer, USHORT PageLen,
			       USHORT PageWidth);

MRESULT EXPENTRY PrintWndProc(HWND hDlg, ULONG Msg, MPARAM mp1, MPARAM mp2)

{
    static CHAR Printer[51];
    static USHORT PageLen, PageWidth;
    CHAR PageLenStr[21], PageWidthStr[21];

    switch (Msg)
    {
    case WM_INITDLG:
	WinSetWindowText(hDlg, "Print DLL tree");

	strcpy(Printer, PrfPrinter);
	WinEnableWindow(WinWindowFromID(hDlg, EID_PRINT_FILE_NAME), FALSE);
	WinSendDlgItemMsg(hDlg, EID_PRINT_FILE_NAME, EM_SETTEXTLIMIT,
			  MPFROMSHORT(sizeof(Printer) - 1), 0L);

	WinSendDlgItemMsg(hDlg, EID_PRINT_PW, EM_SETTEXTLIMIT,
			  MPFROMSHORT(3), 0L);
	WinSendDlgItemMsg(hDlg, EID_PRINT_PL, EM_SETTEXTLIMIT,
			  MPFROMSHORT(3), 0L);

	PageWidth = PrfPageWidth;
	WinSetDlgItemText(hDlg, EID_PRINT_PW, itoa(PrfPageWidth, PageWidthStr, 10));

	PageLen = PrfPageLen;
	WinSetDlgItemText(hDlg, EID_PRINT_PL, itoa(PrfPageLen, PageLenStr, 10));

	if (stricmp(Printer, "LPT1") == 0)
	{
	    WinSendDlgItemMsg(hDlg, RID_PRINT_LPT1,
			      BM_SETCHECK, MPFROMSHORT(TRUE), 0L);

	}
	else if (stricmp(Printer, "LPT2") == 0)
	{
	    WinSendDlgItemMsg(hDlg, RID_PRINT_LPT2,
			      BM_SETCHECK, MPFROMSHORT(TRUE), 0L);

	}
	else
	{
	    WinSendDlgItemMsg(hDlg, RID_PRINT_FILE,
			      BM_SETCHECK, MPFROMSHORT(TRUE), 0L);

	    WinSetDlgItemText(hDlg, EID_PRINT_FILE_NAME, Printer);
	    WinEnableWindow(WinWindowFromID(hDlg, EID_PRINT_FILE_NAME), TRUE);
	}
	CenterDialog(hWndFrame, hDlg);

	WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_PRINT_PW));
	return ((MRESULT) TRUE);
	break;

    case WM_CONTROL:
	switch (SHORT1FROMMP(mp1))
	{
	case EID_PRINT_PW:
	    if (SHORT2FROMMP(mp1) == EN_KILLFOCUS)
	    {
		WinQueryDlgItemText(hDlg, EID_PRINT_PW, 5, PageWidthStr);

		PageWidth = (USHORT) atoi(PageWidthStr);

		if (PageWidth == 0)
		    PageWidth = 80;
		else if (PageWidth < 50)
		    PageWidth = 50;
		else if (PageWidth > 150)
		    PageWidth = 150;

		WinSetDlgItemText(hDlg, EID_PRINT_PW, itoa(PageWidth, PageWidthStr, 10));
	    }
	    break;

	case EID_PRINT_PL:
	    if (SHORT2FROMMP(mp1) == EN_KILLFOCUS)
	    {
		WinQueryDlgItemText(hDlg, EID_PRINT_PL, 5, PageLenStr);

		PageLen = (USHORT) atoi(PageLenStr);

		if (PageLen == 0)
		    PageLen = 60;
		else if (PageLen < 20)
		    PageLen = 20;
		else if (PageLen > 150)
		    PageLen = 150;

		WinSetDlgItemText(hDlg, EID_PRINT_PL, itoa(PageLen, PageLenStr, 10));
	    }
	    break;

	case RID_PRINT_LPT1:
	    strcpy(Printer, "LPT1");
	    WinEnableWindow(WinWindowFromID(hDlg, DID_OK), TRUE);
	    WinEnableWindow(WinWindowFromID(hDlg, EID_PRINT_FILE_NAME), FALSE);
	    WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, DID_OK));
	    break;

	case RID_PRINT_LPT2:
	    strcpy(Printer, "LPT2");
	    WinEnableWindow(WinWindowFromID(hDlg, DID_OK), TRUE);
	    WinEnableWindow(WinWindowFromID(hDlg, EID_PRINT_FILE_NAME), FALSE);
	    WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, DID_OK));
	    break;

	case RID_PRINT_FILE:
	    strcpy(Printer, "");
	    WinEnableWindow(WinWindowFromID(hDlg, EID_PRINT_FILE_NAME), TRUE);

	    WinQueryDlgItemText(hDlg, EID_PRINT_FILE_NAME, 50, Printer);
	    if (EmptyField(Printer))
		WinEnableWindow(WinWindowFromID(hDlg, DID_OK), FALSE);
	    else
		WinEnableWindow(WinWindowFromID(hDlg, DID_OK), TRUE);
	    WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_PRINT_FILE_NAME));
	    break;

	case EID_PRINT_FILE_NAME:
	    if (SHORT2FROMMP(mp1) == EN_CHANGE)
	    {
		WinQueryDlgItemText(hDlg, EID_PRINT_FILE_NAME, 50, Printer);
		if (EmptyField(Printer))
		    WinEnableWindow(WinWindowFromID(hDlg, DID_OK), FALSE);
		else
		    WinEnableWindow(WinWindowFromID(hDlg, DID_OK), TRUE);
	    }
	    break;
	}
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case DID_CANCEL:
	    WinDismissDlg(hDlg, FALSE);
	    break;

	case DID_OK:
	    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				  PRF_PRINTER_KEY, Printer);
	    strcpy(PrfPrinter, Printer);

	    itoa(PageWidth, PageWidthStr, 10);
	    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				  PRF_PAGEWIDTH_KEY, PageWidthStr);
	    PrfPageWidth = PageWidth;

	    itoa(PageLen, PageLenStr, 10);
	    PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				  PRF_PAGELEN_KEY, PageLenStr);
	    PrfPageLen = PageLen;

	    WinEnableWindow(WinWindowFromID(hDlg, DID_CANCEL), FALSE);
	    WinEnableWindow(WinWindowFromID(hDlg, DID_OK), FALSE);

	    CursorWait();
	    _TreeInfoToPrinter(hDlg, Printer, PageLen, PageWidth);
	    CursorNormal();

	    WinDismissDlg(hDlg, TRUE);
	    break;
	}
	break;

    case WM_CLOSE:
	WinPostMsg(hDlg, WM_QUIT, 0L, 0L);
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
    }

    return NULL;
}

static BOOL _TreeInfoToPrinter(HWND hDlg, PCHAR Printer, USHORT PageLen,
			       USHORT PageWidth)

{
    HFILE hFilePrn;
    USHORT PrnPageNr, PrnLineNr, Dummy, Index;
    ULONG Action, BytesWritten;
    APIRET rc;
    CHAR PrnHeader[150], PrintLine[256], ErrorStr[81];
    UCHAR PrintByte;

    if (stricmp(Printer, "LPT1") == 0)
    {
	rc = DosOpen("LPT1", &hFilePrn, &Action, 0L, FILE_NORMAL, FILE_OPEN,
		     OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE, 0L);

	if (rc)
	{
	    WinMessageBox(HWND_DESKTOP, hDlg,
			  "Could not write to LPT1", ApplTitle,
			  1, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

	    return (FALSE);
	}
    }
    else if (stricmp(Printer, "LPT2") == 0)
    {
	rc = DosOpen("LPT2", &hFilePrn, &Action, 0L, FILE_NORMAL, FILE_OPEN,
		     OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE, 0L);
	if (rc)
	{
	    WinMessageBox(HWND_DESKTOP, hDlg,
			  "Could not write to LPT2", ApplTitle,
			  1, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

	    return (FALSE);
	}
    }
    else
    {
	rc = DosOpen(Printer, &hFilePrn, &Action, 0L, FILE_NORMAL,
		     FILE_CREATE | FILE_OPEN,
		     OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE |
		     OPEN_FLAGS_FAIL_ON_ERROR, 0L);

	if (!rc && Action == FILE_EXISTED)
	{
	    sprintf(ErrorStr, "File %s exists, overwrite ?", Printer);
	    rc = WinMessageBox(HWND_DESKTOP, hDlg,
			       ErrorStr, ApplTitle,
			       1, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);

	    DosClose(hFilePrn);

	    if (rc == MBID_NO)
		return (FALSE);

	    DosDelete(Printer);

	    rc = DosOpen(Printer, &hFilePrn, &Action, 0L, FILE_NORMAL, FILE_CREATE,
			 OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE |
			 OPEN_FLAGS_FAIL_ON_ERROR, 0L);
	}

	if (rc)
	{
	    sprintf(ErrorStr, "Could not open printfile (%u)", rc);
	    WinMessageBox(HWND_DESKTOP, hDlg, ErrorStr, ApplTitle,
			  0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

	    return (FALSE);
	}
    }

    PrnPageNr = 0;
    PrnLineNr = 1;

    /*---- print header ----*/

    strcpy(PrnHeader, FrameTitle);

    while (strlen(PrnHeader) < PageWidth - 7)
	strcat(PrnHeader, " ");
    strcat(PrnHeader, "Page ");

    _PrintPageHeader(hFilePrn, PageWidth, PrnHeader,
		     &PrnPageNr, &PrnLineNr);

    Index = 0;

    for (Index = 0; Index < cShowTableData; ++Index)
    {
	memset(PrintLine, 0x00, sizeof(PrintLine));
	if (TreeViewMode)
	    ComposeTreeDetailLine(Index, PrintLine, sizeof(PrintLine), &Dummy);
	else
	    ComposeDllDetailLine(Index, PrintLine);

	PrintLine[PageWidth - 1] = '\0';
	DosWrite(hFilePrn, PrintLine, strlen(PrintLine), &BytesWritten);

        /*---- carriage return / linefeed ----*/
	PrintByte = 0x0D;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	PrintByte = 0x0A;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	++PrnLineNr;

	if (PrnLineNr > PageLen)
	{
            /*--- formfeed ----*/
	    PrintByte = 0x0C;
	    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);

	    _PrintPageHeader(hFilePrn, PageWidth, PrnHeader,
			     &PrnPageNr, &PrnLineNr);
	}
    }

    /*--- formfeed ----*/

    PrintByte = 0x0C;
    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);

    _PrintPageHeader(hFilePrn, PageWidth, PrnHeader,
		     &PrnPageNr, &PrnLineNr);

    for (Index = 1; Index < cShowTableData; ++Index)
    {
	if (pShowTableData[Index].SystemDll)
	    continue;

        /*--- line 1 ---*/

	sprintf(PrintLine, "Module        : %s", pShowTableData[Index].Name);
	DosWrite(hFilePrn, PrintLine, strlen(PrintLine), &BytesWritten);

	PrintByte = 0x0D;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	PrintByte = 0x0A;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	++PrnLineNr;

        /*--- line 2 ---*/

	if (pShowTableData[Index].PathFound &&
		pShowTableData[Index].Loadable)
	    sprintf(PrintLine, "Loaded from   : %s", pShowTableData[Index].Path);
	else
	    sprintf(PrintLine, "Error         : %s", pShowTableData[Index].Path);

	DosWrite(hFilePrn, PrintLine, strlen(PrintLine), &BytesWritten);

	PrintByte = 0x0D;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	PrintByte = 0x0A;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	++PrnLineNr;

        /*--- line 3 ---*/

	strcpy(PrintLine, "File size     : ");
	if (pShowTableData[Index].PathFound)
	{
	    CreateInterpStr(pShowTableData[Index].FileSize,
			    &PrintLine[strlen(PrintLine)]);
	    strcat(PrintLine, " bytes");
	}
	else
	    strcat(PrintLine, "?");

	DosWrite(hFilePrn, PrintLine, strlen(PrintLine), &BytesWritten);

	PrintByte = 0x0D;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	PrintByte = 0x0A;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	++PrnLineNr;

        /*--- line 4 ---*/

	if (pShowTableData[Index].PathFound)
	{
	    strcpy(PrintLine, "File date/time: ");

	    FDateTimeToString(PrintLine + strlen(PrintLine),
	                      &pShowTableData[Index].fdateLastWrite,
		              &pShowTableData[Index].ftimeLastWrite);
	}
	else
	    strcpy(PrintLine, "File date/time: ?");

	DosWrite(hFilePrn, PrintLine, strlen(PrintLine), &BytesWritten);

	PrintByte = 0x0D;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	PrintByte = 0x0A;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	++PrnLineNr;

        /*--- empty line ---*/

	PrintByte = 0x0D;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	PrintByte = 0x0A;
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
	++PrnLineNr;

	if (PrnLineNr > PageLen - 5)
	{
            /*--- formfeed ----*/
	    PrintByte = 0x0C;
	    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);

	    _PrintPageHeader(hFilePrn, PageWidth, PrnHeader,
			     &PrnPageNr, &PrnLineNr);
	}
    }

    DosClose(hFilePrn);

    return (TRUE);
}

static VOID _PrintPageHeader(HFILE hFilePrn, USHORT PageWidth, PSZ PrnHeader,
			     PUSHORT PrnPageNr, PUSHORT PrnLineNr)

{
    CHAR PrnHeaderPrn[150];
    USHORT i;
    ULONG BytesWritten;
    BYTE PrintByte;

    ++(*PrnPageNr);
    *PrnLineNr = 1;

    sprintf(PrnHeaderPrn, "%s%u", PrnHeader, *PrnPageNr);
    DosWrite(hFilePrn, PrnHeaderPrn, strlen(PrnHeaderPrn), &BytesWritten);

    PrintByte = 0x0D;
    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
    PrintByte = 0x0A;
    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
    for (i = 0; i < PageWidth; ++i)
    {
	PrintByte = '-';
	DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
    }
    PrintByte = 0x0D;
    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
    PrintByte = 0x0A;
    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
    PrintByte = 0x0D;
    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);
    PrintByte = 0x0A;
    DosWrite(hFilePrn, &PrintByte, 1, &BytesWritten);

    *PrnLineNr += 3;

}

// The end
