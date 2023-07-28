#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>
#include <esp_now.h>
#include <esp_wifi.h>

// // Trigger library dep detection
// #include "WiFiMulti.h"
// #include "Audio.h"
// #include "SPI.h"
// #include "SD.h"
// #include "FS.h"

// include additional files
#include "MyHeader.h"
#include "MyRFID.hpp"
#include "MyAudioPlayer.hpp"
#include "MyNeoPixel.hpp"

#define FW_VERS_STR "0.0.8" // prints on reboot

Ticker ms100_Timer; //

#define RSSI_MAX_VAL (0)
#define RSSI_MIN_VAL (-100)
int rssi_val = RSSI_MIN_VAL;      // to hold last rssi received
int rssi_max = RSSI_MIN_VAL;      // filtered rssi value
int rssi_val_prev = RSSI_MIN_VAL; //
int cnt_since_signal_lost;
void broadcast(const String &message);

void oneSecTask()
{
  static int x = 0;

  x++;
  if (x >= 2)
  { // 2 sec
    x = 0;
    broadcast("1"); // change id on other controllers here and than upload
  }
}

void ms100_task()
{
  static int x = 0;
  static int y = 0;

  y++;
  if (y >= 5)
  { // 0.5 sec
    y = 0;

    if (rssi_val_prev > rssi_max)
    { // set imedeately
      rssi_max = rssi_val_prev;
    }
    else
    { // apply RC filter

      int delta = rssi_val_prev - rssi_max;
      rssi_max += (delta / 4);
      if (rssi_max < RSSI_MIN_VAL)
        rssi_max = RSSI_MIN_VAL; // limit to min
    }
    Serial.printf("%d, %d\n", rssi_val_prev, rssi_max);

    if (cnt_since_signal_lost < 100)
      cnt_since_signal_lost++;

    rssi_val_prev -= cnt_since_signal_lost; // decrease rssi if signal is lost
    if (rssi_val_prev < RSSI_MIN_VAL)
      rssi_val_prev = RSSI_MIN_VAL; // limit to min
  }

  x++;
  if (x > 9)
  { // 1 sec
    x = 0;
    oneSecTask();
  }
#ifdef USE_MY_NEO_PIXEL
  MyNeoPixelMs100Task();
#endif
}

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
{
  // only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);
  // make sure we are null terminated
  buffer[msgLen] = 0;

  // update rssi_max value
  rssi_val_prev = rssi_val; // assign last received rssi
  // clear signal lost counter
  cnt_since_signal_lost = 0;

  // debug log the message to the serial port
  // Serial.println("Received ID:" + String(buffer) + ",RSSI:" + String(rssi_max));
}

// callback when data is sent
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  // Serial.print("Last Packet Sent to: ");
  // Serial.println(macStr);
  // Serial.print("Last Packet Send Status: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void broadcast(const String &message)
{
  // this will broadcast a message to everyone in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // add broadcast address to peer
  // otherwise you will get "Peer not found." error on esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  // broadcast message
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());

  if (result == ESP_OK)
  {
    // Serial.println("Broadcast message success");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    Serial.println("ESPNOW not Init.");
  }
  else if (result == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
  }
  else if (result == ESP_ERR_ESPNOW_INTERNAL)
  {
    Serial.println("Internal Error");
  }
  else if (result == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    Serial.println("Peer not found.");
  }
  else
  {
    Serial.println("Unknown error");
  }
}

typedef struct
{
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct
{
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  // !!! high rate calls - about 20 times in sec !!!

  int rssi = ppkt->rx_ctrl.rssi;

  rssi_val = rssi; // save last received
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  // Set device in STA mode to begin with
  WiFi.mode(WIFI_STA);
  Serial.print("\n\n\n");
  Serial.print("PoC ESPNow vers: ");
  Serial.println(FW_VERS_STR);
  // Output my MAC address - useful for later
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  // shut down wifi
  WiFi.disconnect();
  // startup ESP Now
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESPNow Init Success");
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    Serial.println("ESPNow Init Failed");
    delay(3000);
    ESP.restart();
  }
  // set rssi data callback
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);

#ifdef USE_MY_NEO_PIXEL
  MyNeoPixelSetup();
#endif

#ifdef USE_MY_AUDIO_PLAYER
  MyAudioPlayerSetup();
#endif

#ifdef USE_MY_RFID
  MyRFIDSetup();
#endif

  ms100_Timer.attach_ms(100, ms100_task); // every 100 ms, call ms100_task()
}

void loop()
{

#ifdef USE_MY_NEO_PIXEL
  MyNeoPixelTask();
#endif

#ifdef USE_MY_AUDIO_PLAYER
  MyAudioPlayerTask();
#endif

#ifdef USE_MY_RFID
  MyRFIDTask();
#endif
}