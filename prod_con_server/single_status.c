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
	char		*buf;
	char		*port;		
	char		*host = "localhost";
	char		*value;
	char		*ending = "\r\n";
	int		cc;
	int		csock;
	
	switch( argc ) {
		case    3:
			port = argv[1];
			value = argv[2];
			break;
		case	4:
			host = argv[1];
			port = argv[2];
			value = argv[3];
			break;
		default:
			fprintf( stderr, "usage: status [host] port value\n" );
			exit(-1);
	}

	if ( ( csock = connectsock( host, port, "tcp" )) == 0 ) {
		fprintf( stderr, "Cannot connect to server.\n" );
		exit( -1 );
	}

	// allocating space for the buffer to send to the server: 7 for "STATUS/", 3 for "\r\n\0"
	buf = malloc(7 + strlen(value) + 3);
	strcpy(buf, "STATUS/");
	strcat(buf, value);
	strcat(buf, ending);

	if ( write( csock, buf, strlen(buf) ) < 0 ) {
		fprintf( stderr, "Status client write: %s\n", strerror(errno) );
		exit( -1 );
	}
	
	if ( (cc = read( csock, buf, BUFSIZE )) <= 0 ) {
        	printf( "Uknown command given or error on the server.\n" );
        } else {
                buf[cc] = '\0';
                printf( "Server's response to requested command: %s\n", buf );
	}
	close( csock );
}
