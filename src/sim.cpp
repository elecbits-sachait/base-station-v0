#include "sim.h"

bool GSM::call(const char *number, uint16_t timeout)
{
  GSM::send_at("ATH", 1, false);
  //GSM::send_at("AT+COLP=0", 1, false);
  GSM::send_at("ATD" + String(number) + ";", 2, false);
  //GSM::send_at("AT+COLP=1", 1, false);
  uint64_t t = millis();
  while (millis() - t < timeout * 1000)
  {
    if (Serial.available())
    {
      String response = Serial.readString();
      if (response.indexOf("+COLP") > -1)
      {
        return true;
      }
      if (response.indexOf("NO") > -1 || response.indexOf("BUSY") > -1)
      {
        return false;
      }
    }
    ESP.wdtFeed();
  }
  return false;
}
bool GSM::msg(const char *number, uint16_t timeout, const char *content)
{
  Serial.write("AT+CMGS=\"");
  Serial.write(number);
  Serial.write("\"\r");
  delay(200);
  Serial.write(content);
  Serial.write(char(26));
  Serial.write("\r");
  uint64_t t = millis();
  while (millis() - t < timeout * 1000)
  {
    if (Serial.available())
    {
      String response = Serial.readString();
      if (response.indexOf("OK") > -1 || response.indexOf("ERROR") > -1)
      {
        return true;
      }
    }
    ESP.wdtFeed();
  }
  return false;
}
bool GSM::inbox(String temp[3])
{
  String message = send_at("AT+CMGR=1", 2, false);
  if (message.indexOf("REC") > -1)
  {
    String msg_content_arr[5] = {""};
    for (uint16_t i = 0, j = 0; i <= message.length(); i++)
    {
      if (j == 4)
      {
        msg_content_arr[j] += message[i];
      }
      else if (message[i] == '"')
      {
        i++;
        while (message[i] != '"')
        {
          msg_content_arr[j] += message[i];
          i++;
        }
        j++;
      }
    }
    temp[0] = msg_content_arr[1];
    temp[1] = msg_content_arr[3];
    temp[2] = msg_content_arr[4];
    send_at("AT+CMGD=1", 2, true);
    return true;
  }
  return false;
}
String GSM::send_at(String command, uint16_t timeout, bool repeat)
{
top:
  String response = "";
  Serial.write(command.c_str());
  Serial.write("\r");
  uint64_t t = millis();
  while (millis() - t < timeout * 1000)
  {
    if (Serial.available())
    {
      response = Serial.readString();
      if (response.indexOf("ERROR") > -1 && repeat == true)
      {
        goto top;
      }
      if (response.indexOf("OK") > -1)
      {
        return response;
      }
    }
    ESP.wdtFeed();
  }
  return response;
}
void GSM::check()
{
  pinMode(5, OUTPUT);
  uint8_t counter = 0;
  //start:
  //counter = 0;
  digitalWrite(5, HIGH);
  delay(100);
  digitalWrite(5, LOW);
  delay(3000);
  while (GSM::send_at("AT", 1, true).indexOf("OK") == -1)
  {
    ESP.wdtFeed();
    counter++;
    if (counter > 4)
    {
      //goto start;
      ESP.reset();
    }
  }
  /*while (GSM::send_at("AT", 2, true).indexOf("OK") == -1)
  {
  }*/
  /*GSM::send_at("AT&F", 1, true);
  GSM::send_at("AT+CMGF=1", 1, true);
  GSM::send_at("ATS0=1", 1, true);
  GSM::send_at("AT+COLP=1", 1, true);
  GSM::send_at("AT+CLTS=1", 1, true);
  GSM::send_at("ATE0", 1, true);
  GSM::send_at("AT&W", 1, true);*/
  //return GSM::send_at("AT+CSPN?", 1, false);
  /*
  at&f
  at+colp=1
  at+clts=1
   GSM::send_at("AT+CMGF=1", 2, true);
  GSM::send_at("ATS0=1", 2, true);
  GSM::send_at("ATE0", 2, true);
  at+csmins=1
at+spe=1
at+sndlevel=0,100
at+sndlevel=1,100

  at+cmte? //temp
  at+sttone=1,16,500
at+cldtmf=1,"F",100
  */
}
void GSM::whitelist(const char *number, uint8_t index)
{
  GSM::send_at("AT+CWHITELIST=2," + String(index) + "," + number, 1, true);
}
void GSM::clear_whitelist(uint8_t index)
{
  GSM::send_at("AT+CWHITELIST=2," + String(index) + ",0000000000", 1, true);
}
String GSM::ussd_at(String ussd, uint16_t timeout)
{
  GSM::send_at("AT+CUSD=1,\"" + String(ussd) + "\"", 1, false);
  uint64_t t = millis();
  while (millis() - t < timeout * 1000)
  {
    if (Serial.available())
    {
      String response = Serial.readString();
      if (response.indexOf("+CUSD") > -1)
      {
        return response;
      }
    }
    ESP.wdtFeed();
  }
  return "TIMEOUT";
}