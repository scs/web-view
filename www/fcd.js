var xmlHttp = null;
var isIE = false;
var bFirstRun = true;
var bAutoExp = true;
var bAutoGain = true;
var b12To10BitCompanding = false;
var bHighDynamicRange = false;
var bRowWiseNoiseCorr = false;
var configOption = 0;

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
    switch(elem("configOption").selectedIndex)
    {
    case 0:
	elem("brightnessControl").style.display = "block";
	elem("imageQuality").style.display = "none";
	elem("imageOrientation").style.display = "none";
	configOption = 0;
	break;
    case 1:
	elem("brightnessControl").style.display = "none";
	elem("imageQuality").style.display = "block";
	elem("imageOrientation").style.display = "none";
	configOption = 1;
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
    bAutoExp = elem("autoExposure").checked;
    onChangeAutoExp();

    bAutoGain = elem("autoGain").checked;
    onChangeAutoGain();

    b12To10BitCompanding = elem("Compand12To10").checked;
    bHighDynamicRange = elem("highDynamicRange").checked;
    bRowWiseNoiseCorr = elem("rowWiseNoiseCorr").checked;

    configOption = elem("configOption").selectedIndex;
    onChangeConfigOption()

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
    if(bAutoExp != elem("autoExposure").checked || bFirstRun)
    {
	bAutoExp = elem("autoExposure").checked;
	parameters = addURIQueryVal(parameters, "autoExp", bAutoExp);
    }
    var intVal;
    if(bAutoExp)
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

    if(bAutoGain != elem("autoGain").checked || bFirstRun)
    {
	bAutoGain = elem("autoGain").checked;
	parameters = addURIQueryVal(parameters, "autoGain", elem("autoGain").checked);
    }
    
    var floatVal;
    if(bAutoGain)
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

    if(b12To10BitCompanding != elem("Compand12To10").checked)
    {
	b12To10BitCompanding = !b12To10BitCompanding;
	parameters = addURIQueryVal(parameters, "compand12To10", b12To10BitCompanding);
    }

    if(bHighDynamicRange != elem("highDynamicRange").checked)
    {
	bHighDynamicRange = !bHighDynamicRange;
	parameters = addURIQueryVal(parameters, "highDynamicRange", bHighDynamicRange);
    }

    if(bRowWiseNoiseCorr != elem("rowWiseNoiseCorr").checked)
    {
	bRowWiseNoiseCorr = !bRowWiseNoiseCorr;
	parameters = addURIQueryVal(parameters, "rowWiseNoiseCorr", bRowWiseNoiseCorr);
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

function setReg()
{
    var elem = document.getElementById("reg");
    reg = parseInt(elem.value);

    elem = document.getElementById("val");
    val = parseInt(elem.value);
}

function getReg()
{
    var elem = document.getElementById("reg");
    reg = parseInt(elem.value);
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
