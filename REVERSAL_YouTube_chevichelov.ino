#define PIN_P2   A1                                                  //Назначаем пин P2
#define PIN_P1   1                                                   //Назначаем пин P1

#define R1   98.9                                                    //Сопротивление резистора R1 в кОм
#define R2   9.7                                                     //Сопротивление резистора R2 в кОм

void setup() {
  pinMode(PIN_P2, INPUT);                                            //Пин установлен на вход
  pinMode(PIN_P1, OUTPUT);                                           //Пин установлен на выход
}

void loop() {
  if (analogRead(PIN_P2) * 5 / 1024.0f * (( R1 + R2 ) / R2) > 0)    //Измеряем напряжение на пине A1 
  {
    digitalWrite(PIN_P1, HIGH);                                     //Если оно выше нуля, подаём сигнал на пин 1
  }
  else
  {
    digitalWrite(PIN_P1, LOW);                                       //Если оно равно нулю, не подаём сигнал на пин 1
  }
}
