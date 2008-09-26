/*! @file cgi_sht.c
 * @brief CGI used for the webinterface of the SHT application.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "inc/oscar.h"
#include "cgi_fcd.h"

#include <time.h>

#define CAPTURE_RAW 0

/*#define DBG_SPAM*/

/*! @brief Main object structure of the CGI. Contains all 'global'
 * variables. */
struct CGI_FCD cgi;

/*! @brief The framework module dependencies of this application. */
struct OSC_DEPENDENCY deps[] = {
	{ "log", OscLogCreate, OscLogDestroy },
	{ "sup", OscSupCreate, OscSupDestroy },
	{ "bmp", OscBmpCreate, OscBmpDestroy },
	{ "cam", OscCamCreate, OscCamDestroy },
	{ "vis", OscVisCreate, OscVisDestroy },
	{ "gpio", OscGpioCreate, OscGpioDestroy }
};

/*! @brief All potential arguments supplied to this CGI. */
struct ARGUMENT args[] =
{
	{ "init", BOOL_ARG, &cgi.args.bInit, &cgi.args.bInit_supplied },
	{ "autoGain", BOOL_ARG, &cgi.args.bAutoGain, &cgi.args.bAutoGain_supplied },
	{ "maxGain", FLOAT_ARG, &cgi.args.maxGain, &cgi.args.bMaxGain_supplied },
	{ "manGain", FLOAT_ARG, &cgi.args.manGain, &cgi.args.bManGain_supplied },
	{ "autoExp", BOOL_ARG, &cgi.args.bAutoExp, &cgi.args.bAutoExp_supplied },
	{ "maxExposure", INT_ARG, &cgi.args.maxExp, &cgi.args.bMaxExp_supplied },
	{ "manExposure", INT_ARG, &cgi.args.manExp, &cgi.args.bManExp_supplied },
	{ "register", INT_ARG, &cgi.args.reg, &cgi.args.bReg_supplied },
	{ "registerValue", INT_ARG, &cgi.args.regValue, &cgi.args.bRegValue_supplied },
	{ "compand12To10", BOOL_ARG, &cgi.args.b12To10BitCompanding, &cgi.args.b12To10BitCompanding_supplied },
	{ "highDynamicRange", BOOL_ARG, &cgi.args.bHighDynamicRange, &cgi.args.bHighDynamicRange_supplied },
	{ "rowWiseNoiseCorr", BOOL_ARG, &cgi.args.bRowWiseNoiseCorr, &cgi.args.bRowWiseNoiseCorr_supplied },
	{ "horizontalFlip", BOOL_ARG, &cgi.args.bHorizontalFlip, &cgi.args.bHorizontalFlip_supplied },
	{ "verticalFlip", BOOL_ARG, &cgi.args.bVerticalFlip, &cgi.args.bVerticalFlip_supplied }
};

/*********************************************//*!
 * @brief Set a bit in a value
 * @param val Value to change.
 * @param bitNr The bit position (counted from LSB)
 * @param bitVal Bit value to set.
 * @return Updated value.
 */
#define SET_BIT(val, bitNr, bitVal) ((val & ~(1 << bitNr)) | (bitVal << bitNr))

/*********************************************//*!
 * @brief Flip a bit in a value
 * @param val Value to change.
 * @param bitNr The bit position (counted from LSB)
 * @return Updated value.
 */
#define FLIP_BIT(val, bitNr) ((val & ~(1 << bitNr)) | ((!(val && (1 << bitNr))) << bitNr))

/*********************************************************************//*!
 * @brief Split the supplied URI string into arguments and parse them.
 * 
 * Matches the argument string with the arguments list (args) and fills in
 * their values. Unknown arguments provoke an error, but missing
 * arguments are just ignored.
 * 
 * @param strSrc The argument string.
 * @param srcLen The length of the argument string.
 * @return SUCCESS or an appropriate error code otherwise
 *//*********************************************************************/
