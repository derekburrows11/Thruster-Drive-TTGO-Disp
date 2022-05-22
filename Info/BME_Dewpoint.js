/* Split the inbound message into separate topics in Homie-like format
//
// This is for input only and combined input/output devices that need Node-RED
// to output to homie style MQTT topics.
//
// Output only devices like Lightwave switches are handled separately.
*/

// msg.meta = {"TH1/0x7D01": {
//              location: "HOME/IN/00/LIVING",
//              type: "Sensor", 
//              description: "Oregon Scientific Temperature/Humidity Sensor"}
//            }

// Setup {
var topicRoot = 'homie/';
var topics = msg.topic.split('/');
var deviceID = topics[1];
var deviceTopicRoot = topicRoot + deviceID;
var sensorTopicRoot = deviceTopicRoot + '/sensors';
var sensorsType = '';
var sensorList = [];
var lastUpdate = new Date();
// }

// output to port 2 if payload is not an object as this is an unknown device type
//if ( (typeof msg.payload) !== 'object' ) {
//    return [null,msg]
//}

// Standard info for all input devices {
node.send([{ 'topic': deviceTopicRoot + '/$implementation', 'payload': 'nrlive/' + topics[0] }]);
node.send([{ 'topic': deviceTopicRoot + '/$name', 'payload': msg.meta.description }]);
node.send([{ 'topic': deviceTopicRoot + '/$location', 'payload': msg.meta.location }]);
node.send([{ 'topic': deviceTopicRoot + '/$type', 'payload': msg.meta.type }]);
node.send([{ 'topic': deviceTopicRoot + '/$source', 'payload': msg.src }]);
node.send([{ 'topic': deviceTopicRoot + '/$stats/$updated', 'payload': lastUpdate }]);
// }

// -- Device Specific -- // {
if ( msg.topic === 'WIFI/SP01' ) edimaxSP2101W();
// --------------------- // }

// -- rssi -- // {
var rssi = getProps(['rssi', 'status.rssi', 'payload.rssi', 'payload.status.rssi']);
if ( rssi !== '' ) {
    node.send( [{ 'topic': deviceTopicRoot + '/$stats/signal', 'payload': rssi }] );
}
// }

// -- battery -- // {
var battery = getProps(['battery', 'status.battery', 'payload.battery', 'payload.status.battery']);
if ( battery !== '' ) {
    node.send( [{ 'topic': deviceTopicRoot + '/$stats/battery', 'payload': battery }] );
}
// }

// ======= SENSOR DATA ======== //

// -- Temperature -- // {
var temperature = getProps(['payload.Temperature', 'payload.temperature.value', 'payload.temperature']);
if ( temperature !== '' ) {
    sensorList.push( 'temperature' );
    sensorsType = 'sensors';
    node.send( [{ 'topic': sensorTopicRoot + '/temperature', 'payload': temperature }] );
}
// }

// -- Humidity -- // {
var humidity = getProps(['payload.Humidity', 'payload.humidity.value', 'payload.humidity']);
if ( humidity !== '' ) {
    sensorList.push( 'humidity' );
    sensorsType = 'sensors';
    node.send( [{ 'topic': sensorTopicRoot + '/humidity', 'payload': humidity }] );
}
// }

// -- Light -- // {
var light = getProps(['payload.Light', 'payload.light.value', 'payload.light', 'LDR']);
if ( light !== '' ) {
    sensorList.push( 'light' );
    sensorsType = 'sensors';
    node.send( [{ 'topic': sensorTopicRoot + '/light', 'payload': light }] );
}
// }

