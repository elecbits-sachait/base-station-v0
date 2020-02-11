#include "wifi_to_hotspot.h"
#include <WiFiClientSecure.h>
#include "MQTT.h"
#include <ArduinoJson.h>
#include "sim.h"
#include "nrf.h"

using BearSSL::X509List;
using BearSSL::PrivateKey;

#define MQTT_HOST "a1lgz1948lk3nw-ats.iot.ap-south-1.amazonaws.com"
#define MQTT_PORT 8883
#define THINGNAME "test_1"
#define SET_WILL "device/test_1/state"

#define SUB_GET_ACCEPTED "$aws/things/test_1/shadow/get/accepted"
#define SUB_UPDATE_DELTA "$aws/things/test_1/shadow/update/delta"
#define SUB_SIM_GET "device/test_1/sim_command"

#define PUB_GET "$aws/things/test_1/shadow/get"
#define PUB_UPDATE "$aws/things/test_1/shadow/update"
#define PUB_SENSOR "device/test_1/sensor"
#define PUB_SIM_POST "device/test_1/sim_response"

#define WIFI_SSID "DEVICE_test_1"
#define WIFI_PASSWORD "DEVICE_test_1"

#define STATE_TAG "state"
#define REPORTED_TAG "reported"
#define DELTA_TAG "delta"
#define CONNECTED_TAG "connected"
#define AUTO_NUM_TAG "auto_num"
#define MAN_NUM_TAG "man_num"
#define ADMIN_NUM_TAG "admin_num"
#define MODE_TAG "mode"
#define DEVICE_ID_TAG "deviceId"
#define USSD_TAG "USSD"
#define CALL_TAG "CALL"
#define SIGNAL_TAG "SIGNAL"
#define SIM_TAG "OPERATOR"
#define SIM "sim_card"
#define CALL_STATE_TAG "call_mode"
#define MSG_STATE_TAG "msg_mode"
#define WIFI_SSID_TAG "wifi_ssid"
#define WIFI_PASSWORD_TAG "wifi_password"
#define LAST_RESET_TAG "last_reset"

#define AUTO_NUM_FILE "/auto_num.txt"
#define MAN_NUM_FILE "/man_num.txt"
#define ROOT_CA "/test_1/ROOT_CA.txt"
#define CLIENT_CRT "/test_1/certificate.crt"
#define KEY "/test_1/private.key"

DynamicJsonDocument receive_doc(4096);
DynamicJsonDocument send_doc(1024);
WiFiClientSecure net;
wifi_to_hotspot wifispot;
MQTTClient client;
GSM sim;
NRF nrf;

pair<const char *, const char *> connected_wifi;
bool sensor_mode = true;
bool sim_state = false;
bool call_state = true;
bool msg_state = true;
time_t now;

String read_fs(const char *);
bool write_fs(String, const char *);
void connect_MQTT();
void connect_internet();
void set_will();
void emergency_loop(const char *, const char *);
void _loop();
void check_msg();
void nrf_mode();
void nrf_check();

X509List cert(read_fs(ROOT_CA).c_str());
X509List client_crt(read_fs(CLIENT_CRT).c_str());
PrivateKey key(read_fs(KEY).c_str());

