#pragma once

#include <Windows.h>
#include <stdio.h>
#include <malloc.h>
#include "VPStructs.h"
#include "POFStructs.h"

#define INITIAL_BUFFER_SIZE 65536
#define MAX_TEXTURES 32
//success/error defines
#define SUCCESS 0
#define USER_ERROR -1
#define FILE_NOT_FOUND -2
#define CREATEFILE_ERROR 1
#define READFILE_ERROR 2
#define WRITEFILE_ERROR 3

int getFileFromVP(LPCSTR *writePath, LPCSTR *filename, LPSTR *readPath);
BOOL getDirentry(HANDLE readHandle, LPCSTR *filename, direntry *de);
BOOL isFileType(LPCSTR filename, LPCSTR extension);
int extractFileFromVP(HANDLE vpHandle, direntry *de, LPCSTR *writePath);
void getTextureNames(int size);

BYTE *buf;
DWORD bufferSize;
LPSTR textureNames[MAX_TEXTURES];
INT32 numTextures;