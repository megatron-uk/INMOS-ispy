/*{{{  BLURB */
/* --------------------------------------------------

 *    mtest.c  --  memory test program
 *
 *    Andy Rabagliati INMOS
 *
 *    Modified by Jeremy Thorp
 *
 * Adapted for Autronica, TRONDHEIM, Norway, under contract to
 * 0yvind Teig <teig@autronica.no> for link on their boards.
 *
 *
 * Copyright andyr@wizzy.com (Andy Rabagliati)         30 August 1994
 *                                                    18 January 1995
 * 
 * This code may be freely used, provided above credits remain intact
 * the CREDITS #define below is kept. Any modifications must be
 * distributed in source.
 * 
 * --------------------------------------------------
 */
/*}}}  */

char *PROGRAM_NAME = "mtest";

#define VERSION_NUMBER  "3.22"
#define VERSION_DATE    "31st Jan 1995"
#define CREDITS         "by Andy Rabagliati <andyr@wizzy.com>"
#define WWW             "<URL http://www.rmii.com/~andyr/>"
#define AUTRONICA       "for AUTRONICA, Trondheim, Norway"


/*{{{  includes */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#ifdef MSDOS
#include "linkio.h"
#endif

/*
   #ifdef HELIOS
   #include <stdlib.h>
   #else
   #include <Malloc.h>
   #endif
 */

#include <stdlib.h>		/* for atoi  */

#include "inmos.h"
#include "mtest32.h"
#include "mtest16.h"
#include "checklib.h"
#include "cklib.h"
/*}}}  */
/*{{{  defines */

#define SEGSIZE 511
#define DOTSIZE 256

#define STDERR  stderr

#define HUNKY_DORY 0
#define BAD_KARMA 1

#define SWITCHAR '-'

/*}}}  */

/*{{{  struct meminfo */
struct meminfo
  {
    unsigned char testno, endmem;
    int memspeed;
    unsigned long error, memsize, watermark;
  };
/*}}}  */
int Kb2 = DOTSIZE * 32, Mb = 0;
LINK TheLink;

/*{{{  Usage */
void Usage (void)
{
	fprintf (stdout, "\nUsage:  ispy | %s [%coption...]  --  version %s of %s.\n\n",PROGRAM_NAME, SWITCHAR, VERSION_NUMBER, VERSION_DATE);
	fprintf (stdout, "Valid options are: \n");
	fprintf (stdout, " %cc      :  Include T2s with C004s on links (default - no)\n", SWITCHAR);
	fprintf (stdout, " %ce <Kb> :  sets ceiling in Kbytes to which memory is tested\n", SWITCHAR);
	fprintf (stdout, " %cl      :  Do not write ..... dots to stderr\n", SWITCHAR);
	fprintf (stdout, " %ct <tp> :  test processor <tp> only\n", SWITCHAR);
	fprintf (stdout, " %ct2     :  test T2s only\n", SWITCHAR);
	fprintf (stdout, " %ct4     :  test T4s and T8s only\n", SWITCHAR);
	fprintf (stdout, " %cq      :  Quick memory sizing option\n", SWITCHAR);
	fprintf (stdout, " %cx      :  Extra information on why memory search stopped\n", SWITCHAR);
	fprintf (stdout, " %c0      :  Do not include root processor in tests\n", SWITCHAR);
	fprintf (stdout, " %ci      :  show progress information\n", SWITCHAR);
	fprintf (stdout, " %ch      :  display this help page\n\n", SWITCHAR);
	fprintf (stdout, "  terminating <;> means memory wraps,\n");
	fprintf (stdout, "  terminating <.> means memory stops. %cX switch will explain further\n", SWITCHAR);
	fprintf (stdout, "  terminating <|> means %cE ceiling reached\n", SWITCHAR);
	fprintf (stdout, "  terminating <?> means no further messages received - a bus error?\n");
	fprintf (stdout, "                  or processor not tested\n\n");
	fprintf (stdout, "                                     %s\n",     AUTRONICA);
	fprintf (stdout, "%s %s\n", CREDITS, WWW);
	exit (0);
}

