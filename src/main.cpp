/*
 * Copyright (c) 2022 Eddi De Pieri
 * Most code borrowed by Pico-DMX by Jostein Løwer 
 * Modified for performance by bjaan
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Description: 
 * Roland Juno G LCD Emulator
 */

// Define in setup to disable all #warnings in library (can be put in User_Setup_Select.h)
#define DISABLE_ALL_LIBRARY_WARNINGS

#include <Arduino.h>

#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#ifdef DRAW_SPLASH
#include <LCDJunoG_splash.h>
#endif
#include <LCDJunoG_extra.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#include "LCDJunoG.h"
LCDJunoG lcdJunoG_cs1;
LCDJunoG lcdJunoG_cs2;

#define ZOOM_X 2
#define ZOOM_Y 3

uint tft_xoffset = 0;
uint tft_yoffset = 0;

volatile uint16_t buffer_cs1[12*123];
volatile uint16_t buffer_cs2[12*123];
volatile uint8_t back_buffer_cs1[12*123];
volatile uint8_t back_buffer_cs2[12*123];

void fillscreenInterlaced(uint32_t bgcolor) {
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);
  for (uint y = 0; y < tft.height(); y+=ZOOM_Y) {
    for (uint x = 0; x < tft.width(); x+=ZOOM_X) {
      tft.drawPixel(x, y, bgcolor);
    }
  }
  tft.endWrite();
}

void setup()
{
  analogReadResolution(12);
  uint32_t analog_read = analogRead(JUNO_BRGT);
  //Serial.begin(115200);
  //delay(2000);
  //Serial.setTimeout(50);
  //Serial.println("juno g lcd emulator");
  tft.init();
  //tft.setRotation(2);
  tft.setRotation(1);
  tft_xoffset = (tft.width() - 240 * ZOOM_X) / 2 - ((tft.width() - 240 * ZOOM_X) / 2 % ZOOM_X);
  tft_yoffset = (tft.height() - 96 * ZOOM_Y) / 2 - ((tft.height() - 96 * ZOOM_Y) / 2 % ZOOM_Y);
#ifdef DRAW_SPLASH
  drawBitmapZoom(0, 0, (const uint8_t *)junog, 240, 90, TFT_BLACK);
  delay(500);
#endif

#define DRAW_INFO
#ifdef DRAW_INFO
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(0, tft.height()/2 -10  , 2);
  tft.setTextSize(2);
  
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Roland JUNO-G LCD Emulator v0.4", tft.width() /2, tft.height() / 2 - 20 );
  //tft.drawString("CPU_FREQ:" + String(rp2040.f_cpu()), tft.width() /2, tft.height() / 2 + 10 );
  delay(500);
  fillscreenInterlaced(TFT_WHITE);
  
  //set back-buffers to full black
  memset((void *) back_buffer_cs1, 255, sizeof back_buffer_cs1);
  memset((void *) back_buffer_cs2, 255, sizeof back_buffer_cs2);
#endif

#ifdef DRAW_PINOUT  
  drawPinout(1000);
#endif
   
  tft.setCursor(0, 0, 2);
  tft.setTextSize(1);
  
  // Setup our DMX Input to read on GPIO 0, from channel 1 to 3
  lcdJunoG_cs1.begin(2, pio0, 1);
  lcdJunoG_cs2.begin(2, pio1, 2);
  lcdJunoG_cs1.read_async(buffer_cs1);
  lcdJunoG_cs2.read_async(buffer_cs2);

  // Setup the onboard LED so that we can blink when we receives packets
  pinMode(LED_BUILTIN, OUTPUT);
}

volatile int start_index_cs1 = 0;
volatile int start_index_cs2 = 0;
volatile uint8_t page_cs1 = 0;
volatile uint8_t page_cs2 = 0;
volatile uint8_t xx_cs1 = 0;
volatile uint8_t xx_cs2 = 0;

bool led_on = false;
long latest_packet_timestamp_cs1 = 0;
long latest_packet_timestamp_cs2 = 0;

void loop()
{  
  if(latest_packet_timestamp_cs1 == lcdJunoG_cs1.latest_packet_timestamp() && latest_packet_timestamp_cs2 == lcdJunoG_cs2.latest_packet_timestamp()) {
    return; // no packet received
  } 
  latest_packet_timestamp_cs1 = lcdJunoG_cs1.latest_packet_timestamp();
  latest_packet_timestamp_cs2 = lcdJunoG_cs2.latest_packet_timestamp();
  tft.startWrite();
  for (uint i = 0; i < 123*12; i++)  
  {
    { //CS 1
      uint8_t val = buffer_cs1[i] & 0xff;
      uint8_t rs = (buffer_cs1[i] >> 9) & 1;
      if (rs == 0) {
#ifdef SHOWCMD
          showcmd( cs, val );
#endif
        if ((val >> 4) == 0xb) {
          page_cs1 = val & 0xf;
          xx_cs1 = 0;
        } 
      } else if (rs == 1 ) {
        if (xx_cs1 < 120) {  // avoid overlap of the right area
          if (back_buffer_cs1[i] != val) {
            int16_t x = tft_xoffset + xx_cs1 * ZOOM_X;
            int16_t y = tft_yoffset + (page_cs1 * 8) * ZOOM_Y;
            //for (int b = 0; b < 8; b++ ) {
              //if (((val >> b) & 0x1) == 1) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              //y+= ZOOM_Y;
              //unrolled:
              if ((val & 0x1) == 0x1) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x2) == 0x2) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x4) == 0x4) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x8) == 0x8) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x10) == 0x10) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x20) == 0x20) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x40) == 0x40) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x80) == 0x80) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
            //}
          }
          xx_cs1++;
        }
      }
      back_buffer_cs1[i] = val;
    }
    { //CS 2
      uint8_t val = buffer_cs2[i] & 0xff;
      uint8_t rs = (buffer_cs2[i] >> 9) & 1;
      if (rs == 0) {
#ifdef SHOWCMD
        showcmd( cs, val );
#endif
        if ((val >> 4) == 0xb) {
          page_cs2 = val & 0xf;
          xx_cs2 = 0;
        } 
      } else if (rs == 1) {
        if (xx_cs2 < 120) {  // avoid overlap of the right area
          if (back_buffer_cs2[i] != val) {
            int16_t x = tft_xoffset + xx_cs2 * ZOOM_X + 120 * ZOOM_X;
            int16_t y = tft_yoffset + (page_cs2 * 8) * ZOOM_Y;
            //for (int b = 0; b < 8; b++ ) {
              //if (((val >> b) & 0x1) == 1) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              //y+= ZOOM_Y;
              //unrolled:
              if ((val & 0x1) == 0x1) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x2) == 0x2) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x4) == 0x4) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x8) == 0x8) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x10) == 0x10) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x20) == 0x20) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x40) == 0x40) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
              y+= ZOOM_Y;
              if ((val & 0x80) == 0x80) tft.drawPixel(x, y, TFT_BLACK); else tft.drawPixel(x, y, TFT_WHITE);
            //}
          }
          xx_cs2++;
        }
      }
      back_buffer_cs2[i] = val;
    }
  }
  tft.endWrite();
  // Blink the LED to indicate that a packet was received 
  if (!led_on) digitalWrite(LED_BUILTIN, HIGH); else digitalWrite(LED_BUILTIN, LOW);
  led_on != led_on;
}
