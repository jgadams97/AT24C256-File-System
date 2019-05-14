//Make sure the data that's written is okay.
//Only write datathat's necessary.
void writeToEEPROMSafe(short addr, char data) {
	while (eepromRead(addr) != data) {
		eepromWrite(addr, data);
	}
}

//Copy data to the EEPROM from a buffer.
void copyToEEPROM(short addr, void *vdata, short size) {
	char *data = (char*)vdata;
	for (short i = 0; i < size; i++) {
		writeToEEPROMSafe(addr + i, *(data + i));
	}
}

//Copy data from the EEPROM into a buffer.
void copyFromEEPROM(short addr, void *vdata, short size) {
	char *data = (char*)vdata;
	for (short i = 0; i < size; i++) {
		*(data + i) = eepromRead(addr + i);
	}
}

//Reformats the EEPROM and clears all data.
void eepromReformat() {
	//Set the file list size to be 0.
	char fileSize = 0;
	copyToEEPROM(FILE_SIZE_ADDR, &fileSize, sizeof(char));
	//Set the journal to be empty.
	short journalSize = 0;
	copyToEEPROM(JOURNAL_SIZE_ADDR, &journalSize, sizeof(short));
	//Set the file data size to be empty.
	short dataSize = 0;
	copyToEEPROM(DATA_SIZE_ADDR, &dataSize, sizeof(short));
}

//Compare file names.
char compareFileNames(char *a, char *b) {
	char i;
	for (i = 0; i < FILE_NAME_SIZE; i++) {
		if (a[i] == 0) {
			if (b[i] == 0)
				return 1;
			else
				return 0;
		}
		if (b[i] == 0) {
			if (a[i] == 0)
				return 1;
			else
				return 0;
		}
		
		if (a[i] != b[i])
			return 0;
	}
	return 1;
}

//Looks for a file within a given directory.
//	Returns the index to its file name block.
char findFileNameBlock(char directory, char *name, char type) {
	FileNameBlock file;
	char addr = 0;
	//Fetch size.
	char size;
	copyFromEEPROM(FILE_SIZE_ADDR, &size, sizeof(char));
	if (size == 0) return undefined;
	//Look for file name.
	do {
		copyFromEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * addr,
			&file, sizeof(FileNameBlock));
		if (file.flag == type) {
			if (compareFileNames(name, file.name) && file.directory == directory) {
				return addr;
			}
		}
		addr++;
	} while (addr != size);
	return undefined;
}

//Checks if a directory exists.
char directoryExists(char directory) {
	
	//If it's at the root, it exists.
	if (directory == (char)undefined) {
		return 1;
	}
	
	//Fetch size.
	char size;
	copyFromEEPROM(FILE_SIZE_ADDR, &size, sizeof(char));
	
	//If the directory is too large or too small, it can't exist.
	if (directory < 0 || directory >= size)
		return 0;
	//Load file name block.
	FileNameBlock file;
	copyFromEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * directory,
		&file, sizeof(FileNameBlock));
	//If the entry is not a folder, it doesn't exist.
	if (file.flag != FILE_FLAG_FOLDER)
		return 0;
	//It exists.
	return 1;
}

//Find the first empty file name block.
char findEmptyFileNameBlock() {
	FileNameBlock file;
	char addr = 0;
	//Fetch size.
	char size;
	copyFromEEPROM(FILE_SIZE_ADDR, &size, sizeof(char));
	//Look for first empty block.
	do {
		copyFromEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * addr,
			&file, sizeof(FileNameBlock));
		if (file.flag == FILE_FLAG_EMPTY)
			return addr;
		addr++;
	} while (addr != size);
	//If we reached the end, that is our empty block assuming
	//	we have not maxed out how many files we can store.
	if (size != MAX_FILES)
		return size;
	else
		return undefined;
}

