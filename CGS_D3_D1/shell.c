#include <stdio.h>
#include "filesys.h"

int main(void) {
	
	// format our virtual disk
	format();
	// write the contents of the virtualdisk into a file called "virtualdiskD3_D1"
	writedisk("virtualdiskD3_D1");
	
  return 0;
}