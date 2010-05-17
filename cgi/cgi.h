/* Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty. This file is offered as-is,
 * without any warranty.
 */

/*! @file template_ipc.h
 * @brief Shared header file between application and its
 * CGI. Contains all information relevant to IPC between these two.
 */

#ifndef CGI_H_
#define CGI_H_

/*! @brief The path of the unix domain socket used for IPC between the application and its user interface. */
#define CGI_SOCKET_PATH "/tmp/IPCSocket."APP_NAME".sock"

/* The parameter IDs to identify the different requests/responses. */
enum ipcParamIds {
	ipcParamIds_putRequest,
	ipcParamIds_getResponse
};

struct cgiBuffer {
	size_t length;
	char data[1024];
};

#endif // #ifndef CGI_H_
