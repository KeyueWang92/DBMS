#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <unistd.h>
#include "projectA.h"


block *bufferpool[BFSIZE]; //bufferpool
metadata fileMD[100] = {0, {0}, {0}, {'n'}, 0}; //metadata for files
scanmap filescan[1000] = {{}, {'n'}, {}}; //metadata for scans

void init_block(block** bptr)
{
	*bptr = malloc(sizeof(block));
	(*bptr)->data = malloc(FRAMESIZE);
	(*bptr)->dirty=0;
	(*bptr)->pinCount=0;
	(*bptr)->empty = 0;
}

void populate_block(block* blockPtr);

void BM_init(){
	fprintf(stderr, "Calling BM_init");
	for (int i = 0; i < BFSIZE; i++) {
        bufferpool[i] = NULL;
	}
}

errCode BM_create_file( const char *filename ) {
	fprintf(stderr, "Calling BM_create_file with name: %s\n", filename);
	FILE *fp;
	fp = fopen(filename, "w+");
	if (fp != NULL) {
		fclose(fp);
		return 0;
	} else
	return -1;
}

fileDesc BM_open_file( const char *filename ) {
	fprintf(stderr, "Calling BM_open_file with name: %s\n", filename);
	FILE *fp;
	fp = fopen(filename, "r+");
	if (fp!= NULL) {
		int fd = fileno(fp);
        //cannot store the metadata if the fd is equal or larger than 10000
        if (fd < 100){  
            //get the blocknumber of the file, init the bitmap in the metadata
            int offset = 0;
            for (int i = 0; i < 1000; i++) {
                char buffer[FRAMESIZE];
                int readC = pread(fd, buffer, FRAMESIZE, offset);
                //check if we can read the FRAMESIZE bytes from the file, if yes, set the bitmap 1
                if (readC > 0) {
                    fileMD[fd].blockCount++;
                    if (buffer[0] != ' ') {
                        fileMD[fd].bitmap[i] = 1;
                        //read each record from the file, init the recCount of each block
                        int offset2 = offset;
                        char* buffer2 = malloc(sizeof(char)*RECSIZE);
                        //for each block, get how many records it contains
                        for (int j = 0; j < RECNUMBER; j++) {
                        //read a record to buffer2
                            pread(fd, buffer2, RECSIZE, offset2);
                            // if the first char of the record is not 'X', increase the recCount of that block
                            if (buffer2[0] != 'X') { 
                                fileMD[fd].recCount[i]++;
                                fileMD[fd].bitmapR[i][j] = 'y';
                                offset2 = offset2 + RECSIZE;
                            } else {
                                offset2 = offset2 + RECSIZE;
                            }
                        }
                        offset = offset + FRAMESIZE;
                    }  
                } else break;
            }
            return fd; 
        }   else {
            fprintf(stderr, "cannot open the file with name: %s\n", filename);
            return -2;  
            }  
	} else {
        fprintf(stderr, "cannot open the file with name: %s\n", filename);
        return -2; 
    }
}

errCode BM_close_file( fileDesc fd ) {
	fprintf(stderr, "Attempting to close file: %d\n", fd);
	int fd2 = close(fd);
	if (fd2 == 0) {
        //close the related scan
        for(int i = 0; i < 100; i++) {
            if(filescan[i].fd == fd) {
                filescan[i].opening = 'n';
            }
        }
		return 0;
	} else return -3;
}

errCode BM_get_first_block( fileDesc fd, block** blockPtr ) {
	fprintf(stderr, "Attempting to read first block from file %d\n", fd);
	block* new_pointer = NULL;
	init_block(&new_pointer);
	new_pointer->pinCount++;
    if (fileMD[fd].blockCount == 0) {
    	fprintf(stderr, "These is no block in the file: %d\n", fd);
    	return -4;
    } else {
    	BM_get_this_block( fd, 0, blockPtr );
    }
    return -4;
}


errCode BM_get_next_block( fileDesc fd, block** blockPtr ) {
	fprintf(stderr, "Attempting to get next block from file %d\n", fd);
	block* new_pointer = NULL;
	init_block(&new_pointer);
	new_pointer->pinCount++;	
	int blockID = (*blockPtr)->blockID + 1;
    if (blockID >= fileMD[fd].blockCount) {
        return -5;
    } else {
        BM_get_this_block( fd, blockID, blockPtr );
    }
    return 0;
}

