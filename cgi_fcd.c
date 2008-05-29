/*! @file cgi_sht.c
 * @brief CGI used for the webinterface of the SHT application.
 * 
 * @author Markus Berner
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "inc/framework.h"
#include "cgi_fcd.h"

#include <time.h>

#define CAPTURE_RAW 0

/*#define DBG_SPAM*/

/*! @brief Main object structure of the CGI. Contains all 'global'
 * variables. */
struct CGI_FCD cgi;

/*! @brief The framework module dependencies of this application. */
struct LCV_DEPENDENCY deps[] = {
        {"log", LCVLogCreate, LCVLogDestroy},
        {"sup", LCVSupCreate, LCVSupDestroy},
        {"bmp", LCVBmpCreate, LCVBmpDestroy},
        {"cam", LCVCamCreate, LCVCamDestroy},
	{"vis", LCVVisCreate, LCVVisDestroy}
};

/*! @brief All potential arguments supplied to this CGI. */
struct ARGUMENT args[] = 
{
  {"init", BOOL_ARG, &cgi.args.bInit, &cgi.args.bInit_supplied},
  {"autoGain", BOOL_ARG, &cgi.args.bAutoGain, &cgi.args.bAutoGain_supplied},
  {"maxGain", FLOAT_ARG, &cgi.args.maxGain, &cgi.args.bMaxGain_supplied},
  {"manGain", FLOAT_ARG, &cgi.args.manGain, &cgi.args.bManGain_supplied},
  {"autoExp", BOOL_ARG, &cgi.args.bAutoExp, &cgi.args.bAutoExp_supplied},
  {"maxExposure", INT_ARG, &cgi.args.maxExp, &cgi.args.bMaxExp_supplied},
  {"manExposure", INT_ARG, &cgi.args.manExp, &cgi.args.bManExp_supplied},
  {"compand12To10", BOOL_ARG, &cgi.args.b12To10BitCompanding, &cgi.args.b12To10BitCompanding_supplied},
  {"highDynamicRange", BOOL_ARG, &cgi.args.bHighDynamicRange, &cgi.args.bHighDynamicRange_supplied},
  {"rowWiseNoiseCorr", BOOL_ARG, &cgi.args.bRowWiseNoiseCorr, &cgi.args.bRowWiseNoiseCorr_supplied}
};

/*! @brief Set a bit in a value
 * @param val Value to change.
 * @param bitNr The bit position (counted from LSB)
 * @param bitVal Bit value to set.
 * @return Updated value.
*/
#define SET_BIT(val, bitNr, bitVal) ((val & ~(1 << bitNr)) | (bitVal << bitNr))

