#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// wenn aktiv: Meldungen per Serial
#define SERIAL

// wenn auskommentiert: delay()
// wenn aktiv: deep sleep nutzen (Hardwaremodifikation notwendig)
//#define DEEP

//sekunden zwischen Aufwachvorgängen
//#define WAIT 120
#define WAIT 10

#define ONE_WIRE_BUS            D4      // DS18B20 pin

const String host = "IPADDRESS";
const unsigned int port = 80;

const String url_start = "/middleware.php/data/";
const String url_stop = ".json?operation=add&value=";

//fill with right values
const String uuid_temp1 = "000";
const String uuid_humid = "111";
const String uuid_temp2 = "222 ";

byte maxwait = 160; //Wifi must connect in < x seconds

#define DHTPIN            2         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)



#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

ESP8266WiFiMulti WiFiMulti;

DHT_Unified dht(DHTPIN, DHTTYPE);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

char temperatureString[6];

void setup() {
  #ifdef SERIAL
    Serial.begin(115200);
    Serial.println("BOOT");

    Serial.print("Wifi...");
  #endif

  //WiFiMulti.addAP("Hades","t4k4tuk4l4nd2");
  WiFiMulti.addAP("ocktown_d0wn911","t4k4tuk4l4nd1");
  WiFiMulti.addAP("ViditFabrik","%m1n!5NK=$");
  //WiFi.begin("Hades","t4k4tuk4l4nd2");
  //WiFi.begin("ocktown_d0wn911","t4k4tuk4l4nd1");
  
  #ifdef SERIAL
    Serial.println("OK");
    Serial.println("DHT...");
  #endif

  dht.begin();

  #ifdef SERIAL
    Serial.print("OK");
  #endif

  sensor_t sensor;
  #ifdef SERIAL
    dht.temperature().getSensor(&sensor);
    Serial.println("------------------------------------");
    Serial.println("Temperature");
    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
    Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
    Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
    Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" °C");
    Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" °C");
    Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" °C");
  #endif
  dht.humidity().getSensor(&sensor);
  #ifdef SERIAL
    Serial.println("------------------------------------");
    Serial.println("Humidity");
    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
    Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
    Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
    Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
    Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
    Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");
    Serial.println("------------------------------------");
  #endif
  // setup OneWire bus
  DS18B20.begin();
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

float getTemperature() {
  Serial.print("Requesting DS18B20 temperature...");
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}

void loop() {

  String url_temp = "";
  float temp2 = getTemperature();
  
  #ifndef SERIAL
    digitalWrite(1, HIGH); //LED off
  #endif

  delay(2500); //If we've just started the power might be somewhat distorted - lets wait a bit to get things setteled...

  sensors_event_t event;
  
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    #ifdef SERIAL
      Serial.println("Error reading temperature!");
    #endif
  } else{
    #ifdef SERIAL
      Serial.print("Temperature: ");
      Serial.print(event.temperature);
      Serial.println(" °C");
    #endif
    url_temp = url_start;
    url_temp += uuid_temp1;
    url_temp += url_stop;
    url_temp += event.temperature;

    sendHttpData(url_temp);
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    #ifdef SERIAL
      Serial.println("Error reading humidity!");
    #endif
  }
  else {
    #ifdef SERIAL
      Serial.print("Humidity: ");
      Serial.print(event.relative_humidity);
      Serial.println("%");
    #endif
    url_temp = url_start;
    url_temp += uuid_humid;
    url_temp += url_stop;
    url_temp += event.relative_humidity;

    sendHttpData(url_temp);
  }
    // convert temperature to a string with two digits before the comma and 2 digits for precision
  dtostrf(temp2, 2, 2, temperatureString);
  // send temperature to the serial console
  Serial.print("Sending temperature: ");
  Serial.println(temperatureString);
  Serial.println(temp2);
  Serial.println(temperatureString);
    url_temp = url_start;
    url_temp += uuid_temp2;
    url_temp += url_stop;
    url_temp += temp2;

    sendHttpData(url_temp);
    
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