errCode BM_get_this_block( fileDesc fd, int blockID, block** blockPtr ) {
    fprintf(stderr, "Attempting to get block %d from file %d\n", blockID, fd);
    block* new_pointer = NULL;
    init_block(&new_pointer);
    new_pointer->pinCount++;
    if (blockID >= fileMD[fd].blockCount) {
        fprintf(stderr, "fail to get block %d from file %d, this block doesn't exist.\n", blockID, fd);
        return -6;
    } else {
        if (fileMD[fd].bitmap[blockID] == 0) {
            fprintf(stderr, "fail to get block %d from file %d, this block has been disposed.\n", blockID, fd);
            return -6;
        } else {
            //try to find the block of the file fd
            for (int i = 0; i < BFSIZE; i++) {
                if (bufferpool[i]!= NULL && (bufferpool[i]->fd == fd) && (bufferpool[i]->blockID == blockID)) {
                    *new_pointer = *bufferpool[i];
                    new_pointer->fd = fd;
                    new_pointer->blockID = blockID;
                    new_pointer->data = bufferpool[i]->data;
                    new_pointer->empty = 1;
                    *blockPtr = new_pointer;
                    return 0;
                }
            }
            //find an empty block to put the file block into it
            for (int i = 0; i < BFSIZE; i++) {
                if (bufferpool[i] == NULL || bufferpool[i]-> empty == 0) {
                    pread(fd, new_pointer->data, FRAMESIZE, blockID*FRAMESIZE);
                    //update the block info
                    new_pointer->fd = fd;
                    new_pointer->blockID = blockID;
                    new_pointer->empty = 1;
                    *blockPtr = new_pointer;
                    bufferpool[i] = new_pointer;
                    return 0;
                }
            }
            //the block is not in the bufferpool
            //find the first block that is unpinned to replace
            for (int i = 0; i < BFSIZE; i++) {
                if ((bufferpool[i]->pinCount = 0)) {
                    //replace the block, which means write this block back to the disk
                    pread(fd, bufferpool[i]->data, FRAMESIZE, (bufferpool[i]->blockID)*FRAMESIZE);
                    //update the block info
                    new_pointer->fd = fd;
                    new_pointer->blockID = 0;
                    *blockPtr = new_pointer;
                    bufferpool[i] = new_pointer;
                    return 0;
                }
            }
        }
        return -6;
    }
}
errCode BM_alloc_block( fileDesc fd ) {
	fprintf(stderr, "Attempting to allocate block in file %d\n", fd);
    //find a place to allocate a new block
    int position;
    int found = 0;
    int blockCount = fileMD[fd].blockCount;
    if(blockCount > 0) {
        for (int i = 0; i < blockCount; i++) {
        //find a block that can be used, which was disposed before
            if (fileMD[fd].bitmap[i] == 0) {
                fileMD[fd].bitmap[i] = 1; //reuse the block
                fileMD[fd].recCount[i] = 0;
                position = i;
                found = 1;
                break;
            } 
        }
    } else {
        //there is no block before, allocate here
        fileMD[fd].bitmap[0] = 1;
        position = 0;
        fileMD[fd].blockCount++;
        fileMD[fd].recCount[0] = 0;
        found = 1;
    }
    //no block to reuse, allocate a new block after the existed blocks
    if (found == 0) {
        fileMD[fd].bitmap[blockCount] = 1;
        position = blockCount;
        fileMD[fd].blockCount++; //allocate a new block
        fileMD[fd].recCount[blockCount] = 0;
    }
    //can't have more than 1000 blocks in a file
    if (position < 1000) {
        char *buf;
        buf = malloc(FRAMESIZE*sizeof(char));
        if (buf == NULL) {
            fprintf(stderr, "fail to allocate a buf");
            return -7;
        } 
        else {
            for (int i = 0; i < FRAMESIZE; i++) {
                buf[i] = ' ';
            }
        }
        int offset = position * FRAMESIZE;
        int ret = pwrite(fd, buf, FRAMESIZE, offset);
        if(ret == -1) {
            fprintf(stderr, "fail to allocate a block in file %d, the block number now is: %d\n", fd, fileMD[fd].blockCount);
            return -7;
        } else {
            return 0;
        }
    } else {
        fprintf(stderr, "fail to allocate a block in file %d, the file canont has more than 10000 blocks.\n", fd);
        return -7;
    }
}

errCode BM_dispose_block( fileDesc fd, int blockID ) {
	fprintf(stderr, "Attempting to dispose of block %d from file %d\n",blockID, fd);
	char *buf;
    buf = malloc(FRAMESIZE*sizeof(char));
    if (buf == NULL) {
        fprintf(stderr, "fail to allocate a buf for dispose the block");
        return -8;
    } else {
        for (int i = 0; i < FRAMESIZE; i++) {
            buf[i] = ' ';
        }
    }
    if (pwrite(fd, buf, FRAMESIZE, blockID*FRAMESIZE) == FRAMESIZE) {	
    	//update metadata
    	fileMD[fd].bitmap[blockID] = 0;
        fileMD[fd].recCount[blockID] = 0;
		return 0;
	} else return -8;
}

errCode BM_unpin_block( block* blockPtr ) {
	fprintf(stderr, "Unpinning block\n");
    int blockid = blockPtr->blockID;
    int fd = blockPtr->fd;
    //write this block to disk
    if (pwrite(fd, blockPtr->data, FRAMESIZE, blockid*FRAMESIZE) == FRAMESIZE) {
    	blockPtr->pinCount = 0;
    	blockPtr->dirty = 0;
		return 0;
	} else return -9;
}

void BM_print_error( errCode ec ) {
	fprintf(stderr, "Printing error code %d\n", ec);
	if (ec == -1) {
		printf("Error: can't create the file.\n");
	} else if (ec == -2) {
		printf("Error: can't open the file.\n");
	} else if (ec == -3) {
		printf("Error: can't close the file.\n");
	} else if (ec == -4) {
		printf("Error: can't get the first block.\n");
	} else if (ec == -5) {
		printf("Error: can't get the next block.\n");
	} else if (ec == -6) {
		printf("Error: can't get this block.\n");
	} else if (ec == -7) {
		printf("Error: can't allocate a new block.\n");
	} else if (ec == -8) {
		printf("Error: can't dispose the block.\n");
	} else if (ec == -9) {
		printf("Error: can't unpin the block.\n");
	}
}