static LCV_ERR CGIParseArguments(const char *strSrc, const int32 srcLen)
{
	unsigned int		code, i;
	const char 		*strSrcLast = &strSrc[srcLen];
	char			*strTemp = (char*)cgi.strArgumentsTemp;
	struct ARGUMENT		*pArg = NULL;
	
	if(srcLen == 0 || *strSrc == '\0')
	{
	    /* Empty string supplied. */
	    return SUCCESS;
	}
	
	/* Intialize all arguments as 'not supplied' */
	for(i = 0; i < sizeof(args)/sizeof(struct ARGUMENT); i++)
	{
	    *args[i].pbSupplied = FALSE;
	}
	
	for(; (uint32)strSrc < (uint32)strSrcLast; strSrc++)
	{
		if(unlikely(*strSrc == '='))
		{
			/* Argument name parsed, find out which argument and continue
			 * to parse its value. */
			*strTemp = '\0';
			pArg = NULL;
			for(i = 0; i < sizeof(args)/sizeof(struct ARGUMENT); i++)
			{
				if(strcmp(args[i].strName, cgi.strArgumentsTemp) == 0)
				{
					/* Found it. */
					pArg = &args[i];
					break;
				}
			}
						
			strTemp = cgi.strArgumentsTemp;
			
			if(unlikely(pArg == NULL))
			{
				LCVLog(ERROR, "%s: Unknown argument encountered: \"%s\"\n",
						__func__, cgi.strArgumentsTemp);
				return -EINVALID_PARAMETER;
			}
		}
		else if(unlikely(*strSrc == '&' || *strSrc == '\0'))
		{
			/* Argument value parsed, convert and read next argument. */
			*strTemp = '\0';
			if(unlikely(pArg == NULL))
			{
				LCVLog(ERROR, "%s: Value without type found: \"%s\"\n",
						__func__, strTemp);
				return -EINVALID_PARAMETER;
			}
			
			strTemp = cgi.strArgumentsTemp;
			
			switch(pArg->enType)
			{
			case STRING_ARG:
				strcpy((char*)pArg->pData, strTemp);
				break;
			case INT_ARG:
				if(sscanf(strTemp, "%d", (int*)pArg->pData) != 1)
				{
					LCVLog(ERROR, "%s: Unable to parse int value of "
							"variable \"%s\" (%s)!\n",
							__func__, pArg->strName, strTemp);
					return -EINVALID_PARAMETER;
				}
				break;
			case SHORT_ARG:
			    if(sscanf(strTemp, "%hd", (short*)pArg->pData) != 1)
			    {
			        LCVLog(ERROR, "%s: Unable to parse short value of "
			                "variable \"%s\" (%s)!\n",
			                __func__, pArg->strName, strTemp);
			        return -EINVALID_PARAMETER;
			    }
			    break;
			case BOOL_ARG:
			    if(strcmp(strTemp, "true") == 0)
			    {
			        *((bool*)pArg->pData) = TRUE;
			    } 
			    else if(strcmp(strTemp, "false") == 0)
			    {
			        *((bool*)pArg->pData) = FALSE;
			    } else {
			        LCVLog(ERROR, "CGI %s: Unable to parse boolean value"
			                "of variable \"%s\" (%s)!\n",
			                __func__,
			                pArg->strName,
			                strTemp);
			        return -EINVALID_PARAMETER;
			    }
			    break;
			case FLOAT_ARG:
			  if(sscanf(strTemp, "%f", (float*)pArg->pData) != 1)
			    {
			      LCVLog(ERROR, "%s: Unable to parse float value of "
				     "variable \"%s\" (%s)!\n",
				     __func__, pArg->strName, strTemp);
			      return -EINVALID_PARAMETER;
			    }
			  break;
			}
			if(pArg->pbSupplied != NULL)
			{
			    *pArg->pbSupplied = TRUE;
			}
			
		}		
		else if(unlikely(*strSrc == '+'))
		{
			/* Spaces are encoded as + */
			*strTemp++ = ' ';
		}
		else if(unlikely(*strSrc == '%')) 
		{
			/* ASCII Hex codes */
			if(sscanf(strSrc+1, "%2x", &code) != 1) 
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

int main()
{
  char *strContentLen;
  int contentLen;
  LCV_ERR err;
  void *pPic = NULL;
  struct LCV_PICTURE picFile;
  uint16 regVal;

  /* -------------- Initialization ------------------ */

  memset(&cgi, 0, sizeof(struct CGI_FCD));

  /* Instantiate the framework. */
  err = LCVCreate(&cgi.hFramework);
  if(err != SUCCESS)
    {
      printf(APP_NAME ": Unable to create framework!\n");
      return -1;
    }

  /* Instantiate all required framework modules. */
  err = LCVLoadDependencies(cgi.hFramework, 
			    deps, 
			    sizeof(deps)/sizeof(struct LCV_DEPENDENCY));
  if(err != SUCCESS)
    {
      fprintf(stderr, "%s: ERROR: Unable to load dependencies! (%d)\n",
	      APP_NAME, 
	      err);
      goto dep_err;
    } 

  LCVLogSetConsoleLogLevel(CRITICAL);
  LCVLogSetFileLogLevel(DEBUG);

  /* Set a frame buffer to capture to. */
  err = LCVCamSetFrameBuffer(0, sizeof(cgi.frameBuf), cgi.frameBuf, TRUE);
    if(err != SUCCESS)
    {
        LCVLog(ERROR, "%s: Unable to set up frame buffer! (%d)\n",
                APP_NAME, err);
        goto fb_err;
    }

#if defined(LCV_HOST) || defined(LCV_SIM)
    /* Create a file reader to load in the test images and
     * apply it to the camera module. */
    err = LCVFrdCreateFileListReader(&cgi.hFileNameReader,
				     "hostImgs.txt");
    if(err != SUCCESS)
      {
	LCVLog(ERROR, "%s: Unable to open file name reader for file!\n", 
	       __func__, 
	       cgi.hFileNameReader);
	return err;
      }

    err = LCVCamSetFileNameReader(cgi.hFileNameReader);
    if(err != SUCCESS)
      {
	LCVLog(ERROR, "%s: Unable to set file name reader for "
	       "camera (%d)!\n", __func__, err);
	return -EDEVICE;
      }
    LCVSimInitialize();
#endif /* LCV_HOST or LCV_SIM */

    /* ---------------- Parse POST arguments -------------*/
    strContentLen = getenv("CONTENT_LENGTH");
    
    if((strContentLen == NULL) || 
       (sscanf(strContentLen,"%d",&contentLen) != 1) ||
       contentLen >= MAX_ARGUMENT_STRING_LEN)
      {
	goto exit_unload;
      } else {
	/* Get the argument string. */
	fgets(cgi.strArgumentsRaw, contentLen + 1, stdin);
#ifdef DBG_SPAM
	LCVLog(DEBUG, "CGI: Arguments: \"%s\"\n", cgi.strArgumentsRaw);
#endif
	err = CGIParseArguments(cgi.strArgumentsRaw, contentLen + 1);
	if(err != SUCCESS)
		 {
		   LCVLog(ERROR, "CGI: Error parsing command line arguments! "
			  "\"%s\"\n", cgi.strArgumentsRaw);
		   goto exit_unload;
		 }
      }

     ---------------- Apply arguments -------------*/
      if(cgi.args.bInit_supplied && cgi.args.bInit)
      {
	/* Initialize the camera. */

	/* Apply default (max) area-of-interest */
	LCVCamSetAreaOfInterest(0,0,0,0);
	/* Set default camera - scene perspective relation */
	LCVCamSetupPerspective(LCV_CAM_PERSPECTIVE_DEFAULT);
	/* Turn on auto-exposure and auto-gain by default. */
	LCVCamSetRegisterValue(REG_AEC_AGC_ENABLE, 0x3);
	/* Turn on continuous capture for this application. */
	LCVCamSetRegisterValue(CAM_REG_CHIP_CONTROL, 0x388);
	/* Set the undocumented reserved almighty Micron register to the
	   "optimal" value. */
	LCVCamSetRegisterValue(CAM_REG_RESERVED_0x20, 0x3d5);
      }


      if(cgi.args.bAutoExp_supplied || cgi.args.bAutoGain_supplied)
      {
	LCVCamGetRegisterValue(REG_AEC_AGC_ENABLE, &regVal);
	if(cgi.args.bAutoExp_supplied)
	  {
	    regVal = SET_BIT(regVal, 0, cgi.args.bAutoExp);
	  }
	if(cgi.args.bAutoGain_supplied)
	  {
	    regVal = SET_BIT(regVal, 1, cgi.args.bAutoGain);
	  }
	LCVCamSetRegisterValue(REG_AEC_AGC_ENABLE, regVal);
      }

    if(cgi.args.bMaxExp_supplied)
      {
	LCVCamSetRegisterValue(REG_MAX_EXP, (uint16)((float)cgi.args.maxExp*1000*0.02955f));
      }
    if(cgi.args.bMaxGain_supplied)
      {
	LCVCamSetRegisterValue(REG_MAX_GAIN, (uint16)(cgi.args.maxGain*16));
      }
    if(cgi.args.bManExp_supplied)
      {
	LCVCamSetRegisterValue(REG_EXPOSURE, (uint16)((float)cgi.args.manExp*1000*0.02955f));
      }
    if(cgi.args.bManGain_supplied)
      {
	LCVCamSetRegisterValue(REG_GAIN, (uint16)(cgi.args.manGain*16));
      }

    if(cgi.args.b12To10BitCompanding_supplied)
      {
	if(cgi.args.b12To10BitCompanding)
	  LCVCamSetRegisterValue(REG_ADC_RESOLUTION_CONTROL, 0x3);
	else
	  LCVCamSetRegisterValue(REG_ADC_RESOLUTION_CONTROL, 0x2);
      }

    if(cgi.args.bHighDynamicRange_supplied)
      {
	/* Enable/disable Auto-Knee-Point adjustion. */
	LCVCamGetRegisterValue(REG_SHUTTER_WIDTH_CONTROL, &regVal);
	regVal = SET_BIT(regVal, 8, cgi.args.bHighDynamicRange);
	LCVCamSetRegisterValue(REG_SHUTTER_WIDTH_CONTROL, regVal);
	/* Enable/disable HDR */
	LCVCamGetRegisterValue(REG_PIXEL_OP_MODE, &regVal);
	regVal = SET_BIT(regVal, 6, cgi.args.bHighDynamicRange);
	LCVCamSetRegisterValue(REG_PIXEL_OP_MODE, regVal);
      }

    if(cgi.args.bRowWiseNoiseCorr_supplied)
      {
	LCVCamGetRegisterValue(REG_ROW_NOISE_CORR_CONTROL_1, &regVal);
	regVal = SET_BIT(regVal, 5, cgi.args.bRowWiseNoiseCorr);
	LCVCamSetRegisterValue(REG_ROW_NOISE_CORR_CONTROL_1, regVal);
      }

#ifdef DBG_SPAM
    LCVLog(DEBUG, "Arguments applied!\n");
#endif
    
    /* ---------------- Frame capture -------------*/
    err = LCVCamSetupCapture(0, LCV_CAM_TRIGGER_MODE_MANUAL);
    if(err != SUCCESS)
      {
	LCVLog(ERROR, APP_NAME ": Unable to trigger frame! (%d)\n", err);
	goto exit_unload;
      }
    
    err = LCVCamReadPicture(0, &pPic, 0, 10000);
    if(err != SUCCESS)
      {
	LCVLog(ERROR, APP_NAME ": Unable to read picture! (%d)\n", err);
	goto exit_unload;
      }
#ifdef DBG_SPAM
    LCVLog(DEBUG, "Image read!\n");
#endif
    if(!CAPTURE_RAW)
      {
	err = LCVVisDebayer(pPic,
			    LCV_CAM_MAX_IMG_WIDTH,
			    LCV_CAM_MAX_IMG_HEIGHT,
			    0,
			    0,
			    cgi.imgBuf);
	if(err != SUCCESS)
	  {
	    LCVLog(ERROR, APP_NAME ": Error debayering image! (%d)\n",
		   err);
	    goto exit_unload;
	  }
      }
    /* --------------- Save as file --------------*/
    if(CAPTURE_RAW)
      {
	picFile.data = pPic;
	picFile.type = LCV_PICTURE_GREYSCALE;
      } else {
	picFile.data = cgi.imgBuf;
	picFile.type = LCV_PICTURE_RGB_24;

      }
    picFile.width = LCV_CAM_MAX_IMG_WIDTH;
    picFile.height = LCV_CAM_MAX_IMG_HEIGHT;

    err = LCVBmpWrite(&picFile, OUT_IMG_NAME);
    if(err != SUCCESS)
      {
	LCVLog(ERROR, APP_NAME ": Unable to write image to file! (%d)\n", err);
	goto exit_unload;
      }

    cgi.timeStamp = LCVSupCycGet();

#ifdef DBG_SPAM
    LCVLog(DEBUG, "Image written!\n");
#endif
    /* ---------------- CGI response --------------*/
    /* Header */
    printf("Content-type: text/html\n\n");

    printf("TS=%lu\n", cgi.timeStamp);
    LCVCamGetRegisterValue(REG_AGC_GAIN_OUTPUT, &regVal);
    printf("curGainFactor=%f\n", ((float)regVal)*0.0625f);
    LCVCamGetRegisterValue(REG_AEC_EXP_OUTPUT, &regVal);
    printf("curExpTime=%hu\n", (34*regVal)/1000);

#ifdef DBG_SPAM
    LCVLog(DEBUG, "Closing...!\n");
#endif
    /* ----------------- Unload -------------------*/	 
 exit_unload:
 fb_err:
    LCVUnloadDependencies(cgi.hFramework,
			  deps,
			  sizeof(deps)/sizeof(struct LCV_DEPENDENCY));
 dep_err:
    LCVDestroy(cgi.hFramework);
    return 0;
}
