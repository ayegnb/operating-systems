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
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>
#include "prodcon.h"

int             count;
int             clients;
int             producers;
int             consumers;
int		prod_served;
int		con_served;
int		clients_rejected;
int		clients_not_identified;
int		prod_rejected;
int		con_rejected;
ITEM**          buf;
pthread_mutex_t count_mutex;
pthread_mutex_t mutex;
sem_t           full;
sem_t           empty;

void handle_status_client(char *buffer, int fd) {
	char	*status_string;

	if ( strstr( buffer, "STATUS/CURRCLI" ) != NULL ) {
		sprintf( status_string, "%d\r\n", clients );
		write( fd, status_string, strlen(status_string) );

	} else if ( strstr( buffer, "STATUS/CURRPROD" ) != NULL ) {
		sprintf( status_string, "%d\r\n", producers );
		write( fd, status_string, strlen(status_string) );

	} else if ( strstr( buffer, "STATUS/CURRCONS" ) != NULL ) {
		sprintf( status_string, "%d\r\n", consumers );
		write( fd, status_string, strlen(status_string) );

	} else if ( strstr( buffer, "STATUS/TOTPROD" ) != NULL ) {
		sprintf( status_string, "%d\r\n", prod_served );
		write( fd, status_string, strlen(status_string) );

	} else if ( strstr( buffer, "STATUS/TOTCONS" ) != NULL ) {
		sprintf( status_string, "%d\r\n", con_served );
		write( fd, status_string, strlen(status_string) );

	} else if ( strstr( buffer, "STATUS/REJMAX" ) != NULL ) {
		sprintf( status_string, "%d\r\n", clients_rejected );
		write( fd, status_string, strlen(status_string) );

	} else if ( strstr( buffer, "STATUS/REJSLOW" ) != NULL ) {
		sprintf( status_string, "%d\r\n", clients_not_identified );
		write( fd, status_string, strlen(status_string) );

	} else if ( strstr( buffer, "STATUS/REJPROD" ) != NULL ) {
		sprintf( status_string, "%d\r\n", prod_rejected );
		write( fd, status_string, strlen(status_string) );

	} else if ( strstr( buffer, "STATUS/REJCONS" ) != NULL ) {
		sprintf( status_string, "%d\r\n", con_rejected );
		write( fd, status_string, strlen(status_string) );

        } else {
                printf( "Server: unknown command.\n" );
                fflush( stdout );
        }
	pthread_mutex_lock( &mutex );
        clients--;
        pthread_mutex_unlock( &mutex );
	
	close( fd );
}

void *handle_producer( void *fd_arg ) {
        int             item_len = 0;
        int             fd;
	ITEM*           item;

        fd = (int) fd_arg;

	if ( write( fd, "GO\r\n", 4 ) ) {
                item = malloc( sizeof(ITEM) );

                if ( read( fd, &item_len, 4 ) ) {
                        item_len = ntohl( item_len );
                        item->size = item_len;
			item->psd = fd;

			sem_wait( &empty );

                        pthread_mutex_lock( &count_mutex );
                        buf[count] = item;
                        count++;
                        printf( "Count is %d.\n", count );
                        pthread_mutex_unlock( &count_mutex );

                        sem_post( &full );

                        printf( "Server: New item was added.\n" );

		}
                        pthread_mutex_lock( &mutex );
                        producers--;
                        clients--;
			prod_served++;
                        pthread_mutex_unlock( &mutex );
                        
                        pthread_exit( 0 );
        }
}

