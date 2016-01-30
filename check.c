/*{{{  header */
/*
 * --------------------------------------------------
 *
 * check.c  --  check program
 * 
 * renamed to ispy - cos its a better name
 *
 * Andy Rabagliati INMOS
 * 
 * Modified by Jeremy Thorp
 *
 * Adapted for Autronica, TRONDHEIM, Norway, under contract to
 * 0yvind Teig <teig@autronica.no> for link on their boards.
 *
 *
 * Copyright andyr@wizzy.com (Andy Rabagliati)         30 August 1994
 *                                                    18 January 1995
 * 
 * 3.24 - Updated November 2014 John Snowdon <john.snowdon@newcastle.ac.uk)
 *			- Several fixes to work within Linux 3.x.x
 * 3.25 - Updated January 2016 John Snowdon <john.snowdon@newcastle.ac.uk)
 *			- Start to tidy up code in prep for development of Python version
 *
 * This code may be freely used, provided above credits remain intact
 * the CREDITS #define below is kept. Any modifications must be
 * distributed in source.
 * 
 * --------------------------------------------------
 */

char *PROGRAM_NAME = "ispy";

#define VERSION_NUMBER  "3.25"
#define VERSION_DATE    "January 2016"
#define CREDITS         "by Andy Rabagliati <andyr@wizzy.com>\nchanges to work with Linux 3.x.x by John Snowdon <john.snowdon@newcastle.ac.uk>\n"
#define WWW             "<URL https://github.com/megatron-uk/>"
#define AUTRONICA       "originally for AUTRONICA, Trondheim, Norway"
#define MyBanner "   # Part rate Link# [  Link0  Link1  Link2  Link3 ]"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "linkio.h"

/* for isatty */
#ifdef MSDOS
#include <io.h>
#else
#ifndef isatty
#include <unistd.h>
#endif
#endif
#include <ctype.h>
#include <malloc.h>
#include <string.h>

#include "inmos.h"
#include "iserver.h"
#undef PROGRAM_NAME         /* remove one defined in iserver.h */
#include "checklib.h"
#include "cklib.h"

#include "type16.h"
#include "type32.h"
#include "check16.h"
#include "check32.h"

#define SWITCHAR 	'-'
#define SEGSIZE  	511
#define EMPTY   	(-1)
#define MAXLENGTH 	1024          /* maximum route length */
#define PERLINE 	9
#define CocoPops        0
#define VerboseSwitch   1

#define DEBUG(x) { if (CocoPops) { fputs("(", stdout); printf x; fputs(")", stdout); fputc((VerboseSwitch ? '\n' : ' '), stdout); fflush(stdout); } }
#define INFO(x) { if (VerboseSwitch) printf x ; fflush(stdout); }

void Usage(void)
{
	printf("\nUsage:  %s [%coption...] [ < file ]    --  version %s of %s.\n\n", PROGRAM_NAME, SWITCHAR, VERSION_NUMBER, VERSION_DATE);
	printf("        This tests the network connectivity\n");
	printf("        and can compare this to a file from a previous run\n\n");
	printf("Options:\n");
	printf(" %cn        :  do not reset the root transputer\n", SWITCHAR);
	printf(" %cr        :  reset the root transputer subsystem\n", SWITCHAR);
	printf(" %cc4       :  read the state of all C004s found\n", SWITCHAR);
	printf(" %ccl       :  read the state of C004s, long form\n", SWITCHAR);
	printf(" %ccr       :  reset all C004s found\n", SWITCHAR);
	printf(" %ccs       :  set all C004s in filename\n", SWITCHAR);
	printf(" %cl <name> :  use this link, else TRANSPUTER envr. var, else %s\n", SWITCHAR, DEFAULTLINK);
	printf(" %cx        :  Ignores any file piped in to %s\n", SWITCHAR, PROGRAM_NAME);
	printf(" %ci        :  Information - gives progress reports\n", SWITCHAR);
	printf(" %ch        :  This help page\n\n", SWITCHAR);
	printf( "                                     %s\n",     AUTRONICA);
	printf( "%s %s\n",     CREDITS, WWW);
	
	exit(0);
}

static unsigned char SSRESETHI[] =
{0,                             /* poke */
 0, 0, 0, 0,                    /* reset */
 1, 0, 0, 0};                   /* asserted */
static unsigned char SSRESETLO[] =
{0,                             /* poke */
 0, 0, 0, 0,                    /* reset */
 0, 0, 0, 0};                   /* de-asserted */
static unsigned char SSANALYSEHI[] =
{0,                             /* poke */
 1, 0, 0, 0,                    /* analyse */
 1, 0, 0, 0};                   /* asserted */
static unsigned char SSANALYSELO[] =
{0,                             /* poke */
 1, 0, 0, 0,                    /* analyse */
 0, 0, 0, 0};                   /* de-asserted */

static unsigned char BOOTSTRING[] =
{23,                            /* length of bootstrap            * ls 2 bits
				 * 1, so its a data     * packet for 1/2
				 * speed links    */
 0xB1,                          /* AJW 1, allow for Iptr store    */
 0xD1,                          /* STL 1                          */
 0x24, 0xF2,                    /* MINT                           */
 0x21, 0xFC,                    /* STHF                           */
 0x24, 0xF2,                    /* MINT                           */
 0x21, 0xF8,                    /* STLF                           */
 0xF0,                          /* REV                            */
 0x60, 0x5C,                    /* LDNLP -4                       */
 0x2A, 0x2A, 0x2A, 0x4A,        /* LDC #AAAA                      */
 0xFF,                          /* OUTWORD                        */
 0x21, 0x2F, 0xFF,              /* START                          */
 0x02, 0x00};                   /* C004 read link                 */

