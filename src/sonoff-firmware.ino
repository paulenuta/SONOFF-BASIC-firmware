/*
 Sonoff: firmware
 More info: https://github.com/tschaban/SONOFF-firmware
 LICENCE: http://opensource.org/licenses/MIT
 2016-10-27 tschaban https://github.com/tschaban
*/

#include <DallasTemperature.h>
#include <OneWire.h>
#include <Ticker.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>


#include "Streaming.h"
#include "sonoff-configuration.h"
#include "sonoff-eeprom.h"
#include "sonoff-led.h"
#include "ota.h"

/* Variables */
SonoffEEPROM memory;
SONOFFCONFIG sonoffConfig;
DEFAULTS sonoffDefault;
SonoffLED LED;

/* Timers */
Ticker buttonTimer;
Ticker temperatureTimer;

/* Libraries init */
WiFiClient esp;
PubSubClient client(esp);
OneWire wireProtocol(TEMP_SENSOR);
DallasTemperature DS18B20(&wireProtocol);

ESP8266WebServer server(80);
DNSServer dnsServer;
ESP8266HTTPUpdateServer httpUpdater;

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println();


  pinMode(BUTTON, INPUT_PULLUP);

  Serial << endl << "EEPROM" << endl;
  sonoffConfig = memory.getConfiguration();

  Serial << endl << "Configuration: " << endl;
  Serial << " - Version: " << sonoffConfig.version << endl;
  Serial << " - Switch mode: " << sonoffConfig.mode << endl;
  Serial << " - Device ID: " << sonoffConfig.id << endl;
  Serial << " - Host name: " << sonoffConfig.host_name << endl;
  Serial << " - WiFi SSID: " << sonoffConfig.wifi_ssid << endl;
  Serial << " - WiFi Password: " << sonoffConfig.wifi_password << endl;
  Serial << " - MQTT Host: " << sonoffConfig.mqtt_host << endl;
  Serial << " - MQTT Port: " << sonoffConfig.mqtt_port << endl;
  Serial << " - MQTT User: " << sonoffConfig.mqtt_user << endl;
  Serial << " - MQTT Password: " << sonoffConfig.mqtt_password << endl;
  Serial << " - MQTT Topic: " << sonoffConfig.mqtt_topic <<  endl;
  Serial << " - DS18B20 present: " << sonoffConfig.temp_present << endl;
  Serial << " - Temp correctin: " << sonoffConfig.temp_correction << endl;
  Serial << " - Temp interval: " << sonoffConfig.temp_interval << endl;

  buttonTimer.attach(0.05, callbackButton);

  Serial << endl << "Setting relay: " << endl;
  pinMode(RELAY, OUTPUT);
  if (memory.getRelayState()==1) {
      digitalWrite(RELAY, HIGH);
  } else {
      digitalWrite(RELAY, LOW);
  }
  
  if (sonoffConfig.wifi_ssid[0]==(char) 0 || sonoffConfig.wifi_password[0]==(char) 0 || sonoffConfig.mqtt_host[0]==(char) 0) {
    Serial << endl << "Missing configuration. Going to configuration mode." << endl;
    memory.saveSwitchMode(2);
    sonoffConfig.mode=2;
  }

  if (String(sonoffDefault.version) != String(sonoffConfig.version)) {      
      Serial << endl << "SOFTWARE WAS UPGRADED from version : " << sonoffConfig.version << " to " << sonoffDefault.version << endl;
      memory.saveVersion(sonoffDefault.version);
      memory.getVersion().toCharArray(sonoffConfig.version,sizeof(sonoffConfig.version));
  }
  
  if (sonoffConfig.mode==1) {
    Serial << endl << "Entering configuration mode over the LAN" << endl;
    runConfigurationLANMode();
  } else if (sonoffConfig.mode==2) {
     Serial << endl << "Entering configuration mode with Access Point" << endl;
     runConfigurationAPMode(); 
  }else {
    Serial << endl << "Entering switch mode" << endl;
    runSwitchMode();  
  }
  
}




/* Connect to WiFI */
void connectToWiFi() {
  
  WiFi.hostname(sonoffConfig.host_name);
  WiFi.begin(sonoffConfig.wifi_ssid, sonoffConfig.wifi_password);
  Serial << endl << "Connecting to WiFi: " << sonoffConfig.wifi_ssid;
  while (WiFi.status() != WL_CONNECTED) {
    Serial << ".";
    delay(CONNECTION_WAIT_TIME);
  }
  Serial << endl << " - Connected" << endl;
  Serial << " - IP: " << WiFi.localIP() << endl;
}



void loop() {
  if (sonoffConfig.mode==0) {
    if (!client.connected()) {
      connectToMQTT();
    } 
    client.loop();
  } else if (sonoffConfig.mode==1) {      
    server.handleClient();    
  } else if (sonoffConfig.mode==2) {
    dnsServer.processNextRequest();
    server.handleClient();
  } else {
    Serial << "Internal Application Error" << endl;
  }  
}
