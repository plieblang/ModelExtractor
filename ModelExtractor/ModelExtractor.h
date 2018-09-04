#pragma once

#include <Windows.h>
#include <stdio.h>
#include <malloc.h>
#include "VPStructs.h"
#include "POFStructs.h"

#define INITIAL_BUFFER_SIZE 65536
#define TOTAL_TEXTURE_TYPES 4
#define SPECIAL_TEXTURE_TYPES TOTAL_TEXTURE_TYPES - 1
#define MAX_VPS 16
#define LONGEST_FSO_FOLDER_NAME 6
//success/error defines
#define SUCCESS 0
#define USER_ERROR -1
#define FILE_NOT_FOUND -2
#define WRONG_FILETYPE -3
#define CREATEFILE_ERROR 1
#define READFILE_ERROR 2
#define WRITEFILE_ERROR 3

void storeVPNames(LPSTR *readPath);
int getFileFromVP(LPCSTR *writePath, LPCSTR *filename, LPSTR *readPath, LPCSTR folder);
int processFile(LPCSTR *writePath, LPCSTR *writeName, LPSTR *readPath, LPCSTR* readName, direntry* de, LPCSTR folder);
BOOL getDirentry(HANDLE readHandle, LPCSTR *filename, direntry *de, LPCSTR folder);
BOOL isFileType(LPCSTR filename, LPCSTR extension);
int compare(const void *str1, const void *str2);
BOOL isError(int err);
int extractFileFromVP(HANDLE vpHandle, direntry *de, LPCSTR *writePath);
void getTextureNames(int size);

BYTE *buf;
UINT bufferSize;
LPSTR *textureNames;
INT32 numTextures;
LPSTR textureTypes[SPECIAL_TEXTURE_TYPES];
LPSTR vpNames[MAX_VPS];
INT32 numVPs;