#include "setup.h"

unsigned long setup_timeout;
bool setup_exit_flag;

void reset_setup_timeout()
{
    setup_timeout = millis() + SETUP_MANAGER_TIMEOUT;
}

void handle_setup_button_in_manager()
{
    if (digitalRead(SETUP_BUTOON_PIN) == LOW) {
        DEBUGV_EASYCOOL("[setup] button pressed\n");

        unsigned long reset_time = millis() + EXIT_MANAGER_BUTTON_TIME;
        while (digitalRead(SETUP_BUTOON_PIN) == LOW) {
            if (millis() > reset_time) {
                flip_leds(true, true);
                setup_exit_flag = true;
            }
            delay(100);
        }
    }
}

void generate_info(char* buffer, size_t max_size)
{
    StaticJsonBuffer<1024> buffer_info;
    JsonObject& json_root = buffer_info.createObject();

    json_root["command"] = "get_info";
    JsonObject& json_data = json_root.createNestedObject("result");

    json_data["easycool_version"] = get_easycool_version();
    json_data["reset_info"] = ESP.getResetInfo();
    json_data["reset_reason"] = ESP.getResetReason();
    json_data["core_version"] = ESP.getCoreVersion();
    json_data["boot_mode"] = ESP.getBootMode();
    json_data["boot_version"] = ESP.getBootVersion();
    json_data["cpu_freq"] = ESP.getCpuFreqMHz();
    json_data["cycle_count"] = ESP.getCycleCount();
    json_data["free_sketch"] = ESP.getFreeSketchSpace();
    json_data["sdk_version"] = ESP.getSdkVersion();
    json_data["temp"] = get_temp();
    json_data["rssi"] = WiFi.RSSI();
    json_data["ssid"] = WiFi.SSID();
    json_data["wifi_status"] = (int) WiFi.status();
    json_data["free_memory"] = ESP.getFreeHeap();
    json_data["chip_id"] = get_device_id();
    json_data["flash_id"] = ESP.getFlashChipId();
    json_data["flash_size"] = ESP.getFlashChipSize();
    json_data["flash_real_size"] = ESP.getFlashChipRealSize();
    json_data["key"] = get_key();

    json_root.printTo(buffer, max_size);
}

void handle_info(WiFiClient &client)
{
    DEBUGV_EASYCOOL("[setup] handle info\n");

    char buffer[1024];
    generate_info(buffer, 1024);

    client.print(buffer);
    client.flush();
}

void handle_update_firmware(WiFiClient &client, JsonObject &json_res)
{
    DEBUGV_EASYCOOL("[setup] start firmware update\n");

    int command = json_res["data"]["command"];
    size_t size = json_res["data"]["size"];
    String md5 = json_res["data"]["md5"];

    DEBUGV_EASYCOOL("[setup] command: %d, size: %d, md5: %s\n", command, size, md5.c_str());

    bool update_begin = Update.begin(size, command);
    bool update_setmd5 = Update.setMD5(md5.c_str());
    DEBUGV_EASYCOOL("[setup] begin: %d, setMD5: %d\n", update_begin, update_setmd5);

    client.printf("{\"command\":\"update_firmware\",\"begin\":%d}", update_begin);
    client.flush();

    if (update_begin) {
        uint32_t written;
        uint32_t total = 0;

        while (!Update.isFinished() && client.connected()) {
            if (client.available()) {
                written = Update.write(client);

                flip_leds(true, true);

                client.printf("{\"command\":\"update_firmware\",\"received\":%d}", written);
                client.flush();
                total += written;
            }
        }

        bool update_end = Update.end();
        DEBUGV_EASYCOOL("[setup] end: %d\n", update_end);

        client.printf("{\"command\":\"update_firmware\",\"end\":%d}", update_end);
        client.flush();
        client.stop();

        setup_exit_flag = true;
    }
}

void generate_scan(char* buffer, size_t max_size)
{
    StaticJsonBuffer<1024> buffer_scan;
    JsonObject& json_root = buffer_scan.createObject();

    json_root["command"] = "scan_wifi";

    WiFi.scanNetworks(true);

    int8_t scan_result = WiFi.scanComplete();

    while (scan_result == WIFI_SCAN_RUNNING) {
        scan_result = WiFi.scanComplete();
        delay(50);
    }

    DEBUGV_EASYCOOL("[setup] scan result: %d\n", scan_result);

    if (scan_result >= 0) {
        JsonObject& json_data = json_root.createNestedObject("result");
        json_data["count"] = scan_result;

        JsonArray& json_networks_array = json_data.createNestedArray("networks");

        for(int8_t i = 0; i < scan_result; ++i) {
            String ssid_scan;
            int32_t rssi_scan;
            uint8_t sec_scan;
            uint8_t* BSSID_scan;
            int32_t chan_scan;
            bool hidden_scan;

            WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan, hidden_scan);
            DEBUGV_EASYCOOL(" %d: [%d][%02X:%02X:%02X:%02X:%02X:%02X] %s (%d) %c\n", i, chan_scan, BSSID_scan[0], BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5], ssid_scan.c_str(), rssi_scan, (sec_scan == ENC_TYPE_NONE) ? ' ' : '*');

            JsonObject& json_networks = json_networks_array.createNestedObject();
            json_networks["ssid"] = ssid_scan;
            json_networks["rssi"] = rssi_scan;
            yield();
        }
    }
    else {
        json_root["result"] = false;
    }

    json_root.printTo(buffer, max_size);
}

