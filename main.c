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
 * @brief Main file of the template application. Mainly contains initialization
 * code.
 */

#include <string.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "mainstate.h"

#define TEST_IMAGE_FN "test.bmp"

/*********************************************************************//*!
 * @brief Initialize everything so the application is fully operable
 * after a call to this function.
 * 
 * @return SUCCESS or an appropriate error code.
 *//*********************************************************************/
OscFunction(static mainFunction)
	uint8 multiBufferIds[2] = {0, 1};
	uint8_t frameBuffers[2][OSC_CAM_MAX_IMAGE_HEIGHT * OSC_CAM_MAX_IMAGE_WIDTH];
	
	/******* Create the framework **********/
	OscCall(OscCreate, &OscModule_log, &OscModule_sup, &OscModule_bmp, &OscModule_cam, &OscModule_hsm, &OscModule_vis, &OscModule_gpio);
	
	OscCall(OscLogSetConsoleLogLevel, INFO);
	OscCall(OscLogSetFileLogLevel, WARN);

#if defined(OSC_HOST) || defined(OSC_SIM)
	{
		void * hFileNameReader;
		
		OscCall(OscFrdCreateConstantReader, &hFileNameReader, TEST_IMAGE_FN);
		OscCall(OscCamSetFileNameReader, hFileNameReader);
	}
#endif /* OSC_HOST or OSC_SIM */
	
	/* Set the camera registers to sane default values. */
	OscCall(OscCamPresetRegs);
	OscCall(OscCamSetAreaOfInterest, 0, 0, OSC_CAM_MAX_IMAGE_WIDTH, OSC_CAM_MAX_IMAGE_HEIGHT);
	
	/* Set up two frame buffers with enough space for the maximum
	 * camera resolution in cached memory. */
	OscCall(OscCamSetFrameBuffer, 0, sizeof frameBuffers[0], frameBuffers[0], true);
	OscCall(OscCamSetFrameBuffer, 1, sizeof frameBuffers[1], frameBuffers[1], true);
	
	/* Create a double-buffer from the frame buffers initilalized above.*/
	OscCall(OscCamCreateMultiBuffer, 2, multiBufferIds);
	OscCall(OscCamSetupPerspective, OSC_CAM_PERSPECTIVE_DEFAULT);
	
	OscCall(stateControl);
	
OscFunctionFail()
	OscDestroy();
OscFunctionEnd()

/*********************************************************************//*!
 * @brief Program entry
 * 
 * @param argc Command line argument count.
 * @param argv Command line argument strings.
 * @return 0 on success
 *//*********************************************************************/
int main(int argc, char ** argv) {
	if (mainFunction() == SUCCESS)
		return 0;
	else
		return 1;
}
