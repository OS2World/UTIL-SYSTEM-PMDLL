
/***********************************************************************

sysdll.c	Set system dlls

Copyright (c) 2001 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) sysdll.c - Ver 2, 31-May-01,13:14:34 $

Revisions	26 Apr 95 AVB - Release
		31 May 01 SHL - Correct window procedure arguments drop casts

***********************************************************************/

#define EXTERN extern

#include "pmdll.h"

static CHAR OldSysDllPath[MAX_LIBPATH_SIZE];

static VOID _FillSysDllLb(HWND hDlg, PSZ Path);

MRESULT EXPENTRY SysDllWndProc(HWND hDlg, ULONG Msg,
			       MPARAM mp1, MPARAM mp2)

{
    CHAR LibPath[MAX_LIBPATH_SIZE], NewEntry[51];
    SHORT i;
    SHORT ItemId, NrOfItems;

    switch (Msg)
    {
    case WM_INITDLG:
	strcpy(OldSysDllPath, SysDllPath);

	_FillSysDllLb(hDlg, SysDllPath);

	WinSendDlgItemMsg(hDlg, EID_SYSPATH_NEW, EM_SETTEXTLIMIT,
			  MPFROMSHORT(sizeof(NewEntry) - 1), 0L);
	WinSetDlgItemText(hDlg, EID_SYSPATH_NEW, "");

	CenterDialog(hWndFrame, hDlg);

	WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, LID_SYSPATH));
	return ((MRESULT) TRUE);
	break;

    case WM_CONTROL:
	switch (SHORT1FROMMP(mp1))
	{
	case LID_SYSPATH:
	    if (SHORT2FROMMP(mp1) == LN_SELECT ||
		    SHORT2FROMMP(mp1) == LN_ENTER)
	    {
		ItemId =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

		if (ItemId == LIT_NONE)
		{
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_UP), FALSE);
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_DOWN), FALSE);
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_REMOVE), FALSE);
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_MODIFY), FALSE);
		    break;
		}

		WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_REMOVE), TRUE);
		WinQueryDlgItemText(hDlg, EID_SYSPATH_NEW,
				    sizeof(NewEntry), NewEntry);
		if (EmptyField(NewEntry))
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_MODIFY), FALSE);
		else
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_MODIFY), TRUE);

		if (ItemId == 0)
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_UP), FALSE);
		else
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_UP), TRUE);

		NrOfItems =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
					    LM_QUERYITEMCOUNT, NULL, NULL));

		if (ItemId == NrOfItems - 1)
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_DOWN), FALSE);
		else
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_DOWN), TRUE);

		if (SHORT2FROMMP(mp1) == LN_ENTER)
		{
		    WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
		    LM_QUERYITEMTEXT, MPFROM2SHORT(ItemId, MAX_LIBPATH_SIZE),
			       MPFROMP(LibPath));

		    WinSetDlgItemText(hDlg, EID_SYSPATH_NEW, LibPath);
		}
	    }
	    break;

	case EID_SYSPATH_NEW:
	    if (SHORT2FROMMP(mp1) == EN_CHANGE)
	    {
		WinQueryDlgItemText(hDlg, EID_SYSPATH_NEW,
				    sizeof(NewEntry), NewEntry);

		if (EmptyField(NewEntry))
		{
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_ADD), FALSE);
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_MODIFY), FALSE);
		}
		else
		{
		    WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_ADD), TRUE);
		    ItemId =
			SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

		    if (ItemId != LIT_NONE)
			WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_MODIFY), TRUE);
		}
	    }
	    break;
	}
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case BID_SYSPATH_DOWN:
	case BID_SYSPATH_UP:
	    ItemId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

	    if (ItemId == LIT_NONE)
		break;

	    WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
		   LM_QUERYITEMTEXT, MPFROM2SHORT(ItemId, MAX_LIBPATH_SIZE),
		       MPFROMP(LibPath));

	    WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
		       LM_DELETEITEM, MPFROMSHORT(ItemId), 0L);

	    if (SHORT1FROMMP(mp1) == BID_SYSPATH_DOWN)
		++ItemId;
	    else
		--ItemId;

	    WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_INSERTITEM,
			      MPFROMSHORT(ItemId), MPFROMP(LibPath));

	    WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_SELECTITEM,
			      MPFROMSHORT(ItemId), MPFROMSHORT(TRUE));
	    break;

	case BID_SYSPATH_REMOVE:
	    ItemId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

	    if (ItemId == LIT_NONE)
		break;

	    WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
		       LM_DELETEITEM, MPFROMSHORT(ItemId), 0L);

	    WinSendMsg(hDlg, WM_CONTROL,
		       MPFROM2SHORT(LID_SYSPATH, LN_SELECT), 0L);
	    break;

	case BID_SYSPATH_ADD:
	    WinQueryDlgItemText(hDlg, EID_SYSPATH_NEW,
				sizeof(NewEntry), NewEntry);
	    strupr(NewEntry);
	    StripSpaces(NewEntry);
	    WinSetDlgItemText(hDlg, EID_SYSPATH_NEW, NewEntry);

	    if (StrInListbox(hDlg, LID_SYSPATH, NewEntry))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, "Path already present",
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

		WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_SYSPATH_NEW));
		break;
	    }

	    if (!CheckDirPath(NewEntry, ErrorStr))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, ErrorStr,
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

		WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_SYSPATH_NEW));
		break;
	    }

	    ItemId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

	    if (ItemId == LIT_NONE)
		ItemId = LIT_END;

	    WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_INSERTITEM,
			      MPFROMSHORT(ItemId), MPFROMP(NewEntry));

	    WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_SELECTITEM,
			      MPFROMSHORT(ItemId), MPFROMSHORT(TRUE));

	    if (ItemId == LIT_END)
	    {
		NrOfItems =
		    SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
					    LM_QUERYITEMCOUNT, NULL, NULL));
		WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_SETTOPINDEX,
				  MPFROMSHORT(NrOfItems - 1), 0L);
	    }

	    WinSetDlgItemText(hDlg, EID_SYSPATH_NEW, "");
	    break;

	case BID_SYSPATH_MODIFY:
	    WinQueryDlgItemText(hDlg, EID_SYSPATH_NEW,
				sizeof(NewEntry), NewEntry);
	    strupr(NewEntry);
	    StripSpaces(NewEntry);
	    WinSetDlgItemText(hDlg, EID_SYSPATH_NEW, NewEntry);

	    if (StrInListbox(hDlg, LID_SYSPATH, NewEntry))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, "Path already present",
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

		WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_SYSPATH_NEW));
		break;
	    }

	    if (!CheckDirPath(NewEntry, ErrorStr))
	    {
		WinMessageBox(HWND_DESKTOP, hDlg, ErrorStr,
		   ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

		WinSetFocus(HWND_DESKTOP, WinWindowFromID(hDlg, EID_SYSPATH_NEW));
		break;
	    }

	    ItemId =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
			  LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), NULL));

	    if (ItemId == LIT_NONE)
		break;

	    WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_SETITEMTEXT,
			      MPFROMSHORT(ItemId), MPFROMP(NewEntry));
	    WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_SELECTITEM,
			      MPFROMSHORT(ItemId), MPFROMSHORT(TRUE));

	    WinSetDlgItemText(hDlg, EID_SYSPATH_NEW, "");
	    break;

	case DID_CANCEL:
	    strcpy(SysDllPath, OldSysDllPath);
	    WinDismissDlg(hDlg, FALSE);
	    break;

	case BID_SYSPATH_RECALL:
	    PrfQueryProfileString(HINI_USERPROFILE, PRF_APPL_NAME, PRF_SYSDLL_KEY,
				  "", SysDllPath, sizeof(SysDllPath));
	    _FillSysDllLb(hDlg, SysDllPath);
	    strcpy(PrfSysDllPath, SysDllPath);
	    break;

	case BID_SYSPATH_SAVE:
	case DID_OK:
	    NrOfItems =
		SHORT1FROMMR(WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
					LM_QUERYITEMCOUNT, NULL, NULL));

	    LibPath[0] = '\0';

	    for (i = 0; i < NrOfItems; ++i)
	    {
		WinSendMsg(WinWindowFromID(hDlg, LID_SYSPATH),
			LM_QUERYITEMTEXT, MPFROM2SHORT(i, MAX_LIBPATH_SIZE),
			   MPFROMP(&LibPath[strlen(LibPath)]));
		strcat(LibPath, ";");
	    }

	    strcpy(SysDllPath, LibPath);
	    strupr(SysDllPath);

	    if (SHORT1FROMMP(mp1) == BID_SYSPATH_SAVE)
	    {
		strcpy(PrfSysDllPath, LibPath);
		strupr(PrfSysDllPath);

		PrfWriteProfileString(HINI_USERPROFILE, PRF_APPL_NAME,
				      PRF_SYSDLL_KEY, PrfSysDllPath);
		break;
	    }

	    if (FileSelected)
		WinPostMsg(hWndClient, WUM_START_REBUILD, 0L, 0L);

	    WinDismissDlg(hDlg, TRUE);
	    break;
	}
	break;

    default:
	return WinDefDlgProc(hDlg, Msg, mp1, mp2);
    }

    return NULL;
}

