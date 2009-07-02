Array.prototype.joinLines = function () {
	var res = this.join("\n")
	
	if (res)
		res += "\n";
	
	return res;
};

String.prototype.splitLines = function () {
	var list = this.split("\n");
	
	if (list.length > 0 && !list[list.length - 1])
		list.pop();
	
	return list;
};

// Converts a number to a string with a specified number of digits before and after the dot together.
Number.prototype.toPrecision2 = function (digits) {
	var parts = this.toString().split(".");
	var p1 = parts[0], p2 = parts[1] || "", pp = p1 + p2;
	var res = p1;
	
	while (pp.slice(0, 1) == "0") {
		digits += 1;
		pp = pp.slice(1);
	}
	
	if (p1.length < digits) {
		res += "." + p2;
		digits += 1; // Because of the dot.
	}
	
	res = res.slice(0, digits);
	digits = Math.max(digits, p1.length)
	
	while (res.length < digits)
		res += "0";
	
	return res;
};

function getSearchArgs() {
	var parts = document.location.search.slice(1).split("&");
	var result = { };
	
	$.each(parts, function () {
		var pair = this.split("=");
		
		if (pair.length == 1)
			result[pair[0]] = true;
		else
			result[pair[0]] = pair.slice(1).join("=");
	})
	
	return result;
}

function setLanguage(lang) {
	$("*[lang]").hide();
	$("*[lang|=" + lang + "]").show();
	$("#lang-select a").removeClass("active");
	$("#lang-select *[id|=" + lang + "]").addClass("active");
}

function createElement(tag, attrs, contents) {
	var elem = $(document.createElement(tag));
	
	if (attrs)
		$.each(attrs, function (key, value) {
			elem.attr(attrs);
		});
	
	if (contents)
		$.each(contents, function (key, value) {
			elem.append(value);
		});
	
	return elem;
}

function buildControls() {
	var inputs = $(".input:parent")
	var ws = $(document.createTextNode(" "));
	
	inputs.filter("[type=checkbox], [type=radio]").each(function () {
		var name = $(this).attr("name")
		var value = $(this).attr("value")
		var id = name + "--" + $(this).attr("value");
		var label = createElement("label", { "for" : id }, $(this).contents());
		var elem = createElement("input", {
			"name" : name,
			"type" : $(this).attr("type"),
			"value" : $(this).attr("value"),
			"id" : id
		});
		
		$(this).after(label);
		$(this).replaceWith(elem);
		
		label.addClass("label-after");
		elem.click(function () {
			var elem = $(this);
			var type = elem.attr("type");
			var value;
			
			if (type == "checkbox")
				if (elem.length == 1)
					value = (elem.fieldValue().length != 0).toString();
				else
					value = elem.fieldValue().join(" ");
			else if (type == "radio")
				value = elem.fieldValue().join(" ");
			else
				value = elem.fieldValue()[0];
			
			inputValues[elem.attr("name")] = value;
		});
		
		if (inputValues[name] && $.inArray(value, inputValues[name].split(" ")) != -1)
			elem.attr("checked", "checked");
	});
	
	inputs.filter("[type=slider]").each(function () {
		var range = $(this).attr("value").split(" ");
		var value = createElement("span");
		var label = createElement("span", { }, $(this).contents());
		var elem = createElement("div", {
			"class" : $(this).attr("class"),
			"type" : $(this).attr("type"),
			"name" : $(this).attr("name")
		});
		
		label.addClass("label-before");
		value.addClass("label-after");
		
		$(this).before(label);
		$(this).after(value);
		$(this).replaceWith(createElement("div", {
			"style" : "display: inline-block; vertical-align: middle; position: relative; top: -2px"
		}, elem));
		
		elem.slider({
			"animate" : true,
			"min" : 0,
			"max" : 1,
			"step" : 1 / 1000000,
			"range" : "min",
			"slide" : function (event, ui) {
				var a = parseFloat(range[0]), b = parseFloat(range[1]);
				var x = ui.value;
				
				if (range[2] == "log")
					x = Math.exp((x * (Math.log(b) - Math.log(a)) + Math.log(a)));
				else
					x = x * (b - a) + a;
				
				inputValues[$(this).attr("name")] = x.toFixed(0);
				value.text(x.toPrecision2(2));
			}
		});
	});
}

// Removes duplicate elements from an array.
function removeDuplicates(arr) {
	var newArr = [];
	var arr = $.makeArray(arr);
	
	outer: for (var i in arr) {
		for (var j in newArr)
			if (newArr[j] == arr[i]) 
				continue outer;
		
		newArr.push(arr[i]);
	}
	
	return newArr;
}