int             kong = TRUE, c4read = 0;
struct tpstats *root;
LINK            TheLink;
char            LinkName[64];
char            FileName[256];

const char *byte_to_binary(int x) {
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1) {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }
    return b;
}

void readC004(struct c4stats * q) {
	unsigned int    length;
	unsigned char   query[2];
	int             i;
	query[0] = 8;
	query[1] = (char) q->route;
	setroute(TheLink, q->parent, 4);
	if (!sendiserver(TheLink, 2, query)) {
		AbortExit(PROGRAM_NAME, "Failed to ask for C004 %d type", q->tpid);
	}
	if (!getiserver(TheLink, &length, query, 2)) {
		AbortExit(PROGRAM_NAME, "Failed to read C004 %d type", q->tpid);
	}
	if (length == 1) {
		q->tptype = C4 + query[0] - 'a';
	}
	query[0] = 2;
	query[1] = (unsigned char) q->route;
	if (!sendiserver(TheLink, (unsigned int) 2, query)) {
		AbortExit(PROGRAM_NAME, "Failed to query C004 %d", q->tpid);
	}
	if (!getiserver(TheLink, &length, (unsigned char *) q->config, 32)) {
		AbortExit(PROGRAM_NAME, "C004 %d failed to respond to query", q->tpid);
	}
	for (i = 0; i < 32; i++) {
		if (q->tptype == C4) {
			/* C004a */
			q->config[i] = (char) ((int) q->config[i] & 0x1F);
		} else {
			/* C004b */
			q->config[i] = (char) (((int) q->config[i] & 0x9F) ^ 0x80);
		}
		if (q->config[i] < 0) {
			q->config[i] = EMPTY;
		}
	}
}

void writeresults(struct tpstats * root, char *LinkName, int c4read, int kong)
{
	struct tpstats *p;
	int error = FALSE;
	if (LinkName[0])
		printf("Using %s ", LinkName);
	printf("%s %s (%s)\n%s", PROGRAM_NAME, VERSION_NUMBER, VERSION_DATE, MyBanner);
	if (kong)
		printf(" CHANGES \n");
	else
		printf(" \n");
	for (p = root; p != NULL; p = p->next)
	{
		if (class(p->tptype) != C4)
		{
			error = writeup(p, c4read, 0);
			putchar('\n');
		} 
		else
		{
			if (c4read == 1)
			{
				writeC4((struct c4stats *) p);
				putchar('\n');
			} 
			else
			{
				if (c4read == 2)
				{
					writeCl((struct c4stats *) p);
					putchar('\n');
				}
			}
		}
	}
}

void test_root_int_mem(const unsigned int bpw, const unsigned char byte_value)
{
/* buffer 0 1 2 3  4  5  6  7  8  9 10 11 12
   32 bit       PP <  addr  >  <   data    >
   16 bit       PP <ad>  <dt>                    */

  unsigned char   buffer[30];
  int i, j;

  buffer[3] = 0;
  buffer[4] = buffer[5] = buffer[6] = 0;
  i = 3 + bpw;
  buffer[i++] = 0x80;
  for (j = 0; j < bpw; j++)
    buffer[i++] = byte_value;
  for (i = 0; i < 2048; i += bpw)
  {
    WriteLink(TheLink, &buffer[3], (2 * bpw) + 1, TIMEOUT);
      if (!(buffer[4] += bpw))
	buffer[5]++;
  }
  buffer[3] = 1;
  buffer[4] = buffer[5] = buffer[6] = 0;
  i = 3 + bpw;
  buffer[i] = 0x80;
  for (i = 0; i < 2048; i += bpw)
  {
    unsigned int byte_count;
    unsigned char mem_value[4];

    WriteLink(TheLink, &buffer[3], bpw + 1, TIMEOUT);
    readbytes(TheLink, mem_value, bpw);
    if (!(buffer[4] += bpw))
	buffer[5]++;
    for (byte_count = 0; byte_count < bpw; byte_count++)
      if (mem_value[byte_count] != byte_value)
	AbortExit(PROGRAM_NAME, "Root processor internal RAM failure");
  }
}

int findtype(void) {
	unsigned char   buffer[30];
	int             bytes, read_result, going;
	int             type = 0;
	char            bpw;
	
	bytes = WriteLink(TheLink, BOOTSTRING, sizeof(BOOTSTRING), TIMEOUT);
	if (bytes == sizeof(BOOTSTRING)) {
		int count = 0;
		bytes = 0;
		going = TRUE;
		while (going) {
			read_result = ReadLink(TheLink, &buffer[bytes], 1, 1);
			count++;
			if (count >= 30) {
				going = FALSE;
			} else if (read_result == 1) {
				bytes++;
			} else if (read_result != ER_LINK_NOSYNC) {
				going = FALSE;
			}
		}
		
		if (bytes == 0) {
			AbortExit(PROGRAM_NAME, "Failed to determine Root Transputer type (result %d)", read_result);
		}
		
		switch (bytes) {
			case 1:
				return (C4);
				break;
			case 2:
				if ((buffer[0] == 0xAA) && (buffer[1] == 0xAA))	{
					type = T16;
					bpw = 2;
				} else {
					AbortExit(PROGRAM_NAME, "Failed to determine Root Transputer type, received %d bytes", bytes);
				}
				break;
			case 4:
				if ((buffer[0] == 0xAA) && (buffer[1] == 0xAA))	{
					type = T32;
					bpw = 4;
				} else {
					AbortExit(PROGRAM_NAME, "Failed to determine Root Transputer type, received %d bytes", bytes);
				}
				break;
			default:
				AbortExit(PROGRAM_NAME, "Failed to determine Root Transputer type, received %d bytes", bytes);
		}
	} else {
		AbortExit(PROGRAM_NAME, "Failed to boot Root Transputer, only took %d byte(s)", bytes);
		/* test_root_int_mem((int) bpw, (unsigned char) 0x55);
		test_root_int_mem((int) bpw, (unsigned char) 0xAA); */
	}
	return (type);
}