// -- Pressure -- // {
var pressure = getProps(['payload.Pressure', 'payload.pressure.value', 'payload.pressure']);
if ( pressure !== '' ) {
    var sealevel = getProps(['payload.Pressure_Sealevel', 'payload.PressureSealevel', 'payload.pressureSealevel', 'payload.pressure_sealevel']);
    if ( sealevel === '' ) {
        var A = 201; //Altitude of sensor  <<-- TODO: replace with lookup from master device object (HOME/deviceLocations)
        var P = pressure;
    	sealevel = round( P/Math.pow( (1-(A/44330.0)), 5.255 ), 2 );        
    }
    sensorList.push( 'pressure' );
    sensorsType = 'sensors';
    node.send( [{ 'topic': sensorTopicRoot + '/pressure', 'payload': sealevel }] );
    node.send( [{ 'topic': sensorTopicRoot + '/pressure_abs', 'payload': pressure }] );
}
// }

// -- Add extra data on combinations -- //

// Calculate dewpoint and heatindex
if ( doesContain(['temperature','humidity'], sensorList) ) {
    heatindex(temperature, humidity);
    dewpoint(temperature, humidity);
}

// -- Output sensor list and type (or debug msg if none) -- // {
    if ( sensorsType !== '' ) {
        node.send( [{ 'topic': sensorTopicRoot + '/$type', 'payload': sensorsType }] );
        node.send( [{ 'topic': sensorTopicRoot + '/$properties', 'payload': sensorList.join(',') }] );
    }
// }

// ====== COMMANDS ====== // {
if ( (msg.src === 'RFX/Lights') | msg.hasOwnProperty('unit') | msg.payload.hasOwnProperty('command') ) {

    // Work out the command and optionally, the unit {
    var command = '', unit = '';
    // msg.command/.unit?
    if ( msg.hasOwnProperty('command') ) {
        command = msg.command;
    }
    if ( msg.hasOwnProperty('unit') ) {
        unit = msg.unit;
    }
    // payload.command/.unit takes preference
    if ( typeof msg.payload === 'object' ) {
        if ( msg.payload.hasOwnProperty('command') ) {
            command = msg.command;
        }
        if ( msg.payload.hasOwnProperty('unit') ) {
            unit = msg.unit;
        }
    } else {
        command = msg.payload;
    }
    // }
    
    // If the command is 1/0 or true/false, convert to standard On/Off {
    if ( (command === 1) || (command === true) ) {
        command = 'On';
    }
    if ( (command === 0) || (command === false) ) {
        command = 'Off';
    }
    // }
    
    if ( command !== '' ) {

        // Record unit command
        if ( unit !== '' ) { // For unit based commands only
            const strUtopic = deviceTopicRoot + '/unit/' + unit;
            // What command recieved
            node.send( [{ 'topic': strUtopic + '/command', 'payload': command }] );
            // When command recieved
            node.send( [{ 'topic': strUtopic + '/timestamp', 'payload': lastUpdate }] );
            // Record last time this command recieved
            node.send( [{ 'topic': strUtopic + '/last' + command, 'payload': lastUpdate }] );
            
            // Save unit command times (in ms since 1970/1/1) {
            const unitCmdTimes = global.get('unitCmdTimes') || {};
            if ( ! unitCmdTimes.hasOwnProperty(strUtopic) ) {
                unitCmdTimes[strUtopic] = {'lastCmd': null, 'lastOn': null, 'lastOff': null};
            }
            unitCmdTimes[strUtopic]['last' + command] = lastUpdate.getTime();
            // }

            // Send elapsed on/off times {
            // Don't update if new cmd is same as old.
            if ( command !== unitCmdTimes[strUtopic].lastCmd ) {
                if ( command === 'On' ) {  // How long was it off? msOn-msOff
                    const msOn = lastUpdate.getTime();
                    const msOff = unitCmdTimes[strUtopic].lastOff || null;
                    if ( msOff ) {
                        node.send( [{ 'topic': strUtopic + '/lastOffFor', 'payload': toHHMMSS( (msOn - msOff) ) }] );
                    }
                }
                if ( command === 'Off' ) {  // How long was it off? mmOff-msOn
                    const msOn  = unitCmdTimes[strUtopic].lastOn || null;
                    const msOff = lastUpdate.getTime();
                    if ( msOn ) {
                        node.send( [{ 'topic': strUtopic + '/lastOnFor', 'payload': toHHMMSS( (msOff - msOn) ) }] );
                    }
                }
            }
            // }

            unitCmdTimes[strUtopic].lastCmd = command;
            
            global.set('unitCmdTimes', unitCmdTimes);
        }

        // Record latest command {
        node.send( [{ 'topic': deviceTopicRoot + '/command/cmd', 'payload': command }] );
        node.send( [{ 'topic': deviceTopicRoot + '/command/unit', 'payload': unit }] );
        node.send( [{ 'topic': deviceTopicRoot + '/command/timestamp', 'payload': lastUpdate }] );
        node.send( [{ 'topic': deviceTopicRoot + '/command', 'payload': { 'cmd': command, 'unit': unit } }] );
        node.send( [{ 'topic': deviceTopicRoot + '/command/last' + command, 'payload': lastUpdate }] );
        // }
    }
}
// --- End of Command output --- // }

