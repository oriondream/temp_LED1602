#include <Wire.h>

/*
  The circuit:
   LCD VSS -> ground
   LCD VCC -> 5V
   LCD VO -> poty slider
   LCD RS -> Arduino 9
   LCD RW -> ground
   LCD E -> Arduino 8
   LCD D4 -> Arduino 5
   LCD D5 -> Arduino 4
   LCD D6 -> Arduino 3
   LCD A -> Arduino 2
*/
#include <DS3231.h>
#include <Wire.h>
// File
#include <SPI.h>
#include <SD.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>

// OneWire
#include <OneWire.h>
#define ONE_WIRE_PIN 6

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// include the library code:

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 9, en = 8, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

bool SD_OK = true;

// Realtime clock object
DS3231  rtc;

bool LED_ON = true;

unsigned long last_time_update = 0;
unsigned long last_temp_update = 0;
unsigned long last_SD_card_write = 0;
unsigned long last_record_data = 0;
unsigned long recent;
unsigned long time;

float t;

byte degree_symbol[8] = {
  B00100,
  B01010,
  B00100,
  B00000,
  B00000,
  B00000,
  B00000,
};

void display_temp()
{
    lcd.setCursor(0, 1);
    lcd.print(t);
    lcd.write(byte(0));
}

String lcd_time;

int year, month, date;

void display_date()
{
    String lcd_date = "";
    lcd_date += (date = rtc.getDate());
    lcd_date += "/";
    bool century_rollover;
    lcd_date += (month = rtc.getMonth(century_rollover));
    lcd_date +="/20";
    lcd_date += (year = rtc.getYear());

    int px;

    switch (lcd_date.length()) 
    {
        case 8:
            px = 8;
            break;
        case 9:
            px = 7;
            break;
        case 10:
            px = 6;
            break;
    }
    lcd.setCursor(px,0);
    lcd.print(lcd_date);
}

byte s;
byte m;
byte h;

void display_time()
{
    // hour part
    lcd_time = "";
    if (h < 10)
    {
        lcd_time += ' ';
    }
    lcd_time += h + String{":"};

    // minute part
    if (m < 10)
    {
        lcd_time += '0';
    }
    lcd_time += m + String{':'};

    // second part
    if (s < 10)
    {
        lcd_time += '0';
    }
    lcd_time += s;

    lcd.setCursor(8, 1);
    lcd.print(lcd_time);
}

void call(void(*f)(...), unsigned long& last_update, unsigned long const freq)
{
    unsigned long now = millis();
    if (now - last_update >= freq)
    {
        f();
        last_update = now;
    }
}
bool century_rollover;

bool H12 = false;
bool PM = false;

class DataEntry 
{
    byte _year, _month, _date, _hour, _minute, _second;       
    float _temperature;
private:
    String zero(byte v) {
        if (v < 10)
            return String{"0"};
        else
            return String{""};
    }
public:
    DataEntry(){};
    DataEntry(const DS3231* rtc, const DallasTemperature* temp)
        : DataEntry(
            rtc->getYear(), rtc->getMonth(century_rollover), rtc->getDate(), 
            rtc->getHour(H12,PM), rtc->getMinute(), rtc->getSecond(),
            temp->getTempCByIndex(0)
          ) {
    }
    DataEntry(byte year, byte month, byte date, byte hour, byte minute, byte second, float temperature)
        : _year(year), _month(month), _date(date), _hour(hour), _minute(minute), _second(second), _temperature(temperature) {}
    byte getYear() {return _year;}
    byte getMonth() {return _month;}
    byte getDate() {return _date;}
    byte getHour() {return _hour;}
    byte getMinute() {return _minute;}
    byte getSecond() {return _second;}
    float getTemp() {return _temperature;}

    String getTimeString() {
      return zero(_hour) + String{_hour} + ":" 
           + zero(_minute) + String{_minute} + ":"
           + zero(_second) + String{_second};
    }

    String getDateString() {
      return String{_year} + "/" + zero(_month) + String{_month} + "/" 
           + zero(_date) + String{_date};
    }

    String getDateTimeString() {
      return String{_year} + zero(_month) + String{_month} 
                           + zero(_date) + String{_date} 
                           + zero(_hour) + String{_hour} 
                           + zero(_minute) + String{_minute} 
                           + zero(_second) + String{_second};
    }
    String getTimeShort() {
      return zero(_hour) + String{_hour} 
           + zero(_minute) + String{_minute};
    }
};

void setRTC()
{
//    rtc.setYear(19);
//    rtc.setMonth(03);
//    rtc.setDate(23);
//    rtc.setHour(18);
//    rtc.setMinute(06);
//    rtc.setSecond(30);  
}

String strSession;
String startTimeStr;
void setup()
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.createChar(0, degree_symbol);
  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  lcd.print("Temp");

  sensors.begin();

  // Open serial communications and wait for port to open:
  //    Serial.begin(9600);
  //    while (!Serial) {
  //        ; // wait for serial port to connect. Needed for native USB port only
  //    }

  //    Serial.print("Initializing SD card...");

    if (!SD.begin(7)) 
    {
        //        Serial.println("initialization failed!");
        return;
    }
    //    Serial.println("initialization done.");

    // Delay a bit so that RTC would properly init
    DataEntry startTime(&rtc, &sensors);
//    startTimeStr = startTime.getTimeShort();
    startTimeStr = "0001";
    
    Wire.begin();
    
    display_date();
}

DataEntry data_entries[10];
byte iDE = 0;

void record_data()
{    
    data_entries[iDE] = DataEntry(&rtc, &sensors);
    iDE = (iDE + 1) % 10;
}

void write_SD()
{
    File fTempLog;
    fTempLog = SD.open("temp.log", FILE_WRITE);

    if (fTempLog) {
        for (auto e:data_entries) {
            String str = e.getDateString() + String{"\t"} 
                       + e.getTimeString() + String{"\t"} + String{e.getTemp()};
            fTempLog.println(str);
        }
        fTempLog.close();
    }
}

void loop() 
{
    time = millis();
    sensors.requestTemperatures();
    t = sensors.getTempCByIndex(0);

    s = rtc.getSecond();
    m = rtc.getMinute();
    h = rtc.getHour(H12,PM);
    
    call(record_data, last_record_data, 1000);
    call(write_SD, last_SD_card_write, 10000);
    call(display_temp, last_temp_update, 1000);
    call(display_time, last_time_update, 1000);
}