void ramtest(struct tpstats * p, int link) {
	unsigned char   testcommand[3];
	setroute(TheLink, p, link);
	testcommand[0] = 0xFF;
	testcommand[1] = 0xFF;
	switch (p->linkno[link]) {
		case T16:
			testcommand[2] = TAG_TEST16;
			break;
		case T32:
			testcommand[2] = TAG_TEST32;
			break;
	}
	if (WriteLink(TheLink, testcommand, 3, TIMEOUT) != 3) {
		writeresults(root, LinkName, c4read, kong);
		AbortExit(PROGRAM_NAME, "Partial results : Timed out sending RAMTEST to processor %d", p->tpid);
	}
}       

void linkspeed(struct tpstats * p){
	static unsigned char LINKSPEED[] = {0xFF, 0xFF, TAG_LSPEED};
	setroute(TheLink, p->parent, p->route);
	if (WriteLink(TheLink, LINKSPEED, 3, TIMEOUT) == 3){
		return;
	} else {
		writeresults(root, LinkName, c4read, kong);
		AbortExit(PROGRAM_NAME, "Partial results : Timed out testing link speed for processor %d", p->tpid);
	}
} 

void sendid(struct tpstats * p) {
	unsigned char   int16[2];
	int16[0] = (unsigned char) p->tpid & 0xFF;
	int16[1] = (unsigned char) p->tpid >> 8;
	setroute(TheLink, p->parent, p->route);
	if (!sendiserver(TheLink, 2, int16)) {
		writeresults(root, LinkName, c4read, kong);
		AbortExit(PROGRAM_NAME, "Partial results : Failed to send Transputer id to processor %d", p->tpid);
	}
}

struct tpstats *create(struct tpstats * parent, int link) {
    struct tpstats *p;
    int             i;

    for (p = root; p->next != NULL; p = p->next);
    if (parent->linkno[link] == C4) {
	p->next = (struct tpstats *) Malloc(sizeof(struct c4stats));
	p->next->tpid = p->tpid + 1;
	p = p->next;
    } else {
	p->next = (struct tpstats *) Malloc(sizeof(struct tpstats));
	p->next->tpid = p->tpid + 1;
	p = p->next;
	p->procspeed = 0;
	p->bootlink = 255;
	p->linkspeed = 0.0f;
	for (i = 0; i < 4; i++){
	    p->links[i] = NULL;
	    p->linkno[i] = UNKNOWN;
	}
    }
    p->next = NULL;
    parent->links[link] = p;
    p->routelen = parent->routelen + 1;
    p->tptype = parent->linkno[link];
    p->parent = parent;
    p->route = link;
    p->info[0] = '\0';
    return (p);
}                               /* create */


void
getstats(struct tpstats *p) {
	unsigned char   string[6];
	unsigned int    i = 0;
	int             tptype;
	if ((getiserver(TheLink, &i, string, sizeof(string))) && (i == 6)) {
		tptype = string[0] + ((char) string[1] << 8);
		if (p->tptype == T32) {
			p->tptype = tptype;
		} else if ((p->tptype == T16) && (class(tptype) == T_414)) {
			p->tptype = T_212;
		}
		p->bootlink = string[3];
		p->links[p->bootlink] = p->parent;
		if (p->parent != NULL) {
			p->linkno[p->bootlink] = p->route;
			p->parent->linkno[p->route] = p->bootlink;
		} else {
			p->linkno[p->bootlink] = HOST_TAG;
		}
		p->procspeed = string[2];
		p->linkspeed = (float) (string[4] + (string[5] << 8));
		if (p->linkspeed != 0.0f) {
			p->linkspeed = (float) (256.0E6 / p->linkspeed);
		}
	} else {
		writeresults(root, LinkName, c4read, kong);
		AbortExit(PROGRAM_NAME, "%d Partial results : Error reading Transputer %d type information", i, p->tpid);
	}
}

int checksum(struct tpstats * p, int link) {
	while (p->parent != NULL) {
		link = (link << 2) + p->route;
		p = p->parent;
	}
	return (link);
}

int nextcandidate(struct tpstats ** child, struct tpstats ** parent)
{
    int             success;
    unsigned char   i;
    for (*parent = root, success = FALSE;
	 (!success) && (*parent != NULL);
	 *parent = (*parent)->next)
	if (class((*parent)->tptype) != C4)
	    for (i = 0; (!success) && (i < 4); i++)
		switch ((*parent)->linkno[i])
		{
		    case T16:
		    case T32:
		    	success = TRUE;
		    	*child = create(*parent, i);
		    	break;
		    default:
		    	success = FALSE;
		    	break;
		}
    return (success);
}

