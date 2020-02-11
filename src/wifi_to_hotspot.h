#ifndef wifi_to_hotspot_h
#define wifi_to_hotspot_h

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FS.h"
#include <ESP8266HTTPClient.h>
#include <utility>
#define WIFI_SAVE_FILE "/wifi_config.txt"
#define WEBPAGE "/wifi_page.html"
#define INTERNET_CHECK_LINK "http://www.google.com"
using std::string;
using std::pair;
using std::make_pair;

extern ESP8266WebServer server;
static string wifi_string;

class wifi_to_hotspot
{
public:
    //configuration function
    wifi_to_hotspot();
    static void config(const char *, const char *);
    //local webpages
    static void main_page();
    static void get_data();
    static void notfound();
    //wifi hotspot switching finction
    static pair<const char *, const char *> get_connected_wifi(uint16_t);
    static bool connect_to_wifi(const char *, const char *, uint16_t);
    static bool check_internet();
    static void wait_for_sec(uint16_t);
    static void update_wifi_list();
};

#endif