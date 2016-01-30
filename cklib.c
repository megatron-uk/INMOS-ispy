
/*
 * --------------------------------------------------
 *
 * cklib.c -- check library routines
 *
 * Copyright andyr@wizzy.com (Andy Rabagliati)	 30 August 1994
 *
 * This code may be freely used, provided credits remain intact
 *
 * --------------------------------------------------
 */

#define CocoPops	FALSE
#define VerboseSwitch	FALSE

#define DEBUG(x) { if (CocoPops) { fputs("(", stderr); fprintf x; fputs(")", stderr); fputc((VerboseSwitch ? '\n' : ' '), stderr); fflush(stderr); } }
#define INFO(x) { if (VerboseSwitch) printf x ; fflush(stderr); }


/*{{{  include & statics */
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#ifndef atoi
#include <stdlib.h>
#endif
#include <malloc.h>

#include "linkio.h"
#include "inmos.h"
#include "checklib.h"
#include "cklib.h"

#define TIMEOUT 40
#define SEGSIZE 511
#define MAXINT	((unsigned)-1)
#define EMPTY ' '

#define SWITCHAR '-'

static char    *TPIDS[] = {"???", "?1/2?", "DISK", "HOST", "T???",
	 "?T16", "?T32", "T16", "T32", "C004", "M212", "T2", "T414", "T800",
			   "T425", "T805", "T801", "", "T225", "T400"};
static char	LETTER[32] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
/*}}}  */
/*{{{  externs */
extern char *PROGRAM_NAME;
/*}}}  */
/*{{{  int bpw(int tptype) */
int bpw(int tptype)
{
    switch (class(tptype)) {
	case T16:
	case M212:
	case T_212:
	case T_225:
		return (2);
	case T32:
	case T_414: /* same as	case T414A: */
	case T414B:
	case T_800:
	case T800C:
	case T800D:
	case T_425:
	case T_805:
	case T_801:
	case T_400:
		return (4);
	default:
		return (0);
    }
}
/*}}}  */
/*{{{  void	       AbortExit(char *String,...) */
void		AbortExit(char *String,...)
{
    va_list	    ArgPtr;
    char	   *p;
    int		    AnInt;
    char	   *AString;
    char	   *ControlString;

    fprintf(stderr, "\nError - %s - ", String);
    va_start(ArgPtr, String);

    ControlString = va_arg(ArgPtr, char *);
    for (p = ControlString; *p; p++)
	if (*p != '%')
	{
	    putc(*p, stderr);
	    continue;
	} else
	{
	    switch (*++p)
	    {
		case 'd':
		    AnInt = va_arg(ArgPtr, int);
		    fprintf(stderr, "%d", AnInt);
		    break;
		case 's':
		    AString = va_arg(ArgPtr, char *);
		    fprintf(stderr, "%s", AString);
		    break;
		default:
		    putc(*p, stderr);
		    break;
	    }
	}

    va_end(ArgPtr);

    fprintf(stderr, ".\n\007");

    exit(1);
}
/*}}}  */
/*{{{  void *Malloc(int size) */
void *Malloc(int size)
{
  void *ptr;

  ptr = (void *) calloc(1, size);

  if (ptr == NULL)
    AbortExit(PROGRAM_NAME, "Ran out of memory");

  return(ptr);
}
/*}}}  */

/*
 * --------------------------------------------------
 *
 * routines to lex check output
 *
 * --------------------------------------------------
 */

