#ifndef FILE_H
#define FILE_H
#include <stdint.h>

struct FILEINFO {
	uint8_t name[8], ext[3], type;
	int8_t reserve[10];
	uint16_t time, date, clustno;
	uint32_t size;
};


void init_filesystem();
void file_readfat(uint32_t *fat, int64_t img);
void file_loadfile(int32_t clustno, int32_t size, uint8_t *buf, uint32_t *fat, int64_t img);
struct FILEINFO *file_search(int8_t *name, struct FILEINFO *finfo, int32_t max);

#endif // FILE_h
