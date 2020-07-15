#include <PCF8814.h>
#include <EEPROM.h>
 
// Variables you might want to play with
byte useThreshold = 2;                  // 0 = Off, 1 = Rising, 2 = Falling
byte theThreshold = 128;                // 0-255, Multiplied by voltageConst
unsigned int timePeriod = 50;          // 0-65535, us or ms per measurement (max 0.065s or 65.535s)
byte voltageRange = 1;                  // 1 = 0-3.3V, 2 = 0-1.65V, 3 = 0-0.825V
byte ledBacklight = 100;
 
boolean autoHScale = true;             // Automatic horizontal (time) scaling
boolean linesNotDots = true;            // Draw lines between data points
 
// Variables that can probably be left alone
const byte vTextShift = 0;              // Vertical text shift (to vertically align info)
const byte numOfSamples = 50;          // Leave at 100 for 96x68 pixel display
unsigned int HQadcReadings[numOfSamples];
byte adcReadings[numOfSamples];
byte thresLocation = 0;                 // Threshold bar location
float voltageConst = 0.052381;          // Scaling factor for converting 0-63 to V
float avgV = 0.0;    
float maxV = 0.0;
float minV = 0.0;
float ptopV = 0.0;
float theFreq = 0;
 
const byte theAnalogPin = 0;             // Data read pin
 
const byte lcdLED = 3;                   // LED Backlight

//PCF8814 Lcd(13,11,10,6); // LCD sets SPI SCLK: 7 pin, SDA: 8 pin, CS: 9 pin. RESET: 6 pin 
PCF8814 Lcd(4,5,6,8);
// SW SPI:
//U8GLIB_MINI12864 u8g(lcdSCK, lcdMOSI, lcdCS, lcdA0, lcdRESET);
// HW SPI:
//U8GLIB_PCF8812 u8g(lcdSCK, lcdMOSI, lcdCS, lcdA0, lcdRESET);
 

// High speed ADC code
// From: http://forum.arduino.cc/index.php?PHPSESSID=e21f9a71b887039092c91a516f9b0f36&topic=6549.15
#define FASTADC 1
// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
 

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
 
