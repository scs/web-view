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

var xmlHttp = null;
var isIE = false;
var bFirstRun = true;
var bSetRegister = false;

var config = {
    bAutoExp: true,
    bAutoGain: true,
    b12To10BitCompanding: false,
    bHighDynamicRange: false,
    bRowWiseNoiseCorr: false,
    bHorizontalFlip: false,
    bVerticalFlip: false,
    configOption: 0
}

function onChangeAutoRefresh()
{
	bAutoRefresh = document.getElementById("autoRefresh").checked;
	if(bAutoRefresh == true)
	{
		updateData();
	}
}

function onChangeParam()
{
	if(bAutoRefresh == false)
	{
		updateData();
	}
}

function onChangeAutoExp()
{
    var checkbox = document.getElementById("autoExposure");
    
    if(checkbox.checked)
    {
	elem("autoExpTimeContainer").style.display = "block";
	elem("manExpTimeContainer").style.display = "none";
    } else {
	elem("autoExpTimeContainer").style.display = "none";
	elem("manExpTimeContainer").style.display = "block";
    }
}

function onChangeAutoGain()
{
    var checkbox = document.getElementById("autoGain");
    
    if(checkbox.checked)
    {
	elem("autoGainFactorContainer").style.display = "block";
	elem("manGainFactorContainer").style.display = "none";
    } else {
	elem("autoGainFactorContainer").style.display = "none";
	elem("manGainFactorContainer").style.display = "block";
    }
}

function onChangeConfigOption()
{
    config.configOption = elem("configOption").selectedIndex;

    switch(config.configOption)
    {
    case 0:
	elem("brightnessControl").style.display = "block";
	elem("imageQuality").style.display = "none";
	elem("imageOrientation").style.display = "none";
	elem("registerModify").style.display = "none";	
	break;
    case 1:
	elem("brightnessControl").style.display = "none";
	elem("imageQuality").style.display = "block";
	elem("imageOrientation").style.display = "none";
	elem("registerModify").style.display = "none";	
	break;
    case 2:
	elem("brightnessControl").style.display = "none";
	elem("imageQuality").style.display = "none";
	elem("imageOrientation").style.display = "block";	
	elem("registerModify").style.display = "none";	
	break;
    case 3:
	elem("brightnessControl").style.display = "none";
	elem("imageQuality").style.display = "none";
	elem("imageOrientation").style.display = "none";	
	elem("registerModify").style.display = "block";	
	break;
    }
}

function getHTTPObject() 
{
    try
    {
	// Firefox, Opera 8.0+, Safari
	xmlHttp=new XMLHttpRequest();
	if (xmlHttp.overrideMimeType) 
	{
            // set type accordingly to anticipated content type
            xmlHttp.overrideMimeType('text/html');
        }
    }
    catch (e)
    {
	// Internet Explorer
	isIE = true;
	
	var msxmlhttp = new Array(
	    'Msxml2.XMLHTTP.5.0',
	    'Msxml2.XMLHTTP.4.0',
	    'Msxml2.XMLHTTP.3.0',
	    'Msxml2.XMLHTTP',
	    'Microsoft.XMLHTTP');
	for (var i = 0; i < msxmlhttp.length; i++) {
	    try {
		xmlHttp = new ActiveXObject(msxmlhttp[i]);
	    } catch (e) {
		xmlHttp = null;
	    }
	}
	
	if(xmlHttp == null)
	{
	    alert("Your browser does not support AJAX!");
	}
	
    }
}

function onLoad()
{
    /* Read current configuration. */
    config.bAutoExp = elem("autoExposure").checked;
    onChangeAutoExp();

    config.bAutoGain = elem("autoGain").checked;
    onChangeAutoGain();

    config.b12To10BitCompanding = elem("Compand12To10").checked;
    config.bRowWiseNoiseCorr = elem("rowWiseNoiseCorr").checked;

    config.configOption = elem("configOption").selectedIndex;
    onChangeConfigOption();

    updateData();
}

function addURIQueryVal(URIQuery, identifier, value)
{
    if(URIQuery != "")
    {
	URIQuery += "&";
    }
    URIQuery += identifier + "=" + value;
    return URIQuery;
}

function elem(elemIdentifier)
{
    return document.getElementById(elemIdentifier);
}

