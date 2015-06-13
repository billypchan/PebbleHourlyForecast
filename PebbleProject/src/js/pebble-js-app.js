var LOG_JSON = 0;
var LOG_JSON_HOURLY = 0;
var LOG_MSG = 0;
var LOG_STORE = 0;
var LOG_URL = 0;
var LOG_ICON = 0;
var LOG_WUKEY = 0;

var DEBUG_MUNICH = 0;
var DEBUG_IS_SKIP = 0;
var DEBUG_MOON = 0;
var DEBUG_STRING_LEN = 0;
var DEBUG_DEG_FONT = 0;

var KEY_wunderground = "YOUR_WU_KEY";
var wuKey;
var row4 = "wspd";
var locationType = 1;

///caches
///cache for config.html
var jsonConditions;
var jsonHourly;

///setting
var showPop = 2; ///1 for always show
var isImperial = false;
var isBlack = false;
var isSkip = false;

var xhrRequest = function(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function initWuKey() {
    KEY_wunderground = "YOUR_WU_KEY";
    if (LOG_WUKEY) {
        console.log("initWuKey " + KEY_wunderground);
    }
}

function processCondition(responseText) {

    isImperial = (localStorage.getItem("isImperial") === "true");

    if (LOG_STORE) {
        console.log("isImperial is " + isImperial);
    }

    // responseText contains a JSON object with weather info
    var json = JSON.parse(responseText);

    /*example response:
     {
     "response": {
     "version":"0.1",
     "termsofService":"http://www.wunderground.com/weather/api/d/terms.html",
     "features": {
     "conditions": 1
     }
     }
     ,  "current_observation": {
     "image": {
     "url":"http://icons.wxug.com/graphics/wu2/logo_130x80.png",
     "title":"Weather Underground",
     "link":"http://www.wunderground.com"
     },
     "display_location": {
     "full":"München, Germany",
     "city":"München",
     "state":"",
     "state_name":"Germany",
     "country":"DL",
     "country_iso3166":"DE",
     "zip":"00000",
     "magic":"1",
     "wmo":"10865",
     "latitude":"48.150000",
     "longitude":"11.510000",
     "elevation":"535.00000000"
     },
     "observation_location": {
     "full":"Munich, ",
     "city":"Munich",
     "state":"",
     "country":"DL",
     "country_iso3166":"DE",
     "latitude":"48.34777069",
     "longitude":"11.81338024",
     "elevation":"1486 ft"
     },
     "estimated": {
     },
     "station_id":"EDDM",
     "observation_time":"Last Updated on April 3, 5:20 PM CEST",
     "observation_time_rfc822":"Fri, 03 Apr 2015 17:20:00 +0200",
     "observation_epoch":"1428074400",
     "local_time_rfc822":"Fri, 03 Apr 2015 17:26:02 +0200",
     "local_epoch":"1428074762",
     "local_tz_short":"CEST",
     "local_tz_long":"Europe/Berlin",
     "local_tz_offset":"+0200",
     "weather":"Clear",
     "temperature_string":"48 F (9 C)",
     "temp_f":48,
     "temp_c":9,
     "relative_humidity":"37%",
     "wind_string":"From the West at 8 MPH",
     "wind_dir":"West",
     "wind_degrees":280,
     "wind_mph":8,
     "wind_gust_mph":0,
     "wind_kph":13,
     "wind_gust_kph":0,
     "pressure_mb":"1017",
     "pressure_in":"30.04",
     "pressure_trend":"0",
     "dewpoint_string":"23 F (-5 C)",
     "dewpoint_f":23,
     "dewpoint_c":-5,
     "heat_index_string":"NA",
     "heat_index_f":"NA",
     "heat_index_c":"NA",
     "windchill_string":"NA",
     "windchill_f":"NA",
     "windchill_c":"NA",
     "feelslike_string":"48 F (9 C)",
     "feelslike_f":"48",
     "feelslike_c":"9",
     "visibility_mi":"N/A",
     "visibility_km":"N/A",
     "solarradiation":"--",
     "UV":"-1","precip_1hr_string":"-9999.00 in (-9999.00 mm)",
     "precip_1hr_in":"-9999.00",
     "precip_1hr_metric":"--",
     "precip_today_string":"0.00 in (0.0 mm)",
     "precip_today_in":"0.00",
     "precip_today_metric":"0.0",
     "icon":"clear",
     "icon_url":"http://icons.wxug.com/i/c/k/clear.gif",
     "forecast_url":"http://www.wunderground.com/global/stations/10865.html",
     "history_url":"http://www.wunderground.com/history/airport/EDDM/2015/4/3/DailyHistory.html",
     "ob_url":"http://www.wunderground.com/cgi-bin/findweather/getForecast?query=48.34777069,11.81338024",
     "nowcast":""
     }
     }
     */

     if(!json || !json.current_observation || !json.current_observation.temp_c)
{
    if (LOG_JSON) {
            console.log("error! responseText is " + responseText);
}
        var dictionary = {
        "KEY_TEMPERATURE": "Error getting weather data!"
    };

    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
        function(e) {
            console.log("array info sent to Pebble successfully!");
        },
        function(e) {
            console.log("Error sending weather info to Pebble!");
        }
    );
}

    if (LOG_JSON) {
        console.log("json is " + responseText);
    }

    // Temperature in Kelvin requires adjustment
    //     console.log("isImperial is " + isImperial);
    var temperature = (isImperial) ? Math.round(json.current_observation.temp_f) : Math.round(json.current_observation.temp_c);


    if (LOG_MSG) {

        console.log("Temperature is " + temperature);
    }
    // Conditions
    var conditions = json.current_observation.weather;
    if (LOG_MSG) {
        console.log("Conditions are " + conditions);
    }

    ///current weather icon
    var weatherIcon = json.current_observation.icon_url;
    conditions = "U";

    ///TODO: dictionary
    if (weatherIcon.indexOf("/clear.gif") > -1) {
        conditions = "1";
    }
    if (weatherIcon.indexOf("/sunny.gif") > -1) {
        conditions = "1";
    }
    if (weatherIcon.indexOf("/mostlysunny.gif") > -1) {
        conditions = "1";
    } else if (weatherIcon.indexOf("/nt_clear.gif") > -1) {
        conditions = "a";
    } else if (weatherIcon.indexOf("/partlycloudy.gif") > -1) {
        conditions = "2";
    } else if (weatherIcon.indexOf("/nt_partlycloudy.gif") > -1) {
        conditions = "b";
    } else if (weatherIcon.indexOf("/partlysunny.gif") > -1) {
        conditions = "3";
    } else if (weatherIcon.indexOf("/cloudy.gif") > -1) {
        conditions = "4";
    } else if (weatherIcon.indexOf("/nt_cloudy.gif") > -1) {
        conditions = "4";
    } else if (weatherIcon.indexOf("/rain.gif") > -1) {
        conditions = "D";
    }
    ///use rain drop img 
    else if (weatherIcon.indexOf("/nt_rain.gif") > -1) {
        conditions = "D";
    } else if (weatherIcon.indexOf("/mostlycloudy.gif") > -1) {
        conditions = "4"; ///double cloud
    } else if (weatherIcon.indexOf("/nt_mostlycloudy.gif") > -1) {
        conditions = "4";
    } else if (weatherIcon.indexOf("/storm.gif") > -1) {
        conditions = "F"; ///TODO: S
    } else if (weatherIcon.indexOf("/nt_storm.gif") > -1) {
        conditions = "z";
    } else if (weatherIcon.indexOf("/tstorms.gif") > -1) {
        conditions = "S"; ///TODO: S
    } else if (weatherIcon.indexOf("/nt_tstorms.gif") > -1) {
        conditions = "S";
    } else if (weatherIcon.indexOf("/fog.") > -1) {
        conditions = "6";
    } else if (weatherIcon.indexOf("/nt_fog.") > -1) {
        conditions = "6"; ///TODO: c not shown?
    }

    if (LOG_MSG) {
        weatherIcon
        console.log("weatherIcon is " + weatherIcon);
        console.log("Conditions icon is " + conditions);
    }

    var city = json.current_observation.observation_location.city;
    if (typeof city == 'undefined') {
        city = '';
    }

    if (LOG_MSG) {
        console.log("city is " + city);
        console.log("solarradiation is " + json.current_observation.solarradiation);
    }

    var lenLimit = 10;
        var uv = json.current_observation.UV;

        if(uv <= 0)
        {
            lenLimit = 15;
        }

    var isLongCityName = false;


///no need to cut the city name is uv not shown
    if (city.length > lenLimit) {
        city = city.substring(0, lenLimit);

if(city.slice(-1) == ",")
{
    city = city.substring(0, city.length - 1);
}    

        city += ".";
        isLongCityName = true;
    }

    if (LOG_MSG) {
        console.log("city is " + city);
    }
    var windGust = isImperial ? Math.round(json.current_observation.wind_gust_mph) : Math.round(json.current_observation.wind_gust_kph);
    var wind = windGust;

    var windSpd = isImperial ? Math.round(json.current_observation.wind_mph) : Math.round(json.current_observation.wind_kph);

    if (windSpd > wind || wind == 0) {
        wind = windSpd;
    } else {
        wind = "G" + wind;
    }

    if (wind == "0") {
        wind = "";
    }

    ///TODO: city, feelslike_c, wind_gust_kph
    var feelslike = isImperial ? Math.round(json.current_observation.feelslike_f) : Math.round(json.current_observation.feelslike_c);

    var degree;

    if (temperature != feelslike)
    // if(true)
    {
        degree = temperature + "°\n";
        if (DEBUG_STRING_LEN) {
            feelslike = "100";
        }
        degree += "f" + feelslike + "°\n";
    } else {
        degree = temperature + "°";
    }

    if(DEBUG_DEG_FONT)
    {
        degree = "88°";
    }

    ////wind
    var unit = isImperial ? "mph" : "kmh";
    if (windGust > 0 || windSpd > 0) {
        var dir = json.current_observation.wind_dir;
        if (dir != "Variable") {
            if (LOG_MSG) {
                console.log("dir is " + dir);
            }

            if (dir == "North") {
                dir = "N";
            } else if (dir == "South") {
                dir = "S";
            } else if (dir == "East") {
                dir = "E";
            } else if (dir == "West") {
                dir = "W";
            }

            wind += "" + dir + "";
        } else {
            wind += unit + "";
        }
    }

    if (DEBUG_STRING_LEN) {
        //wind = "G88NWW";
    }

    if (LOG_MSG) {
        console.log("wind is " + wind);
    }

    var msg = "";
    var precip = isImperial ? json.current_observation.precip_1hr_in : json.current_observation.precip_1hr_metric;
    if (!isNaN(precip) && precip > 0) {
        msg += precip + isImperial ? "in " : "mm ";
    }

    msg += city;

    var elevation = Math.floor(json.current_observation.display_location.elevation);
    if (isLongCityName) {
        msg += elevation + "m";
    } else {
        msg += "·" + elevation + "m";
    }
    ///debug
    var timeRaw = json.current_observation.observation_time_rfc822;
    var time = /(\d\d:\d\d):\d\d/.exec(timeRaw)[1];
    if (LOG_MSG) {
        console.log("timeRaw is " + timeRaw);
        console.log("time is " + time);
    }
    msg += "·" + time + "";


    if (uv > 0) {
        msg += "·UV" + uv;
    }
    if (LOG_MSG) {
        console.log("msg is " + msg); ///Muenchen-Stadt·535m·16:00 is too long.
    }

    ///TODO: if msg to too long like muenchen stadt, trim the city name to 14(?) char

    ///debug icon
    var cDebug = json.current_observation.icon_url.match(/\/([\w]*)\.gif/);
    var relative_humidity = json.current_observation.relative_humidity;

    if (LOG_ICON) {
        wind += " " + cDebug[1];
        console.log("icon is " + cDebug[1]);
    }

    wind += " H" + relative_humidity;

    var pressure;
    if (isImperial) {
        pressure = json.current_observation.pressure_in + "in";
    } else {
        pressure = json.current_observation.pressure_mb + "mb";
    }

    wind += " " + pressure;

    ///send the cached moon data every time
    var strMoon = localStorage.getItem("strMoon");

    if (DEBUG_MOON) {
        strMoon = "a";
}

    // Assemble dictionary using our keys
    var dictionary = {
        "KEY_TEMPERATURE": msg,
        "KEY_DEGREE": degree.trim(),
        "KEY_CONDITION": conditions,
        "KEY_WIND": wind,
        "KEY_MOON": strMoon
    };

    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
        function(e) {
            console.log("array info sent to Pebble successfully!");
        },
        function(e) {
            console.log("Error sending weather info to Pebble!");
        }
    );
}

