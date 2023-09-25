/*ВАЖНО, ПЕРЕЗАЛЕЙТЕ БИБЛИОТЕКУ NA226.h ИНАЧЕ У ВАС НЕ БУДУТ РАБОТАТЬ ГРАФИКИ*/
#include <INA226.h>      //Добавляем библиотеку из архива
#include <SPI.h>         //Добавляем библиотеку из архива
#include <TFT_ST7735.h>  //Добавляем библиотеку из архива

TFT_ST7735 tft = TFT_ST7735();  //Инициируем класс
INA226_Class INA226;            //Инициируем класс


//Объявляем переменные
unsigned long TIME            = 0;  //Обявляем переменную для милисекунд
unsigned long TIME_TEMPERATURE= 0;  //Обявляем переменную для милисекунд
unsigned long TIME_GRAPH      = 0;  //Обявляем переменную для милисекунд
unsigned long TIME_DISPLAY    = 0;  //Обявляем переменную для милисекунд

#define SHUNT_MICRO_OHM 699  //Сопротивление шунта в микромах, 0,1 Ом это 100000 микрон
#define MAXIMUM_AMPS 99      //Максимальное значение тока, значения 1 = 0.1А - ограничено до 99 = 9.9А

const float MINIMUM_TEMPERATURE = 30;  //Минимальная температура при которой начнет работать ШИМ вентилятора.
const float MAXIMUM_TEMPERATURE = 60;  //Температура при которой скорость вентилятора будет максимальной.

#define B 3950               //B-коэффициент
#define SERIAL_R 99000       //Сопротивление последовательного резистора, 100 кОм
#define THERMISTOR_R 70000  //Номинальное сопротивления термистора, 74 кОм
#define NOMINAL_T 25         //Номинальная температура (при которой TR = 74 кОм)

#define PIN_INPUT A0   //Устанавливаем пин А0 на приём даных с терморезистрора
#define PIN_RESET A1   //Устанавливаем пин А1 для кнопки сброса счётчика мАч
#define PIN_SELECT A2  //Устанавливаем пин А2 на переключение
#define PIN_FAN 3      //Устанавливаем пин D3 для шин сигнала на вентилятор

#define PIN_BUTTON_A3 A3  //Устанавливаем пин A3 для кнопки 3
#define PIN_BUTTON_A6 A6  //Устанавливаем пин A6 для кнопки 4
#define PIN_BUTTON_A7 A7  //Устанавливаем пин A7 для кнопки 5
#define PIN_BUTTON_12 12  //Устанавливаем пин D12 для кнопки 6


//Вспомогательные переменные для обработки данных
float TEMPERATURE = 0, VOLTS, AMPS, WATTS, mAh, Wh, SAVE_MAX;
byte FAN = 0;
unsigned long SET_MILLISECOND;

//Строим графики
float VOLTAGE_GRAPH[125], VOLTAGE_GRAPH_SUMM = 0, MIN_VOLTAGE_GRAPH = 36, MAX_VOLTAGE_GRAPH = 0, MAX_AMP_GRAPH;  //Обявляем массив для хнанения в нём данных по графику напряжения и вспомогательные переменные для получения среднего значения
float AMP_GRAPH[125], AMP_GRAPH_SUMM = 0;                                                                        //Обявляем массив для хнанения в нём данных по графику силы тока и вспомогательные переменные для получения среднего значения
byte MINUT_GRAPH = 0, SECOND_GRAPH = 0;                                                                          //Считаем в минутах и секундах

//Проверяем кнопки на одинарное нажатие
bool SELECT_GRAPH = false, BUTTOM_RESET = true, BUTTOM_SELECT = true, FIRST_FAN = true, SELECT_DISPLAY = true;  //Вспомогательные переменные для проверки одинарного нажатия кнопки

