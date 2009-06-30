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

/*! @file cgi_template.c
 * @brief CGI used for the webinterface of the SHT application.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "oscar.h"
#include "cgi.h"
#include "errno.h"
#include "string.h"

OscFunction(copyAll, int outFd, int inFd)
	loop {
		char buffer[1024];
		char * pNext = buffer;
		size_t num, remaining = sizeof buffer;
		
		do {
			num = read(inFd, pNext, remaining);
			
			pNext += num;
			remaining -= num;
		} until (remaining == 0 || num == 0);
		
		pNext = buffer;
		remaining = (sizeof buffer) - remaining;
		
		if (remaining == 0)
			break;
		
		do {
			num = write(outFd, buffer, remaining);
			
			pNext += num;
			remaining -= num;
		} until (remaining == 0 || num == 0);
	};
OscFunctionEnd()

OscFunction(processRequest)
	int fd, err;
	
	printf("Status: 200 OK\n");
	printf("Content-Type: text/plain\n");
	printf("\n");
	
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	OscAssert_m(fd >= 0, "Error creating the socket: %s", strerror(errno));
	
	{
		struct sockaddr_un servaddr = { .sun_family = AF_UNIX };
		
		strncpy(servaddr.sun_path, CGI_SOCKET_PATH, sizeof servaddr.sun_path);
		
		err = connect(fd, (struct sockaddr *) &servaddr, SUN_LEN(&servaddr));
		OscAssert_m(err == 0, "Error connecting to the server: %s", strerror(errno));
	}
	
	OscCall(copyAll, fd, 0);
	
	err = shutdown(fd, 1);
	OscAssert_m(err == 0, "Error closing the writing part of the connection: %s", strerror(errno));
	
	OscCall(copyAll, 1, fd);
	
	err = close(fd);
	OscAssert_m(err == 0, "Error closing the writing part of the connection: %s", strerror(errno));
OscFunctionEnd()

OscFunction(mainFunction)
	OscCall(OscCreate, &OscModule_log);
	
	OscCall(processRequest);
OscFunctionFinally()
	OscDestroy();
OscFunctionEnd()

int main(int argc, char ** argv) {
	if (mainFunction() == SUCCESS)
		return 0;
	else
		return 1;
}
