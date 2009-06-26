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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cgi.h"

OscFunction(processRequest)
	OSC_IPC_CHAN_ID ipcChannel;
	struct cgiBuffer buffer;
	
	OscCall(OscIpcRegisterChannel, &ipcChannel, CGI_SOCKET_PATH, 0);
	
	cgiBuffer.length = fread(&buffer.data, 1, sizeof buffer.data, stdin);
	OscAssert(ferror(stdin) == 0);
	
	OscIpcSetParam(ipcChannel, &buffer, ipcParamIds_putRequest, sizeof buffer);
	OscIpcGetParam(ipcChannel, &buffer, ipcParamIds_getResponse, sizeof buffer);
	
	fwrite(&buffer.data, 1, sizeof buffer.data, stdin);
	OscAssert(ferror(stdin) == 0);
	
OscFunctionFinally()
	OscCall(OscIpcUnregisterChannel, ipcChannel);
OscFunctionEnd()

OscFunction(mainFunction)
	OscCall(OscCreate, OscModule_log, OscModule_ipc);
	
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