/*
 * Задаём цвета для показаний
 * Цвета можно указывать, как ST7735_НАЗВАНИЕ_ЦВЕТА, например ST7735_WHITE - белый, так и в формате RGB565, например 0xFFFF - белый
 * Сайт что бы конвертировать HEX (RGB8880 в RGB565 https://trolsoft.ru/ru/articles/rgb565-color-picker
*/
#define COLOR_VOLTS ST7735_WHITE       //Красим Вольты в белый
#define COLOR_AMPS ST7735_RED          //Красим Амперы в карсный
#define COLOR_WATTS ST7735_GREEN       //Красим Ватты в зелёный
#define COLOR_INDICATOR ST7735_YELLOW  //Красим индикаторы в жёлтый цвет



void setup() {
  TCCR2B = TCCR2B & 0b11111000 | 0x01;  //Включаем частоту ШИМ'а  вентилятора на ногах 3 и 11: 31250 Гц.

  pinMode(PIN_INPUT, INPUT);             //Устанавливаем пин на приём сигнала
  pinMode(PIN_RESET, INPUT_PULLUP);      //Устанавливаем пин на приём сигнала
  pinMode(PIN_SELECT, INPUT_PULLUP);     //Устанавливаем пин на приём сигнала
  pinMode(PIN_BUTTON_A3, INPUT_PULLUP);  //Устанавливаем пин на приём сигнала
  pinMode(PIN_BUTTON_12, INPUT_PULLUP);  //Устанавливаем пин на приём сигнала

  INA226.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM);  //Установливаем максимальный ток и сопротивление шунта
  INA226.setBusConversion(8244);                //Время конверсии в микросекундах (140,204,332,588,1100,2116,4156,8244)8244µs=8.244 ms
  INA226.setShuntConversion(8244);              //Время конверсии в микросекундах (140,204,332,588,1100,2116,4156,8244)8244µs=8.244 ms
  INA226.setAveraging(4);                       //Среднее количество чтений n раз (1,4,16,64,128,256,512,1024)
  INA226.setMode(INA_MODE_CONTINUOUS_BOTH);     //Шина / шунт измеряется постоянно

  tft.init();                 //Инициализация дисплея.
  tft.setRotation(1);         //Переворачиваем дисплей.
  tft.fillScreen(TFT_BLACK);  //Указываем цвет заливки дисплея

  //Закрашиваем не закрашенные места на экране
  tft.fillRect(-1, -2, 160, 128, ST7735_BLACK);
  tft.fillRect(2, 1, 154, 122, ST7735_BLACK);


  STATIC();
  SET_MILLISECOND = millis();  //Сохраняем текущее время
}

void STATIC() {
  tft.setTextColor(COLOR_VOLTS, ST7735_BLACK);  //цвет текста белый, цвет заливки текста чёрный
  //Все статические данные, которые будут отображаться на дисплее
  tft.drawRightString("t:", 0, 0, 2);           //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("C", 55, 0, 2);           //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("o", 62, 0, 1);           //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("fan:", 115, 0, 2);       //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("%", 155, 0, 2);          //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("V", 155, 37, 4);         //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.setTextColor(COLOR_AMPS, ST7735_BLACK);   //цвет текста белый, цвет заливки текста чёрный
  tft.drawRightString("A", 120, 71, 2);         //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.setTextColor(COLOR_WATTS, ST7735_BLACK);  //цвет текста зелёный, цвет заливки текста чёрный
  tft.drawRightString("W", 122, 95, 2);         //"Текст", положение по оси Х, положение по оси Y, размер шрифта
  tft.drawRightString("mAh:", 0, 114, 2);       //"Текст", положение по оси Х, положение по оси Y, размер шрифта
}


