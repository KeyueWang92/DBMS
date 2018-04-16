#include <assert.h>
#include <memory.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
//#include "record.h"
#include "projectA.c"

int main(int argc, char const *argv[])
{
	BM_create_file("mytest.dat");
	fileDesc fd = BM_open_file("mytest.dat");
	BM_alloc_block(fd);
	record* recPtr;
	record* recPtr2;
	record rec2;
	recPtr2 = &rec2;
	record rec;
	recPtr = &rec;
	rec.attribute1 = 1;
	rec.attribute2 = 2;
	rec.attribute3 = 3;
	rec.attribute4 = 'a';
	rec.attribute5 = 'b';
	rec.attribute6 = 'c';
	rec.attribute7 = 'd';
	//char* string = malloc(sizeof(char)*20);
	char* string2 = malloc(sizeof(char)*16);
	//printf("%d, %d, %d\n", recPtr->attribute1, recPtr->attribute2, recPtr->attribute3);
	memcpy(string2, recPtr, 16);
	pwrite(fd, string2, 16, 0);
	//printf("%s\n", string2);
	//for (int i = 0; i < 16; i++) {
	//	string[i] = string2[i];
	//}
	//printf("%s\n", string);
	pread(fd, recPtr2, 16, 0);
	printf("%d, %d, %d, %c, %c, %c, %c\n", rec2.attribute1, rec2.attribute2, rec2.attribute3, rec2.attribute4,rec2.attribute5,rec2.attribute6,rec2.attribute7);
	return 0;
}