/*{{{  void	       tidy(struct tpstats * root) */
void		tidy(struct tpstats * root)
{
    struct tpstats *p, *next;
    int		    i, j, progress = TRUE, tidied = FALSE;
    while (!tidied)
    {
	while (progress && (!tidied))
	{
	    tidied = TRUE;
	    progress = FALSE;
	    for (p = root; p != NULL; p = p->next)
		if (p->routelen == (-1))
		{		/* not tidied yet */
		    if (class(p->tptype) != C4)
		    {
			for (i = 0; i < 4; i++)
			    if ((p->linkno[i] >= 0) && (p->linkno[i] <= 3))
			    {	/* valid link no */
				for (next = root; (next != NULL) &&
				     (next->tpid != p->tpno[i]);
				     next = next->next);
				if (next != NULL)
				    p->links[i] = next;
				else
				    AbortExit("lex", "unable to find Transputer %d", p->tpno[i]);
			    } else
			    if ((class(p->linkno[i]) == C4) && (p->links[i] != NULL))
			    {
				next = find(root, p->tpno[i]);
				if (next == NULL)
				    AbortExit("lex", "unable to find C004 %d", p->tpno[i]);
				p->links[i] = next;
				next->parent = p;
				next->route = i;
			    } else
				p->links[i] = NULL;
			p->parent = p->links[p->bootlink];
			if (p->parent == NULL)
			{
			    if ((class(p->tptype) != C4) && (p->linkno[p->bootlink] == HOST_TAG))
				p->routelen = 0;
			    else
				tidied = FALSE;
			} else
			{
			    if (p->parent->routelen >= 0)
			    {
				/* parent tidied already */
				p->routelen = p->parent->routelen + 1;
				if (class(p->tptype) != C4)
				{
				    for (j = 0; (j < 4) &&
				    ((p->parent->linkno[j] != p->bootlink) ||
				     (p->parent->links[j] != p));
					 j++);
				    if (j < 4)
				    {
					p->route = j;
					progress = TRUE;
				    } else
					tidied = FALSE;
				}
			    } else
				tidied = FALSE;
			}
		    } else
		    if (p->parent == NULL)
			tidied = FALSE;
		    else
			p->routelen = p->parent->routelen + 1;
		}
	}
	if (!tidied)
	{
	    progress = TRUE;
	    for (p = root; progress && p != NULL; p = p->next)
	    {
		progress = FALSE;
		if ((class(p->tptype) == C4) && p->parent == NULL)
		    AbortExit("lex", "cannot find parent of C004 %d", p->tpid);
		else
		if (p->routelen == (-1))
		{
		    /* not tidied yet */
		    for (j = 0; j < 4; j++)
			if ((p->links[j] != NULL) &&
			    (p->links[j]->routelen >= 0))
			{
			    fprintf(stderr, "lex - Warning - Processor %d boot link changed from %d to %d\n",
				    p->tpid, p->bootlink, j);
			    p->bootlink = j;
			    p->route = p->linkno[j];
			    p->parent = p->links[p->bootlink];
			    progress = TRUE;
			}
		}
	    }
	}
	if (!tidied && !progress)
	{
	    for (p = root; p->routelen != (-1); p = p->next);
	    AbortExit("lex", "Unable to find path to part %d", p->tpid);
	}
    }
}
/*}}}  */
/*{{{  int	       readtp(char *tputer) */
int		readtp(char *tputer)
{
    int		    i, type = 0;
    char	    revision;
    tputer[5] = '\0';
    for (i = 4; tputer[i] == ' '; i--)
	tputer[i] = '\0';
    for (i = 0; (i < NUMTPIDS + TPOFFSET) && (strcmp(TPIDS[i], tputer)); i++);
    if (i < NUMTPIDS + TPOFFSET)/* found it there */
	type = (i - TPOFFSET) * 10;
    else
    {
	revision = toupper(tputer[4]);
	tputer[4] = (char) 0;
	for (i = 0;
	     (i < NUMTPIDS + TPOFFSET) && (strcmp(TPIDS[i], tputer)); i++);
	if (i < NUMTPIDS + TPOFFSET)
	{			/* found it there */
	    type = (i - TPOFFSET) * 10;
	    if ((revision >= 'A') && (revision <= 'J'))
		type += revision - 'A';
	}
    }
    return (type);
}
/*}}}  */
/*{{{  int	       lex(....) */
int		lex(FILE * stream, struct tpstats ** root,
				    char *Pipe, char *LinkName, char *Banner)
{
    struct tpstats *curr, *next;
    char	    line[LINESIZE];
    int		    i,j;
    curr = *root = NULL;
    *LinkName = '\0';
    while (fgets(line, LINESIZE, stream) != NULL) {
	for (i = 0; (line[i]) && (line[i] != '\n'); i++);
	line[i] = '\0';
	if (line[0] == '#')
	    printf("#%s\n", line);
	else if (!strncmp(line, "Using ", 6)) {
	    strcpy(Pipe, line);
	    for (i=6; line[i] == ' '; i++);
	    for (j=0; line[i] != ' '; i++, j++)
		LinkName[j] = line[i];
	    LinkName[j] = '\0';
	    fgets(Banner, LINESIZE, stream); /* next line, regardless */
	    for (i = 0; (Banner[i]) && (Banner[i] != '\n'); i++);
	    Banner[i] = '\0';
	    {
		char *c;
		c = strstr(Banner, " by Andy");
		c && (*c = '\0');
	    }

	} else {
	    int		    procspeed, bootlink, tpid;
	    char	    bootspeed[6];
	    char	    tputer[6];
	    char	    linkinfo[4][10];
	    char	    delimiter, c;
	    i = sscanf(line, "%d %5c-%2d %s %d [ %s %s %s %s ]",
		       &tpid, tputer, &procspeed,
		       bootspeed, &bootlink,
		linkinfo[0], linkinfo[1], linkinfo[2], linkinfo[3]);
	    if (i == 9)
	    {
		next = (struct tpstats *) Malloc(sizeof(struct tpstats));
		if (curr != NULL)
		    curr->next = next;
		else
		    *root = next;
		curr = next;
		strcpy(curr->info, line);
		curr->next = NULL;
		curr->extra = NULL;
		curr->parent = NULL;
		curr->routelen = -1;
		curr->tpid = tpid;
		curr->procspeed = procspeed;
		curr->bootlink = bootlink;
		if (sscanf(bootspeed, "%fM", &curr->linkspeed))
			curr->linkspeed = curr->linkspeed * 1.0E6;
		else if (sscanf(bootspeed, "%fk", &curr->linkspeed))
			curr->linkspeed = curr->linkspeed * 1.0E3;
		else curr->linkspeed = 0.0;
		tputer[5] = '\0';
		curr->tptype = readtp(tputer);
		for (i = 0; i < 4; i++)
		{
		    if ((linkinfo[i][0] == '.') || (linkinfo[i][0] == '?'))
			curr->linkno[i] = TXXX;
		    else
		    {
			int		tpno;
			if (sscanf(linkinfo[i], "%d:%c", &tpno, &c) == 2)
			{
			    curr->tpno[i] = tpno;
			    if (c == 'C')
				curr->linkno[i] = C4;
			    else
				curr->linkno[i] = c - '0';
			    curr->links[i] = curr;	/* for benefit of C004 */
			} else
			{
			    curr->linkno[i] = readtp(linkinfo[i]);
			    curr->links[i] = NULL;	/* for benefit of C004 */
			}
		    }
		}
	    } else if (i > 0)
	    {
		struct c4stats *q;
		int		from = 0, to;
		char		bracket, *c;
		i = sscanf(line, "%d %s", &tpid, tputer);
		if ((i == 2) && (tputer[0] == 'C'))
		{
		    q = (struct c4stats *) Malloc(sizeof(struct c4stats));
		    if (curr != NULL)
			curr->next = (struct tpstats *) q;
		    else
			*root = (struct tpstats *) q;
		    curr = curr->next;
		    strcpy(q->info, line);
		    q->next = NULL;
		    q->extra = NULL;
		    q->parent = NULL;
		    q->routelen = -1;
		    q->tpid = tpid;
		    q->tptype = readtp(tputer);
		    if (class(q->tptype) != C4)
			AbortExit("lex", "%s ???", line);
		    strcpy(q->config, "				       ");
		    for (i = 0; (line[i] != '[') && (line[i] != '(') && line[i]; i++);
		    bracket = line[i++];
		    switch (bracket)
		    {
			case '[':
			    for (to = 0; (line[i] != 0) && (line[i] != ']'); i++)
				switch (line[i])
				{
				    case '-':
					to++;
				    case ' ':
					break;
				    default:
					for (from = 0; LETTER[from] != line[i]; from++);
					q->config[to] = (char) from;
					to++;
					break;
				}
			    break;
			case '(':
			    c = &line[i];
			    while (*c!=')' && (*c))
			    {
				while (*c && ((*c<'0') || (*c>'9')))
				    c++;
				if (*c)
				    from = atoi(c);
				while (*c && (*c!='-') && (*c!='>'))
				    c++;
				delimiter = *c;
				while (*c && ((*c<'0') || (*c>'9')))
				    c++;
				if (*c)
				    to = atoi(c);
				while (*c && (*c>='0') && (*c<='9'))
				    c++;
				if (!*c)
				    continue;
				while (*c && ((*c<'0') || (*c>'9')))
				    c++;
				switch (delimiter)
				{
				    case '-':
					if (q->config[from] == EMPTY)
					    q->config[from] = (char) to;
					else
					    AbortExit("lex", "C004 %d: link %d connected to %d and %d",
						      tpid, from, to, q->config[from]);
					if ((q->config[to] == EMPTY) || (to == from))
					    q->config[to] = (char) from;
					else
					    AbortExit("lex", "C004 %d: link %d connected to %d and %d",
						      tpid, to, from, q->config[to]);
					break;
				    case '>':
					if (q->config[from] == EMPTY)
					    q->config[from] = (char) to;
					else
					    AbortExit("lex", "C004 %d: link %d connected twice",
						      tpid, from);
					break;
				}
			    }
		    }
		}
	    } else
		fprintf(stderr, "%s ???\n", line);
	}
    }
    if (*root != NULL)	 {
	tidy(*root);
	for (curr = (*root), i = 0; curr != NULL; curr = curr->next)
	    i++;
	return (i);
    } else
	return (0);
}
/*}}}  */
/*{{{  struct tpstats *find(struct tpstats * root, int id) */
struct tpstats *find(struct tpstats * root, int id)
{
    struct tpstats *p;
    for (p = root; (p != NULL) && (p->tpid != id); p = p->next);
    return (p);
}
/*}}}  */
/*{{{  char	      *tptostr(int tp) */
char	       *tptostr(int tp)
{
    static char	    buffer[8];
    if ((tp / 10) < NUMTPIDS)
	strcpy(buffer, (TPIDS[(class(tp) / 10) + TPOFFSET]));
    else
	sprintf(buffer, "T#%-2d", tp / 10);
    if (tp >= (-50))
    {
	buffer[4] = (char) 'a' + (tp - class(tp));
	buffer[5] = (char) 0;
    }
    return (buffer);
}
/*}}}  */
/*{{{  int	       writeup(struct tpstats * p, int c4read) */
int		writeup(struct tpstats * p, int c4read, int comment)
{
    int		    i;
    int		    errfound = FALSE;

    if (p != NULL)
	if (class(p->tptype) != C4)
	{
	    if (comment)
		printf("#%3d %-5s-%2d ", p->tpid, tptostr(p->tptype), p->procspeed);
	    else
		printf("%4d %-5s-%2d ",	 p->tpid, tptostr(p->tptype), p->procspeed);
	    if (p->linkspeed > 1.0E6)
		printf("%3.1fM", p->linkspeed/1.0E6);
	    else
		printf("%3.0fk", p->linkspeed/1.0E3);
	    if (p->bootlink < 4)
		printf("%2d [", p->bootlink);
	    else
		printf(" - [");
	    for (i = 0; i < 4; i++)
	    {
		if (p->links[i] == NULL)
		    switch (class(p->linkno[i]))
		    {
			case UNKNOWN:
			case HALF:
			case DISK:
			case HOST_TAG:
			case BAD16:
			case BAD32:
			case T16:
			case T32:
			case T414A:
			    printf("%7s", tptostr(p->linkno[i]));
			    break;
			case C4:
			    printf("   C004");
			    break;
			default:
			    if (p->links[i] == NULL)
				printf("    ...");
			    break;
		    }
		else
		    switch (p->linkno[i])
		    {
			case 0:
			case 1:
			case 2:
			case 3:
			    printf(" %4d:%1d", p->links[i]->tpid, p->linkno[i]);
			    break;
			case C4:
			case C4 + 1:
			    if (c4read)
				printf(" %4d:C", p->links[i]->tpid);
			    else
				printf("   C004");
			    break;
			default:
			    printf(" %4d:?", p->links[i]->tpid);
			    break;
		    }
	    }
	    printf(" ]%s", p->info);
	    for (i = 0; i < 4; i++)
		if (p->links[i] == NULL)
		    switch (p->linkno[i])
		    {
			case BAD16:
			case BAD32:
			    break;
			case T16:
			case T32:
			    errfound = TRUE;
			    break;
			case T414A:
			    break;
		    }
	}
    return (errfound);
}
/*}}}  */
/*{{{  void	       writeC4(struct c4stats * q) */
void		writeC4(struct c4stats * q)
{
    char	    inputs[32];
    int		    i, error = FALSE;
    for (i = 0; i < 32; i++)
	inputs[i] = EMPTY;
    for (i = 0; i < 32; i++)
	if (q->config[i] != EMPTY)
	{
	    int		    j;
	    j = q->config[i];
	    if (inputs[j] != EMPTY)
		error = TRUE;
	}
    printf("%4d %-5s   [", q->tpid, tptostr(q->tptype));
    for (i = 0; i < 32; i++)
    {
	if (!(i % 8))
	    putchar(' ');
	if (q->config[i] == EMPTY)
	    putchar('-');
	else
	    putchar(q->config[i]);
    }

    if (error)
	printf(" ]?");
    else
	printf(" ]");
}
/*}}}  */
/*{{{  void	       writeCl(struct c4stats * q) */
void		writeCl(struct c4stats * q)
{
    char	    inputs[32];
    int		    i, error = FALSE;
    for (i = 0; i < 32; i++)
	inputs[i] = EMPTY;
    for (i = 0; i < 32; i++)
	if (q->config[i] != EMPTY)
	{
	    int		    j;
	    j = q->config[i];
	    if (inputs[j] != EMPTY)
		error = TRUE;
	}
    printf("%4d %-5s (", q->tpid, tptostr(q->tptype));
    for (i = 0; i < 32; i++)
    {
	int		j;
	if (q->config[i] != EMPTY)
	{
	    j = q->config[i];
	    if (q->config[j] == (char) i)
	    {
		if (j >= i)
		    printf("%d-%d ", i, j);
	    } else
		printf("%d>%d ", i, j);
	}
    }

    if (error)
	printf(")?");
    else
	printf(")");
}
/*}}}  */

