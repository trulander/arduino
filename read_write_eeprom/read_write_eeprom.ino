#include <EEPROM.h>
const int EEPROM_MIN_ADDR = 0;
const int EEPROM_MAX_ADDR = 1023;

boolean eeprom_is_addr_ok(int addr) {
  return ((addr >= EEPROM_MIN_ADDR) && (addr <= EEPROM_MAX_ADDR));
}

boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {
  int i;
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) {
    return false;
  }

  for (i = 0; i < numBytes; i++) {
    EEPROM.write(startAddr + i, array[i]);
  }

  return true;
}

boolean eeprom_write_string(int addr, const char* string) {
  int numBytes; // actual number of bytes to be written
  numBytes = strlen(string) + 1;
  return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}


boolean eeprom_read_string(int addr, char* buffer, int bufSize) {
  byte ch; // byte read from eeprom
  int bytesRead; // number of bytes read so far

  if (!eeprom_is_addr_ok(addr)) { // check start address
    return false;
  }

  if (bufSize == 0) { // how can we store bytes in an empty buffer ?
    return false;
  }

  if (bufSize == 1) {
    buffer[0] = 0;
    return true;
  }

  bytesRead = 0; // initialize byte counter
  ch = EEPROM.read(addr + bytesRead); // read next byte from eeprom
  buffer[bytesRead] = ch; // store it into the user buffer
  bytesRead++; // increment byte counter

  while ( (ch != 0x00) && (bytesRead < bufSize) && ((addr + bytesRead) <= EEPROM_MAX_ADDR) ) {
    ch = EEPROM.read(addr + bytesRead);
    buffer[bytesRead] = ch; // store it into the user buffer
    bytesRead++; // increment byte counter
  }

  if ((ch != 0x00) && (bytesRead >= 1)) {
    buffer[bytesRead - 1] = 0;
  }

  return true;
}

const int BUFSIZE = 15;
char buf[BUFSIZE];

String myString;
char myStringChar[BUFSIZE];

void setup()
{
Serial.begin(9600);

Serial.println("Saving string to address 0");
strcpy(buf, "ABCDEFGHIJKLMN");
eeprom_write_string(0, buf);

Serial.println("Saving variable string to address 15");
myString="12345678901234";
myString.toCharArray(myStringChar, BUFSIZE); //convert string to char array
strcpy(buf, myStringChar);
eeprom_write_string(15, buf);

Serial.print("Reading string from address 0: ");
eeprom_read_string(0, buf, BUFSIZE);
Serial.println(buf);

Serial.print("Reading string from address 15: ");
eeprom_read_string(15, buf, BUFSIZE);
Serial.println(buf);
}


void loop()
{

}
