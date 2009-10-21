/* Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty. This file is offered as-is,
 * without any warranty.
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
#include <stdlib.h>

#include "ipc.h"
#include "cgi/cgi.h"
#include "mainstate.h"

#define BUFFER_SIZE (1024)

#ifdef OSC_HOST
//#define HTTP_DIR "/var/www/"
#define HTTP_DIR ""
#else
#define HTTP_DIR "/home/httpd/"
#endif

#define CGI_IMAGE_NAME HTTP_DIR "image.bmp"

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

OscFunction(static readHeader, char ** pBuffer, char ** pHeader)
	char * newline = strchr(*pBuffer, '\n');
	OscAssert_m(newline != NULL, "No newline found.");
	
	*newline = 0;
	*pHeader = strtrim(*pBuffer);
	*pBuffer = newline + 1;
OscFunctionEnd()

OscFunction(static readArgument, char ** pBuffer, char ** pKey, char ** pValue)
	char * newline, * colon;
	
	newline = strchr(*pBuffer, '\n');
	OscAssert_m(newline != NULL, "No newline found.");
	*newline = 0;
	
	colon = strchr(*pBuffer, ':');
	OscAssert_m(colon != NULL, "No colon found.");
	*colon = 0;
		
	*pKey = strtrim(*pBuffer);
	*pValue = strtrim(colon + 1);
	*pBuffer = newline + 1;
OscFunctionEnd()

OscFunction(static writeArgument, char ** pBuffer, size_t * pBufferSize, char * pKey, char * pValue)
	int n = snprintf(*pBuffer, *pBufferSize, "%s: %s\n", pKey, pValue);
	
	OscAssert_m(n < *pBufferSize, "No buffer space left.");
	
	*pBuffer += n;
	*pBufferSize -= n;
OscFunctionEnd()

OscFunction(static processRequest, char ** pResponse, char * request, struct MainState * mainState)
	static char buffer[1024];
	char * pNext = buffer;
	size_t remaining = sizeof buffer;
	char * header;
	
	*buffer = 0;
	OscCall(readHeader, &request, &header);
	
	if (strcmp(header, "SetOptions") == 0) {
		until (*request == 0) {
			char * key, * value;
			
			OscCall(readArgument, &request, &key, &value);
			OscMark_m("%s: %s", key, value);
		
			if (strcmp(key, "exposureTime") == 0) {
				mainState->options.exposureTime = strtol(value, NULL, 10);
			} else if (strcmp(key, "colorType") == 0) {
				if (strcmp(value, "none") == 0)
					mainState->options.colorType = ColorType_none;
				else if (strcmp(value, "gray") == 0)
					mainState->options.colorType = ColorType_gray;
				else if (strcmp(value, "raw") == 0)
					mainState->options.colorType = ColorType_raw;
				else if (strcmp(value, "debayered") == 0)
					mainState->options.colorType = ColorType_debayered;
				else
					OscFail_m("Unknown value '%s' for argument '%s' of command '%s'.", value, key, header);
			} else {
				OscFail_m("Unknown argument '%s' for command '%s'.", key, header);
			}
			
			throwEvent(mainState, MainStateEvent_ipcSetOptions);
		}
	} else if (strcmp(header, "GetImage") == 0) {
		char numBuf[32]; // FIXME: writeArgument should somehow be extended to allow number conversions and such.
		char * pEnumBuf = NULL; // FIXME: ditto.
			
		if (mainState->imageInfo.colorType != ColorType_none) {
			struct OSC_PICTURE pic = {
				.data = mainState->imageInfo.data,
				.width = mainState->imageInfo.width,
				.height = mainState->imageInfo.height,
			};
			
			if (mainState->imageInfo.colorType == ColorType_debayered)
				pic.type = OSC_PICTURE_BGR_24;
			else
				pic.type = OSC_PICTURE_GREYSCALE;
			
			OscCall(OscBmpWrite, &pic, CGI_IMAGE_NAME);
			
			OscCall(writeArgument, &pNext, &remaining, "path", CGI_IMAGE_NAME);
		}
		
		snprintf(numBuf, sizeof numBuf, "%d", mainState->imageInfo.width);
		OscCall(writeArgument, &pNext, &remaining, "width", numBuf);
		
		snprintf(numBuf, sizeof numBuf, "%d", mainState->imageInfo.height);
		OscCall(writeArgument, &pNext, &remaining, "height", numBuf);
		
		snprintf(numBuf, sizeof numBuf, "%d", mainState->imageInfo.exposureTime);
		OscCall(writeArgument, &pNext, &remaining, "exposureTime", numBuf);
		
		if (mainState->imageInfo.colorType == ColorType_none) {
			pEnumBuf = "none";
		} else if (mainState->imageInfo.colorType == ColorType_gray) {
			pEnumBuf = "gray";
		} else if (mainState->imageInfo.colorType == ColorType_raw) {
			pEnumBuf = "raw";
		} else if (mainState->imageInfo.colorType == ColorType_debayered) {
			pEnumBuf = "debayered";
		}
		
		OscCall(writeArgument, &pNext, &remaining, "colorType", pEnumBuf);
	} else if (strcmp(header, "GetSystemInfo") == 0) {
		struct OscSystemInfo * pInfo;
		
		OscCall(OscCfgGetSystemInfo, &pInfo);
		
		OscCall(writeArgument, &pNext, &remaining, "cameraModel", pInfo->hardware.board.revision);
		OscCall(writeArgument, &pNext, &remaining, "imageSensor", pInfo->hardware.imageSensor.hasBayernPattern ? "Color" : "Grayscale");
		OscCall(writeArgument, &pNext, &remaining, "uClinuxVersion", pInfo->software.uClinux.version);
	}
	
//	sprintf(buffer, sizeof buffer, "Header: %s", header);
	
	*pResponse = buffer;
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
OscFunction(handleIpcRequests, struct MainState * mainState)
	static enum ipcState state = ipcState_uninitialized;
	static int socketFd;
	static int fd;
	static char buffer[BUFFER_SIZE];
	static char * pNext;
	static size_t remaining;
	
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
		
		err = chmod(addr.sun_path, SERV_SOCKET_PERMISSIONS);
		OscAssert_m(err >= 0, "Unable to set access permissions of "
					"socket file node \"%s\"! (%s)", addr.sun_path, strerror(errno));

		err = listen(socketFd, 5);
		OscAssert_m(err == 0, "Error listening on the socket: %s", strerror(errno));
		
		state = ipcState_listening;
	} else if (state == ipcState_listening) {
		{
			struct sockaddr remoteAddr;
			unsigned int remoteAddrLen = sizeof(struct sockaddr);
			
			fd = accept(socketFd, &remoteAddr, &remoteAddrLen);
		}
		
		if (fd < 0) {
			OscAssert_m(errno == EAGAIN, "Error accepting a connection: %s", strerror(errno));
		} else {
			int err = fcntl(fd, F_SETFL, O_NONBLOCK);
			OscAssert_m(err == 0, "Error setting O_NONBLOCK: %s", strerror(errno));

			pNext = buffer;
			remaining = BUFFER_SIZE;
			state = ipcState_recieving;
		}
	} else if (state == ipcState_recieving) {
		ssize_t numRead;
		
		OscAssert_m(remaining > 0, "No buffer space left.");
		numRead = read(fd, buffer + BUFFER_SIZE - remaining, remaining);
		
		if (numRead > 0) {
			// We got some data.
			pNext += numRead;
			remaining -= numRead;
		} else if (numRead < 0) {
			OscAssert_m(errno == EAGAIN, "Error while reading from the IPC connection: %s", strerror(errno));
		} else {
			*pNext = 0;
			OscCall(processRequest, &pNext, buffer, mainState);
			remaining = strlen(pNext);
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
