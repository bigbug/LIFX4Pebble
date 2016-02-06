function lifx_getState() {
  var xhr = new XMLHttpRequest();
  xhr.open( 'GET', 'https://api.lifx.com/v1/lights/all', true);
  xhr.setRequestHeader( 'Authorization', 'Bearer ' + localStorage.getItem(0) );
  //xhr.withCredentials = true;
  xhr.onreadystatechange = function() {
      if (xhr.readyState == 4 && xhr.status == 200) {
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
      }
  };
  xhr.send();
}

function lifx_toggle() {
  var xhr = new XMLHttpRequest();
  xhr.open( 'POST', 'https://api.lifx.com/v1/lights/all/toggle', true);
  xhr.setRequestHeader( 'Authorization', 'Bearer ' + localStorage.getItem(0) );
  xhr.onreadystatechange = function() {
      console.log( "state status", xhr.status, xhr.statusText);
      //lifx_getState();
  };
  xhr.send();
}

function lifx_state(powerState, color, brightness, duration) {
  
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
  } else if(e.payload.DURATION_OFF>0) {
    lifx_state("off", null, null, e.payload.DURATION_OFF);
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