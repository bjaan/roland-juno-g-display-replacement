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
volatile uint8_t back_buffer[240][96/8]; /* max X address register for CS1 and CS2 times max Y register */
volatile uint16_t pixel_x[240]; /* X location (pre-calculated) of actual pixel on new screen */
volatile uint16_t pixel_y[96]; /* Y location (pre-calculated) of actual pixel on new screen */

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

void drawPixels(uint8_t val, uint8_t xx, uint8_t yy) {
  int16_t x = pixel_x[yy];
  int8_t y = xx * 8;
  //for (int b = 0; b < 8; b++ ) {
    //if (((val >> b) & 0x1) == 1) tft.drawPixel(x, pixel_y[y+b], TFT_BLACK); else tft.drawPixel(x, pixel_y[y+b], TFT_WHITE);
    //unrolled:
    if ((val & 0x1) == 0x1)   tft.drawPixel(x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(x, pixel_y[y++]  , TFT_WHITE);
    if ((val & 0x2) == 0x2)   tft.drawPixel(x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(x, pixel_y[y++], TFT_WHITE);
    if ((val & 0x4) == 0x4)   tft.drawPixel(x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(x, pixel_y[y++], TFT_WHITE);
    if ((val & 0x8) == 0x8)   tft.drawPixel(x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(x, pixel_y[y++], TFT_WHITE);
    if ((val & 0x10) == 0x10) tft.drawPixel(x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(x, pixel_y[y++], TFT_WHITE);
    if ((val & 0x20) == 0x20) tft.drawPixel(x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(x, pixel_y[y++], TFT_WHITE);
    if ((val & 0x40) == 0x40) tft.drawPixel(x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(x, pixel_y[y++], TFT_WHITE);
    if ((val & 0x80) == 0x80) tft.drawPixel(x, pixel_y[y  ], TFT_BLACK); else tft.drawPixel(x, pixel_y[y  ], TFT_WHITE);
  //}
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
  tft.drawString("Roland JUNO-G LCD Emulator v0.5", tft.width() /2, tft.height() / 2 - 20 );
  //tft.drawString("CPU_FREQ:" + String(rp2040.f_cpu()), tft.width() /2, tft.height() / 2 + 10 );
  {  //precalculate new display pixel indices
    for (uint8_t x = 0; x < 240; x++) {
      pixel_x[x] = tft_xoffset + x * ZOOM_X;
    }
    for (uint8_t y = 0; y < 96; y++) {
      pixel_y[y] = tft_yoffset + y * ZOOM_Y;
    }
  }
  { //initialize back buffer
    for (uint8_t x = 0; x < 239; x++) {
      for (uint8_t y = 0; y < 96/8; y++) {
        back_buffer[x][y] = 255;
      }
    }
  }
  fillscreenInterlaced(TFT_WHITE);
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

volatile uint8_t x_cs1 = 0; //X address register for CS1: 4 bit: values 0 up and to including 15: actually max. 12 as 12*8 = 96
volatile uint8_t x_cs2 = 0; //X address register for CS2: 4 bit: values 0 up and to including 15: actually max. 12 as 12*8 = 96
volatile uint8_t y_cs1 = 0; //Y address counter for CS1: 7 bit: values 0 up and to including 127: actually max. 120
volatile uint8_t y_cs2 = 120; //Y address counter for CS2: 7 bit: values 0 up and to including 127: actually max. 120. Here it is set, and later reset to 120, to avoid having to add an offset of 120 all the time

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
      /* 
        buffer_cs1[i] 
          bits: 
          ??????s? vvvvvvvv
          s = R/S (INSTRUCTION/DATA REGISTER SELECTION) pin GP6         
          v = value bits (8 bits: values 0 up to and including 255)
      */
      uint8_t val = buffer_cs1[i] & 0xff /* 00000000 11111111*/;
      uint8_t rs = (buffer_cs1[i] /*000000s0 00000000*/ >> 9) /*00000000 0000000s*/ & 1; 
      if (rs == 0) { //INSTRUCTION REGISTER
#ifdef SHOWCMD
          showcmd( cs, val );
#endif
        if ((val & 0xf0 /*11110000*/) == 0xb0 /*1011000*/) { //Sets the X address at the X address register
          x_cs1 = val & 0x0f /*00001111*/;
          y_cs1 = 0;
        }
      } else if (rs == 1) { //DATA REGISTER
        //writes data on JUNO_D0 to JUNO_D7 pins into display data RAM.
        if (y_cs1 < 120) {  // avoid overflow
          if (back_buffer[y_cs1][x_cs1] != val) {
            back_buffer[y_cs1][x_cs1] = val;
            drawPixels(val, x_cs1, y_cs1);
          }
          y_cs1++; //After the writing instruction, Y address is increased by 1 automatically.
        }
      }    
    }
    { //CS 2
      /* 
        buffer_cs2[i] 
          bits: 
          ??????s? vvvvvvvv
          s = R/S (INSTRUCTION/DATA REGISTER SELECTION) pin GP6         
          v = value bits (8 bits: values 0 up to and including 255)
      */
      uint8_t val = buffer_cs2[i] & 0xff;
      uint8_t rs = (buffer_cs2[i] >> 9) & 1;
      if (rs == 0) { //INSTRUCTION REGISTER
#ifdef SHOWCMD
        showcmd( cs, val );
#endif
        if ((val & 0xf0 /*11110000*/) == 0xb0 /*1011000*/) { //Sets the X address at the X address register
          x_cs2 = val & 0x0f /*00001111*/;
          y_cs2 = 120; // 120 is left most pixel of the right hand part
        }
      } else if (rs == 1) { //DATA REGISTER
        //writes data on JUNO_D0 to JUNO_D7 pins into display data RAM.
        if (y_cs2 >= 120 && y_cs2 < 240) {  // avoid overflow  
          if (back_buffer[y_cs2][x_cs2] != val) {
            back_buffer[y_cs2][x_cs2] = val;
            drawPixels(val, x_cs2, y_cs2);
          }
          y_cs2++; //After the writing instruction, Y address is increased by 1 automatically.
        }
      }
    }
  }
  tft.endWrite();
  // Blink the LED to indicate that a packet was received 
  if (!led_on) digitalWrite(LED_BUILTIN, HIGH); else digitalWrite(LED_BUILTIN, LOW);
  led_on != led_on;
}
