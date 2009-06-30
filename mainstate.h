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
