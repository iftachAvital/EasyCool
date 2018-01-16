#include "utils.h"

const char* easycool_version = "1_0_0";
String key;

const char* get_easycool_version()
{
    return easycool_version;
}

void init_utils()
{
    DEBUGV_EASYCOOL("[utils] start init\n");

    uint8_t setup_res = SPIFFS_FAIL;

    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(SETUP_BUTOON_PIN, INPUT_PULLUP);
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);

    bool spiffs_res = SPIFFS.begin();
    bool init_sensor_res = init_temp();
    DEBUGV_EASYCOOL("[utils] init spiffs: %d\n", spiffs_res);
    if (spiffs_res) {
        setup_res = LOAD_KEY_FAIL;
        bool load_key_res = load_key();
        DEBUGV_EASYCOOL("[utils] load_key: %d\n", load_key_res);
        if (load_key_res) {
            setup_res = LOAD_WIFI_FAIL;
            bool init_wifi_res = (init_wifi() != -1);
            DEBUGV_EASYCOOL("[utils] init_wifi: %d\n", init_wifi_res);
            if (init_wifi_res) {
                setup_res = SETUP_SENSOR_FAIL;
                DEBUGV_EASYCOOL("[utils] init_temp: %d\n", init_sensor_res);
                if (init_sensor_res) {
                    setup_res = SETUP_OK;
                }
            }
        }
    }

    if (setup_res != SETUP_OK) {
        handle_sos_error(setup_res);
    }
}

void handle_sos_error(int sos_num)
{
    DEBUGV_EASYCOOL("[utils] start handle sos error, sos: %d\n", sos_num);
    bool is_updated = false;

    set_leds(false, false);

    unsigned long timeout = millis() + UTILS_ERROR_TIMEOUT;

    while (millis() < timeout) {
        yield();
        handle_setup_button();
        non_block_sos(sos_num, RED_LED_PIN);

        // handle special task for specific error
        switch (sos_num) {
            case SETUP_SENSOR_FAIL:
                if (WiFi.isConnected() && !is_updated) {
                    is_updated = update_sensor_fail();
                }
                break;
        }
    }

    restart();
}

void handle_utils()
{
    handle_setup_button();
    handle_temp();
    yield();
}

void handle_setup_button()
{
    if (digitalRead(SETUP_BUTOON_PIN) == LOW) {
        DEBUGV_EASYCOOL("[utils] button pressed\n");
        unsigned long start_press = millis();
        unsigned long press_time;

        bool blue_before = !digitalRead(BLUE_LED_PIN);
        bool red_before = !digitalRead(RED_LED_PIN);

        set_leds(false, false);

        while (digitalRead(SETUP_BUTOON_PIN) == LOW) {
            press_time = millis() - start_press;

            if (press_time > SETUP_BUTTON_TIME && press_time < RESET_BUTTON_TIME) {
                set_leds(true, true);
            }
            else if (press_time > RESET_BUTTON_TIME) {
                flip_leds(true, true);
            }
            delay(100);
        }

        if (press_time > SETUP_BUTTON_TIME && press_time < RESET_BUTTON_TIME) {
            setup_manager();
        }
        else if (press_time > RESET_BUTTON_TIME) {
            restart();
        }
        else {
            DEBUGV_EASYCOOL("[utils] button released before time\n");
            set_leds(blue_before, red_before);
        }
    }
}

void non_block_sos(uint8_t sos_num, uint8_t pin)
{
    static unsigned long next_flash;
    static uint8_t counter = 0;
    static uint8_t current_sos_num;

    if (millis() > next_flash) {
        if (counter++ < current_sos_num*2) {
            digitalWrite(pin, !digitalRead(pin));
            next_flash = millis() + FILP_RATE;
        }
        else {
            delay(1000);
            counter = 0;
            current_sos_num = sos_num;
        }
    }
}

void set_leds(bool blue, bool red)
{
    digitalWrite(BLUE_LED_PIN, blue ? LOW : HIGH);
    digitalWrite(RED_LED_PIN, red ? LOW : HIGH);
}

void flip_leds(bool blue, bool red)
{
    if (blue) {
        digitalWrite(BLUE_LED_PIN, !digitalRead(BLUE_LED_PIN));
    }
    if (red) {
        digitalWrite(RED_LED_PIN, !digitalRead(RED_LED_PIN));
    }
}

void block_sos(uint8_t pin, int repeat, unsigned long duration)
{
    int i;

    for (i = 0; i < repeat; i++) {
        digitalWrite(pin, LOW);
        delay(duration);
        digitalWrite(pin, HIGH);
        delay(duration);
    }
}

void restart()
{
    DEBUGV_EASYCOOL("[utils] restarting\n");
    delay(1000);
    ESP.restart();
    delay(5000);
}

bool save_wifi(JsonObject& json_root)
{
    File wifi_file = SPIFFS.open(WIFI_FILE_NAME, "w");
    if (wifi_file) {
        json_root.printTo(wifi_file);
        wifi_file.close();
        return true;
    }
    return false;
}

int init_wifi()
{
    File wifi_file = SPIFFS.open(WIFI_FILE_NAME, "r");
    if (!wifi_file) {
        DEBUGV_EASYCOOL("[utils]failed to open wifi file\n");
        return -1;
    }

    size_t size = wifi_file.size();
    if (size > 512) {
        DEBUGV_EASYCOOL("[utils] wifi file size is too large\n");
        return -1;
    }

    std::unique_ptr<char[]> buf(new char[size]);
    wifi_file.readBytes(buf.get(), size);

    StaticJsonBuffer<512> json_buffer;
    JsonObject& json = json_buffer.parseObject(buf.get());

    wifi_file.close();

    if (!json.success() || !json.containsKey("ssid") || !json.containsKey("pass")) {
        DEBUGV_EASYCOOL("[utils] failed to parse wifi file\n");
        return -1;
    }

    DEBUGV_EASYCOOL("[utils] connecting to wifi: ssid: %s, pass: %s\n", json["ssid"].asString(), json["pass"].asString());
    return WiFi.begin(json["ssid"].asString(), json["pass"].asString());
}

bool save_key(JsonObject& json_root)
{
    File key_file = SPIFFS.open(KEY_FILE_NAME, "w");
    if (key_file) {
        json_root.printTo(key_file);
        key_file.close();
        return true;
    }
    return false;
}

bool load_key()
{
    File key_file = SPIFFS.open(KEY_FILE_NAME, "r");
    if (!key_file) {
        DEBUGV_EASYCOOL("[utils] failed to open key file\n");
        return false;
    }

    size_t size = key_file.size();
    if (size > 512) {
        DEBUGV_EASYCOOL("[utils] key file size is too large\n");
        return false;
    }

    std::unique_ptr<char[]> buf(new char[size]);
    key_file.readBytes(buf.get(), size);

    StaticJsonBuffer<512> json_buffer;
    JsonObject& json = json_buffer.parseObject(buf.get());

    key_file.close();

    if (!json.success() || !json.containsKey("key")) {
        DEBUGV_EASYCOOL("[utils] failed to parse key file\n");
        return false;
    }

    key = json["key"].asString();
    DEBUGV_EASYCOOL("[utils] key loaded: %s\n", key.c_str());

    return true;
}

String get_key()
{
    return key;
}
