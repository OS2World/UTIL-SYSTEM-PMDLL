#define EXTERN extern
#include "pmdll.h"


#define MAX_EXPORT_ENTRIES   1024
#define MAX_IMPORT_MODULES    256

static BOOL _FillExportedProcedures (HWND hDlg, USHORT DetailEntry,
                                     PSZ ErrorStr);
static BOOL _FillImportedModules (HWND hDlg, USHORT DetailEntry, PSZ ErrorStr);



MRESULT EXPENTRY DetailsDllWndProc (HWND hDlg, USHORT Msg, MPARAM mp1, MPARAM mp2)

{
static PSHORT pDetailEntry;
CHAR   TitleStr[41];


switch (Msg)
  {
  case WM_INITDLG:
       pDetailEntry = PVOIDFROMMP (mp2);

       strcpy (TitleStr, "Details ");
       strcat (TitleStr, pShowTableData[*pDetailEntry].Name);
       WinSetWindowText (hDlg, TitleStr);


       /*--- get current libpath --*/

       if (!GetLibPath (Libpath, MAX_LIBPATH_SIZE, ErrorStr))
         {
         WinMessageBox (HWND_DESKTOP, hWndFrame, ErrorStr,
		        ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
         WinDismissDlg (hDlg, FALSE);
         break;
         }


       /*--- fill exported procedures ---*/

       if (!_FillExportedProcedures (hDlg, *pDetailEntry, ErrorStr))
         {
         WinMessageBox (HWND_DESKTOP, hWndFrame, ErrorStr,
		        ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
         WinDismissDlg (hDlg, FALSE);
         break;
         }


       /*--- fill imported modules ---*/

       if (!_FillImportedModules (hDlg, *pDetailEntry, ErrorStr))
         {
         WinMessageBox (HWND_DESKTOP, hWndFrame, ErrorStr,
		        ApplTitle, 0, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
         WinDismissDlg (hDlg, FALSE);
         break;
         }


       CenterDialog (HWND_DESKTOP, hDlg);

       WinSetFocus (HWND_DESKTOP, WinWindowFromID (hDlg, LID_DETAILS_DLL_EXP_RES));
       return ((MRESULT) TRUE);


  case WM_COMMAND:
        switch (SHORT1FROMMP(mp1))
	 {
	 case DID_CANCEL:
	      WinDismissDlg (hDlg, FALSE);
	      break;
	 }
       break;


  default:
       return WinDefDlgProc (hDlg, Msg, mp1, mp2);
  }


return NULL;
}





static BOOL _FillExportedProcedures (HWND hDlg, USHORT DetailEntry,
                                     PSZ ErrorStr)

{
PEXPORT_TABLE_DATA pExportTable;
CHAR   ModuleName[9], LbStr[MAX_EXPORT_NAME_LENGTH + 20];
APIRET rc;
USHORT NrOfEntries, i;
BOOL   Bit32, Result;


rc = DosAllocMem ((PPVOID) &pExportTable,
       		  MAX_EXPORT_ENTRIES * sizeof (EXPORT_TABLE_DATA),
        	  PAG_READ | PAG_WRITE | PAG_COMMIT);

if (rc)
  {
  strcpy (ErrorStr, "Couldn't allocate memory");
  return (FALSE);
  }


NrOfEntries = MAX_EXPORT_ENTRIES;

CursorWait ();
Result = ExeHdrGetExports (pShowTableData[DetailEntry].Path, &Bit32,
                           &NrOfEntries, pExportTable, ModuleName,
                           LbStr, ErrorStr);
CursorNormal ();

if (!Result)
  {
  DosFreeMem (pExportTable);
  return (FALSE);
  }


WinSetDlgItemText (hDlg, TID_DETAILS_DLL_MOD_NAME, ModuleName);

if (Bit32)
  WinSetDlgItemText (hDlg, TID_DETAILS_DLL_1632, "32");
else
  WinSetDlgItemText (hDlg, TID_DETAILS_DLL_1632, "16");

WinSetDlgItemText (hDlg, TID_DETAILS_DLL_DESCR, LbStr);

for (i = 0; i < NrOfEntries; ++i)
  {
  sprintf (LbStr, "@%u  %s", pExportTable[i].Ordinal,
           pExportTable[i].Name);

  if (pExportTable[i].Resident)
    {
    WinSendDlgItemMsg (hDlg, LID_DETAILS_DLL_EXP_RES, LM_INSERTITEM,
                       MPFROMSHORT (LIT_END), MPFROMP (LbStr));
    }
  else
    {
    WinSendDlgItemMsg (hDlg, LID_DETAILS_DLL_EXP_NRES, LM_INSERTITEM,
	               MPFROMSHORT (LIT_END), MPFROMP (LbStr));
    }
  }


DosFreeMem (pExportTable);


return (TRUE);
}






static BOOL _FillImportedModules (HWND hDlg, USHORT DetailEntry, PSZ ErrorStr)

{
PIMPORT_TABLE_DATA pImportTable;
APIRET rc;
USHORT NrOfEntries, i;
BOOL   Bit32, Result;


rc = DosAllocMem ((PPVOID) &pImportTable,
       		  MAX_IMPORT_MODULES * sizeof (IMPORT_TABLE_DATA),
        	  PAG_READ | PAG_WRITE | PAG_COMMIT);

if (rc)
  {
  strcpy (ErrorStr, "Couldn't allocate memory");
  return (FALSE);
  }


NrOfEntries = MAX_IMPORT_MODULES;

CursorWait ();
Result = ExeHdrGetImports (pShowTableData[DetailEntry].Path, FALSE, Libpath,
                           &Bit32, &NrOfEntries, pImportTable, ErrorStr);
CursorNormal ();

if (!Result)
  {
  DosFreeMem (pImportTable);
  return (FALSE);
  }


for (i = 0; i < NrOfEntries; ++i)
  {
  WinSendDlgItemMsg (hDlg, LID_DETAILS_DLL_IMP_MOD, LM_INSERTITEM,
                     MPFROMSHORT (LIT_END), MPFROMP (pImportTable[i].Name));
  }


DosFreeMem (pImportTable);


return (TRUE);
}


