/*
** Энкодер
** Для управлением яркостью LED используется энкодер Sparkfun
*/
 
int brightness = 120;       // яркость LED, начинаем с половины
int fadeAmount = 1;        // шаг изменения яркости LED
unsigned long currentTime;
unsigned long loopTime;
unsigned long loopTime2;
const int pin_A = 6;       // pin 12
const int pin_B = 7;       // pin 11
unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;
//3 gnd
//4 +5v

#define gnd 4
#define vcc 3
#define pinout 11
 
void setup()  {
  Serial.begin(9600);
  pinMode(pin_A, INPUT);
  pinMode(pin_B, INPUT);
  currentTime = millis();
  loopTime = currentTime; 
  pinMode(vcc, OUTPUT);
  pinMode(pinout, OUTPUT);
  pinMode(gnd, INPUT);  
  digitalWrite(gnd, HIGH); 
  //digitalWrite(vcc, LOW); 
} 
 
void loop()  {
  currentTime = millis();
  if(currentTime >= (loopTime + 5)){ // проверяем каждые 5мс (200 Гц)
    encoder_A = digitalRead(pin_A);     // считываем состояние выхода А энкодера 
    encoder_B = digitalRead(pin_B);     // считываем состояние выхода B энкодера    
    if((!encoder_A) && (encoder_A_prev)){    // если состояние изменилось с положительного к нулю
      if(encoder_B) {
        // выход В в полож. сост., значит вращение по часовой стрелке
        // увеличиваем яркость, не более чем до 255
        if(brightness + fadeAmount <= 255) brightness += fadeAmount;               
      }else{
        // выход В в 0 сост., значит вращение против часовой стрелки     
        // уменьшаем яркость, но не ниже 0
        if(brightness - fadeAmount >= 0) brightness -= fadeAmount;               
      }   
 
    }   
    encoder_A_prev = encoder_A;     // сохраняем значение А для следующего цикла 
     
    analogWrite(pinout, brightness);   // устанавливаем яркость на 9 ножку
    
    loopTime = currentTime;
  }                       
  if(currentTime >= (loopTime2 + 1000)){
      Serial.println(brightness);
      loopTime2 = currentTime;
  }
}