//Create a new file name block.
//	Returns index of created file.
//	Returns undefined if there is no space left.
//	Does NOT check if the file already exists.
//	Nor does it check if its directory exists.
//	Size is the total size of the data the file contains.
char createFileNameBlock(char directory, char type, short size, char *name) {
	FileNameBlock file;
	//Fetch the index of the first empty file name block.
	char index = findEmptyFileNameBlock();
	//If it failed to find one, there are no spaces left.
	if (index == (char)undefined) {
		return undefined;
	}
	//Load the empty file name block.
	copyFromEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * index,
		&file, sizeof(FileNameBlock));
	//If it's a FINAL entry, make a new FINAL entry.
	if (file.flag == FILE_FLAG_FINAL) {
		//Make sure there is room for a new final entry.
		if (index + 1 != MAX_FILES) {
			//Create our new FINAL entry.
			copyToEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * (index + 1),
				&file, sizeof(FileNameBlock));
		}
	}
	//Fill the name with spaces.
	for (char i = 0; i < FILE_NAME_SIZE; i++)
		file.name[i] = 0;
	//Copy file name to file name block.
	for (char i = 0; i < FILE_NAME_SIZE; i++) {
		if (name[i] == 0)
			break;
		file.name[i] = name[i];
	}
	//Set the flag.
	file.flag = type;
	//Set the directory.
	file.directory = directory;
	//Set size.
	file.size = size;
	copyToEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * index,
		&file, sizeof(FileNameBlock));
	//Increment size.
	char fileSize;
	copyFromEEPROM(FILE_SIZE_ADDR, &fileSize, sizeof(char));
	fileSize++;
	copyToEEPROM(FILE_SIZE_ADDR, &fileSize, sizeof(char));
	return index;
}

//Deletes a file name block at an index.
//	Does NOT check if the file exists.
void deleteFileNameBlock(char index) {
	char size;
	copyFromEEPROM(FILE_SIZE_ADDR, &size, sizeof(char));
	//If the index is too small or too large, just do nothing.
	if (index < 0 || index >= size)
		return;
	//If we are deleting a file name at the end,
	//	then you can simply decrease the size.
	if (index == size - 1) {
		size--;
		copyToEEPROM(FILE_SIZE_ADDR, &size, sizeof(char));
		return;
	}
	//Store an empty file flag.
	FileNameBlock file;
	file.flag = FILE_FLAG_EMPTY;
	copyToEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * index,
		&file, sizeof(FileNameBlock));
}

//Creates a folder.
Folder createFolder(Folder *parent, char *name) {
	Folder myFolder;
	myFolder.status = 0;
	char directory;
	if (parent == NULL)
		directory = undefined;
	else
		directory = parent->index;
	myFolder.directory = directory;
	//Check if the directory exists.
	if (!directoryExists(directory)) {
		myFolder.status = 1;
	}
	//Check if the file doesn't already exist.
	if (findFileNameBlock(directory, name, FILE_FLAG_FOLDER) != (char)undefined) {
		myFolder.status = 2;
		return myFolder;
	}
	//Create new file name block.
	char index = createFileNameBlock(directory, FILE_FLAG_FOLDER, 0, name);
	myFolder.index = index;
	//Copy name to folder.
	for (char i = 0; i < FILE_NAME_SIZE; i++) {
		if (name[i] == 0) {
			myFolder.name[i] = 0;
			break;
		} else {
			myFolder.name[i] = name[i];
		}
	}
	myFolder.name[FILE_NAME_SIZE] = 0;
	myFolder.seek = 0;
	return myFolder;
	
}