static OSC_ERR CGIParseArguments(const char *strSrc, const int32 srcLen)
{
	unsigned int code, i;
	const char *strSrcLast = &strSrc[srcLen];
	char *strTemp = (char*)cgi.strArgumentsTemp;
	struct ARGUMENT *pArg = NULL;
	
	if (srcLen == 0 || *strSrc == '\0')
	{
		/* Empty string supplied. */
		return SUCCESS;
	}
	
	/* Intialize all arguments as 'not supplied' */
	for (i = 0; i < sizeof(args)/sizeof(struct ARGUMENT); i++)
	{
		*args[i].pbSupplied = FALSE;
	}
	
	for (; (uint32)strSrc < (uint32)strSrcLast; strSrc++)
	{
		if (unlikely(*strSrc == '='))
		{
			/* Argument name parsed, find out which argument and continue to parse its value. */
			*strTemp = '\0';
			pArg = NULL;
			for (i = 0; i < sizeof(args)/sizeof(struct ARGUMENT); i++)
			{
				if (strcmp(args[i].strName, cgi.strArgumentsTemp) == 0)
				{
					/* Found it. */
					pArg = &args[i];
					break;
				}
			}
						
			strTemp = cgi.strArgumentsTemp;
			
			if (unlikely(pArg == NULL))
			{
				OscLog(ERROR, "%s: Unknown argument encountered: \"%s\"\n", __func__, cgi.strArgumentsTemp);
				return -EINVALID_PARAMETER;
			}
		}
		else if (unlikely(*strSrc == '&' || *strSrc == '\0'))
		{
			/* Argument value parsed, convert and read next argument. */
			*strTemp = '\0';
			if (unlikely(pArg == NULL))
			{
				OscLog(ERROR, "%s: Value without type found: \"%s\"\n",
						__func__, strTemp);
				return -EINVALID_PARAMETER;
			}
			
			strTemp = cgi.strArgumentsTemp;
			
			switch (pArg->enType)
			{
			case STRING_ARG:
				strcpy((char*)pArg->pData, strTemp);
				break;
			case INT_ARG:
				if (sscanf(strTemp, "%d", (int*)pArg->pData) != 1)
				{
					OscLog(ERROR, "%s: Unable to parse int value of variable \"%s\" (%s)!\n", __func__, pArg->strName, strTemp);
					return -EINVALID_PARAMETER;
				}
				break;
			case SHORT_ARG:
				if (sscanf(strTemp, "%hd", (short*)pArg->pData) != 1)
				{
					OscLog(ERROR, "%s: Unable to parse short value of variable \"%s\" (%s)!\n", __func__, pArg->strName, strTemp);
					return -EINVALID_PARAMETER;
				}
				break;
			case BOOL_ARG:
				if (strcmp(strTemp, "true") == 0)
				{
					*((bool*)pArg->pData) = TRUE;
				}
				else if (strcmp(strTemp, "false") == 0)
				{
					*((bool*)pArg->pData) = FALSE;
				}
				else
				{
					OscLog(ERROR, "CGI %s: Unable to parse boolean value of variable \"%s\" (%s)!\n", __func__, pArg->strName, strTemp);
					return -EINVALID_PARAMETER;
				}
				break;
			case FLOAT_ARG:
				if (sscanf(strTemp, "%f", (float*)pArg->pData) != 1)
				{
					OscLog(ERROR, "%s: Unable to parse float value of variable \"%s\" (%s)!\n", __func__, pArg->strName, strTemp);
					return -EINVALID_PARAMETER;
				}
				break;
			}
			if (pArg->pbSupplied != NULL)
			{
				*pArg->pbSupplied = TRUE;
			}
			
		}
		else if (unlikely(*strSrc == '+'))
		{
			/* Spaces are encoded as + */
			*strTemp++ = ' ';
		}
		else if (unlikely(*strSrc == '%'))
		{
			/* ASCII Hex codes */
			if (sscanf(strSrc+1, "%2x", &code) != 1)
			{
				/* Unknown code */
				code = '?';
			}
			*strTemp++ = code;
			strSrc +=2;
		}
		else
		{
			*strTemp++ = *strSrc;
		}
	}
	
	return SUCCESS;
}

