#ifndef MY_HEADER_H
#define MY_HEADER_H

// Exports

extern int rssi_max;

void MyNeoPixelSetup();
void MyNeoPixelTask();
void MyNeoPixelMs100Task();

void MyAudioPlayerSetup();
void MyAudioPlayerTask();
void MyAudioPlayeFile(const char *path);

void MyRFIDSetup();
void MyRFIDTask();
#endif