/*}}}  */
/*{{{  getresults */
struct tpstats *getresults(LINK TheLink, struct tpstats *root, int *delta)
{
	struct tpstats *p;
	struct meminfo *q;
	unsigned char results[16];
	unsigned int i, ok;
	ok = getiserver (TheLink, &i, (unsigned char *) results, sizeof (results));
	if ((i == 16) && ok)
	{
		int id;
		unsigned long error;
		id = results[0] + (results[1] << 8);
		p = find (root, id);
		if (p == NULL)
			AbortExit (PROGRAM_NAME, "Cannot find transputer %d", id);
		q = (struct meminfo *) p->extra;
		q->testno = results[2];
		q->endmem = results[3];
		for (i = 15, error = 0; i >= 12; i--)
			error = (error << 8) + results[i];
		q->error = q->error | error;	/* accumulate errors  */
		switch (q->endmem)
		{
			int speed;
			case 0:		/* still going  */
			case 2:		/* ceiling  */
				for (i = 7, q->memsize = 0; i >= 4; i--)
					q->memsize = (q->memsize << 8) + results[i];
				speed = results[8] + (results[9] << 8);
				speed = (speed * p->procspeed) / 1000;
				switch (bpw (p->tptype))
				{
					case 2:
						speed = speed / 2;
						q->memsize *= 2;
						break;
					case 4:
						q->memsize *= 4;
						break;
				}
				if (q->memspeed)
					*delta = speed - q->memspeed;	/* otherwise leave it alone  */
				q->memspeed = speed;
		}
		return (p);
	}
	else
	{
		fprintf (stderr, "getresults error : received %d bytes\n", i);
		return (NULL);
	}
}

void sendparams(LINK TheLink, struct tpstats *p, int quick, long endaddr)
{
	struct meminfo *q;
	unsigned char params[8];
	int i;
	int r;
	
	printf("sendparams\n");
	q = (struct meminfo *) p->extra;
	params[0] = (unsigned char) (p->tpid & 0xFF);
	params[1] = (unsigned char) (p->tpid >> 8);
	if (quick)
		params[2] = 4;
	else
		params[2] = 0;
	params[3] = 0;		/* pause  */
	switch (bpw (p->tptype))
	{
		case 2:
			if (endaddr > 64)
				endaddr = 64;
			endaddr = ((endaddr * 1024) / 2) - 1;
			break;
		case 4:
			endaddr = ((endaddr * 1024) / 4) - 1;
			break;
	}
	for (i = 4; i < 8; i++)
	{
		params[i] = (unsigned char) (endaddr & 0xFF);
		endaddr = endaddr >> 8;
	}
	setroute (TheLink, p, 4);	/* to application  */
	r = sendiserver(TheLink, 8, params);
	if (!r)
		AbortExit(PROGRAM_NAME, "sending test parameters to processor %d", p->tpid);
	q->watermark = -2048l;
	q->memspeed = 0;		/* initialise  */
	q->memsize = 0l;
	q->endmem = 0;
}

