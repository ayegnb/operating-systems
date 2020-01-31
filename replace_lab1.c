/*
Operating Systems
File I/O with system calls – part 3

In this lab, you will continue to experiment with some of the system calls introduced last week, in particular: open, close, read, write and lseek.  Each person should submit individually, when the lab is due, regardless of whether the task is finished or not.
Always approach your programming problems, no matter how simple, by writing the steps of your planned solution in comments before beginning coding them.  Do not concern yourself with error handling until after you have the primary functionality working.
Begin with the solution from Monday.
The program must take 4 arguments: 
	•	a source file name, 
	•	a destination file name,
	•	a file offset, and 
	•	a number of characters.  
prog sourcefile destfile offset num
This program must read num bytes from the offset in the source file and write those num bytes at the same offset in the destination file.
You must use the system calls listed above, not stdio functions.  You may use printf for printing messages and string.h functions.  Do not make any assumptions about the contents of the files or their sizes.
You must assure that the source file exists and is large enough to be able to take num bytes from the requested offset.  The destination file does not have to exist and should be created if it doesn’t exist.  Thus the file size also doesn’t matter – just write the bytes in the requested location.
You can and should use the same replace_at function that you wrote for the previous exercise to write into the destination file.
NOTE: If you need to examine a file that begins with nulls, you may have to use a program called od, for octal dump:
od -c filename
*/

#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>

int replace_at(int fd1, int fd2, off_t offset, int n);

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

	int replaced;
	if((replaced = replace_at(fd1, fd2, atoi(argv[3]), atoi(argv[4]))) == -1) {
		exit(-1);
	} else {
		close(fd1);
		close(fd2);
		exit(0);
	}
}

int replace_at(int fd1, int fd2, off_t offset, int n) {
	// creating string to store bytes from the source file
	char buf[n+1];


	// READING FROM THE SOURCE FILE

	// finding the number of bytes in the source file
	off_t fd1_size = lseek(fd1, 0, SEEK_END)+1;

	// changing offset back to the beginning of the source file
	lseek(fd1, 0, SEEK_SET);

	// checking if the offset is in the range of bytes in the source file
	if((offset+n) >= fd1_size) {
		printf("The provided offset is out of range of provided source file. Terminating program.\n");
		return -1;
	}

        // changing the offset to the given one
        off_t new_offset = lseek(fd1, offset, SEEK_SET);

        // if something gone wrong return -1
        if(new_offset == -1) return -1;

	// read bytes from the source file to the string buf
	if(read(fd1, buf, n) == -1) {
		printf("Error reading fron the source file.\n");
		return -1;
	} else {
		printf("The string read from the source file: %s.\n", buf);
	}


	// WRITING TO THE DESTINATION FILE

	// changing the offset in the destination file
	lseek(fd2, offset, SEEK_SET);

	// writing bytes to the destination file
	if(write(fd2, buf, n) == -1) {
		printf("Error writing bytes to the destination file.\n");
		return -1;
	} else {
		return new_offset;
	}
}
