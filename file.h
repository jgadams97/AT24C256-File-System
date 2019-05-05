/*
	A very simple file system for EEPROMs.
	
	File names are stored linearly, meaning finding a file
	takes O(n) time where n is the max number of files the
	device can hold.
	
	File data is stored in a journaling file system. This
	allows for files to be of arbitrary size and make use
	of the entire memory device.
	
	File data blocks are connected through a linked list.
	This allows for quick retrieval of all the data of a
	file once its file name is found.
	
	File name blocks consist of...
		1. 	The files name as a string of ASCII characters.
			All file names are the same length. If the file
			name does not fill the entire length, spaces
			are appended onto the end.
		2.	A file flag. This specifies whether the file is
			indeed a file, a folder, an empty entry, or the
			final entry in the file list.
		3.	A directory. This is the index of the folder
			that the file belongs to.
		4.	A size telling you the number of bytes in the
			file.
		5.	An address. If the file is indeed a file, this
			will be the address of its first data block.
	
	File data blocks consist of...
		1.	A count. This is the number of data blocks
			associated with this file.
		2.	A size. This is how much of the data block is
			actually filled up by the file data.
		3.	A "next" pointer which points to the next data
			block.
		4.	The data itself.
	
	The journal is broken up into two parts:
		1. 	The journal size, which is simply a single
			value representing the current size of the
			journal.
		2.	The journal itself, which is a list of pointers
			to empty data blocks.
			
	
		
*/

#ifndef LIB_EEPROM_FILE
#define LIB_EEPROM_FILE

#define undefined -1
#define MAX_FILES 64
#define FILE_NAME_SIZE 8
#define DATA_BLOCK_SIZE 64
#define FILE_FLAG_FILE 0
#define FILE_FLAG_FOLDER 1
#define FILE_FLAG_EMPTY 2
#define FILE_FLAG_FINAL 3

typedef struct {
	char name[FILE_NAME_SIZE];
	char flag;
	char directory;
	short size;
	short address;
} FileNameBlock;

typedef struct {
	short next;
	short size;
	char data[DATA_BLOCK_SIZE];
} FileDataBlock;

typedef struct {
	short size;
	short address;
	short seekPos;
	short seekAddr;
	char status;
	char index;
	char directory;
	char name[FILE_NAME_SIZE + 1];
} File;

typedef struct {
	char status;
	char index;
	char directory;
	char name[FILE_NAME_SIZE + 1];
	char seek;
} Folder;

#define FILE_SIZE_ADDR 0
#define FILE_NAME_ADDR sizeof(char)
#define JOURNAL_SIZE_ADDR MAX_FILES * sizeof(FileNameBlock)
#define JOURNAL_ADDR JOURNAL_SIZE_ADDR + sizeof(short)
#define DATA_SIZE_ADDR JOURNAL_ADDR + sizeof(short) * MAX_FILES
#define DATA_ADDR DATA_SIZE_ADDR + sizeof(short)
char FILE_ROOT_SEEK = 0;
#include "AT24C256.h"

void copyToEEPROM(short addr, void *vdata, short size);
void copyFromEEPROM(short addr, void *vdata, short size);
void eepromReformat();
char compareFileNames(char *a, char *b);
char findFileNameBlock(char directory, char *name, char type);
char directoryExists(char directory);
char findEmptyFileNameBlock();
char createFileNameBlock(char directory, char type, short size, char *name);
void deleteFileNameBlock(char index);
Folder createFolder(Folder *parent, char *name);
File createFile(short size, Folder *folder, char *name);
void deleteFile(File *myFile);
void deleteFolder(Folder *myFolder);
void resizeFile(File *myFile, short newSize);
void printFileData(File f);
char listFilesInFolder(Folder *folder, char *name);
Folder openFolder(Folder *parent, char *name);
File openFile(Folder *parent, char *name);
char readFile(File *file);
void seekFile(File *file, short pos);
void writeFile(File *file, char c);

#include "file.c"

#endif