//return msg;
// =========== DEVICE SPECIFIC FUNCTIONS =========== // {

function edimaxSP2101W() {
    // TODO: Change fixed IP to dynamic (from ARP data)
    
    const sp01 = global.get('sp01') || {'state': null, 'ip': '192.168.1.103', 'lastOn': null, 'lastOff':null, 'lastCmd': null};
    node.send([null,{global: { 'sp01': sp01 }}]);
    
    const info = getProps('payload.deviceinfo');
    node.send([{ 'topic': deviceTopicRoot + '/$online', 'payload': true }]);
    node.send([{ 'topic': deviceTopicRoot + '/$mac', 'payload': translateMac(info.mac) }]);
    node.send([{ 'topic': deviceTopicRoot + '/$localip', 'payload': sp01.ip }]);
    node.send([{ 'topic': deviceTopicRoot + '/$fw/name', 'payload': info.model }]);
    node.send([{ 'topic': deviceTopicRoot + '/$fw/version', 'payload': info.fwVersion }]);
    
    const status = getProps('payload.status');
    sensorsType = 'sensors';
    sensorList.push( 'power' );
    node.send( [{ 'topic': sensorTopicRoot + '/power', 'payload': status.nowPower }] );
    sensorList.push( 'current' );
    node.send( [{ 'topic': sensorTopicRoot + '/current', 'payload': status.nowCurrent }] );
    sensorList.push( 'stats' );
    node.send( [{ 'topic': sensorTopicRoot + '/stats', 'payload': { 'power':{'day': status.day, 'week': status.week, 'month': status.month}, 'cost':status.cost} }] );

    // Update command status if needed
    if ( sp01.state !== status.state ) {
        sp01.state = status.state;
        const cmd = status.state ? 'On' : 'Off';
        node.send( [{ 'topic': deviceTopicRoot + '/command', 'payload': { 'cmd': cmd } }] );
        node.send( [{ 'topic': deviceTopicRoot + '/command/cmd', 'payload': cmd }] );
        node.send( [{ 'topic': deviceTopicRoot + '/command/timestamp', 'payload': status.lastToggleTime }] );
        node.send( [{ 'topic': deviceTopicRoot + '/command/last' + cmd, 'payload': status.lastToggleTime }] );
        // Also sent pseudo "unit" entries to match LightwaveRF, etc
        node.send( [{ 'topic': deviceTopicRoot + '/command/unit', 'payload': 0 }] );
        node.send( [{ 'topic': deviceTopicRoot + '/unit/0/command', 'payload': cmd }] );
        node.send( [{ 'topic': deviceTopicRoot + '/unit/0/timestamp', 'payload': status.lastToggleTime }] );
        node.send( [{ 'topic': deviceTopicRoot + '/unit/0/last' + cmd, 'payload': status.lastToggleTime }] );

        // Save unit command times (in ms since 1970/1/1) 
        const lastToggle = new Date(status.lastToggleTime)
        sp01['last' + cmd] = lastToggle.getTime();

        // Save elapsed on/off times {
        // Don't update if new cmd is same as old.
        if ( command !== sp01.lastCmd ) {
            if ( cmd === 'On' ) {  // How long was it off? msOn-msOff
                const msOn = lastToggle.getTime();
                const msOff = sp01.lastOff;
                if ( msOff ) {
                    node.send( [{ 'topic': deviceTopicRoot + '/unit/0/lastOffFor', 'payload': toHHMMSS( (msOn - msOff) ) }] );
                }
            }
            if ( cmd === 'Off' ) {  // How long was it off? mmOff-msOn
                const msOn  = sp01.lastOn;
                const msOff = lastToggle.getTime();
                if ( msOn ) {
                    node.send( [{ 'topic': deviceTopicRoot + '/unit/0/lastOnFor', 'payload': toHHMMSS( (msOff - msOn) ) }] );
                }
            }
        }
        // }
        
        sp01.lastCmd = cmd;
    }

    global.set('sp01', sp01);
}

