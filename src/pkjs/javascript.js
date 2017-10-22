function appMessageAck(e) {
    console.log("options sent to Pebble successfully");
}

function appMessageNack(e) {
    console.log("options not sent to Pebble: " + e.error.message);
}

Pebble.addEventListener("ready", function() {
  // Send to watchapp
  Pebble.getTimelineToken(function(token) {
    var dict = {};
    dict['WATCHAPP_READY_SIGNAL'] = token;
    Pebble.sendAppMessage(dict, function() {
      console.log('Send successful: ' + JSON.stringify(dict));
    }, function() {
      console.log('Send failed!');
    });
  });

Pebble.addEventListener("showConfiguration", function() {
	var uri = "http://rodgerleblanc.github.io/Bridge/Aplite/index.html";
	if(Pebble.getActiveWatchInfo) {
		var watch = Pebble.getActiveWatchInfo();
		if (watch.platform !== "aplite")
			uri = "http://rodgerleblanc.github.io/Bridge/Time/index.html";
	}
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var color = configData['color'];
  var vibration = configData['vibration'];
  var fontSize = configData['font_size'];

  var dict = {};
  dict['CONFIG_KEY_VIBRATION_TYPE'] = parseInt(vibration);
  dict['CONFIG_KEY_LARGE_FONT'] = parseInt(fontSize);
  dict['CONFIG_KEY_MAIN_COLOR'] = parseInt(color);

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});
	
  /*
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
  */
  
  /*
    // An hour ahead
  var date = new Date();
  date.setHours(date.getHours() + 1);

  // Create the pin
  var pin = {
    "id": "pin-" + Math.round((Math.random() * 100000)),
    "time": date.toISOString(),
    "layout": {
      "type": "genericPin",
      "title": "Trying Bridge Calendar pin",
      "tinyIcon": "system://images/CALENDAR_EVENT"
    }
  };

  console.log('Inserting pin in the future: ' + JSON.stringify(pin));

  insertUserPin(pin, function(responseText) { 
    console.log('Result: ' + responseText);
  });
  */
  
});



/******************************* timeline lib *********************************/

// The timeline public URL root
//var API_URL_ROOT = 'https://timeline-api.getpebble.com/';

/**
 * Send a request to the Pebble public web timeline API.
 * @param pin The JSON pin to insert. Must contain 'id' field.
 * @param type The type of request, either PUT or DELETE.
 * @param callback The callback to receive the responseText after the request has completed.
 */
//function timelineRequest(pin, type, callback) {
  // User or shared?
/*
  var url = API_URL_ROOT + 'v1/user/pins/' + pin.id;

  // Create XHR
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    console.log('timeline: response received: ' + this.responseText);
    callback(this.responseText);
  };
  xhr.open(type, url);

  // Get token
  Pebble.getTimelineToken(function(token) {
    // Add headers
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.setRequestHeader('X-User-Token', '' + token);

    // Send
    xhr.send(JSON.stringify(pin));
    console.log('timeline: request sent. Token: ' + token);
  }, function(error) { console.log('timeline: error getting timeline token: ' + error); });
}
*/

/**
 * Insert a pin into the timeline for this user.
 * @param pin The JSON pin to insert.
 * @param callback The callback to receive the responseText after the request has completed.
 */
//function insertUserPin(pin, callback) {
 // timelineRequest(pin, 'PUT', callback);
//}

/**
 * Delete a pin from the timeline for this user.
 * @param pin The JSON pin to delete.
 * @param callback The callback to receive the responseText after the request has completed.
 */
//function deleteUserPin(pin, callback) {
 // timelineRequest(pin, 'DELETE', callback);
//}

/***************************** end timeline lib *******************************/