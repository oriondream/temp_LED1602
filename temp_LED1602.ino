#include <Wire.h>
#include "RTClib.h"

/**
 * IMPORTANT
 * 
 *     MAKE SURE THAT LIBRARIES VERSION ARE CORRECT !!!
 * 
 * Libraries:
 *   name: RTClib
 *     version: 1.3.3
 *     author: Adafruit
 *   name:DS3231
 *     version: 1.0.2
 *     author: Andrew Wickert
 *   name:MAX 31850 DallasTemp
 *     version: 1.1.4
 *     author: Adafruit
 */

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
const int RS = 9, EN = 8, D4 = 14, D5 = 15, D6 = 16, D7 = 17;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

bool SD_OK = true;

// Realtime clock object
RTC_DS3231  rtc;
DateTime rtc_now;

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
byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;

void display_date()
{
    String lcd_date = "";
    DateTime dt = rtc.now();
    int pos = 0;
    lcd_date += dt.day();
    if (dt.day() < 10) ++pos;
    lcd_date += "-";
    lcd_date += dt.month();
    if (dt.month() < 10) ++pos;
    lcd_date += "-";
    lcd_date += dt.year();

    lcd.setCursor(6 + pos,0);
    lcd.print(lcd_date);
}

uint8_t last_date, last_hour, last_minute;

void display_time()
{
    lcd_time = "";

    uint8_t hh, mm, ss;
    hh = rtc_now.hour();
    if (hh != last_hour) {
        last_hour = hh;
        if (rtc_now.day() != last_date) {
            last_date = rtc_now.day();
            display_date();
        }
        
        if (hh < 10)
        {
            lcd_time += ' ';
        }
        lcd_time += hh + String{":00:00"};
        lcd.setCursor(8, 1);
        lcd.print(lcd_time);
    }
    else {
        mm = rtc_now.minute();
        if (mm != last_minute) {
            last_minute = mm;
            if (mm < 10)
            {
                lcd_time += '0';
            }
            lcd_time += mm + String{":00"};
            lcd.setCursor(11, 1);
            lcd.print(lcd_time);
        }
        else {
            ss == rtc_now.second();
            if (rtc_now.second() < 10)
            {
                lcd_time += '0';
            }
            lcd_time += rtc_now.second();
            lcd.setCursor(14, 1);
            lcd.print(lcd_time);
        }
    }
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
    DateTime _dt;
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
    DataEntry(const RTC_DS3231* rtc, const DallasTemperature* temp)
        : DataEntry(rtc->now(), temp->getTempCByIndex(0)
          ) {
    }
    DataEntry(const DateTime& dt, float temperature)
        : _dt(dt), _temperature(temperature) {}
    const DateTime getDateTime() {return _dt;}
    float getTemp() {return _temperature;}

    String getTimeString() {
      return zero(_dt.hour()) + String{_dt.hour()} + ":" 
           + zero(_dt.minute()) + String{_dt.minute()} + ":"
           + zero(_dt.second()) + String{_dt.second()};
    }

    String getDateString() {
      return String{_dt.year()} + "/" + zero(_dt.month()) + String{_dt.month()} + "/" 
           + zero(_dt.day()) + String{_dt.day()};
    }

    String getDateTimeString() {
      return String{_dt.year()} + zero(_dt.month()) + String{_dt.month()} 
                           + zero(_dt.day()) + String{_dt.day()} 
                           + zero(_dt.hour()) + String{_dt.hour()} 
                           + zero(_dt.minute()) + String{_dt.minute()} 
                           + zero(_dt.second()) + String{_dt.second()};
    }
    String getTimeShort() {
      return zero(_dt.hour()) + String{_dt.hour()} 
           + zero(_dt.minute()) + String{_dt.minute()};
    }
};

void setRTC()
{
//    DateTime _now(2021,5,25,20,34,50);
//    rtc.adjust(_now);
}

String strSession;
String startTimeStr;

const byte INTERRUPT_DIGITAL_PIN = 2;
#define ALRM1_MATCH_EVERY_SEC  0b1111  // once a second
#define ALRM1_MATCH_SEC        0b1110  // when seconds match
#define ALRM1_MATCH_MIN_SEC    0b1100  // when minutes and seconds match
#define ALRM1_MATCH_HR_MIN_SEC 0b1000  // when hours, minutes, and seconds match

#define ALRM2_ONCE_PER_MIN     0b111   // once per minute (00 seconds of every minute)
#define ALRM2_MATCH_MIN        0b110   // when minutes match
#define ALRM2_MATCH_HR_MIN     0b100   // when hours and minutes match

volatile bool time_updated = true;

void update_time()
{
    time_updated = true;
}

void setupInterrupt()
{
    // enable the 1 Hz output
    rtc.writeSqwPinMode (DS3231_SquareWave1Hz);    
    pinMode(INTERRUPT_DIGITAL_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_DIGITAL_PIN), update_time, FALLING);
}

DataEntry data_entries[10];
byte iDE = 0;

void record_data()
{    
    sensors.requestTemperatures();
    t = sensors.getTempCByIndex(0);   

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

    call(record_data, last_record_data, 1000);
    call(write_SD, last_SD_card_write, 10000);
    call(display_temp, last_temp_update, 1000);
    
    if (time_updated) 
    {
        rtc_now = rtc.now();
        display_time();
        time_updated = false;
    }
}

void setup()
{    
    Serial.begin(9600);
    // set up the LCD's number of columns and rows:
    lcd.begin(16, 2);
    lcd.createChar(0, degree_symbol);
    // Print a message to the LCD.
    lcd.setCursor(0, 0);
    lcd.print("Temp");
    
    sensors.begin();
    
    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) 
    {
         ; // wait for serial port to connect. Needed for native USB port only
    }
    
    Serial.print("Initializing SD card...");

    if (!SD.begin(7)) 
    {
        Serial.println("initialization failed!");
        return;
    }
    Serial.println("initialization done.");

    // Delay a bit so that RTC would properly init
    DataEntry startTime(&rtc, &sensors);
    // startTimeStr = startTime.getTimeShort();
    startTimeStr = "0001";
    
    Wire.begin();

    setRTC();

    display_date();

    setupInterrupt();
}