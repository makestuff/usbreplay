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
#include <string.h>
#include <ctype.h>
#include "libdump.h"
#include "makestuff.h"

unsigned char *readFile(const char *name, long *length);

typedef struct {
	const char *buf;
	const char *p;
	long length;
} ReplayContext;

/*
 * This line represents a control endpoint read or write. Parse all the parameters
 * so we're ready to execute the operation with libusb. Currently doesn't actually
 * do the operation, it just prints what it would do.
 * 
 * TODO: Implement operation with libusb
 */
void doControlMessage(ReplayContext *this, char readOrWrite) {
	unsigned char bRequestType, bRequest;
	unsigned short wValue, wIndex;
	unsigned int length, i;
	const char *data;
	char *buf;
	bRequestType = (unsigned char)strtoul(this->p, NULL, 16);
	this->p += 2;
	if ( *this->p++ != ' ' ) {
		exit(1);
	}
	bRequest = (unsigned char)strtoul(this->p, NULL, 16);
	this->p += 2;
	if ( *this->p++ != ' ' ) {
		exit(1);
	}
	wValue = (unsigned short)strtoul(this->p, NULL, 16);
	this->p += 4;
	if ( *this->p++ != ' ' ) {
		exit(2);
	}
	wIndex = (unsigned short)strtoul(this->p, NULL, 16);
	this->p += 4;
	if ( *this->p++ != ' ' || *this->p++ != '-' || *this->p++ != '>' || *this->p++ != ' ' ) {
		exit(3);
	}
	data = this->p;
	while ( *this->p++ != '\n' );
	length = (this->p - data)/3;
	buf = malloc(length);
	for ( i = 0; i < length; i++ ) {
		buf[i] = (unsigned char)strtoul(data, NULL, 16);
		data += 3;
	}
	printf("C: 0x%02X 0x%02X 0x%04X 0x%04X %s %d bytes:", bRequestType, bRequest, wValue, wIndex, readOrWrite=='R'?"read":"wrote", length);
	dumpSimple((const uint8 *)buf, length);
	free(buf);
}

/*
 * This line represents a nonzero endpoint read or write. Parse all the parameters
 * so we're ready to execute the operation with libusb. Currently doesn't actually
 * do the operation, it just prints what it would do.
 * 
 * TODO: Implement operation with libusb
 */
void doEndpointMessage(ReplayContext *this, char readOrWrite, int endpoint) {
	unsigned int length, i;
	const char *data;
	char *buf;
	data = this->p;
	while ( *this->p++ != '\n' );
	length = (this->p - data)/3;
	buf = malloc(length);
	for ( i = 0; i < length; i++ ) {
		buf[i] = (unsigned char)strtoul(data, NULL, 16);
		data += 3;
	}
	printf("EP%d %s %d bytes:", endpoint, readOrWrite=='R'?"read":"wrote", length);
	dumpSimple((const uint8 *)buf, length);
	free(buf);
}

/*
 * Identify whether it's a control endpoint operation or not, and delegate.
 */
void processLine(ReplayContext *this) {
	char readOrWrite = *this->p++;
	int endpoint = *this->p++;
	if ( !isdigit(endpoint) || *this->p++ != ':' || *this->p++ != ' ' ) {
		exit(4);
	}
	endpoint -= '0';
	if ( readOrWrite != 'R' && readOrWrite != 'W' ) {
		exit(10);
	}
	if ( endpoint == 0 ) {
		doControlMessage(this, readOrWrite);
	} else {
		doEndpointMessage(this, readOrWrite, endpoint);
	}
}

/*
 * Constructor for a ReplayContext which stores the parser context.
 */
long construct(ReplayContext *this, const char *filename) {
	long length;
	this->p = this->buf = (const char *)readFile(filename, &length);
	this->length = length;
	return length;
}

/*
 * Free up the memory allocated to read the file at construction time.
 */
void destroy(ReplayContext *this) {
	free((char*)this->buf);
	this->p = this->buf = NULL;
	this->length = 0;
}

/*
 * Return true if there is some more data.
 */
bool hasData(ReplayContext *this) {
	if ( this->p >= this->buf + this->length ) {
		return false;
	} else {
		return true;
	}
}

/*
 * Create a parser, read and process each line in turn. Destroy parser.
 */
int main(int argc, const char *argv[]) {
	ReplayContext cxt;
	long returnCode;

	if ( argc != 2 ) {
		fprintf(stderr, "Synopsis: %s <file>\n", argv[0]);
		exit(5);
	}
	returnCode = construct(&cxt, argv[1]);
	if ( returnCode < 0 ) {
		fprintf(stderr, "Unable to read %s returnCode %ld\n", argv[1], returnCode);
		exit(6);
	}
	while ( hasData(&cxt) ) {
		processLine(&cxt);
	}
	printf("%s\n", cxt.p);

	destroy(&cxt);
}
