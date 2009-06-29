/*	A collection of example applications for the LeanXcam platform.
	Copyright (C) 2008 Supercomputing Systems AG
	
	This library is free software; you can redistribute it and/or modify it
	under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation; either version 2.1 of the License, or (at
	your option) any later version.
	
	This library is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
	General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this library; if not, write to the Free Software Foundation,
	Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*! @file main.c
 * @brief Implements the IPC handling of the template application.
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "template.h"
#include "cgi/cgi.h"
#include "mainstate.h"

#define BUFFER_SIZE (1024)

struct argument {
	char * key;
	char * value;
};

struct command {
	char * name;
	struct argument args[];
};

/*! @brief Strips whiltespace from the beginning and the end of a string and returns the new beginning of the string. Be advised, that the original string gets mangled! */
static char * strtrim(char * str) {
	char * end = strchr(str, 0) - 1;

	while (*str != 0 && strchr(" \t\n", *str) != NULL)
		str += 1;

	while (end > str && strchr(" \t\n", *end) != NULL)
		end -= 1;

	*(end + 1) = 0;

	return str;
}

OscFunction(getHeader, char ** pHeader, char ** pBuffer)
	char * newline = strchr(*pBuffer, '\n');
	OscAssert_m(newline != NULL, "No newline found.");
	
	*newline = 0;
	*pHeader = strtrim(*pBuffer);
	*pBuffer = newline + 1;
OscFunctionEnd()

OscFunction(getArgument, char ** pKey, char ** pValue, char ** pBuffer)
	char * newline, * colon;
	
	newline = strchr(*pBuffer, '\n');
	*newline = 0;
	colon = strchr(*pBuffer, 0);
	*colon = 0;
	
	
	///////////////
	OscAssert_e(fgets(buffer, sizeof buffer, file) == NULL, -EUNABLE_TO_READ)
	
	colon = strchr(buffer, ':');
	
	*colon = 0;
	colon += 1;
	
	*pKey = strtrim(buffer);
	*pValue = strtrim(colon);
OscFunctionEnd()

OscFunction(processRequest, FILE * request, FILE * response)
	static char buffer[1024];
OscFunctionEnd()

enum ipcState {
	ipcState_uninitialized,
	ipcState_listening,
	ipcState_recieving,
	ipcState_sending
};

/*********************************************************************//*!
 * @brief Checks for IPC events, schedules their handling and
 * acknowledges any executed ones.
 * 
 * @param pMainState Initalized HSM main state variable.
 * @return 0 on success or an appropriate error code.
 *//*********************************************************************/
OscFunction(handleIpcRequests, MainState * pMainState)
	static enum ipcState state = ipcState_uninitialized;
	static int socketFd;
	static int fd;
	static uint8_t buffer[BUFFER_SIZE];
	static uint8_t * pNext;
	size_t remaining;
	
	if (state == ipcState_uninitialized) {
		int err;
		struct sockaddr_un addr = { .sun_family = AF_UNIX };
		
		OscAssert_m(strlen(CGI_SOCKET_PATH) < sizeof (addr.sun_path), "Path too long.")
		strcpy(addr.sun_path, CGI_SOCKET_PATH);
		unlink(addr.sun_path);
		
		socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
		OscAssert_m(socketFd >= 0, "Cannot open a socket.");
		
		err = fcntl(socketFd, F_SETFL, O_NONBLOCK);
		OscAssert_m(err == 0, "Error setting O_NONBLOCK: %s", strerror(errno));
		
		err = bind(socketFd, (struct sockaddr *) &addr, SUN_LEN(&addr));
		OscAssert_m(err == 0, "Error binding the socket: %s", strerror(errno));
		
		err = listen(socketFd, 5);
		OscAssert_m(err == 0, "Error listening on the socket: %s", strerror(errno));
		
		state = ipcState_listening;
	} else if (state == ipcState_listening) {
		{
			unsigned int remoteAddrLen;
			struct sockaddr remoteAddr;
			
			fd = accept(socketFd, &remoteAddr, &remoteAddrLen);
		}
		
		if (fd < 0) {
			OscAssert_m(errno == EAGAIN, "Error accepting a connection: %s", strerror(errno));
		} else {
			pNext = buffer;
			remaining = BUFFER_SIZE;
			state = ipcState_recieving;
		}
	} else if (state == ipcState_recieving) {
		ssize_t numRead = read(fd, buffer + BUFFER_SIZE - remaining, remaining);
		
		OscAssert_m(remaining > 0, "No buffer space left.");
		
		if (numRead > 0) {
			// We got some data.
			pNext += numRead;
			remaining -= numRead;
		} else if (numRead < 0) {
			OscAssert_m(errno == EAGAIN, "Error while reading from the IPC connection: %s", strerror(errno));
		} else {
			uint8_t outBuffer[BUFFER_SIZE];
			FILE * inFile = fmemopen(buffer, BUFFER_SIZE - remaining, "r");
			FILE * outFile = fmemopen(outBuffer, BUFFER_SIZE, "w");
			
			OscCall(processRequest, inFile, outFile);
			remaining = ftell(outFile);
			
			OscAssert(fclose(inFile) == 0);
			OscAssert(fclose(outFile) == 0);
			
			memcpy(buffer, outBuffer, BUFFER_SIZE);
			
			pNext = buffer;
			state = ipcState_sending;
		}
	} else if (state == ipcState_sending) {
		if (remaining > 0) {
			ssize_t numWritten;
			
			numWritten = write(fd, pNext, remaining);
			
			if (numWritten > 0) {
				// We've written some data.
				pNext += numWritten;
				remaining -= numWritten;
			} else if (numWritten < 0) {
				OscAssert_m(errno == EAGAIN, "Error while writing to the IPC connection: %s", strerror(errno));
			}
		} else {
			close(fd);
			state = ipcState_listening;
		}
	}
OscFunctionEnd()