void messageReceived(String &topic, String &payload)
{
  sim.send_at("at+sttone=1,16,500", 1, false);
  if (topic == SUB_GET_ACCEPTED || topic == SUB_UPDATE_DELTA)
  {
    deserializeJson(receive_doc, payload);
    String _state = receive_doc[STATE_TAG];
    deserializeJson(receive_doc, _state);
    if (topic == SUB_GET_ACCEPTED)
    {
      String _desired = receive_doc[DELTA_TAG];
      deserializeJson(receive_doc, _desired);
    }
    JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
    reported[CONNECTED_TAG] = "true";
    reported[DEVICE_ID_TAG] = THINGNAME;
    reported[WIFI_SSID_TAG] = connected_wifi.first;
    reported[WIFI_PASSWORD_TAG] = connected_wifi.second;
    reported[SIM] = String(sim_state);
    reported[LAST_RESET_TAG] = String(millis() / 1000) + "sec ago";

    if (receive_doc[CALL_STATE_TAG].size() > 0)
    {
      if (receive_doc[CALL_STATE_TAG][0] == "OFF")
      {
        call_state = false;
      }
      else if (receive_doc[CALL_STATE_TAG][0] == "ON")
      {
        call_state = true;
      }
    }
    JsonArray call_state_json = reported.createNestedArray(CALL_STATE_TAG);
    if (call_state)
    {
      call_state_json.add("ON");
    }
    else
    {
      call_state_json.add("OFF");
    }

    if (receive_doc[MSG_STATE_TAG].size() > 0)
    {
      if (receive_doc[MSG_STATE_TAG][0] == "OFF")
      {
        msg_state = false;
      }
      else if (receive_doc[MSG_STATE_TAG][0] == "ON")
      {
        msg_state = true;
      }
    }
    JsonArray msg_state_json = reported.createNestedArray(MSG_STATE_TAG);
    if (msg_state)
    {
      msg_state_json.add("ON");
    }
    else
    {
      msg_state_json.add("OFF");
    }

    if (receive_doc[AUTO_NUM_TAG].size() > 0)
    {
      JsonArray auto_num_json = reported.createNestedArray(AUTO_NUM_TAG);
      if (write_fs(receive_doc[AUTO_NUM_TAG], AUTO_NUM_FILE))
      {
        for (uint8_t i = 0; i < receive_doc[AUTO_NUM_TAG].size(); i++)
        {
          auto_num_json.add(receive_doc[AUTO_NUM_TAG][i]);
        }
      }
      else
      {
        auto_num_json.add("erroe_adding_number_to_fs");
      }
    }
    if (receive_doc[MAN_NUM_TAG].size() > 0)
    {
      JsonArray man_num_json = reported.createNestedArray(MAN_NUM_TAG);
      if (write_fs(receive_doc[MAN_NUM_TAG], MAN_NUM_FILE))
      {
        for (uint8_t i = 0; i < receive_doc[MAN_NUM_TAG].size(); i++)
        {
          man_num_json.add(receive_doc[MAN_NUM_TAG][i]);
        }
      }
      else
      {
        man_num_json.add("erroe_adding_number_to_fs");
      }
    }
    if (receive_doc[ADMIN_NUM_TAG].size() > 0)
    {
      JsonArray admin_num_json = reported.createNestedArray(ADMIN_NUM_TAG);
      for (uint8_t i = 0; i < receive_doc[ADMIN_NUM_TAG].size(); i++)
      {
        sim.whitelist(receive_doc[ADMIN_NUM_TAG][i], i + 1);
        admin_num_json.add(receive_doc[ADMIN_NUM_TAG][i]);
      }
      for (uint8_t i = receive_doc[ADMIN_NUM_TAG].size(); i < 3; i++)
      {
        sim.clear_whitelist(i + 1);
      }
    }
    if (receive_doc[MODE_TAG].size() > 0)
    {
      if (receive_doc[MODE_TAG][0] == "AUTO")
      {
        sensor_mode = true;
        nrf_mode();
      }
      if (receive_doc[MODE_TAG][0] == "MANUAL")
      {
        sensor_mode = false;
        nrf_mode();
      }
    }
    JsonArray mode_json = reported.createNestedArray(MODE_TAG);
    if (sensor_mode)
    {
      mode_json.add("AUTO");
    }
    else
    {
      mode_json.add("MANUAL");
    }
    payload = "";
    serializeJson(send_doc, payload);
    //Serial.println(payload);
    client.publish(PUB_UPDATE, payload, 0, 1);
    receive_doc.clear();
    send_doc.clear();
  }
  if (topic == SUB_SIM_GET)
  {
    deserializeJson(receive_doc, payload);
    String _data;
    if (receive_doc[SIM_TAG].size() > 0)
    {
      send_doc[SIM_TAG] = sim.send_at("AT+CSPN?", 1, false);
    }
    if (receive_doc[CALL_TAG].size() > 0)
    {
      send_doc[CALL_TAG] = sim.call(receive_doc[CALL_TAG][0], 1);
    }
    if (receive_doc[SIGNAL_TAG].size() > 0)
    {
      send_doc[SIGNAL_TAG] = sim.send_at("AT+CSQ", 1, false);
    }
    if (receive_doc[USSD_TAG].size() > 0)
    {
      send_doc[USSD_TAG] = sim.ussd_at(receive_doc[USSD_TAG][0], 2);
    }
    serializeJson(send_doc, _data);
    //Serial.println(_data);
    client.publish(PUB_SIM_POST, _data, 0, 1);
    send_doc.clear();
    receive_doc.clear();
  }
}

