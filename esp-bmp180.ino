#include <Adafruit_BMP085_U.h>
#include <Arduino.h>
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <Adafruit_Sensor.h>

#include <Streaming.h>

// wenn aktiv: Meldungen per Serial
#define SERIAL

// wenn auskommentiert: delay()
// wenn aktiv: deep sleep nutzen (Hardwaremodifikation notwendig)
#define DEEP

//sekunden zwischen Aufwachvorgängen
//#define WAIT 120
#define WAIT 600  //wait 10min in deepsleep

const String host = "192.168.2.230"; //Volkeszaehler Server IP
const unsigned int port = 80;

const String url_start = "/middleware.php/data/";
const String url_stop = ".json?operation=add&value=";

//fill with right values
const String uuid_temp3 = "00000";
const String uuid_pres = "11111";

byte maxwait = 160; //Wifi must connect in < x seconds

ESP8266WiFiMulti WiFiMulti;

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

void displaySensorDetails(void)
{
  sensor_t sensor;
  bmp.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" hPa");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" hPa");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" hPa");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void setup() {
  #ifdef SERIAL
    Serial.begin(115200);
    Serial.println("BOOT");

    Serial.print("Wifi...");
  #endif

  //WiFiMulti.addAP("Hades","t4k4tuk4l4nd2");
  WiFiMulti.addAP("ESP-net", "35p-r0cks-iot");
  WiFiMulti.addAP("ocktown_d0wn911","t4k4tuk4l4nd1");
  //WiFiMulti.addAP("ViditFabrik","%m1n!5NK=$");
  
  #ifdef SERIAL
    Serial.println("OK");
    
  #endif
    
  #ifdef SERIAL
  Serial.println("Pressure Sensor Test...."); Serial.println("");
  
  /* Initialise the sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  else{
    #ifdef SERIAL
    Serial.println("OK");
    #endif
    }
  
  /* Display some basic information on this sensor */
  displaySensorDetails();
  #endif
}

bool waitWifi() {
  while((WiFiMulti.run() != WL_CONNECTED) && maxwait > 0) {
      #ifdef SERIAL
        Serial.println("Wait Wifi");
      #endif
      delay(1000);
      maxwait--;
  }

  if(WiFiMulti.run() == WL_CONNECTED) return true;
      Serial.println(WiFi.localIP());           // for debug
  return false;
}

void sendHttpData(String url) {
    HTTPClient http;

    if(waitWifi()) {

      #ifdef SERIAL
        Serial.print("GET: "); Serial.println(url);   // for debug auskommentiert
      #endif
      http.begin(host, port, url); //HTTP
      int httpCode = http.GET();
      #ifdef SERIAL
      if(httpCode) {
          if(httpCode == 200) {
              String payload = http.getString();
              Serial.println(payload);
          }else{
            Serial.print("HTTP "); Serial.println(httpCode);
          }
      } else {
          Serial.print("[HTTP] GET... failed, no connection or no HTTP server\n");
      }
      #endif
    }else{
      #ifdef SERIAL
        Serial.print("No WiFi available\n");
      #endif
    }
}

void loop() {

  String url_temp = "";

  #ifndef SERIAL
    digitalWrite(1, HIGH); //LED off
  #endif

  delay(2500); //If we've just started the power might be somewhat distorted - lets wait a bit to get things setteled...
 
/* Get a new sensor event */ 
  sensors_event_t event2;
  bmp.getEvent(&event2);
  /* Display the results (barometric pressure is measure in hPa) */
  if (event2.pressure)
  {
    /* Display atmospheric pressue in hPa */
    Serial.print("Pressure:    ");
    Serial.print(event2.pressure);
    Serial.println(" hPa");
    
    /* Calculating altitude with reasonable accuracy requires pressure    *
     * sea level pressure for your position at the moment the data is     *
     * converted, as well as the ambient temperature in degress           *
     * celcius.  If you don't have these values, a 'generic' value of     *
     * 1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA   *
     * in sensors.h), but this isn't ideal and will give variable         *
     * results from one day to the next.                                  *
     *                                                                    *
     * You can usually find the current SLP value by looking at weather   *
     * websites or from environmental information centers near any major  *
     * airport.                                                           *
     *                                                                    *
     * For example, for Paris, France you can check the current mean      *
     * pressure and sea level at: http://bit.ly/16Au8ol                   */
     
    /* First we get the current temperature from the BMP085 */
    float temperature;
    bmp.getTemperature(&temperature);
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    /* Then convert the atmospheric pressure, and SLP to altitude         */
    /* Update this next line with the current SLP for better results      */
    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
    Serial.print("Altitude:    "); 
    Serial.print(bmp.pressureToAltitude(seaLevelPressure,
                                        event2.pressure)); 
    Serial.println(" m");
    Serial.println("");
    url_temp = url_start;
    url_temp += uuid_pres;
    url_temp += url_stop;
    url_temp += event2.pressure;

    sendHttpData(url_temp);
    
    url_temp = url_start;
    url_temp += uuid_temp3;
    url_temp += url_stop;
    url_temp += temperature;

    sendHttpData(url_temp);   
  }
  else
  {
    Serial.println("Sensor error");
  }
  #ifdef SERIAL
    Serial.println("SLEEP");
    Serial.println(WiFi.localIP());
    Serial.flush();
  #endif
  
  #ifdef DEEP
    ESP.deepSleep(1000000 * WAIT);
  #else
    //@todo disconnect WiFi to save power?
    delay(1000 * WAIT);
  #endif
} 
