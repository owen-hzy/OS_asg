#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "directory.h"

void PrintFileSystemInfo(char *device);
void PrintRootDirectory(char *device);
void Recover(char *device, char *recover_file, char *output_file);
void Cleanse(char *device, char *delete_file);

int main(int argc, char* argv[]) {
	// Milestone 1 - Detect valid arguments
	int input_error = 0;
	
	if (argc < 4) {
		input_error = 1;
	} else if (strcmp(argv[1], "-d") != 0) {
		input_error = 1;	
	} else {
		if (strcmp(argv[3], "-i") == 0 && argc == 4) {
			PrintFileSystemInfo(argv[2]);
		} else if (strcmp(argv[3], "-l") == 0 && argc == 4) {
			PrintRootDirectory(argv[2]);
		} else if (strcmp(argv[3], "-r") == 0 && argc == 7 && strcmp(argv[5], "-o") == 0) {
			Recover(argv[2], argv[4], argv[6]);
		} else if (strcmp(argv[3], "-x") == 0 && argc == 5) {
			Cleanse(argv[2], argv[4]);
		} else {
			input_error = 1;
		}
	}

	if (input_error == 1) {
		printf("Usage: %s -d [device filename] [other arguments]\n", argv[0]);
		printf("-i			Print file system information\n-l			List the root directory\n-r target -o dest	Recover the target deleted file\n-x target		Cleanse the target deleted file\n");
		exit(-1);
	}
	return 0;
}

// Milestone 2 - Print file system information
void PrintFileSystemInfo(char *device) {
	FILE *fp = fopen(device, "r");
	struct BootEntry boot_entry;
	fread(&boot_entry, 1, sizeof(struct BootEntry), fp);
	printf("Number of FATs = %u\n", boot_entry.BPB_NumFATs);
	printf("Number of bytes per sector = %u\n", boot_entry.BPB_BytesPerSec);
	printf("Number of sectors per cluster = %u\n", boot_entry.BPB_SecPerClus);
	printf("Number of reserved sectors = %u\n", boot_entry.BPB_RsvdSecCnt);
	printf("First FAT starts at byte = %u\n", boot_entry.BPB_RsvdSecCnt * boot_entry.BPB_BytesPerSec);
	printf("Data area starts at byte = %u\n", (boot_entry.BPB_RsvdSecCnt + boot_entry.BPB_FATSz32 * boot_entry.BPB_NumFATs) * boot_entry.BPB_BytesPerSec);
	fclose(fp);
}

// Milestone 3 - List directory content
void PrintRootDirectory(char *device) {
	FILE *fp = fopen(device, "r");
	struct BootEntry boot_entry;
	fread(&boot_entry, 1, sizeof(struct BootEntry), fp);
	uint32_t rootDirectory_offset = (boot_entry.BPB_RsvdSecCnt + boot_entry.BPB_FATSz32 * boot_entry.BPB_NumFATs) * boot_entry.BPB_BytesPerSec;
	uint32_t fat_offset = boot_entry.BPB_RsvdSecCnt * boot_entry.BPB_BytesPerSec;

	uint32_t cluster_id = boot_entry.BPB_RootClus;
	uint32_t i = 1;
	uint32_t iteration_times = 1;
	do { 
		// iterate the root directory
		fseek(fp, (long) (rootDirectory_offset + (cluster_id - 2) * boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec), SEEK_SET);
		for (; i <= (iteration_times * (boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec) / sizeof(struct DirEntry)); i++) {
			struct DirEntry dir_entry;
			fread(&dir_entry, 1, sizeof(struct DirEntry), fp);
			
			// Handle filename
			char filename[13];
			if (dir_entry.DIR_Name[0] == 0x00) {
				break;
			}
			if (dir_entry.DIR_Attr == 0x0F) {
				memcpy(filename, "LFN entry", 10);
				printf("%d, %s\n", i, filename);
				continue;
			} else {
				if (dir_entry.DIR_Name[0] == 0xE5) {
					dir_entry.DIR_Name[0] = 0x3F; //change to '?'
				}
				int j = 0;
				for (j = 0; j < 8; j++) {
					if (dir_entry.DIR_Name[j] == 0x20) {
						break;
					}
					filename[j] = dir_entry.DIR_Name[j];
				}
				if (dir_entry.DIR_Name[8] != 0x20) {
					filename[j++] = '.';
					int k = 8;
					for (k = 8; k < 11; k++) {
						if (dir_entry.DIR_Name[k] == 0x20) {
							break;
						}
						filename[j++] = dir_entry.DIR_Name[k];
					}
				}
				if ((dir_entry.DIR_Attr & 0x10) != 0x00) { // directory
					filename[j++] = '/';
				}
				filename[j] = '\0';
			}
			
			printf("%d, %s, %u, %u\n", i, filename, dir_entry.DIR_FileSize, ((uint32_t) dir_entry.DIR_FstClusHI << 8) | dir_entry.DIR_FstClusLO);
		}
		// find next fat
		fseek(fp, (long) fat_offset + 4 * cluster_id, SEEK_SET);
		fread(&cluster_id, 1, sizeof(int), fp);
		iteration_times++;
	} while (cluster_id < 0x0FFFFFF8);
	fclose(fp);
}

