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

OscFunction(processRequest)
/*	OSC_IPC_CHAN_ID ipcChannel;
	struct cgiBuffer buffer;
	
	OscCall(OscIpcRegisterChannel, &ipcChannel, CGI_SOCKET_PATH, 0);
	
	cgiBuffer.length = fread(&buffer.data, 1, sizeof buffer.data, stdin);
	OscAssert(ferror(stdin) == 0);
	
	OscIpcSetParam(ipcChannel, &buffer, ipcParamIds_putRequest, sizeof buffer);
	OscIpcGetParam(ipcChannel, &buffer, ipcParamIds_getResponse, sizeof buffer);
	
	fwrite(&buffer.data, 1, sizeof buffer.data, stdin);
	OscAssert(ferror(stdin) == 0);
*/
	
	int sock, err;
	struct sockaddr_un servaddr;/* address of server */
	char * data = "Foo!\n";
	
	/*      Set up address structure for server socket */
	servaddr = (struct sockaddr_un) { };
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, CGI_SOCKET_PATH);
	
	/* Create a UNIX datagram socket for client */
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("client: socket");
		return 2;
	}
	
	err = connect(sock, (struct sockaddr *) &servaddr, SUN_LEN(&servaddr));
	if (err < 0) {
		close(sock);
		perror("client: connect");
		return 3;
	}
	
	write(sock, data, strlen(data));
	
	shutdown(sock, 1);
	
	{
		char buffer[1024];
		int nRead = read(sock, buffer, sizeof buffer);
		
		buffer[nRead] = 0;
		
		printf("%s\n", buffer);
	}
	
	close(sock);
	printf("Client done\n");
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