//Creates a file and returns it.
//	If it failed to create the file, it will still
//	return a file but the status will not be 0.
//		1	Directory doesn't exist.
//		2	File already exists.
//		3	Max files reached.
File createFile(short size, Folder *folder, char *name) {
	File myFile;
	myFile.address = undefined;
	//Fetch directory index.
	char directory;
	if (folder == NULL)
		directory = undefined;
	else
		directory = folder->index;
	//Check if the directory exists.
	if (!directoryExists(directory)) {
		myFile.status = 1;
		return myFile;
	}
	//Check if the file doesn't already exist.
	if (findFileNameBlock(directory, name, FILE_FLAG_FILE) != (char)undefined) {
		myFile.status = 2;
		return myFile;
	}
	//Create new file name block.
	char index = createFileNameBlock(directory, FILE_FLAG_FILE, size, name);
	myFile.index = index;
	//Copy name to file.
	for (char i = 0; i < FILE_NAME_SIZE; i++) {
		if (name[i] == 0) {
			myFile.name[i] = 0;
			break;
		} else {
			myFile.name[i] = name[i];
		}
	}
	myFile.name[FILE_NAME_SIZE] = 0;
	//If it failed, then we can't store anymore files.
	if (index == (char)undefined) {
		myFile.status = 3;
		return myFile;
	}
	//Get a count of how many data blocks will we need.
	short count = size % DATA_BLOCK_SIZE == 0 ?
		size / DATA_BLOCK_SIZE : size / DATA_BLOCK_SIZE + 1;
	//Offset.
	short offset = size % DATA_BLOCK_SIZE;
	//Load journal size.
	short journalSize;
	copyFromEEPROM(JOURNAL_SIZE_ADDR, &journalSize, sizeof(short));
	short oldJournalSize = journalSize;
	//Address and previous address....
	short addr = undefined;
	short paddr = undefined;
	
	//While the journal size isn't zero...
	while (journalSize > 0 && count > 0) {
		//Pop an address off the journal.
		FileDataBlock data;
		paddr = addr;
		journalSize--;
		copyFromEEPROM(JOURNAL_ADDR + sizeof(short) * journalSize,
			&addr, sizeof(short));
		//If we haven't set the address yet, this is the address.
		if (myFile.address == (short)undefined) {
			myFile.address = addr;
		} else {
			//Link previous data block to this one.
			copyFromEEPROM(DATA_ADDR + paddr, &data, sizeof(short) * 2);
			if (data.next != addr || data.size != DATA_BLOCK_SIZE) {
				data.next = addr;
				data.size = DATA_BLOCK_SIZE;
				copyToEEPROM(DATA_ADDR + paddr, &data, sizeof(short) * 2);
			}
		}

		//Loop until this journal entry does not have a next data.
		do {
			//Load the data block.
			copyFromEEPROM(DATA_ADDR + addr, &data, sizeof(short) * 2);
			//Decrement the count.
			count--;
			//Go to the next data block.
			paddr = addr;
			addr = data.next;
		} while (addr != (short)undefined && count > 0);
		
		
		//If we still have data left but our count is empty,
		//	we need to tell the journal there's still empty
		//	space left.
		if (addr != (short)undefined) {
			//Push next item onto journal.
			copyToEEPROM(JOURNAL_ADDR + sizeof(short) * journalSize,
				&addr, sizeof(short));
			journalSize++;
		}
		
		//If we've reached the last item, make the end undefined.
		if (count == 0) {
			//Make the last file data block empty.
			data.next = undefined;
			data.size = offset;
			if (paddr != (short)undefined) {
				copyToEEPROM(DATA_ADDR + paddr, &data, sizeof(short) * 2);
			} else {
				copyToEEPROM(DATA_ADDR + myFile.address, &data, sizeof(short) * 2);
			}
		}
		
		//If we did reach the end, update addr.
		if (addr == (short)undefined) {
			addr = paddr;
		}
	}
	//Save new journal size.
	if (journalSize != oldJournalSize)
		copyToEEPROM(JOURNAL_SIZE_ADDR, &journalSize, sizeof(short));
	
	//Load data size.
	short dataSize;
	copyFromEEPROM(DATA_SIZE_ADDR, &dataSize, sizeof(short));
	short oldDataSize = dataSize;
	//If we still have count left even after using
	//	the whole journal...
	while (count > 0) {
		FileDataBlock data;
		paddr = addr;
		addr = dataSize;
		dataSize += sizeof(FileDataBlock);
		//If we haven't set the address yet, this is the address.
		if (myFile.address == (short)undefined) {
			myFile.address = addr;
		} else {
			//Link previous data block to this one.
			copyFromEEPROM(DATA_ADDR + paddr, &data, sizeof(short) * 2);
			if (data.next != addr || data.size != DATA_BLOCK_SIZE) {
				data.next = addr;
				data.size = DATA_BLOCK_SIZE;
				copyToEEPROM(DATA_ADDR + paddr, &data, sizeof(short) * 2);
			}
		}
		count--;
		
		//If we've reached the last item, make the end undefined.
		if (count == 0) {
			//Make the last file data block empty.
			if (data.next != (short)undefined || data.size != offset) {
				data.next = undefined;
				data.size = offset;
				copyToEEPROM(DATA_ADDR + addr, &data, sizeof(short) * 2);
			}
		}
	}
	//Save new size.
	if (dataSize != oldDataSize)
		copyToEEPROM(DATA_SIZE_ADDR, &dataSize, sizeof(short));
	
	myFile.status = 0;
	myFile.size = size;
	myFile.seekAddr = myFile.address;
	myFile.seekPos = 0;
	myFile.directory = directory;
	
	//Insert the address into the file name block.
	FileNameBlock fnb;
	copyFromEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * index,
		&fnb, sizeof(FileNameBlock));
	fnb.address = myFile.address;
	copyToEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * index,
		&fnb, sizeof(FileNameBlock));
	
	return myFile;
}