void setup()
{
  WiFi.mode(WIFI_OFF);
  Serial.begin(9600);
  sim.check();
  /*if (sim.check().indexOf("OK") > -1)
  {
    sim_state = true;
  }
  else
  {
    sim_state = false;
  }*/
  radio.begin();
  radio.openReadingPipe(0, rAddress[1]);
  radio.openReadingPipe(1, rAddress[2]);
  radio.openReadingPipe(2, rAddress[3]);
  radio.openReadingPipe(3, rAddress[4]);
  radio.startListening();
  set_will();
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
  client.begin(MQTT_HOST, MQTT_PORT, net);
  client.onMessage(messageReceived);
  nrf_mode();
}

void loop()
{
  while (client.connected())
  {
    uint64_t m = millis();
    while (millis() - m < 10000)
    {
      client.loop();
      ESP.wdtFeed();
      nrf_check();
    }
    _loop();
  }
  connect_internet();
  check_msg();
  uint64_t m = millis();
  while (millis() - m < 10000)
  {
    server.handleClient();
    ESP.wdtFeed();
    nrf_check();
  }
}

String read_fs(const char *filename)
{
  String data;
  File file = SPIFFS.open(filename, "r");
  if (!file)
  {
    // Serial.println("can't open cert");
    return "ERROR";
  }
  if (file.available())
  {
    data = file.readString();
  }
  file.close();
  return data;
}
bool write_fs(String data, const char *filename)
{
  File file = SPIFFS.open(filename, "w");
  if (!file)
  {
    //Serial.println("can't open file");
    return false;
  }
  if (file.print(data))
  {
    //Serial.println("File content was appended");
    file.close();
    return true;
  }
  file.close();
  return false;
}
void connect_MQTT()
{
  configTime(1, 0, "pool.ntp.org");
  now = time(nullptr);
  uint8_t sntp_count = 0;
  while (now < 1578150958)
  {
    if (sntp_count == 9)
    {
      ESP.reset();
    }
    delay(1000);
    now = time(nullptr);
    sntp_count++;
  }
  if (client.connect(THINGNAME))
  {
    //Serial.println("connected!");
    if (!client.subscribe(SUB_GET_ACCEPTED))
    {
      // Serial.println("ERROR");
    }
    if (!client.subscribe(SUB_UPDATE_DELTA))
    {
      // Serial.println("ERROR2");
    }
    if (!client.subscribe(SUB_SIM_GET))
    {
      // Serial.println("ERROR3");
    }
    client.publish(PUB_GET, "", 0, 0);
  }
  else
  {
    // Serial.print("SSL Error Code: ");
    // Serial.println(net.getLastSSLError());
    ESP.reset();
  }
}
void connect_internet()
{
  connected_wifi = wifispot.get_connected_wifi(10);
  if (connected_wifi.first == "")
  {
    if (WiFi.getMode() != WIFI_AP_STA)
    {
      //Serial.println("hotspot");
      wifispot.config(WIFI_SSID, WIFI_PASSWORD);
    }
  }
  else
  {
    //Serial.print("wifi= ");
    //Serial.print(connected_wifi.first);
    //Serial.print("  password= ");
    //Serial.println(connected_wifi.second);
    connect_MQTT();
  }
}
void set_will()
{
  String will;
  JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
  reported[CONNECTED_TAG] = "false";
  reported[DEVICE_ID_TAG] = THINGNAME;
  serializeJson(send_doc, will);
  client.setWill(SET_WILL, will.c_str(), 0, 0);
  send_doc.clear();
}
void emergency_loop(const char *filename, const char *sensor_name)
{
  for (uint8_t i = 0; i <= 50; i++)
  {
    nrf.tx_data("P");
    delay(50);
    ESP.wdtFeed();
  }
  String content;
  now = time(nullptr);
  send_doc[DEVICE_ID_TAG] = THINGNAME;
  send_doc["time"] = now;
  send_doc["sensor"] = sensor_name;
  serializeJson(send_doc, content);
  deserializeJson(receive_doc, read_fs(filename));
  if (client.publish(PUB_SENSOR, content, 0, 1))
  {
    if (call_state)
    {
      for (uint8_t i = 0; i < receive_doc.size(); i++)
      {
        if(sim.call(receive_doc[i], 30)){
          break;
        }
      }
    }
    if (msg_state)
    {
      for (uint8_t i = 0; i < receive_doc.size(); i++)
      {
        sim.msg(receive_doc[i], 60, content.c_str());
      }
    }
  }
  else
  {
    for (uint8_t i = 0; i < receive_doc.size(); i++)
    {
           if(sim.call(receive_doc[i], 30)){
          break;
        }
          }
    for (uint8_t i = 0; i < receive_doc.size(); i++)
    {
      sim.msg(receive_doc[i], 60, content.c_str());
    }
  }
  check_msg();
  nrf_mode();
  receive_doc.clear();
  send_doc.clear();
}
void _loop()
{
  /* if (Serial.available())
  {
    String response = Serial.readString();
    if (response.indexOf("+CPIN: NOT READY") > -1)
    {
      response = "";
      JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
      sim_state = false;
      reported[SIM] = sim_state;
      serializeJson(send_doc, response);
      client.publish(PUB_UPDATE, response, 0, 1);
      send_doc.clear();
    }*/
  check_msg();
  if (sim.send_at("AT+CSPN?", 2, false).indexOf("ERROR") > -1&& sim_state==true)
  {
    String response;
    JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
    sim_state = false;
    reported[SIM] = String(sim_state);
    serializeJson(send_doc, response);
    client.publish(PUB_UPDATE, response, 0, 1);
    send_doc.clear();
  }
  else if (sim.send_at("AT+CSPN?", 2, false).indexOf("OK") > -1&& sim_state==false)
  {
    String response;
    JsonObject reported = send_doc.createNestedObject(STATE_TAG).createNestedObject(REPORTED_TAG);
    sim_state = true;
    reported[SIM] = String(sim_state);
    serializeJson(send_doc, response);
    client.publish(PUB_UPDATE, response, 0, 1);
    send_doc.clear();
  }
  //}
  ESP.wdtFeed();
}
void check_msg()
{
  String msg_content[3];
  if (sim.inbox(msg_content))
  {
    sim.send_at("at+sttone=1,16,500", 1, false);
    if (msg_content[2].indexOf("MANUAL") > -1 && sensor_mode == true)
    {
      String data;
      send_doc[MODE_TAG] = "MANUAL";
      serializeJson(send_doc, data);
      sensor_mode = false;
      client.publish(PUB_UPDATE, data, 0, 1);
      nrf_mode();
    }
    if (msg_content[2].indexOf("AUTO") > -1 && sensor_mode == false)
    {
      String data;
      send_doc[MODE_TAG] = "AUTO";
      serializeJson(send_doc, data);
      sensor_mode = true;
      client.publish(PUB_UPDATE, data, 0, 1);
      nrf_mode();
    }
    if (msg_content[2].indexOf("STATUS") > -1)
    {
      String content;
      content = "RECEIVED ON\n";
      content += msg_content[1];
      content += "\nREAD ON\n";
      content += "\n";
      now = time(nullptr);
      now += 19800;
      struct tm timeinfo;
      gmtime_r(&now, &timeinfo);
      content += asctime(&timeinfo);
      content += "\n";
      content += "\nSENSOR MODE:- ";
      content += sensor_mode;
      content += "\nSIM NETWORK:-";
      content += sim.send_at("AT+CSQ", 2, true);
      content += "\nSENT ON\n";
      now = time(nullptr);
      now += 19800;
      gmtime_r(&now, &timeinfo);
      content += asctime(&timeinfo);
      content += "\nLAST RESET\n";
      content += String(millis() / 1000);
      content += " seconds AGO\n";
      content += "\0";
      sim.msg(msg_content[0].c_str(), 5, content.c_str());
    }
  }
}
void nrf_mode()
{
  if (sensor_mode)
  {
    for (uint8_t i = 0; i <= 50; i++)
    {
      nrf.tx_data("A");
      delay(20);
      ESP.wdtFeed();
    }
  }
  else
  {
    for (uint8_t i = 0; i <= 50; i++)
    {
      nrf.tx_data("M");
      delay(20);
      ESP.wdtFeed();
    }
  }
}
void nrf_check()
{
  char nrf_data = nrf.rx_data();
  if (nrf_data != 'X')
  {
    if (nrf_data == '?')
    {
      nrf_mode();
    }
    if (sensor_mode)
    {
      if (nrf_data == 'D')
      {
        emergency_loop(AUTO_NUM_FILE, "DOOR");
      }
      if (nrf_data == 'I')
      {
        emergency_loop(AUTO_NUM_FILE, "PIR");
      }
      if (nrf_data == 'V')
      {
        emergency_loop(AUTO_NUM_FILE, "VIBRATION");
      }
          if (nrf_data == 'B')
    {
      emergency_loop(AUTO_NUM_FILE, "BUTTON");
    }
    return;
    }
    if (nrf_data == 'B')
    {
      emergency_loop(MAN_NUM_FILE, "BUTTON");
    }
  }
}