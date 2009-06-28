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

#include "template.h"
#include "cgi/cgi.h"
#include <string.h>

#define MAX_ARGS

struct argument {
	char * key;
	char * value;
};

struct command {
	char * name;
	struct argument args[];
};

static OSC_ERR CheckIpcRequests(uint32 * pParamId)
{
	OSC_ERR err;
	struct IPC_DATA *pIpc = &data.ipc;
	struct OSC_IPC_REQUEST *pReq = &pIpc->req;
	
	if (pIpc->enReqState != REQ_STATE_IDLE) {
		// This means we still have an unacknowledged request from last time. Proceed with the acknowledgement instead of already getting new ones.
		err = ENO_MSG_AVAIL;
	}
	
	/* Get the next request. */
	err = OscIpcGetRequest(pIpc->ipcChan, pReq);
	if (err == SUCCESS) {
		/* We have a request. */
		
		/* In case of success simply return the parameter ID of the requested parameter. */
		*pParamId = pReq->paramID;
	} else {
		/* Getting request not successful => analyze why. */
		if (likely(err == -ENO_MSG_AVAIL))
			err = ENO_MSG_AVAIL;
		else
			OscLog(ERROR, "%s: Error getting IPC request! (%d)\n", __func__, err);
	}
	
	return err;
}

static OSC_ERR AckIpcRequests()
{
	struct IPC_DATA *pIpc = &data.ipc;
	struct OSC_IPC_REQUEST *pReq = &pIpc->req;
	OSC_ERR err;
	bool bSuccess;
	
	if (pIpc->enReqState == REQ_STATE_IDLE) {
		/* Nothing to acknowledge. */
		return SUCCESS;
	} else if (pIpc->enReqState == REQ_STATE_NACK_PENDING) {
		bSuccess = FALSE; }
	else {
		bSuccess = TRUE;
	}
	
	err = OscIpcAckRequest(pIpc->ipcChan, pReq, bSuccess);
	if (err == SUCCESS) {
		/* Ack sent successfully. Now we're ready for the next request.*/
		pIpc->enReqState = REQ_STATE_IDLE;
	} else if (err == -ETRY_AGAIN) {
		/* Not really an error, just means we have to try again later, which will happen soon enough. */
		err = SUCCESS;
	}
	
	return err;
}

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

OscFunction(getHeader, char ** pHeader) {
	static char buffer[1024];
	
	OscAssert_e(fgets (buffer, sizeof buffer, stdin) == NULL, -EUNABLE_TO_READ;)
	
	*pHeader = strtrim(buffer);
OscFunctionEnd()

OscFunction(getArgument, char ** pKey, char ** pValue) {
	static char buffer[1024];
	char * colon;
	
	OscAssert_e(fgets (buffer, sizeof buffer, stdin) == NULL, -EUNABLE_TO_READ;)
	
	colon = strchr(buffer, ':');
	
	*colon = 0;
	colon += 1;
	
	*pKey = strtrim(buffer);
	*pValue = strtrim(colon);
OscFunctionEnd()

OscFunction(processRequest, FILE * request, FILE * response)
	
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
	static uint8_t buffer[1024];
	static * uint8_t pNext;
	uint8_t * const pEnd = buffer + length(buffer);
	
	if (ipcState == ipcState_uninitialized) {
		int err;
		struct sockaddr_un addr = { .sun_family = AF_UNIX };
		
		OscAssert_m(strlen(CGI_SOCKET_PATH) >= sizeof (addr.sun_path), "Path too long.")
		strcpy(addr.sun_path, pName);
		
		socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
		OscAssert_m(socketFd >= 0, "Cannot open a socket.");
		
		err = fcntl(socketFd, F_SETFL, O_NONBLOCK);
		OscAssert_m(err >= 0, "Cannot set O_NONBLOCK.");
		
		err = bind(socketFd, (struct sockaddr *) &addr, SUN_LEN(&addr));
		OscAssert_m(err >= 0, "Cannot bind to the socket.");
		
		err = listen(socketFd, 1);
		OscAssert_m(err >= 0, "Cannot listen on the socket.");
		
		state = ipcState_listening;
	} else if (ipcState == ipcState_listening) {
		{
			unsigned int remoteAddrLen;
			struct sockaddr remoteAddr;
			
			fd = accept(sock, &remoteAddr, &remoteAddrLen);
		}
		
		if (fd < 0) {
			OscAssert_m(errno != EAGAIN, "Error accepting a connection: %s", strerror(errno));
		} else {
			pNext = buffer;
			state = ipcState_recieving;
		}
	} else if (ipcState == ipcState_recieving) {
		ssize_t numRead;
		
		errno = 0;
		numRead = read(fd, pNext, pEnd - pNext);
		
		if (numRead > 0) {
			pNext += numRead;
		} else if (numRead == 0) {
			OscAssert_m(errno == 0, "Error while reading from socket: %s", strerror(errno));
			OscCall(processRequest(buffer));
			state = ipcState_sending;
		}
	} else if (ipcState == ipcState_sending) {
		
	}
OscFunctionEnd()
