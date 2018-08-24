#include "ModelExtractor.h"

/*
Read in the vp files from the given dir
load all the direntries into memory (probably) and look for the given pof name
write it to the given output folder
while writing it, look for the 'TXTR' chunk
when found, copy each filename into an array
when done writing the pof, look for those textures in the same way as finding the pof
*/

int main(int argc, char* argv[]) {
	if (argc != 3) {
		printf("Usage: %s <Path\\To\\InputFolder> <Path\\To\\Output.pof>", argv[0]);
		return USER_ERROR;
	}

	LPSTR readFolderPath = (LPSTR)_malloca(MAX_PATH + 1);
	LPSTR writeFilePath = (LPSTR)_malloca(MAX_PATH + 1);
	LPSTR writeFolderPath = (LPSTR)_malloca(MAX_PATH + 1);
	LPSTR pofName = (LPSTR)_malloca(MAX_PATH + 1);

	int rv;

	bufferSize = INITIAL_BUFFER_SIZE;
	buf = (BYTE*)malloc(bufferSize);

	for (int i = 0; i < MAX_TEXTURES; i++) {
		textureNames[i] = (LPSTR)_malloca(MAX_PATH + 1);
	}

	strncpy_s(readFolderPath, MAX_PATH + 1, argv[1], strnlen_s(argv[1], MAX_PATH));
	//strip the leading ".\" that PowerShell might use
	if (readFolderPath[0] == '.' && readFolderPath[1] == '\\') {
		readFolderPath += 2;
	}

	//strip trailing slashes
	size_t len = strnlen_s(readFolderPath, MAX_PATH);
	if (readFolderPath[len - 1] == '\\' || readFolderPath[len - 1] == '/') {
		readFolderPath[len - 1] = 0;
	}

	strncpy_s(writeFilePath, MAX_PATH + 1, argv[2], strnlen_s(argv[2], MAX_PATH));
	strncpy_s(writeFolderPath, MAX_PATH + 1, argv[2], strnlen_s(argv[2], MAX_PATH));

	char *c = strrchr(writeFilePath, '\\');
	if (c != NULL) {
		c++;
	}
	else {

	}
	strncpy_s(pofName, MAX_PATH + 1, c, strnlen_s(c, MAX_PATH));

	c = strrchr(writeFolderPath, '\\');
	if (c != NULL) {
		*c = 0;
	}
	else {

	}

	rv = getFileFromVP(&writeFolderPath, &pofName, &readFolderPath);
	if (!isError(rv)) {
		getTextureNames(rv);
		for (int i = 0; i < numTextures; i++) {
			rv = getFileFromVP(&writeFolderPath, &(textureNames[i]), &readFolderPath);
			if (isError(rv)) {
				break;
			}
		}
	}

	free(buf);
	buf = NULL;
	if (rv == FILE_NOT_FOUND) {
		printf("Could not find a file");
	}
	else if (rv == CREATEFILE_ERROR) {
		printf("Error creating a necessary file");
	}
	else if (rv == READFILE_ERROR) {
		printf("Error reading a file");
	}
	else if (rv == WRITEFILE_ERROR) {
		printf("Error writing the vp file");
	}
	else {
		printf("Success");
		return SUCCESS;
	}
	return rv;
}

//writePath is the folder where the output file should be written, w/o the filename
//@return file size if file is found and written; error otherwise
int getFileFromVP(LPCSTR *writePath, LPCSTR *filename, LPSTR *readPath) {
	direntry de;
	size_t len = strnlen_s(*readPath, MAX_PATH);
	HANDLE fileSearchHandle;
	WIN32_FIND_DATAA fileInfo;
	char readFilePath[MAX_PATH + 1];
	int rv;

	strncpy_s(readFilePath, MAX_PATH + 1, *readPath, len);
	strcat_s(readFilePath, MAX_PATH + 1, "\\*");
	fileSearchHandle = FindFirstFileA(readFilePath, &fileInfo);

	rv = processFile(writePath, filename, readPath, &fileInfo, &de);
	if (isError(rv)) {
		return rv;
	}
	if (rv != FILE_NOT_FOUND && rv != WRONG_FILETYPE) {
		return rv;
	}

	while (FindNextFileA(fileSearchHandle, &fileInfo)) {
		if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}

		rv = processFile(writePath, filename, readPath, &fileInfo, &de);
		if (isError(rv)) {
			return rv;
		}
		if (rv != FILE_NOT_FOUND && rv != WRONG_FILETYPE) {
			return rv;
		}
	}

	return rv;
}