function processHourly(responseText) {
    // responseText contains a JSON object with weather info
    var json = JSON.parse(responseText);

    /*example response:
       {
       "response": {
       "version":"0.1",
       "termsofService":"http://www.wunderground.com/weather/api/d/terms.html",
       "features": {
       "hourly": 1
       }
       }
       ,
       "hourly_forecast": [
       {
       "FCTTIME": {
       "hour": "17","hour_padded": "17","min": "00","min_unpadded": "0","sec": "0","year": "2015","mon": "3","mon_padded": "03","mon_abbrev": "Mar","mday": "31","mday_padded": "31","yday": "89","isdst": "1","epoch": "1427814000","pretty": "5:00 PM CEST on March 31, 2015","civil": "5:00 PM","month_name": "March","month_name_abbrev": "Mar","weekday_name": "Tuesday","weekday_name_night": "Tuesday Night","weekday_name_abbrev": "Tue","weekday_name_unlang": "Tuesday","weekday_name_night_unlang": "Tuesday Night","ampm": "PM","tz": "","age": "","UTCDATE": ""
       },
       "temp": {"english": "50", "metric": "10"},
       "dewpoint": {"english": "40", "metric": "4"},
       "condition": "Rain",
       "icon": "rain",
       "icon_url":"http://icons.wxug.com/i/c/k/rain.gif",
       "fctcode": "13",
       "sky": "100",
       "wspd": {"english": "31", "metric": "50"},
       "wdir": {"dir": "W", "degrees": "276"},
       "wx": "Rain/Wind",
       "uvi": "0",
       "humidity": "70",
       "windchill": {"english": "-9999", "metric": "-9999"},
       "heatindex": {"english": "-9999", "metric": "-9999"},
       "feelslike": {"english": "50", "metric": "10"},
       "qpf": {"english": "0.02", "metric": "1"},
       "snow": {"english": "0.0", "metric": "0"},
       "pop": "81",
       "mslp": {"english": "29.62", "metric": "1003"}
       }
       ,
       
       
       //may rain example
       {
     "FCTTIME": {
     "hour": "14","hour_padded": "14","min": "00","min_unpadded": "0","sec": "0","year": "2015","mon": "4","mon_padded": "04","mon_abbrev": "Apr","mday": "27","mday_padded": "27","yday": "116","isdst": "1","epoch": "1430136000","pretty": "2:00 PM CEST on April 27, 2015","civil": "2:00 PM","month_name": "April","month_name_abbrev": "Apr","weekday_name": "Monday","weekday_name_night": "Monday Night","weekday_name_abbrev": "Mon","weekday_name_unlang": "Monday","weekday_name_night_unlang": "Monday Night","ampm": "PM","tz": "","age": "","UTCDATE": ""
     },
     "temp": {"english": "71", "metric": "22"},
     "dewpoint": {"english": "46", "metric": "8"},
     "condition": "Chance of Rain",
     "icon": "chancerain",
     "icon_url":"http://icons.wxug.com/i/c/k/chancerain.gif",
     "fctcode": "12",
     "sky": "100",
     "wspd": {"english": "5", "metric": "8"},
     "wdir": {"dir": "ESE", "degrees": "104"},
     "wx": "Thundershowers",
     "uvi": "1",
     "humidity": "41",
     "windchill": {"english": "-9999", "metric": "-9999"},
     "heatindex": {"english": "-9999", "metric": "-9999"},
     "feelslike": {"english": "71", "metric": "22"},
     "qpf": {"english": "0.27", "metric": "7"},
     "snow": {"english": "0.0", "metric": "0"},
     "pop": "87",
     "mslp": {"english": "29.54", "metric": "1000"}
     }
     ,
       */
    if (LOG_JSON_HOURLY) {
        console.log("json is " + responseText);
    }

    var temperature = "";
    var wspd = "";
    var pop = "";
    var fctcode = "";
    var hour = "";

    if (LOG_JSON) {
        console.log("json.hourly_forecast.length is " + json.hourly_forecast.length);
    }

    for (var i = 0; i < json.hourly_forecast.length; i++) {
        temperature += isImperial ? Math.round(json.hourly_forecast[i].temp.english) : Math.round(json.hourly_forecast[i].temp.metric);
        temperature += " ";

        if (row4 == "wind") {
            var dir = json.hourly_forecast[i].wdir.dir;
            if (dir == "NNE" || dir == "NNW") {
                dir = "N";
            } else if (dir == "ENE" || dir == "ESE") {
                dir = "E";
            } else if (dir == "SSE" || dir == "SSW") {
                dir = "S";
            } else if (dir == "WNW" || dir == "WSW") {
                dir = "W";
            }

            ///debug text width
            //dir = "NW";

            var wm = isImperial ? Math.round(json.hourly_forecast[i].wspd.english) : Math.round(json.hourly_forecast[i].wspd.metric);
            //wm = 99;
            wspd += wm + dir + " ";
        } else if (row4 == "dewpoint") {
            var dewpoint = isImperial ? Math.round(json.hourly_forecast[i].dewpoint.english) : Math.round(json.hourly_forecast[i].dewpoint.metric);
            wspd += "d" + dewpoint + "° ";
        } else if (row4 == "sky") {
            var str = Math.round(json.hourly_forecast[i].sky);
            wspd += str + "% ";
        } else if (row4 == "uv") {
            var str = json.hourly_forecast[i].uvi;
            // console.log("uv is ***" + str+"***");
            if (!str) {
                wspd += "N/A" + " ";
            } else {
                wspd += "uv" + str + " ";
            }
        } else if (row4 == "humidity") {
            var str = Math.round(json.hourly_forecast[i].humidity);
            wspd += "H" + str + " ";
        } else if (row4 == "feelslike") {
            var str = isImperial ? Math.round(json.hourly_forecast[i].feelslike.english) : Math.round(json.hourly_forecast[i].feelslike.metric);
            wspd += "f" + str + "° ";
        } else if (row4 == "pop") {
            var str = Math.round(json.hourly_forecast[i].pop);
            wspd += "p" + str + " ";
        } else if (row4 == "mslp") {
            var str = isImperial ? Math.round(json.hourly_forecast[i].mslp.english) : Math.round(json.hourly_forecast[i].mslp.metric);
            wspd += str + " ";
        }

        pop += Math.round(json.hourly_forecast[i].pop) + " ";
        hour += Math.round(json.hourly_forecast[i].FCTTIME.hour) + " ";

        var code = Math.round(json.hourly_forecast[i].fctcode);

        if (code <= 9) {} else {
            code = String.fromCharCode(code - 10 + 65);
        }

        ///night icon set
        if (code <= 3) {
            if (json.hourly_forecast[i].icon_url.indexOf("nt_") > -1) {
                code = String.fromCharCode(code - 1 + 97);
            }
        }

        fctcode += code + " ";

        ///if clear and http://icons.wxug.com/i/c/k/clear.gif, use sun
        ///else clear and http://icons.wxug.com/i/c/k/nt_clear.gif, use night
    } ///for
    if (LOG_MSG) {

        console.log("row4 is " + row4);
        console.log("wspd is " + wspd);
        console.log("#wspd is " + wspd.length);

        console.log("hour is " + hour);
        console.log("fctcode is " + fctcode);
        //[PHONE] pebble-app.js:?: JS: Hourly forecast: wspd is 6SW 6SE 6E 8E 11E 13E 13SE 10E 10NW 11NW 13NW 8NW 6W 6W 6W 8W 11W 16NW 19NW 21NW 23W 26W 26W 27W 26W 26W 26W 24W 24W 23W 23W 21W 16W 11W 8W 8W 
        //[PHONE] pebble-app.js:?: JS: Hourly forecast: #wspd is 143
        console.log("pop is " + pop); //pop is 56 2 0 0 4 5 17 4 1 2 2 2 3 3 3 3 3 3 3 3 3 3 3 4 3 17 33 32 24 38 46 44 42 45 57 80 
    }

    var dictionary = {
        "KEY_ARRAY_TEMPERATURE": temperature,
        "KEY_ARRAY_TIME": hour,
        "KEY_ARRAY_CONDITION": fctcode,
        "KEY_ARRAY_WINDSPD": wspd, ///TODO: can be replace with row4 setting
        "KEY_ARRAY_POP": pop
    };

    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
        function(e) {
            console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
            console.log("Error sending weather info to Pebble!");
        }
    );
}

