#include <Arduino.h>
#include <Debounce.h>
#include "fauxmoESP.h"
#include "ESP8266WiFi.h"

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <EEPROM.h>

#define SERIAL_BAUDRATE 115200
#define BOT_TOKEN_LENGTH 30
#define DEFAULT_PIN_STATE_EEPROM 31

#define LED_PIN 2
#define BUTTON_PIN 0
Debounce button(BUTTON_PIN);

char device_custom_name[BOT_TOKEN_LENGTH] = "";
fauxmoESP fauxmo;
char buttonState = 0 ;
//flag for saving data
bool shouldSaveConfig = false;
char default_pin_state[2] = "0";

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void readBotTokenFromEeprom() {
  for (int i = 0; i < BOT_TOKEN_LENGTH; i++ ) {
    device_custom_name[i] = EEPROM.read(i);
  }

  default_pin_state[0] = EEPROM.read(DEFAULT_PIN_STATE_EEPROM);
    
  EEPROM.commit();
}

void writeBotTokenToEeprom() {
  for (int i = 0; i < BOT_TOKEN_LENGTH; i++ ) {
    EEPROM.write(i, device_custom_name[i]);
  }
  EEPROM.write(DEFAULT_PIN_STATE_EEPROM,default_pin_state[0] );
  EEPROM.commit();
}

void setup() {
  WiFi.mode(WIFI_STA); 
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  Serial.begin(SERIAL_BAUDRATE);
  Serial.println("Serial Setup");
  
  EEPROM.begin(512);
  
  readBotTokenFromEeprom();

  digitalWrite(LED_PIN, default_pin_state[0]  == '1' ? HIGH : LOW);
  
  WiFiManager wm;
  
  wm.setSaveConfigCallback(saveConfigCallback);
  //Adding an additional config on the WIFI manager webpage for the bot token

  WiFiManagerParameter custom_bot_id("NOME", "device_custom_name", device_custom_name , 30);
  WiFiManagerParameter default_output_state("OUTPUT", "default_pin_state", default_pin_state , 1);

  wm.addParameter(&custom_bot_id);
  wm.addParameter(&default_output_state);

  Serial.println("Criando wifi");
  wm.autoConnect("Mandacaru");

  strcpy(device_custom_name, custom_bot_id.getValue());
  strcpy(default_pin_state , default_output_state.getValue());
  
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

void loop() {
  fauxmo.handle();
  
  if (button.read() == HIGH) {
    Serial.println("Resetando configurações");
  
    WiFi.disconnect();
    delay(500);
    ESP.reset();
  }
}
