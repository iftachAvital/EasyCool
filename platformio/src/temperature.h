#ifndef TEMPERATUR_H
#define TEMPERATUR_H

#include "MyWire.h"
#include "debug.h"
#include "utils.h"

#define TEMP_SENSOR_PIN 4
#define TEMP_MESURE_INTERVAL 1000
#define TEMP_SENSOR_RETRY 5
#define TEMP_ERROR -128.0f


bool init_temp();
bool call_temp();
bool read_temp();
void handle_sensor_fault();
void handle_temp();
float get_temp();

#endif