char DATA_RESULT[20];                                        //Объявляем переменную
static char* DISPLAY_TEXT(float DATA, byte COUNT, byte FLOAT)  //функция затирает предыдущие показания пробелами
{
  char DATA_DISPLAY[20];                      //Объявляем переменную
  dtostrf(DATA, COUNT, FLOAT, DATA_DISPLAY);  //Конвертируем показания в привычные глазу данные для дальнейшего вывода на экран

  byte LEN = strlen(DATA_DISPLAY);  //Узнаём длину полученных данных

  for (byte i = 0; i < COUNT - (LEN - 1); ++i)  //Вычисляем сколько пробелов не хватает
  {
    strcpy(DATA_RESULT, " ");  //Создаём строку из недостающих пробелов
  }
  strcat(DATA_RESULT, DATA_DISPLAY);  //Добавляем недостающие пробелы

  return DATA_RESULT;  //Отдаём результат
}

byte MAPFLOAT(float VALUE, byte MININ, byte MAXIN, byte MINOUT, byte MAXOUT) {
  return (float)(VALUE - MININ) * (MAXOUT - MINOUT) / (float)(MAXIN - MININ) + MINOUT;
}

void BACKGROUND() {
  byte HEIGHT = tft.height();
  byte WIDGHT = tft.width();
  for (byte x = 26; x <= WIDGHT; x += 5) {
    if (x - 1 <= WIDGHT - 10)
      for (byte y = 18; y < HEIGHT - 20; y += 18) {
        tft.drawFastHLine(x, y, 2, ST7735_YELLOW);
      }


    if ((x - 1) % 25 == 0) {
      tft.drawFastHLine((x - 1 == 25 ? x : (x - 1 == WIDGHT - 10 ? x - 2 : x - 1)), 0, 3, ST7735_YELLOW);
      tft.drawFastHLine((x - 1 == 25 ? x : (x - 1 == WIDGHT - 10 ? x - 2 : x - 1)), HEIGHT - 20, 3, ST7735_YELLOW);
    }
  }

  for (byte x = 26; x <= WIDGHT; x += 25) {
    for (byte y = 0; y < HEIGHT - 20; y += 5) {
      tft.drawFastVLine(x, y, 2, ST7735_YELLOW);
    }
  }
}


void CLEAR_GRAPH()
{
  for (byte y= 0; y < 6; ++y)
    for (byte x= 0; x < 5; ++x)
      tft.fillRect(27 + 25 * x,  1 + 18 * y, 24, 17, ST7735_BLACK);

  byte HEIGHT = tft.height();
  byte WIDGHT = tft.width();
  for (byte x = 26; x <= WIDGHT; x += 5) {
    if (x - 1 <= WIDGHT - 10)
      for (byte y = 18; y < HEIGHT - 20; y += 18) {
        tft.drawFastHLine(x + 2, y, 3, ST7735_BLACK);
      }

    if ((x - 1) % 25 == 0) {
      tft.drawFastHLine((x - 1 == 25 ? x + 3 : (x - 1 == WIDGHT - 10 ? x : x + 1)), 0, 24, ST7735_BLACK);
      tft.drawFastHLine((x - 1 == 25 ? x + 3 : (x - 1 == WIDGHT - 10 ? x : x + 1)), HEIGHT - 20, 24, ST7735_BLACK);
    }
  }

  for (byte x = 26; x <= WIDGHT; x += 25) {
    for (byte y = 0; y < HEIGHT - 20; y += 5) {
      tft.drawFastVLine(x, y + 2, 3, ST7735_BLACK);
    }
  }
}

void CLEAR(bool DATA) {
  tft.fillScreen(TFT_BLACK);  //Указываем цвет заливки дисплея
  if (DATA)
    STATIC();
}