void solve(struct tpstats * current, int length, unsigned char *buffer) {
    struct tpstats *p;
    int             id;

    /*
    printf("solve: processor %d [", current->tpid);
    for (id=0; id<length; id++) {
    	printf("%d ", buffer[id]);
	}
	printf("]\n");
	*/
	
    switch (buffer[0]) {
		case qHALF:
			if (length == 2) {
				current->linkno[buffer[1]] = HALF;
			}
			break;
		case qTXXX:
			switch (length) {
				case 2:
					current->linkno[buffer[1]] = TXXX;
					break;
				case 3: /* myself booting myself */
					current->linkno[buffer[1]] = buffer[2];
					current->linkno[buffer[2]] = buffer[1];
					current->links[buffer[1]] = current;
					current->links[buffer[2]] = current;
					break;
				case 5: /* another booted transputer */
					id = ((int) buffer[2] << 8) + (int) buffer[1];
					for (p = root; (p != NULL) && (p->tpid != id); p = p->next);
					if (p == NULL) {
						writeresults(root, LinkName, c4read, kong);
						AbortExit(PROGRAM_NAME, " (%d:%d-%d:%d) : Check link connections", current->tpid, buffer[4], id, buffer[3]);
					}
					current->links[buffer[4]] = p;
					current->linkno[buffer[4]] = buffer[3];
					p->links[buffer[3]] = current;
					p->linkno[buffer[3]] = buffer[4];
					break;
			}
			break;
		case qDISK:
			if ((current->tptype == T_212) && (buffer[1] == 2)) {
				current->tptype = M212;
				current->linkno[2] = DISK;
				break;
			} else {
				current->linkno[buffer[1]] = UNKNOWN;
				break;
			}
			/* first look says T16, T32, C4 */
		case qT32:
			current->linkno[buffer[1]] = T32;
			break;
		case qT16:
			current->linkno[buffer[1]] = T16;
			break;
		case qC4:
			current->linkno[buffer[1]] = C4;
			break;
    }
}