//Deletes a file.
void deleteFile(File *myFile) {
	//Delete the file name block.
	deleteFileNameBlock(myFile->index);
	//Append its address to the journal.
	if (myFile->size > 0) {
		short journalSize;
		copyFromEEPROM(JOURNAL_SIZE_ADDR, &journalSize, sizeof(short));
		copyToEEPROM(JOURNAL_ADDR + sizeof(short) * journalSize,
			&myFile->address, sizeof(short));
		journalSize++;
		copyToEEPROM(JOURNAL_SIZE_ADDR, &journalSize, sizeof(short));
		myFile->size = 0;
	}
}

//Deletes a folder.
void deleteFolder(Folder *myFolder) {
	//Delete the folder name block.
	deleteFileNameBlock(myFolder->index);
}

//Resizes a file to a new size.
void resizeFile(File *myFile, short newSize) {
	//Deleting the file and recreating it will resize it.
	//	This is because the journal is a stack, so the
	//	free space created from the most recently deleted
	//	file is the first space to be filled in by a new
	//	file. This means that the file will preserve its
	//	data if deleted and immediately recreated.
	deleteFile(myFile);
	Folder myFolder;
	myFolder.directory = myFile->directory;
	File newFile = createFile(newSize, &myFolder, myFile->name);
	char *myFilePtr = (char*)myFile;
	char *newFilePtr = (char*)&newFile;
	for (char i = 0; i < sizeof(File); i++) {
		*myFilePtr = *newFilePtr;
	}
}

void printFileData(File f) {
	short count = f.size % DATA_BLOCK_SIZE == 0 ?
		f.size / DATA_BLOCK_SIZE : f.size / DATA_BLOCK_SIZE + 1;
	if (count == 0) {
		return;
	}
	
	short addr = f.address;
	int i = 0;
	do {
		FileDataBlock data;
		copyFromEEPROM(DATA_ADDR + addr, &data, sizeof(FileDataBlock));
		addr = data.next;
	} while (addr != (short)undefined && i < 30);
}

//List files in a folder.
//	Returns 0 when reaches the end.
//	Stores name in "name".
char listFilesInFolder(Folder *folder, char *name) {
	char directory;
	if (folder == NULL)
		directory = undefined;
	else
		directory = folder->index;
	FileNameBlock file;
	char addr;
	if (folder == NULL)
		addr = FILE_ROOT_SEEK;
	else
		addr = folder->seek;
	//Fetch size.
	char size;
	copyFromEEPROM(FILE_SIZE_ADDR, &size, sizeof(char));
	//If we've reached the end, no more files.
	if (folder == NULL) {
		if (FILE_ROOT_SEEK >= size) {
			FILE_ROOT_SEEK = 0;
			return 0;
		}
	} else {
		if (folder->seek >= size) {
			folder->seek = 0;
			return 0;
		}
	}
	//Look for file name.
	do {
		copyFromEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * addr,
			&file, sizeof(FileNameBlock));
		if (file.flag != FILE_FLAG_EMPTY) {
			if (file.directory == directory) {
				//Update seek.
				if (folder == NULL)
					FILE_ROOT_SEEK = addr + 1;
				else
					folder->seek = addr + 1;
				//Load name.
				char namePos = 0;
				if (file.flag == FILE_FLAG_FOLDER)
					name[namePos++] = '/';
				for (char i = 0; i < FILE_NAME_SIZE; i++) {
					if (file.name[i] == 0) break;
					name[namePos++] = file.name[i];
				}
				if (file.flag == FILE_FLAG_FOLDER)
					name[namePos++] = '/';
				name[namePos] = 0;
				return 1;
			}
		}
		addr++;
	} while (addr != size);
	if (folder == NULL)
		FILE_ROOT_SEEK = 0;
	else
		folder->seek = 0;
	return 0;
}