/*********************************************************************//*!
 * @brief Execution starting point
 * 
 * Handles initialization, control and unloading.
 * @return 0 on success, -1 otherwise
 *//*********************************************************************/
int main()
{
	char *strContentLen;
	int contentLen;
	OSC_ERR err;
	void *pPic = NULL;
	struct OSC_PICTURE picFile;
	uint16 regVal;
	enum EnBayerOrder enBayerOrder;
	
	/* -------------- Initialization ------------------ */
	
	memset(&cgi, 0, sizeof(struct CGI_FCD));
	
	/* Instantiate the framework. */
	err = OscCreate(&cgi.hFramework);
	if (err != SUCCESS)
	{
		printf(APP_NAME ": Unable to create framework!\n");
		return -1;
	}
	
	/* Instantiate all required framework modules. */
	err = OscLoadDependencies(cgi.hFramework, deps, sizeof(deps)/sizeof(struct OSC_DEPENDENCY));
	if (err != SUCCESS)
	{
		fprintf(stderr, "%s: ERROR: Unable to load dependencies! (%d)\n", APP_NAME, err);
		goto dep_err;
	}
	
	OscLogSetConsoleLogLevel(CRITICAL);
	OscLogSetFileLogLevel(DEBUG);
	
	/* Set a frame buffer to capture to. */
	err = OscCamSetFrameBuffer(0, sizeof(cgi.frameBuf), cgi.frameBuf, TRUE);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to set up frame buffer! (%d)\n", APP_NAME, err);
		goto fb_err;
	}
	
	err = OscCamSetAreaOfInterest(0, 0, OSC_CAM_MAX_IMG_WIDTH, OSC_CAM_MAX_IMG_HEIGHT);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to set area of interest! (%d)\n", APP_NAME, err);
	}
#if defined(OSC_HOST) || defined(OSC_SIM)
	/* Create a file reader to load in the test images and apply it to the camera module. */
	err = OscFrdCreateFileListReader(&cgi.hFileNameReader, "hostImgs.txt");
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to open file name reader for file!\n", __func__, cgi.hFileNameReader);
		return err;
	}
	
	err = OscCamSetFileNameReader(cgi.hFileNameReader);
	if (err != SUCCESS)
	{
		OscLog(ERROR, "%s: Unable to set file name reader for camera (%d)!\n", __func__, err);
		return -EDEVICE;
	}
	OscSimInitialize();