/*
 * --------------------------------------------------
 *
 * fns to talk to check network
 *
 * --------------------------------------------------
 */

int readbytes(LINK TheLink, unsigned char *string, unsigned int maxlength) {
	unsigned int bytes_to_go = maxlength;
	unsigned char *read_point = string;
	int bytesread = 0;
	int count = 10;

	while ((bytes_to_go > 0) && (count > 0)) {
		bytesread = ReadLink(TheLink, read_point, bytes_to_go, TIMEOUT);
		DEBUG((stderr, "readbytes(bytesread = %d)", bytesread));
		bytes_to_go -= bytesread;
		read_point += bytesread;
		count--;
	}

	if (bytesread < 0) {
		fprintf(stderr, "Read from link failed - result = %i\n", bytesread);
		bytesread = 0;
	} else {
		if (count == 0) {
			fprintf(stderr, "Read from link failed (Timeout)\n");
		} else {
			bytesread = maxlength - bytes_to_go;
		}
	}

	DEBUG((stderr, "readbytes(bytesread = %d)", bytesread));
	if (CocoPops) {
		int i;
		fputs("(", stderr);
		for (i = 0; i < bytesread; i++) {
			fprintf (stderr, "%0X ", (int) *(string + i) );
		}
		fputs(")", stderr);
		fputc('\n', stderr);
		fflush(stderr);
	}

	return (bytesread);
}

