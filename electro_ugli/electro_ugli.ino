#include <PCF8814.h> 
#include <math.h>
#include <TimerOne.h>
#include <PID_v1.h>
#include <EEPROM.h>
#include <OneWire.h>

//PCF8814 Lcd(13,11,10,6); // LCD sets SPI SCLK: 7 pin, SDA: 8 pin, CS: 9 pin. RESET: 6 pin
PCF8814 Lcd(4,5,6,8);

#define sensor A0
#define alertsensor 10
#define heater 3 
#define lcdled A3 
#define button1 2 
#define button2 7 

#define LED_PIN 13     // номер вывода светодиода равен 13

int countpreviev;
int button1val;
int button2val;
long previousMillis,prevbuttonmillis,prevlongbuttmil;
int sensorval;
int counttemp;
double summtemp,temp,tempalert;
byte bufdata[9];
byte addr[8];
int16_t raw;

double Setpoint, Input, Output;
double aggKp=4, aggKi=0.2, aggKd=1;
double consKp=1, consKi=0.05, consKd=0.25;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint, consKp, consKi, consKd, DIRECT);
OneWire  ds(alertsensor);

const short temptable_11[][2] PROGMEM = {
    { 6, 350 },
    { 14, 310 },
    { 832, 65 },
    { 858, 60 },
    { 883, 55 },
    { 904, 50 },
    { 924, 45 },
    { 940, 40 }, 
    { 955, 35 }, 
    { 968, 30 },
    { 977, 25 },
    { 987, 20 },
    { 994, 15 },
    { 999, 10 } 
};

#define BEDTEMPTABLE_LEN (sizeof(temptable_11)/sizeof(*temptable_11))
#define PGM_RD_W(x)   (short)pgm_read_word(&x)
static float analog2tempBed(int raw) {
    float celsius = 0;
    byte i;
 
    for (i = 1; i < BEDTEMPTABLE_LEN; i++)
    {
        if (PGM_RD_W(temptable_11[i][0]) > raw)
        {
            celsius = PGM_RD_W(temptable_11[i - 1][1]) +
                (raw - PGM_RD_W(temptable_11[i - 1][0])) *
                (float)(PGM_RD_W(temptable_11[i][1]) - PGM_RD_W(temptable_11[i - 1][1])) /
                (float)(PGM_RD_W(temptable_11[i][0]) - PGM_RD_W(temptable_11[i - 1][0]));
            break;
        }
    }
 
    // Overflow: Set to last value in the table
    if (i == BEDTEMPTABLE_LEN) celsius = PGM_RD_W(temptable_11[i - 1][1]);
 
    return celsius;
}

//Установка Таймера2.
//Конфигурирует 8-битный Таймер2 ATMega для выработки прерывания
//с дискретом 64 мкс (период переполнения около 16мс)
void SetupTimer2()  {
  //Установки Таймер2: Делитель частоты /1024, режим 0
  //период дискретизации 64 мкс, т.е прер.16384мкс
  TCCR2A = 0; 
  TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20;   //на 1024
  //Подключение прерывания по переполнению Timer2
  TIMSK2 = 1<<TOIE2;
}

//Timer2 указатель вектора прерывания по переполнению
ISR(TIMER2_OVF_vect) {
///здесь выполняется прерывание по таймеру 2
}


void EEPROM_float_write(int addr, float val) // запись в ЕЕПРОМ
{  
  byte *x = (byte *)&val;
  for(byte i = 0; i < 4; i++) EEPROM.write(i+addr, x[i]);
}

float EEPROM_float_read(int addr) // чтение из ЕЕПРОМ
{    
  byte x[4];
  for(byte i = 0; i < 4; i++) x[i] = EEPROM.read(i+addr);
  float *y = (float *)&x;
  return y[0];
}