void collectData(void) {
  unsigned int tempThres = 0;
  unsigned int i = 0;
 
  if (autoHScale == true) {
    // With automatic horizontal (time) scaling enabled,
    // scale quickly if the threshold location is far, then slow down
    if (thresLocation > 5*numOfSamples/8) {
      timePeriod = timePeriod + 10;
    } else if (thresLocation < 3*numOfSamples/8) {
      timePeriod = timePeriod - 10;
    } else if (thresLocation > numOfSamples/2) {
      timePeriod = timePeriod + 2;
    } else if (thresLocation < numOfSamples/2) {
      timePeriod = timePeriod - 2;
    }
  }
  // Enforce minimum time periods
  if (timePeriod < 35) {
    timePeriod = 35;
  }
   
  // Adjust voltage contstant to fit the voltage range
  if (voltageRange == 1) {
    voltageConst = 0.0523810; // 0-3.30V
  } else if (voltageRange == 2) {
    voltageConst = 0.0261905; // 0-1.65V
  } else if (voltageRange == 3) {
    voltageConst = 0.0130952; //0-0.825V
  }
   
  // If using threshold, wait until it has been reached
  if (voltageRange == 1) tempThres = theThreshold << 2;
  else if (voltageRange == 2) tempThres = theThreshold << 1;
  else if (voltageRange == 3) tempThres = theThreshold;
  if (useThreshold == 1) {
     i = 0; while ((analogRead(theAnalogPin)>tempThres) && (i<32768)) i++;
     i = 0; while ((analogRead(theAnalogPin)<tempThres) && (i<32768)) i++;
  }
  else if (useThreshold == 2) {
     i = 0; while ((analogRead(theAnalogPin)<tempThres) && (i<32768)) i++;
     i = 0; while ((analogRead(theAnalogPin)>tempThres) && (i<32768)) i++;
  }
 
  // Collect ADC readings
  for (i=0; i<numOfSamples; i++) {
    // Takes 35 us with high speed ADC setting
    HQadcReadings[i] = analogRead(theAnalogPin);
    if (timePeriod > 35)
      delayMicroseconds(timePeriod-35);
  }
  for (i=0; i<numOfSamples; i++) {
    // Scale the readings to 0-63 and clip to 63 if they are out of range.
    if (voltageRange == 1) {
      if (HQadcReadings[i]>>4 < 0b111111) adcReadings[i] = HQadcReadings[i]>>4 & 0b111111;
      else adcReadings[i] = 0b111111;
    } else if (voltageRange == 2) {
      if (HQadcReadings[i]>>3 < 0b111111) adcReadings[i] = HQadcReadings[i]>>3 & 0b111111;
      else adcReadings[i] = 0b111111;
    } else if (voltageRange == 3) {
      if (HQadcReadings[i]>>2 < 0b111111) adcReadings[i] = HQadcReadings[i]>>2 & 0b111111;
      else adcReadings[i] = 0b111111;
    }
    // Invert for display
    adcReadings[i] = 65-adcReadings[i];
  }
   
  // Calculate and display frequency of signal using zero crossing
  if (useThreshold != 0) {
     if (useThreshold == 1) {
        thresLocation = 1;
        while ((adcReadings[thresLocation]<(68-(theThreshold>>2))) && (thresLocation<numOfSamples-1)) (thresLocation++);
        thresLocation++;
        while ((adcReadings[thresLocation]>(68-(theThreshold>>2))) && (thresLocation<numOfSamples-1)) (thresLocation++);
     }
     else if (useThreshold == 2) {
        thresLocation = 1;
        while ((adcReadings[thresLocation]>(68-(theThreshold>>2))) && (thresLocation<numOfSamples-1)) (thresLocation++);
        thresLocation++;
        while ((adcReadings[thresLocation]<(68-(theThreshold>>2))) && (thresLocation<numOfSamples-1)) (thresLocation++);
     }
 
     theFreq = (float) 1000/(thresLocation * timePeriod) * 1000;
  }
   
  // Average Voltage
  avgV = 0;
  for (i=0; i<numOfSamples; i++)
     avgV = avgV + adcReadings[i];
  avgV = (67-(avgV / numOfSamples)) * voltageConst;
 
  // Maximum Voltage
  maxV = 67;
  for (i=0; i<numOfSamples; i++)
     if (adcReadings[i]<maxV) maxV = adcReadings[i];
  maxV = (67-maxV) * voltageConst;
 
  // Minimum Voltage
  minV = 0;
  for (i=0; i<numOfSamples; i++)
     if (adcReadings[i]>minV) minV = adcReadings[i];
  minV = (67-minV) * voltageConst;
 
  // Peak-to-Peak Voltage
  ptopV = maxV - minV;
}
/* 
void handleSerial(void) {
  char inByte;
  char dataByte;
  boolean exitLoop = false;
  do {
    // Clear out buffer
    do {
      inByte = Serial.read();
    } while (Serial.available() > 0);
   
    Serial.print("\nArduino LCD Oscilloscope\n");
    Serial.print(" 1 - Change threshold usage (currently: ");
      if (useThreshold == 0) Serial.print("Off)\n");
      else if (useThreshold == 1) Serial.print("Rise)\n");
      else if (useThreshold == 2) Serial.print("Fall)\n");
    Serial.print(" 2 - Change threshold value (currently: ");
      Serial.print(theThreshold, DEC); Serial.print(")\n");
    Serial.print(" 3 - Change sampling period (currently: ");
      Serial.print(timePeriod, DEC); Serial.print(")\n");
    Serial.print(" 4 - Change voltage range (currently: ");
      if (voltageRange == 1) Serial.print("0-3.3V)\n");
      else if (voltageRange == 2) Serial.print("0-1.65V)\n");
      else if (voltageRange == 3) Serial.print("0-0.825V)\n");
    Serial.print(" 5 - Toggle auto horizontal (time) scaling (currently: ");
      if (autoHScale == true) Serial.print("On)\n");
      else if (autoHScale == false) Serial.print("Off)\n");
    Serial.print(" 6 - Toggle line/dot display (currently: ");
      if (linesNotDots == true) Serial.print("Lines)\n");
      else if (linesNotDots == false) Serial.print("Dots)\n");
    Serial.print(" 8 - Exit\n");
     
    // Wait for input/response, refresh display while in menu
  
  
  
    /////////////////////////////////
    do {
      collectData();
      // Picture Display Loop
      Lcd.Clear();  
      do { draw(); } while( u8g.nextPage() );
    } while (Serial.available() == 0);
    /////////////////////////////////////
   
   
   
    inByte = Serial.read();
     
    if (inByte == '1') {
      Serial.print("Change threshold usage\n");
      Serial.print(" 0 - Off\n");
      Serial.print(" 1 - Rise\n");
      Serial.print(" 2 - Fall\n");
      do { } while (Serial.available() == 0);
      dataByte = Serial.read();
      if (dataByte == '0') useThreshold = 0;
      else if (dataByte == '1') useThreshold = 1;
      else if (dataByte == '2') useThreshold = 2;
    } else if (inByte == '2') {
      Serial.print("Change threshold value (thresholds for 0-3.3V,0-1.65V,0-0.825V ranges)\n");
      Serial.print(" 0 - 32 (0.41V, 0.21V, 0.10V)\n");
      Serial.print(" 1 - 80 (1.04V, 0.52V, 0.26V)\n");
      Serial.print(" 2 - 128 (1.66V, 0.83V, 0.41V)\n");
      Serial.print(" 3 - 176 (2.28V, 1.14V, 0.57V)\n");
      Serial.print(" 4 - 224 (2.90V, 1.45V, 0.72V)\n");
      do { } while (Serial.available() == 0);
      dataByte = Serial.read();
      if (dataByte == '0') theThreshold = 32;
      else if (dataByte == '1') theThreshold = 80;
      else if (dataByte == '2') theThreshold = 128;
      else if (dataByte == '3') theThreshold = 176;
      else if (dataByte == '4') theThreshold = 224;
    } else if (inByte == '3') {
      Serial.print("Change sampling frequency\n");
      Serial.print(" 0 - 28 kHz (35 us/sample)\n");
      Serial.print(" 1 - 20 kHz (50 us/sample)\n");
      Serial.print(" 2 - 10 kHz (100 us/sample)\n");
      Serial.print(" 3 - 5 kHz (200 us/sample)\n");
      Serial.print(" 4 - 2.5 kHz (400 us/sample)\n");
      do { } while (Serial.available() == 0);
      dataByte = Serial.read();
      if (dataByte == '0') timePeriod = 35;
      else if (dataByte == '1') timePeriod = 50;
      else if (dataByte == '2') timePeriod = 100;
      else if (dataByte == '3') timePeriod = 200;
      else if (dataByte == '4') timePeriod = 400;
    } else if (inByte == '4') {
      Serial.print("Change voltage range\n");
      Serial.print(" 1 - 0-3.3V\n");
      Serial.print(" 2 - 0-1.65V\n");
      Serial.print(" 3 - 0-0.825V\n");
      do { } while (Serial.available() == 0);
      dataByte = Serial.read();
      if (dataByte == '1') voltageRange = 1;
      else if (dataByte == '2') voltageRange = 2;
      else if (dataByte == '3') voltageRange = 3;
    } else if (inByte == '5') {
      Serial.print("Toggle auto horizontal (time) scaling\n");
      Serial.print(" 0 - Off\n");
      Serial.print(" 1 - On\n");
      do { } while (Serial.available() == 0);
      dataByte = Serial.read();
      if (dataByte == '0') autoHScale = false;
      else if (dataByte == '1') autoHScale = true;
    } else if (inByte == '6') {
      Serial.print("Toggle line/dot display\n");
      Serial.print(" 0 - Lines\n");
      Serial.print(" 1 - Dots\n");
      do { } while (Serial.available() == 0);
      dataByte = Serial.read();
      if (dataByte == '0') linesNotDots = true;
      else if (dataByte == '1') linesNotDots = false;
    } else if (inByte == '8') {
      Serial.print("Bye!\n");
      exitLoop = true;
    }
  } while (exitLoop == false);
}
*/ 
void draw(void) {
  int i;
  char buffer[16];
   
 // u8g.setFont(u8g_font_micro);
   
  // Draw static text
  Lcd.GotoXY_pix(0,5+vTextShift);
  Lcd.Print("Av");
  
  Lcd.GotoXY_pix(0,11+vTextShift);
  Lcd.Print("Mx");
  
  Lcd.GotoXY_pix(0,17+vTextShift);
  Lcd.Print("Mn");
  
  Lcd.GotoXY_pix(0,24+vTextShift);
  Lcd.Print("PP");
  
  //Lcd.GotoXY_pix(0,29+vTextShift);
  //Lcd.Print("Th");
  
 // Lcd.GotoXY_pix(91,34+vTextShift);
 // Lcd.Print("V");
  
  Lcd.GotoXY_pix(0,34+vTextShift);
  Lcd.Print("Tm");
 
  Lcd.GotoXY_pix(0,47+vTextShift);
  Lcd.Print("ms");
 
  Lcd.GotoXY_pix(15, 54+vTextShift);
  Lcd.Print("Hz");
  
  //Lcd.GotoXY_pix(0,59+vTextShift);
  //Lcd.Print("R");
  
   
  // Draw dynamic text
  if (autoHScale == true) 
  {
     // Lcd.GotoXY_pix(65, 5);
     // Lcd.Print("A");
    
  }
      dtostrf(avgV, 3, 2, buffer);
      Lcd.GotoXY_pix(12, 5+vTextShift);
      Lcd.Print(buffer);
  
      dtostrf(maxV, 3, 2, buffer);
      Lcd.GotoXY_pix(12, 11+vTextShift);
      Lcd.Print(buffer);
  
      dtostrf(minV, 3, 2, buffer);
      Lcd.GotoXY_pix(12, 17+vTextShift);
      Lcd.Print(buffer);
  
      dtostrf(ptopV, 3, 2, buffer);
      Lcd.GotoXY_pix(12, 24+vTextShift);
      Lcd.Print(buffer);
  
      dtostrf(theFreq, 5, 0, buffer);
      Lcd.GotoXY_pix(0, 63+vTextShift);
      Lcd.Print(buffer);
  
  if (useThreshold == 0) {
      //Lcd.GotoXY_pix(12, 29+vTextShift);
      //Lcd.Print("Off");
  } else if (useThreshold == 1) {
      //Lcd.GotoXY_pix(12, 29+vTextShift);
      //Lcd.Print("Rise");
      dtostrf((float) (theThreshold>>2) * voltageConst, 3, 2, buffer);
  } else if (useThreshold == 2) {
         // Lcd.GotoXY_pix(12, 29+vTextShift);
          //Lcd.Print("Fall");
          dtostrf((float) (theThreshold>>2) * voltageConst, 3, 2, buffer);
  }
   Lcd.GotoXY_pix(12, 35+vTextShift);
   Lcd.Print(buffer);  

  // Correctly format the text so that there are always 4 characters
  if (timePeriod < 400) {
    dtostrf((float) timePeriod/1000 * 25, 3, 2, buffer);
  } else if (timePeriod < 4000) {
    dtostrf((float) timePeriod/1000 * 25, 3, 1, buffer);
  } else if (timePeriod < 40000) {
    dtostrf((float) timePeriod/1000 * 25, 3, 0, buffer);
  } else { // Out of range
    dtostrf((float) 0.00, 3, 2, buffer);
  }
  Lcd.GotoXY_pix(12, 41+vTextShift);
   Lcd.Print(buffer);
  //u8g.drawStr(12, 41+vTextShift, buffer);
  /*
  if (voltageRange == 1) {
    Lcd.GotoXY_pix(4, 59+vTextShift);
   Lcd.Print("0-3.30");
    //u8g.drawStr(4, 59+vTextShift, "0-3.30");
  } else if (voltageRange == 2) {
    Lcd.GotoXY_pix(4, 59+vTextShift);
   Lcd.Print("0-1.65");
    //u8g.drawStr(4, 59+vTextShift, "0-1.65");
  } else if (voltageRange == 3) {
    Lcd.GotoXY_pix(4, 59+vTextShift);
   Lcd.Print("0-0.83");
    //u8g.drawStr(4, 59+vTextShift, "0-0.83");
  }
   */
   
  // Display graph lines
  Lcd.Line ((96-numOfSamples),0,(96-numOfSamples),67, PIXEL_ON);
  //u8g.drawLine((128-numOfSamples),0,(128-numOfSamples),63);
  if (useThreshold != 0)
     for (i=48; i<95; i+=3)
        Lcd.Pixel(i,68-(theThreshold>>2),PIXEL_ON);
//        u8g.drawPixel(i,64-(theThreshold>>2));
  for (i=0; i<67; i+=5) {
    Lcd.Pixel(53,i,PIXEL_ON);
     //u8g.drawPixel(53,i);
     Lcd.Pixel(78,i,PIXEL_ON);
     //u8g.drawPixel(78,i);
     Lcd.Pixel(103,i,PIXEL_ON);
    // u8g.drawPixel(103,i);
     Lcd.Pixel(127,i,PIXEL_ON);
    // u8g.drawPixel(127,i);
  }
  // Threshold bar
  for (i=0; i<68; i+=3)
     Lcd.Pixel(thresLocation+(96-numOfSamples),i,PIXEL_ON);
     //u8g.drawPixel(thresLocation+(128-numOfSamples),i);
  // Draw ADC readings
  if (linesNotDots == true) {
    for (i=1; i<numOfSamples; i++) // Draw using lines
      Lcd.Line (i+(96-numOfSamples)-1,adcReadings[i-1],i+(96-numOfSamples),adcReadings[i], PIXEL_ON);
//      u8g.drawLine(i+(128-numOfSamples)-1,adcReadings[i-1],i+(128-numOfSamples),adcReadings[i]);
  } else {
    for (i=2; i<numOfSamples; i++) // Draw using points
    Lcd.Pixel(i+(96-numOfSamples),adcReadings[i],PIXEL_ON);
//      u8g.drawPixel(i+(128-numOfSamples),adcReadings[i]);
  }
}
 
void setup() {
  Serial.begin(9600);

  Lcd.Init();

  //Lcd.Contrast(20);
  delay(100);
  Lcd.Mirror(1,1);
  //u8g.begin();
  
  Lcd.GotoXY(2,4);
  Lcd.Print("Osciloscope");
  delay(2000);
  Lcd.Clear(); 
  // Turn on LED backlight
  Lcd.GotoXY(5,3);
  Lcd.Print("Turn on");
  Lcd.GotoXY(2,4);
  Lcd.Print("LED backlight");
  delay(2000); 
  analogWrite(lcdLED, ledBacklight);

  #if FASTADC
    // set prescale to 16
    sbi(ADCSRA,ADPS2) ;
    cbi(ADCSRA,ADPS1) ;
    cbi(ADCSRA,ADPS0) ;
  #endif
  delay(100);
} 
 
void loop() {
  collectData();
    Serial.println(freeRam());
  // Picture Display Loop
  Lcd.Clear();
  
  //u8g.firstPage();  
  //do { 
  draw(); //} ;//while( u8g.nextPage() );
 
  // If user sends any serial data, show menu
  if (Serial.available() > 0) {
    //handleSerial();
  }
 
  // rebuild the picture after some delay
  delay(100);
}
