#ifndef IPC_H_
#define IPC_H_

#include "mainstate.h"
#include <sys/stat.h>
#include <sys/types.h>

/*! @brief File permissions of the server socket file node. */
#define SERV_SOCKET_PERMISSIONS     \
	(S_IXUSR | S_IRUSR | S_IWUSR |  \
		S_IXGRP | S_IRGRP | S_IWGRP |  \
		S_IXOTH | S_IROTH | S_IWOTH)

OscFunctionDeclare(handleIpcRequests, struct MainState * pMainState)

#endif // #ifndef IPC_H_
