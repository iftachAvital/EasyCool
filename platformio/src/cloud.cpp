#include "cloud.h"

unsigned long last_update;

bool update_sensor_fail()
{
    DEBUGV_EASYCOOL("[cloud] start update sensor fail\n");
    return update_cloud(COMMAND_SENSOR_FAULT);
}

bool update_cloud(int command)
{
    int res_code = make_cloud_request(command);

    switch (res_code) {
        case HTTP_CODE_OK:
            last_update = millis() + UPDATE_RATE;
            return true;

        case HTTP_CODE_UNAUTHORIZED:
            handle_sos_error(BAD_KEY);
            return false;

        default:
            WiFi.reconnect();
            return false;
    }
}

void handle_cloud()
{
    static bool is_init = false;

    if (!is_init) {
        DEBUGV_EASYCOOL("[cloud] start cloud init\n");
        is_init = update_cloud(COMMAND_INIT);
    }
    else if (millis() > last_update) {
        DEBUGV_EASYCOOL("[cloud] start update cloud\n");
        update_cloud(COMMAND_UPDATE);
    }
}

int make_cloud_request(int command)
{
    HTTPClient client;
    String to_send;
    String res = "";

    client.begin(URL, PORT, PATH);
    client.addHeader("Content-Type", "application/json");
    client.setTimeout(HTTP_TIMEOUT);

    StaticJsonBuffer<1024> json_buffer;
    JsonObject& root = json_buffer.createObject();

    JsonObject& data = root.createNestedObject("data");
    JsonObject& wifi = data.createNestedObject("wifi");
    JsonObject& json_system = data.createNestedObject("system");

    switch (command) {
        case COMMAND_INIT:
            json_system["easycool_version"] = get_easycool_version();
            json_system["reset_info"] = ESP.getResetInfo();
            json_system["reset_reason"] = ESP.getResetReason();
            json_system["core_version"] = ESP.getCoreVersion();
            json_system["boot_mode"] = ESP.getBootMode();
            json_system["boot_version"] = ESP.getBootVersion();
            json_system["cpu_freq"] = ESP.getCpuFreqMHz();
            json_system["cycle_count"] = ESP.getCycleCount();
            json_system["flash_id"] = ESP.getFlashChipId();
            json_system["flash_real_size"] = ESP.getFlashChipRealSize();
            json_system["flash_size"] = ESP.getFlashChipSize();
            json_system["free_sketch"] = ESP.getFreeSketchSpace();
            json_system["sdk_version"] = ESP.getSdkVersion();
        case COMMAND_UPDATE:
            data["temp"] = get_temp();
            break;
    }

    switch (command) {
        case COMMAND_INIT:
            root["command"] = "initiate";
            break;
        case COMMAND_UPDATE:
            root["command"] = "update";
            break;
        case COMMAND_SENSOR_FAULT:
            root["command"] = "sensor_fail";
            break;
    }

    wifi["channel"] = WiFi.channel();
    wifi["gateway_ip"] = WiFi.gatewayIP().toString();
    wifi["dns_ip"] = WiFi.dnsIP().toString();
    wifi["hostname"] = WiFi.hostname();
    wifi["local_ip"] = WiFi.localIP().toString();
    wifi["rssi"] = WiFi.RSSI();
    wifi["ssid"] = WiFi.SSID();

    json_system["free_heap"] = ESP.getFreeHeap();

    root["version"] = API_VERSION;
    root["device_id"] = get_device_id();
    root["key"] = get_key();
    root["data"] = data;

    root.printTo(to_send);

    int http_code = client.POST(to_send);

    DEBUGV_EASYCOOL("[cloud] received http code: %d\n", http_code);

    if (http_code == HTTP_CODE_OK) {
        res = client.getString();
        DEBUGV_EASYCOOL("[cloud] respond: %s\n", res.c_str());
    }

    client.end();
    return http_code;
}