#endif /* OSC_HOST or OSC_SIM */
	
	/* ---------------- Parse POST arguments -------------*/
	strContentLen = getenv("CONTENT_LENGTH");
	
	if ((strContentLen == NULL) || (sscanf(strContentLen,"%d",&contentLen) != 1) || contentLen >= MAX_ARGUMENT_STRING_LEN)
	{
		goto exit_unload;
	}
	else
	{
		/* Get the argument string. */
		fgets(cgi.strArgumentsRaw, contentLen + 1, stdin);
#ifdef DBG_SPAM
		OscLog(DEBUG, "CGI: Arguments: \"%s\"\n", cgi.strArgumentsRaw);
#endif
		err = CGIParseArguments(cgi.strArgumentsRaw, contentLen + 1);
		if (err != SUCCESS)
		{
			OscLog(ERROR, "CGI: Error parsing command line arguments! \"%s\"\n", cgi.strArgumentsRaw);
			goto exit_unload;
		}
	}
	
	/* ---------------- Apply arguments -------------*/
	if (cgi.args.bInit_supplied && cgi.args.bInit)
	{
		/* Initialize the camera. */
		
		/* Apply default (max) area-of-interest */
		OscCamSetAreaOfInterest(0,0,0,0);
		/* Set default camera - scene perspective relation */
		OscCamSetupPerspective(OSC_CAM_PERSPECTIVE_DEFAULT);
		/* Turn on auto-exposure and auto-gain by default. */
		OscCamSetRegisterValue(REG_AEC_AGC_ENABLE, 0x3);
		/* Turn on continuous capture for this application. */
		OscCamSetRegisterValue(CAM_REG_CHIP_CONTROL, 0x388);
		/* Set the undocumented reserved almighty Micron register to the
			"optimal" value. */
		OscCamSetRegisterValue(CAM_REG_RESERVED_0x20, 0x3d5);
	}
	
	if (cgi.args.bAutoExp_supplied || cgi.args.bAutoGain_supplied)
	{
		OscCamGetRegisterValue(REG_AEC_AGC_ENABLE, &regVal);
		if (cgi.args.bAutoExp_supplied)
		{
			regVal = SET_BIT(regVal, 0, cgi.args.bAutoExp);
		}
		if (cgi.args.bAutoGain_supplied)
		{
			regVal = SET_BIT(regVal, 1, cgi.args.bAutoGain);
		}
		OscCamSetRegisterValue(REG_AEC_AGC_ENABLE, regVal);
	}
	
	if (cgi.args.bMaxExp_supplied)
	{
		OscCamSetRegisterValue(REG_MAX_EXP, (uint16)((float)cgi.args.maxExp*1000*0.02955f));
	}
	if (cgi.args.bMaxGain_supplied)
	{
		OscCamSetRegisterValue(REG_MAX_GAIN, (uint16)(cgi.args.maxGain*16));
	}
	if (cgi.args.bManExp_supplied)
	{
		OscCamSetRegisterValue(REG_EXPOSURE, (uint16)((float)cgi.args.manExp*1000*0.02955f));
	}
	if (cgi.args.bManGain_supplied)
	{
		OscCamSetRegisterValue(REG_GAIN, (uint16)(cgi.args.manGain*16));
	}
	
	if (cgi.args.bRegValue_supplied && cgi.args.bReg_supplied)
	{
		OscCamSetRegisterValue(cgi.args.reg, cgi.args.regValue);
	}
	
	if (cgi.args.b12To10BitCompanding_supplied)
	{
		if (cgi.args.b12To10BitCompanding)
			OscCamSetRegisterValue(REG_ADC_RESOLUTION_CONTROL, 0x3);
		else
			OscCamSetRegisterValue(REG_ADC_RESOLUTION_CONTROL, 0x2);
	}

	if (cgi.args.bHighDynamicRange_supplied)
	{
		/* Enable/disable Auto-Knee-Point adjustion. */
		OscCamGetRegisterValue(REG_SHUTTER_WIDTH_CONTROL, &regVal);
		regVal = SET_BIT(regVal, 8, cgi.args.bHighDynamicRange);
		OscCamSetRegisterValue(REG_SHUTTER_WIDTH_CONTROL, regVal);
		/* Enable/disable HDR */
		OscCamGetRegisterValue(REG_PIXEL_OP_MODE, &regVal);
		regVal = SET_BIT(regVal, 6, cgi.args.bHighDynamicRange);
		OscCamSetRegisterValue(REG_PIXEL_OP_MODE, regVal);
	}

	if (cgi.args.bRowWiseNoiseCorr_supplied)
	{
		OscCamGetRegisterValue(REG_ROW_NOISE_CORR_CONTROL_1, &regVal);
		regVal = SET_BIT(regVal, 5, cgi.args.bRowWiseNoiseCorr);
		OscCamSetRegisterValue(REG_ROW_NOISE_CORR_CONTROL_1, regVal);
	}
	
	if (cgi.args.bHorizontalFlip_supplied && cgi.args.bVerticalFlip_supplied)
	{
		regVal = cgi.args.bHorizontalFlip << 1 | cgi.args.bVerticalFlip;
		switch (regVal)
		{
		case 0:
			OscCamSetupPerspective(OSC_CAM_PERSPECTIVE_DEFAULT);
			break;
		case 1:
			OscCamSetupPerspective(OSC_CAM_PERSPECTIVE_VERTICAL_MIRROR);
			break;
		case 2:
			OscCamSetupPerspective(OSC_CAM_PERSPECTIVE_HORIZONTAL_MIRROR);
			break;
		case 3:
			OscCamSetupPerspective(OSC_CAM_PERSPECTIVE_180DEG_ROTATE);
			break;
		}
	}
	