// Milestone 4 - Recover one-cluster-sized files only
void Recover(char *device, char *recover_file, char *output_file) {
	FILE *fp = fopen(device, "r");
	struct BootEntry boot_entry;
	fread(&boot_entry, 1, sizeof(struct BootEntry), fp);
	uint32_t rootDirectory_offset = (boot_entry.BPB_RsvdSecCnt + boot_entry.BPB_FATSz32 * boot_entry.BPB_NumFATs) * boot_entry.BPB_BytesPerSec;
	uint32_t fat_offset = boot_entry.BPB_RsvdSecCnt * boot_entry.BPB_BytesPerSec;
	uint32_t cluster_id = boot_entry.BPB_RootClus;
	char file_to_be_split[13]; // prevent strtok to split original string
	strcpy(file_to_be_split, recover_file);
	char *name = strtok(file_to_be_split, ".");
	char *ext = strtok(NULL, ".");

	uint8_t found = 0;
	do {
		fseek(fp, (long) (rootDirectory_offset + (cluster_id - 2) * boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec), SEEK_SET);

		uint32_t i;
		for (i = 0; i < (boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec) / sizeof(struct DirEntry); i++) {
			struct DirEntry dir_entry;
			fread(&dir_entry, 1, sizeof(struct DirEntry), fp);

			if (dir_entry.DIR_Name[0] == 0xE5 && dir_entry.DIR_Attr != 0x0F) {
				uint8_t j = 0;
				for (j = 1; j < strlen(name); j++) {
					if (dir_entry.DIR_Name[j] != ((uint8_t) name[j])) {
						break;
					}
				}
				if (j == strlen(name) && ext == NULL) {
					if ((j < 8 && dir_entry.DIR_Name[j] != 0x20) || dir_entry.DIR_Name[8] != 0x20) {
						continue;
					}
					found = 1;
				} else if (j == strlen(name) && ext != NULL) {
					uint8_t k = 0;
					for (k = 0; k < strlen(ext); k++) {
						if (dir_entry.DIR_Name[8 + k] != ((uint8_t) ext[k])) {
							break;
						}
					}
					if (k == strlen(ext)) {
						found = 1;
					}
				}
			}

			if (found == 1) {
				uint32_t fat = 0;
				fseek(fp, (long) fat_offset + 4 * (((uint32_t) dir_entry.DIR_FstClusHI << 8) | dir_entry.DIR_FstClusLO), SEEK_SET);
				fread(&fat, 1, sizeof(int), fp);
				// check if occupied or not
				if (fat == 0) {
					FILE *of = fopen(output_file, "w");
					if (of != NULL) {
						fseek(fp, (long) (rootDirectory_offset + ((((uint32_t) dir_entry.DIR_FstClusHI << 8) | dir_entry.DIR_FstClusLO) - 2) * boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec), SEEK_SET);
						char buffer;
						uint32_t size = dir_entry.DIR_FileSize > ((uint32_t) boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec) ? ((uint32_t) boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec) : dir_entry.DIR_FileSize;
						uint32_t p = 0;
						for (p = 0; p < size; p++) {
							fread(&buffer, 1, 1, fp);
							fwrite(&buffer, 1, 1, of);
						}
						fclose(of);

						printf("%s: recovered\n", recover_file);
					} else {
						printf("%s: failed to open\n", output_file);
					}
				} else {
					printf("%s: error - fail to recover\n", recover_file);
				}
				break;
			}
		}
		// find next fat
		fseek(fp, (long) fat_offset + 4 * cluster_id, SEEK_SET);
		fread(&cluster_id, 1, sizeof(int), fp);
	} while (cluster_id < 0x0FFFFFF8 && found == 0);
	fclose(fp);

	if (found == 0) {
		printf("%s: error - file not found\n", recover_file);
		exit(-1);
	}
}

