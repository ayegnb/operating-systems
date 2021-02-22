#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "prodcon.h"

int main( int argc, char *argv[] ) {
	char		value[BUFSIZE];
	char		*buf;
	char		*port;		
	char		*host = "localhost";
	char		*ending = "\r\n";
	int		cc;
	int		csock;
	
	switch( argc ) {
		case    2:
			port = argv[1];
			break;
		case    3:
			host = argv[1];
			port = argv[2];
			break;
		default:
			fprintf( stderr, "usage: status [host] port\n" );
			exit(-1);
	}

	for (;;) {
		printf( "Enter the status value you want to get or type q to quit.\n" );
		fflush( stdout );

		fgets( value, BUFSIZE, stdin );

		if ( value[0] == 'q' ) {
			close(csock);
			break;
		}

		// allocating space for the buffer to send to the server: 7 for "STATUS/", 3 for "\r\n\0"
		buf = malloc(7 + strlen(value) + 3);
		strcpy(buf, "STATUS/");
		strcat(buf, value);
		strcat(buf, ending);

		if ( ( csock = connectsock( host, port, "tcp" )) == 0 )
		{
			fprintf( stderr, "Cannot connect to server.\n" );
			exit( -1 );
		}

		if ( write( csock, buf, strlen(buf) ) < 0 ) {
			fprintf( stderr, "Status client write: %s\n", strerror(errno) );
			exit( -1 );
		}	

		if ( (cc = read( csock, buf, BUFSIZE )) <= 0 ) {
                	printf( "Uknown command given or error on the server.\n" );
                        close(csock);
                        break;
                } else {
                        buf[cc] = '\0';
                        printf( "Server's response to requested command: %s\n", buf );
		}
		
		close( csock );
	}
}
