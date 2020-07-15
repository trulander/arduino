#include "U8glib.h"
 
const byte lcdLED = 6;                   // LED Backlight
const byte lcdA0 = 7;                    // Data and command selections. L: command  H : data
const byte lcdRESET = 8;                 // Low reset
const byte lcdCS = 9;                    // SPI Chip Select (internally pulled up), active low
const byte lcdMOSI = 11;                 // SPI Data transmission
const byte lcdSCK = 13;                  // SPI Serial Clock
 
// SW SPI:
//U8GLIB_MINI12864 u8g(lcdSCK, lcdMOSI, lcdCS, lcdA0, lcdRESET);
// HW SPI:
U8GLIB_MINI12864 u8g(lcdSCK, lcdMOSI, lcdCS, lcdA0, lcdRESET);
 
void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(u8g_font_micro);
  u8g.drawStr( 0, 20, "Hello World!");
}
 
void setup() {
  u8g.begin();
}
 
void loop() {
  // picture loop
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
   
  // rebuild the picture after some delay
  delay(1000);
}