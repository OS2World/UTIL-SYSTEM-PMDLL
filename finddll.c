
/***********************************************************************

finddll.c - Find DLL in Libpath

Copyright (c) 2001, 2012 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) finddll.c - Ver 5, 13-May-12,19:47:08 $

1994-06-26 AVB Baseline
2001-05-14 SHL Use F....ToText to correct Y2K errors
2001-05-31 SHL Correct window procedure arguments drop casts
2012-04-22 SHL Avoid OpenWatcom warnings
2012-05-12 SHL Sync with extended LIBPATH mods

***********************************************************************/

#define EXTERN extern

#include "pmdll.h"

MRESULT EXPENTRY FindDllWndProc(HWND hDlg, ULONG Msg, MPARAM mp1, MPARAM mp2)
{
    static CHAR LibPath[MAX_LIBPATH_SIZE];
    FILESTATUS FileStatus;
    CHAR FindDllStr[9], FullFindDllStr[13], FoundPath[CCHMAXPATH], HlpStr[51];
    APIRET rc;

    switch (Msg) {
    case WM_INITDLG:
	if (!GetLibPath(LibPath, MAX_LIBPATH_SIZE, ErrorStr, TRUE)) {
	    WinMessageBox(HWND_DESKTOP, hWndFrame, ErrorStr,
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
	    WinDismissDlg(hDlg, FALSE);
	    break;
	}

	WinSendDlgItemMsg(hDlg, EID_FIND_DLL, EM_SETTEXTLIMIT,
			  MPFROMSHORT(8), 0L);
	WinSetDlgItemText(hDlg, EID_FIND_DLL, "");

	CenterDialog(hWndFrame, hDlg);

	WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_FIND_DLL));
	return ((MRESULT) TRUE);
	break;

    case WM_CONTROL:
	switch (SHORT1FROMMP(mp1))
	{
	case EID_FIND_DLL:
	    if (SHORT2FROMMP(mp1) == EN_CHANGE)
	    {
		WinSetDlgItemText(hDlg, TID_FIND_DLL, "");
		WinSetDlgItemText(hDlg, TID_FIND_DLL_DATE, "");
		WinSetDlgItemText(hDlg, TID_FIND_DLL_SIZE, "");

		WinQueryDlgItemText(hDlg, EID_FIND_DLL, 9, FindDllStr);

		if (EmptyField(FindDllStr))
		    WinEnableWindow(WinWindowFromID(hDlg, DID_OK), FALSE);
		else
		    WinEnableWindow(WinWindowFromID(hDlg, DID_OK), TRUE);

		WinEnableWindow(WinWindowFromID(hDlg, BID_FIND_DLL), FALSE);
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
	    WinQueryDlgItemText(hDlg, EID_FIND_DLL, 9, FullFindDllStr);
	    strcat(FullFindDllStr, ".DLL");

	    rc = DosSearchPath(SEARCH_PATH | SEARCH_IGNORENETERRS,
			       LibPath,
			       FullFindDllStr,
			       (PUCHAR)FoundPath,
			       CCHMAXPATH);

	    if (rc)
	    {
		WinEnableWindow(WinWindowFromID(hDlg, BID_FIND_DLL), FALSE);

		if (rc == ERROR_FILE_NOT_FOUND)
		    strcpy(FoundPath, "DLL could not be located");
		else
		    sprintf(FoundPath, "Error searching DLL: %lu", rc);
	    }
	    else
	    {
		strupr(FoundPath);

		if (strchr(FoundPath, '*') ||
			strchr(FoundPath, '?'))
		{
		    WinEnableWindow(WinWindowFromID(hDlg, BID_FIND_DLL), FALSE);
		    WinSetDlgItemText(hDlg, TID_FIND_DLL_DATE, "-");
		    WinSetDlgItemText(hDlg, TID_FIND_DLL_SIZE, "-");
		}
		else
		{
		    if (!DosQPathInfo(FoundPath, FIL_STANDARD,
				  (PBYTE)&FileStatus, sizeof(FILESTATUS)))
		    {
			CreateInterpStr(FileStatus.cbFile, HlpStr);
			strcat(HlpStr, " bytes");
			WinSetDlgItemText(hDlg, TID_FIND_DLL_SIZE, HlpStr);

			FDateTimeToString(HlpStr,
					  &FileStatus.fdateLastWrite,
					  &FileStatus.ftimeLastWrite);

			WinSetDlgItemText(hDlg, TID_FIND_DLL_DATE, HlpStr);
		    }

		    WinEnableWindow(WinWindowFromID(hDlg, BID_FIND_DLL), TRUE);
		}
	    }

	    WinSetDlgItemText(hDlg, TID_FIND_DLL, FoundPath);
	    WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_FIND_DLL));
	    break;

	case BID_FIND_DLL:
	    WinQueryDlgItemText(hDlg, TID_FIND_DLL, CCHMAXPATH,
				CurrentFileName);

	    WinDismissDlg(hDlg, TRUE);
	    break;
	}
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
    } // switch

    return NULL;
}