void check(int subsys, int c4read, int c4reset, int information, int do_reset) {
    /* int             c004count = 0, c004s[100]; */
    do {
        int i, success;
        struct tpstats * parent, * p;
        if (do_reset) {
            if (information) {
                INFO((", Reset root"));
            }
            success = ResetLink(TheLink);
            if (success < 0) {
                AbortExit(PROGRAM_NAME, "reset link failed (result = %i)", success);
            }
        }

        if (subsys) {
            long delay;
            if (information) {
                INFO((", re"));
            }
            if (WriteLink(TheLink, SSRESETLO, sizeof(SSRESETLO), TIMEOUT) != sizeof(SSRESETLO)) {
                AbortExit(PROGRAM_NAME, "Failed to reset Subsystem port");
            }
            for (delay = 0; delay < 100000; delay++) {
                /* this is a delay loop */
            }
            if (information) {
                INFO(("set"));
            }
            if (WriteLink(TheLink, SSRESETHI, sizeof(SSRESETHI), TIMEOUT) != sizeof(SSRESETHI)) {
                AbortExit(PROGRAM_NAME, "Failed to reset Subsystem port");
            }
            for (delay = 0; delay < 1000000; delay++) {
                /* this is a delay loop */
            }
            if (information) {
                INFO((" subsystem"));
            }
            if (WriteLink(TheLink, SSRESETLO, sizeof(SSRESETLO), TIMEOUT) != sizeof(SSRESETLO)) {
                AbortExit(PROGRAM_NAME, "Failed to reset Subsystem port");
            }
            for (delay = 0; delay < 100000; delay++) {
                /* this is a delay loop */
            }
        }

        root = (struct tpstats * ) Malloc(sizeof(struct tpstats));
        p = root;
        parent = NULL;
        p-> tpid = 0;
        p-> procspeed = 0;
        p-> bootlink = 255;
        p-> linkspeed = 0.0f;
        for (i = 0; i < 4; i++) {
            p-> links[i] = NULL;
            p-> linkno[i] = UNKNOWN;
        }
        p-> routelen = 0;
        p-> parent = NULL;
        p-> next = NULL;
        p-> info[0] = '\0';

        if (information) {
            INFO((", CPU type\n"));
        }

        //printf("finding type\n");
        p-> tptype = findtype();
        if (information) {
            writeup(p, 0, information);
            putchar('\n');
        }

        success = TRUE;
        while (success) {
            /* int             c4 = FALSE; */

            if (information) {
                INFO(("# CPU"));
            }
            switch (p-> tptype) {
                /*{{{  case T32:*/
            case T32:
                if (!load(TheLink, p,
                        type32_code.CodeSize,
                        type32_code.Offset,
                        type32_code.WorkSpace,
                        type32_code.VectorSpace,
                        type32_code.BytesPerWord,
                        type32_code.Code)) {
                    if (information) {
                        INFO(("\n"));
                    }
                    writeresults(root, LinkName, c4read, kong);
                    AbortExit(PROGRAM_NAME, "Partial results : Failed to load TYPE32 code on Transputer %d", p-> tpid);
                }
                break;
                /*{{{  case T16:*/
            case T16:
                if (!load(TheLink, p,
                        type16_code.CodeSize,
                        type16_code.Offset,
                        type16_code.WorkSpace,
                        type16_code.VectorSpace,
                        type16_code.BytesPerWord,
                        type16_code.Code)) {
                    if (information) {
                        INFO(("\n"));
                    }
                    writeresults(root, LinkName, c4read, kong);
                    AbortExit(PROGRAM_NAME, "Partial results : Failed to load TYPE16 code on Transputer %d", p-> tpid);
                }
                break;
            }

            if (information) {
                INFO((", link"));
            }

            if (p-> routelen == 0) {
                unsigned char * buffer;
                unsigned int tmp;
                buffer = (unsigned char * ) Malloc(257);
                tmp = WriteLink(TheLink, buffer, 257, TIMEOUT);
                if (tmp != 257) {
                    /* for link speed */
                    if (information) {
                        INFO(("\n"));
                    }
                    writeresults(root, LinkName, c4read, kong);
                    AbortExit(PROGRAM_NAME, "Partial results : Only wrote %d bytes to Transputer %d boot link\n", tmp, p-> tpid);
                }
                free(buffer);
            } else {
                linkspeed(p);
            }

            if (information) {
                INFO((" speed\n"));
            }
            getstats(p);
            if (information) {
                writeup(p, 0, information);
                printf("\n");
            }
            if (information) {
                INFO(("# Loading ispy"));
            }

            if (p-> tptype != T_414) {
                /* T414A */
                int j;
                switch (bpw(p-> tptype)) {
                    /*{{{  case 2:*/
                case 2:
                    if (!load(TheLink, p,
                            check16_code.CodeSize,
                            check16_code.Offset,
                            check16_code.WorkSpace,
                            check16_code.VectorSpace,
                            check16_code.BytesPerWord,
                            check16_code.Code)) {
                        if (information) {
                            INFO(("\n"));
                        }
                        writeresults(root, LinkName, c4read, kong);
                        AbortExit(PROGRAM_NAME, "Partial results : Failed to load CHECK16 code on Transputer %d", p-> tpid);
                    }
                    break;
                    /*{{{  case 4:*/
                case 4:
                    if (!load(TheLink, p,
                            check32_code.CodeSize,
                            check32_code.Offset,
                            check32_code.WorkSpace,
                            check32_code.VectorSpace,
                            check32_code.BytesPerWord,
                            check32_code.Code)) {
                        if (information) {
                            INFO(("\n"));
                        }
                        writeresults(root, LinkName, c4read, kong);
                        AbortExit(PROGRAM_NAME, "Partial results : Failed to load CHECK32 code on Transputer %d", p-> tpid);
                    }
                    break;
                }
                sendid(p);
                if (information) {
                    INFO((", id %d, probe\n", p-> tpid));
                }

                success = TRUE;
                j = 0;
                while ((j < 3) && success) {
                    unsigned int length;
                    unsigned char buffer[8];
                    success = getiserver(TheLink, & length,
                        buffer, (unsigned) sizeof(buffer));
                    if (success) {
                        if (buffer[0] == qHALF) /* this always precedes qC4 */
                            j--;
                        solve(p, length, buffer);
                        if (information) {
                            writeup(p, 0, information);
                            printf("\n");
                        }
                    }
                    j++;
                }
                if (!success) {
                    writeresults(root, LinkName, c4read, kong);
                    AbortExit(PROGRAM_NAME, "Partial results : Failed to probe links on Transputer %d", p-> tpid);
                }
                for (j = 0; j < 4; j++) {
                    switch (p-> linkno[j]) {
                    case C4:
                        {
                            struct tpstats * q;
                            q = create(p, j);
                            /* c4 = TRUE; */
                        }
                        break;
                    }
                }
            }
#if 0 /* cannot test this */
            if (c4) {
                if (information)
                    INFO(("# C004s "));
                stop(p);
                switch (bpw(p-> tptype)) {
                case 2:
                    load(TheLink, p,
                        c00416_code.CodeSize,
                        c00416_code.Offset,
                        c00416_code.WorkSpace,
                        c00416_code.VectorSpace,
                        c00416_code.BytesPerWord,
                        c00416_code.Code);
                    break;
                case 4:
                    load(TheLink, p,
                        c00432_code.CodeSize,
                        c00432_code.Offset,
                        c00432_code.WorkSpace,
                        c00432_code.VectorSpace,
                        c00432_code.BytesPerWord,
                        c00432_code.Code);
                    break;
                }
                sendid(p);
                if (c4reset) {
                    int reset;
                    unsigned char command[2];
                    int i, j, sum = 0;
                    for (i = 0, reset = FALSE;
                        (!reset) && (i < 4); i++) {
                        if (p-> linkno[i] == C4) {
                            sum = checksum(p, i);
                            for (j = 0;
                                (sum != c004s[j]) && (j < c004count); j++);
                            if (j == c004count)
                                reset = TRUE;
                        }
                    }
                    if (reset) {
                        i--; /* undo 'for' increment */
                        if (information)
                            INFO(("# Reset C004 %d\n", p-> links[i] - > tpid));
                        setroute(TheLink, p, 4); /* application */
                        command[1] = (unsigned char) i;
                        command[0] = '\4';
                        sendiserver(TheLink, 2, command); /* reset */
                        c004s[c004count++] = sum;
                        while (root - > next != NULL) {
                            for (p = root;
                                (p-> next != NULL) && (p-> next - > next != NULL); p = p-> next);
                            free(p-> next);
                            p-> next = NULL;
                        }
                        free(root);
                    }
                    success = !reset;
                }
            } else
# endif
            success = TRUE;
            if (success) {
                success = nextcandidate( & p, & parent);
                if (!success) {
                    c4reset = FALSE;
                }
            }
        } /* while */
    } while (c4reset);
}

