struct debug_buffer
  {
   ULONG   Pid;        /* Debuggee Process ID */
   ULONG   Tid;        /* Debuggee Thread ID */
   LONG    Cmd;        /* Command or Notification */
   LONG    Value;      /* Generic Data Value */
   ULONG   Addr;       /* Debuggee Address */
   ULONG   Buffer;     /* Debugger Buffer Address */
   ULONG   Len;        /* Length of Range */
   ULONG   Index;      /* Generic Identifier Index */
   ULONG   MTE;        /* Module Handle */
   ULONG   EAX;        /* Register Set */
   ULONG   ECX;
   ULONG   EDX;
   ULONG   EBX;
   ULONG   ESP;
   ULONG   EBP;
   ULONG   ESI;
   ULONG   EDI;
   ULONG   EFlags;
   ULONG   EIP;
   ULONG   CSLim;      /* Byte Granular Limits */
   ULONG   CSBase;     /* Byte Granular Base */
   UCHAR   CSAcc;      /* Access Bytes */
   UCHAR   CSAtr;      /* Attribute Bytes */
   USHORT  CS;
   ULONG   DSLim;
   ULONG   DSBase;
   UCHAR   DSAcc;
   UCHAR   DSAtr;
   USHORT  DS;
   ULONG   ESLim;
   ULONG   ESBase;
   UCHAR   ESAcc;
   UCHAR   ESAtr;
   USHORT  ES;
   ULONG   FSLim;
   ULONG   FSBase;
   UCHAR   FSAcc;
   UCHAR   FSAtr;
   USHORT  FS;
   ULONG   GSLim;
   ULONG   GSBase;
   UCHAR   GSAcc;
   UCHAR   GSAtr;
   USHORT  GS;
   ULONG   SSLim;
   ULONG   SSBase;
   UCHAR   SSAcc;
   UCHAR   SSAtr;
   USHORT  SS;
  } DebugBuf;


#define DBG_C_Null          0
#define DBG_C_Go            7
#define DBG_C_Term          8
#define DBG_C_Connect      21
#define DBG_C_Continue     27

#define DBG_N_SUCCESS       0
#define DBG_N_ERROR        -1
#define DBG_N_ProcTerm     -6
#define DBG_N_Exception    -7
#define DBG_N_ModuleLoad   -8
#define DBG_N_CoError      -9
#define DBG_N_ThreadTerm   -10
#define DBG_N_AsyncStop    -11
#define DBG_N_NewProc      -12
#define DBG_N_AliasFree    -13
#define DBG_N_Watchpoint   -14
#define DBG_N_ThreadCreate -15
#define DBG_N_ModuleFree   -16
#define DBG_N_RangeStep    -17

