# AT24C256 File System
A journaling file system for the AT24C256 I2C EEPROM. 

In order to use it, you must reformat the EEPROM once.

```
#include "file.h"

void setup() {
  Serial.begin(9600);
  //Replace 0 with the address of the EEPROM.
  //Replace 2 with the digital pin connected to SDA.
  //Replace 3 with the digital pin connected to SCL.
  if (!eepromSetup(0, 2, 3)) {
    Serial.println("Failed to connect to the EEPROM.");
    return;
  }
  
  eepromReformat();
}
```

After running this once, you can read and write files and folders to it. Here are the methods regarding file creation.

```
File createFile(short size, Folder *folder, char *name);
void deleteFile(File *myFile);
void resizeFile(File *myFile, short newSize);
File openFile(Folder *parent, char *name);
char readFile(File *file);
void seekFile(File *file, short pos);
void writeFile(File *file, char c);
```

Notice how some of the functions take a "Folder" parameter. This allows you to specify which folder you want your file to exist within. If you want your file to be at the root of the file system, simply use NULL.

For the functions that return a File type, you can check a file's ".status" to see if any errors occurred when reading or writing the file, such as attempting to create a file that already exists. 

When you read and write bytes to files, it automatically increments the seek pointer.

```
//Writing to a file.
File myFile = createFile(100, NULL, "test");
char data[] = "Hello, World!";
seekFile(&myFile, 0);
for (int i = 0; i < strlen(data); i++) {
  writeFile(&myFile, data[i]);
}

//Reading from a file.
File myFile = openFile(NULL, "test");
char data[myFile.size];
seekFile(&myFile, 0);
for (int i = 0; i < myFile.size; i++) {
  data[i] = readFile(&myFile);
}
```

Files do not need to be closed. "File" is not an object but a struct. No memory leaks can occur with this design. You also do not need to worry about "committing" your writes to files. Whenever you call "writeFile", that byte is immediately written to the EEPROM. 

Note that due to EEPROMs having limited writes, deleting files will not clear the data within the file, nor will resizing the file or creating a new file. This means that if you create a new file, the file's contents will be whatever contents that just so happened to exist at the memory location it was assigned. If you want to clear the file after creating it, you'll want to do that manually by writing 0x00 characters to the file once it's created. 

Here are the functions for handling folders.

```
Folder createFolder(Folder *parent, char *name);
void deleteFolder(Folder *myFolder);
Folder openFolder(Folder *parent, char *name);
char listFilesInFolder(Folder *folder, char *name);
```

Folders can contain files. They cannot be written to.

The "listFilesInFolder" function can be used to list all files contained within a folder. The "name" parameter is the memory location to store the name of the file.

```
Folder myFolder = openFolder(NULL, "myFolder");

char name[100];
while (listFilesInFolder(&myFolder, name)) {
  Serial.println(name);
}
```

Note that this will also list folders as well and folder names will have a "/" character appended to the beginning and end of the name. 

Can this support AT24Cx chips of different sizes? Probably. I don't know because I do not own other sizes. 16-bit shorts are used to store memory addresses, so if the size is significantly larger you may need to change the data types for it to work. Changing shorts to longs will cause the size of the journal to increase as well.
