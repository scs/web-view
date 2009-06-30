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

enum ImageType {
	ImageType_none, // No valid image yet
	ImageType_gray, // Image from a grayscale sensor
	ImageType_raw, // Raw image from a color sensor
	ImageType_debayered // Debayered image from a color sensor
};

struct IpcImageInfo {
	uint8_t data[3 * OSC_CAM_MAX_IMAGE_WIDTH * OSC_CAM_MAX_IMAGE_HEIGHT];
	int width, height;
	enum ImageType type;
};

enum MainStateEvent {
	MainStateEvent_CamNewImage,
	MainStateEvent_IpcSetRawCapture,
	MainStateEvent_IpcSetColorCapture
};

struct MainState {
	Hsm super;
	State cameraGray, cameraColor, cameraColor_captureRaw, cameraColor_captureDebayered;
	uint8_t * pCurrentImage;
	struct IpcImageInfo imageInfo;
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
