#include <stdint.h>

#pragma pack(push, 1)
struct DirEntry {
	uint8_t DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	uint8_t DIR_CrtTimeTenth;	/* Created time (tenths of second) */
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	uint16_t DIR_FstClusHI;	/* High 2 bytes of the first cluster address */
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint16_t DIR_FstClusLO;	/* Low 2 bytes of the first cluster address */
	uint32_t DIR_FileSize;
};
#pragma pack(pop)
