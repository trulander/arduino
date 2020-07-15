void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
}
 
int i = 0;
 
void loop() {
  digitalWrite(2, bitRead(i, 0));
  digitalWrite(3, bitRead(i, 1));
  digitalWrite(4, bitRead(i, 2));
  digitalWrite(5, bitRead(i, 3));
  digitalWrite(6, bitRead(i, 4));
  digitalWrite(7, bitRead(i, 5));
  digitalWrite(8, bitRead(i, 6));
  digitalWrite(9, bitRead(i, 7));
  digitalWrite(10, bitRead(i, 8));
  digitalWrite(11, bitRead(i, 9));
  digitalWrite(12, bitRead(i, 10));
  digitalWrite(13, bitRead(i, 11));
   
  i++;
  if (i >= 4096) {
    i = 0;
  }
}