void *handle_consumer( void *fd_arg ) {
        int             fd;
	int		letters_index;
	int		cc = 1;
	int		i;
	uint32_t        item_len;
	char		*letters;
	ITEM*           item;

        fd = (int) fd_arg;

	sem_wait( &full );

	pthread_mutex_lock( &count_mutex );
	item = buf[count-1];
	buf[count-1] = NULL;
	count--;
	printf( "Count is %d.\n", count );
	pthread_mutex_unlock( &count_mutex );

	sem_post( &empty );

	item_len = htonl(item->size);
	
	write( fd, &item_len, 4 );

	if( write( item->psd, "GO\r\n", 4 ) == -1 ) {
		printf( "Server: error sending GO to the producer before streaming letters to the consumer.\n" );
		fflush( stdout );
		pthread_exit( 0 );
	}

	if ( item->size < BUFSIZE ) {
		letters = malloc( ( item->size + 1 ) * sizeof( char ) );
		letters_index = 0;

		do {
		        if ( item->size <= letters_index ) break;
		
		        cc = read( item->psd, ( void* ) ( letters + letters_index ), item->size - letters_index );
		        letters_index += cc;
		} while ( cc != 0 );

		if ( letters_index < item->size ) {
			printf( "Server: the client has gone.\n" );
		} else {
			letters[item->size] = '\0';
			write( fd, letters, item->size );
			free( letters );

			write( item->psd, "DONE\r\n", 6 );

			printf( "Server: Consumer successfully got the item.\n" );
		}
	} else {
		for ( i = BUFSIZE; i <= item->size; i += BUFSIZE ) {

			letters = malloc( ( BUFSIZE + 1 ) * sizeof( char ) );
			letters_index = 0;

			do {
				if ( BUFSIZE <= letters_index ) break;
			
				cc = read( item->psd, ( void* ) ( letters + letters_index ), BUFSIZE - letters_index );
				letters_index += cc;
			} while ( cc != 0 );

			if ( letters_index < BUFSIZE ) {
				printf( "Server: the client has gone.\n" );
			} else {
				letters[BUFSIZE] = '\0';
				write( fd, letters, BUFSIZE );
				free( letters );
			}
		}

		if ( item->size % BUFSIZE != 0 ) {
			letters = malloc( ( (item->size % BUFSIZE) + 1 ) * sizeof( char ) );
			letters_index = 0;

			do {
				if ( (item->size % BUFSIZE) <= letters_index ) break;
			
				cc = read( item->psd, ( void* ) ( letters + letters_index ), (item->size % BUFSIZE) - letters_index );
				letters_index += cc;
			} while ( cc != 0 );

			if ( letters_index < (item->size % BUFSIZE) ) {
				printf( "Server: the client has gone.\n" );
			} else {
				letters[(item->size % BUFSIZE)] = '\0';
				write( fd, letters, (item->size % BUFSIZE ) );
				free( letters );

				write( item->psd, "DONE\r\n", 6 );

				printf( "Server: Consumer successfully got the item.\n" );
			}
		} else {
			if ( letters_index >= BUFSIZE ) {
				write( item->psd, "DONE\r\n", 6 );

				printf( "Server: Consumer successfully got the item.\n" );
			}
		}
	}

        pthread_mutex_lock( &mutex );
        consumers--;
        clients--;
	con_served++;
        pthread_mutex_unlock( &mutex );
        
	close( item->psd );
	close( fd );
	free( item );
        pthread_exit( 0 );
}

