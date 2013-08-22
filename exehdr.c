
/***********************************************************************

exehdr.c - Exe header processing routines
$Id: exehdr.c,v 1.9 2013/04/03 00:12:42 Steven Exp $

Copyright (c) 2001, 2013 Steven Levine and Associates, Inc.
Copyright (c) 1996 Arthur Van Beek
All rights reserved.

1996-03-31 AVB Baseline
2001-05-25 SHL Support LX without 16-bit stub
2001-05-31 SHL Correct window procedure arguments drop casts
2006-03-15 YD Show error reason
2012-04-22 SHL Avoid OpenWatcom warnings
2013-03-24 SHL Optimize ExeHdrGetImports
2013-03-24 SHL Enhance file open dialog to include all executables
2013-03-24 SHL Supply .DLL only if no extension (Gregg Young)

***********************************************************************/

#define INCL_DOSSESMGR

#include "pmdll.h"

#define OFFSET_EXE_HEADER	60

static BOOL _ExeHdrGetHeader(PSZ pszPgmName, PBOOL pfIs32Bit,
			     PULONG pulOffsetExeHdr,
			     PEXE_HEADER32 pExeHdr32,
			     PEXE_HEADER16 pExeHdr16,
			     PSZ pszError);

BOOL ExeHdrGetImports(PSZ pszPgmName, BOOL fTestLoadModuleParm, PSZ pszLibpath,
		      PBOOL pfIs32Bit, PUSHORT pusNrOfEntries,
		      PIMPORT_TABLE_DATA pImportTable, PSZ pszError)

