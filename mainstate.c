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

/*! @file mainstate.c
 * @brief Main State machine for template application.
 * 
 * Makes use of Framework HSM module.
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "oscar.h"
#include "mainstate.h"
#include "ipc.h"

#define CGI_IMAGE_PATH

Msg const * mainState_top(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == START_EVT) {
		STATE_START(mainState, &mainState->cameraColor_captureDebayered);
		
		return NULL;
	} else if (msg->evt == MainStateEvent_ipcSetOptions) {
		OscMark_m("%d", mainState->options.exposureTime);
		OscCamSetShutterWidth(mainState->options.exposureTime * 1000);
		
		return NULL;
	}

	return msg;
}

Msg const * mainState_cameraGray(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == MainStateEvent_camNewImage) {
		mainState->imageInfo.width = OSC_CAM_MAX_IMAGE_WIDTH;
		mainState->imageInfo.height = OSC_CAM_MAX_IMAGE_HEIGHT;
		memcpy(mainState->imageInfo.data, mainState->pCurrentImage, mainState->imageInfo.width * mainState->imageInfo.height);
		mainState->imageInfo.colorType = ColorType_gray;
		
		return NULL;
	}
	
	return msg;
}

Msg const * mainState_cameraColor(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == MainStateEvent_ipcSetOptions) {
		if (mainState->options.colorType == ColorType_raw) {
			STATE_TRAN(mainState, &mainState->cameraColor_captureRaw);
		} else if (mainState->options.colorType == ColorType_debayered) {
			STATE_TRAN(mainState, &mainState->cameraColor_captureDebayered);
		}
		
		return msg;
	}
		
	return msg;
}

Msg const * mainState_cameraColor_captureRaw(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == MainStateEvent_camNewImage) {
		mainState->imageInfo.width = OSC_CAM_MAX_IMAGE_WIDTH;
		mainState->imageInfo.height = OSC_CAM_MAX_IMAGE_HEIGHT;
		memcpy(mainState->imageInfo.data, mainState->pCurrentImage, mainState->imageInfo.width * mainState->imageInfo.height);
		mainState->imageInfo.colorType = ColorType_gray;
		
		return NULL;
	}
	
	return msg;
}

Msg const * mainState_cameraColor_captureDebayered(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == MainStateEvent_camNewImage) {
		mainState->imageInfo.width = OSC_CAM_MAX_IMAGE_WIDTH;
		mainState->imageInfo.height = OSC_CAM_MAX_IMAGE_HEIGHT;
		OscVisDebayer(mainState->pCurrentImage, mainState->imageInfo.width, mainState->imageInfo.height, 0, mainState->imageInfo.data);
		mainState->imageInfo.colorType = ColorType_debayered;
		
		return NULL;
	}
	
	return msg;
}

OscFunction(static mainStateInit, struct MainState * me)
	HsmCtor(&me->super, "mainState", mainState_top);
	StateCtor(&me->cameraGray, "cameraGray", &me->super.top, mainState_cameraGray);
	StateCtor(&me->cameraColor, "cameraColor", &me->super.top, mainState_cameraColor);
	StateCtor(&me->cameraColor_captureRaw, "cameraColor_captureRaw", &me->super.top, mainState_cameraColor_captureRaw);
	StateCtor(&me->cameraColor_captureDebayered, "cameraColor_captureDebayered", &me->super.top, mainState_cameraColor_captureDebayered);
	
	HsmOnStart(&me->super);
OscFunctionEnd()

OscFunction(stateControl)
	static struct MainState mainState;
	
//	mainState = (struct MainState) { }; 
	memset(&mainState, 0, sizeof mainState); // The variant above overwrites memory beyond the structure.
	
	/* Setup main state machine */
	OscCall(mainStateInit, &mainState);
	OscCall(OscSimInitialize);
	/*----------- infinite main loop */
	loop {
		// prepare next capture
		OscCall(OscCamSetupCapture, OSC_CAM_MULTI_BUFFER);
		OscCall(OscGpioTriggerImage);
		
		/*----------- wait for captured picture */
		loop {
			OscCall(handleIpcRequests, &mainState);
			
			OscCall(OscCamReadPicture, OSC_CAM_MULTI_BUFFER, &mainState.pCurrentImage, 0, 1);
			
			if (OscLastStatus() == SUCCESS)
				break;
			
			/*----------- procress CGI request
			 * Check for CGI request only if ReadPicture generated a
			 * time out. Process request directely or involve state
			 * engine with event */
		}

		// process frame by state engine (post-setup) Parallel with next capture
		throwEvent(&mainState, MainStateEvent_camNewImage);
		
		/* Advance the simulation step counter. */
		OscSimStep();
	}
OscFunctionEnd()
