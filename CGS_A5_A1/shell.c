#include <stdio.h>
#include "filesys.h"

int main(void) {
	
	// format our virtual disk
	format();
	
	// Open a new file or continue writing to an existing one
	MyFILE * newfile = myfopen("testfile.txt", "w");

	// populate the file with 4kb worth of 'X's
	for (int i = 0; i < 1 * BLOCKSIZE; i++) myfputc("ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i % 26], newfile);
	
	// close file
	myfclose(newfile);

	// write the contents of the virtualdisk into a file called "virtualdiskD3_D1"
	writedisk("virtualdiskA5_A1");

	// Open the previous file to read
	MyFILE * openfile = myfopen("testfile.txt", "r");

	// Open a real file in our folder to write to
  FILE *f = fopen("testfileA5_A1_copy.txt", "w");

	// Print out whole file
	while (1) {
		char readFile = myfgetc(openfile);
		if (readFile == EOF) break;
		// printf("%c", readFile);
		fprintf(f, "%c", readFile);
	}
	
	// close virtual disk file
	myfclose(openfile);

	// close real file
	fclose(f);

	// Create a directory/directories
	mymkdir("myfirstdir/myseconddir/mythirddir/");
	
	// List contents in myseconddir (the function returns and prints the list as well)
	mylistdir("/myfirstdir/myseconddir/");
		
	// write out virtual disk
	writedisk("virtualdiskA5_A1_a");

	// Create a file and write a Byte in it
	newfile = myfopen("myfirstdir/myseconddir/testfile.txt", "w");
	myfputc('A', newfile);
	myfclose(newfile);

	// List out the contents in myseconddir
	mylistdir("myfirstdir/myseconddir/");

	writedisk("virtualdiskA5_A1_b");
  return 0;
}