int main( int argc, char *argv[] ) {
        char                    *service;
        char                    buffer[16];
        struct sockaddr_in      fsin;
        socklen_t               alen;
        int                     msock;
        int                     ssock;
        int                     rport = 0;
        int                     cc = 0;
        int                     status;
        int                     max_items = 0;
	int			nfds;
	int			fd;
	int			select_val;
	int			i;
        bool                    can_accept;
        pthread_t               thread;
	fd_set			rfds;
	fd_set			afds;
	time_t			raw_time;
	time_t			creation_time;
	time_t			arrival_times[512];

        if ( argc == 2 ) {
                rport = 1;
                max_items = atoi( argv[1] );
        } else if ( argc == 3 ) {
                service = argv[1];
                max_items = atoi( argv[2] );
        } else {
                fprintf( stderr, "usage: server [port] maxitems\n" );
                exit(-1);
        }

        if ( max_items <= 0 ) {
                fprintf( stderr, "Server: maxitems must be more than zero.\n" );
	        exit( -1 );
        }

        buf = malloc( max_items * sizeof( ITEM* ) );

        pthread_mutex_init( &count_mutex, 0 );
        pthread_mutex_init( &mutex, 0 );
        sem_init( &full, 0, 0 );
        sem_init( &empty, 0, max_items );

        count = 0;
        clients = 0;
	clients_rejected = 0;
	clients_not_identified = 0;
        consumers = 0;
        producers = 0;
	con_served = 0;
	prod_served = 0;
	con_rejected = 0;
	prod_rejected = 0;

	for ( i = 0; i < 512; i++ ) arrival_times[i] = -1;

        msock = passivesock( service, "tcp", QLEN, &rport );
        if ( rport ) {
                //      Tell the user the selected port
                printf( "Server: port is %d.\n", rport );
                fflush( stdout );
        }

        nfds = msock + 1;

        FD_ZERO( &afds );

        FD_SET( msock, &afds );

        for (;;) {
                memcpy((char *)&rfds, (char *)&afds, sizeof(rfds));

		struct timeval timeout = {REJECT_TIME, 0};
		select_val = select( nfds, &rfds, (fd_set *) 0, (fd_set *) 0, &timeout );

                if ( select_val < 0) {
                        fprintf( stderr, "server select: %s\n", strerror(errno) );
                        exit(-1);
                }

                if (FD_ISSET( msock, &rfds ))
                {
                        alen = sizeof(fsin);
                        ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
                        if (ssock < 0) {
                                fprintf( stderr, "accept: %s\n", strerror(errno) );
                                exit(-1);
                        }

                        pthread_mutex_lock( &mutex );
                        if ( clients < MAX_CLIENTS ) {
                                clients++;
                                FD_SET( ssock, &afds );

				time( &creation_time );
				arrival_times[ssock] = creation_time;

                                if ( ssock+1 > nfds ) nfds = ssock+1;

                        } else {
				clients_rejected++;
                                close( ssock );
                                fprintf( stderr, "Server: rejected the client.\n" );
                        }
                        pthread_mutex_unlock( &mutex );
                }

                int descriptors[nfds];
                for ( fd = 0; fd < nfds; fd++ ) {
			if (select_val == 0) continue;

			time( &raw_time );
			if ( fd != msock && arrival_times[fd] != -1 && raw_time-arrival_times[fd] > REJECT_TIME ) {

				printf( "Client too long has not identified itself. Rejecting.\n" );
				fflush( stdout );
				pthread_mutex_lock( &mutex );
				clients_not_identified++;
				clients--;
				pthread_mutex_unlock( &mutex );
				arrival_times[fd] = -1;
				close( fd );
				FD_CLR( fd, &afds );
				if ( fd+1 == nfds ) nfds--;

			} else if (fd != msock && FD_ISSET(fd, &rfds)) {
                                if(( cc = read( fd, buffer, 15 ) )) {
                                        buffer[cc] = '\0';
					arrival_times[fd] = -1;

                                        if ( strstr( buffer, "PRODUCE\r\n" ) != NULL ) {
                                                pthread_mutex_lock( &mutex );
                                                can_accept = producers < MAX_PROD;
                                                if ( can_accept ) producers++;
                                                pthread_mutex_unlock( &mutex );

                                                if( can_accept ) {
                                                        descriptors[fd] = fd;
                                                        pthread_create( &thread, NULL, handle_producer, (void *) descriptors[fd]);
                                                } else {
		                                        printf( "Server: rejected the producer.\n" );
		                                        fflush( stdout );

		                                        pthread_mutex_lock( &mutex );
		                                        clients--;
							prod_rejected++;
		                                        pthread_mutex_unlock( &mutex );
		                                        close( fd );
						}
                                        } else if ( strstr( buffer, "CONSUME\r\n" ) != NULL ) {
                                                pthread_mutex_lock( &mutex );
                                                can_accept = consumers < MAX_CON;
                                                if ( can_accept ) consumers++;
                                                pthread_mutex_unlock( &mutex );

                                                if( can_accept ) {
                                                        descriptors[fd] = fd;
                                                        pthread_create( &thread, NULL, handle_consumer, (void *) descriptors[fd]);
                                                } else {
		                                        printf( "Server: rejected the consumer.\n" );
		                                        fflush( stdout );

		                                        pthread_mutex_lock( &mutex );
		                                        clients--;
							con_rejected++;
		                                        pthread_mutex_unlock( &mutex );
		                                        close( fd );
						}
					} else {
						handle_status_client(buffer, fd);
					}
                                }

                                FD_CLR( fd, &afds );
                                if ( fd+1 == nfds ) nfds--;
                        }

                }
        }      
        free(buffer);
}
