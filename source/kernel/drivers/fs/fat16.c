#include <fat16.h>
#include <kmalloc.h>

#define SECTOR_SIZE 512
#define ROOT_DIR_ENTRY_SIZE 32

struct fat16BPB
{
	byte jmpBoot[3];
	char oemName[8];
	uint16_t bytesPerSector;
	uint8_t sectorsPerCluster;
	uint16_t reservedSectorCount;
	uint8_t numberOfFATS;
	uint16_t rootEntryCount;
	uint16_t totalSectors16;
	uint8_t mediaType;
	uint16_t fatSizeSectors;
	uint16_t sectorsPerTrack;
	uint16_t numberOfHeads;
	uint32_t hiddenSectorCount;
	uint32_t totalSectors32;
	uint8_t driveNumber;
	uint8_t reserved;
	uint8_t extendedBootSignature;
	uint32_t serialNumber;
	char volumeLabel[11];
	char filesystemType[8];
} __attribute__ ((__packed__));

struct fat16RootDirEntry
{
	char fileName[8];
	char fileExt[3];
	byte attrib;
	byte zero;
	uint8_t creationTimeTenthSecond;
	uint16_t creationTime;
	uint16_t creationDate;
	uint16_t lastAccessDate;
	uint16_t firstClusterHigh;
	uint16_t lastWriteTime;
	uint16_t lastWriteDate;
	uint16_t firstClusterLow;
	uint32_t size;
} __attribute__ ((__packed__));

/*
Reads a file from a FAT16 filesystem into a specified buffer.
The storage device will be determined from the file path.
*/
bool fat16_read_file(char* filePath, void* buffer)
{
	return FALSE;
}

/*
Reads sector(s) from a FAT16 filesystem.
*/
bool fat16_read_sectors()
{
	return FALSE;
}