/*
* Strategy :- For each Transputer on the reference tree, see if we can find
* a match on the real tree.
*
* First:- mark them all as not found, ref->routelen = -1.
*
* For each ref Transputer,
*
* mark all real transputers as not yet scanned, real->extra = NULL. trace a
* path from ref TP to its root - follow same path back from real root to see
* if TPs match.
*
* skip search if we encounter unfound transputers on the way. repeat while
* there is progress
*
*/
int tpcompare(int tp1, int tp2)
{
  int res;
  int ctp1 = class(tp1);
  int ctp2 = class(tp2);
  char nametp1[8];
  char nametp2[8];

  strcpy(nametp1, tptostr(tp1));
  strcpy(nametp2, tptostr(tp2));

  res = (ctp1==ctp2);

  DEBUG(("tp1 = %d, \"%s\", tp2 = %d, \"%s\", ctp1 = %d, ctp2 = %d, res = %d", 
	 tp1, nametp1, tp2, nametp2, ctp1, ctp2, res));

  return(res);
}

void getparams(int argc, char *argv[], 
				    char *LinkName, char *FileName, 
				    int *mapfile,
				    int *subsys, int *c4read, int *c4reset,
				    int *c4set, int *kong, int *do_reset,
				    int *information)
{
    char           *c;
    c = getenv("TRANSPUTER");
    if (c != NULL)
	strcpy(LinkName, c);
    else strcpy(LinkName, DEFAULTLINK);
    while (--argc > 0)
    {
	argv++;
	c = *argv;
#ifdef MSDOS
	if ((*c != '-') && (*c != '/'))
	    Usage();
#else
	if (*c != SWITCHAR)
	    Usage();
#endif
	++c;
	switch (*c)
	{
	    case '?':
	    case 'h':
	    case 'H':
		Usage();
		break;
	    case 'l':
	    case 'L':
		if (argc-- == 1)
		    AbortExit(PROGRAM_NAME, "name or hex address missing after the L option");
		++argv;
		strcpy(LinkName, *argv);
		break;
	    case 'x':
	    case 'X':
		*kong = FALSE;
		break;
	    case 'i':
	    case 'I':
		*information = TRUE;
		break;
	    case 'n':
	    case 'N':
		*do_reset = FALSE;
		break;
	    case 'r':
	    case 'R':
		*subsys = TRUE;
		break;
	    case 'm':
	    case 'M':
		if (argc-- == 1)
		    AbortExit(PROGRAM_NAME, 
		     "file name missing after the M option");
		++argv;
		strcpy(FileName, *argv);
		*mapfile = TRUE;
		break;
	    case 'c':
	    case 'C':
		c++;
		switch (*c)
		{
		    case 'r':
		    case 'R':
			*c4reset = TRUE;
			break;
		    case '4':
			*c4read = 1;
			break;
		    case 'L':
		    case 'l':
			*c4read = 2;
			break;
		    case 'S':
		    case 's':
			*c4set = TRUE;
			break;
		    default:
			Usage();
			break;
		}
		break;          
	    default:
		Usage();
		break;
	}
    }
}
/*}}}  */
/*{{{  sort */
struct tpstats *sort(struct tpstats * root)
{
    struct tpstats **p;
    int             sorted;
    sorted = FALSE;
    while (!sorted)
    {
	sorted = TRUE;
	for (p = (&root); ((*p) != NULL) && ((*p)->next != NULL); p = (&((*p)->next)))
	    if ((*p)->tpid > (*p)->next->tpid)
	    {
		struct tpstats *temp;
		temp = *p;
		*p = temp->next;
		temp->next = temp->next->next;
		(*p)->next = temp;
		sorted = FALSE;
	    }
    }
    return (root);
}
/*}}}  */
/*{{{  follow */
struct tpstats *follow(struct tpstats * p, struct tpstats * q, int fromroot)
{
    DEBUG(("starting follow()"));

    if ((!fromroot) && (p->routelen == (-1)))   /* not located yet */
	return (p);
    if (!fromroot && class(p->tptype) == C4)
	return (NULL);
    if (p->extra != NULL)
	return (NULL);          /* been here already */
    if (p->parent == NULL)
	return (q);
    else
    {
	int             i, lno, unfound = FALSE;
	struct tpstats *r = NULL;
	if (class(p->tptype) == C4)
	{
	    for (lno = 0; (lno < 4) && p->parent->links[lno] != p; lno++);
	    if (lno > 3)
		AbortExit(PROGRAM_NAME, "Internal error in follow");
	    p->extra = p;       /* mark this one as searched */
	    r = follow(p->parent, q, FALSE);
	    p->extra = NULL;    /* done with this tree search */
	    r = r->links[lno];  /* back-tracing */
	} else
	    for (i = 0; (i < 4) && (r == NULL); i++)
		if ((fromroot) || (i == (int) p->bootlink))
		{
		    struct tpstats *temp = p->links[i];
		    lno = p->linkno[i];
		    if (temp != NULL)
		    {
			p->extra = p;   /* mark this one as searched */
			r = follow(temp, q, FALSE);
			p->extra = NULL;        /* done with this tree search */
			if (r == temp)
			{
			    unfound = TRUE;
			    r = NULL;   /* encountered unfound transputer */
			} else
			{
			    if ((r == NULL) ||  /* cant find it */
				(r->links[lno] == NULL) ||      /* bad connection */
				((int) r->linkno[lno] != i) ||  /* ditto */
			     (!tpcompare(r->links[lno]->tptype, p->tptype)))
				r = NULL;       /* keep searching */
			    if (r != NULL)
			    {
				p->bootlink = (char) i;
				r = r->links[lno];
			    }
			}
		    }
		}
	if ((r == NULL) && unfound)
	    return (p);
	else
	    return (r);
    }
}
/*}}}  */
/*}}}  */