static VOID _FillSysDllLb(HWND hDlg, PSZ Path)

{
    CHAR LibPath[MAX_LIBPATH_SIZE], *pLibPath;
    SHORT i, ItemCount;

    strcpy(LibPath, Path);
    WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_DELETEALL, 0L, 0L);

    i = 0;
    ItemCount = 0;
    pLibPath = LibPath;

    while (LibPath[i])
    {
	if (LibPath[i] == ';')
	{
	    LibPath[i] = '\0';
	    WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_INSERTITEM,
			      MPFROMSHORT(LIT_END), MPFROMP(pLibPath));
	    ItemCount++;
	    pLibPath = &LibPath[i + 1];
	}

	++i;
    }

    if (pLibPath < &LibPath[i])
    {
	WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_INSERTITEM,
			  MPFROMSHORT(LIT_END), MPFROMP(pLibPath));
	ItemCount++;
    }

    if (ItemCount == 0)
    {
	WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_UP), FALSE);
	WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_DOWN), FALSE);
	WinEnableWindow(WinWindowFromID(hDlg, BID_SYSPATH_REMOVE), FALSE);
    }
    else
    {
	WinSendDlgItemMsg(hDlg, LID_SYSPATH, LM_SELECTITEM,
			  MPFROMSHORT(0), MPFROMSHORT(TRUE));
    }

    return;
}
