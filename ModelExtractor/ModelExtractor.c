#include "ModelExtractor.h"

/*
TODO folder permissions
TODO delete file if error
TODO search through subdirs with recursion
maybe put behind a switch though to speed it up
also can check for the desired file at the same time
TODO convert isFileType to use pointers
TODO extract files to proper fs data directories
TODO remove limit on textures and VP files
*/

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

	LPSTR readFolderPath = (LPSTR)_malloca(MAX_PATH);
	LPSTR writeFilePath = (LPSTR)_malloca(MAX_PATH);
	LPSTR writeFolderPath = (LPSTR)_malloca(MAX_PATH);
	LPSTR pofName = (LPSTR)_malloca(MAX_PATH);

	int rv;

	bufferSize = INITIAL_BUFFER_SIZE;
	buf = (BYTE*)malloc(bufferSize);

	for (int i = 0; i < MAX_TEXTURES; i++) {
		textureNames[i] = (LPSTR)_malloca(MAX_PATH);
	}
	for (int i = 0; i < MAX_VPS; i++) {
		vpNames[i] = (LPSTR)_malloca(MAX_PATH);
	}

	strncpy_s(readFolderPath, MAX_PATH, argv[1], strnlen_s(argv[1], MAX_PATH - 1));
	//strip the leading ".\" that PowerShell might use
	if (readFolderPath[0] == '.' && readFolderPath[1] == '\\') {
		readFolderPath += 2;
	}

	//strip trailing slashes
	size_t len = strnlen_s(readFolderPath, MAX_PATH);
	if (readFolderPath[len - 1] == '\\' || readFolderPath[len - 1] == '/') {
		readFolderPath[len - 1] = 0;
	}

	strncpy_s(writeFilePath, MAX_PATH, argv[2], strnlen_s(argv[2], MAX_PATH - 1));
	strncpy_s(writeFolderPath, MAX_PATH, argv[2], strnlen_s(argv[2], MAX_PATH - 1));

	char *c = strrchr(writeFilePath, '\\');
	if (c != NULL) {
		c++;
	}
	else {

	}
	strncpy_s(pofName, MAX_PATH, c, strnlen_s(c, MAX_PATH - 1));

	c = strrchr(writeFolderPath, '\\');
	if (c != NULL) {
		*c = 0;
	}
	else {

	}

	storeVPNames(&readFolderPath);
	qsort(vpNames, numVPs, sizeof(char*), compare);

	rv = getFileFromVP(&writeFolderPath, &pofName, &readFolderPath);
	//TODO make sure file was found
	if (!isError(rv)) {
		getTextureNames(rv);
		for (int i = 0; i < numTextures; i++) {
			rv = getFileFromVP(&writeFolderPath, &(textureNames[i]), &readFolderPath);
			if (isError(rv)) {
				break;
			}
			if (rv == FILE_NOT_FOUND) {
				printf("Could not find %s\n", textureNames[i]);
			}
		}
	}

	free(buf);
	buf = NULL;
	if (rv == CREATEFILE_ERROR) {
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

void storeVPNames(LPSTR *readPath) {
	HANDLE fileSearchHandle;
	WIN32_FIND_DATAA fileInfo;
	char readFilePath[MAX_PATH];
	size_t len = strnlen_s(*readPath, MAX_PATH);

	strncpy_s(readFilePath, MAX_PATH, *readPath, len);
	strcat_s(readFilePath, MAX_PATH, "\\*");
	fileSearchHandle = FindFirstFileA(readFilePath, &fileInfo);

	if (isFileType(fileInfo.cFileName, "vp")) {
		strncpy_s(vpNames[numVPs++], MAX_PATH, fileInfo.cFileName, strnlen_s(fileInfo.cFileName, MAX_PATH));
	}

	while (FindNextFileA(fileSearchHandle, &fileInfo)) {
		if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}

		if (isFileType(fileInfo.cFileName, "vp")) {
			if (numVPs > MAX_VPS) {
				printf("Only using the first 16 VP files.  Please report this so the limit can be increased.");
			}
			strncpy_s(vpNames[numVPs++], MAX_PATH, fileInfo.cFileName, strnlen_s(fileInfo.cFileName, MAX_PATH));
		}
	}
}

//writePath is the folder where the output file should be written, w/o the filename
//@return file size if file is found and written; error otherwise
int getFileFromVP(LPCSTR *writePath, LPCSTR *filename, LPSTR *readPath) {
	direntry de;
	int rv;

	for (int i = 0; i < numVPs; i++) {
		rv = processFile(writePath, filename, readPath, &vpNames[i], &de);
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
int processFile(LPCSTR *writePath, LPCSTR *writeName, LPSTR *readPath, LPCSTR* readName, direntry* de) {
	HANDLE readHandle;
	char readFilePath[MAX_PATH];
	size_t len = strnlen_s(*readPath, MAX_PATH);
	int rv;

	strncpy_s(readFilePath, MAX_PATH, *readPath, len);
	strcat_s(readFilePath, MAX_PATH, "\\");
	strncat_s(readFilePath, MAX_PATH, *readName, MAX_PATH - 1);
	readHandle = CreateFileA(readFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (readHandle == INVALID_HANDLE_VALUE) {
		rv = CREATEFILE_ERROR;
	}
	else {
		if (getDirentry(readHandle, writeName, de)) {
			rv = extractFileFromVP(readHandle, de, writePath);
			CloseHandle(readHandle);
			return rv;
		}
		else {
			rv = FILE_NOT_FOUND;
		}
	}

	CloseHandle(readHandle);

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
	UINT pos = 0;
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

int compare(const void *str1, const void *str2) {
	return _strnicmp(*((char**)str1), *((char**)str2), MAX_PATH);
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
	LPSTR writeFilePath = (LPSTR)_malloca(MAX_PATH);
	LARGE_INTEGER li;
	DWORD numRead, numWritten;
	int err;

	li.QuadPart = de->offset;
	SetFilePointerEx(vpHandle, li, NULL, FILE_BEGIN);

	strncpy_s(writeFilePath, MAX_PATH, *writeDirPath, strnlen_s(*writeDirPath, MAX_PATH));
	strncat_s(writeFilePath, MAX_PATH, "\\", 1);
	strncat_s(writeFilePath, MAX_PATH, de->filename, strnlen_s(de->filename, 32));
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

	if (numTextures > MAX_TEXTURES) {
		printf("Only using the first 16 textures files. Please report this so the limit can be increased.");
	}

	//each texture consists of a (string)
	//(string) == an int specifying length of string and char[length] for the string itself
	for (int i = 0; i < numTextures; i++) {
		memcpy_s(&textureNameLen, sizeof(INT32), buf + offset, sizeof(INT32));
		offset += sizeof(INT32);
		memcpy_s(textureNames[i], MAX_PATH, buf + offset, textureNameLen);
		offset += textureNameLen;
	}
}