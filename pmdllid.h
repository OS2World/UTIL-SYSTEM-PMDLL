
/***********************************************************************

pmdllid.h	Resource Definitions

Copyright (c) 2001 Steven Levine and Associates, Inc.
Copyright (c) 1995 Arthur Van Beek
All rights reserved.

   $TLIB$: $ &(#) %n - Ver %v, %f $
   TLIB: $ &(#) pmdllid.h - Ver 2, 26-Jun-01,18:14:54 $

Revisions	16 Jul 95 AVB -	Release
		26 Jun 01 SHL -	Add MIT_USE_SYSDLLS

***********************************************************************/

#define	FID_PMDLL		  100
#define	MID_FILE		  110
#define	MIT_ABOUT		  111
#define	MIT_FILE_OPEN		  112
#define	MIT_FILE_REFRESH	  113
#define	MIT_FILE_TRACE		  114
//#define MIT_FILE_DETAIL	  115
#define	MIT_FIND_DLL		  116
#define	MIT_PRINT		  117
#define	MIT_EXIT		  119
#define	MID_OPTIONS		  120
#define	MIT_CUR_DIR		  121
#define	MIT_SYSTEM_DLLS		  122
#define	MIT_FONT		  123
#define	MIT_LIBPATH		  124
#define	MIT_TEST_LOAD		  125
#define	MIT_TREE_VIEW		  126
#define	MIT_RESOURCES		  127
#define	MIT_LOAD_SYSTEM_DLLS	  128

#define	FID_TREE		  200

#define	IID_PMDLL		  100

#define	DID_SELECT_FILE		 1100
#define	TID_SELECT_FILE_PATH	 1101
#define	LID_SELECT_FILE_DRIVES	 1102
#define	LID_SELECT_FILE_DIRS	 1103
#define	LID_SELECT_FILE_FILES	 1104

#define	DID_SELECT_FONT		 1200
#define	LID_SELECT_FONT		 1201

#define	DID_BUILDING_TREE	 1300
#define	TID_BUILDING_TREE	 1301

#define	DID_EDIT_SYSPATH	 1400
#define	LID_SYSPATH		 1401
#define	BID_SYSPATH_UP		 1402
#define	BID_SYSPATH_DOWN	 1403
#define	BID_SYSPATH_REMOVE	 1404
#define	BID_SYSPATH_ADD		 1405
#define	BID_SYSPATH_MODIFY	 1406
#define	BID_SYSPATH_SAVE	 1407
#define	BID_SYSPATH_RECALL	 1408
#define	EID_SYSPATH_NEW		 1409

#define	DID_PRINT		 1500
#define	EID_PRINT_PW		 1501
#define	EID_PRINT_PL		 1502
#define	RID_PRINT_LPT1		 1503
#define	RID_PRINT_LPT2		 1504
#define	RID_PRINT_FILE		 1505
#define	EID_PRINT_FILE_NAME	 1506

#define	DID_EDIT_LIBPATH	 1600
#define	LID_LIBPATH		 1601
#define	BID_LIBPATH_UP		 1602
#define	BID_LIBPATH_DOWN	 1603
#define	BID_LIBPATH_REMOVE	 1604
#define	BID_LIBPATH_ADD		 1605
#define	BID_LIBPATH_MODIFY	 1606
#define	EID_LIBPATH_NEW		 1607

#define	DID_FIND_DLL		 1700
#define	EID_FIND_DLL		 1701
#define	TID_FIND_DLL		 1702
#define	BID_FIND_DLL		 1703
#define	TID_FIND_DLL_DATE	 1704
#define	TID_FIND_DLL_SIZE	 1705

#define	DID_CHANGE_CUR_DIR	 1800
#define	TID_CD_PATH		 1801
#define	LID_CD_DRIVES		 1802
#define	LID_CD_DIRS		 1803

#define	DID_TRACE_DLL		 1900
#define	TID_TRACE_EXE		 1901
#define	TID_TRACE_DLL_STARTDIR	 1902
#define	EID_TRACE_EXE_PARM	 1903
#define	RID_TRACE_DLL_SKIP_SYS	 1904
#define	LID_TRACE_DLL		 1905
#define	GID_TRACE		 1906
#define	BID_TRACE_START		 1907
#define	BID_TRACE_STOP		 1908
#define	BID_TRACE_OK		 1909

#define	DID_DETAILS_DLL		 2000
#define	TID_DETAILS_DLL_MOD_NAME 2001
#define	TID_DETAILS_DLL_1632	 2002
#define	TID_DETAILS_DLL_DESCR	 2003
#define	LID_DETAILS_DLL_EXP_RES	 2004
#define	LID_DETAILS_DLL_EXP_NRES 2005
#define	LID_DETAILS_DLL_IMP_MOD	 2006
#define	LID_DETAILS_DLL_IMP_PROC 2007

#define	DID_RESOURCE		 2100
#define	EID_RES_MAX_IMPORTS	 2101
#define	EID_RES_MAX_DLLS	 2102
#define	EID_RES_MAX_DEPTH	 2103
#define	BID_RES_DEFAULTS	 2104

#define	DID_ABOUT		 9900
#define	TID_PROGRAM_NAME	 9901

// The end
