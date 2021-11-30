#include <stdio.h>
#include "filesys.h"

int main(void) {
	
	// format our virtual disk
	format();
	
	// Open a new file or continue writing to an existing one
	MyFILE * newfile = myfopen("testfile.txt", "w");
	
	// populate the file with 4kb worth of 'X's
	// for (int i = 0; i < 4 * BLOCKSIZE; i++) myfputc('X', newfile);
	
	// close file
	// myfclose(newfile);

	// write the contents of the virtualdisk into a file called "virtualdiskD3_D1"
	writedisk("virtualdiskC3_C1");

	// test myfgetc(...)
	
  return 0;
}