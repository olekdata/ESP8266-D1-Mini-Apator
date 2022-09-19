#include <Arduino.h>
#include <SPI.h>
#include "secrets.h"
#include <ESP8266WiFi.h>
#include "rf_mbus.hpp"
#include "crc.hpp"
#include "utils.hpp"
#include "mbus_packet.hpp"
#include "mqtt.h"
#include <ArduinoJson.h>
#include <vector>
#include "util.h"
#include "wmbus_utils.h"
#include <string.h>
#include <string>
#include "util.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Ticker.h>

comfoair::MQTT *mqtt;

uint8_t MBpacket[291];

AsyncWebServer server(80);


Ticker secondTick;

int watchdogCount = 0;

void ISRwatchdog(){
  if (++watchdogCount > 5){
      ESP.reset();
  }
}

//WiFiClient client;


void setup() {
  Serial.begin(112500);
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (i++ > 20) {
        ESP.restart();
      }
  }

  Serial.println("");
  //Serial.print("Connected to ");
  //Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP8266 Apator");
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  memset(MBpacket, 0, sizeof(MBpacket));
  
  rf_mbus_init();

  mqtt = new comfoair::MQTT();
  mqtt->setup();
//  ESP.wdtEnable(1000*60*5);
  secondTick.attach(60, ISRwatchdog);
}

char mqttTopicMsgBuf[30];
char mqttTopicValBuf[200];
char otherBuf[30];
char buff[200];

unsigned int sec = 0;
unsigned int f_count = 0;
//int wdt_count = 0;



void loop() {
  //server.
  if (rf_mbus_task(MBpacket)) {

    printf("\n");
    uint8_t lenWithoutCrc = crcRemove(MBpacket, packetSize(MBpacket[0]));
    
    std::vector<uchar> frame(MBpacket, MBpacket+lenWithoutCrc);
    std::string sf = bin2hex(frame);
    ///Serial.print("frame ="); Serial.println(sf.c_str());
    Serial.printf("frame = %s\n", sf.c_str());
/*
    dumpHex(MBpacket+3, 4, dll_id, true);

  for (uint8_t i = 0; i < len; i++) {
    uint8_t j = i;
    if (revert) 
      j = len-i;
    sprintf(buffHex, "%02X", data[j]);
     strcpy(buff, buffHex);
     buff += 2;
  }
*/
    char dll_id[9];
    sprintf(dll_id + 0, "%02X", frame[7]);
    sprintf(dll_id + 2, "%02X", frame[6]);
    sprintf(dll_id + 4, "%02X", frame[5]);
    sprintf(dll_id + 6, "%02X", frame[4]);

    Serial.printf("id = %s\n", dll_id);
    

    std::vector<uchar> key;
    key.assign(16,0);
    std::vector<uchar>::iterator pos;
    pos = frame.begin();
    uchar iv[16];
    int i=0;
    for (int j=0; j<8; ++j) { iv[i++] = frame[2+j]; }
    for (int j=0; j<8; ++j) { iv[i++] = frame[11]; }
    pos = frame.begin() + 15;
    int num_encrypted_bytes = 0;
    int num_not_encrypted_at_end = 0;
    if (decrypt_TPL_AES_CBC_IV(frame, pos, key, iv, &num_encrypted_bytes, &num_not_encrypted_at_end)) {
      int i;
      memcpy(&i, &frame[40], 4);
      Serial.printf("value=%d\n",i);
      if ((i > 0) and (i/1000 < 10000)  ) {
        Serial.print("Ramka OK ");
        sprintf(mqttTopicMsgBuf, "%s/%s", MQTT_PREFIX, dll_id);
      } else {
        Serial.print("Ramka zła wartość ");
        sprintf(mqttTopicMsgBuf, "%s/%s_", MQTT_PREFIX, dll_id);
      };
      
      sprintf(mqttTopicValBuf, "%d.%d", i / 1000, i % 1000);
      if (mqtt->writeToTopic(mqttTopicMsgBuf, mqttTopicValBuf)) {
        Serial.println("wysłana");
        ++f_count;
        //wdt_count = 0;
        watchdogCount = 0;
      } else {
        Serial.println("nie wysłana");
      }
    }
    else {
      Serial.println("Ramka zła");
    }
      
    memset(MBpacket, 0, sizeof(MBpacket));
  }
  unsigned long sec_;
  sec_ = millis()/1000;

  if (sec_ > sec) {
    sec = sec_;
//    Serial.print("+");
    if ((sec % 60) == 0) {
      sprintf(mqttTopicMsgBuf, "%s/Live", MQTT_PREFIX);
      sprintf(mqttTopicValBuf, "%d;%d", sec/60,f_count);
      mqtt->writeToTopic(mqttTopicMsgBuf, mqttTopicValBuf);
    }
  }

  //mqtt->loop();


}

