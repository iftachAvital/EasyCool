#include "temperature.h"

MyWire ds(TEMP_SENSOR_PIN);
int reconnect_sensor_counter;
float temp = TEMP_ERROR;

bool init_temp()
{
    reconnect_sensor_counter = 0;

    bool res_call_temp = call_temp();
    bool res_read_temp = false;

    if (res_call_temp) {
        delay(1000);
        res_read_temp = read_temp();
    }

    return res_call_temp && res_read_temp;
}

bool reset_temp()
{
    if (ds.reset() == FALSE) {
        DEBUGV_EASYCOOL("[temperature] error on reset\n");
        return false;
    }
    return true;
}

bool call_temp()
{
    if (reset_temp()) {
        yield();
        ds.skip();
        yield();
        ds.write(0x44);
        yield();
        return true;
    }

    return false;
}

bool read_temp()
{
    int i;
    byte scratchpad[9];

    if (!reset_temp()) {
        return false;
    }

    yield();
    ds.skip();
    yield();
    ds.write(0xBE);
    yield();

    DEBUGV_EASYCOOL("[temperature] scratchpad: ");

    for(i = 0; i < 9; i++) {
        scratchpad[i] = ds.read();
        DEBUGV_EASYCOOL("%x ", scratchpad[i]);
    }

    bool res_crc = ds.crc8(scratchpad, 8) == scratchpad[8];

    if (res_crc) {
        int16_t raw_temperature = (((int16_t)scratchpad[1]) << 8) | scratchpad[0];
        temp = (float)raw_temperature * 0.0625;
    }

    DEBUGV_EASYCOOL("crc: %d, temp: %s\n", res_crc, String(temp).c_str());
    return res_crc;
}

void handle_sensor_fault()
{
    yield();

    reconnect_sensor_counter++;

    DEBUGV_EASYCOOL("[temperature] handle sensor fault, counter: %d\n", reconnect_sensor_counter);

    if (reconnect_sensor_counter > TEMP_SENSOR_RETRY) {
        handle_sos_error(SETUP_SENSOR_FAIL);
    }
    else {
        handle_temp();
    }
}

void handle_temp()
{
    static unsigned long next_read;

    if(millis() > next_read) {
        if (read_temp() && call_temp()) {
            reconnect_sensor_counter = 0;
            next_read = millis() + TEMP_MESURE_INTERVAL;
        }
        else {
            handle_sensor_fault();
        }
    }
}

float get_temp()
{
    return temp;
}
