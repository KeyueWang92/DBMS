#include "projectA.c"
#include "projectB.h"

block* blockPtr = NULL;

void HFL_init() 
{
    fprintf(stderr, "Calling HFL_init\n");
    BM_init();
}


errCode HFL_create_file(char* filename)
{
    fprintf(stderr, "Attempting to create file with name %s\n", filename);
    if (BM_create_file(filename) < 0) return -1;
    else return 0; 
}


fileDesc HFL_open_file(char* filename)
{
    fprintf(stderr, "Opening file with name %s\n", filename);
    return BM_open_file(filename);
}


errCode HFL_close_file(fileDesc fd)
{
    fprintf(stderr, "Attempting to close file %d\n", fd);
    if (BM_close_file(fd) < 0) return -2;
    return 0;
}


recordID HFL_insert_rec(fileDesc fd, record* rec)
{
    fprintf(stderr, "Attempting to insert record into file %d\n", fd);
    //the record will be inserted into an available place
    int blockCount = fileMD[fd].blockCount;
    if (blockCount == 0) { //no block now, alloc a new block for inserting
        if (BM_alloc_block(fd)== 0)
        //after alloc a new block, we can try to insert it again
        return HFL_insert_rec(fd, rec);
    }
    for (int i = 0; i < blockCount; i++) {
        //first search a place that the recCount is less than recNumber and not been disposed before
        if (fileMD[fd].recCount[i] < RECNUMBER && fileMD[fd].bitmap[i] == 1) {
            BM_get_this_block(fd, i, &blockPtr);
            for (int j = 0; j < RECNUMBER; j++) {
                //this is an available place for the record
                if (fileMD[fd].bitmapR[i][j] != 'y') {
                    int replacePoint = j * RECSIZE;
                    char* record = malloc(RECSIZE);
                    memcpy(record, rec, RECSIZE);
                    for (int k = 0; k < RECSIZE; k++) {
                        blockPtr->data[replacePoint] = record[k];
                        replacePoint++;
                    }
                    //after insert the data, update the metadata of that file
                    fileMD[fd].bitmapR[i][j] = 'y';
                    fileMD[fd].recCount[i]++;
                    //since we change the data in the bufferpool, this block is dirty now
                    blockPtr->dirty = 1;
                    return (i * RECNUMBER + j);
                }
            }
        }
    } //each block is already has RECNUMBER records, so need to alloc new block to insert the record
    if (BM_alloc_block(fd) == 0) {
        //after alloc a new block, we can try to insert it again
        HFL_insert_rec(fd, rec);
    } else {
        return -10;
    }
    return -10;
}

errCode HFL_delete_rec(fileDesc fd, recordID rid)
{
    fprintf(stderr, "Attempting to delete record with ID %d from file %d\n", rid, fd);
    int blockID = rid/RECNUMBER;
    if (BM_get_this_block(fd, blockID, &blockPtr) < 0) return -4;
    int rid2 = rid - blockID * RECNUMBER;
    if (fileMD[fd].bitmapR[blockID][rid2] == 'y') {
        //delete the record from the bufferpool
        int deletePoint = rid2 * RECSIZE;
        for (int k = 0; k < RECSIZE; k++) {
            blockPtr->data[deletePoint] = 'X';
            deletePoint++;
        }
        fileMD[fd].bitmapR[blockID][rid2] = 'n';
        fileMD[fd].recCount[blockID]--;
        //since we change the data in the bufferpool, this block is dirty now
        blockPtr->dirty = 1;
        return 0;
    }
    //has been deleted already
    return 0; 
}


errCode HFL_get_first_rec(fileDesc fd, record** rec)
{
    fprintf(stderr, "Attempting to get first record from file %d\n", fd);
    if(HFL_get_this_rec(fd, 0, rec) < 0) {
       return -5; 
    } else return 0;   
}

errCode HFL_get_next_rec(fileDesc fd, record** rec)
{
    fprintf(stderr, "Attempting to get next record from file %d\n", fd);
    int recordID = fileMD[fd].curRecord + 1;
    return HFL_get_this_rec(fd, recordID, rec);
}

errCode HFL_get_this_rec(fileDesc fd, recordID rid, record** rec)
{
    fprintf(stderr, "Attempting to get record %d from file %d\n", rid, fd);
    int blockID = rid / RECNUMBER;
    int rid2 = rid - blockID * RECNUMBER;
    //if there is no record in that position, return err
    if(fileMD[fd].bitmapR[blockID][rid2] == 'n') {
        fileMD[fd].curRecord = rid;
        return -7;
    }
    if(BM_get_this_block(fd, blockID, &blockPtr) < 0) return -7;
    record r;
    record* recPtr;
    recPtr = &r;
    char* buf = malloc(sizeof(char) * FRAMESIZE);
    buf = blockPtr->data;
    r.attribute1 = buf[rid2*RECSIZE];
    r.attribute2 = buf[rid2*RECSIZE + 4];
    r.attribute3 = buf[rid2*RECSIZE + 8];
    r.attribute4 = buf[rid2*RECSIZE + 12];
    r.attribute5 = buf[rid2*RECSIZE + 13];
    r.attribute6 = buf[rid2*RECSIZE + 14];
    r.attribute7 = buf[rid2*RECSIZE + 15];
    rec = &recPtr;
    fileMD[fd].curRecord = rid;
    return 0;
}

scanDesc HFL_open_file_scan(fileDesc fd)
{
    fprintf(stderr, "Opening a file scan for file %d\n", fd);
    for (int i = 0; i < 1000; i++) {
        //open an available scan
        if (filescan[i].opening =='n') {
            filescan[i].opening = 'y';
            filescan[i].fd = fd;
            filescan[i].currentRec = 0;
            return i;
        }     
    }
    return -8;//errorcode, cannot open a new scan
}

errCode HFL_find_next_rec(scanDesc sd, record** rec)
{
    fprintf(stderr, "scanDesc %d is scanning to the next record\n", sd);
    if (sd > 999) return -9;
    recordID rid = filescan[sd].currentRec;
    int fd = filescan[sd].fd;
    if(HFL_get_this_rec(fd, rid, rec) < 0) return -9;
    filescan[sd].currentRec++;
    return 0;
}

errCode HFL_close_file_scan(scanDesc sd)
{
    fprintf(stderr, "Closing file scan %d", sd);
    if (sd > 999) return -11; 
    filescan[sd].opening = 'n';
    return 0;
}

void HFL_print_error(errCode ec)
{
  fprintf(stderr, "Printing error code %d\n", ec);
  if (ec == -1) {
        printf("Error: can't create the file.\n");
    } else if (ec == -2) {
        printf("Error: can't close the file.\n");
    } else if (ec == -3) {
        printf("Error: can't insert the record.\n");
    } else if (ec == -4) {
        printf("Error: can't delete the record.\n");
    } else if (ec == -5) {
        printf("Error: can't get the first record.\n");
    } else if (ec == -6) {
        printf("Error: can't get the next record.\n");
    } else if (ec == -7) {
        printf("Error: can't get this record.\n");
    } else if (ec == -8) {
        printf("Error: can't open a new scan.\n");
    } else if (ec == -9) {
        printf("Error: can't find next record.\n");
    } else if (ec == -10) {
        printf("Error: can't insert the record.\n");
    } else if (ec == -11) {
        printf("Error: can't close the scan.-- \n");
    }
}


