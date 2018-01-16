#ifndef SETUP_H
#define SETUP_H

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "debug.h"
#include "utils.h"
#include "temperature.h"

#define SOFT_AP_SERVER_PORT 8888
#define AP_PASS "easycool"

#define SETUP_MANAGER_TIMEOUT (60 * 1000)
#define EXIT_MANAGER_BUTTON_TIME (4 * 1000)

void reset_setup_timeout();
void handle_setup_button_in_manager();

void generate_info(char* buffer, size_t max_size);
void handle_info(WiFiClient &client);
void handle_update_firmware(WiFiClient &client, JsonObject &json_res);
void generate_scan(char* buffer, size_t max_size);
void handle_scan_wifi(WiFiClient &client);
void handle_connect_wifi(WiFiServer &soft_ap_server, WiFiClient &client, JsonObject &json_res);
void handle_set_key(WiFiClient &client, JsonObject &json_res);
void setup_manager();

#endif