// =========== UTILITY FUNCTIONS =========== //

function getProps(props) {
    if ( (typeof props) === 'string' ) {
        props = [props];
    }
    if ( ! Array.isArray(props) ) {
        return undefined;
    }
    let ans = '';
    for (var i = 0; i < props.length; i++) {
        try { // breaks if an intermediate property doesn't exist
            ans = RED.util.getMessageProperty(msg, props[i]);
            if ( typeof ans !== 'undefined' ) {
                break;
            }
        } catch(e) { 
            // Do nothing
        }
    }
    return ans || '';
}

// Is arrCheck a subset of arrSuperSet?
function doesContain(arrCheck,arrSuperSet) {
    return arrCheck.every(elem => arrSuperSet.indexOf(elem) > -1);
}

// Convert ms to hh:mm:ss (ES2015+)
function toHHMMSS(mSecs) {
    const secs = mSecs / 1000;
    
    const sec_num = parseInt(secs, 10);
    const hours   = Math.floor(sec_num / 3600) % 24;
    const minutes = Math.floor(sec_num / 60) % 60;
    const seconds = sec_num % 60;
    
    return [hours,minutes,seconds]
        .map(v => v < 10 ? "0" + v : v)
        .join(":");
        // Put the following between map and join if you want to remove zero hours/minutes
        //.filter((v,i) => v !== "00" || i > 0)
}

// translate mac address string into : separated mac address string
function translateMac(mac) {
    const s = mac.split('');
    return s[0] + s[1] + ':' + s[2] + s[3] + ':' + s[4] + s[5] + ':' + s[6] + s[7] + ':' + s[8] + s[9] + ':' + s[10] + s[11];
}

// Output dewpoint
function dewpoint(temperature, humidity){
    if ( typeof temperature === 'string' ) temperature = parseFloat(temperature)
    if ( typeof humidity === 'string' ) humidity = parseFloat(humidity)
    sensorList.push( 'dewpoint' )
    sensorsType = 'sensors'
    if ( !('dewpoint' in msg.payload) ) {
        node.send( [{ 'topic': sensorTopicRoot + '/dewpoint', 'payload': dewPointFast(temperature, humidity) }] )
    } else {
        node.send( [{ 'topic': sensorTopicRoot + '/dewpoint', 'payload': msg.payload.dewpoint }] )
    }
}

// Output heatindex
function heatindex(temperature, humidity){
    if ( typeof temperature === 'string' ) temperature = parseFloat(temperature)
    if ( typeof humidity === 'string' ) humidity = parseFloat(humidity)
    sensorList.push( 'heatindex' )
    sensorsType = 'sensors'
    if ( !('heatindex' in msg.payload) ) {
        node.send( [{ 'topic': sensorTopicRoot + '/heatindex', 'payload': computeHeatIndex(temperature, humidity, false) }] )
    } else {
        node.send( [{ 'topic': sensorTopicRoot + '/heatindex', 'payload': msg.payload.heatindex }] )
    }
}

function round(mynum, decimals) {
    if ( typeof mynum === 'string' ) mynum = parseFloat(mynum)
    var n = Math.pow(10, decimals);
    return Math.round( (n * mynum).toFixed(decimals) )  / n;
}

