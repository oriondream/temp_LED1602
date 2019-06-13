/*
  LiquidCrystal Library - Hello World

  Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
  library works with all LCD displays that are compatible with the
  Hitachi HD44780 driver. There are many of them out there, and you
  can usually tell them by the 16-pin interface.

  This sketch prints "Hello World!" to the LCD
  and shows the time.

  The circuit:
   LCD RS pin to digital pin 12
   LCD Enable pin to digital pin 11
   LCD D4 pin to digital pin 5
   LCD D5 pin to digital pin 4
   LCD D6 pin to digital pin 3
   LCD D7 pin to digital pin 2
   LCD R/W pin to ground
   LCD VSS pin to ground
   LCD VCC pin to 5V
   10K resistor:
   ends to +5V and ground
   wiper to LCD VO pin (pin 3)

  Library originally added 18 Apr 2008
  by David A. Mellis
  library modified 5 Jul 2009
  by Limor Fried (http://www.ladyada.net)
  example added 9 Jul 2009
  by Tom Igoe
  modified 22 Nov 2010
  by Tom Igoe
  modified 7 Nov 2016
  by Arturo Guadalupi

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/LiquidCrystalHelloWorld

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

File myFile;
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

void display_temp()
{
    lcd.setCursor(0, 1);
    lcd.print(t);   
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

void display_time()
{
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

class DataEntry 
{
    byte _year, _month, _date, _hour, _minute, _second;       
    float _temperature;
public:
    DataEntry(){};
    DataEntry(byte year, byte month, byte date, byte hour, byte minute, byte second, float temperature)
        : _year(year), _month(month), _date(date), _hour(hour), _minute(minute), _second(second), _temperature(temperature) {}
    byte getYear() {return _year;}
    byte getMonth() {return _month;}
    byte getDate() {return _date;}
    byte getHour() {return _hour;}
    byte getMinute() {return _minute;}
    byte getSecond() {return _second;}
    float getTemp() {return _temperature;}
};

void setup()
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

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

    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    myFile = SD.open("test.txt", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) 
    {
        //        Serial.print("Writing to test.txt...");
        myFile.println("testing 1, 2, 3.");
        // close the file:
        myFile.close();
        //        Serial.println("done.");
    } 
    else {
        // if the file didn't open, print an error:
        //        Serial.println("error opening test.txt");
    }

    Wire.begin();
    
    display_date();
    
//    rtc.setYear(19);
//    rtc.setMonth(03);
//    rtc.setDate(23);
//    rtc.setHour(18);
//    rtc.setMinute(06);
//    rtc.setSecond(30);
}

bool H12 = false;
bool PM = false;

DataEntry data_entries[10];
byte iDE = 0;

byte s;
byte m;
byte h;

void record_data()
{    
    data_entries[iDE] = DataEntry(year, month, date, h, m, s, t);
    iDE = (iDE + 1) % 10;
}

void write_SD()
{
    File fTempLog;
    fTempLog = SD.open("temp.log", FILE_WRITE);

    if (fTempLog) {
        for (auto e:data_entries) {
            String str = String{e.getDate()} + String{"/"} + String{e.getMonth()} + String{"/"} + String{e.getYear()} + String{"\t"} 
                       + String{e.getHour()} + String{":"} + String{e.getMinute()} + String{":"} + String{e.getSecond()} + String{"\t"} + String{e.getTemp()};
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

//    if (time - last_temp_update > 1000)
//    {
//        display_temp();
//        last_temp_update = time;
//    }

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

    call(record_data, last_record_data, 1000);
    //call(write_SD, last_SD_card_write, 10000);
    call(display_temp, last_temp_update, 1000);
    call(display_time, last_time_update, 1000);
}
