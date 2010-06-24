/* Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty. This file is offered as-is,
 * without any warranty.
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
	fflush(stdout);
	
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	OscAssert_m(fd >= 0, "Error creating the socket: %s", strerror(errno));
	
	{
		struct sockaddr_un servaddr;
		servaddr.sun_family = AF_UNIX;
		
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
	OscCall(processRequest);
OscFunctionEnd()

int main(int argc, char ** argv) {
	if (mainFunction() == SUCCESS)
		return 0;
	else
		return 1;
}
