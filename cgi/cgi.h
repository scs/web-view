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

/*! @file cgi.h
 * @brief Header file of the CGI used for the webinterface of the template application.
 */

#ifndef CGI_H_
#define CGI_H_

#define CGI_SOCKET_PATH "/tmp/oscar-ipc.sock"

struct cgiBuffer {
	size_t length;
	char buffer[1024];
};

/* @brief The different data types of the argument string. */

#endif /*CGI_TEMPLATE_H_*/
