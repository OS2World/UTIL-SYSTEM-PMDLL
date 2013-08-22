
/***********************************************************************

libpath.c - Libpath editor

Copyright (c) 2001, 2012 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) libpath.c - Ver 3, 13-May-12,19:43:20 $

1995-04-26 AVB Baseline
2001-05-31 SHL Correct window proc arguments
2012-05-12 SHL Sync with extended LIBPATH mods

***********************************************************************/

#define EXTERN extern

#include "pmdll.h"

static CHAR PathError[300];

MRESULT EXPENTRY LibpathWndProc(HWND hDlg, ULONG Msg, MPARAM mp1, MPARAM mp2)

{
    CHAR LibPath[MAX_LIBPATH_SIZE], *pLibPath, NewEntry[51];
    SHORT i, ItemCount;
    APIRET rc;
    SHORT ItemId, NrOfItems;

    switch (Msg)
    {
    case WM_INITDLG:
	if (!GetLibPath(LibPath, MAX_LIBPATH_SIZE, ErrorStr, FALSE))
	{
	    WinMessageBox(HWND_DESKTOP, hWndFrame, ErrorStr,
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
	    WinDismissDlg(hDlg, FALSE);
	    break;
	}

	i = 0;
	ItemCount = 0;
	pLibPath = LibPath;

	while (LibPath[i])
	{
	    if (LibPath[i] == ';')
	    {
		LibPath[i] = '\0';
		strupr(pLibPath);

		if (StrInListbox(hDlg, LID_LIBPATH, pLibPath))
		{
		    sprintf(PathError,
			 "The path:\n\n%s\n\nappears twice in your libpath",
			    pLibPath);
		    WinMessageBox(HWND_DESKTOP, hWndFrame, PathError,
		    ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
		}

		WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_INSERTITEM,
				  MPFROMSHORT(LIT_END), MPFROMP(pLibPath));

		ItemCount++;
		pLibPath = &LibPath[i + 1];
	    }

	    ++i;
	}

	if (pLibPath < &LibPath[i])
	{
	    WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_INSERTITEM,
			      MPFROMSHORT(LIT_END), MPFROMP(pLibPath));
	    ItemCount++;
	}

	if (ItemCount == 0)
	{
	    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_UP), FALSE);
	    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_DOWN), FALSE);
	    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_REMOVE), FALSE);
	}
	else
	{
	    WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_SELECTITEM,
			      MPFROMSHORT(0), MPFROMSHORT(TRUE));
	}

	WinSendDlgItemMsg(hDlg, EID_LIBPATH_NEW, EM_SETTEXTLIMIT,
			  MPFROMSHORT(sizeof(NewEntry) - 1), 0L);
	WinSetDlgItemText(hDlg, EID_LIBPATH_NEW, "");

	CenterDialog(hWndFrame, hDlg);

	WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, LID_LIBPATH));
	return ((MRESULT) TRUE);
	break;

    case WM_CONTROL:
	switch (SHORT1FROMMP(mp1))
	{
	case LID_LIBPATH:
	    if (SHORT2FROMMP(mp1) == LN_SELECT ||
		    SHORT2FROMMP(mp1) == LN_ENTER)
	    {
		ItemId =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

		if (ItemId == LIT_NONE)
		{
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_UP), FALSE);
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_DOWN), FALSE);
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_REMOVE), FALSE);
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_MODIFY), FALSE);
		    break;
		}

		WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_REMOVE), TRUE);
		WinQueryDlgItemText(hDlg, EID_LIBPATH_NEW,
				    sizeof(NewEntry), NewEntry);
		if (EmptyField(NewEntry))
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_MODIFY), FALSE);
		else
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_MODIFY), TRUE);

		if (ItemId == 0)
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_UP), FALSE);
		else
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_UP), TRUE);

		NrOfItems =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
					    LM_QUERYITEMCOUNT, NULL, NULL));

		if (ItemId == NrOfItems - 1)
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_DOWN), FALSE);
		else
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_DOWN), TRUE);

		if (SHORT2FROMMP(mp1) == LN_ENTER)
		{
		    WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
		    LM_QUERYITEMTEXT, MPFROM2SHORT(ItemId, MAX_LIBPATH_SIZE),
			       MPFROMP(LibPath));

		    WinSetDlgItemText(hDlg, EID_LIBPATH_NEW, LibPath);
		}
	    }
	    break;

	case EID_LIBPATH_NEW:
	    if (SHORT2FROMMP(mp1) == EN_CHANGE)
	    {
		WinQueryDlgItemText(hDlg, EID_LIBPATH_NEW,
				    sizeof(NewEntry), NewEntry);

		if (EmptyField(NewEntry))
		{
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_ADD), FALSE);
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_MODIFY), FALSE);
		}
		else
		{
		    WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_ADD), TRUE);
		    ItemId =
			SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

		    if (ItemId != LIT_NONE)
			WinEnableWindow(WinWindowFromID(hDlg, BID_LIBPATH_MODIFY), TRUE);
		}
	    }
	    break;
	}
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case BID_LIBPATH_DOWN:
	case BID_LIBPATH_UP:
	    ItemId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

	    if (ItemId == LIT_NONE)
		break;

	    WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
		   LM_QUERYITEMTEXT, MPFROM2SHORT(ItemId, MAX_LIBPATH_SIZE),
		       MPFROMP(LibPath));

	    WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
		       LM_DELETEITEM, MPFROMSHORT(ItemId), 0L);

	    if (SHORT1FROMMP(mp1) == BID_LIBPATH_DOWN)
		++ItemId;
	    else
		--ItemId;

	    WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_INSERTITEM,
			      MPFROMSHORT(ItemId), MPFROMP(LibPath));

	    WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_SELECTITEM,
			      MPFROMSHORT(ItemId), MPFROMSHORT(TRUE));
	    break;

	case BID_LIBPATH_REMOVE:
	    ItemId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

	    if (ItemId == LIT_NONE)
		break;

	    rc = WinMessageBox(HWND_DESKTOP, hDlg,
			       "Remove selected item from LIBPATH?",
		   ApplTitle, 0, MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);

	    if (rc == MBID_NO)
		break;

	    WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
		       LM_DELETEITEM, MPFROMSHORT(ItemId), 0L);

	    WinSendMsg(hDlg, WM_CONTROL,
		       MPFROM2SHORT(LID_LIBPATH, LN_SELECT), 0L);
	    break;

	case BID_LIBPATH_ADD:
	    WinQueryDlgItemText(hDlg, EID_LIBPATH_NEW,
				sizeof(NewEntry), NewEntry);
	    strupr(NewEntry);
	    StripSpaces(NewEntry);
	    WinSetDlgItemText(hDlg, EID_LIBPATH_NEW, NewEntry);

	    if (StrInListbox(hDlg, LID_LIBPATH, NewEntry))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, "Path already present",
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

		WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_LIBPATH_NEW));
		break;
	    }

	    if (!CheckDirPath(NewEntry, ErrorStr))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, ErrorStr,
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

		WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_LIBPATH_NEW));
		break;
	    }

	    ItemId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

	    if (ItemId == LIT_NONE)
		ItemId = LIT_END;

	    WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_INSERTITEM,
			      MPFROMSHORT(ItemId), MPFROMP(NewEntry));

	    WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_SELECTITEM,
			      MPFROMSHORT(ItemId), MPFROMSHORT(TRUE));

	    if (ItemId == LIT_END)
	    {
		NrOfItems =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
					    LM_QUERYITEMCOUNT, NULL, NULL));
		WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_SETTOPINDEX,
				  MPFROMSHORT(NrOfItems - 1), 0L);
	    }

	    WinSetDlgItemText(hDlg, EID_LIBPATH_NEW, "");
	    break;

	case BID_LIBPATH_MODIFY:
	    WinQueryDlgItemText(hDlg, EID_LIBPATH_NEW,
				sizeof(NewEntry), NewEntry);
	    strupr(NewEntry);
	    StripSpaces(NewEntry);
	    WinSetDlgItemText(hDlg, EID_LIBPATH_NEW, NewEntry);

	    if (StrInListbox(hDlg, LID_LIBPATH, NewEntry))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, "Path already present",
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

		WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_LIBPATH_NEW));
		break;
	    }

	    if (!CheckDirPath(NewEntry, ErrorStr))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, ErrorStr,
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

		WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_LIBPATH_NEW));
		break;
	    }

	    ItemId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

	    if (ItemId == LIT_NONE)
		break;

	    WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_SETITEMTEXT,
			      MPFROMSHORT(ItemId), MPFROMP(NewEntry));
	    WinSendDlgItemMsg(hDlg, LID_LIBPATH, LM_SELECTITEM,
			      MPFROMSHORT(ItemId), MPFROMSHORT(TRUE));

	    WinSetDlgItemText(hDlg, EID_LIBPATH_NEW, "");
	    break;

	case DID_CANCEL:
	    WinDismissDlg(hDlg, FALSE);
	    break;

	case DID_OK:
	    rc = WinMessageBox(HWND_DESKTOP, hDlg, "Update CONFIG.SYS?",
			       ApplTitle, 0, MB_YESNO |
			       MB_ICONQUESTION | MB_APPLMODAL);

	    if (rc == MBID_NO)
		break;

	    NrOfItems =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
					LM_QUERYITEMCOUNT, NULL, NULL));

	    LibPath[0] = '\0';

	    for (i = 0; i < NrOfItems; ++i)
	    {
		WinSendMsg(WinWindowFromID(hDlg, LID_LIBPATH),
			LM_QUERYITEMTEXT, MPFROM2SHORT(i, MAX_LIBPATH_SIZE),
			   MPFROMP(&LibPath[strlen(LibPath)]));
		strcat(LibPath, ";");
	    }

	    if (!UpdateLibPath(LibPath, ErrorStr))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, ErrorStr,
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
		WinDismissDlg(hDlg, FALSE);
		break;
	    }

	    CsChanged = TRUE;

	    if (FileSelected)
	    {
		rc = WinMessageBox(HWND_DESKTOP, hDlg, "Refresh tree window?",
				   ApplTitle, 0, MB_YESNO |
				   MB_ICONQUESTION | MB_APPLMODAL);

		if (rc == MBID_YES)
		    WinPostMsg(hWndClient, WUM_START_REBUILD, 0L, 0L);
	    }

	    WinDismissDlg(hDlg, TRUE);
	    break;
	}
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
    }

    return NULL;
}
