/* Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty. This file is offered as-is,
 * without any warranty.
 */

/*! @file mainstate.h
 * @brief Definitions for main state machine
	************************************************************************/
	
#ifndef MAINSTATE_H
#define MAINSTATE_H

#include "oscar.h"

/*!
 * @brief Macro to throw an event to be handled by the statemachine.
 * 
 * @param mainState Pointer to state machine.
 * @param event Event to be thrown.
 */
#define throwEvent(mainState, event) \
	HsmOnEvent(&(mainState)->super, &mainStateMsg[event])

enum ColorType {
	ColorType_none, // No valid image yet
	ColorType_gray, // Image from a grayscale sensor
	ColorType_raw, // Raw image from a color sensor
	ColorType_debayered // Debayered image from a color sensor
};

enum MainStateEvent {
	MainStateEvent_camNewImage,
	MainStateEvent_ipcSetOptions
};

static const Msg mainStateMsg[] = {
	{ MainStateEvent_camNewImage },
	{ MainStateEvent_ipcSetOptions }
};

struct MainState {
	Hsm super;
	State cameraGray, cameraColor, cameraColor_captureRaw, cameraColor_captureDebayered;
	uint8_t * pCurrentImage;
	struct {
		uint8_t data[3 * OSC_CAM_MAX_IMAGE_WIDTH * OSC_CAM_MAX_IMAGE_HEIGHT];
		int width, height;
		int exposureTime;
		enum ColorType colorType;
	} imageInfo;
	struct {
		int exposureTime;
		enum ColorType colorType;
	} options;
};

/*********************************************************************//*!
 * @brief Give control to statemachine.
 * 
 * @return SUCCESS or an appropriate error code otherwise
 * 
 * The function does never return normally except in error case.
 *//*********************************************************************/
OscFunctionDeclare(stateControl);

#endif /*MAINSTATE_H*/
