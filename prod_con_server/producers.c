#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <math.h>
#include "prodcon.h"

char	*port;
char	*host;
double	rate;
double	bad;

double poisson_random_interarrival_delay( double r ) {
	return ( log( (double) 1.0 - ( (double) rand() )/( (double) RAND_MAX ) ) )/-r;
}

void *produce() {
        char            buf[8];
	char		*letters;
        int             csock;
        int             cc = 0;
	int		i;
        uint32_t        item_len;
	ITEM            item;
	double		bad_client_prob;
	double		sleep_time;

	// sleeping random amount of time
	sleep_time = poisson_random_interarrival_delay( rate ) * 1000000;

	printf( "I'm sleeping for %f milliseconds\n", sleep_time );
	fflush( stdout );

	if ( usleep( sleep_time ) == -1 ) {
		printf( "Producer: usleep error %s\n", strerror(errno) );
		exit( -1 );
	}

	if ( ( csock = connectsock( host, port, "tcp" )) == 0 ) {
                fprintf( stderr, "Producer: cannot connect to server.\n" );
                exit( -1 );
        }

	// determine the probability of a bad client with the random function
	bad_client_prob = rand() % 100;
	
	if ( bad_client_prob <= bad ) {
		printf( "Bad client\n" );
		fflush( stdout );
		sleep( SLOW_CLIENT );
	}

        if ( write( csock, "PRODUCE\r\n", 9 ) == -1 ) {
                fprintf( stderr, "Producer: PRODUCE token writing error: %s\n", strerror(errno) );
                exit( -1 );
        }

	item.size = rand() % MAX_LETTERS;

        cc = read( csock, buf, 4 );
        buf[4] = '\0';

        if ( cc == 4 && strcmp( buf, "GO\r\n" ) == 0 ) {
                item_len = htonl( item.size );

                write( csock, &item_len, 4 );

		cc = read( csock, buf, 4 );
        	buf[4] = '\0';

		if ( cc == 4 && strcmp( buf, "GO\r\n" ) == 0 ) {

			if ( item.size < BUFSIZE ) {
				letters = malloc( item.size * sizeof( char ) );

				for ( int i = 0; i < item.size; i++ ) letters[i] = 'X';

				write( csock, letters, item.size );
				free( letters );
			} else {
				for ( i = BUFSIZE; i < item.size; i += BUFSIZE ) {
					letters = malloc( BUFSIZE * sizeof( char ) );

					for ( int i = 0; i < BUFSIZE; i++ ) letters[i] = 'X';

					write( csock, letters, BUFSIZE );
					free( letters );
				}

				if ( item.size % BUFSIZE != 0 ) {
					letters = malloc( item.size % BUFSIZE * sizeof( char ) );

					for ( int i = 0; i < item.size % BUFSIZE; i++ ) letters[i] = 'X';

					write( csock, letters, item.size % BUFSIZE );
					free( letters );
				}
			}

			cc = read( csock, buf, 6 );
			buf[6] = '\0';

			if ( cc == 6 && strcmp( buf, "DONE\r\n" ) == 0 ) {
				printf( "Producer: sent %d bytes\n", item.size );
		                close( csock );
			        pthread_exit( 0 );
			} else {
				printf( "Producer: undetermined error\n" );
		                close( csock );
			        pthread_exit( 0 );
			}
		}

		pthread_exit( 0 );
        }

        printf( "Producer: rejected by server.\n" );
        close( csock );
	pthread_exit( 0 );
}

/*
**      Producer Client
*/
int
main( int argc, char *argv[] ) {
        int             producers;
        int             working_threads;
        int             cc;
        int             csock;
        int             status;

        if ( argc == 5 ) {
                host            = "localhost";
                port            = argv[1];
                producers	= atoi(argv[2]);
		rate		= atof(argv[3]);
		bad		= atoi(argv[4]);
        } else if ( argc == 6 ) {
                host            = argv[1];
                port            = argv[2];
                producers	= atoi(argv[3]);
		rate		= atof(argv[4]);
		bad		= atoi(argv[5]);
        } else {
                fprintf( stderr, "usage: producer [host] port num rate bad\n" );
                exit(-1);
        }
	
	if ( producers > 2000 ) {
		printf( "Number of clients should be less than 2000.\n" );
		fflush( stdout );
		exit( -1 );
	}

	if ( rate <= 0 ) {
		printf( "Rate should be a floating number greater than zero.\n" );
		fflush( stdout );
		exit( -1 );
	}

	if ( bad < 0 || bad > 100 ) {
		printf( "Bad should be an integer between 0 and 100.\n" );
		fflush( stdout );
		exit( -1 );
	}

        pthread_t threads[producers];
        working_threads = 0;

        for (;;) {
                if ( producers <= working_threads ) {
                       working_threads = 0;

                       while ( working_threads < producers ) {
                               pthread_join( threads[working_threads], NULL );
                               working_threads++;
                       }
		       working_threads = 0;
                       pthread_exit( 0 );
                }

                status = pthread_create( &threads[working_threads], NULL, produce, NULL );
                if ( status != 0 ) {
			fprintf( stderr, "Producer: thread create error %d.\n", status );
			exit( -1 );
		}

		working_threads++;
        }

        fprintf( stderr, "Producer: cannot connect to server.\n" );
	exit( -1 );
}
