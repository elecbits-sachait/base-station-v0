#include "wifi_to_hotspot.h"

ESP8266WebServer server(80);

wifi_to_hotspot::wifi_to_hotspot()
{
    if (!SPIFFS.begin())
    {
        ESP.reset();
    }
    update_wifi_list();
}
void wifi_to_hotspot::config(const char *ssid, const char *password)
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid, password, 1, false, 1);
    IPAddress ip(192, 168, 1, 1);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress mask(255, 255, 255, 0);
    WiFi.softAPConfig(ip, gateway, mask);
    server.on("/", HTTP_GET, main_page);
    server.on("/", HTTP_POST, get_data);
    server.onNotFound(notfound);
    server.begin();
}
void wifi_to_hotspot::main_page()
{
    uint8_t n = WiFi.scanNetworks();
    String web_page = "<div id=\"wifiList\"  class=\"wifiList\">";
    for (uint8_t i = 0; i < n; i++)
    {
        web_page += WiFi.SSID(i);
        web_page += ",";
    }
    web_page += "</div>";
    File wifi_page_file = SPIFFS.open(WEBPAGE, "r");
    if (!wifi_page_file)
    {
        //Serial.println("can't open web");
    }
    if (wifi_page_file.available())
    {
        web_page += wifi_page_file.readString();
    }
    wifi_page_file.close();
    server.send(200, "text/html", web_page);
    wait_for_sec(5);
}
void wifi_to_hotspot::get_data()
{
    String data = server.arg("plain");
    File wifi_config_file = SPIFFS.open(WIFI_SAVE_FILE, "a");
    if (!wifi_config_file)
    {
       // Serial.println("can't open file");
    }
    if (wifi_config_file.print(data))
    {
        //Serial.println("File content was appended");
        server.send(200, "text/plain", data);
    }
    else
    {
       // Serial.println("File append failed");
        server.send(200, "text/plain", "ERROR! refresh page and try again");
    }
    wifi_config_file.close();
    wait_for_sec(2);
    update_wifi_list();
}
void wifi_to_hotspot::notfound()
{
    server.send(200, "text/html", "<a href=\"http://192.168.1.1/\">CLICK HERE</a>");
}
void wifi_to_hotspot::update_wifi_list()
{
    File wifi_config_file = SPIFFS.open(WIFI_SAVE_FILE, "r");
    if (!wifi_config_file)
    {
       // Serial.println("can't open file");
    }
    if (wifi_config_file.available())
    {
        wifi_string = wifi_config_file.readString().c_str();
        //Serial.println(wifi_string.c_str());
    }
    wifi_config_file.close();
}
pair<const char *, const char *> wifi_to_hotspot::get_connected_wifi(uint16_t timeout)
{
    if (wifi_string.length() > 0)
    {
        char *wifi_list = strdup(wifi_string.c_str());
        char *current_ptr = strtok(wifi_list, "|");
        const char*ssid;
        const char* pass;
        uint8_t n = WiFi.scanNetworks();
        while (current_ptr != NULL)
        {
            ssid = current_ptr;
            current_ptr = strtok(NULL, "|");
            pass = current_ptr;
            for (uint8_t j = 0; j < n; j++)
            {
                /*Serial.print(WiFi.SSID(j));
                Serial.print("  ");
                Serial.print(j);
                Serial.print("  ");
                Serial.println(ssid);*/
                if (strcmp(ssid, WiFi.SSID(j).c_str()) == 0)
                {
                    if (connect_to_wifi(ssid, pass, timeout))
                    {
                       /* Serial.print("ssid= ");
                        Serial.print(ssid);
                        Serial.print("  password= ");
                        Serial.println(pass);*/
                        return make_pair(ssid, pass);
                    }
                }
            }
            current_ptr = strtok(NULL, "|");
        }
        free(wifi_list);
        free(current_ptr);
    }
    return make_pair("", "");
}
bool wifi_to_hotspot::connect_to_wifi(const char *ssid, const char *pass, uint16_t timeout)
{
    WiFi.begin(ssid, pass);
    uint64_t m = millis();
    while (millis() - m < timeout * 1000)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            if (check_internet())
            {
                WiFi.mode(WIFI_STA);
                return true;
            }
            else
            {
                WiFi.disconnect();
                return false;
            }
        }
        server.handleClient();
        ESP.wdtFeed();
    }
    return false;
}
bool wifi_to_hotspot::check_internet()
{
    HTTPClient http;
    http.begin(INTERNET_CHECK_LINK);
    if (http.GET() == HTTP_CODE_OK)
    {
        return true;
    }
    http.end();
    return false;
}
void wifi_to_hotspot::wait_for_sec(uint16_t sec)
{
    uint64_t start_millis = millis();
    while (millis() - start_millis < sec * 1000)
    {
        server.handleClient();
        ESP.wdtFeed();
    }
}