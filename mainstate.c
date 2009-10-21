/* Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty. This file is offered as-is,
 * without any warranty.
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
		struct OscSystemInfo * pInfo;
		
		OscCfgGetSystemInfo(&pInfo);
		 
		if (pInfo->hardware.imageSensor.hasBayernPattern)
			STATE_START(mainState, &mainState->cameraColor_captureRaw);
		else
			STATE_START(mainState, &mainState->cameraGray);
		
		return NULL;
	} else if (msg->evt == MainStateEvent_ipcSetOptions) {
		OscCamSetShutterWidth(mainState->options.exposureTime * 1000);
		
		return NULL;
	}

	return msg;
}

Msg const * mainState_cameraGray(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == MainStateEvent_camNewImage) {
		mainState->imageInfo.colorType = ColorType_gray;
		
		memcpy(mainState->imageInfo.data, mainState->pCurrentImage, mainState->imageInfo.width * mainState->imageInfo.height);
		
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
		mainState->imageInfo.colorType = ColorType_raw;
		
		memcpy(mainState->imageInfo.data, mainState->pCurrentImage, mainState->imageInfo.width * mainState->imageInfo.height);
		
		return NULL;
	}
	
	return msg;
}

Msg const * mainState_cameraColor_captureDebayered(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == MainStateEvent_camNewImage) {
		mainState->imageInfo.colorType = ColorType_debayered;
		
		OscVisDebayer(mainState->pCurrentImage, mainState->imageInfo.width, mainState->imageInfo.height, 0, mainState->imageInfo.data);
		
		return NULL;
	}
	
	return msg;
}

OscFunction(static mainStateInit, struct MainState * me)
	HsmCtor(&me->super, "mainState", mainState_top);
	StateCtor(&me->cameraGray, "cameraGray", &me->super.top, mainState_cameraGray);
	StateCtor(&me->cameraColor, "cameraColor", &me->super.top, mainState_cameraColor);
	StateCtor(&me->cameraColor_captureRaw, "cameraColor_captureRaw", &me->cameraColor, mainState_cameraColor_captureRaw);
	StateCtor(&me->cameraColor_captureDebayered, "cameraColor_captureDebayered", &me->cameraColor, mainState_cameraColor_captureDebayered);
	
	HsmOnStart(&me->super);
OscFunctionEnd()

OscFunction(stateControl)
	static struct MainState mainState;
	
//	mainState = (struct MainState) { }; 
	memset(&mainState, 0, sizeof mainState); // The variant above overwrites memory beyond the structure. Bug in GCC?
	
	// Setup main state machine
	OscCall(mainStateInit, &mainState);
	OscCall(OscSimInitialize);
	// infinite main loop
	loop {
		// prepare next capture
		OscCall(OscCamSetupCapture, OSC_CAM_MULTI_BUFFER);
		OscCall(OscGpioTriggerImage);
		
		// wait for captured picture
		loop {
			// procress CGI request
			OscCall(handleIpcRequests, &mainState);
			
			OscCall(OscCamReadPicture, OSC_CAM_MULTI_BUFFER, &mainState.pCurrentImage, 0, 1);
			
			if (OscLastStatus() == SUCCESS)
				break;
		}
		
		// Build up image information sent to the web interface along the image.
		{
			uint32_t exposureTime;
			
			OscCall(OscCamGetShutterWidth, &exposureTime);
			mainState.imageInfo.exposureTime = (exposureTime + 500) / 1000;
			
			mainState.imageInfo.width = OSC_CAM_MAX_IMAGE_WIDTH;
			mainState.imageInfo.height = OSC_CAM_MAX_IMAGE_HEIGHT;
		}
		
		// process frame by state engine (post-setup) Parallel with next capture
		throwEvent(&mainState, MainStateEvent_camNewImage);
		
		/* Advance the simulation step counter. */
		OscSimStep();
	}
OscFunctionEnd()
