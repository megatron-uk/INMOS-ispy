
/*
 --   ---------------------------------------------------------------------------
 --
 --      ISERVER  -  INMOS standard file server
 --
 --      b004link.c
 --
 --      Link module for B004 type boards in IBM PCs running Linux
 --
 --      Copyright (c) INMOS Ltd., 1988.
 --      Copyright (c) Christoph Niemann, 1993
 --      All Rights Reserved.
 --
 --   ---------------------------------------------------------------------------
*/

#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "link-driver.h"
#include "linkio.h"
#include "inmos.h"
#include "iserver.h"


#define NULL_LINK -1

PRIVATE LINK ActiveLink = NULL_LINK;

/*
 *   Open Link
 */
PUBLIC LINK OpenLink ( Name )
	char *Name;
{

	if ( ActiveLink != NULL_LINK )
	{
		return( ER_LINK_CANT );
	}
	if ( *Name == '\0')
	{
		if ( ( ActiveLink = open( "/dev/link0" , O_RDWR ) ) < 0 )
		{
			perror( "/dev/link");
			return( ER_LINK_BUSY );
		}
	}
	else
	{
		if ( ( ActiveLink = open( Name , O_RDWR ) ) < 0 )
		{
			perror( Name );
			return( ER_LINK_BUSY );
		}
	}
	
	return(ActiveLink);
}

/*
 *   Close Link
 */
int CloseLink ( LinkId )
	int LinkId;
{
	if ( LinkId != ActiveLink )
	{
		perror("Error - ispy - Link not active ");
		return( -1 );
	}
	close(ActiveLink);
	ActiveLink = NULL_LINK;
	return(TRUE);
}

/*
 *   Read Link
 */
int ReadLink ( LinkId, Buffer, Count, Timeout )
	LINK LinkId;
	char *Buffer;
	unsigned int Count;
	int Timeout;
{
	return (read(LinkId, Buffer, Count));
}

/*
 *   Write Link
 */
int WriteLink ( LinkId, Buffer, Count, Timeout )
	LINK LinkId;
	char *Buffer;
	unsigned int Count;
	int Timeout;
{
	return (write(LinkId, Buffer, Count));
}

/*
 *   Reset Link
 */
PUBLIC int ResetLink ( LinkId )
	LINK LinkId;
{
	if ( LinkId != ActiveLink )
	{
		perror("Error - ispy - Link not active ");
		return( -1 );
	}
	if ( ioctl( LinkId, LINKRESET, 0) != 0 )
	{
		perror("Error - ispy - ioctl failed ");
		return( -1 );
	}
	return( 1 );
}

/*
 *   Analyse Link
 */ 
PUBLIC int AnalyseLink ( LinkId )
	LINK LinkId;
{
	if ( LinkId != ActiveLink )
	{
		perror("Error - ispy - Link not active ");
		return( -1 );
	}
	if ( ioctl( LinkId, LINKANALYSE, 0) != 0 )
	{
		perror("Error - ispy - ioctl failed ");
		return( -1 );
	}
	return( 1 );
}

/*
 *   Test Error
 */ 
PUBLIC int TestError ( LinkId )
	LINK LinkId;
{
	if ( LinkId != ActiveLink )
	{
		perror("Error - ispy - Link not active ");
		return( -1 );
	}
	return ( ioctl( LinkId, LINKERROR, 0 ) );
}

/*
 *   Test Read
 */
PUBLIC int TestRead ( LinkId )
	LINK LinkId;
{
	if ( LinkId != ActiveLink )
	{
		perror("Error - ispy - Link not active ");
		return( -1 );
	}
	return ( ioctl( LinkId, LINKREADABLE, 0 ) );
}




/*
 *   Test Write
 */

PUBLIC int TestWrite ( LinkId )
	LINK LinkId;
{
	if ( LinkId != ActiveLink )
	{
		perror("Error - ispy - Link not active ");
		return( -1 );
	}
	return ( ioctl( LinkId, LINKWRITEABLE, 0 ) );
}



/*
 *   Eof
 */
