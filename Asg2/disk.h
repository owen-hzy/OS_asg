#include <stdint.h>

#pragma pack(push, 1)
struct BootEntry {
	uint8_t BS_jmpBoot[3];	/* Assembly instruction to jump to boot code */
	uint8_t BS_OEMName[8];	/* OEM Name in ASCII */
	uint16_t BPB_BytesPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;	/* Size in sectors of the reserved area */
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;	/* Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32 */
	uint16_t BPB_TotSec16;	/* 16-bit value of number of sectors in file system */
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;	/* Number of sectors before the start of partition */
	uint32_t BPB_TotSec32;
	uint32_t BPB_FATSz32;
	uint16_t BPB_ExtFlags;
	uint16_t BPB_FSVer;
	uint32_t BPB_RootClus;
	uint16_t BPB_FSInfo;
	uint16_t BPB_BkBootSec;
	uint8_t BPB_Reserved[12];
	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint32_t BS_VolID;
	uint8_t BS_VolLab[11];
	uint8_t BS_FilSysType[8];
};
#pragma pack(pop)
