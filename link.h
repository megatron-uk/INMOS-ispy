EXTERN LINK	OpenLink(char *Name);
EXTERN int	CloseLink(LINK LinkId);
EXTERN int	ReadLink(LINK LinkId, char *Buffer, unsigned int Count, int Timeout);
EXTERN int	WriteLink(LINK LinkId, char *Buffer, unsigned int Count, int Timeout);
EXTERN int	ResetLink(LINK LinkId);
EXTERN int	AnalyseLink(LINK LinkId);
EXTERN int	TestError(LINK LinkId);

/*
 *  eof
 */