function convertCtoF(c) {
    if ( typeof c === 'string' ) c = parseFloat(c)
    return c * 1.8 + 32;
}
function convertFtoC(f) {
    if ( typeof f === 'string' ) f = parseFloat(f)
    return (f - 32) * 0.55555;
}

// See: http://arduinotronics.blogspot.co.uk/2013/12/temp-humidity-w-dew-point-calcualtions.html {
// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point }
function dewPointFast(celsius, humidity) {
    if ( typeof celsius === 'string' ) celsius = parseFloat(celsius)
    if ( typeof humidity === 'string' ) humidity = parseFloat(humidity)
    var a = 17.271;
    var b = 237.7;
    var temp = Math.log(humidity * 0.01) + ((a * celsius) / (b + celsius));
    var Td = (b * temp) / (a - temp);
    return round( Td, 2 );
}
/*
function dCalc(humidity, celcius) {
	//iMV is the intermidate Value 
	var iMV = (Math.log(humidity / 100)+((17.27 * celcius)/(237.3 + celcius)))/17.27;
	var dewPoint = (237.3 *iMV)/(1-iMV);
	return dewPoint.toFixed(2);
}
function computeDew1(temp, rh) {
    if ( (temp === null || temp.length === 0) ||
        (rh === null || rh.length === 0) ) {
        return;
    }
    if (rh >= 85) {
        dewp = temp-0.133 * ( 100.0-rh );
    }    
    if (rh < 85) {
        dewp = temp-0.2 * ( 95.0-rh );
    }    
    return (temp - dewp).toPrecision(4);
}
function computeDew2(temp, rh) {
    if ( (temp === null || temp.length === 0) ||
        (rh === null || rh.length === 0) ) {
        return;
    }
    tem2 = temp;
    tm = -1.0*temp;
    es = 6.112*Math.exp(-1.0*17.67*tm/(243.5 - tm));
    ed = rh/100.0*es;
    eln = Math.log(ed/6.112);
    dewp = -243.5*eln/(eln - 17.67 );
    return (temp - dewp).toPrecision(4);
}
*/

//boolean isFahrenheit: True == Fahrenheit; False == Celcius
function computeHeatIndex(temperature, percentHumidity, isFahrenheit) {
    if ( typeof temperature === 'string' ) temperature = parseFloat(temperature)
    if ( typeof percentHumidity === 'string' ) percentHumidity = parseFloat(percentHumidity)
    
    // Using both Rothfusz and Steadman's equations
    // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
    var hi;

    if (!isFahrenheit) temperature = convertCtoF(temperature);

    hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

  if (hi > 79) {
    hi = -42.379 +
             2.04901523 * temperature +
            10.14333127 * percentHumidity +
            -0.22475541 * temperature * percentHumidity +
            -0.00683783 * Math.pow(temperature, 2) +
            -0.05481717 * Math.pow(percentHumidity, 2) +
             0.00122874 * Math.pow(temperature, 2) * percentHumidity +
             0.00085282 * temperature * Math.pow(percentHumidity, 2) +
            -0.00000199 * Math.pow(temperature, 2) * Math.pow(percentHumidity, 2);

    if((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25) * Math.sqrt((17.0 - Math.abs(temperature - 95.0)) * 0.05882);

    else if((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  return isFahrenheit ? round( hi, 2 ) : round( convertFtoC(hi), 2 );
}

/*
function heatIndex(tempC, humidity) {
    var tempF = convertCtoF(tempC);
    var c1 = -42.38, c2 = 2.049, c3 = 10.14, c4 = -0.2248, c5= -6.838e-3, c6=-5.482e-2, c7=1.228e-3, c8=8.528e-4, c9=-1.99e-6  ;
    var T = tempF;
    var R = humidity;
    
    var A = (( c5 * T) + c2) * T + c1;
    var B = ((c7 * T) + c4) * T + c3;
    var C = ((c9 * T) + c8) * T + c6;
    
    var rv = (C * R + B) * R + A;
    return round( rv, 1 );
}
*/
// EOF }