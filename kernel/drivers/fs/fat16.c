#include <fat16.h>

#define SECTOR_SIZE 512
#define ROOT_DIR_ENTRY_SIZE 32

struct fat16BPB
{
	uint8_t jmpBoot[3];
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
	uint8_t attrib;
	uint8_t zero;
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
