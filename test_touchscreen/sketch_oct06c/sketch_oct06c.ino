#include <PCF8814.h> 

PCF8814 Lcd(13,11,10,6); // LCD sets SPI SCLK: 7 pin, SDA: 8 pin, CS: 9 pin. RESET: 6 pin


void setup()
{
 // analogReference(INTERNAL);
  Lcd.Init();
  Lcd.Mirror(1,1);
  Lcd.Contrast(15);
}

int readX() // returns the value of the touch screen's X-axis
{
  int xr=0;
  pinMode(9, INPUT);   // A0
  pinMode(5, OUTPUT);    // A1
  pinMode(4, INPUT);   // A2
  pinMode(8, OUTPUT);   // A3
  digitalWrite(5, LOW); // set A1 to GND
  digitalWrite(8, HIGH);  // set A3 as 5V
//  delay(5); // short delay is required to give the analog pins time to adjust to their new roles
  xr=analogRead(6); 
  return xr;
}

int readY() // returns the value of the touch screen's Y-axis
{
  int yr=0;
  pinMode(9, OUTPUT);   // A0
  pinMode(5, INPUT);    // A1
  pinMode(4, OUTPUT);   // A2
  pinMode(8, INPUT);   // A3
  digitalWrite(9, LOW); // set A0 to GND
  digitalWrite(4, HIGH);  // set A2 as 5V
//  delay(5); // short delay is required to give the analog pins time to adjust to their new roles
  yr=analogRead(7);
  return yr;
}

void displayA()
{
  Lcd.Clear();
  Lcd.GotoXY(0,3);
  Lcd.Print("Азаза");
  Lcd.GotoXY(0,0);
  delay (1000);
  Lcd.Clear();
}
 
void displayB()
{
  Lcd.Clear();
  Lcd.GotoXY(0,3);
  Lcd.Print("Лалка затралел");
  Lcd.GotoXY(0,0);
  delay (1000);
  Lcd.Clear();
}

void loop()
{
  char text[15];
  char buf[15];
  String tex;
  int x,y = 0;
  x=readX();
  y=readY();

  /*

   if (y>104 && x>104 && x<500 && y <500)
  {
    displayA();
  }
  else
    if (y>500 && x>500 && y<1023 && x<1023)
    {
      displayB();
    }
    else
  */
      if (x<1024 && y<1024)
      {
        tex= String(x);
        tex.toCharArray(buf, 6);
        sprintf(text,"%s %s","X: ",buf);
        Lcd.GotoXY(0,0);
        Lcd.Print(text);
      
        tex= String(y);
        tex.toCharArray(buf, 6);
        sprintf(text,"%s %s","Y: ",buf);
        Lcd.GotoXY(0,1);
        Lcd.Print(text);
        Lcd.Pixel(x/9.8333,y/13.5588,PIXEL_ON);
      }
    //delay (10);
}
