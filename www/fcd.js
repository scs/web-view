var xmlHttp = null;
var isIE = false;
var bFirstRun = true;
var bAutoExp = true;
var bAutoGain = true;

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

    if(bAutoExp != elem("autoExposure").checked || bFirstRun)
    {
	bAutoExp = elem("autoExposure").checked;
	parameters = addURIQueryVal(parameters, "autoExp", bAutoExp);
    }
    if(bAutoExp)
	parameters = addURIQueryVal(parameters, "maxExposure", elem("maxExpTime").value);
    else
	parameters = addURIQueryVal(parameters, "manExposure", elem("manExpTime").value);

    if(bAutoGain != elem("autoGain").checked || bFirstRun)
    {
	bAutoGain = elem("autoGain").checked;
	parameters = addURIQueryVal(parameters, "autoGain", elem("autoGain").checked);
    }
    
    if(bAutoGain)
	parameters = addURIQueryVal(parameters, "maxGain", elem("maxGainFactor").value);
    else
	parameters = addURIQueryVal(parameters, "manGain", elem("manGainFactor").value);

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
		alert(response[i]);
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