//@return file size if successful; error if failed; or WRONG_FILETYPE
int processFile(LPCSTR *writePath, LPCSTR *filename, LPSTR *readPath, WIN32_FIND_DATAA* fileInfo, direntry* de) {
	HANDLE readHandle;
	char readFilePath[MAX_PATH + 1];
	size_t len = strnlen_s(*readPath, MAX_PATH);
	int rv;

	if (isFileType(fileInfo->cFileName, "vp")) {
		strncpy_s(readFilePath, MAX_PATH + 1, *readPath, len);
		strcat_s(readFilePath, MAX_PATH + 1, "\\");
		strncat_s(readFilePath, MAX_PATH + 1, fileInfo->cFileName, MAX_PATH);
		readHandle = CreateFileA(readFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (readHandle == INVALID_HANDLE_VALUE) {
			rv = CREATEFILE_ERROR;
		}
		else {
			if (getDirentry(readHandle, filename, de)) {
				rv = extractFileFromVP(readHandle, de, writePath);
				CloseHandle(readHandle);
				return rv;
			}
			else {
				rv = FILE_NOT_FOUND;
			}
		}

		CloseHandle(readHandle);
	}

	rv = WRONG_FILETYPE;
	return rv;
}

//@return TRUE/FALSE for whether file is found
BOOL getDirentry(HANDLE readHandle, LPCSTR *filename, direntry *de) {
	BOOL rv;
	DWORD numRead;
	DWORD totalRead = 0;

	header head;
	rv = ReadFile(readHandle, &head, sizeof(header), &numRead, NULL);

	DWORD direntriesSize = head.direntries * sizeof(direntry);
	LARGE_INTEGER li;
	DWORD pos = 0;
	size_t filenameLen = strnlen_s(*filename, MAX_PATH);

	li.QuadPart = head.diroffset;
	SetFilePointerEx(readHandle, li, NULL, FILE_BEGIN);

	do {
		rv = ReadFile(readHandle, buf, bufferSize, &numRead, NULL);

		if (rv) {
			for (int i = 0; i < head.direntries; i++) {
				if (pos + sizeof(direntry) > bufferSize) {
					int leftover = bufferSize - pos;
					memcpy_s(buf, leftover, buf + pos, leftover);
					ReadFile(readHandle, buf + leftover, bufferSize - leftover, &numRead, NULL);
					pos = 0;
				}
				memcpy_s(de, sizeof(direntry), buf + pos, sizeof(direntry));
				if (de->size > 0 && !_strnicmp(*filename, de->filename, filenameLen)) {
					return TRUE;
				}
				pos += sizeof(direntry);
			}
		}
		else break;

		totalRead += numRead;
		pos = 0;
	} while (totalRead < direntriesSize);

	return FALSE;
}

BOOL isFileType(LPCSTR filename, LPCSTR extension) {
	LPSTR fileExt = strrchr(filename, '.');
	if (fileExt != NULL) {
		fileExt += 1;
	}
	else return FALSE;

	if (!_strnicmp(fileExt, extension, 2)) {
		return TRUE;
	}
	return FALSE;
}

//IMPORTANT make sure to keep updated
BOOL isError(int err) {
	if (err == USER_ERROR) {
		return TRUE;
	}
	else if (err == CREATEFILE_ERROR) {
		return TRUE;
	}
	else if (err == READFILE_ERROR) {
		return TRUE;
	}
	else if (err == WRITEFILE_ERROR) {
		return TRUE;
	}
	return FALSE;
}

//TODO maybe change failure return value so it doesn't conflict with user error
//TODO maybe don't read the whole file into memory at once?
//writePath needs to be the full absolute path of the file
//@return file size if file is found inside VP and written successfully; error otherwise
int extractFileFromVP(HANDLE vpHandle, direntry *de, LPCSTR *writeDirPath) {
	HANDLE writeHandle;
	LPSTR writeFilePath = (LPSTR)_malloca(MAX_PATH + 1);
	LARGE_INTEGER li;
	DWORD numRead, numWritten;
	int err;

	li.QuadPart = de->offset;
	SetFilePointerEx(vpHandle, li, NULL, FILE_BEGIN);

	strncpy_s(writeFilePath, MAX_PATH + 1, *writeDirPath, strnlen_s(*writeDirPath, MAX_PATH + 1));
	strncat_s(writeFilePath, MAX_PATH + 1, "\\", 1);
	strncat_s(writeFilePath, MAX_PATH + 1, de->filename, strnlen_s(de->filename, 32));
	writeHandle = CreateFileA(writeFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (writeHandle == INVALID_HANDLE_VALUE) {
		return CREATEFILE_ERROR;
	}

	if (de->size > bufferSize) {
		realloc(buf, de->size);
	}

	err = ReadFile(vpHandle, buf, de->size, &numRead, NULL);
	if (err) {
		err = WriteFile(writeHandle, buf, de->size, &numWritten, NULL);
	}
	else {
		CloseHandle(writeHandle);
		return READFILE_ERROR;
	}

	CloseHandle(writeHandle);
	if (err) {
		return de->size;
	}
	else return WRITEFILE_ERROR;
}

//When this function is called, buf should contain the pof file
//TODO figure out whether there can be multiple texture sections in the file (probably not)
void getTextureNames(int size) {
	//header is 8 bytes large
	DWORD offset = 8;
	chunk section;
	size_t chunkSize = sizeof(chunk);
	INT32 textureNameLen;

	do {
		memcpy_s(&section, chunkSize, buf + offset, chunkSize);
		offset += section.length;
	} while (section.chunk_id[0] != 'T' || section.chunk_id[1] != 'X' || section.chunk_id[2] != 'T' || section.chunk_id[3] != 'R');

	offset = offset - section.length + 4 + sizeof(INT32);
	memcpy_s(&numTextures, sizeof(INT32), buf + offset, sizeof(INT32));
	offset += sizeof(INT32);

	//each texture consists of a (string)
	//(string) == an int specifying length of string and char[length] for the string itself
	for (int i = 0; i < numTextures; i++) {
		memcpy_s(&textureNameLen, sizeof(INT32), buf + offset, sizeof(INT32));
		offset += sizeof(INT32);
		memcpy_s(textureNames[i], MAX_PATH, buf + offset, textureNameLen);
		offset += textureNameLen;
	}
}