#ifdef DBG_SPAM
	OscLog(DEBUG, "Arguments applied!\n");
#endif
	
	/* ---------------- Frame capture -------------*/
	err = OscCamSetupCapture(0);
	if (err != SUCCESS)
	{
		OscLog(ERROR, APP_NAME ": Unable to setup frame! (%d)\n", err);
		goto exit_unload;
	}
	
	err = OscGpioTriggerImage();
	if (err != SUCCESS)
	{
		OscLog(ERROR, APP_NAME ": Unable to trigger frame! (%d)\n", err);
		goto exit_unload;
	}
	
	err = OscCamReadPicture(0, &pPic, 0, 10000);
	if (err != SUCCESS)
	{
		OscLog(ERROR, APP_NAME ": Unable to read picture! (%d)\n", err);
		goto exit_unload;
	}
#ifdef DBG_SPAM
	OscLog(DEBUG, "Image read!\n");
#endif
	if (!CAPTURE_RAW)
	{
		err = OscCamGetBayerOrder(&enBayerOrder, 0, 0);
		if (err != SUCCESS)
		{
			OscLog(ERROR, APP_NAME ": Error getting bayer order! (%d)\n", err);
		}
		
		err = OscVisDebayer(pPic, OSC_CAM_MAX_IMG_WIDTH, OSC_CAM_MAX_IMG_HEIGHT, enBayerOrder, cgi.imgBuf);
		if (err != SUCCESS)
		{
			OscLog(ERROR, APP_NAME ": Error debayering image! (%d)\n", err);
			goto exit_unload;
		}
	}
	/* --------------- Save as file --------------*/
	if (CAPTURE_RAW)
	{
		picFile.data = pPic;
		picFile.type = OSC_PICTURE_GREYSCALE;
	}
	else
	{
		picFile.data = cgi.imgBuf;
		picFile.type = OSC_PICTURE_BGR_24;
	}
	picFile.width = OSC_CAM_MAX_IMG_WIDTH;
	picFile.height = OSC_CAM_MAX_IMG_HEIGHT;
	
	err = OscBmpWrite(&picFile, OUT_IMG_NAME);
	if (err != SUCCESS)
	{
		OscLog(ERROR, APP_NAME ": Unable to write image to file! (%d)\n", err);
		goto exit_unload;
	}

	cgi.timeStamp = OscSupCycGet();
	
#ifdef DBG_SPAM
	OscLog(DEBUG, "Image written!\n");
#endif
	/* ---------------- CGI response --------------*/
	/* Header */
	printf("Content-type: text/html\n\n");
	
	printf("TS=%lu\n", cgi.timeStamp);
	OscCamGetRegisterValue(REG_AGC_GAIN_OUTPUT, &regVal);
	printf("curGainFactor=%f\n", ((float)regVal)*0.0625f);
	OscCamGetRegisterValue(REG_AEC_EXP_OUTPUT, &regVal);
	printf("curExpTime=%hu\n", (34*regVal)/1000);
	if (cgi.args.bReg_supplied)
	{
		OscCamGetRegisterValue(cgi.args.reg, &regVal);
		printf("curRegisterValue=%hu\n", regVal);
	}
#ifdef DBG_SPAM
	OscLog(DEBUG, "Closing...!\n");
#endif
	/* ----------------- Unload -------------------*/
exit_unload:
fb_err:
	OscUnloadDependencies(cgi.hFramework, deps, sizeof(deps)/sizeof(struct OSC_DEPENDENCY));
dep_err:
	OscDestroy(cgi.hFramework);
	return 0;
}
