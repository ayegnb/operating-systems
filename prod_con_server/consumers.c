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
#include <fcntl.h>
#include <math.h>
#include "prodcon.h"

char	*port;
char	*host;
double	rate;
double	bad;

double poisson_random_interarrival_delay( double r ) {
	return ( log( (double) 1.0 - ( (double) rand() )/( (double) RAND_MAX ) ) )/-r;
}

void consume_letters( char **letters, int stream_size ) {
	int		dev_file;

	dev_file = open( "/dev/null", O_WRONLY );

	if ( dev_file == -1 ) {
		fprintf( stderr, "Consumer: cannot open the file dev/null.\n" );
	} else {
		write( dev_file, *letters, stream_size );
		close( dev_file );
	}
}

void *consume() {
        char		*letters;
	char            file_name[40];
	char		bytes[12];
	int		csock;
        int             letters_index;
        int             item_len;
        int             cc = 0;
	int		i;
	int             fd;
	int		bytes_received;
	double		bad_client_prob;
	double		sleep_time;
	pthread_t       thread_id;
	ITEM            item;

	// sleeping random amount of time
	sleep_time = poisson_random_interarrival_delay( rate ) * 1000000;

	printf( "I'm sleeping for %f milliseconds\n", sleep_time );
	fflush( stdout );

	if ( usleep( sleep_time ) == -1 ) {
		printf( "Producer: usleep error %s\n", strerror(errno) );
		exit( -1 );
	}

	if ( ( csock = connectsock( host, port, "tcp" )) == 0 ) {
                fprintf( stderr, "Consumer: cannot connect to server.\n" );
                exit( -1 );
        }

	// determine the probability of a bad client with the random function
	bad_client_prob = rand() % 100;
	
	if ( bad_client_prob <= bad ) {
		printf( "Bad client\n" );
		fflush( stdout );
		sleep( SLOW_CLIENT );
	}

        if ( write( csock, "CONSUME\r\n", 9) < 0 ) {
                fprintf( stderr, "Consumer: CONSUME token writing error: %s\n", strerror(errno) );
                pthread_exit( 0 );
        }

	thread_id = pthread_self();
	sprintf( file_name, "%ld.txt", (long) thread_id );

	if ( (fd = open( file_name, O_CREAT | O_WRONLY, S_IRWXU )) == -1 ) {
		fprintf( stderr, "Consumer: cannot open the file %s.\n", file_name );
	}

        cc = read( csock, &item_len, 4 );

        if ( cc == 4 ) {
                item_len = ntohl( item_len );

                item.size = item_len;

		if ( item_len < BUFSIZE ) {
			letters = malloc( ( item_len + 1 ) * sizeof( char ) );
			letters_index = 0;
			cc = 1;

			do {
				if ( item_len <= letters_index ) break;

				cc = read( csock, ( void* ) ( letters + letters_index ), item_len - letters_index );
				letters_index += cc;
				bytes_received += cc;
			} while( cc != 0 );

			if ( letters_index == item_len ) {
		                letters[item_len] = '\0';
		                consume_letters( &letters, item_len );
		                free( letters );

				sprintf( bytes, " %d", bytes_received );
				printf( "Consumer: received%s bytes\n", bytes );
				fflush( stdout );

				if ( bytes_received == item_len ) {
					write( fd, SUCCESS, strlen(SUCCESS) );
					lseek( fd, strlen(SUCCESS), SEEK_SET );
					write( fd, bytes, strlen(bytes) );
				} else {
					write( fd, BYTE_ERROR, strlen(BYTE_ERROR) );
					lseek( fd, strlen(BYTE_ERROR), SEEK_SET );
					write( fd, bytes, strlen(bytes) );
				}
				close( fd );
		                close( csock );
			        pthread_exit( 0 );
			} else {
				printf( "Consumer: undetermined error.\n" );
				write( fd, REJECT, strlen(REJECT) );
				close( fd );
		                close( csock );
			        pthread_exit( 0 );
			}
		} else {
			for ( i = BUFSIZE; i <= item_len; i += BUFSIZE ) {
				letters = malloc( ( BUFSIZE + 1 ) * sizeof( char ) );
				letters_index = 0;
				cc = 1;

				do {
					if ( BUFSIZE <= letters_index ) break;

					cc = read( csock, ( void* ) ( letters + letters_index ), BUFSIZE - letters_index );
					letters_index += cc;
					bytes_received += cc;
				} while( cc != 0 );

				if ( letters_index == BUFSIZE ) {
				        letters[BUFSIZE] = '\0';
				        consume_letters( &letters, BUFSIZE );
				        free( letters );
				} else {
					printf( "Consumer: undetermined error.\n" );
					write( fd, REJECT, strlen(REJECT) );
					close( fd );
				        close( csock );
					pthread_exit( 0 );
				}
			}

			if ( item_len % BUFSIZE != 0 ) {
				letters = malloc( ( (item_len % BUFSIZE) + 1 ) * sizeof( char ) );
				letters_index = 0;
				cc = 1;

				do {
					if ( (item_len % BUFSIZE) <= letters_index ) break;

					cc = read( csock, ( void* ) ( letters + letters_index ), (item_len % BUFSIZE) - letters_index );
					letters_index += cc;
					bytes_received += cc;
				} while( cc != 0 );

				if ( letters_index == (item_len % BUFSIZE) ) {
				        letters[item_len % BUFSIZE] = '\0';
				        consume_letters( &letters, item_len % BUFSIZE );
				        free( letters );

					sprintf( bytes, " %d", bytes_received );
					printf( "Consumer: received%s bytes\n", bytes );
					fflush( stdout );

					if ( bytes_received == item_len ) {
						write( fd, SUCCESS, strlen(SUCCESS) );
						lseek( fd, strlen(SUCCESS), SEEK_SET );
						write( fd, bytes, strlen(bytes) );
					} else {
						write( fd, BYTE_ERROR, strlen(BYTE_ERROR) );
						lseek( fd, strlen(BYTE_ERROR), SEEK_SET );
						write( fd, bytes, strlen(bytes) );
					}
					close( fd );
		                	close( csock );
			        	pthread_exit( 0 );
				} else {
					printf( "Consumer: undetermined error.\n" );
					write( fd, REJECT, strlen(REJECT) );
					close( fd );
				        close( csock );
					pthread_exit( 0 );
				}
			} else {
				sprintf( bytes, " %d", bytes_received );
				printf( "Consumer: received%s bytes\n", bytes );
				fflush( stdout );

				if ( bytes_received == item_len ) {
					write( fd, SUCCESS, strlen(SUCCESS) );
					lseek( fd, strlen(SUCCESS), SEEK_SET );
					write( fd, bytes, strlen(bytes) );
				} else {
					write( fd, BYTE_ERROR, strlen(BYTE_ERROR) );
					lseek( fd, strlen(BYTE_ERROR), SEEK_SET );
					write( fd, bytes, strlen(bytes) );
				}
				close( fd );
	                	close( csock );
		        	pthread_exit( 0 );
			}
		}
        } else {
		printf( "Consumer: rejected by server.\n" );
		write( fd, REJECT, strlen(REJECT) );
		close( fd );
		close( csock );
		pthread_exit( 0 );
	}

        printf( "Consumer: server quitted unexpectedly.\n" );
        close( csock );
	pthread_exit( 0 );
}