void buttoninterrupt(void) {
  
  if(digitalRead(button2) == 0 || digitalRead(button1) == 0){
      if(digitalRead(button1) == 0){
        button1val = 0;
      }   
      if(digitalRead(button2) == 0){
        button2val = 0;
      }
   }else{
      prevlongbuttmil = millis();
   }    

  digitalWrite(LED_PIN, button1val); 


    if(button1val == 0){
      if(millis() - prevlongbuttmil > 1000 || millis() - prevbuttonmillis > 600){
        prevbuttonmillis = millis();
        if(Setpoint <= 299 && digitalRead(lcdled) == HIGH){
          Setpoint = Setpoint + 1;
        }else{
          analogWrite(lcdled, 255);  
        }
      }
      button1val = 1;
    }
    
    if(button2val == 0){
      if(millis() - prevlongbuttmil > 1000 || millis() - prevbuttonmillis > 600){
        prevbuttonmillis = millis();
        if(Setpoint >= 1 && digitalRead(lcdled) == HIGH){
          Setpoint = Setpoint - 1;
        }else{
          analogWrite(lcdled, 255);  
        }
      }
      button2val = 1;
    }  

}
///////////////////////////////////////////      SETUP         //////////////////////////////////
void setup() { 
  analogReference(DEFAULT);
  Setpoint = EEPROM_float_read(0);
  countpreviev = EEPROM_float_read(5);
  //Setpoint =0;
  button1val = 1;
  button2val = 1;
  pinMode(LED_PIN, OUTPUT);
  pinMode(sensor, INPUT);
  pinMode(lcdled, OUTPUT);
  pinMode(button1, INPUT_PULLUP);//поддягиваем резистор к +5
  pinMode(button2, INPUT_PULLUP);//поддягиваем резистор к +5
  analogWrite(lcdled, 255);  
  pinMode(11, INPUT);
  digitalWrite(11, HIGH);  
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);  
  
  Lcd.Init();
  Lcd.Mirror(1,1);
  Lcd.GotoXY(6,0);

  if(Setpoint == 0 || countpreviev > 0){
    if(Setpoint == 0){//если температура установлена в 0, то запишем в показы приветствия 6
      EEPROM_float_write(5, 6);
    }else{
      EEPROM_float_write(5, countpreviev - 1);
    }
    Lcd.GotoXY_pix(0, 30);
    Lcd.PrintWide("Драстите!");
    delay(2000);
    Lcd.Clear(); 
    Lcd.GotoXY_pix(6, 0);
    Lcd.Print("Кароч с др!");
    Lcd.GotoXY_pix(6, 8);
    Lcd.Print("там исполнения");  
    Lcd.GotoXY_pix(6, 16);
    Lcd.Print("всяких желаний");  
    Lcd.GotoXY_pix(6, 24);
    Lcd.Print("и тд, итп.");          
    delay(4000); 
    Lcd.Clear(); 
    Lcd.GotoXY_pix(6, 0);
    Lcd.Print("Этот подарок");
    Lcd.GotoXY_pix(6, 8);
    Lcd.Print("в честь 27");    
    Lcd.GotoXY_pix(6, 16);
    Lcd.Print("Дня Рождения");  
    Lcd.GotoXY_pix(6, 24);
    Lcd.Print("Вадима М, От:");        
    Lcd.GotoXY_pix(6, 32);    
    Lcd.Print("Санина С и");  
    Lcd.GotoXY_pix(6, 40);
    Lcd.Print("Ивановой С");  
    delay(5000);        
  }else{
    Lcd.GotoXY_pix(0, 3);
    Lcd.PrintWide("Драстите!");
    delay(1000);
    Lcd.Clear(); 
    Lcd.GotoXY_pix(32, 15);
    Lcd.PrintWide("Это"); 
    Lcd.GotoXY_pix(8, 23);
    Lcd.PrintWide("Углегрев");      
    Lcd.GotoXY_pix(22, 31);
    Lcd.Print("Для калича");      
    Lcd.GotoXY_pix(16, 38);    
    Lcd.PrintWide("вер 1.0");  
    delay(3000);
  }

  counttemp = 0;
    


 // SetupTimer2();  //инициализировать прерывания таймера 2  без библиотек(влияет на шим на пинах 3 11 )
  Timer1.initialize(50000);
  Timer1.attachInterrupt(buttoninterrupt); // buttoninterrupt to run every 0.15 seconds(влияет на шим на пинах 9 10)

  //initialize the variables we're linked to
  Input = analog2tempBed(sensorval);

  //turn the PID on
  myPID.SetMode(AUTOMATIC);
    Lcd.Clear(); 
}