function locationSuccess(pos) {
    ///request moon phrase every day
    var moonTime = localStorage.getItem("moonTime");
    var curTime = new Date().getTime() / 1000;

    var diffTime = curTime - moonTime;

    if (LOG_MSG) {
        console.log("moonTime " + moonTime);
        console.log("curTime - moonTime " + diffTime);
    }

    var timeReload = 60 * 60 * 24;
    if (DEBUG_MOON) {
        // moonTime = null;
        timeReload = 1;
    }

    if (moonTime == null || diffTime > timeReload) ///TODO: better cycle for moon?
    {
        if (LOG_MSG) {
            console.log("getting moon data...");
        }
        moonTime = curTime
        localStorage.setItem("moonTime", moonTime);

        var url = "http://api.wunderground.com/api/" + KEY_wunderground + "/astronomy/q/" +
            pos.coords.latitude + "," + pos.coords.longitude + ".json";

        if (locationType == 2) {
            url = "http://api.wunderground.com/api/" + KEY_wunderground + "/astronomy/q/" +
                localStorage.getItem("city") + ".json";

        } else if (locationType == 3) {
            ///TODO: no need to wait for GPS?
            url = "http://api.wunderground.com/api/" + KEY_wunderground + "/astronomy/q/" +
                localStorage.getItem("coordinateLat") + "," + localStorage.getItem("coordinateLog") + ".json";

        }

        xhrRequest(url, 'GET',
            function(responseText) {
                var json = JSON.parse(responseText);

                //console.log("jsonMoon is " + jsonMoon);
                var ageOfMoon = Math.round(json.moon_phase.ageOfMoon);
                console.log("ageOfMoon is " + ageOfMoon);

                var strMoon = "a";

                if (ageOfMoon < 26) {
                    strMoon = String.fromCharCode(ageOfMoon + 97);
                } else {
                    strMoon = String.fromCharCode(ageOfMoon - 26 + 65);
                }

                console.log("strMoon is " + strMoon);
                localStorage.setItem("strMoon", strMoon);

    if (DEBUG_MOON) {
        strMoon = "a";
}
                var dictionary = {
                    "KEY_MOON": strMoon
                };

                // Send to Pebble
                Pebble.sendAppMessage(dictionary,
                    function(e) {
                        console.log("Moon info sent to Pebble successfully!");
                    },
                    function(e) {
                        console.log("Error sending weather info to Pebble!");
                    }
                );

            }
        );

    }


    ///skip half of requests. i.e. called every 2 hours.
    if (!wuKey || wuKey == "null") {
        isSkip = (localStorage.getItem("isSkip") === "true");

        console.log("skipped = " + isSkip);
        localStorage.setItem("isSkip", !isSkip);

        if (DEBUG_IS_SKIP) {
            isSkip = false;
        }

        if (isSkip == true) {
            console.log("skipped locationSuccess");
            isSkip = !isSkip;

            console.log("skipped = " + isSkip);
            return;
        }
    } else {
        isSkip = false;
    }

    isImperial = (localStorage.getItem("isImperial") === "true");
    if (LOG_MSG) {
        console.log("isImperial is " + isImperial);
    }

    {
        // Construct URL

        //Munich is 48.133333, 11.566667

        var url;
        if (locationType == 1) {
            url = "http://api.wunderground.com/api/" + KEY_wunderground + "/conditions/q/" +
                pos.coords.latitude + "," + pos.coords.longitude + ".json";
        }
        if (locationType == 2) {
            url = "http://api.wunderground.com/api/" + KEY_wunderground + "/conditions/q/" +
                localStorage.getItem("city") + ".json";

        } else if (locationType == 3) {
            ///TODO      
            url = "http://api.wunderground.com/api/" + KEY_wunderground + "/conditions/q/" +
                localStorage.getItem("coordinateLat") + "," + localStorage.getItem("coordinateLog") + ".json";
        }

        if (DEBUG_MUNICH == 1) {
            url = "http://api.wunderground.com/api/" + KEY_wunderground + "/conditions/q/" + "48.1333" + "," + "11.5666" + ".json";
        }


        if (LOG_URL) {
            console.log("url is " + url); //url is http://api.wunderground.com/api/00c3b0edc15c35d8/conditions/q/48.13822030791464,11.565617507642278.json
        }

        // Send request to OpenWeatherMap
        xhrRequest(url, 'GET',
            function(responseText) {
                jsonHourly = responseText;
                processCondition(responseText); {
                    // Construct URL

                    var url;
                    if (locationType == 1) {
                        url = "http://api.wunderground.com/api/" + KEY_wunderground + "/hourly/q/" +
                            pos.coords.latitude + "," + pos.coords.longitude + ".json";

                    }
                    if (locationType == 2) {
                        url = "http://api.wunderground.com/api/" + KEY_wunderground + "/hourly/q/" +
                            localStorage.getItem("city") + ".json";

                    } else if (locationType == 3) {
                        ///TODO      
                        url = "http://api.wunderground.com/api/" + KEY_wunderground + "/hourly/q/" +
                            localStorage.getItem("coordinateLat") + "," + localStorage.getItem("coordinateLog") + ".json";
                    }

                    if (DEBUG_MUNICH == 1) {
                        url = "http://api.wunderground.com/api/" + KEY_wunderground + "/hourly/q/" +
                            "48.1333" + "," + "11.5666" + ".json";
                    }

                    if (LOG_URL) {
                        console.log("url is " + url); //url is http://api.wunderground.com/api/00c3b0edc15c35d8/conditions/q/48.13822030791464,11.565617507642278.json
                    }

                    // Send request to OpenWeatherMap
                    xhrRequest(url, 'GET',
                        function(responseText) {
                            jsonConditions = responseText;
                            processHourly(jsonConditions);
                        }
                    );
                }

            }
        );
    }
}

