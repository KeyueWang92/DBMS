#include <stdio.h>
#include <stdlib.h>
#include "record.h"
#ifndef PROJECTA_H
#define PROJECTA_H

#define FRAMESIZE 4096
#define BFSIZE 8
#define RECSIZE 16
#define RECNUMBER 256


/*The key for accessing a file's metadata,
 *like its physical location, # blocks, ...*/
typedef int fileDesc;
typedef int errCode; 
typedef struct _block{
    int pinCount;
    int dirty;
    char* data; // void data[FRAMESIZE];
    fileDesc fd;
    int blockID;
 	int empty; 
} block;

typedef struct _metadata{
    int blockCount; //count the number of block that the file contains
    int bitmap[1000];//a bitmap that indicate whether the block has been disposed/empty or not, if yes set it to 0(default), otherwise 1
    int recCount[1000];//this indicate how many records that each block contains
    char bitmapR[1000][256];//a bitmap that indicate whether a slot contains a record or not. 'y' for yes. 'n' for no.
    int curRecord;//show the current recordID that the file is scanned
} metadata;

typedef struct _scanmap {
    fileDesc fd;
    char opening;
    int currentRec;
} scanmap; //struct that track the information of several scans

/*Perform necessary initializations for the buffer layer. 
 *This involves creating a buffer pool with a specified
 *size and frame size, and initializing bookkeeping
 *variables for each frame in the pool.
 */
void BM_init();

/*Create a new DB file on disk.
 *Return 0 if the operation succeeds or an error code if it fails.
 */
errCode BM_create_file( const char *filename );

/*Open an existing file and return a file descriptor for it.
 */
fileDesc BM_open_file( const char *filename ); 

/*Close an opened file and return 0 if the operation succeeds
 *or an error code if it fails. 
 */
errCode BM_close_file( fileDesc fd );

/*Make blockPtr point to the buffer pool location of the file's first block.
 *If the block does not exist in the buffer pool yet, read it in from the file.
 *This might involve replacing a block in the buffer pool if all frames 
 *are full.
 *Return 0 if the operation succeeds or an error code if it fails.
 */
errCode BM_get_first_block( fileDesc fd, block** blockPtr );

/*Make blockPtr point to the buffer pool location of the file's next block.
 *Again, this might involve replacing a block in the buffer pool if all frames
 *are full.
 *Return 0 if the operation succeeds or an error code if it fails.
 */
errCode BM_get_next_block( fileDesc fd, block** blockPtr );

/*Make blockPtr point to the buffer pool location of the block with ID blockID.
 *Again, this might involve replacing a block in the buffer pool if all frames
 *are full. 
 *Return 0 if the operation succeeds or an error code if it fails.
 */
errCode BM_get_this_block( fileDesc, int blockID, block** blockPtr );

/*Add a new block to an open file and save the new block's ID.
 *This means both allocating an extra on-disk block for the file 
 *and updating the metadata for that file. Return 0 if the operation
 *succeeds or an error code if it fails.
 */
errCode BM_alloc_block( fileDesc fd );


/*Remove the block with ID blockID from an open file.
 *This means both freeing disk space and updating the file's metadata.
 *Return 0 if the operation succeeds or an error code if it fails.
 */
errCode BM_dispose_block( fileDesc, int blockID );


/*Mark the block that blockPtr points to as not in use,
 *so that it can be removed when the buffer pool gets full and a block has
 *to be replaced. When a block is unpinned, the buffer manager should write
 *it to disk immediately so that it doesn't have to keep track of which blocks
 *are dirty (this makes bookkeeping simpler). 
 *Return 0 if the operation succeeds or an error code if it fails.
 */
errCode BM_unpin_block( block* blockPtr );

/*Print the error message that corresponds to the error code.
 */
void BM_print_error( errCode ec );

#endif
