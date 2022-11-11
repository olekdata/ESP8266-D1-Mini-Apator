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

#define 	BUILDTIME "2022-11-11 10:00"

AsyncWebServer server(80);


Ticker secondTick;

int watchdogCount = 0;

void ISRwatchdog(){
  if (++watchdogCount > 5){
      ESP.reset();
  }
}


IPAddress staticIP(192, 168, 1, 223);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);


void setup() {
  Serial.begin(112500);

  if (WiFi.config(staticIP, gateway, subnet, dns, dns) == false) {
    Serial.println("WiFi Configuration failed.");
  }

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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    char s[60];
    sprintf(s, "Hi! I am ESP8266 Apator (%s)", BUILDTIME);
    request->send(200, "text/plain", s);
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  memset(MBpacket, 0, sizeof(MBpacket));
  
  rf_mbus_init();

  mqtt = new comfoair::MQTT();
  mqtt->setup();
  secondTick.attach(60, ISRwatchdog);
}

char mqttTopicMsgBuf[30];
char mqttTopicValBuf[200];
char otherBuf[30];
char buff[200];

unsigned int sec = 0;
unsigned int f_count = 0;



void loop() {
  if (rf_mbus_task(MBpacket)) {

    printf("\n");
    uint8_t lenWithoutCrc = crcRemove(MBpacket, packetSize(MBpacket[0]));
    
    std::vector<uchar> frame(MBpacket, MBpacket+lenWithoutCrc);
    std::string sf = bin2hex(frame);
    Serial.printf("frame = %s\n", sf.c_str());

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
      std::vector<uchar>::iterator fv;
      fv = std::find(pos, frame.end(), 0x10);
      if (fv != frame.end()){
        int v;
        memcpy(&v, &fv[1], 4);
        Serial.printf("value=%d, pos-%d\n",v, fv-frame.begin());
        if ((v > 0) and (v/1000 < 10000)  ) {
          Serial.print("Ramka OK ");
          sprintf(mqttTopicMsgBuf, "%s/%s", MQTT_PREFIX, dll_id);
        } else {
          Serial.print("Ramka zła wartość ");
          sprintf(mqttTopicMsgBuf, "%s/%s_", MQTT_PREFIX, dll_id);
        };
     
        sprintf(mqttTopicValBuf, "%d.%03d", v / 1000, v % 1000);
        if (mqtt->writeToTopic(mqttTopicMsgBuf, mqttTopicValBuf)) {
          Serial.println("wysłana");
          ++f_count;
          watchdogCount = 0;
        } else {
          Serial.println("nie wysłana");
        }
      }
      else {
          Serial.println("brak x10");
      }
/*
      for (int j=0; j<4; ++j) { sprintf(mqttTopicValBuf+2*j, "%02X", frame[40+j]);}
      mqttTopicValBuf[8] = 0;
      sprintf(mqttTopicMsgBuf, "%s/%s_x", MQTT_PREFIX, dll_id);
      mqtt->writeToTopic(mqttTopicMsgBuf, mqttTopicValBuf);
*/      
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
      sprintf(mqttTopicValBuf, "%d;%d - %s ", sec/60,f_count, BUILDTIME);
      mqtt->writeToTopic(mqttTopicMsgBuf, mqttTopicValBuf);
    }
  }
}