int getiserver(LINK TheLink, unsigned int *length, unsigned char *string, unsigned int maxlength){
	if (readbytes(TheLink, string, 2) == 2){
		*length = (string[1] << 8) + string[0];
		if ((*length <= maxlength) && (length) && (readbytes(TheLink, string, *length) == *length)) {
			return (TRUE);
		} else {
			if (length) {
				return (FALSE);
			} else {
				/* length of zero */
				return (TRUE);
			}
		}
	} else {
		return (FALSE);
	}
}

int sendiserver(LINK TheLink, unsigned int length, unsigned char *buffer) {
	unsigned char   int16[2];
	
	int16[0] = (unsigned char) (length & 0xFF);
	int16[1] = (unsigned char) (length >> 8);
	
	if ((WriteLink(TheLink, int16, 2, TIMEOUT) == 2) && length) {
		unsigned int start=0, written=0; /* , writing=64;  */
		do {
			/* if ((length-written) < writing)
			writing = length-written; */
			start += written;
			written = WriteLink(TheLink, &buffer[start], (length-written), TIMEOUT);
		} while (written && ((start+written) < length));
		
		if ((start+written) == length) {
			return (TRUE);
		} else {
			return (FALSE);
		}
	} else {
		return (FALSE);
	}
}

void setroute(LINK TheLink, struct tpstats * p, int lastlink) {
	unsigned char  *route;
	struct tpstats *q;
	unsigned int    i;
	static unsigned char ROUTE[] = {0xFF, 0xFF, TAG_SETPATH};
	if (p != NULL) {
		route = (unsigned char *) Malloc((p->routelen) + 1);
		
		if (route == NULL) {
			AbortExit("loader", "out of memory in setroute, route length %d", (p->routelen) + 1);
		}
		route[p->routelen] = (unsigned char) lastlink;
		
		for (i = p->routelen, q = p; (i > 0) && (q != NULL); i--, q = q->parent) {
			route[i - 1] = (unsigned char) q->route;
		}
		
		if ((i != 0) || (q->parent != NULL)) {
			AbortExit("loader", "Error finding route thread, processor %d", p->tpid);
		}
		if ((WriteLink(TheLink, ROUTE, sizeof(ROUTE), TIMEOUT) == sizeof(ROUTE)) && sendiserver(TheLink, (p->routelen) + 1, route)) {
			/* noop */
		} else {
			if (lastlink == (-1)) {
				AbortExit("loader", "Timed out stopping processor %d", p->tpid);
			}  else {
				if (lastlink < 4) {
					AbortExit("loader", "Timed out setting route to processor %d",  p->links[lastlink]->tpid);
				}
				//else {
				//	if (lastlink == 4) {
				//		AbortExit("loader", "Timed out talking to application on processor %d", p->tpid);
				//	}
				//}
			}
		
			free(route);
		}
	}
}

