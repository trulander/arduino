// подключаем две библиотеки для работы с LCD и математических вычислений

#include <math.h>
#define boiler 4

int backLight = 13;

void setup(void) {

  Serial.begin(9600); 
  digitalWrite(backLight, HIGH); 
digitalWrite(boiler, HIGH);
}

 // создаем метод для перевода показаний сенсора в градусы Цельсия 
double Getterm(int RawADC) {
  double temp;
  temp = log(((10230000/RawADC) - 10000));
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp = temp - 273.15;
  return temp;
}

// создаем метод для вывода на экран показаний сенсора
void printTemp(void) { 
  double temp = Getterm(analogRead(0));  // считываем показания с сенсора
  Serial.println(temp);
  }


void loop(void) {
  printTemp(); // вызываем метод, созданный ранее
  delay(1000);
}
