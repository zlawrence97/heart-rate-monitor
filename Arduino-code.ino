#include <SparkFunHTU21D.h>

#include "Wire.h"
#include <LiquidCrystal.h>
#define DS1307_ADDRESS 0x68
#define HTU21D_ADDRESS 0x40

char incoming;
int i, c, outlier, received = 0;
int extremePin = 8;
int pulsePin = A0;
char sent[28], date[28], snumt[2], snumb[2], snumh[2], snumm[2], snumday[2], snummonth[2], snumyr[2];
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
float temperature;

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

HTU21D myHumidity;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  interruptSetup();
  Serial.begin(9600);
  while(!Serial);
  lcd.begin(16,2);
  pinMode(extremePin, OUTPUT);
  myHumidity.begin();
}

void loop() {
  while(Serial.available() > 0){
    c = 0;
    lcd.clear();
    lcd.setCursor(0,1);
    
    readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
    //readHTU21Dtime(temperature);
    pint_to_char(BPM, hour, minute);//, temperature);
    //cint_to_char(dayOfMonth, month, year);
    
    incoming = Serial.read();
    if(incoming == 'v'){
      Serial.print('m');
      lcd.print('m');
      delay(500);
    }else if(incoming == 'z' || incoming == 'r'){
      strcat(sent, snumb);
      strcat(sent, snumh);
      strcat(sent, snumm);
      strcat(sent, "78");
      Serial.print(sent);
    }else if(incoming == 'p'){
      lcd.print(BPM);
      delay(1000*3000);
    }else if(incoming == 's'){
      char x = Serial.read();
      lcd.print(x);
      delay(1000);
    }else if(incoming == 'd'){
      strcpy(date, snumday);
      strcat(date, snummonth);
      strcat(date, snumyr);
      Serial.print(date);
    }
    
    lcd.clear();
    lcd.print(hour);
    lcd.print(":");
    lcd.print(minute);
    delay(1000);
    lcd.clear();
    for(int y = 0;y < 14;y++){
      sent[y] = '\0';
      date[y] = '\0';
    }
  }
}

void readHTU21Dtime(float temperature){
  Wire.beginTransmission(HTU21D_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(HTU21D_ADDRESS, 1);
  
  // request 1
  temperature = (myHumidity.readTemperature());
}

void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayofWeek, 
byte *dayofMonth, byte *month, byte *year){
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS, 7);
  
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year){
  // sets time and date data to DS3231
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.endTransmission();
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val){
  return( (val/10*16) + (val%10) );
}

// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val){
  return( (val/16*10) + (val%16) );
}

void pint_to_char(int beats, int hr, int mins){//, float temp){
  itoa(beats, snumb, 10);
  itoa(hr, snumh, 10);
  itoa(mins, snumm, 10);
//  itoa(temp, snumt, 10);
}

void cint_to_char(int d, int m, int y){
  itoa(d, snumday, 10);
  itoa(m, snummonth, 10);
  itoa(y, snumyr, 10);
}


