
/***********************************************************************

sel_font.c	Load/select font

Copyright (c) 2001 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) sel_font.c - Ver 4, 28-Aug-01,13:39:48 $

Revisions	18 Jun 94 AVB - Release
		10 May 01 SHL - LoadFonts: use FM_... defines
		23 May 01 SHL - LoadFonts: make SYSMONO key optional
		31 May 01 SHL - Correct window proc arguments, drop casts
		28 Aug 01 SHL - LoadFonts: rework error detect/reporting
		28 Aug 01 SHL - SelectFontWndProc: show font name

***********************************************************************/

#define EXTERN extern

#include "pmdll.h"

static VOID _DrawItemFontLb(HWND, SHORT, MPARAM);
static USHORT _CreateLogFont(HPS, LONG, SHORT);

#define FONT_ID_LB		1
#define FONT_ID_CLIENT_AREA	2


MRESULT EXPENTRY SelectFontWndProc(HWND hDlg, ULONG Msg,
				   MPARAM mp1, MPARAM mp2)
{
    SHORT i;
    SHORT Height;

    switch (Msg)
    {
    case WM_INITDLG:
	for (i = 0; i < NrLoadedFonts; ++i)
	{
	    CHAR sz[FACESIZE + 20];
	    sprintf(sz, "%s %upt", fm[i].szFacename, fm[i].lEmHeight);
	    WinSendDlgItemMsg(hDlg, LID_SELECT_FONT, LM_INSERTITEM,
			      MPFROM2SHORT(LIT_END, 0),
			      MPFROMP(sz));
	}

	WinSendDlgItemMsg(hDlg, LID_SELECT_FONT, LM_SELECTITEM,
			  MPFROMSHORT((SHORT) FontEntry),
			  MPFROMSHORT(TRUE));

	CenterDialog(hWndFrame, hDlg);
	break;

    case WM_MEASUREITEM:
	Height = 0;

	for (i = 0; i < NrLoadedFonts; ++i)
	{
	    if ((SHORT) (fm[i].lMaxBaselineExt + fm[i].lExternalLeading) > Height)
		Height = (SHORT) (fm[i].lMaxBaselineExt + fm[i].lExternalLeading);
	}

	return (MRFROM2SHORT(Height, 0));
	break;

    case WM_DRAWITEM:
	_DrawItemFontLb(hDlg, LID_SELECT_FONT, mp2);
	return (MRFROMSHORT(TRUE));
	break;

    case WM_CONTROL:
	switch (SHORT1FROMMP(mp1))
	{
	case LID_SELECT_FONT:
	    if (SHORT2FROMMP(mp1) == LN_ENTER)
		WinPostMsg(hDlg, WM_COMMAND, MPFROMSHORT(DID_OK), 0L);
	    break;
	}
	break;

    case WM_COMMAND:
	switch (SHORT1FROMMP(mp1))
	{
	case DID_OK:
	    i = SHORT1FROMMR(WinSendDlgItemMsg(hDlg, LID_SELECT_FONT,
					       LM_QUERYSELECTION,
					     MPFROMSHORT(LIT_FIRST), NULL));
	    if (i != LIT_NONE)
		FontEntry = i;

	    WinDismissDlg(hDlg, TRUE);
	    break;

	case DID_CANCEL:
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

static VOID _DrawItemFontLb(HWND hDlg, SHORT LbIdent, MPARAM mp2)
{
    HWND hListbox;
    HPS hPSLb;
    POWNERITEM Listbox;
    static FONTMETRICS FmLb;
    POINTL LbPos;
    CHAR LbText[30];
    SHORT FmEntryLb;

    Listbox = (POWNERITEM) PVOIDFROMMP(mp2);

    hPSLb = Listbox -> hps;

    FmEntryLb = (SHORT) Listbox -> idItem;

    if (_CreateLogFont(hPSLb, FONT_ID_LB, FmEntryLb))
	DosBeep(1000, 1000);

    GpiQueryFontMetrics(hPSLb, sizeof(FONTMETRICS), &FmLb);

    GpiSetCharSet(hPSLb, FONT_ID_LB);

    LbPos.x = Listbox -> rclItem.xLeft;
    LbPos.y = Listbox -> rclItem.yBottom + FmLb.lMaxDescender;
    if (Listbox -> fsStateOld == Listbox -> fsState)
    {
	hListbox = WinWindowFromID(hDlg, LbIdent);
	WinSendMsg(hListbox, LM_QUERYITEMTEXT,
		   MPFROM2SHORT(Listbox -> idItem, 200),
		   MPFROMP(LbText));

	WinFillRect(hPSLb, &(Listbox -> rclItem), CLR_BACKGROUND);
	GpiCharStringAt(hPSLb, &LbPos, strlen(LbText),
			LbText);
    }

    GpiDeleteSetId(hPSLb, FONT_ID_LB);
    GpiSetCharSet(hPSLb, LCID_DEFAULT);

    return;
}

static USHORT _CreateLogFont(HPS hPS, LONG FontId, SHORT FmEntry)
{
    static LONG lMaxFonts;
    static LONG lOldFont;
    LONG Result;
    ERRORID ErrorId;
    FATTRS Fat;

    if (FmEntry > MAX_FONTS)
	return (1);

    lMaxFonts = MAX_FONTS;
    lOldFont = FontId;

    if (!fm[FmEntry].szFacename[0])
	return (4);

    if (GpiSetCharSet(hPS, 0L) == GPI_ERROR)
	return (5);

    GpiDeleteSetId(hPS, lOldFont);

    Fat.usRecordLength = sizeof(FATTRS);
    Fat.lMatch = 0;
    Fat.fsSelection = fm[FmEntry].fsSelection;
    strcpy(Fat.szFacename, fm[FmEntry].szFacename);
    Fat.idRegistry = fm[FmEntry].idRegistry;
    Fat.usCodePage = 850;
    Fat.lMaxBaselineExt = fm[FmEntry].lMaxBaselineExt;
    Fat.lAveCharWidth = fm[FmEntry].lAveCharWidth;
    Fat.fsType = 0;
    Fat.fsFontUse = 0;

    Result = GpiCreateLogFont(hPS, (PSTR8) NULL,
			      lOldFont, &Fat);

    if (Result != 2)
    {
	if (Result == 0)
	{
	    ErrorId = WinGetLastError(hAB);
	    return ((USHORT) ErrorId);
	}

	if (Result == 1)
	    return (7);
    }

    return (0);
}

VOID LoadFonts(VOID)
{
    LONG lMaxFonts;
    APIRET rc;
    LONG lRemFonts;
    HPS hps;
    SHORT i;
    RECTL rDesktop;
    CHAR FontPath[CCHMAXPATH];

    PrfQueryProfileString(HINI_USERPROFILE, "PM_Fonts", "SYSMONO",
			  "NOT FOUND", FontPath, CCHMAXPATH);

    if (strcmp(FontPath, "NOT FOUND") != 0)
    {
	if (GpiLoadFonts(hAB, FontPath) != GPI_OK)
	{
	    rc = WinGetLastError(hAB);
	    AskShutWithMessage(rc, "Can not load SYSMONO fonts (0x%lx)", rc);
	}
    }

    lMaxFonts = MAX_FONTS;

    hps = WinGetPS(HWND_DESKTOP);
    if (hps == NULLHANDLE)
    {
	rc = WinGetLastError(hAB);
	ShutWithMessage(rc, "WinGetPS failed (0x%lx)", rc);
    }

    lRemFonts = GpiQueryFonts(hps, QF_PUBLIC | QF_PRIVATE,
			      NULL, &lMaxFonts,
			      sizeof(FONTMETRICS), fm);

    if (lRemFonts == GPI_ALTERROR)
    {
	rc = WinGetLastError(hAB);
	WinReleasePS(hps);
	ShutWithMessage(rc, "GpiQueryFonts failed (0x%lx)", rc);
    }

    if (!WinReleasePS(hps))
    {
	rc = WinGetLastError(hAB);
	ShutWithMessage(rc, "WinReleasePS failed (0x%lx)", rc);
    }

    /* Locate usable monospaced fonts */

    for (i = 0; i < (SHORT)lMaxFonts; ++i)
    {
	// Pick fixed-width bit-mapped standard
	if (fm[i].fsType & FM_TYPE_FIXED &&
	    ~fm[i].fsDefn & FM_DEFN_OUTLINE &&
	    !(fm[i].fsSelection &
	      (FM_SEL_ITALIC|FM_SEL_UNDERSCORE|FM_SEL_NEGATIVE|
	       FM_SEL_OUTLINE|FM_SEL_STRIKEOUT|FM_SEL_BOLD)))
	{
	    fm[NrLoadedFonts] = fm[i];
	    ++NrLoadedFonts;
	}
    }

    if (NrLoadedFonts == 0)
	ShutWithMessage(1, "Loaded %lu fonts.  No useful fonts found.", lRemFonts);

    /* Select monospaced font */

    if (PrfFontNr > (USHORT)NrLoadedFonts)
    {
	// Pick new default
	FontEntry = -1;
	WinQueryWindowRect(HWND_DESKTOP, &rDesktop);

	for (i = 0; i < NrLoadedFonts; ++i)
	{
	    if (rDesktop.xRight > 640)
	    {
		/*---- XGA, SVGA ----*/
		if (strcmp(fm[i].szFacename, "System Monospaced") == 0 &&
			fm[i].lEmHeight == 16L &&
			fm[i].lAveCharWidth == 9L)
		{
		    FontEntry = i;
		    FontOffset = (SHORT) fm[FontEntry].lMaxDescender;
		    FontHeight = (SHORT) (fm[FontEntry].lMaxBaselineExt +
					  fm[FontEntry].lExternalLeading);
		    FontWidth = (SHORT) fm[FontEntry].lAveCharWidth;
		    break;
		}
	    }
	    else
	    {
		/*---- VGA ----*/
		if (strcmp(fm[i].szFacename, "System Monospaced") == 0 &&
			fm[i].lEmHeight == 13L &&
			fm[i].lAveCharWidth == 8L)
		{
		    FontEntry = i;
		    FontOffset = (SHORT) fm[FontEntry].lMaxDescender;
		    FontHeight = (SHORT) (fm[FontEntry].lMaxBaselineExt +
					  fm[FontEntry].lExternalLeading);
		    FontWidth = (SHORT) fm[FontEntry].lAveCharWidth;
		    break;
		}
	    }
	} // for
	if (FontEntry < 0)
	    ShutWithMessage(1, "Loaded %lu fonts.  Can not select default for screen resolution.", lRemFonts);
    }
    else
    {
	FontEntry = PrfFontNr;
	FontOffset = (SHORT) fm[FontEntry].lMaxDescender;
	FontHeight = (SHORT) (fm[FontEntry].lMaxBaselineExt +
			      fm[FontEntry].lExternalLeading);
	FontWidth = (SHORT) fm[FontEntry].lAveCharWidth;
    }


} /* LoadFonts */

USHORT ChangeFont(HWND hWnd, HPS * hPS, USHORT Cp)
{
    static LONG lMaxFonts;
    static LONG lOldFont;
    LONG result;
    SIZEL Size;
    HDC HdcInit;
    FATTRS fat;

    if (*hPS == (HPS) NULL)
    {
/*---- initialize PS ----*/

	HdcInit = WinOpenWindowDC(hWnd);
	Size.cx = 0;
	Size.cy = 0;
	*hPS = GpiCreatePS(hAB, HdcInit, &Size,
			   PU_PELS | GPIF_DEFAULT |
			   GPIT_NORMAL | GPIA_ASSOC);

	if (*hPS == GPI_ERROR)
	    return (1);
    }

    if (FontEntry > MAX_FONTS)
	return (2);

    lMaxFonts = MAX_FONTS;
    lOldFont = FONT_ID_CLIENT_AREA;

    if (strlen(fm[FontEntry].szFacename) == 0)
	return (3);

    if (GpiSetCharSet(*hPS, 0L) == GPI_ERROR)
	return (5);

    GpiDeleteSetId(*hPS, lOldFont);

    fat.usRecordLength = sizeof(FATTRS);
    fat.lMatch = 0;
    fat.fsSelection = fm[FontEntry].fsSelection;
    strcpy(fat.szFacename, fm[FontEntry].szFacename);
    fat.idRegistry = fm[FontEntry].idRegistry;
    fat.usCodePage = Cp;
    fat.lMaxBaselineExt = fm[FontEntry].lMaxBaselineExt;
    fat.lAveCharWidth = fm[FontEntry].lAveCharWidth;
    fat.fsType = 0;
    fat.fsFontUse = 0;

    result = GpiCreateLogFont(*hPS, (PSTR8) NULL,
			      lOldFont, &fat);

    if (result != 2)
    {
	if (result == 0)
	    return ((USHORT) WinGetLastError(hAB));

	if (result == 1)
	    return (7);
    }

    if (GpiSetCharSet(*hPS, lOldFont) == GPI_ERROR)
	return (4);

    FontOffset = (SHORT) fm[FontEntry].lMaxDescender;
    FontHeight = (SHORT) (fm[FontEntry].lMaxBaselineExt +
			  fm[FontEntry].lExternalLeading);
    FontWidth = (SHORT) fm[FontEntry].lAveCharWidth;

    return (0);
}

/* The end */
