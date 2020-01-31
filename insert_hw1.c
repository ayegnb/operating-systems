/*
Operating Systems
Homework 1

In this homework, start with the last in-class assignment and change the function replace_at, which was used to write num characters at the offset into the destination file.   For the in-class work, the function writes the bytes no matter the original file size.  If the file has bytes in it, they get overwritten.
Instead of the replace_at function, you should write the function:
insert_at( int fd, off_t offset, char *buf, int n )
This function should insert the n chars from the buf at the offset in the file.  If bytes are already there, the new bytes should be inserted in between them.  If there are no bytes already there, then the new bytes just should be written.  The destination file’s size should be num bytes larger after the program runs sucessfully.
The program must meet all the specifications of the in-class exercise, except for the function explained above.  It should give error messages about any problems with the arguments or the files or the I/O.
The insert_at function is independent work – it must be done alone.  It should contain all its own error handling.
*/

#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>

int insert_at(int fd1, int fd2, off_t offset, int n);

int main(int argc, char **argv){

        if(argc != 5) {
                printf("Wrong number of arguments, should include: prog filesrc filedest offset num.\n");
                exit(-1);
        }

        int fd1 = open(argv[1], O_RDWR);
        int fd2 = open(argv[2], O_RDWR | O_CREAT, S_IRWXU);

        if(fd1 != -1 && fd2 != -1) {
                printf("The source file descriptor is: %d\n", fd1);
                printf("The destination file descriptor is: %d\n", fd2);
        } else {
                perror("Error: ");
                printf("The error while opening either of the files happened.\n");
                exit(-1);
        }

        int inserted;
        if((inserted = insert_at(fd1, fd2, atoi(argv[3]), atoi(argv[4]))) == -1) {
                exit(-1);
        } else {
                close(fd1);
                close(fd2);
                exit(0);
        }
}

int insert_at(int fd1, int fd2, off_t offset, int n) {
        // creating string to store bytes from the source file
        char buf[n+1];


        // READING FROM THE SOURCE FILE

        // finding the number of bytes in the source file
        off_t fd1_size = lseek(fd1, 0, SEEK_END) - 1;

        // changing offset back to the beginning of the source file
        lseek(fd1, 0, SEEK_SET);

        // checking if the offset is in the range of bytes in the source file
        if((offset+n) >= fd1_size) {
                printf("The provided offset is out of range of provided source file. Terminating program.\n");
                return -1;
        }

        // changing the offset to the given one
        lseek(fd1, offset, SEEK_SET);

        // read bytes from the source file to the string buf
        if(read(fd1, buf, n) == -1) {
		perror("Error: ");
                printf("Error reading from the source file.\n");
                return -1;
        } else {
                printf("The string read from the source file: %s.\n", buf);
        }


        // INSERTING INTO THE DESTINATION FILE

	// finding the number of bytes in the destindation file
        off_t fd2_size = lseek(fd2, 0, SEEK_END) - 1;

        // changing offset back to the beginning of the destination file
        lseek(fd2, 0, SEEK_SET);

	// checking if bytes in the file should be shifted right
	if(fd2_size-offset > 0) {
		// need to shift
		char shifted[fd2_size-offset];
		lseek(fd2, offset, SEEK_SET);

		if(read(fd2, shifted, fd2_size-offset) == -1) {
			perror("Error: ");
			return -1;
		} else {
			printf("The string to be shifted is: %s\n", shifted);
			lseek(fd2, offset+strlen(buf), SEEK_SET);
			if(write(fd2, shifted, strlen(shifted-1)) == -1) {
				perror("Error: ");
				return -1;
			} else {
				printf("Successfully shifted bytes in the destination file.\n");
			}
		}
	}

        // changing the offset in the destination file
        lseek(fd2, offset, SEEK_SET);

        // writing bytes to the destination file
        if(write(fd2, buf, n) == -1) {
                printf("Error writing bytes to the destination file.\n");
                return -1;
        } else {
                return 0;
        }
}
