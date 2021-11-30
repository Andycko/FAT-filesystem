#include <stdio.h>
#include "filesys.h"

int main(void) {
	
	// format our virtual disk
	format();
	
	// Open a new file or continue writing to an existing one
	MyFILE * newfile = myfopen("testfile.txt", "w");

	// populate the file with 4kb worth of 'X's
	for (int i = 0; i < 4 * BLOCKSIZE; i++) myfputc("ABCDEFGHIJKLMNOPQRSTUVWXWz"[i % 25], newfile);
	
	// close file
	myfclose(newfile);

	// write the contents of the virtualdisk into a file called "virtualdiskD3_D1"
	writedisk("virtualdiskC3_C1");

	// Open the previous file to read
	MyFILE * newfile2 = myfopen("testfile.txt", "r");

	// Print out whole file
	while (1) {
		char readFile = myfgetc(newfile2);
		if (readFile == EOF) break;
		printf("%c", readFile);

		// TODO: write to real file
	}
	
	// close file 2
	myfclose(newfile2);

  return 0;
}