function locationError(err) {
    console.log("Error requesting location!");
}

function getWeatherWithLocation() {
    console.log("getWeatherWithLocation");
    locationSuccess(0);
}

function getWeather() {
    console.log("getWeather");
    navigator.geolocation.getCurrentPosition(
        locationSuccess,
        locationError, {
            timeout: 30000, ///ori is 15000
            maximumAge: 60000
        }
    );
}

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
    function(e) {
        console.log("AppMessage received!");
        // getWeather();
        getWeatherUpdate();

    }
);

var options;

var isBlackOri;
var isImperialOri;
var row4Ori;
var locationTypeOri;
var logOri;
var latOri;
var cityOri;

Pebble.addEventListener("showConfiguration", function() {
    console.log("showing configuration");
    isBlackOri = isBlack;
    isImperialOri = isImperial;
    row4Ori = row4;
    locationTypeOri = locationType;
    logOri = localStorage.getItem("coordinateLog");
    latOri = localStorage.getItem("coordinateLat");
    cityOri = localStorage.getItem("city");

    options = localStorage.getItem('options') ? localStorage.getItem('options') : "";

    console.log("options read " + options);
    var url = 'http://www.googledrive.com/host/0B8BtI1iYN4hfeDA1azhJOUZyN0E?' + encodeURIComponent(options);
    Pebble.openURL(url);

    console.log("url " + url);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    // webview closed
    //Using primitive JSON validity and non-empty check
    if (e.response.charAt(0) == "{" && e.response.slice(-1) == "}" && e.response.length > 5) {
        options = JSON.parse(decodeURIComponent(e.response));
        console.log("Options = " + JSON.stringify(options));
        localStorage.setItem("options", JSON.stringify(options));


        locationType = localStorage.getItem("locationType");
        var city;
        if (options["radio-auto"]) {
            locationType = 1;
        } else if (options["radio-city"]) {
            locationType = 2;
            var c = options["text-city"].replace(/\s+/g, '');
            localStorage.setItem("city", c);
            city = c;
        } else if (options["radio-coord"]) {
            locationType = 3;
            localStorage.setItem("coordinateLog", options["text-coordinateLog"]);
            localStorage.setItem("coordinateLat", options["text-coordinateLat"]);
        }
        localStorage.setItem("locationType", locationType);

        /////////
        if (options["radio-dewpoint"]) {
            row4 = "dewpoint";
        } else if (options["radio-sky"]) {
            row4 = "sky";
        } else if (options["radio-wind"]) {
            row4 = "wind";
        } else if (options["radio-uv"]) {
            row4 = "uv";
        } else if (options["radio-humidity"]) {
            row4 = "humidity";
        } else if (options["radio-feelslike"]) {
            row4 = "feelslike";
        } else if (options["radio-pop"]) {
            row4 = "pop";
        } else if (options["radio-mslp"]) {
            row4 = "mslp";
        }
        if (LOG_MSG) {
            console.log("row4 = " + row4);
        }
        localStorage.setItem("row4", row4);

        /////////
        if (options["wuKey"]) {
            KEY_wunderground = options["wuKey"];
            if (LOG_WUKEY) {
                console.log("wuKey = " + KEY_wunderground);
            }
            localStorage.setItem("wuKey", KEY_wunderground);
        } else {
            localStorage.setItem("wuKey", null);
        }

        ////////////////////////////
        if (options["pop-choice-1"]) {
            showPop = 1;
        } else if (options["pop-choice-2"]) {
            showPop = 2;
        } else if (options["pop-choice-3"]) {
            showPop = 3;
        }
        if (LOG_MSG) {

            console.log("showPop = " + showPop);
        }
        localStorage.setItem("showPop", showPop);

        ////////////////////////////
        isImperial = options["radio-choice-1"];
        if (LOG_MSG) {

            console.log("isImperial = " + isImperial);
        }
        localStorage.setItem("isImperial", isImperial);

        if (LOG_MSG) {
            console.log("locationTypeOri = " + locationTypeOri);
            console.log("locationType = " + locationType);
        }

        ///recreate the msg and send to pebble
        if (isImperialOri != isImperial ||
            row4Ori != row4 ||
            locationTypeOri != locationType ||
            logOri != localStorage.getItem("coordinateLog") ||
            latOri != localStorage.getItem("coordinateLat") ||
            city != cityOri
        ) {
            localStorage.setItem("isSkip", false);
            // getWeather();
            getWeatherUpdate();

        }
        ////////////////////////////

        isBlack = options["color-choice-1"];
        console.log("isBlack = " + isBlack);

        if (isBlack) {
            isBlack = 1;
        } else {
            isBlack = 0;
        }

        localStorage.setItem("isBlack", isBlack);

        // if (isBlackOri != isBlack) {
        // Assemble dictionary using our keys
        var dictionary = {
            "KEY_POP": showPop,
            "KEY_COLOR": isBlack
        };

        Pebble.sendAppMessage(dictionary,
            function(e) {
                console.log("setting info sent to Pebble successfully!");
            },
            function(e) {
                console.log("Error: setting sending weather info to Pebble!");
            }
        );
        //} 
        // else {
        //   console.log("isBlack same as before");
        // }

    } else {
        console.log("Cancelled");
    }
});

