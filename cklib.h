/*
 * --------------------------------------------------
 *
 * lex.h  --  interface to lex.c
 *
 * andyr@wizzy.com (Andy Rabagliati)   30 August 1994
 *
 * This code may be freely used, provided credits remain intact
 *
 * --------------------------------------------------
 */

#define LINESIZE	200
#define NUMTPIDS	6
#define TPOFFSET	14
#define class(tptype)	((((tptype+(TPOFFSET*10))/10)-TPOFFSET)*10)

struct tpstats
{
    struct tpstats 	*parent,	/* where I was born */
		   			*next;	/* sequential list, handy to have */
    void	  		*extra;	/* used by other programs to extend struct */
    char	    	info[LINESIZE];	/* original line */
    int		    	tpid,	/* number assigned by CHECKOUT */
		    		tptype,	/* Transputer type */
		    		route,	/* incremental route here */
		    		routelen,	/* route length from host */
		    		bootlink,	/* link CHECKOUT is using for lifeline */
		    		procspeed;	/* processor speed */
    float	    	linkspeed;	/* Mbytes/sec down boot link */
    struct tpstats *links[4];	/* pointers to other Transputers */
    int		    	tpno[4];	/* temporary - other transputer number */
    int		    	linkno[4];	/* link numbers, or non-transputer type */
};

struct c4stats
{
    struct tpstats  *parent,	/* where I was born */
		   			*next;	/* sequential list, handy to have */
    void	   		*extra;	/* used by other programs to extend struct */
    char	    	info[LINESIZE];	/* original line */
    int		    	tpid,	/* number assigned by CHECKOUT */
		    		tptype,	/* Transputer type */
		    		route,	/* incremental route here */
		    		routelen;	/* route length from host */
    char	    	config[32]; /* state */
};

int		bpw(int tptype);
void		AbortExit(char *String,...);
void	       *Malloc(int size);

int		lex(FILE * stream, struct tpstats ** root,
				  char *Pipe, char *LinkName, char *Banner);
struct tpstats *find(struct tpstats * root, int id);
int		readtp(char *tputer);
EXTERN char    *tptostr(int tp);
int		writeup(struct tpstats * p, int c4read, int comment);
						/* writes one line */
void		writeC4(struct c4stats * q);	/* C004 shorthand */
void		writeCl(struct c4stats * q);	/* C004 longhand */

/***********************************************************
 * definitions for loader fns
 ***********************************************************/

EXTERN LINK	OpenLink(char *Name);
EXTERN int	CloseLink(LINK LinkId);
EXTERN int	ReadLink(LINK LinkId, unsigned char *Buffer, unsigned int Count, int Timeout);
EXTERN int	WriteLink(LINK LinkId, unsigned char *Buffer, unsigned int Count, int Timeout);
EXTERN int	ResetLink(LINK LinkId);
EXTERN int	AnalyseLink(LINK LinkId);
EXTERN int	TestError(LINK LinkId);

int		readbytes(LINK TheLink,
			  unsigned char *string, unsigned int maxlength);
int		getiserver(LINK TheLink, unsigned int *length,
			     unsigned char *string, unsigned int maxlength);
int		sendiserver(LINK TheLink, unsigned int length,
					    unsigned char *buffer);
void		setroute(LINK TheLink, struct tpstats * p, int lastlink);
void		tpboot(LINK TheLink, struct tpstats * p);
int		load(LINK TheLink, struct tpstats * p,
				     long CodeSize,
				     long Offset,
				     long WorkSpace,
				     long VectorSpace,
				     int BytesPerWord,
				     unsigned char *Code);

#define stop(p)	    setroute(TheLink, p, TAG_HALT)


/*
 * eof
 */
