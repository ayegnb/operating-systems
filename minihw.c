#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SIZE 1024

int rows, cols;
int m1[MAX_SIZE][MAX_SIZE];
int m2[MAX_SIZE][MAX_SIZE];
int result[MAX_SIZE][MAX_SIZE];

struct arg_struct {
	int *mat1;
	int *mat2;
	int *res;
	int tid;
	int r;
	int c;
};

void compute_sequential();

void *compute_threads(void *tid);

int main(int argc, char **argv) {

	if(argc != 5) {
		printf("Wrong number of arguments, correct usage: progname rows cols val1 val2\n");
		return -1;
	}

	int status, i, j, val1, val2;
	rows = atoi( argv[1] );
	cols = atoi( argv[2] );
	rows = ( rows > MAX_SIZE ) ? MAX_SIZE : rows;
        cols = ( cols > MAX_SIZE ) ? MAX_SIZE : cols;
        val1 = atoi(argv[3]);
        val2 = atoi(argv[4]);

	pthread_t threads[rows];
	int index[rows];
	struct timeval begin;
	struct timeval end;
	double elapsed;

	for( i = 0; i < rows; i++ ) {
		for( j = 0; j < cols; j++ ) {
			m1[i][j] = val1;
		}
	}

	for( i = 0; i < cols; i++ ) {
		for( j = 0; j < rows; j++ ) {
			m2[i][j] = val2;
		}
	}

	// sequential computation
	gettimeofday(&begin, NULL);
	
	compute_sequential();
	
	gettimeofday(&end, NULL);
	
	elapsed = 1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec);
	fprintf( stderr, "Time elapsed after sequential computation: %f microseconds.\n", elapsed );

	// computation with threads
	gettimeofday(&begin, NULL);
	for( i = 0; i < rows; i++ ) {
		index[i] = i;
		status = pthread_create( &threads[i], NULL, compute_threads, (void*) &index[i] );
		
		if ( status != 0 ) {
			fprintf( stderr, "pthread_create returned error %d.\n", 
				status );
			return -1;
		}
	}

	gettimeofday(&end, NULL);

        for( i = 0; i < rows; i++ ){
                for( j = 0; j < rows; j++ )
                        printf( " %d ", result[i][j] );
                printf( "\n" );
        }

        elapsed = 1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec);
	fprintf( stderr, "Time elapsed after computation with threads: %f microseconds.\n", elapsed );

	for ( i = 0; i < rows; i++ ) pthread_join( threads[i], NULL );
	
	pthread_exit(0);
}

void compute_sequential() {

	int i, j, k, sum = 0;

	for( i = 0; i < rows; i++ ) {
		for( j = 0; j < rows; j++ ) {
			for( k = 0; k < cols; k++ ) {
				sum += m1[i][k] * m2[k][j];
			}

			result[i][j] = sum;
			sum = 0;
		}
	}
}

void *compute_threads( void *tid ) {
	int *row = (int *)tid;
	int j, k, sum = 0;

	for( j = 0; j < rows; j++ ) {
		for( k = 0; k < cols; k++ ) {
			sum += m1[*row][k] * m2[k][j];
		}

		result[*row][j] = sum;
		sum = 0;
	}

	pthread_exit( NULL );
}