function updateData() 
{
    var parameters = "";

    getHTTPObject();

    if(bFirstRun)
    {
	parameters = addURIQueryVal(parameters, "init", true);
	bFirstRun = false;
    }
    if(config.bAutoExp != elem("autoExposure").checked || bFirstRun)
    {
	config.bAutoExp = elem("autoExposure").checked;
	parameters = addURIQueryVal(parameters, "autoExp", config.bAutoExp);
    }
    var intVal;
    if(config.bAutoExp)
    {
	intVal = parseInt(elem("maxExpTime").value);
	if(!isNaN(intVal) && intVal >  1)
	    parameters = addURIQueryVal(parameters, "maxExposure", intVal);
	else
	    parameters = addURIQueryVal(parameters, "maxExposure", 1);
    } else {
	intVal = parseInt(elem("manExpTime").value);
	if(!isNaN(intVal) && intVal >  1)
	    parameters = addURIQueryVal(parameters, "manExposure", intVal);
	else
	    parameters = addURIQueryVal(parameters, "manExposure", 1);
    }

    if(config.bAutoGain != elem("autoGain").checked || bFirstRun)
    {
	config.bAutoGain = elem("autoGain").checked;
	parameters = addURIQueryVal(parameters, "autoGain", elem("autoGain").checked);
    }
    
    var floatVal;
    if(config.bAutoGain)
    {
	floatVal = parseFloat(elem("maxGainFactor").value);
	if(!isNaN(floatVal) && floatVal > 0.2)
	    parameters = addURIQueryVal(parameters, "maxGain", floatVal);
	else
	    parameters = addURIQueryVal(parameters, "maxGain", 0.2);
    } else {
	floatVal = parseFloat(elem("manGainFactor").value);
	if(!isNaN(floatVal) && floatVal > 0.2)
	    parameters = addURIQueryVal(parameters, "manGain", floatVal);
	else
	    parameters = addURIQueryVal(parameters, "manGain", 0.2);
    }

    if(config.b12To10BitCompanding != elem("Compand12To10").checked)
    {
	config.b12To10BitCompanding = !config.b12To10BitCompanding;
	parameters = addURIQueryVal(parameters, "compand12To10", config.b12To10BitCompanding);
    }

    if(config.bRowWiseNoiseCorr != elem("rowWiseNoiseCorr").checked)
    {
	config.bRowWiseNoiseCorr = !config.bRowWiseNoiseCorr;
	parameters = addURIQueryVal(parameters, "rowWiseNoiseCorr", config.bRowWiseNoiseCorr);
    }

    if(config.bHorizontalFlip != elem("horizontalFlip").checked || 
      config.bVerticalFlip != elem("verticalFlip").checked)
    {
	if(config.bHorizontalFlip != elem("horizontalFlip").checked)
	    config.bHorizontalFlip = !config.bHorizontalFlip;
	if(config.bVerticalFlip != elem("verticalFlip").checked)
	    config.bVerticalFlip = !config.bVerticalFlip;

	parameters = addURIQueryVal(parameters, "horizontalFlip", config.bHorizontalFlip);
	parameters = addURIQueryVal(parameters, "verticalFlip", config.bVerticalFlip);
    }

    if(config.configOption == 3 && !isNaN(parseInt(elem("register").value)))
    {
	parameters = addURIQueryVal(parameters, "register", parseInt(elem("register").value));
    }

    if(bSetRegister == true && !isNaN(parseInt(elem("nextRegisterValue").value)))
    {
	parameters = addURIQueryVal(parameters, "registerValue", parseInt(elem("nextRegisterValue").value));
	bSetRegister = false;
    }


    bFirstRun = false;

    xmlHttp.open('POST', 'cgi-bin/fcd.cgi', true);
    xmlHttp.onreadystatechange = useHttpResponse;
    xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlHttp.setRequestHeader("Content-length", parameters.length);
    xmlHttp.setRequestHeader("Connection", "close");
    xmlHttp.send(parameters);
}

function zeroExtend(strArg)
{
    while(strArg.length < 7)
    {
	strArg = "0" + strArg;
    }
    return strArg;
}

function useHttpResponse()
{
    if(xmlHttp.readyState==4)
    {
	var response = xmlHttp.responseText.split('\n');
	var i = 0;
	var arg;
	var argSplit;
	var temp;
	
	// Separate the different parameters of the response.
	while(response[i])
	{
	    // Separate parameter name and parameter value
	    arg = response[i];
	    argSplit = arg.split('=');
	    
	    // Depending on the parameter name, invoke a different action.
	    switch(argSplit[0])
	    {
	    case "TS":
		elem("liveImage").src = "live.bmp?" + argSplit[1];
		break;
	    case "curGainFactor":
	    case "curExpTime":
		if(isIE)
		{
		    elem(argSplit[0]).innerText = argSplit[1];
		} else {
		    elem(argSplit[0]).innerHTML = argSplit[1];
		}
		break;
	    case "curRegisterValue":
		if(isIE)
		{
		    elem(argSplit[0]).innerText = "0x" + parseInt(argSplit[1]).toString(16);
		} else {
		    elem(argSplit[0]).innerHTML = "0x" + parseInt(argSplit[1]).toString(16);
		}
		break;
	    default:
		// Something unexpected received.
		//window.location="off.html";
		return;
		break;
	    }
	    i++;	
	}
	setTimeout("updateData()", 1);
    }
}

function setRegister()
{
    bSetRegister = true;
}

function modParam(paramName, mod, min, max)
{
    var valueString;
    var value;
    var elem;
    
    elem = document.getElementById(paramName);
    valueString = elem.value;
    value = parseFloat(valueString);
    value += mod;
    
    if(value < min)
    {
	value = min;
    } 
    if(value > max)
    {
	value = max;
    }
    elem.value = value;
    switch(paramName)
    {
    case 'corrSens':
	    bCorrSensChanged = true;
	    break;
    case 'stopCrit':
	    bStopCritChanged = true;
	    break;
    }
}
