#ifndef nrf_h
#define nrf_h
#include <Arduino.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
///A -> AUTO_MODE
//B -> MANUAL_MODE
//P -> PAUSE ON
//S -> SENSOR TRIGER
//O -> ACKNOWLEDGEMENT
//? -> ASK_MODE
//X -> NULL
#define BASE_NAME   002
#define BASE_MASK   ((uint64_t)BASE_NAME<<8)

#define BASE_ADDR     (BASE_NAME)
#define BUTTON_ADDR  BASE_MASK  | (BASE_NAME+1)
#define PIR_ADDR  BASE_MASK  | (BASE_NAME+2)
#define DOOR_ADDR  BASE_MASK  | (BASE_NAME+3)
#define VIB_ADDR  BASE_MASK  | (BASE_NAME+4)
#define CAM_ADDR BASE_MASK | (BASE_NAME+5)
extern RF24 radio;
//const uint64_t rAddress[] = {0x7878787878LL, 0xB3B4B5B6F1LL, 0xB3B4B5B6F2LL, 0xB3B4B5B6F3LL, 0xB3B4B5B6F4LL};
const uint64_t rAddress[]={BASE_ADDR,BUTTON_ADDR,PIR_ADDR,DOOR_ADDR,VIB_ADDR,CAM_ADDR};
class NRF {
  public:
  NRF();
  static void tx_data(const char *);
  static char rx_data();
};

#endif
