/*
 * Copyright andyr@wizzy.com (Andy Rabagliati)	 30 August 1994
 *
 * This code may be freely used, provided credits remain intact
 *
 */

#define tint_from_link	tint32_from_link
#define UNKNOWN	 -140
#define HALF	 -130
#define DISK	 -120
#define HOST_TAG -110
#define TXXX	 -100
#define BAD16	 -90
#define BAD32	 -80
#define T16	 -70
#define T32	 -60
#define C4	 -50
#define M212	 -40
#define T_212	 -30
#define T_414	-20
#define T414A	-20
#define T414B	-19
#define T_800	-10
#define T800C	-8
#define T800D	-7
#define T_425	0
#define T_805	10
#define T_801	20
#define T_225	40
#define T_400	50
#define qTXXX	((unsigned char)0)
#define qC4	((unsigned char)1)
#define qT16	((unsigned char)2)
#define qT32	((unsigned char)3)
#define qHALF	((unsigned char)4)
#define qM212	((unsigned char)5)
#define qDISK	((unsigned char)6)
#define TAG_SETPATH	((unsigned char)0)
#define TAG_LSPEED	((unsigned char)9)
#define TAG_BOOT	((unsigned char)2)
#define TAG_TEST16	((unsigned char)3)
#define TAG_TEST32	((unsigned char)4)
#define TAG_HALT	((unsigned char)255)
#define ESCAPE	((short)-1)


