// NeoPixel
// https://github.com/Makuna/NeoPixelBus

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <Ticker.h>
#include "MyHeader.h"

Ticker stripTimer;       //
#define MY_PIXEL_COUNT 4 // this example assumes 4 pixels, making it smaller will cause a failure
const uint16_t PixelCount = MY_PIXEL_COUNT;

#define colorSaturation 128 // 1-255

NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt3Ws2812xMethod> strip(PixelCount, 12); // Pavels Test board on pin12
NeoPixelBus<NeoGrbFeature, NeoSk6812Method> stripESP(PixelCount, 38);        // ESP32  onboard LED on pin38

RgbColor red(255, 0, 0);
RgbColor orange(200, 80, 0);
RgbColor yellow(140, 140, 0);
RgbColor green(0, 255, 0);

RgbColor blue(0, 0, 255);
RgbColor white(255);
RgbColor black(0);

#define DEF_COLOR white

// ##################################################################

void setPixelsInColor(RgbColor color)
{

  color = color.Dim(colorSaturation); // adjust intensity
  strip.SetPixelColor(0, color);
  strip.SetPixelColor(1, color);
  strip.SetPixelColor(2, color);
  strip.Show();

  stripESP.SetPixelColor(0, color);
  stripESP.Show();
}

void MyNeoPixelMs100Task()
{ // to animate pixel colors
  static int y = 0;
  static bool Hz5 = 0;

  y++;
  if (y >= 2)
  { // 0.2 sec
    y = 0;
    Hz5 = !Hz5; // blink at 5Hz
    if (rssi_max < -80)
    {
      setPixelsInColor(red);
    }
    else if (rssi_max < -60)
    {
      setPixelsInColor(orange);
    }
    else if (rssi_max < -40)
    {
      setPixelsInColor(yellow);
    }
    else if (rssi_max < -30)
    {
      setPixelsInColor(green);
    }
    else
    { // above -30
      if (Hz5)
      {
        setPixelsInColor(green);
      }
      else
      {
        setPixelsInColor(black); // to blink
      }
    }
  }
}

void MyNeoPixelSetup()
{

  Serial.println();
  Serial.print("NeoPixel Initializing...");
  Serial.flush();

  // this resets all the neopixels to an off state
  strip.Begin();
  strip.Show();

  stripESP.Begin();
  stripESP.Show();

  Serial.println(" Done");
}

void MyNeoPixelTask()
{
}
