/* 
 * Copyright (C) 2010,2016 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>

/*
 * Allocate a buffer big enough to fit file into, then read the file into it,
 * then write the file length to the location pointed to by 'length'. Naturally,
 * responsibility for the allocated buffer passes to the caller.
 *
 * Error conditions:
 *   *length == -1: File could not be opened.
 *   *length == -2: Insufficient memory.
 *   *length == -3: Entire file could not be read.
 */
unsigned char *readFile(const char *name, long *length) {
	FILE *file;
	unsigned char *buffer;
	long fileLen;
    long returnCode;

	file = fopen(name, "rb");
	if ( !file ) {
		*length = -1;
		return NULL;
	}
	
	fseek(file, 0, SEEK_END);
	fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate enough space for a NUL terminator, just in case it's a text file
	buffer = (unsigned char *)malloc(fileLen + 1);
	if ( !buffer ) {
		*length = -2;
		fclose(file);
		return NULL;
	}
	returnCode = fread(buffer, 1, fileLen, file);
	if ( returnCode != fileLen ) {
		*length = -3;
	} else {
		*length = fileLen;
	}
	fclose(file);
	return buffer;
}