/*
**      Consumer Client
*/
int
main( int argc, char *argv[] ) {
        int             consumers;
        int             working_threads;
        int             cc = 0;
        int             status;

        if ( argc == 5 ) {
                host            = "localhost";
                port            = argv[1];
                consumers	= atoi(argv[2]);
		rate		= atof(argv[3]);
		bad		= atoi(argv[4]);
        } else if ( argc == 6 ) {
                host            = argv[1];
                port            = argv[2];
                consumers	= atoi(argv[3]);
		rate		= atof(argv[4]);
		bad		= atoi(argv[5]);
        } else {
                fprintf( stderr, "usage: consumers [host] port num rate bad\n" );
                exit(-1);
        }
	
	if ( consumers > 2000 ) {
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

        pthread_t threads[consumers];
        working_threads = 0;

        for (;;) {
                if ( consumers <= working_threads ) {
                       working_threads = 0;

                       while ( working_threads < consumers ) {
                               pthread_join( threads[working_threads], NULL );
                               working_threads++;
                       }

                       pthread_exit( 0 );
                }

                status = pthread_create( &threads[working_threads], NULL, consume, NULL );
                if ( status != 0 ) {
			fprintf( stderr, "Consumer: thread create error %d.\n", status );
			exit( -1 );
		}

		working_threads++;
        }

        fprintf( stderr, "Consumer: cannot connect to server.\n" );
	exit( -1 );

}
