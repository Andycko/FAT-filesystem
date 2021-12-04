/* filesys.c
 * 
 * provides interface to virtual disk
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "filesys.h"
#include <assert.h>

diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
fatentry_t   currentDirIndex         = 0 ;

/* writedisk : writes virtual disk out to physical disk
 * 
 * in: file name of stored virtual disk
 */

void writedisk ( const char * filename )
{
   printf ( "writedisk> virtualdisk[0] = %s\n", virtualDisk[0].data ) ;
   FILE * dest = fopen( filename, "w" ) ;
   if ( fwrite ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
   fclose(dest) ;
   
}

void readdisk ( const char * filename )
{
   FILE * dest = fopen( filename, "r" ) ;
   if ( fread ( virtualDisk, sizeof(virtualDisk), 1, dest ) < 0 )
      fprintf ( stderr, "write virtual disk to disk failed\n" ) ;
   //write( dest, virtualDisk, sizeof(virtualDisk) ) ;
      fclose(dest) ;
}


/* the basic interface to the virtual disk
 * this moves memory around
 */

void writeblock ( diskblock_t * block, int block_address )
{
   //printf ( "writeblock> block %d = %s\n", block_address, block->data ) ;
   memmove ( virtualDisk[block_address].data, block->data, BLOCKSIZE ) ;
   //printf ( "writeblock> virtualdisk[%d] = %s / %d\n", block_address, virtualDisk[block_address].data, (int)virtualDisk[block_address].data ) ;
}


/* read and write FAT
 * 
 * please note: a FAT entry is a short, this is a 16-bit word, or 2 bytes
 *              our blocksize for the virtual disk is 1024, therefore
 *              we can store 512 FAT entries in one block
 * 
 *              how many disk blocks do we need to store the complete FAT:
 *              - our virtual disk has MAXBLOCKS blocks, which is currently 1024
 *                each block is 1024 bytes long
 *              - our FAT has MAXBLOCKS entries, which is currently 1024
 *                each FAT entry is a fatentry_t, which is currently 2 bytes
 *              - we need (MAXBLOCKS /(BLOCKSIZE / sizeof(fatentry_t))) blocks to store the
 *                FAT
 *              - each block can hold (BLOCKSIZE / sizeof(fatentry_t)) fat entries
 */


/*
 * Prepares the virtualdisk for usage,
 * names it, creates and populates the fat table
 */
void format ( )
{
	// prepare block 0 : fill it with '\0'
	diskblock_t block = emptyBlock();
	// use strcpy() to set the name of the disk
	strcpy(block.data, "CS3026 Operating Systems Assesment 2 - Andrej Szalma");
	// write block to virtual disk at index 0
	writeblock(&block, 0);


	// Initialize FAT table with UNUSED
	for (int i = 0; i < MAXBLOCKS; i++) FAT[i] = UNUSED;
	// Set first three records for block 0 and the FAT table itself
	// This can be typed statically as we know the size of our disk already
	FAT[0] = ENDOFCHAIN;
	FAT[1] = 2;
	FAT[2] = ENDOFCHAIN;
	// copy the FAT table to the disk
	copyFAT();

	// Create an empty directory as a wrapper and set it to root, this block will be for naming purposes
	block = emptyBlock();
	block.dir.isdir = TRUE;
	block.dir.nextEntry = 0;
	strcpy(block.dir.entrylist->name, "root");
	// Set firstblock to 4 as we will create a new block there which will hold the actual contents of the root
	block.dir.entrylist->firstblock = 4;
	block.dir.entrylist->used = TRUE;
	
	// write block to virtual disk after the FAT table
	writeblock(&block, 3);

	// Create the actual directory for root which will hold the contents of root
	block = emptyBlock();
	block.dir.isdir = TRUE;
	block.dir.nextEntry = 0;
	block.dir.name = virtualDisk[3].dir.entrylist->name;
	strcpy(block.dir.entrylist->name, ".");
	block.dir.entrylist->firstblock = 4;
	block.dir.entrylist->used = TRUE;

	// write to virtual disk after root wrapper
	writeblock(&block, 4);

	// Note in FAT table that a file exists in block 3 and save to disk
	FAT[3] = ENDOFCHAIN;
	FAT[4] = ENDOFCHAIN;
	copyFAT();

	// Set root directory index on disk and current directory index
	rootDirIndex = 4;
	currentDirIndex = 4;
}


/*
 * Creates a new file or reads/writes an existing one
 * and saves it into the vitrual disk
 */
MyFILE * myfopen(const char * filenamePath, const char * mode) {
	// Mismatched mode, return null pointer
	if (!(strcmp(mode,"r") == 0 || strcmp(mode,"w") == 0)) {
		printf("Wrong manipulation mode was selected. Modes = [\"r\",\"w\"]\n");
		return NULL;
	}
	
	int saveCurrentDirIndex = currentDirIndex;
	char * filename = malloc(sizeof(MAXNAME));	
	
	// if path is absolute then set current directory index to root
	if (filenamePath[0] == '/') {
		currentDirIndex = rootDirIndex;
	}	

	// progress to the correct directory if the filenamePath is a path
	if (strchr(filenamePath, '/') != NULL) {
		char * filenameCopy = strdup(filenamePath);
		char * path = malloc(strlen(filenamePath));
		char * token = strtok(filenameCopy, "/");

		while (1) {
			strcpy(filename, token);
			token = strtok(NULL, "/");

			if (token) {
				strcat(path,filename);
				strcat(path,"/");
				}
			else break;
		}

		mychdir(path);
		free(path);		
		free(filenameCopy);
	} else {
			strcpy(filename, filenamePath);
	}

	// create a new file, allocate memory to it, sets file mode and first byte
	MyFILE * file = malloc(sizeof(MyFILE));
	strcpy(file->mode, mode);
	file->pos = 0;

	// find current directory
	diskblock_t block = virtualDisk[currentDirIndex];
	// check if file already exists and save index
	int filepos = -1;	
	for (int i = 0; i < DIRENTRYCOUNT; i++) {
		if (block.dir.entrylist[i].used && strcmp(block.dir.entrylist[i].name, filename) == 0) {
			filepos = i;
			break;
		}
	}

	// if file exists set blockno and buffer
	if (filepos != -1) {
		file->blockno = block.dir.entrylist[filepos].firstblock;
		file->buffer = virtualDisk[file->blockno];
	// if file doesn't exist and writemode is enabled
	} else if (strcmp(mode, "w") == 0) {
		// find free file space in directory
		int freeDirPos = -1;
		for (int i = 0; i < DIRENTRYCOUNT; i++) {
			if (!block.dir.entrylist[i].used) {	
				freeDirPos = i;
				break;
			}
		}
		if (freeDirPos == -1) {
			printf("File can't be created, directory full.\n");
			free(file);
			currentDirIndex = saveCurrentDirIndex;
			free(filename);
			return NULL;
		}
		// find free space in FAT table, set it to EOC and save index into file
		int freeFat = findFreeFAT();
		FAT[freeFat] = ENDOFCHAIN;
		file->blockno = freeFat;
		copyFAT();

		// save file into directory and directory address to file
		strcpy(block.dir.entrylist[freeDirPos].name, filename);
		block.dir.entrylist[freeDirPos].firstblock = file->blockno;
		block.dir.entrylist[freeDirPos].used = TRUE;
		
		// write updated block (directory) back to the virtualdisk
		writeblock(&block, currentDirIndex);
		
		// create an empty block for the file buffer
		block = emptyBlock();
		writeblock(&block, file->blockno);
		
		// set file buffer based on updated virtualdisk 
		file->buffer = virtualDisk[file->blockno];

	// if file doesn't exist and readmode is enabled
	} else {
		printf("File doesn't exist and read mode is enabled");
		return NULL;
	}

	currentDirIndex = saveCurrentDirIndex;
	free(filename);
	return file;
}

/*
 * Write a byte into a file stream
 * and saves it into the vitrual disk
 */
void myfputc(int b, MyFILE * stream) {
	// if mode is read or file is empty, return
	if(stream == NULL || strcmp(stream->mode, "r") == 0) return;
	stream->buffer.data[stream->pos] = b;
	stream->pos++;
	stream->filelength++;
	
	if(stream->pos == BLOCKSIZE) {
		// write buffer to disk
		writeblock(&stream->buffer, stream->blockno);
		
		// find free block and update FAT
		int freeBlock = findFreeFAT();
		FAT[stream->blockno] = freeBlock;
		FAT[freeBlock] = ENDOFCHAIN;
		copyFAT();

		// create a new empty data block and write it onto the free
		diskblock_t block = emptyBlock();
		writeblock(&block, freeBlock);

		// update stream properties
		stream->blockno = freeBlock;
		stream->buffer = virtualDisk[freeBlock];

		// reset stream pos
		stream->pos = 0;
	}
}

/*
 * Saves out any remaining blocks that haven't been written 
 * and saves it into the vitrual disk
 */
void myfclose(MyFILE * stream) {
	if(stream == NULL) return;
	if (strcmp(stream->mode, "w") == 0) {
		// add EOF at the end of the file
		myfputc(EOF, stream);
		writeblock(&stream->buffer,stream->blockno);
	}
	free(stream);
}

/*
 * Returns a file in a form of a stream, always returns
 * the next byte of the file or EOF
 * (every file created by us already has EOF at the end, so no need to do any checks)
 */
char myfgetc(MyFILE * stream) {
	// when reach end of block try to go to next one
	if (stream->pos == BLOCKSIZE && FAT[stream->blockno] != ENDOFCHAIN) {
		stream->pos = 0;
		stream->blockno = FAT[stream->blockno];
		stream->buffer = virtualDisk[stream->blockno];
	}

	// increment position
	stream->pos++;
	
	return stream->buffer.data[stream->pos - 1];
} 

/*
 * Creates a new directory based on the path
 */
void mymkdir (const char * path) {
	// Save current dir positions
	dirblock_t * tmpCurrentDir = &virtualDisk[currentDirIndex].dir;

	char * pathCopy = strdup(path);

	// if path is absolute then set current directory to root
	if (pathCopy[0] == '/') {
		tmpCurrentDir = &virtualDisk[rootDirIndex].dir;
	}

	// slice path name where the slashes are
	char * dirName = strtok(pathCopy, "/");

	while (dirName) {
		// Check if directory exists
		int index = findEntry(dirName, tmpCurrentDir, 'd');
		// If exists, set current directory to that one
		if (index != -1) {			
			tmpCurrentDir = &virtualDisk[tmpCurrentDir->entrylist[index].firstblock].dir;
		} else {
			// create directory in current directory
			tmpCurrentDir = createDir(dirName, tmpCurrentDir);
		}
		
		// go to next directory in path
		dirName = strtok(NULL, "/");
	}
	free(pathCopy);
}

/*
 * Lists contents of a directory
 */
char ** mylistdir (char * path) {
	int saveCurrentDirIndex = currentDirIndex;
	char ** res = malloc(DIRENTRYCOUNT * MAXNAME * sizeof(char));

	// Temporarly change path
	mychdir(path);

	// Print out all the entries and save to resulting array
	printf("(%s) >> ", virtualDisk[currentDirIndex].dir.name);
	for (int i=0; i < DIRENTRYCOUNT; i++) {
		printf("%s\t", virtualDisk[currentDirIndex].dir.entrylist[i].name);
		res[i] = malloc(sizeof(res) * MAXNAME);
		if (strlen(virtualDisk[currentDirIndex].dir.entrylist[i].name) != 0)
			strcpy(res[i], virtualDisk[currentDirIndex].dir.entrylist[i].name);
	}
	printf("\n");

	// Change current path back to its previous state
	currentDirIndex = saveCurrentDirIndex;
	return res;
}

/*
 * Changes currentDirIndex to a different directory
 */
void mychdir (char * path) {
	// if path is absolute then set current directory to root
	if (path[0] == '/') {
		currentDirIndex = rootDirIndex;
	}

	char * pathCopy = strdup(path);
	char * dirName = strtok(pathCopy, "/");
	
	// Progress to the final directory
	while (dirName) {
		int index = findEntry(dirName, &virtualDisk[currentDirIndex].dir, 'd');
		// If exists, set current directory to that one
		if (index != -1) currentDirIndex = virtualDisk[currentDirIndex].dir.entrylist[index].firstblock;			
		else printf("Directory %s doesn't exist in %s\n", dirName, virtualDisk[currentDirIndex].dir.name);

		dirName = strtok(NULL, "/");
	}
	free(pathCopy);
}

// ### HELPER FUNCTIONS ###

/* 
 * create an empty disk block initialized with \0s
 */
diskblock_t emptyBlock() {
	diskblock_t emptyBlock;
	for (int i = 0; i < MAXBLOCKS; i++) emptyBlock.data[i] = '\0';
	return emptyBlock;
}

/*
 * Copy the contents of FAT into two blocks on
 * our virtual disk and saves it
 */
void copyFAT() {

	diskblock_t block = emptyBlock();
	
	// save the first half of the fat table into a block
	for (int i = 0; i < FATENTRYCOUNT; i++){
		block.fat[i] = FAT[i];
	}
	// write the block to the virtual disk
	writeblock(&block, 1);
	
	// save the second half of the fat table into a block
	for (int i = 0; i < FATENTRYCOUNT; i++){
		block.fat[i] = FAT[FATENTRYCOUNT + i];
	}
	// write the block to the virtual disk
	writeblock(&block, 2);
}

/*
 * Find unused block in FAT table
 */
int findFreeFAT() {
	for (int i = 4; i < MAXBLOCKS; i++) {
		if (FAT[i] == UNUSED) {
			return i;				
		}
	}
	return -1;
}

/* 
 * Find directory or file and return index if exists
 */
int findEntry(char * name, dirblock_t * currDir, char type) {
	for (int i = 0; i < DIRENTRYCOUNT; i++) {
		if (currDir->entrylist[i].used && strcmp(currDir->entrylist[i].name, name) == 0) {
				if (type == 'f' && !currDir->entrylist[i].isdir) return i;
				if (type == 'd' && currDir->entrylist[i].isdir) return i;
		}
	}
	return -1;
}

/*
 * Create a directory in a specified directory at a specified index in FAT
 */
dirblock_t * createDir(char * dirName, dirblock_t * parentBlock) {
	int freeFatIndex = findFreeFAT();
	int nextFreeEntryIndex = -1;

	// find free entry in current directory
	for (int i = 0; i < DIRENTRYCOUNT; i++) {
		if (!parentBlock->entrylist[i].used) {
			nextFreeEntryIndex = i;
			break;
		}
	}

	if (nextFreeEntryIndex != -1) {
		// Create a block for the new directory
  	diskblock_t block = emptyBlock();
    block.dir.isdir = 1;
    block.dir.nextEntry = 0;
		block.dir.name = parentBlock->entrylist[nextFreeEntryIndex].name;
		// add link to self
		block.dir.entrylist->used = TRUE;
		block.dir.entrylist->firstblock = freeFatIndex;
		strcpy(block.dir.entrylist->name, ".");
		
		// add link to parent directory
		block.dir.entrylist[1].used = TRUE;
		block.dir.entrylist[1].firstblock = parentBlock->entrylist->firstblock;
		strcpy(block.dir.entrylist[1].name, "..");
		
		// save block
    writeblock(&block, freeFatIndex);
		FAT[freeFatIndex] = ENDOFCHAIN;
		copyFAT();
    
		// Create the new driectory in the current directory
		parentBlock->entrylist[nextFreeEntryIndex].used = TRUE;
		parentBlock->entrylist[nextFreeEntryIndex].isdir = TRUE;
		parentBlock->entrylist[nextFreeEntryIndex].firstblock = freeFatIndex;
		strcpy(parentBlock->entrylist[nextFreeEntryIndex].name, dirName);

	} else {
		printf("Current directory is full\n");
		return NULL;
	}

	return &virtualDisk[freeFatIndex].dir;
}

// This function is not working properly but is enough for debugging
void printBlock ( int blockIndex )
{
	if (virtualDisk[blockIndex].dir.isdir) {
		for (int i = 0; i < DIRENTRYCOUNT; i++) {
			if (virtualDisk[blockIndex].dir.entrylist[i].used)
			printf ( "virtualdisk[%d].entrylist[%d] = %s\n", blockIndex, i, virtualDisk[blockIndex].dir.entrylist[i].name) ;
		}
	}
  else printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}

void printFAT() {
	for (int i = 0; i < 20; i++) {
		printf("FAT[%d]: %d\n", i, FAT[i]);
		printBlock(i);
	}
}
