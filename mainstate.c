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

const Msg mainStateMsg[] = {
	{ MainStateEvent_CamNewImage },
	{ MainStateEvent_IpcSetRawCapture },
	{ MainStateEvent_IpcSetColorCapture }
};

/*********************************************************************//*!
 * @brief Inline function to throw an event to be handled by the statemachine.
 * 
 * @param pHsm Pointer to state machine
 * @param evt Event to be thrown.
 *//*********************************************************************/
#define ThrowEvent(mainState, event) \
	HsmOnEvent(&(mainState)->super, &mainStateMsg[event])

Msg const * mainState_top(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == START_EVT) {
		STATE_START(mainState, &mainState->cameraGray);
		return NULL;
	}
/*	if (msg == mainStateMsg[])
	switch (msg->evt)
	{
	case START_EVT:
		STATE_START(me, &me->captureRaw);
		return 0;
	case IPC_GET_COLOR_IMG_EVT:
	case IPC_GET_RAW_IMG_EVT:
	case IPC_GET_APP_STATE_EVT:
	case IPC_SET_CAPTURE_MODE_EVT:
		// If the IPC event is not handled in the actual substate, a negative acknowledge is returned by default.
		data.ipc.enReqState = REQ_STATE_NACK_PENDING;
	return 0;
	}
	return msg;
*/
	return msg;
}

Msg const * mainState_cameraGray(Hsm * hsm, Msg const * msg) {
	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	if (msg->evt == MainStateEvent_CamNewImage) {
		mainState->imageInfo.width = OSC_CAM_MAX_IMAGE_WIDTH;
		mainState->imageInfo.height = OSC_CAM_MAX_IMAGE_HEIGHT;
		memcpy(mainState->imageInfo.data, mainState->pCurrentImage, mainState->imageInfo.width * mainState->imageInfo.height);
		mainState->imageInfo.type = ImageType_gray;
		
		return 0;
	}
	
	return msg;
}

Msg const * mainState_cameraColor(Hsm * hsm, Msg const * msg) {
//	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	return msg;
}

Msg const * mainState_cameraColor_captureRaw(Hsm * hsm, Msg const * msg) {
//	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	return msg;
}

Msg const * mainState_cameraColor_captureDebayered(Hsm * hsm, Msg const * msg) {
//	struct MainState * mainState = containerOf(hsm, struct MainState, super);
	
	return msg;
}

#if 0
Msg const * mainState_CaptureColor(struct MainState * me, Msg const * msg)
{
	struct APPLICATION_STATE *pState;
	bool bCaptureColor;
	
	switch (msg->evt)
	{
	case ENTRY_EVT:
		data.ipc.state.enAppMode = APP_CAPTURE_COLOR;
		data.pCurRawImg = data.u8FrameBuffers[0];
		return 0;
	case FRAMESEQ_EVT:
		/* Sleep here for a short while in order not to violate the vertical
		 * blank time of the camera sensor when triggering a new image
		 * right after receiving the old one. This can be removed if some
		 * heavy calculations are done here. */
		usleep(1000);
		return 0;
	case FRAMEPAR_EVT:
		/* Process the image. */
		ProcessFrame(data.pCurRawImg);
		
		/* Timestamp the capture of the image. */
		data.ipc.state.imageTimeStamp = OscSupCycGet();
		data.ipc.state.bNewImageReady = TRUE;
		return 0;
	case IPC_GET_APP_STATE_EVT:
		/* Fill in the response and schedule an acknowledge for the request. */
		pState = (struct APPLICATION_STATE*)data.ipc.req.pAddr;
		memcpy(pState, &data.ipc.state, sizeof(struct APPLICATION_STATE));
		
		data.ipc.enReqState = REQ_STATE_ACK_PENDING;
		return 0;
	case IPC_GET_COLOR_IMG_EVT:
		/* Write out the image to the address space of the CGI. */
		memcpy(data.ipc.req.pAddr, data.u8ResultImage, sizeof(data.u8ResultImage));
		
		data.ipc.state.bNewImageReady = FALSE;
		
		/* Mark the request as executed, so it will be acknowledged later. */
		data.ipc.enReqState = REQ_STATE_ACK_PENDING;
		return 0;
	case IPC_SET_CAPTURE_MODE_EVT:
		/* Read the option from the address space of the CGI. */
		bCaptureColor = *((bool*)data.ipc.req.pAddr);
		if (bCaptureColor == FALSE)
		{
			/* Need to capture raw images from now on, this is done in the captureRaw state.  */
			STATE_TRAN(me, &me->captureRaw);
		}
		data.ipc.enReqState = REQ_STATE_ACK_PENDING;
		return 0;
	}
	return msg;
}

