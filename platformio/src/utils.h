#ifndef UTILS_H
#define UTILS_H

#include <FS.h>
#include <Arduino.h>
#include "debug.h"
#include "setup.h"
#include "cloud.h"

#define BLUE_LED_PIN 12
#define RED_LED_PIN 14
#define SETUP_BUTOON_PIN 5

#define SETUP_BUTTON_TIME 2000
#define RESET_BUTTON_TIME 4000

#define FILP_RATE 200

#define SETUP_OK 0
#define SPIFFS_FAIL 2
#define LOAD_KEY_FAIL 3
#define LOAD_WIFI_FAIL 4
#define SETUP_SENSOR_FAIL 5
#define BAD_KEY 6

#define UTILS_ERROR_TIMEOUT (1000*60*10)
#define SENSOR_ERROR_TIMEOUT (1000*60*10)

#define SENSOR_RETRY_BEFOR_ERROR 3

#define WIFI_FILE_NAME F("/wifi.json")
#define KEY_FILE_NAME F("/key.json")

#define get_device_id() String(ESP.getChipId(), HEX)

const char* get_easycool_version();
void init_utils();

void handle_sos_error(int sos_num);

void handle_utils();
void handle_setup_button();

void non_block_sos(uint8_t sos_num, uint8_t pin);
void block_sos(uint8_t pin, int repeat, unsigned long duration);
void set_leds(bool blue, bool red);
void flip_leds(bool blue, bool red);

void restart();

int init_wifi();
bool save_wifi(JsonObject& json_root);

bool save_key(JsonObject& json_root);
bool load_key();
String get_key();

#endif