void tpboot(LINK TheLink, struct tpstats * p) {
    static unsigned char TPBOOT[] = {0xFF, 0xFF, TAG_BOOT};
    if (p->parent != NULL) {
    	setroute(TheLink, p->parent, p->route);
    	if (WriteLink(TheLink, TPBOOT, 3, TIMEOUT) == 3) {
    		/* noop */
    	} else {
	    	AbortExit("loader", "Timed out preparing to boot processor %d", p->tpid);
	    }
    }
}

int load(LINK TheLink, struct tpstats * p,
				     long CodeSize,
				     long Offset,
				     long WorkSpace,
				     long VectorSpace,
				     int BytesPerWord,
				     unsigned char *Code)
{
	#include "boot.h"
	long 		count, i, length;
	int 		flag, j, k;
	unsigned char 	params[16];
	
	printf("load - tpboot\n");
	tpboot(TheLink, p);
	length = CodeSize;
	if (p->parent != NULL) {
		printf("load - sendiserver 1\n");
		flag = sendiserver(TheLink, sizeof(boot), boot);
	} else {
		printf("load - writelink 1\n");
		flag = WriteLink(TheLink, boot, sizeof(boot), TIMEOUT) == sizeof(boot);
	}
	if (flag) {
		j = 0;
		for (k = 0; k < BytesPerWord; k++) {
			params[j++] = (unsigned char) (WorkSpace & 0xFF);
			WorkSpace = WorkSpace >> 8;
		}
		
		for (k = 0; k < BytesPerWord; k++) {
			params[j++] = (unsigned char) (VectorSpace & 0xFF);
			VectorSpace = VectorSpace >> 8;
		}
	
		for (k = 0; k < BytesPerWord; k++) {
			params[j++] = (unsigned char) (CodeSize & 0xFF);
			CodeSize = CodeSize >> 8;
		}
	
		for (k = 0; k < BytesPerWord; k++) {
			params[j++] = (unsigned char) (Offset & 0xFF);
			Offset = Offset >> 8;
		}
	
		if (p->parent != NULL) {
			printf("load - sendiserver 2\n");
			flag = sendiserver(TheLink, 4 * BytesPerWord, params);
		} else {
			printf("load - writelink 2\n");
			flag = (WriteLink(TheLink, params, 4 * BytesPerWord, TIMEOUT) == (4 * BytesPerWord));
		}
		
		for (i = 0; (i < length) && flag; i += count)	{
			count = length - i;
			if (count > SEGSIZE) {
				count = SEGSIZE;
			}
			if (p->parent != NULL) {
				printf("load - sendiserver 3\n");
				flag = sendiserver(TheLink, (int) count, &Code[i]);
			} else {
				printf("load - writelink 3\n");
				flag = WriteLink(TheLink, &Code[i], (int) count, TIMEOUT) == (int) count;
			}
		}
	}
	printf("load - finished [%d]\n", flag);
	return (flag);

}
/*}}}  */
