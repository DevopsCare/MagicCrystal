#include "MyHeader.h"

#define RFID_UART 1 // Serial# (0, 1 or 2) will be used for the loopback
#define RXPIN 17    //
#define TXPIN -1    // -1 = not used

typedef struct
{
  uint32_t uid;     /* rfid tag uid */
  const char *path; // path to mp3 file to play
} rfid_tag_t;

// declare RFID_Serial (as reference) related to RFID_UART number defined above (only for Serial1 and Serial2)
#if SOC_UART_NUM > 1 && RFID_UART == 1
HardwareSerial &RFID_Serial = Serial1;
#elif SOC_UART_NUM > 2 && RFID_UART == 2
HardwareSerial &RFID_Serial = Serial2;
#endif

int play_file = -1;

rfid_tag_t myTag[] = {
    // define tags used
    {12912668, "01.mp3"},
    {12455736, "02.mp3"},
    {0x0B690F4, "03.mp3"},
    {0x0C08495, "04.mp3"},
};

#define TAG_COUNT (sizeof(myTag) / sizeof(rfid_tag_t))

// ##################################################################

// General callback function for any UART -- used with a lambda std::function within HardwareSerial::onReceive()
void processOnReceiving(HardwareSerial &mySerial)
{
  uint8_t buff[4];
  uint32_t RxVal;

  if (mySerial.available() > sizeof(buff))
  {
    Serial.println("RFID Rx out of size");
    while (mySerial.available())
      mySerial.read(); // empty serial buffer
    return;            // skip out of size
  }

  int read_len = mySerial.readBytes(buff, sizeof(buff));

  if (read_len != sizeof(buff))
  {
    Serial.println("RFID Rx wrong size");
  }

  // convert rx buffer to uint32 value (reverse byte order)
  uint8_t *ptr8 = (uint8_t *)&RxVal;
  for (int i = sizeof(RxVal) - 1; i >= 0; i--)
  {
    *ptr8++ = buff[i];
  }

  // Serial.print(RxVal, HEX); //
  // Serial.println();

  for (int i = 0; i < TAG_COUNT; i++)
  {
    if (RxVal == myTag[i].uid)
    {
      play_file = i;
      Serial.print("RFID: 0x");
      Serial.print(RxVal, HEX); //
      Serial.println();
      break; // exit for loop
    }
  }
}

void MyRFIDSetup()
{

  Serial.println();
  Serial.print("RFID Initializing...");
  Serial.flush();

  RFID_Serial.begin(9600, SERIAL_8N1, RXPIN);
  RFID_Serial.setRxTimeout(6); // 6 bytes timeout before fire onReceive()
  RFID_Serial.onReceive([]()
                        { processOnReceiving(RFID_Serial); },
                        true);
  Serial.println(" Done");
}

void MyRFIDTask()
{

  if (play_file != -1)
  {
    if (play_file < TAG_COUNT)
      MyAudioPlayeFile(myTag[play_file].path);
    play_file = -1;
  }
}
