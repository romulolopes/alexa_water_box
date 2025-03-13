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


Debounce sensor_100(SENSOR_100);
Debounce sensor_85(SENSOR_85);
Debounce sensor_71(SENSOR_71);
Debounce sensor_57(SENSOR_57);
Debounce sensor_42(SENSOR_42);
Debounce sensor_28(SENSOR_28);
Debounce sensor_14(SENSOR_14);
Debounce sensor_0(SENSOR_0);

#define POWER_PIN 0 //d3
#define BUTTON_PIN 0
Debounce button(BUTTON_PIN);

char device_custom_name[BOT_TOKEN_LENGTH] = "";
fauxmoESP fauxmo;
char buttonState = 0 ;

char last_water_level = 0;
char current_water_level = 0;

//flag for saving data
bool shouldSaveConfig = false;

unsigned long lastTime = 0; 
const long interval = 30000;  // 2 segundos


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
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);  // Evita problemas no boot
  
  
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
  int value = 0;
  digitalWrite(POWER_PIN, HIGH );
  delay(50);
  if (sensor_100.read() == HIGH) {
    value = 100;
  } else if (sensor_85.read() == HIGH) {
    value = 85;
  } else if (sensor_71.read() == HIGH) {
    value = 71;
  } else if (sensor_57.read() == HIGH) {
    value = 57;
  } else if (sensor_42.read() == HIGH) {
    value = 42;
  } else if (sensor_28.read() == HIGH) {
    value = 28;
  } else if (sensor_14.read() == HIGH) {
    value = 14;
  } else if (sensor_0.read() == HIGH) {
    value = 0;
  } else {
    value = 0;
  }
  digitalWrite(POWER_PIN, LOW);
  return value;
}

void loop() {
  fauxmo.handle();
  
  if (button.read() == HIGH) {
    Serial.println("Resetando configurações");
  
    WiFi.disconnect();
    delay(500);
    ESP.restart();
  }

  if (millis() - lastTime >= interval) {
    lastTime = millis();
  
    current_water_level = read_water_level();
    if (current_water_level != last_water_level){
        fauxmo.setState(device_custom_name, true, current_water_level);  
    }
    last_water_level = current_water_level;
  } 
  yield();  
}