Msg const * mainState_CaptureRaw(struct MainState * me, Msg const * msg)
{
	struct APPLICATION_STATE *pState;
	bool bCaptureColor;
	
	switch (msg->evt)
	{
	case ENTRY_EVT:
		data.ipc.state.enAppMode = APP_CAPTURE_RAW;
		data.pCurRawImg = data.u8FrameBuffers[0];
		return 0;
	case FRAMESEQ_EVT:
		/* Timestamp the capture of the image. */
		data.ipc.state.imageTimeStamp = OscSupCycGet();
		data.ipc.state.bNewImageReady = TRUE;
		
		/* Sleep here for a short while in order not to violate the vertical
		 * blank time of the camera sensor when triggering a new image
		 * right after receiving the old one. This can be removed if some
		 * heavy calculations are done here. */
		usleep(1000);
		
		return 0;
	case FRAMEPAR_EVT:
		return 0;
	case IPC_GET_APP_STATE_EVT:
		/* Fill in the response and schedule an acknowledge for the request. */
		pState = (struct APPLICATION_STATE*)data.ipc.req.pAddr;
		memcpy(pState, &data.ipc.state, sizeof(struct APPLICATION_STATE));
		
		data.ipc.enReqState = REQ_STATE_ACK_PENDING;
		return 0;
	case IPC_GET_RAW_IMG_EVT:
		/* Write out the raw image to the address space of the CGI. */
		memcpy(data.ipc.req.pAddr, data.pCurRawImg, OSC_CAM_MAX_IMAGE_WIDTH*OSC_CAM_MAX_IMAGE_HEIGHT);
		
		data.ipc.state.bNewImageReady = FALSE;
		
		/* Mark the request as executed, so it will be acknowledged later. */
		data.ipc.enReqState = REQ_STATE_ACK_PENDING;
		return 0;
	case IPC_SET_CAPTURE_MODE_EVT:
		/* Read the option from the address space of the CGI. */
		bCaptureColor = *((bool*)data.ipc.req.pAddr);
		if (bCaptureColor == TRUE)
		{
			/* Need to capture colored images from now on, this is done in the captureRaw state.  */
			STATE_TRAN(me, &me->captureColor);
		}
		data.ipc.enReqState = REQ_STATE_ACK_PENDING;
		return 0;
	}
	return msg;
}
#endif

OscFunction(static mainStateInit, struct MainState * me)
	HsmCtor(&me->super, "mainState", mainState_top);
	StateCtor(&me->cameraGray, "cameraGray", &me->super.top, mainState_cameraGray);
	StateCtor(&me->cameraColor, "cameraColor", &me->super.top, mainState_cameraColor);
	StateCtor(&me->cameraColor_captureRaw, "cameraColor_captureRaw", &me->super.top, mainState_cameraColor_captureRaw);
	StateCtor(&me->cameraColor_captureDebayered, "cameraColor_captureDebayered", &me->super.top, mainState_cameraColor_captureDebayered);
	
	HsmOnStart(&me->super);
OscFunctionEnd()

OscFunction(stateControl)
	struct MainState mainState = { };
	
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
			usleep(500000);
			
			OscCall(OscCamReadPicture, OSC_CAM_MULTI_BUFFER, &mainState.pCurrentImage, 0, 1);
			
			if (OscLastStatus() == SUCCESS)
				break;
			
			/*----------- procress CGI request
			 * Check for CGI request only if ReadPicture generated a
			 * time out. Process request directely or involve state
			 * engine with event */
		}

		// process frame by state engine (post-setup) Parallel with next capture
		ThrowEvent(&mainState, MainStateEvent_CamNewImage);
		
		/* Advance the simulation step counter. */
		OscSimStep();
	}
OscFunctionEnd()
