#include "nrf.h"

RF24 radio(16, 15);
NRF::NRF()
{
 
}
void NRF::tx_data(const char *text)
{
  radio.openWritingPipe(rAddress[0]);
  radio.stopListening();
  radio.write(text, sizeof(text));
  radio.openReadingPipe(0, rAddress[1]);
  radio.openReadingPipe(1, rAddress[2]);
  radio.openReadingPipe(2, rAddress[3]);
  radio.openReadingPipe(3, rAddress[4]);
  radio.startListening();
}
char NRF::rx_data()
{
  if (radio.available())
  {
    char text[2];
    radio.read(&text, sizeof(text));
    //Serial.println(text);
    return text[0];
  }
  return 'X';
}