// Milestone 5 - Cleanse the deleted file
void Cleanse(char *device, char *delete_file) {
	FILE *fp = fopen(device, "r+");
	struct BootEntry boot_entry;
	fread(&boot_entry, 1, sizeof(struct BootEntry), fp);
	uint32_t rootDirectory_offset = (boot_entry.BPB_RsvdSecCnt + boot_entry.BPB_FATSz32 * boot_entry.BPB_NumFATs) * boot_entry.BPB_BytesPerSec;
	uint32_t fat_offset = boot_entry.BPB_RsvdSecCnt * boot_entry.BPB_BytesPerSec;
	uint32_t cluster_id = boot_entry.BPB_RootClus;
	char file_to_be_split[13]; // prevent strtok to split original string
	strcpy(file_to_be_split, delete_file);
	char *name = strtok(file_to_be_split, ".");
	char *ext = strtok(NULL, ".");

	uint8_t found = 0;
	do {
		fseek(fp, (long) (rootDirectory_offset + (cluster_id - 2) * boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec), SEEK_SET);

		uint32_t i;
		for (i = 0; i < (boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec) / sizeof(struct DirEntry); i++) {
			struct DirEntry dir_entry;
			fread(&dir_entry, 1, sizeof(struct DirEntry), fp);

			if (dir_entry.DIR_Name[0] == 0xE5 && dir_entry.DIR_Attr != 0x0F) {
				uint8_t j = 0;
				for (j = 1; j < strlen(name); j++) {
					if (dir_entry.DIR_Name[j] != ((uint8_t) name[j])) {
						break;
					}
				}
				if (j == strlen(name) && ext == NULL) {
					if ((j < 8 && dir_entry.DIR_Name[j] != 0x20) || dir_entry.DIR_Name[8] != 0x20) {
						continue;
					}
					found = 1;
				} else if (j == strlen(name) && ext != NULL) {
					uint8_t k = 0;
					for (k = 0; k < strlen(ext); k++) {
						if (dir_entry.DIR_Name[8 + k] != ((uint8_t) ext[k])) {
							break;
						}
					}
					if (k == strlen(ext)) {
						found = 1;
					}
				}
			}

			if (found == 1) {
				uint8_t cleanse_eligible = 1;
				if (dir_entry.DIR_FileSize != 0) {
					uint32_t fat = 0;
					fseek(fp, (long) fat_offset + 4 * (((uint32_t) dir_entry.DIR_FstClusHI << 8) | dir_entry.DIR_FstClusLO), SEEK_SET);
					fread(&fat, 1, sizeof(int), fp);
					// check if occupied or not
					if (fat == 0) {
						fseek(fp, (long) (rootDirectory_offset + ((((uint32_t) dir_entry.DIR_FstClusHI << 8) | dir_entry.DIR_FstClusLO) - 2) * boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec), SEEK_SET);
						uint32_t size = boot_entry.BPB_SecPerClus * boot_entry.BPB_BytesPerSec;
						uint32_t p = 0;
						for (p = 0; p < size; p++) {
							fwrite("0", 1, 1, fp);
						}
						printf("%s: cleansed\n", delete_file);
					} else {
						cleanse_eligible = 0;
					}
				} else {
					cleanse_eligible = 0;
				}

				if (cleanse_eligible == 0) {
					printf("%s: fail to cleanse\n", delete_file);
				}
				break;
			}
		}
		// find next fat
		fseek(fp, (long) fat_offset + 4 * cluster_id, SEEK_SET);
		fread(&cluster_id, 1, sizeof(int), fp);
	} while (cluster_id < 0x0FFFFFF8 && found == 0);
	fclose(fp);

	if (found == 0) {
		printf("%s: error - file not found\n", delete_file);
	}
}

