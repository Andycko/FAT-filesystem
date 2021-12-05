#include <stdio.h>
#include "filesys.h"

int main(void) {
	
	format();
	mymkdir("/firstdir/seconddir");

	MyFILE * newfile = myfopen("/firstdir/seconddir/testfile1.txt", "w");
	for (int i = 0; i < 34; i++) myfputc("I am the content of testfile1.txt"[i], newfile);
	myfclose(newfile);

	mylistdir("/firstdir/seconddir");
	mychdir("/firstdir/seconddir");
	mylistdir(".");

	newfile = myfopen("testfile2.txt", "w");
	for (int i = 0; i < 34; i++) myfputc("I am the content of testfile2.txt"[i], newfile);
	myfclose(newfile);

	mymkdir("thirddir");

	newfile = myfopen("thirddir/testfile3.txt", "w");
	for (int i = 0; i < 34; i++) myfputc("I am the content of testfile3.txt"[i], newfile);
	myfclose(newfile);

	writedisk("virtualdiskA5_A1_a");

	myremove("testfile1.txt");
	myremove("testfile2.txt");
	
	writedisk("virtualdiskA5_A1_b");

	mychdir("thirddir");
	myremove("testfile3.txt");

	writedisk("virtualdiskA5_A1_c");

	mychdir("..");
	myrmdir("thirddir");
	mychdir("/firstdir");
	myrmdir("seconddir");
	mychdir("..");
	myrmdir("firstdir");

	writedisk("virtualdiskA5_A1_d");

  return 0;
}