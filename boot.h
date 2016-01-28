
/* --------------------------------------------------
 *
 *    boot.h  --  boot code for check
 *
 * Copyright andyr@wizzy.com (Andy Rabagliati)	 30 August 1994
 *
 * This code may be freely used, provided credits remain intact
 *
 * --------------------------------------------------
 */



static unsigned char boot[] =
{0x8B,	       /*	  BYTE END-START		     */
	       /*	  ORG  0			     */
	       /* START					     */
 0x25, 0x0A,   /*	  J    INIT-$	 ; bootchan in C     */
 0x21, 0xF5,   /* STOP	  STOPP				     */
	       /*					     */
	       /* EPILOG		 ; index in AREG     */
 0x71,	       /*	  LDL  WS			     */
 0x60, 0xEE,   /*	  STNL -2	 ; BOOTCHAN	     */
 0x72,	       /*	  LDL  VEC			     */
 0x71,	       /*	  LDL  WS			     */
 0x60, 0xEF,   /*	  STNL -1	 ; VEC space pointer */
 0x60, 0x43,   /*	  LDC  STOP-$-2			     */
 0x21, 0xFB,   /*	  LDPI				     */
 0x71,	       /*	  LDL  WS			     */
 0x60, 0xED,   /*	  STNL -3	 ; Clean stop on ret */
 0x71,	       /*	  LDL  WS			     */
 0x70,	       /*	  LDL  BOOTCHAN			     */
 0x73,	       /*	  LDL  LENGTH			     */
 0xF7,	       /*	  IN		 ; read code	     */
 0x71,	       /*	  LDL  WS			     */
 0x60, 0x5D,   /*	  LDNLP -3			     */
 0x23, 0xFC,   /*	  GAJW				     */
 0x34,	       /*	  LDNL ENTRY			     */
 0xF6,	       /*	  GCALL				     */
	       /*					     */
	       /* READCODE				     */
	       /* WS	  EQC  1			     */
	       /* VEC	  EQC  2			     */
	       /* LENGTH  EQC  3			     */
	       /* ENTRY	  EQC  4	 ; byte offset	     */
	       /*					     */
 0x11,	       /*	  LDLP WS			     */
 0x70,	       /*	  LDL  0	 ; bootchan	     */
 0x44,	       /*	  LDC  4			     */
 0x23, 0xF4,   /*	  BCNT				     */
 0xF7,	       /*	  IN				     */
	       /* ; read WS, VEC slots, code length, entry	   */
	       /* ; space is used as follows:-			   */
	       /* ; WS from MEMSTART+4 to MEMSTART+4+((WS+3)*bpw)  */
	       /* ; VEC from WS end to MEMSTART+4+((WS+VEC+3)*bpw) */
	       /* ; CODE					   */
	       /* ; Param 1 Of called PROC is VEC space Ptr	   */
 0x71,	       /*	  LDL  WS			     */
 0x83,	       /*	  ADC  3	 ; RETptr, BOOT, Vec */
 0x62, 0x47,   /*	  LDC  START-$-2 ; MEMSTART	     */
 0x21, 0xFB,   /*	  LDPI		 ; MEMSTART	     */
 0x84,	       /*	  ADC  4	 ; room for STOPP    */
 0xFA,	       /*	  WSUB				     */
 0xD1,	       /*	  STL  WS	 ; Wrksp/Code start  */
 0x61, 0x4D,   /*	  LDC  READCODE-$-2		     */
 0x21, 0xFB,   /*	  LDPI		 ; code after EPILOG */
 0x71,	       /*	  LDL  WS	 ; check overwrite   */
 0x60, 0x5D,   /*	  LDNLP -3	 ; start of params   */
 0xF9,	       /*	  GT				     */
 0xA6,	       /*	  CJ   ENOUGH-$			     */
 0x61, 0x44,   /*	  LDC  READCODE-$-2		     */
 0x21, 0xFB,   /*	  LDPI		 ; code after EPILOG */
 0x53,	       /*	  LDNLP 3			     */
 0xD1,	       /*	  STL  WS	 ; Fix it	     */
 0x71,	       /* ENOUGH  LDL  WS			     */
 0x73,	       /*	  LDL  LENGTH			     */
 0xF2,	       /*	  BSUB		 ; space after code  */
 0x41,	       /*	  LDC  1			     */
 0x23, 0xF4,   /*	  BCNT				     */
 0x60, 0x8F,   /*	  ADC  -1	 ; Bytes per word -1 */
 0xF5,	       /*	  ADD				     */
 0x41,	       /*	  LDC  1			     */
 0x23, 0xF4,   /*	  BCNT				     */
 0x23, 0xF2,   /*	  NOT				     */
 0x81,	       /*	  ADC  1	 ; Word mask	     */
 0x24, 0xF6,   /*	  AND		 ; round up to word  */
 0xD2,	       /*	  STL  VEC	 ; VEC space start   */
 0x74,	       /*	  LDL  ENTRY			     */
 0x71,	       /*	  LDL  WS			     */
 0xF2,	       /*	  BSUB				     */
 0xD4,	       /*	  STL  ENTRY			     */
 0x70,	       /*	  LDL  BOOTCHAN			     */
 0x41,	       /*	  LDC  1			     */
 0x23, 0xF4,   /*	  BCNT		 ; bpw independent   */
 0x22, 0xFC,   /*	  DIV		 ; Bytes Per Word    */
 0x43,	       /*	  LDC  3	 ; leave last 2 bits */
 0x24, 0xF6,   /*	  AND		 ; Index of boot chan*/
 0x65, 0x08,   /*	  J    EPILOG-$			     */
	       /*	  ALIGN 4	 ; INIT might be WPTR*/
	       /* MINT	  EQC  1			     */
	       /* BOOTCHAN EQC	0			     */
 0xD0,	       /* INIT	  STL  0			     */
 0xD0,	       /*	  STL  0			     */
 0x66, 0x4E,   /*	  LDC  START-$-2		     */
 0x21, 0xFB,   /*	  LDPI				     */
 0x60, 0x5B,   /*	  LDNLP -5			     */
 0x23, 0xFC,   /*	  GAJW				     */
 0xD0,	       /*	  STL  0			     */
 0xD0,	       /*	  STL  BOOTCHAN			     */
 0x24, 0xF2,   /*	  MINT				     */
 0xD1,	       /*	  STL  MINT			     */
 0x71,	       /*	  LDL  MINT			     */
 0x21, 0xFC,   /*	  STLF		 ; lo priority Q     */
 0x71,	       /*	  LDL  MINT			     */
 0x21, 0xF8,   /*	  STHF		 ; hi		     */
 0x22, 0xF9,   /*	  TESTERR			     */
 0x25, 0xF7,   /*	  CLRHALTERR			     */
 0x40,	       /*	  LDC  0			     */
 0x25, 0xF4,   /*	  STTIMER	 ; clock	     */
 0x40,	       /*	  LDC  0  ; loop to init Links,Timers*/
 0xD2,	       /*	  STL  2			     */
 0x4B,	       /*	  LDC  11			     */
 0xD3,	       /*	  STL  3			     */
 0x71,	       /* L101A	  LDL  MINT			     */
 0x72,	       /*	  LDL  2			     */
 0x71,	       /*	  LDL  MINT			     */
 0xFA,	       /*	  WSUB				     */
 0xE0,	       /*	  STNL 0			     */
 0x12,	       /*	  LDLP 2			     */
 0x49,	       /*	  LDC  LXXX-L101A		     */
 0x22, 0xF1,   /*	  LEND		 ; end		     */
 0x66, 0x06,   /* LXXX	  J    READCODE-$		     */
	       /*	  ; padding for timeslice to tread on*/
 0,0,0,0       /*	  BYTE 0,0,0,0			     */
	       /* END					     */ };


/*
 *  eof
 */
