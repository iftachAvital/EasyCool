#include "main.h"

unsigned long last_connected;
wl_status_t last_wifi_status;
wl_status_t current_wifi_status;
bool is_disconnected;

void start_wifi_disconnected()
{
    last_connected = millis();
    is_disconnected = false;
    set_leds(false, false);
}

void setup() {
    #ifdef DEBUG_EASYCOOL_PORT
        DEBUG_EASYCOOL_PORT.begin(115200);
        delay(200);
    #endif

    DEBUGV_EASYCOOL("[main] start setup\n");

    init_utils();
    start_wifi_disconnected();
    DEBUGV_EASYCOOL("[main] end setup\n");
    DEBUGV_EASYCOOL("[main] start loop\n");
}

void loop() {
    handle_utils();

    current_wifi_status = WiFi.status();

    if (current_wifi_status == WL_CONNECTED) {
        if (!(last_wifi_status == WL_CONNECTED)) {
            // first time after connected
            DEBUGV_EASYCOOL("[wifi] connected\n");
            set_leds(true, false);
        }
        handle_cloud();
    }
    else {
        if (last_wifi_status == WL_CONNECTED) {
            // first time after disconnected
            DEBUGV_EASYCOOL("[wifi] disconnected\n");
            start_wifi_disconnected();
        }
        else {
            if (millis() > (last_connected + WIFI_DISCONNECT_TIMEOUT) && !is_disconnected) {
                DEBUGV_EASYCOOL("[wifi] turn off\n");
                is_disconnected = WiFi.disconnect(true);
            }
            else if (millis() > (last_connected + WIFI_OFF_TIMEOUT)) {
                restart();
            }
        }
        non_block_sos(current_wifi_status, BLUE_LED_PIN);
    }

    last_wifi_status = current_wifi_status;
}