// getInputNames: Get a list of names of all input elements in the given context. context is optional and defaults to the document.
function getInputNames(context) {
	var foo = $("input", context);
	
	return removeDuplicates($("input", context).map(function() {
		return $(this).attr("name");
	}))
}

function getInputValue(name, context) {
	var elem = $("input[name=" + name + "]", context);
	var type = elem.attr("type");
	
	if (type == "checkbox")
		if (elem.length == 1)
			return (elem.fieldValue().length != 0).toString();
		else
			return elem.fieldValue().join(" ");
	else if (type == "radio")
		return elem.fieldValue().join(" ");
	else
		return elem.fieldValue()[0];
}

/* getInputValues: Get a map of input element names to their values. context is optional and defaults to the document. */
function getInputValues(context) {
	var table = { };
	
	$.each(getInputNames(context), function () {
		table[this] = getInputValue(this, context);
	});
	
	return table;
}

function serializeValues(data) {
	var list = [];
	
	$.each(data, function (key) {
		list.push(key + ": " + this);
	})
	
	return list.joinLines();
}

function parseValues(data) {
	var obj = { };
	
	$.each(data.splitLines("\n"), function () {
		var pos = this.indexOf(":");
		
		obj[$.trim(this.slice(0, pos))] = $.trim(this.slice(pos + 1));
	})
	
	return obj;
}

var outputValueHooks = {
	colorType: function (value) {
		if (value == "gray")
			return "8 bit grayscale";
		else if (value == "raw")
			return "8 bit grayscale";
		else if (value == "debayered")
			return "8 bit RGB";
	}
};

function exchangeState(header, data, onLoad, onError) {
	$.ajax({
		async: true,
		cache: false,
		contentType: "text/plain",
		data: header + "\n" + serializeValues(data),
		error: onError,
		success: function (data) {
			if (onLoad)
				onLoad(parseValues(data));
		},
		timeout: 2000,
		type: "POST",
		url: "/cgi-bin/cgi"
	});
}

function asynLoadImage(url, onLoad, onError) {
	var img = $(new Image());
	
	img.load(onLoad);
	img.error(onError);
	img.attr("src", url + "?dummy=" + (new Date()).getTime());
}

var offBanner = {
	active: false,
	show: function () {
		var wheelPos = 0;
		
		if (this.active)
			return;
		
		this.active = true;
		$("#off").stop(true, true).fadeIn("fast");
		
		$("#off .wheel").everyTime("100ms", function () {
			wheelPos = (wheelPos + 1) % 12;
			$(this).css("background-position", -wheelPos * 32 + "px");
		});
	},
	hide: function () {
		if (!this.active)
			return;
		
		this.active = false;
		$("#off").stop(true, true).fadeOut("fast", function () {
			$(".wheel", this).stopTime();
		});
	}
}

var stateControl = {
	currentState: "online",
	pulledState: "",
	pullState: function (state) {
		var that = this;
		
		if (state != this.pulledState) {
			$(document).stopTime("stateControl");
			this.pulledState = state;
		}
		
		if (state != this.currentState) {
			$(document).oneTime("2s", "stateControl", function () {
				that.changeState(state);
			});
		}
	},
	changeState: function (state) {
		var states = {
			offline: function () {
				offBanner.show();
			},
			online: function () {
				offBanner.hide();
			}
		};
		
		this.currentState = state;
		states[state]();
	}
}

function updateCycle() {
	function offline() {
		stateControl.pullState("offline");
		
		$(document).oneTime("0.5s", function () {
			exchangeState("GetSystemInfo", { }, function (data) {
				$.each(data, function (key, value) {
					function id(value) {
						return value;
					};
					
					$("#" + key).text((outputValueHooks[key] || id)(value));
				});
				
				online();
			}, offline);
		});
	}
	
	function online() {
		stateControl.pullState("online");
		
		exchangeState("GetImage", { }, function (data) {
			asynLoadImage("image.bmp?" + data.imgTS, function () {
				$(this).attr("id", "image");
				$("#image").replaceWith(this);
				
				$.each(data, function (key, value) {
					function id(value) {
						return value;
					};
					
					$("#" + key).text((outputValueHooks[key] || id)(value));
				})
				
				// Close the loop.
				online();
			}, function (event) {
			//	console.log(event);
				offline();
			});
			
			if (data.exposureTime != inputValues.exposureTime)
				exchangeState("SetOptions", {
					exposureTime: inputValues.exposureTime
				});
			
			if (data.colorType != inputValues.colorType)
				exchangeState("SetOptions", {
					colorType: inputValues.colorType
				});
		}, function (request, status) {
		//	console.log(status);
			offline();
		});
	}
	
	offline();
}
