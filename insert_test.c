#include <assert.h>
#include <memory.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
//#include "record.h"
#include "projectB.c"

int main(int argc, char const *argv[])
{
	BM_create_file("mytest.dat");
	fileDesc fd = BM_open_file("mytest.dat");
	BM_alloc_block(fd);
	record* recPtr;
	record rec;
	record rec2;
	recPtr = &rec;
	rec.attribute1 = 1;
	rec.attribute2 = 2;
	rec.attribute3 = 3;
	rec.attribute4 = 'a';
	rec.attribute5 = 'b';
	rec.attribute6 = 'c';
	rec.attribute7 = 'd';
	for (int i = 0; i < 100; i++) {
		printf("insert No.%d record\n", i);
		int errcode = HFL_insert_rec(fd, recPtr);	
		printf("errcode: %d\n", errcode);
	}
	block* blockPtr = NULL;
	BM_get_first_block(fd, &blockPtr);
	printf("%d\n", fileMD[fd].blockCount);
	rec2.attribute1 = blockPtr->data[0];
	rec2.attribute2 = blockPtr->data[4];
	rec2.attribute7 = blockPtr->data[15];
	printf("%d, %d, %c\n", rec2.attribute1, rec2.attribute2, rec2.attribute7);
	HFL_get_first_rec(fd, &recPtr);
	printf("%d, %d, %c\n", recPtr->attribute1, recPtr->attribute2, recPtr->attribute7);
	HFL_get_next_rec(fd, &recPtr);
	printf("%d, %d, %c\n", recPtr->attribute1, recPtr->attribute2, recPtr->attribute7);
	HFL_get_this_rec(fd, 200, &recPtr);
	printf("%d, %d, %c\n", recPtr->attribute1, recPtr->attribute2, recPtr->attribute7);
	HFL_delete_rec(fd, 0);
	printf("%c\n", blockPtr->data[0]);
	int err = HFL_get_first_rec(fd, &recPtr);
	printf("%d\n", err);
	BM_unpin_block(blockPtr);
	BM_close_file(fd);
	return 0;
}
