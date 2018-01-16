#ifndef CLOUD_H
#define CLOUD_H

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "debug.h"
#include "utils.h"

#define URL "api.easycool.io"
#define PORT 80
#define API_VERSION "3"
#define PATH "/" API_VERSION "/device"
#define HTTP_TIMEOUT 5000

#define UPDATE_RATE (1000*60*10)

#define COMMAND_INIT 0
#define COMMAND_UPDATE 1
#define COMMAND_SENSOR_FAULT 2

bool update_sensor_fail();
bool update_cloud(int command);
void handle_cloud();
int make_cloud_request(int command);

#endif