void loop() {
  //Получаем данные от INA226
  VOLTS = INA226.getBusMilliVolts() / 10e2;
  AMPS  = INA226.getBusMicroAmps() / 10e4;
  WATTS = INA226.getBusMicroWatts() / 10e6;

  //Обнуляем отрицательные показания
  if (VOLTS < 0)
    VOLTS = 0;

  if (AMPS < 0)
    AMPS = 0;

  if (WATTS < 0)
    WATTS = 0;

  mAh += AMPS * (millis() - SET_MILLISECOND) / 3600000 * 1000;  //расчет емкости  в мАч
  SET_MILLISECOND = millis();                                   //Обнавляем текущее время


  //Определяем температуру на датчике с помощью модифицированной формулы Стейнхарта — Харта: 1/T = 1/T + 1/B * ln (R/R0)
  //Подробней можете прочитать в статье http://psenyukov.ru/%D0%BF%D0%BE%D0%B4%D0%BA%D0%BB%D1%8E%D1%87%D0%B5%D0%BD%D0%B8%D0%B5-%D1%82%D0%B5%D1%80%D0%BC%D0%B8%D1%81%D1%82%D0%BE%D1%80%D0%B0-%D0%BA-arduino/
  int t = analogRead(PIN_INPUT);  //Считываем показания датчика температуры с пина А1
  float tr = 1023.0 / t - 1;
  tr = SERIAL_R / tr;
  float TEMPERATURE;
  TEMPERATURE = tr / THERMISTOR_R;
  TEMPERATURE = log(TEMPERATURE);
  TEMPERATURE /= B;
  TEMPERATURE += 1.0 / (NOMINAL_T + 273.15);
  TEMPERATURE = 1.0 / TEMPERATURE;
  TEMPERATURE -= 273.15;


  //Рассчитываем  ШИМ вентилятора.
  if (TEMPERATURE >= MINIMUM_TEMPERATURE && TEMPERATURE <= MAXIMUM_TEMPERATURE)  //Если температура в среднем показателе, расчитываем обороты
  {
    if (FIRST_FAN) {
      analogWrite(PIN_FAN, 255);  //Даём пинок вентилятору, что бы он не дёргался при старте.
      FIRST_FAN = false;
    }
    FAN = (TEMPERATURE - MINIMUM_TEMPERATURE) * 255 / (MAXIMUM_TEMPERATURE - MINIMUM_TEMPERATURE);
  } else if (TEMPERATURE < MINIMUM_TEMPERATURE)  //Если температура минимум
  {
    FAN = 0;  //Отключаем вентилятор
    FIRST_FAN = true;
  } else if (TEMPERATURE >= MAXIMUM_TEMPERATURE)  //Если температура максимум
  {
    FAN = 255;  //Включаем вентилятор
  }

  MIN_VOLTAGE_GRAPH = min(MIN_VOLTAGE_GRAPH, VOLTS);  //Находим максимальное значение напряжения
  MAX_VOLTAGE_GRAPH = max(MAX_VOLTAGE_GRAPH, VOLTS);  //Находим минимальное значение напряжения
  MAX_AMP_GRAPH     = max(MAX_AMP_GRAPH, AMPS);       //Находим максимальное значение силы тока


  if (millis() - TIME_GRAPH >= 1000)  //Считываем данные раз в секунду
  {
    VOLTAGE_GRAPH_SUMM  += VOLTS;     //Сумируем показания напряжения
    AMP_GRAPH_SUMM      += AMPS;      //Сумируем показания силы тока
    ++SECOND_GRAPH;
    if (SECOND_GRAPH >= 60)           //Отсчитываем одну минуту (60 секунд)
    {
      byte COUNT_ARRAY = sizeof(VOLTAGE_GRAPH) / sizeof(float) - 1;  //Получаем длину массива
      if (MINUT_GRAPH >= COUNT_ARRAY)                                //Если количество минут больше длины массива, то сдвигаем его на 30 минут влево
      {
        for (byte i = 0; i < COUNT_ARRAY; ++i) {
          VOLTAGE_GRAPH[i]  = VOLTAGE_GRAPH[i + 1];
          AMP_GRAPH[i]      = AMP_GRAPH[i + 1];
        }

        MINUT_GRAPH -= 1;

        if (!digitalRead(PIN_SELECT) || SELECT_GRAPH)  //Очищаем экран от станых значений
          CLEAR_GRAPH();
      }


      VOLTAGE_GRAPH[MINUT_GRAPH]  = VOLTAGE_GRAPH_SUMM / 60;  //Посчитываем среднее арифметическое показаний за минуту (60 секунд)
      AMP_GRAPH[MINUT_GRAPH]      = AMP_GRAPH_SUMM / 60;
      SECOND_GRAPH                = 0;
      VOLTAGE_GRAPH_SUMM          = 0;
      AMP_GRAPH_SUMM              = 0;
      ++MINUT_GRAPH;
    }
    TIME_GRAPH = millis();
  }

  if (BUTTOM_SELECT)  //Переключаемся между экранами
    if (!digitalRead(PIN_SELECT)) {
      BUTTOM_SELECT     = false;
      if (SELECT_GRAPH) {
        SELECT_GRAPH    = false;
        CLEAR(true);
      } else {
        SELECT_GRAPH    = true;
        TIME_DISPLAY    = 0;
        SELECT_DISPLAY  = true;
        CLEAR(false);
      }
    }
  if (digitalRead(PIN_SELECT))
    BUTTOM_SELECT = true;


 
  if (SELECT_GRAPH) {
    BACKGROUND();

    float MAX   = max(MAX_VOLTAGE_GRAPH, 0);
    MAX         = max(MAX_AMP_GRAPH, MAX);
    MAX         = ceil(max(MAX_VOLTAGE_GRAPH * MAX_AMP_GRAPH, MAX));
    if (MAX == 0)
      MAX       = 1;

    if (MAX != SAVE_MAX)
      CLEAR_GRAPH();
    SAVE_MAX    = MAX;

    byte HEIGHT = tft.height() - 20;
    byte WIDGHT = tft.width() - 26;
    for (byte i = 26; i < MINUT_GRAPH + 26; ++i) {  //Рисуем графики на экране
      
      tft.drawLine(i, MAPFLOAT(VOLTAGE_GRAPH[i - 26], 0, MAX, HEIGHT, 0), i, MAPFLOAT(VOLTAGE_GRAPH[(i - 26 == 0 ? i - 26 : i - 27)], 0, MAX, HEIGHT, 0), COLOR_VOLTS);

      tft.drawLine(i, MAPFLOAT(AMP_GRAPH[i - 26], 0, MAX, HEIGHT, 0), i, MAPFLOAT(AMP_GRAPH[(i - 26 == 0 ? i - 26 : i - 27)], 0, MAX, HEIGHT, 0), COLOR_AMPS);

      tft.drawLine(i, MAPFLOAT(VOLTAGE_GRAPH[i - 26] * AMP_GRAPH[i - 26], 0, MAX, HEIGHT, 0), i, MAPFLOAT(VOLTAGE_GRAPH[(i - 26 == 0 ? i - 26 : i - 27)] * AMP_GRAPH[(i - 26 == 0 ? i - 26 : i - 27)], 0, MAX, HEIGHT, 0), COLOR_WATTS);
    }
    
    byte DIVIDER = 6;
    tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);  //цвет текста белый, цвет заливки текста чёрный
    for (byte y = 0; y <= HEIGHT; y += 18) {
      tft.drawRightString(DIVIDER == 7 ? "   0" : DISPLAY_TEXT((MAX / 6 * DIVIDER), 3, (MAX / 6 * DIVIDER) >= 10 ? 0 : 1), 0, y > 0 ? y == HEIGHT ? y - 8 : y - 3 : y, 1);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта
      --DIVIDER;
    }
    
    if (millis() - TIME_DISPLAY >= 5000 || TIME_DISPLAY == 0)
    {
      tft.fillRect(0,  144, 160, 20, ST7735_BLACK);
      if (!SELECT_DISPLAY)
      {
        tft.drawRightString("min", 0, 114, 1);    //"Текст", положение по оси Х, положение по оси Y, размер шрифта
        for (byte x = 26; x <= WIDGHT; x += 25)
          tft.drawRightString(DISPLAY_TEXT(x - 1, 3, 0), x + 25, 114, 1);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта
      }
      else
      {
        tft.setTextColor(COLOR_VOLTS, ST7735_BLACK);   
        tft.drawRightString("min:", 0, 114, 1);    //"Текст", положение по оси Х, положение по оси Y, размер шрифта
        tft.drawRightString(DISPLAY_TEXT(MIN_VOLTAGE_GRAPH, 5, 2), 65, 114, 1);   //"Текст", положение по оси Х, положение по оси Y, размер шрифта
        tft.drawRightString("V", 72, 114, 1);      //"Текст", положение по оси Х, положение по оси Y, размер шрифта
        tft.drawRightString("max:", 105, 114, 1);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта
        tft.drawRightString(DISPLAY_TEXT(MAX_VOLTAGE_GRAPH, 5, 2), 147, 114, 1);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта
        tft.drawRightString("V", 154, 114, 1);     //"Текст", положение по оси Х, положение по оси Y, размер шрифта                           //цвет текста белый, цвет заливки текста чёрный
      }
      TIME_DISPLAY = millis();
      if (SELECT_DISPLAY)
        SELECT_DISPLAY = false;
      else
        SELECT_DISPLAY = true;
    }
  } else {
    tft.setTextColor(COLOR_VOLTS, ST7735_BLACK);                     //цвет текста белый, цвет заливки текста чёрный
    if (millis() - TIME_TEMPERATURE >= 1000)                      //Добавляем задержку в 10 милисекунд
    {
      tft.drawRightString(DISPLAY_TEXT(TEMPERATURE, 4, 0), 45, 0, 2);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта
      tft.drawRightString(DISPLAY_TEXT(FAN / 2.55, 3, 0), 145, 0, 2);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта
      TIME_TEMPERATURE = millis();
    }
    if (millis() - TIME >= 100)                                   //Добавляем задержку в 10 милисекунд
    {
      tft.drawRightString(DISPLAY_TEXT(VOLTS, 5, 2), 138, 19, 6);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта


      tft.setTextColor(COLOR_AMPS, ST7735_BLACK);                 //цвет текста белый, цвет заливки текста чёрный
      tft.drawRightString(DISPLAY_TEXT(AMPS, 6, 3), 110, 65, 4);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта


      tft.setTextColor(COLOR_WATTS, ST7735_BLACK);                 //цвет текста белый, цвет заливки текста чёрный
      tft.drawRightString(DISPLAY_TEXT(WATTS, 7, 3), 110, 89, 4);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта

      char mAh_DISPLAY[12];
      dtostrf(mAh, 10, 2, mAh_DISPLAY);               //Конвертируем показания в привычные глазу данные для дальнейшего вывода на экран
      tft.drawRightString(mAh_DISPLAY, 155, 114, 2);  //"Текст", положение по оси Х, положение по оси Y, размер шрифта
      TIME = millis();
    }


    tft.setTextColor(COLOR_INDICATOR, ST7735_BLACK);
  }

  if (BUTTOM_RESET)  //Очищаем все сохранённые значения
    if (!digitalRead(PIN_RESET)) {
      if (SELECT_GRAPH) {
        CLEAR(false);
      }
      BUTTOM_RESET        = false;
      mAh                 = 0;
      MIN_VOLTAGE_GRAPH   = 36;
      MAX_VOLTAGE_GRAPH   = 0;
      MAX_AMP_GRAPH       = 0;
      memset(VOLTAGE_GRAPH, 0, (sizeof(VOLTAGE_GRAPH) / sizeof(float)) - 1);
      memset(AMP_GRAPH, 0, (sizeof(AMP_GRAPH) / sizeof(float)) - 1);
      SECOND_GRAPH        = 0;
      VOLTAGE_GRAPH_SUMM  = 0;
      AMP_GRAPH_SUMM      = 0;
      MINUT_GRAPH         = 0;
      TIME_DISPLAY        = 0;
      SELECT_DISPLAY      = true;
    }
  if (digitalRead(PIN_RESET)) {
    BUTTOM_RESET          = true;
  }

  analogWrite(PIN_FAN, FAN);  //Передаём сигнал для вентилятора на 3 пин
}
