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


diskblock_t  virtualDisk [MAXBLOCKS] ;           // define our in-memory virtual, with MAXBLOCKS blocks
fatentry_t   FAT         [MAXBLOCKS] ;           // define a file allocation table with MAXBLOCKS 16-bit entries
fatentry_t   rootDirIndex            = 0 ;       // rootDir will be set by format
direntry_t * currentDir              = NULL ;
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
 * The format function prepares the virtualdisk for usage,
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

	// Create an empty directory and set it to root
	block = emptyBlock();
	block.dir.isdir = 1;
	block.dir.nextEntry = 1;
	strcpy(block.dir.entrylist->name, "root");
	// write block to virtual disk after the FAT table
	writeblock(&block, 3);

	// Note in FAT table that a file exists in block 3 and save to disk
	FAT[3] = ENDOFCHAIN;
	copyFAT();

	// Set root directory index on disk and current directory index
	rootDirIndex = 3;
	currentDirIndex = 3;
}


/*
 * Helper functions
 */

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

void printBlock ( int blockIndex )
{
   printf ( "virtualdisk[%d] = %s\n", blockIndex, virtualDisk[blockIndex].data ) ;
}