void handle_scan_wifi(WiFiClient &client)
{
    DEBUGV_EASYCOOL("[setup] start scan wifi\n");

    char buffer[1024];
    generate_scan(buffer, 1024);

    client.print(buffer);
    client.flush();
}

void handle_connect_wifi(WiFiServer &soft_ap_server, WiFiClient &client, JsonObject &json_res)
{
    DEBUGV_EASYCOOL("[setup] start connect wifi\n");

    client.printf("{\"command\":\"connect_wifi\",\"result\":%d}", true);
    client.flush();

    client.stop();
    soft_ap_server.stop();

    set_leds(false, false);

    WiFi.mode(WIFI_STA);

    delay(500);

    wl_status_t status = WiFi.status();

    if (status != WL_DISCONNECTED || status != WL_NO_SSID_AVAIL || status != WL_IDLE_STATUS || status != WL_CONNECT_FAILED) {
        DEBUGV_EASYCOOL("[setup] start wifi status: %d, disconnecting\n", status);
        WiFi.disconnect();
        delay(500);
    }

    DEBUGV_EASYCOOL("[setup] connecting to ssid: %s, pass: %s\n", json_res["data"]["ssid"].asString(), json_res["data"]["pass"].asString());

    WiFi.begin(json_res["data"]["ssid"].asString(), json_res["data"]["pass"].asString());
    status = WiFi.status();

    while(status != WL_CONNECTED && status != WL_NO_SSID_AVAIL && status != WL_CONNECT_FAILED) {
        flip_leds(true, false);
        delay(100);
        status = WiFi.status();
    }

    DEBUGV_EASYCOOL("[setup]returned status: %d\n", status);
    set_leds(false, false);
    delay(500);
    block_sos(status == WL_CONNECTED ? BLUE_LED_PIN : RED_LED_PIN, status, 500);

    if (status == WL_CONNECTED) {
        bool save_res = save_wifi(json_res["data"].asObject());
        DEBUGV_EASYCOOL("[setup] saving wifi creds: %d\n", save_res);
    }

    setup_exit_flag = true;
}

void handle_set_key(WiFiClient &client, JsonObject &json_res)
{
    DEBUGV_EASYCOOL("[setup] start set key\n");
    DEBUGV_EASYCOOL("[setup] recieved key: %s\n", json_res["data"]["key"].asString());

    bool save_res = save_key(json_res["data"].asObject());
    DEBUGV_EASYCOOL("[setup] saving key: %d\n", save_res);

    client.printf("{\"command\":\"set_key\",\"result\":%d}", save_res);
    client.flush();
}

void setup_manager()
{
    DEBUGV_EASYCOOL("[setup] start setup manager\n");

    set_leds(true, true);

    IPAddress soft_ap_server_ip(192,168,4,1);
    WiFiServer soft_ap_server(soft_ap_server_ip, SOFT_AP_SERVER_PORT);

    String ap_name = "EasyCool " + get_device_id();
    String ap_pass = AP_PASS;

    WiFi.disconnect();

    reset_setup_timeout();

    WiFi.mode(WIFI_AP_STA);
    delay(100);
    WiFi.softAP(ap_name.c_str(), ap_pass.c_str());
    delay(1000);
    soft_ap_server.begin();

    setup_exit_flag = false;

    while (millis() < setup_timeout && !setup_exit_flag) {
        WiFiClient client = soft_ap_server.available();

        if (client) {
            if (client.connected()) {
                reset_setup_timeout();
                DEBUGV_EASYCOOL("[setup] connected to client\n");

                handle_info(client);

                while (client.connected() && !setup_exit_flag) {
                    if (client.available()) {
                        String res = client.readString();

                        DynamicJsonBuffer buffer_rescived;
                        JsonObject& json_res = buffer_rescived.parseObject(res);

                        if(json_res.containsKey("command")) {
                            String command = json_res["command"].asString();

                            if (command.equals("scan_wifi")) {
                                handle_scan_wifi(client);
                            }

                            else if (command.equals("connect_wifi")) {
                                handle_connect_wifi(soft_ap_server, client, json_res);
                            }

                            else if (command.equals("set_key")) {
                                handle_set_key(client, json_res);
                            }

                            else if (command.equals("update_firmware")) {
                                handle_update_firmware(client, json_res);
                            }
                        }
                    }

                    yield();
                    handle_setup_button_in_manager();
                }
            }

            DEBUGV_EASYCOOL("[setup] disconnected from client\n");
            // close the connection:
            client.stop();
            break;
        }

        yield();
        handle_setup_button_in_manager();
    }

    soft_ap_server.stop();

    WiFi.mode(WIFI_STA);

    DEBUGV_EASYCOOL("[setup] exit setup manager\n");

    restart();
}
