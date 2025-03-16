
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif 

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Debounce.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include "SinricPro.h"
#include "SinricProDimSwitch.h"


#define APP_KEY           "1b938788-f911-4a6c-8589-68707f59faa9"      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        "5a022722-4e92-48c2-80a5-df9a16aa59b6-e48205e8-8405-438b-9ad7-d50fb7efd256"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define DIMSWITCH_ID      "67d6b03c947cbabd20ddceb6"    // Should look like "5dc1564130xxxxxxxxxxxxxx"


#define BAUD_RATE         115200                // Change baudrate to your need


#define SENSOR_100  13  // GPIO13
#define SENSOR_85   12  // GPIO12
#define SENSOR_71   14  // GPIO14
#define SENSOR_57   2  // GPIO2
#define SENSOR_42   15  // D8
#define SENSOR_28   4  // GPIO4
#define SENSOR_14   5  // GPIO5
#define SENSOR_0    3  // rx
#define BUTTON_PIN 0
Debounce button(BUTTON_PIN);


SinricProDimSwitch &myDimSwitch = SinricPro[DIMSWITCH_ID];

bool onPowerState(const String &deviceId, bool &state) {
  return true; // request handled properly
}

bool onPowerLevel(const String &deviceId, int &powerLevel) {
  return true;
}

bool onAdjustPowerLevel(const String &deviceId, int &levelDelta) {
  return true;
}


void setupWiFi() {
  WiFi.setSleepMode(WIFI_NONE_SLEEP); 
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA); 
  
  WiFiManager wm;
  
  Serial.println("Criando wifi");
  wm.setTimeout(180); // 180 segundos (3 min) para conectar ao Wi-Fi
  if (!wm.autoConnect("Caixa dagua")) {
    Serial.println("Falha ao conectar Wi-Fi! Reiniciando...");
    delay(3000);
    ESP.restart();
  }
  Serial.printf("Connected");
}

void setupSinricPro() {
  // set callback function to device
  myDimSwitch.onPowerState(onPowerState);
  myDimSwitch.onPowerLevel(onPowerLevel);
  myDimSwitch.onAdjustPowerLevel(onAdjustPowerLevel);

  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup() {
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SENSOR_100, INPUT);
  pinMode(SENSOR_85, INPUT);
  pinMode(SENSOR_71, INPUT);
  pinMode(SENSOR_57, INPUT);
  pinMode(SENSOR_42, INPUT);
  pinMode(SENSOR_28, INPUT);
  pinMode(SENSOR_14, INPUT);
  pinMode(SENSOR_0, INPUT);

  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
  setupWiFi();
  setupSinricPro();
}


uint8_t read_water_level() {
    if (digitalRead(SENSOR_100) == HIGH) return 100;
    if (digitalRead(SENSOR_85) == HIGH) return 85;
    if (digitalRead(SENSOR_71) == HIGH) return 65;
    //if (digitalRead(SENSOR_57) == HIGH) return 57;
    if (digitalRead(SENSOR_42) == HIGH) return 50;
    if (digitalRead(SENSOR_28) == HIGH) return 30;
    if (digitalRead(SENSOR_14) == HIGH) return 15;
    if (digitalRead(SENSOR_0) == HIGH) return 5;
    return 0;
}

void loop() {
  SinricPro.handle();

    // Verifica se o botão foi pressionado para resetar o Wi-Fi
  if (button.read() == HIGH) {
    Serial.println("Resetando configurações de Wi-Fi...");

    WiFiManager wm;
    wm.resetSettings();
    
    delay(1000);
    ESP.restart(); 
  }

  static unsigned long lastChange = 0;
   if (millis() - lastChange >= 10000) { // Verifica se passou 1 segundo
    lastChange = millis();
    myDimSwitch.sendPowerLevelEvent(read_water_level());
  }

}
