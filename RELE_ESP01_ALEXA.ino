#include <Arduino.h>
#include <Debounce.h>
#include "fauxmoESP.h"
#include "ESP8266WiFi.h"

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <EEPROM.h>

#define SERIAL_BAUDRATE 115200
#define BOT_TOKEN_LENGTH 30

#define SENSOR_100  7                     
#define SENSOR_75  6                     
#define SENSOR_50  5                     
#define SENSOR_25  4                     
#define SENSOR_0  3                     
Debounce sensor_100(SENSOR_100);
Debounce sensor_75(SENSOR_75);
Debounce sensor_50(SENSOR_50);
Debounce sensor_25(SENSOR_25);
Debounce sensor_0(SENSOR_0);

#define LED_PIN 2
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
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(SENSOR_100, INPUT);
  pinMode(SENSOR_75, INPUT);
  pinMode(SENSOR_50, INPUT);
  pinMode(SENSOR_25, INPUT);
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
  wm.addParameter(&default_output_state);

  Serial.println("Criando wifi");
  wm.autoConnect("Mandacaru");

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

    if (strcmp(device_name, device_custom_name) == 0) {
      digitalWrite(LED_PIN, state ? HIGH : LOW);
    }
  });

}

void read_water_level(){
  if (sensor100.read() == HIGH){
    last_water_level = 100;  
  }else if (sensor75.read() == HIGH){
    last_water_level = 75;
  }else if (sensor50.read() == HIGH){
    last_water_level = 50;
  }else if (sensor25.read() == HIGH){
    last_water_level = 25;
  }else if (sensor0.read() == HIGH){
    last_water_level = 0;
  }else{
    last_water_level = 0 ;
  }
}

void loop() {
  fauxmo.handle();
  
  if (button.read() == HIGH) {
    Serial.println("Resetando configurações");
  
    WiFi.disconnect();
    delay(500);
    ESP.reset();
  }
  
  current_water_level = read_water_level();
  if (current_water_level != last_water_level){
      fauxmo.setState(device_custom_name, true, current_water_level);  
  }
  last_water_level = current_water_level;
  
  delay(2000);
}
