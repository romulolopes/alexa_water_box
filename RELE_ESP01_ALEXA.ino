#include <Arduino.h>
#include <Debounce.h>
#include "fauxmoESP.h"
#include "ESP8266WiFi.h"

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <EEPROM.h>

#define SERIAL_BAUDRATE 115200
#define BOT_TOKEN_LENGTH 30

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

char device_custom_name[BOT_TOKEN_LENGTH] = "";
fauxmoESP fauxmo;
char buttonState = 0 ;

char last_water_level = 0;
char current_water_level = 0;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void readBotTokenFromEeprom() {
  for (int i = 0; i < BOT_TOKEN_LENGTH; i++ ) {
    device_custom_name[i] = EEPROM.read(i);
  }
  
  EEPROM.commit();
}

void writeBotTokenToEeprom() {
  for (int i = 0; i < BOT_TOKEN_LENGTH; i++ ) {
    EEPROM.write(i, device_custom_name[i]);
  }
  EEPROM.commit();
}

void setup() {
  
  WiFi.mode(WIFI_STA); 
  
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SENSOR_100, INPUT);
  pinMode(SENSOR_85, INPUT);
  pinMode(SENSOR_71, INPUT);
  pinMode(SENSOR_57, INPUT);
  pinMode(SENSOR_42, INPUT);
  pinMode(SENSOR_28, INPUT);
  pinMode(SENSOR_14, INPUT);
  pinMode(SENSOR_0, INPUT);

  Serial.begin(SERIAL_BAUDRATE);
  Serial.println("Serial Setup");
  
  EEPROM.begin(512);
  
  readBotTokenFromEeprom();
  
  WiFiManager wm;
  
  wm.setSaveConfigCallback(saveConfigCallback);
  //Adding an additional config on the WIFI manager webpage for the bot token

  WiFiManagerParameter custom_bot_id("NOME", "device_custom_name", device_custom_name , 30);
  
  wm.addParameter(&custom_bot_id);
  
  Serial.println("Criando wifi");
  wm.setTimeout(180); // 180 segundos (3 min) para conectar ao Wi-Fi
  if (!wm.autoConnect("Caixa dagua")) {
    Serial.println("Falha ao conectar Wi-Fi! Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  
  strcpy(device_custom_name, custom_bot_id.getValue());
  
  if (shouldSaveConfig) {
    writeBotTokenToEeprom();
  }

  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80); // This is required for gen3 devices
  fauxmo.enable(true);

  // Fauxmo
  fauxmo.addDevice(device_custom_name);
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {

    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
  });

}

int read_water_level() {
    if (digitalRead(SENSOR_100) == HIGH) return 100;
    if (digitalRead(SENSOR_85) == HIGH) return 85;
    if (digitalRead(SENSOR_71) == HIGH) return 65;
    //if (digitalRead(SENSOR_57) == HIGH) return 57;
    if (digitalRead(SENSOR_42) == HIGH) return 50;
    if (digitalRead(SENSOR_28) == HIGH) return 30;
    if (digitalRead(SENSOR_14) == HIGH) return 15;
    if (digitalRead(SENSOR_0) == HIGH) return 2;
    return 0;
}

void loop() {
  fauxmo.handle();
  
  if (button.read() == HIGH) {
    Serial.println("Resetando configurações");
  
    WiFi.disconnect();
    delay(500);
    ESP.restart();
  }

  static unsigned long lastTime = millis();
  if (millis() - lastTime >= 1000) {
    lastTime = millis();
  
    current_water_level = read_water_level();
    Serial.printf("Current water level %i\n", (int)current_water_level);
    if (current_water_level != last_water_level){
      uint8_t adjusted_value = map(current_water_level, 0, 100, 0, 255);
      fauxmo.setState(device_custom_name, true, adjusted_value);  
    }
    last_water_level = current_water_level;
  } 


   static unsigned long last = millis();
   if (millis() - last > 5000) {
        last = millis();
        Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }

}