function getWeatherUpdate() {
    if (locationType == 1) {
        getWeather();
    } else {
        getWeatherWithLocation();
    }
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready',
    function(e) {
        console.log("Hourly forecast: PebbleKit JS ready!");

        isImperial = (localStorage.getItem("isImperial") === "true");
        isBlack = (localStorage.getItem("isBlack") === "true");

        wuKey = localStorage.getItem("wuKey");
        row4 = localStorage.getItem("row4");
        locationType = localStorage.getItem("locationType");

        if (!locationType) {
            locationType = 1;
            localStorage.setItem("locationType", locationType);
        }
        console.log("ready, locationType = " + locationType);

        if (LOG_WUKEY) {
            console.log("ready, wuKey = " + wuKey);
            console.log("ready, #wuKey = " + wuKey.length);
        }
        if (!wuKey || wuKey == "null") {
            if (LOG_WUKEY) {
                console.log("ready, no wuKey case!");
            }
            initWuKey();
        }

        if (LOG_WUKEY) {
            console.log("ready, after initWuKey(), wuKey = " + wuKey);
        }

        if (!wuKey || wuKey == null || wuKey == "null") {
            var isSkipVar = localStorage.getItem("isSkip");
            // console.log("ready, isSkipVar = " + isSkipVar );

            isSkip = (localStorage.getItem("isSkip") === "true");

            ///first install, always load
            if (isSkipVar == null) {
                isSkip = false;
            }
        } else {

            if (LOG_WUKEY) {
                console.log("ready, has wuKey case!");
            }
            KEY_wunderground = wuKey;
            isSkip = false;
        }

        if (KEY_wunderground == null || KEY_wunderground === null || KEY_wunderground.length == 0 || KEY_wunderground === 'undefined') {
            if (LOG_WUKEY) {
                console.log("ready, KEY_wunderground is null!");
            }
            initWuKey();
        }

        if (LOG_WUKEY) {
            console.log("ready, isSkip = " + isSkip);
            console.log("ready, KEY_wunderground = " + KEY_wunderground);
        }

        // Get the initial weather
        getWeatherUpdate();
    }
);