//Opens a folder.
Folder openFolder(Folder *parent, char *name) {
	char directory;
	if (parent == NULL)
		directory = undefined;
	else
		directory = parent->index;
	Folder myFolder;
	myFolder.status = 0;
	myFolder.directory = directory;
	myFolder.index = findFileNameBlock(directory, name, FILE_FLAG_FOLDER);
	if (myFolder.index == (char)undefined) {
		myFolder.status = 1;
		return myFolder;
	}
	for (char i = 0; i < FILE_NAME_SIZE; i++)
		myFolder.name[i] = name[i];
	myFolder.name[FILE_NAME_SIZE] = 0;
	myFolder.seek = 0;
	return myFolder;
}

//Opens a file.
File openFile(Folder *parent, char *name) {
	char directory;
	if (parent == NULL)
		directory = undefined;
	else
		directory = parent->index;
	File myFile;
	myFile.status = 0;
	myFile.directory = directory;
	myFile.index = findFileNameBlock(directory, name, FILE_FLAG_FILE);
	if (myFile.index == (char)undefined) {
		myFile.status = 1;
		return myFile;
	}
	FileNameBlock fnb;
	copyFromEEPROM(FILE_NAME_ADDR + sizeof(FileNameBlock) * myFile.index,
		&fnb, sizeof(FileNameBlock));
	
	for (char i = 0; i < FILE_NAME_SIZE; i++)
		myFile.name[i] = name[i];
	myFile.name[FILE_NAME_SIZE] = 0;
	
	myFile.size = fnb.size;
	myFile.address = fnb.address;
	myFile.seekPos = 0;
	myFile.seekAddr = myFile.address;
	
	return myFile;
}

//Reads a byte from a file.
char readFile(File *file) {
	char ret = 0;
	
	//Load the file data block.
	FileDataBlock data;
	copyFromEEPROM(DATA_ADDR + file->seekAddr, &data, sizeof(FileDataBlock));
	
	//Return nothing if we're at the end.
	if (data.next == (short)undefined) {
		if (file->seekPos == data.size) {
			return 0;
		}
	}
	
	//Load the character.
	copyFromEEPROM(DATA_ADDR + file->seekAddr + file->seekPos + sizeof(short) * 2, &ret, 1);
	
	//Increase the seek pointer.
	if (file->seekPos < DATA_BLOCK_SIZE)
		file->seekPos++;
	
	//If we've left the block, look for the next block.
	if (file->seekPos == DATA_BLOCK_SIZE) {
		if (file->seekAddr != (short)undefined) {
			file->seekPos = 0;
			file->seekAddr = data.next;
		}
	}
	
	return ret;
}

//Changes the seek pointer of a file.
void seekFile(File *file, short pos) {
	file->seekPos = 0;
	file->seekAddr = file->address;
	for (short i = 0; i < pos; i++)
		readFile(file);
}

//Reads a byte from a file.
void writeFile(File *file, char c) {
	//Load the file data block.
	FileDataBlock data;
	copyFromEEPROM(DATA_ADDR + file->seekAddr, &data, sizeof(short) * 2);
	
	//Return if we're at the end.
	if (data.next == (short)undefined) {
		if (file->seekPos == DATA_BLOCK_SIZE) {
			return;
		}
	}
	
	//Store the character.
	copyToEEPROM(DATA_ADDR + file->seekAddr + sizeof(short) * 2 + file->seekPos, &c, 1);
	
	//Increase the seek pointer.
	if (file->seekPos < DATA_BLOCK_SIZE)
		file->seekPos++;
	
	//If we've left the block, look for the next block.
	if (file->seekPos == DATA_BLOCK_SIZE) {
		if (file->seekAddr != (short)undefined) {
			file->seekPos = 0;
			file->seekAddr = data.next;
		}
	}
}
