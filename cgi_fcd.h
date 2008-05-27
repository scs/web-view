/*! @file cgi_fcd.h
 * @brief Header file of the CGI used for frame capture device demo.
 * 
 * @author Markus Berner
 */

#ifndef CGI_FCD_H_
#define CGI_FCD_H_

#define LCV_CAM_MAX_IMG_WIDTH 752
#define LCV_CAM_MAX_IMG_HEIGHT 480

/*! @brief The name of this application. */
#define APP_NAME "CGI_FCD"

/*! @brief The name of the file as which the captured image is stored for
 *  retrieval by the web server. */
#define OUT_IMG_NAME "../live.bmp"

/*! @brief The maximum length of the POST argument string supplied
 * to this CGI.*/
#define MAX_ARGUMENT_STRING_LEN 1024
/*! @brief The maximum length of the name string of a GET/POST
 * argument. */
#define MAX_ARG_NAME_LEN 32

/* Register definitions */
#define REG_AEC_AGC_ENABLE 0xaf
#define REG_MAX_EXP 0xbd
#define REG_MAX_GAIN 0x36
#define REG_EXPOSURE 0x0b
#define REG_GAIN 0x35
#define REG_AGC_GAIN_OUTPUT 0xba
#define REG_AEC_EXP_OUTPUT 0xbb

/* @brief The different data types of the argument string. */
enum EnArgumentType
{
	STRING_ARG,
	INT_ARG,
	SHORT_ARG,
	BOOL_ARG,
	FLOAT_ARG
};

/*! @brief A single POST/GET argument. */
struct ARGUMENT
{
	/*! @brief The name of the argument. */
	char				strName[MAX_ARG_NAME_LEN];
	/*! brief The data type of the argument. */
	enum EnArgumentType enType;
	/*! @brief Pointer to the variable this argument should be parsed 
	 * to.*/
	void				*pData;
	/*! @brief Pointer to a variable storing whether this argument has
	 * been supplied or not.*/
	bool                *pbSupplied;
};

/*! @brief Holds the values parsed from the argument string. */
struct ARGUMENT_DATA
{
	/*! @brief The maximum gain during auto-exposure to be set.*/
	float maxGain;
	/*! @brief Says whether the argument maxGain has been 
	 * supplied or not. */
	bool bMaxGain_supplied;
	/*! @brief The manual gain to be set.*/
	float manGain;
	/*! @brief Says whether the argument manGain has been 
	 * supplied or not. */
	bool bManGain_supplied;
	/*! @brief Maximum exposure time during auto-exposure. */
	int maxExp;
	/*! @brief Says whether the argument maxExp has been
	 * supplied or not. */
	bool bMaxExp_supplied;
	/*! @brief Manual exposure time during auto-exposure. */
	int manExp;
	/*! @brief Says whether the argument manExp has been
	 * supplied or not. */
	bool bManExp_supplied;
	/*! @brief Auto-Exposure. */
	bool bAutoExp;
	/*! @brief Says whether the argument bAutoExp has been
	 * supplied or not. */
	bool bAutoExp_supplied;
	/*! @brief Auto-Gain. */
	bool bAutoGain;
	/*! @brief Says whether the argument bAutoGain has been
	 * supplied or not. */
	bool bAutoGain_supplied;
};

/*! @brief Main object structure of the CGI. Contains all 'global'
 * variables. */
struct CGI_FCD
{
  /*! @brief Handle to the framework. */
  void *hFramework;
  /*! @brief The raw argument string as supplied by the web server. */
  char strArgumentsRaw[MAX_ARGUMENT_STRING_LEN];
  /*! @brief Temporary variable for argument extraction. */
  char strArgumentsTemp[MAX_ARGUMENT_STRING_LEN];
  
  /*! @brief Temporary data buffer for the images to be saved. */
  uint8 frameBuf[LCV_CAM_MAX_IMG_WIDTH*LCV_CAM_MAX_IMG_HEIGHT];
  /*! @brief Temporary data buffer for the color image to be saved. */
  uint8 imgBuf[LCV_CAM_MAX_IMG_WIDTH*LCV_CAM_MAX_IMG_HEIGHT*3];
  /*! @brief Time stamp of the captured image. */
  uint32 timeStamp;
  /*! @brief The GET/POST arguments of the CGI. */
  struct ARGUMENT_DATA	args;
#if defined(LCV_HOST) || defined(LCV_SIM)
  /*! @brief Handle to the file name reader used to feed the camera
    with images on the host. */
  void *hFileNameReader;
#endif /* LCV_HOST or LCV_SIM */
};

#endif /*CGI_FCD_H_*/