////////////////////////////////      LOOP       ///////////////////////////////////
void loop() {
 
  char buffer[16];  
  sensorval = analogRead(sensor);
  Input = analog2tempBed(sensorval);

  if(counttemp >= 100){
    temp = summtemp / 100;

    if(tempalert < 75){
      double gap = abs(Setpoint-temp); //определение дистанции до setpoint
      if(gap<25)
      {  //более мягкие коэффициенты
        myPID.SetTunings(consKp, consKi, consKd);
      }
      else
      {
         //агрессивные коэффициенты
         myPID.SetTunings(aggKp, aggKi, aggKd);
      }
      
      myPID.Compute();
    }else{
      Output = 0;
    }
    analogWrite(heater,Output);
    
    counttemp = 1;
    summtemp = Input;
  }else{
    summtemp = summtemp + Input;
    counttemp = counttemp + 1;
  }





    if(millis() - previousMillis > 1000){
        previousMillis = millis();
        ds.search(addr);
        ds.reset();
        ds.select(addr);//пропуск ROM 
        ds.write(0xBE, 1);//комманда на чтение данных из датчика
        ds.read_bytes(bufdata, 9);
        
        ds.reset();
        ds.select(addr);//пропуск ROM 
        ds.write(0x44, 1);//инициализация измерения датчиком.(время измерения не менее 750мс до получения данных)
        
        raw = (bufdata[1] << 8) | bufdata[0];
 
        byte cfg = (bufdata[4] & 0x60);
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
 
        tempalert = (float)raw / 16.0;
  
        Lcd.Clear(); 
        
/*
        Lcd.GotoXY_pix(1, 5);
        Lcd.PrintWide("Pow:");
        
        dtostrf((float) (Output * 100) / 255, 3, 2, buffer);
        Lcd.GotoXY_pix(45, 5);
        Lcd.PrintWide(buffer);

        Lcd.GotoXY_pix(1, 15);
        Lcd.PrintWide("Temp:");
                
        dtostrf(temp, 4, 0, buffer);
        Lcd.GotoXY_pix(50, 15);
        Lcd.PrintWide(buffer);                        

        Lcd.GotoXY_pix(1, 25);
        Lcd.PrintWide("Conf:");
        
        dtostrf(Setpoint, 3, 0, buffer);
        Lcd.GotoXY_pix(50, 25);
        Lcd.PrintWide(buffer);   
*/

        Lcd.GotoXY_pix(0, 10);
        Lcd.Print("Мощность %");
        
        dtostrf((float) (Output * 100) / 255, 3, 0, buffer);
        Lcd.GotoXY_pix(64, 10);
        Lcd.PrintWide(buffer);

        Lcd.GotoXY_pix(0, 28);
        Lcd.Print("Температура");
                
        dtostrf(temp, 3, 0, buffer);
        Lcd.GotoXY_pix(64, 28);
        Lcd.PrintWide(buffer);                        

        Lcd.GotoXY_pix(0, 46);
        Lcd.Print("Настройка");
        
        dtostrf(Setpoint, 3, 0, buffer);
        Lcd.GotoXY_pix(64, 46);
        Lcd.PrintWide(buffer); 

        Lcd.GotoXY_pix(0, 56);
        Lcd.Print("Перегрев"); 

        dtostrf(tempalert, 3, 1, buffer);
        Lcd.GotoXY_pix(53, 56);
        Lcd.PrintWide(buffer);                 
    }

    //сохранение настроек через 5сек после последнего изменения
    if(millis() - prevbuttonmillis > 10000){
      if(Setpoint != EEPROM_float_read(0)){
        EEPROM_float_write(0, Setpoint);
      }
    }
      
   

    //отключение подсветки через 90сек
    if(millis() - prevbuttonmillis > 90000){
        analogWrite(lcdled, 0);
    }

    
    
}