struct tpstats *sortl(struct tpstats *root)	/* by route length  */
{
  struct tpstats **p;
  int sorted;
  sorted = FALSE;
  while (!sorted)
    {
      sorted = TRUE;
      for (p = (&root); ((*p) != NULL) && ((*p)->next != NULL); p = (&((*p)->next)))
	if ((*p)->routelen > (*p)->next->routelen)
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
/*{{{  sortid */
struct tpstats *
sortid (struct tpstats *root)
{
  struct tpstats **p;
  int sorted;
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

void printresults (FILE * stream, struct tpstats *p, int quick, int extra)
{
	static char *tests[] =
	{
		"", "stuck bits test",
		"random byte test", "random word test",
		"run code test",
		"address test"
	};
	
	struct meminfo *q;
	q = (struct meminfo *) p->extra;
	/*{{{  bus time out  */
	if (q && !q->endmem)
	{
		fprintf (stream, "\n%s%luK", p->info, (q->memsize - q->watermark) / 1024);
		if (!quick)
			fprintf (stream, ",%d", q->memspeed);
		fprintf (stream, "?");
	}
	else
		fprintf (stream, "\n%s", p->info);
	
	if (q && (q->endmem == 1) && extra)
	{
		unsigned long mint;
		switch (bpw (p->tptype))
		{
			case 2:
				mint = 0x8000;
				break;
			case 4:
				mint = 0x80000000;
				break;
			default:
				mint = 0;
				break;
		}
		fprintf (stream, "\n# 2K block at %0*lX, bits in error %0*lX, %s",
			bpw (p->tptype) * 2, mint + 2048l + q->memsize,
			bpw (p->tptype) * 2, q->error, tests[q->testno]);
	}
}

void getparams(int argc, char *argv[], int *zero, unsigned long *endaddr,
	   int *c4s, int *logging, int *info,
	   int *soak, int *T2, int *T4,
	   int *only, int *quick, int *extra, char *CheckFile)
{
  /*{{{  command line switches  */
  ++argv;
  while (--argc > 0)
    {
      char *c;
      if ((char) (**argv) != SWITCHAR)
	Usage ();          /* strcpy(CheckFile, *argv); */ 
      else {
         c = *argv;
         ++c;
         switch (*c)
	   {
	   case '?':
	   case 'h':
	   case 'H':
	     Usage ();
	     break;
	   case '0':
	     *zero = FALSE;
	     break;
	   case 'c':
	   case 'C':
	     *c4s = TRUE;
	     break;
	   case 'e':
	   case 'E':
	     if (argc-- == 1)
	       AbortExit (PROGRAM_NAME, " no address after the E option");
	     ++argv;
	     *endaddr = atol (*argv);
	     break;
	   case 'i':
	   case 'I':
	     *info = TRUE;
	     break;
	   case 'l':
	   case 'L':
	     *logging = FALSE;
	     break;
	   case 's':
	   case 'S':
	     *soak = TRUE;
	     break;
	   case 't':
	   case 'T':
	     ++c;
	  switch (*c)
	       {
	       case '2':
	         *T2 = TRUE;
	         break;
	       case '4':
	         *T4 = TRUE;
	         break;
	       default:
	         if (argc-- == 1)
		   AbortExit (PROGRAM_NAME, " no transputer number after the T option");
	         ++argv;
	         *only = atoi (*argv);
	         break;
	       }
	     break;
	   case 'q':
	   case 'Q':
	     *quick = TRUE;
	     break;
	   case 'x':
	   case 'X':
	     *extra = TRUE;
	     break;
	   default:
	     Usage ();
	     break;
	   }
      }
      ++argv;
    }
  /*}}}  */

  if (*only != -1)
    *zero = TRUE;
  if ((*T2 == FALSE) && (*T4 == FALSE))
    *T2 = *T4 = TRUE;
}
/*}}}  */
/*{{{  main */
int 
main (int argc, char *argv[])
{
  /*{{{  DECLs  */
  struct tpstats *root, *p;
  struct meminfo *q;
  char Pipe[128], Banner[128];
  char LinkName[64];
  char CheckFile[64];
  int transputers, i;
  int logging = TRUE, c4s = FALSE, info = FALSE;
  int zero = TRUE, quick = FALSE, extra = FALSE, soak = FALSE;
  int T2 = FALSE, T4 = FALSE;
  unsigned long endaddr = 0x40000;
  int ok;
  int only = -1, iterations = 1;
  /*}}}  */
  CheckFile[0] = '\0';
  getparams (argc, argv, &zero, &endaddr, &c4s, &logging,
	     &info, &soak, &T2, &T4, &only, &quick, &extra, CheckFile);
  if (info)
    printf ("# mtest reading ispy results\n");
  if (CheckFile[0]) {
     FILE *checkin;
     checkin = fopen(CheckFile, "r");
     transputers = lex (checkin, &root, Pipe, LinkName, Banner);
  } else
     transputers = lex (stdin, &root, Pipe, LinkName, Banner);
  root = sortl (root);		/* sort by route length in case of /R option  */
  if (!transputers--)
    AbortExit (PROGRAM_NAME, "No transputers found");
  TheLink = OpenLink (LinkName);
  ok = (TheLink > 0);
  if (!ok)
    AbortExit (PROGRAM_NAME, "Error opening link name \"%s\"", LinkName);
  /*{{{  load 'em up */
  if (info)
    printf ("# load");
  for (p = root; (p != NULL) && ok; p = p->next)
    if (zero || (!zero && (p->parent != NULL)))
      {
	if ((only == -1) || (only == p->tpid))	/* only load ours */
	  switch (bpw (p->tptype))
	    {
	      /*{{{  16 bitters */
	    case 2:
	      if (T2)
		{
		  if (info)
		    printf (" %d", p->tpid);
		  stop (p);
		  ok = load (TheLink, p,
			     mtest16_code.CodeSize,
			     mtest16_code.Offset,
			     mtest16_code.WorkSpace,
			     mtest16_code.VectorSpace,
			     mtest16_code.BytesPerWord,
			     mtest16_code.Code);
		}
	      break;
	      /*}}}  */
	      /*{{{  32 bitters */
	    case 4:
	      if (T4)
		{
		  if (info)
		    printf (" %d", p->tpid);
		  stop (p);
		  ok = load (TheLink, p,
			     mtest32_code.CodeSize,
			     mtest32_code.Offset,
			     mtest32_code.WorkSpace,
			     mtest32_code.VectorSpace,
			     mtest32_code.BytesPerWord,
			     mtest32_code.Code);
		}
	      /*}}}  */
	    }
	/*{{{  init  */
	if (!ok)
	  printf ("\n%s - Error loading processor %d\n", PROGRAM_NAME, p->tpid);
	p->extra = Malloc (sizeof (struct meminfo));
	q = (struct meminfo *) p->extra;
	i = strlen (p->info);
	p->info[i++] = ' ';
	p->info[i] = '\0';
	q->endmem = 3;
	q->error = 0UL;
	q->memsize = 0UL;
        q->watermark = 0UL;
	/*}}}  */
      }
    else
      {
	p->extra = Malloc (sizeof (struct meminfo));
	q = (struct meminfo *) p->extra;
	q->endmem = 3;
	q->error = 0UL;
	q->memsize = 0UL;
        q->watermark = 0UL;
      }
  /*}}}  */
  if (info)
    printf ("\n# start");
  while (ok && iterations--)	/* there are still iterations to go  */
    {
      int count = 0;

      if (only == -1)
	/*{{{  start them all */
	for (p = root; p != NULL; p = p->next)
	  {
	    if (zero || (!zero && (p->parent != NULL)))
	      {
		/*{{{  start T4s ? */
		if (T4 && bpw (p->tptype) == 4)
		  {
		    count++;
		    ((struct meminfo *) p->extra)->endmem = 2;
		  }
		/*}}}  */
		/*{{{  start T2s ? */
		if (T2 && bpw (p->tptype) == 2)
		  {
		    if (c4s)
		      {
			count++;
			((struct meminfo *) p->extra)->endmem = 2;
		      }
		    else
		      {
			for (i = 0; (class (p->linkno[i]) != C4) && (i < 4); i++);
			if (i == 4)
			  {
			    count++;
			    ((struct meminfo *) p->extra)->endmem = 2;
			  }
		      }
		  }
		/*}}}  */
	      }
	    if (((struct meminfo *) p->extra)->endmem != 3)
	      {
		if (info)
		  printf (" %d", p->tpid);
		sendparams (TheLink, p, quick, endaddr);
	      }
	  }
      /*}}}  */
      else
	{
	  /*{{{  only one to be tested */
	  for (p = root; (p != NULL) && (p->tpid != only); p = p->next);
	  if (p != NULL)
	    {
	      count++;
	      ((struct meminfo *) p->extra)->endmem = 2;
	      if (info)
		printf (" %d", p->tpid);
	      sendparams (TheLink, p, quick, endaddr);
	    }
	  /*}}}  */
	}

      for (; ok && (count || soak); count--)
	{
	  int endmemory = FALSE;
	  while (ok && !endmemory)	/* we havent received endmem  */
	    {
	      int delta = 0;
	      /*{{{  logging */
	      if (!(Kb2++ % (DOTSIZE / 2)))
		{

#ifdef MSC
		  kbhit ();
#endif

		  if (logging)
		    {
		      if (Kb2 == ((DOTSIZE * 32) + 1))
			{
			  for (p = root; p != NULL; p = p->next)
			    {
			      q = (struct meminfo *) p->extra;
			      if (q->error)
				printresults (stderr, p, quick, extra);
			    }
			  fprintf (stderr, "\n<%5dMb> ", Mb);
			  Mb += DOTSIZE / 16;
			  Kb2 = 1;
			}
		      putc ('.', stderr);
		      fflush (stderr);
		    }
		}
	      /*}}}  */
	      /*{{{  get results */
	      p = getresults (TheLink, root, &delta);
	      ok = (p != NULL);
	      if (ok)
		{
		  char terminator = '$';
		  q = (struct meminfo *) p->extra;
		  switch (q->endmem)
		    {
		    case 0:	/* still ok  */
		    case 2:	/* ceiling  */
		      if (!quick && delta)
			{
			  sprintf (&p->info[strlen (p->info)], "%luK,%d ",
			       ((q->memsize - 2048l) - q->watermark) / 1024,
				   q->memspeed - delta);
			  q->watermark = q->memsize - 2048l;
			}
		    }
		  if (q->endmem)
		    {
		      endmemory = TRUE;
		      switch (q->endmem)
			{
			case 2:	/* ceiling  */
			  terminator = '|';
			  break;
			case 1:	/* error  */
			  terminator = '.';
			  break;
			case 255:	/* wrap  */
			  terminator = ';';
			  break;
			}
		      sprintf (&p->info[strlen (p->info)], "%luK",
			       (q->memsize - q->watermark) / 1024);
		      if (!quick)
			sprintf (&p->info[strlen (p->info)], ",%d%c",
				 q->memspeed, terminator);
		      else
			sprintf (&p->info[strlen (p->info)], "%c",
				 terminator);
		    }
		}
	      else
		fprintf (stderr, "Failed to get results\n");
	      /*}}}  */
	    }			/* endmemory  */
	  /*{{{  send params if soak  */
	  if (soak)
	    sendparams (TheLink, p, quick, endaddr);
	  /*}}}  */
	}			/* count */
    }
  /*{{{  print results  */
  root = sortid (root);
  if (logging)
    putc ('\n', stderr);
  if (info)
    putc ('\n', stdout);
  printf ("%s | %s %s\n%s RAM", Pipe, PROGRAM_NAME, VERSION_NUMBER,
	  Banner);
  if (!quick)
    printf (",cycle");
  for (p = root; p != NULL; p = p->next)
    printresults (stdout, p, quick, extra);
  /*}}}  */
  CloseLink (TheLink);
  printf ("\n");
  return (0);
}
/*}}}  */
