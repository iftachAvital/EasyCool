#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include "setup.h"
#include "utils.h"
#include "cloud.h"
#include "debug.h"

#define WIFI_DISCONNECT_TIMEOUT (1000*60*5)
#define WIFI_OFF_TIMEOUT WIFI_DISCONNECT_TIMEOUT*2

void start_wifi_disconnected();

#endif
