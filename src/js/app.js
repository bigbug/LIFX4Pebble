function lifx_isAuthorized() {
  var token = localStorage.getItem(0);
  if(token===null || token===undefined || token.length<10) {
    Pebble.showSimpleNotificationOnPebble("Not authorized", "Go to the settings screen on your phone to provide your personal LIFX token.");
    return false;
  }
  return true;
}

function xhr_errorHandling(xhr) {
  if(xhr.status===null) {
    Pebble.showSimpleNotificationOnPebble("Undefined error", "HTTP request didn't return any status.");
  } else {
    Pebble.showSimpleNotificationOnPebble("Status "+xhr.status, xhr.statusText);
  }
}

function lifx_getState() {
  if(!lifx_isAuthorized()) return;
  var xhr = new XMLHttpRequest();
  xhr.open( 'GET', 'https://api.lifx.com/v1/lights/all', true);
  xhr.setRequestHeader( 'Authorization', 'Bearer ' + localStorage.getItem(0) );
  //xhr.withCredentials = true;
  xhr.onreadystatechange = function() {
      if (xhr.readyState == 4 || xhr.status == 200) {
        var myArr = JSON.parse(xhr.responseText);
        var myState = (myArr[0].power=="on") ? 1 : 0;
        console.log("returned lifx-state: "+myState+" raw: "+myArr[0].power);
        var dict = {
          LIFX_STATE_POWER: myState,
          LIFX_STATE_BRIGHTNESS: parseInt(myArr[0].brightness*100)
        };
        // Send a string to Pebble
        Pebble.sendAppMessage(dict, function(e) {
          console.log('Send successful.');
        }, function(e) {
          console.log('Send failed!');
        });
      } else {
        xhr_errorHandling(xhr);
      }
  };
  xhr.send();
}

function lifx_toggle() {
  if(!lifx_isAuthorized()) return;
  var xhr = new XMLHttpRequest();
  xhr.open( 'POST', 'https://api.lifx.com/v1/lights/all/toggle', true);
  xhr.setRequestHeader( 'Authorization', 'Bearer ' + localStorage.getItem(0) );
  xhr.onreadystatechange = function() {
    if(xhr.status == 200 || xhr.status==207) {
        console.log( "toggled status", xhr.status, xhr.statusText);
      } else {
        xhr_errorHandling(xhr);
      }
      lifx_getState();
  };
  xhr.send();
}

function lifx_state(powerState, color, brightness, duration, saturation) {
  if(!lifx_isAuthorized()) return;
  
  if(powerState === null)
    powerState = "off";
  
  if(duration === null)
    duration = 1.0;
  
  var a = {
    "power": powerState,
    "duration": duration
  };
  
  if(brightness !== null)
    a.brightness = brightness;
  
  if(color !== null) {
    a.color = color;
  }
  
  if(saturation !== null && saturation !== undefined) {
    if(a.color === null || a.color === undefined) {
      a.color = "saturation:"+saturation;
    } else {
      a.color = a.color + " saturation:"+saturation;
    }
  }
  
  console.log("LIFX raw options: "+JSON.stringify(a));
  
  var xhr = new XMLHttpRequest();
  xhr.open( 'PUT', 'https://api.lifx.com/v1/lights/all/state', true);
  xhr.setRequestHeader( 'Authorization', 'Bearer ' + localStorage.getItem(0) );
  //xhr.withCredentials = true;
  xhr.onreadystatechange = function() {
      console.log( "state status", xhr.status, xhr.statusText);
      if(xhr.status == 200 || xhr.status==207) {
        console.log("successfully changed state");
        var dict = {
          LIFX_STATE_POWER: (powerState=="on") ? 1 : 0
        };
        // Send a string to Pebble
        Pebble.sendAppMessage(dict, function(e) {
          console.log('Send successful.');
        }, function(e) {
          console.log('Send failed!');
        });
      } else {
        xhr_errorHandling(xhr);
      }
  };
  xhr.send(JSON.stringify(a));
}

Pebble.addEventListener('ready', function (e) {
  console.log('connect!' + e.ready);
  
  //toggleLamp();
  lifx_getState();
  
  console.log(e.type);
});

Pebble.addEventListener('appmessage', function (e) {
  console.log(JSON.stringify(e.payload));
  if(e.payload.TOGGLE===0) {
    lifx_state("off");
  } else if(e.payload.REQUEST_STATE>=-1) {
    lifx_getState();
  } else if(e.payload.TOGGLE==1) {
    lifx_state("on");
  } else if (e.payload.TOGGLE==2) {
    lifx_toggle();
  } else if (e.payload.DIMM>=0) {
    lifx_state("on", null, e.payload.DIMM/100, null);
  } else if (e.payload.SATURATION>=0) {
    lifx_state("on", null, null, null, e.payload.SATURATION/100);
  } else if(e.payload.COLOR===0) {
    lifx_state("on", "white");
  } else if(e.payload.COLOR==1) {
    lifx_state("on", "red");
  } else if(e.payload.COLOR==2) {
    lifx_state("on", "orange");
  } else if(e.payload.COLOR==3) {
    lifx_state("on", "yellow");
  } else if(e.payload.COLOR==4) {
    lifx_state("on", "green");
  } else if(e.payload.COLOR==5) {
    lifx_state("on", "blue");
  } else if(e.payload.DURATION_OFF>=0) {
    lifx_state("off", null, null, e.payload.DURATION_OFF);
  } else if(e.payload.DURATION_ON>=0) {
    lifx_state("on", null, null, e.payload.DURATION_ON);
  } else if(e.payload.DURATION_OFF_MS>=0) {
    lifx_state("off", null, null, e.payload.DURATION_OFF/1000);
  } else if(e.payload.DURATION_ON_MS>=0) {
    lifx_state("on", null, null, e.payload.DURATION_ON/1000);
  }
});

Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL('http://bigbug.github.io/lifx/');
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var config_data = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ', JSON.stringify(config_data));

  localStorage.setItem(0, config_data.lifx_token);
  console.log("Token: " + localStorage.getItem(0));
});