{
    EXE_HEADER32 ExeHdr32;
    EXE_HEADER16 ExeHdr16;
    FILESTATUS FileStatus;
    HMODULE hMod;
    HFILE hFile;
    APIRET rc;
    ULONG Action;
    ULONG BytesRead;
    ULONG NewPtr;
    ULONG TableOffset;
    ULONG TableEntries;
    ULONG ModRefTableOffset;
    ULONG OffsetExeHeader;
    BYTE EntryLength;
    USHORT i;
    USHORT usValue;
    USHORT MaxEntries;
    CHAR *pStr;
    CHAR *pStr2;
    UCHAR szDLLName[CCHMAXPATH];

    MaxEntries = *pusNrOfEntries;
    memset(pImportTable, 0x00, (MaxEntries) * sizeof(IMPORT_TABLE_DATA));

    if (!_ExeHdrGetHeader(pszPgmName, pfIs32Bit, &OffsetExeHeader,
			  &ExeHdr32, &ExeHdr16, pszError))
    {
	return FALSE;
    }

    rc = DosOpen(pszPgmName, &hFile, &Action, 0L,
		 FILE_NORMAL, FILE_OPEN,
		 OPEN_ACCESS_READONLY | OPEN_SHARE_DENYWRITE, 0L);

    if (rc)
    {
	sprintf(pszError, "Could not open file (%lu)", rc);
	return (FALSE);
    }

    if (*pfIs32Bit)
    {
	TableOffset = OffsetExeHeader + ExeHdr32.ImportModTableOffset;
	TableEntries = ExeHdr32.NrOfImportModEntries;

	if ((ULONG) *pusNrOfEntries < TableEntries)
	{
	    sprintf(pszError, "Import table too small %u (%lu needed), increase 'Maximum number of Imports per DLL' setting",
		    *pusNrOfEntries, TableEntries);
	    DosClose(hFile);
	    return (FALSE);
	}

	rc = DosChgFilePtr(hFile, TableOffset, FILE_BEGIN, &NewPtr);

	if (rc || NewPtr != TableOffset)
	{
	    sprintf(pszError, "Could not change file pointer to %lu (%u)",
		    TableOffset, rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	*pusNrOfEntries = 0;

	for (i = 0; i < (USHORT) TableEntries; ++i)
	{
	    rc = DosRead(hFile, &EntryLength, sizeof(BYTE), &BytesRead);

	    if (rc || BytesRead != sizeof(BYTE))
	    {
		sprintf(pszError, "Could not read entry table length (%u)", rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    if ((USHORT) EntryLength == 0 ||
		    (USHORT) EntryLength > MAX_DLL_ENTRY_LENGTH)
	    {
		sprintf(pszError, "Invalid table entry length (%u)",
			(USHORT) EntryLength);
		DosClose(hFile);
		return (FALSE);
	    }

	    // 2013-03-24 SHL optimize
	    pStr = pImportTable[*pusNrOfEntries].Name;
	    rc = DosRead(hFile, pStr,
			 (USHORT) EntryLength, &BytesRead);

	    if (rc || BytesRead != (USHORT) EntryLength)
	    {
		sprintf(pszError, "Could not read entry table name (%u)",
			rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    pStr[BytesRead] = 0;
	    strupr(pStr);

	    if (strcmp(pStr, "DOSCALLS") == 0)
		strcpy(pStr, "DOSCALL1");

	    // 2013-03-24 SHL default extension if no extension
	    pStr2 = strrchr(pStr, '.');
	    if (!pStr2)
		strcat(pStr, ".DLL");	// Default extension

	    pStr2 = strrchr(pszPgmName, '\\');

	    if (pStr2 == NULL)
		pStr2 = pszPgmName;
	    else
		++pStr2;

	    if (strcmp(pStr2, pStr))
		++(*pusNrOfEntries);	// Count new entry
	} // for
    } // if 32-bit
    else
    {
	// 16-bit
	TableEntries = ExeHdr16.NrOfModRefTableEntries;

	if ((ULONG) *pusNrOfEntries < TableEntries)
	{
	    sprintf(pszError, "Import table too small %u (%lu needed), increase 'Maximum number of Imports per DLL' setiing",
		    *pusNrOfEntries, TableEntries);
	    DosClose(hFile);
	    return (FALSE);
	}

	TableOffset = OffsetExeHeader + (ULONG) ExeHdr16.ImpNamesTableOffset;
	ModRefTableOffset = OffsetExeHeader + (ULONG) ExeHdr16.ModRefTableOffset;

	*pusNrOfEntries = 0;

	for (i = 0; i < (USHORT) TableEntries; ++i)
	{
	    rc = DosChgFilePtr(hFile, ModRefTableOffset + i * sizeof(USHORT),
			       FILE_BEGIN, &NewPtr);

	    if (rc || NewPtr != ModRefTableOffset + i * sizeof(USHORT))
	    {
		sprintf(pszError, "Could not change file pointer to %lu (%u)",
			ModRefTableOffset + i * sizeof(USHORT), rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    rc = DosRead(hFile, &usValue, sizeof(USHORT), &BytesRead);

	    if (rc || BytesRead != sizeof(USHORT))
	    {
		sprintf(pszError, "Could not read entry table length (%u)", rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    rc = DosChgFilePtr(hFile, TableOffset + usValue, FILE_BEGIN, &NewPtr);

	    if (rc || NewPtr != TableOffset + usValue)
	    {
		sprintf(pszError, "Could not change file pointer to %lu (%u)",
			TableOffset + usValue, rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    rc = DosRead(hFile, &EntryLength, sizeof(BYTE), &BytesRead);

	    if (rc || BytesRead != sizeof(BYTE))
	    {
		sprintf(pszError, "Could not read entry table length (%u)", rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    if ((USHORT) EntryLength == 0 ||
		    (USHORT) EntryLength > MAX_DLL_ENTRY_LENGTH)
	    {
		sprintf(pszError, "Invalid entry length (%u)", (USHORT) EntryLength);
		DosClose(hFile);
		return (FALSE);
	    }

	    // 2013-03-24 SHL optimize
	    pStr = pImportTable[*pusNrOfEntries].Name;
	    rc = DosRead(hFile, pStr,
			 (USHORT) EntryLength, &BytesRead);

	    if (rc || BytesRead != (USHORT) EntryLength)
	    {
		sprintf(pszError, "Could not read entry table name (%u)", rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    pStr[BytesRead] = 0;
	    strupr(pStr);

	    if (strcmp(pStr, "DOSCALLS") == 0)
		strcpy(pStr, "DOSCALL1");	// Force name

	    // 2013-03-24 SHL default extension if no extension
	    pStr2 = strrchr(pStr, '.');
	    if (!pStr2)
		strcat(pStr, ".DLL");	// Default extension

	    pStr2 = strrchr(pszPgmName, '\\');

	    if (pStr2 == NULL)
		pStr2 = pszPgmName;
	    else
		++pStr2;

	    if (strcmp(pStr2, pStr))
		++(*pusNrOfEntries);	// Count new entry
	} // for
    } // if 16-bit

    DosClose(hFile);

    for (i = 0; i < *pusNrOfEntries; ++i)
    {
	pImportTable[i].Dynamic = FALSE;

	rc = DosSearchPath(SEARCH_PATH | SEARCH_IGNORENETERRS,
			   pszLibpath,
			   pImportTable[i].Name,
			   (PUCHAR)pImportTable[i].Path,
			   CCHMAXPATH);

	if (rc)
	{
	    pImportTable[i].PathFound = FALSE;
	    pImportTable[i].Loadable = FALSE;

	    if (rc == ERROR_FILE_NOT_FOUND)
		strcpy(pImportTable[i].Path, "Path to DLL not in LIBPATH");
	    else
		sprintf(pImportTable[i].Path, "Error %u searching path", rc);
	}
	else
	{
	    pImportTable[i].PathFound = TRUE;

	    DosQPathInfo(pImportTable[i].Path, FIL_STANDARD, (PBYTE) & FileStatus,
			 sizeof(FILESTATUS));
	    pImportTable[i].FileSize = FileStatus.cbFile;
	    pImportTable[i].fdateLastWrite = FileStatus.fdateLastWrite;
	    pImportTable[i].ftimeLastWrite = FileStatus.ftimeLastWrite;

	    rc = DosSearchPath(SEARCH_PATH | SEARCH_IGNORENETERRS, SysDllPath,
			       pImportTable[i].Name, szDLLName, CCHMAXPATH);

	    if (rc || fLoadSysDlls)
	    {
		// Not a system DLL or loading system DLLs
		pImportTable[i].SystemDll = FALSE;

		if (fTestLoadModuleParm)
		{
		    strcpy(pImportTable[i].LoadErrorCause, "Unknown");
		    rc = DosLoadModule(pImportTable[i].LoadErrorCause, CCHMAXPATH,
				       pImportTable[i].Path, &hMod);
		    if (rc)
		    {
			if (rc == ERROR_FILE_NOT_FOUND)
			{
			    pImportTable[i].Loadable = TRUE;
			    pImportTable[i].LoadError = 0;
			    pImportTable[i].FreeError = 0;
			}
			else
			{
			    pImportTable[i].Loadable = FALSE;
			    pImportTable[i].LoadError = rc;
			    pImportTable[i].FreeError = 0;
			}
		    }
		    else
		    {
			pImportTable[i].Loadable = TRUE;
			pImportTable[i].LoadError = 0;
			pImportTable[i].FreeError = DosFreeModule(hMod);
		    }
		}
		else
		{
		    pImportTable[i].SystemDll = FALSE;
		    pImportTable[i].Loadable = TRUE;
		}
	    }
	    else
	    {
		// Is a system DLL
		pImportTable[i].SystemDll = TRUE;
		pImportTable[i].Loadable = TRUE;
	    }
	}
    }

    return (TRUE);
}

static BOOL _ExeHdrGetHeader(PSZ pszPgmName, PBOOL pfIs32Bit,
			     PULONG pulOffsetExeHdr,
			     PEXE_HEADER32 pExeHdr32,
			     PEXE_HEADER16 pExeHdr16,
			     PSZ pszError)

{
    HFILE hFile;
    APIRET rc;
    ULONG Action, BytesRead, NewPtr;
    CHAR Buffer[OFFSET_EXE_HEADER + sizeof(ULONG)];

    rc = DosOpen(pszPgmName, &hFile, &Action, 0L,
		 FILE_NORMAL, FILE_OPEN,
		 OPEN_ACCESS_READONLY | OPEN_SHARE_DENYWRITE, 0L);

    if (rc)
    {
	sprintf(pszError, "Could not open file (%lu)", rc);
	return (FALSE);
    }

    rc = DosRead(hFile, &Buffer, sizeof(Buffer), &BytesRead);
    if (rc)
    {
	sprintf(pszError, "Could not read file (%lu)", rc);
	DosClose(hFile);
	return (FALSE);
    }

    if (BytesRead < sizeof(Buffer))
    {
	sprintf(pszError, "Invalid exe/dll size (%lu bytes)", BytesRead);
	DosClose(hFile);
	return (FALSE);
    }

    if (strncmp(&Buffer[0], "LX", 2) == 0)
	*pulOffsetExeHdr = 0;		// No 16-bit stub
    else
    {
	if (strncmp(&Buffer[0], "MZ", 2))
	{
	    sprintf(pszError, "File is not an executable or dll ('%0.2s')", &Buffer[0]);
	    DosClose(hFile);
	    return (FALSE);
	}

	/*--- 16 or 32 bit program ?? ---*/

	memcpy(pulOffsetExeHdr, &Buffer[OFFSET_EXE_HEADER], sizeof(ULONG));

	rc = DosChgFilePtr(hFile, *pulOffsetExeHdr, FILE_BEGIN, &NewPtr);

	if (rc || NewPtr != *pulOffsetExeHdr)
	{
	    sprintf(pszError, "Could not jump to EXE header offset (%lu, %lu)",
		    *pulOffsetExeHdr, rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	rc = DosRead(hFile, &Buffer, 2, &BytesRead);
	if (rc)
	{
	    sprintf(pszError, "Could not read file (%lu)", rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	if (BytesRead != 2)
	{
	    sprintf(pszError, "Invalid exe/dll size (%lu bytes)", BytesRead);
	    DosClose(hFile);
	    return (FALSE);
	}
    }

    if (strncmp(Buffer, "LX", 2) == 0)
	*pfIs32Bit = TRUE;
    else
    {
	if (strncmp(Buffer, "NE", 2) != 0)
	{
	    if (strncmp(Buffer, "MZ", 2) != 0)
		sprintf(pszError, "Unsupported exe type ('%0.2s')", Buffer);
	    else
		sprintf(pszError, "EXE is a DOS executable");

	    DosClose(hFile);
	    return (FALSE);
	}

	*pfIs32Bit = FALSE;
    }

    rc = DosChgFilePtr(hFile, *pulOffsetExeHdr, FILE_BEGIN, &NewPtr);

    if (rc || NewPtr != *pulOffsetExeHdr)
    {
	sprintf(pszError, "Could no jump to EXE header offset (%lu, %lu)",
		*pulOffsetExeHdr, rc);
	DosClose(hFile);
	return (FALSE);
    }

    if (*pfIs32Bit)
    {
	rc = DosRead(hFile, pExeHdr32, sizeof(EXE_HEADER32), &BytesRead);
	if (rc)
	{
	    sprintf(pszError, "Could not read exe header (%lu)", rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	if (BytesRead != sizeof(EXE_HEADER32))
	{
	    sprintf(pszError, "Could not completely read exe header (%lu)", BytesRead);
	    DosClose(hFile);
	    return (FALSE);
	}
    }
    else
    {
	rc = DosRead(hFile, pExeHdr16, sizeof(EXE_HEADER16), &BytesRead);
	if (rc)
	{
	    sprintf(pszError, "Could not read exe header (%lu)", rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	if (BytesRead != sizeof(EXE_HEADER16))
	{
	    sprintf(pszError, "Could not completely read exe header (%lu)", BytesRead);
	    DosClose(hFile);
	    return (FALSE);
	}
    }

    DosClose(hFile);

    return (TRUE);
}

BOOL ExeHdrGetExports(PSZ pszPgmName, PBOOL pfIs32Bit, PUSHORT pusNrOfEntries,
		      PEXPORT_TABLE_DATA ExportTable, PSZ ModuleName,
		      PSZ ModuleDescr, PSZ pszError)

{
    EXE_HEADER32 ExeHdr32;
    EXE_HEADER16 ExeHdr16;
    EXPORT_TABLE_DATA SwapData;
    HFILE hFile;
    APIRET rc;
    ULONG Action, BytesRead, NewPtr, TableOffset, OffsetExeHeader;
    BYTE EntryLength;
    USHORT i, j, Lowest, MaxEntries;

    memset(ExportTable, 0x00, (*pusNrOfEntries) * sizeof(EXPORT_TABLE_DATA));
    strcpy(ModuleName, "");
    strcpy(ModuleDescr, "");
    MaxEntries = *pusNrOfEntries;

    if (!_ExeHdrGetHeader(pszPgmName, pfIs32Bit, &OffsetExeHeader,
			  &ExeHdr32, &ExeHdr16, pszError))
	return (FALSE);

    rc = DosOpen(pszPgmName, &hFile, &Action, 0L,
		 FILE_NORMAL, FILE_OPEN,
		 OPEN_ACCESS_READONLY | OPEN_SHARE_DENYWRITE, 0L);

    if (rc)
    {
	sprintf(pszError, "Could not open file (%lu)", rc);
	return (FALSE);
    }

    if (*pfIs32Bit)
	TableOffset = OffsetExeHeader + ExeHdr32.ResNameTableOffset;
    else
	TableOffset = OffsetExeHeader + ExeHdr16.ResNameTableOffset;

    rc = DosChgFilePtr(hFile, TableOffset, FILE_BEGIN, &NewPtr);

    if (rc || NewPtr != TableOffset)
    {
	sprintf(pszError, "Could not change file pointer to %lu (%u)",
		TableOffset, rc);
	DosClose(hFile);
	return (FALSE);
    }

    /*--- read module name ---*/

    rc = DosRead(hFile, &EntryLength, sizeof(BYTE), &BytesRead);

    if (rc || BytesRead != sizeof(BYTE))
    {
	sprintf(pszError, "Could not read entry table length (%u)", rc);
	DosClose(hFile);
	return (FALSE);
    }

    if ((USHORT) EntryLength == 0 ||
	    (USHORT) EntryLength > MAX_DLL_ENTRY_LENGTH)
    {
	sprintf(pszError, "Invalid table entry length (%u)",
		(USHORT) EntryLength);
	DosClose(hFile);
	return (FALSE);
    }

    rc = DosRead(hFile, ModuleName, (USHORT) EntryLength, &BytesRead);

    if (rc || BytesRead != (USHORT) EntryLength)
    {
	sprintf(pszError, "Could not read module name (%u)", rc);
	DosClose(hFile);
	return (FALSE);
    }

    ModuleName[EntryLength] = '\0';

    *pusNrOfEntries = 0;
    rc = DosChgFilePtr(hFile, TableOffset + EntryLength + 3,
		       FILE_BEGIN, &NewPtr);

    if (rc || NewPtr != TableOffset + EntryLength + 3)
    {
	sprintf(pszError, "Could not change file pointer to %lu (%u)",
		TableOffset, rc);
	DosClose(hFile);
	return (FALSE);
    }

    /*--- read exports ---*/

    if (*pfIs32Bit)
    {
	sprintf(pszError, "EntryTableOffset: %lu\nOffsetExeHeader: %lu",
		ExeHdr32.EntryTableOffset, OffsetExeHeader);
	DosClose(hFile);
	return (FALSE);
    }

    for (;;)
    {
	rc = DosRead(hFile, &EntryLength, sizeof(BYTE), &BytesRead);

	if (rc || BytesRead != sizeof(BYTE))
	{
	    sprintf(pszError, "Could not read entry table length (%u)", rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	if ((USHORT) EntryLength == 0)
	    break;

	if ((USHORT) EntryLength > MAX_EXPORT_NAME_LENGTH)
	{
	    sprintf(pszError, "Invalid table entry length (%u) %u",
		    (USHORT) EntryLength, *pusNrOfEntries);
	    DosClose(hFile);
	    return (FALSE);
	}

	rc = DosRead(hFile, ExportTable[*pusNrOfEntries].Name, (USHORT) EntryLength,
		     &BytesRead);

	if (rc || BytesRead != (USHORT) EntryLength)
	{
	    sprintf(pszError, "Could not read module name (%u)", rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	rc = DosRead(hFile, &ExportTable[*pusNrOfEntries].Ordinal, sizeof(USHORT),
		     &BytesRead);

	if (rc || BytesRead != sizeof(USHORT))
	{
	    sprintf(pszError, "Could not read ordinal (%u)", rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	ExportTable[*pusNrOfEntries].Resident = TRUE;

	++(*pusNrOfEntries);

	if (*pusNrOfEntries >= MaxEntries)
	{
	    sprintf(pszError, "Too much entries in resident name table (> %lu)",
		    MaxEntries);
	    DosClose(hFile);
	    return (FALSE);
	}
    }

    /*--- non-resident name table ---*/

    if ((*pfIs32Bit && ExeHdr32.NonResNameTableLen) ||
	    (!(*pfIs32Bit) && ExeHdr16.NonResNameTableLen))
    {
	if (*pfIs32Bit)
	    TableOffset = ExeHdr32.NonResNameTableOffset;
	else
	    TableOffset = ExeHdr16.NonResNameTableOffset;

	rc = DosChgFilePtr(hFile, TableOffset, FILE_BEGIN, &NewPtr);

	if (rc || NewPtr != TableOffset)
	{
	    sprintf(pszError, "Could not change file pointer to %lu (%u)",
		    TableOffset, rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	rc = DosRead(hFile, &EntryLength, sizeof(BYTE), &BytesRead);

	if (rc || BytesRead != sizeof(BYTE))
	{
	    sprintf(pszError, "Could not read entry table length (%u)", rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	if ((USHORT) EntryLength > MAX_EXPORT_NAME_LENGTH)
	{
	    sprintf(pszError, "Invalid table entry length (%u) %u",
		    (USHORT) EntryLength, *pusNrOfEntries);
	    DosClose(hFile);
	    return (FALSE);
	}

	rc = DosRead(hFile, ModuleDescr, (USHORT) EntryLength, &BytesRead);

	if (rc || BytesRead != (USHORT) EntryLength)
	{
	    sprintf(pszError, "Could not read module description (%u)", rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	ModuleDescr[EntryLength] = '\0';

	rc = DosChgFilePtr(hFile, TableOffset + EntryLength + 3,
			   FILE_BEGIN, &NewPtr);

	if (rc || NewPtr != TableOffset + EntryLength + 3)
	{
	    sprintf(pszError, "Could not change file pointer to %lu (%u)",
		    TableOffset, rc);
	    DosClose(hFile);
	    return (FALSE);
	}

	/*--- read exports ---*/

	for (;;)
	{
	    rc = DosRead(hFile, &EntryLength, sizeof(BYTE), &BytesRead);

	    if (rc || BytesRead != sizeof(BYTE))
	    {
		sprintf(pszError, "Could not read entry table length (%u)", rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    if ((USHORT) EntryLength == 0)
		break;

	    if ((USHORT) EntryLength > MAX_EXPORT_NAME_LENGTH)
	    {
		sprintf(pszError, "Invalid table entry length (%u) %u",
			(USHORT) EntryLength, *pusNrOfEntries);
		DosClose(hFile);
		return (FALSE);
	    }

	    rc = DosRead(hFile, ExportTable[*pusNrOfEntries].Name, (USHORT) EntryLength,
			 &BytesRead);

	    if (rc || BytesRead != (USHORT) EntryLength)
	    {
		sprintf(pszError, "Could not read module name (%u)", rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    rc = DosRead(hFile, &ExportTable[*pusNrOfEntries].Ordinal, sizeof(USHORT),
			 &BytesRead);

	    if (rc || BytesRead != sizeof(USHORT))
	    {
		sprintf(pszError, "Could not read ordinal (%u)", rc);
		DosClose(hFile);
		return (FALSE);
	    }

	    ExportTable[*pusNrOfEntries].Resident = FALSE;

	    ++(*pusNrOfEntries);

	    if (*pusNrOfEntries >= MaxEntries)
	    {
		sprintf(pszError, "Too much entries in resident name table (> %lu)",
			MaxEntries);
		DosClose(hFile);
		return (FALSE);
	    }
	}
    }

    DosClose(hFile);

    /*--- sort entries ---*/

    for (i = 0; i < *pusNrOfEntries; ++i)
    {
	Lowest = i;
	SwapData = ExportTable[i];

	for (j = (USHORT) (i + 1); j < *pusNrOfEntries; ++j)
	{
	    if (ExportTable[j].Ordinal < SwapData.Ordinal)
	    {
		Lowest = j;
		SwapData = ExportTable[j];
	    }
	}

	ExportTable[Lowest] = ExportTable[i];
	ExportTable[i] = SwapData;
    }

    return (TRUE);
}

// The end
