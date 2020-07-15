#include "SPIFlash.h"
//#include <SPI.h>
#include <avr/wdt.h>
//////////////////////////////////////////
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0x1F44 for adesto(ex atmel) 4mbit flash
//                             0xEF30 for windbond 4mbit flash
//////////////////////////////////////////
SPIFlash flash(2, 0xEF30);
byte buf[1024];
void setup() {
  Serial.begin(115200);
  while (!Serial);
  if (flash.initialize())
    Serial.println("Init OK!");
  else
    Serial.println("Init FAIL!");
}

void loop() {
  char cmd;
  if (!Serial.available()) return;
  cmd = Serial.read();
  if (cmd == 't') {
    Serial.print("COM ok\n");
    return;
  }
  if (cmd == 'i')
  {
    Serial.print("DeviceID: ");
    Serial.print(flash.readDeviceId(), HEX);
    Serial.print('\n');
    return;
  }
  if (cmd == 'a')
  {
    flash.chipErase();
    while (flash.busy());
    Serial.print("OK");
    Serial.print('\n');
    return;
  }
  if (cmd == 'e')
  {
    long  sector = Serial.parseInt();
    Serial.read(); // разделитель
    flash.blockErase4K(sector);
    Serial.print("OK");
    Serial.print(sector);
    Serial.print('\n');
    return;
  }
  if (cmd == '2')
  {
    flash.blockErase4K(0);
    delay(1000);
    for (int i=0;i<128;i++)
      buf[i]=i;
    flash.writeBytes(0, buf, 128);
    delay(1000);
    for (int i=0;i<128;i++)
      buf[i]=0;
    flash.readBytes(0, buf, 128);
    delay(1000);
    for (int i=0;i<128;i++)
    {
      Serial.print(buf[i], HEX);
      Serial.print(" ");
      if (i>0 && (i+1)%16==0)Serial.print('\n');
    }
    Serial.print("OK");
    Serial.print(0);
    Serial.print('\n');
    return;
  }
  if (cmd == 'w')
  {
    long addr = Serial.parseInt();
    Serial.read(); // разделитель
    for (int bufsz = 0; bufsz < 128; bufsz++)
    {
      while (Serial.available() == 0);
      buf[bufsz] = Serial.read();
    }
    flash.writeBytes(addr, buf, 128);
    Serial.print("OK");
    Serial.print(addr);
    Serial.print('\n');
    return;
  }
  if (cmd == 'r') {
    long addr = Serial.parseInt();
    Serial.read(); // разделитель
    for (int i = 0; i < 4; i++)
    {
      flash.readBytes(addr + (i * 1024) + 0, buf, 1024);
      for (int j = 0; j < 1024; j++)
        Serial.write(buf[j]);
    }
    return;
  }
}

