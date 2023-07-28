#include "WString.h"

//**********************************************************************************************************
//*    audioI2S-- I2S audiodecoder for ESP32,                                                              *
//*    https://github.com/schreibfaul1/ESP32-audioI2S                                                      *
//**********************************************************************************************************

#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"
#include "SPI.h"
#include "SD.h"
#include "FS.h"

// These pins will be used for SD Card
#define SD_CS_PIN 11 //
#define SD_CLK_PIN 6
#define SD_MOSI_PIN 8
#define SD_MISO_PIN 7
#define SD_CARD_DETECT 18

#define I2S_SD_PIN 2    // DIN Digital Input Signal
#define I2S_SCK_PIN 5   // BCLK Bit Clock Input
#define I2S_LRCLK_PIN 4 // LRCLK Frame Clock. Left/right clock for I2S and LJ mode. Sync clock for TDM mode
#define I2S_SD_MODE_PIN 21
/* I2S_SD_MODE_PIN modes:
  set output to LOW = shutdown
  set output to HIGH = Left ch @ R=1k
                       Right ch @ R=210k
                       Both LR ch @ R=633k
 */

// Digital I/O used
#define SD_CS SD_CS_PIN
#define SPI_MOSI SD_MOSI_PIN
#define SPI_MISO SD_MISO_PIN
#define SPI_SCK SD_CLK_PIN

#define I2S_DOUT I2S_SD_PIN
#define I2S_BCLK I2S_SCK_PIN
#define I2S_LRC I2S_LRCLK_PIN

Audio audio;
bool sd_card_detected;

// ##################################################################

void MyAudioPlayeFile(const char *path)
{
  static String path_prev("");
  String path_curr(path);

  if (sd_card_detected == false)
  {
    Serial.println("SD Card not detected");
    return;
  }

  if (audio.isRunning() && path_curr == path_prev)
  {
    // already playing
    return;
  }

  audio.connecttoFS(SD, path);
  path_prev = path_curr;

  Serial.print("Play: ");
  Serial.print(path); //
  Serial.println();
}

void MyAudioPlayerSetup()
{

  Serial.println();
  Serial.print("AudioPlayer Initializing...");
  Serial.flush();

  pinMode(SD_CARD_DETECT, INPUT);

  digitalWrite(I2S_SD_MODE_PIN, HIGH); //
  pinMode(I2S_SD_MODE_PIN, OUTPUT);

  digitalWrite(SD_CS, HIGH);
  pinMode(SD_CS, OUTPUT);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  // SPI.setFrequency(1000000);

  // SD.begin(SD_CS); // moved to card detect section

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21); // 0...21

  // audio.connecttoFS(SD, "02.mp3");

  Serial.println(" Done");
}

void MyAudioPlayerTask()
{
  static bool pin;
  static bool pin_prev = !pin;
  static unsigned long lastMillis = millis();

  pin = digitalRead(SD_CARD_DETECT);

  if ((millis() - lastMillis > 100) && (pin != pin_prev))
  {
    lastMillis = millis();
    pin_prev = pin;
    if (pin == HIGH)
    { // sd card removed
      audio.stopSong();
      SD.end();
      sd_card_detected = false;
      Serial.println("SD card removed");
    }
    else
    { // card detected
      Serial.println("SD card inserted");
      if (SD.begin(SD_CS) == true)
      {
        sd_card_detected = true;
      }
      else
      {
        Serial.println("SD card mount failed");
      }
    }
  }

  audio.loop();
}