int             compare(struct tpstats * root, struct tpstats * reference)
{
    int             progress = TRUE;
    int             same = 0, i, refmax, max = 0;
    struct tpstats *p, *q;

    DEBUG(("starting same()"));

    for (p = reference; p != NULL; p = p->next)
    {
	if (p->tpid > max)
	    max = p->tpid;
	p->routelen = (-1);     /* mark them as not found */
    }
    refmax = max;
    for (p = root; p != NULL; p = p->next)
    {
	p->extra = NULL;        /* not allocated yet */
	for (i = 0; (p->info[i + 1]) && (p->info[i] != ']'); i++);
	strcpy(p->info, &p->info[i + 1]);
    }
    for (p = root; p != NULL; p = p->next)
	p->tpid = ++max;        /* renumber them out of the way */

    /* no match if the root tp is different in root & reference */
    progress = tpcompare(root->tptype, reference->tptype);

    while (progress)
    {
	progress = 0;
	for (p = reference; p != NULL; p = p->next)
	    if (p->routelen)
	    {
		q = follow(p, root, TRUE);
		if ((q != NULL) && (q != p) && (q->extra == NULL))
		{
		    q->tpid = p->tpid;
		    p->routelen = 0;    /* found */
		    q->extra = q;       /* allocated */
		    if (class(p->tptype) == C4)
			memcpy(((struct c4stats *) q)->config,
			       ((struct c4stats *) p)->config, 32);
		    progress++;
		}
	    }
    }
    for (p = reference; p != NULL; p = p->next)
	if (p->routelen == -1)
	{
	    struct tpstats *r;
	    r = (struct tpstats *) Malloc(sizeof(struct tpstats));
	    r->next = root->next;
	    root->next = r;     /* stuff it in somewhere, it'll be sorted */
	    r->tpid = p->tpid;
	    r->routelen = -1;   /* route length from host */
	    r->tptype = p->tptype;      /* Transputer type */
	    r->procspeed = p->procspeed;        /* processor speed */
	    r->route = 0;       /* incremental route here */
	    r->bootlink = p->bootlink;
	    r->linkspeed = 0.0f;/* Mbytes/sec down boot link set to zero to
				 * mark it as not present */
	    for (i = 0; i < 4; i++)
	    {
		r->links[i] = p->links[i];      /* pointers to other
						 * Transputers */
		r->linkno[i] = p->linkno[i];    /* link numbers, or
						 * non-transputer type */
	    }
	    r->parent = NULL;
	    strcpy(r->info, " MISSING ");
	    r->extra = NULL;
	    same = -1;
	}
    for (p = reference; p != NULL; p = p->next)
    {
	q = find(root, p->tpid);
	if ((q != NULL) && (q->routelen != -1) && (class(p->tptype) != C4))
	    /* a match */
	{
	    q->info[0] = ' ';
	    q->info[1] = '\0';
	    for (i = 0; i < 4; i++)
	    {
		if ((p->links[i] != NULL) && (q->links[i] == NULL))
		{
		    sprintf(&(q->info[strlen(q->info)]), "%d-", i);
		    same = -1;
		}
		if ((p->links[i] == NULL) && (q->links[i] == NULL))
		    sprintf(&(q->info[strlen(q->info)]), "  ");
		if ((p->links[i] == NULL) && (q->links[i] != NULL))
		{
		    sprintf(&(q->info[strlen(q->info)]), "%d+", i);
		    if (!same)
			same = 1;
		}
		if ((p->links[i] != NULL) && (q->links[i] != NULL))
		{
		    if ((p->links[i]->tpid != q->links[i]->tpid) ||
			(p->linkno[i] != q->linkno[i]))
		    {
			sprintf(&(q->info[strlen(q->info)]), "%d?", i);
			same = -1;
		    } else
			sprintf(&(q->info[strlen(q->info)]), "  ");
		}
	    }
	}
    }
    max = refmax;
    for (p = root; p != NULL; p = p->next)
	if (p->tpid > refmax)
	{                       /* an extra transputer */
	    p->tpid = ++max;    /* renumber it */
	    strcpy(p->info, " -EXTRA- \0");
	}

    DEBUG(("ending same()"));
	
    return (same);
}
/*}}}  */
/*{{{  setC004 */
void            setC004(struct c4stats * p)
{
    unsigned char   command[100];
    int             i, length;
    setroute(TheLink, p->parent, 4);    /* application */
    command[1] = (unsigned char) p->route;
    command[0] = '\4';
    sendiserver(TheLink, 2, command);   /* reset */
    for (i = 0, length = 0; i < 32; i++)
	if (p->config[i] != EMPTY)
	{
	    command[length++] = '\0';
	    command[length++] = p->config[i];
	    command[length++] = (unsigned char) i;
	}
    if (length > 0)
    {
	tpboot(TheLink, (struct tpstats *) p);  /* raw mode */
	sendiserver(TheLink, length, command);  /* config */
    }
}
/*}}}  */
/*{{{  main */
int main(int argc, char *argv[]) {
    struct tpstats *reference = NULL;
    char            Pipe[128];
    char            Banner[128];
    int             transputers = 0;
    int             same = 0, subsys = FALSE, information = FALSE;
    int             c4reset = FALSE, c4set = FALSE, mapfile = FALSE;
    int do_reset = TRUE;

    FileName[0] = '\0';
    LinkName[0] = '\0';
    getparams(argc, argv, LinkName, FileName, &mapfile,
	      &subsys, &c4read, &c4reset, &c4set,
	      &kong, &do_reset, &information);

	if (kong) {
    	if (!isatty(fileno(stdin))) { 
			if (information) {
				INFO(("# Reading input template\n"));
			}
			transputers = lex(stdin, &reference, Pipe, LinkName, Banner);
		} else {
			kong = FALSE;
			c4set = FALSE;
		}
	} else {
		c4set = FALSE;
    }

    /*  try to open link  */
    TheLink = OpenLink(LinkName);
    
    if (TheLink < 0) {
		switch (TheLink){
			case ER_LINK_BAD:
				AbortExit(PROGRAM_NAME, "cannot access root transputer - Transputer no longer available");
			case ER_LINK_CANT:
				AbortExit(PROGRAM_NAME, "cannot access root transputer - No response from transputer");
			case ER_LINK_SOFT:
				AbortExit(PROGRAM_NAME, "cannot access root transputer - Comms software failure");
			case ER_LINK_NODATA:
				AbortExit(PROGRAM_NAME, "cannot access root transputer - Empty data packet received");
			case ER_LINK_NOSYNC:
				AbortExit(PROGRAM_NAME, "cannot access root transputer - Link syncronisation lost");
			case ER_LINK_BUSY:
				AbortExit(PROGRAM_NAME, "cannot access root transputer - Link Hardware already in use");
			case ER_NO_LINK:
				AbortExit(PROGRAM_NAME, "cannot access root transputer - No link interface hardware");
			case ER_LINK_SYNTAX:
				AbortExit(PROGRAM_NAME, "cannot access root transputer - Bad link specifier");
			default:
				AbortExit(PROGRAM_NAME, "cannot access root transputer (result = %d)", TheLink);
		};
	}

    /*{{{  map network */
    if (information) {
		INFO(("# Test"));
    }
    check(subsys, c4read, c4reset, information, do_reset);
    
    if (kong) {
		/*{{{  try to match network to template */
		if (information) {
			INFO(("Matching network to template\n"));
			/*
			writeresults(root, LinkName, c4read, kong);
			writeresults(reference, LinkName, c4read, kong);
			*/
		}
		same = compare(root, reference);
		if (!transputers) {
			AbortExit(PROGRAM_NAME, "No Transputers found");
		}
		root = sort(root);
	}
    
    if (c4set || c4read) {
		/*{{{  do C004 stuff */
		struct tpstats *p;
		struct c4stats *q;
		if (c4set) {
			if (information) {
				INFO(("Setting C004s "));
			}
			for (q = (struct c4stats *) root; q != NULL; q = (struct c4stats *) q->next) {
				if ((class(q->tptype) == C4) && (q->extra != NULL)) {
					if (information) {
						INFO(("%d ", q->tpid));
					}
					setC004(q);
				}
				if (information) {
					INFO(("\n"));
				}
				while (root->next != NULL) {
					for (p = root; (p->next != NULL) && (p->next->next != NULL); p = p->next) {
						if (class(p->tptype) == C4) {
							q = (struct c4stats *) p->next;
							free(q);
						} else {
							free(p->next);
						}
					}
					p->next = NULL;
				}
				free(root);
			
				if (information) {
					INFO(("Re-testing network after C004s set\n"));
				}

				if (do_reset) {
					check(subsys, c4read, FALSE, information, do_reset);
				} else {
					AbortExit(PROGRAM_NAME, "Unable to reset network due to N option");
				}
			
				if (information) {
					INFO(("Matching network to template\n"));
				}
				same = compare(root, reference);
			
				if (!transputers) {
					AbortExit(PROGRAM_NAME, "No Transputers found");
				}
				root = sort(root);
			}
			if (information) {
				INFO(("Reading C004 settings\n"));
			}
			for (q = (struct c4stats *) root; q != NULL; q = (struct c4stats *) q->next) {
				if (class(q->tptype) == C4) {
					readC004(q);
				}
			}
		}
	}
    
	if (!c4read && c4set) {
		c4read = 1;
	}
    writeresults(root, LinkName, c4read, kong);
    CloseLink(TheLink);
    
    /*{{{  display result info and return() */
    switch (same) {
		case -1:
			if (information) {
				INFO(("Input Template will not load onto network\n"));
			}
			return (-1);
		case 0:
			if (information && kong) {
				INFO(("Network is identical to input template\n"));
			}
			return (0);
		case 1:
			if (information) {
				INFO(("Network is a superset of input template\n"));
			}
			return (0);
	}
    return (0);
}