/*
  DS3231.cpp - Class file for the DS3231 Real-Time Clock

  Version: 1.0.1
  (c) 2014 Korneliusz Jarzebski
  www.jarzebski.pl

  This program is free software: you can redistribute it and/or modify
  it under the terms of the version 3 GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
#include "ET_DS3231.h"

const uint8_t daysArray [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };
const uint8_t dowArray[] PROGMEM   = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

//=============================================================================================
#if (ARDUINO >= 100)
  #include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems

  // Macro to deal with the difference in I2C write functions from old and new Arduino versions.
  #define _I2C_WRITE write
  #define _I2C_READ  read
#else
  #include <WProgram.h>
  #define _I2C_WRITE send
  #define _I2C_READ  receive
#endif

static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) 
{
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom(addr, (byte)1);
  return Wire._I2C_READ();
}

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) 
{
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire._I2C_WRITE((byte)val);
  Wire.endTransmission();
}
//=============================================================================================


//=============================================================================================
// Start of Class : DateTime
//=============================================================================================

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed
const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) 
{
  if (y >= 2000)
      y -= 2000;
  uint16_t days = d;
  for (uint8_t i = 1; i < m; ++i)
    days += pgm_read_byte(daysInMonth + i - 1);
  if (m > 2 && y % 4 == 0)
    ++days;
  return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) 
{
  return ((days * 24L + h) * 60 + m) * 60 + s;
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second
DateTime::DateTime (uint32_t t) 
{
  t -= SECONDS_FROM_1970_TO_2000;    // bring to 2000 timestamp from 1970

  ss = t % 60;
  t /= 60;
  mm = t % 60;
  t /= 60;
  hh = t % 24;
  uint16_t days = t / 24;
  uint8_t leap;
  for (yOff = 0; ; ++yOff) 
  {
        leap = yOff % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
  }
  for (m = 1; ; ++m) 
  {
    uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
    if(leap && m == 2)
      ++daysPerMonth;
    if (days < daysPerMonth)
      break;
    days -= daysPerMonth;
  }
  d = days + 1;
}

DateTime::DateTime (uint16_t year, 
                    uint8_t month, 
                    uint8_t day, 
                    uint8_t hour, 
                    uint8_t min, 
                    uint8_t sec) 
{
  if(year >= 2000)
     year -= 2000;
  yOff = year;
  m = month;
  d = day;
  hh = hour;
  mm = min;
  ss = sec;
}

DateTime::DateTime (const DateTime& copy):
  yOff(copy.yOff),
  m(copy.m),
  d(copy.d),
  hh(copy.hh),
  mm(copy.mm),
  ss(copy.ss)
{}

static uint8_t conv2d(const char* p) 
{
  uint8_t v = 0;
  if('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using F() would further reduce the RAM footprint, see below.
DateTime::DateTime (const char* date, const char* time) 
{
  // sample input: date = "Dec 26 2009", time = "12:34:56"
  yOff = conv2d(date + 9);
  
  // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
  switch (date[0]) 
  {
    case 'J': m = (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7); break;
    case 'F': m = 2; break;
    case 'A': m = date[2] == 'r' ? 4 : 8; break;
    case 'M': m = date[2] == 'r' ? 3 : 5; break;
    case 'S': m = 9; break;
    case 'O': m = 10; break;
    case 'N': m = 11; break;
    case 'D': m = 12; break;
  }
  d = conv2d(date + 4);
  hh = conv2d(time);
  mm = conv2d(time + 3);
  ss = conv2d(time + 6);
}

// A convenient constructor for using "the compiler's time":
// This version will save RAM by using PROGMEM to store it by using the F macro.
//   DateTime now (F(__DATE__), F(__TIME__));
DateTime::DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time) 
{
  // sample input: date = "Dec 26 2009", time = "12:34:56"
  char buff[11];
  memcpy_P(buff, date, 11);
  yOff = conv2d(buff + 9);
  
  // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
  switch (buff[0]) 
  {
    case 'J': m = (buff[1] == 'a') ? 1 : ((buff[2] == 'n') ? 6 : 7); break;
    case 'F': m = 2; break;
    case 'A': m = buff[2] == 'r' ? 4 : 8; break;
    case 'M': m = buff[2] == 'r' ? 3 : 5; break;
    case 'S': m = 9; break;
    case 'O': m = 10; break;
    case 'N': m = 11; break;
    case 'D': m = 12; break;
  }
  d = conv2d(buff + 4);
  memcpy_P(buff, time, 8);
  hh = conv2d(buff);
  mm = conv2d(buff + 3);
  ss = conv2d(buff + 6);
}

uint8_t DateTime::dayOfTheWeek() const 
{    
  uint16_t day = date2days(yOff, m, d);
  return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

uint32_t DateTime::unixtime(void) const 
{
  uint32_t t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}

long DateTime::secondstime(void) const 
{
  long t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  return t;
}

DateTime DateTime::operator+(const TimeSpan& span) 
{
  return DateTime(unixtime()+span.totalseconds());
}

DateTime DateTime::operator-(const TimeSpan& span) 
{
  return DateTime(unixtime()-span.totalseconds());
}

TimeSpan DateTime::operator-(const DateTime& right) 
{
  return TimeSpan(unixtime()-right.unixtime());
}
//=============================================================================================
// End of Class : DateTime
//=============================================================================================

//=============================================================================================
// Start of Class : TimeSpan
// TimeSpan implementation
//=============================================================================================
TimeSpan::TimeSpan (int32_t seconds):
  _seconds(seconds)
{
}

TimeSpan::TimeSpan (int16_t days, int8_t hours, int8_t minutes, int8_t seconds):
  _seconds((int32_t)days*86400L + (int32_t)hours*3600 + (int32_t)minutes*60 + seconds)
{
}

TimeSpan::TimeSpan (const TimeSpan& copy):
  _seconds(copy._seconds)
{
}

TimeSpan TimeSpan::operator+(const TimeSpan& right) 
{
  return TimeSpan(_seconds+right._seconds);
}

TimeSpan TimeSpan::operator-(const TimeSpan& right) 
{
  return TimeSpan(_seconds-right._seconds);
}
//=============================================================================================
// End of Class : TimeSpan
//=============================================================================================

//=============================================================================================
// Start of Class : ET_DS3231
//=============================================================================================
bool ET_DS3231::begin(void)
{
  Wire.begin();

  setBattery(true, false);

  t.year = 2000;              
  t.month = 1;
  t.day = 1;
  t.hour = 0;
  t.minute = 0;
  t.second = 0;
  t.dayOfWeek = 6;
  t.unixtime = 946681200;

  return true;
}

//=============================================================================================
// Start of Copy From : RTClib.cpp
//=============================================================================================
static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }
//=============================================================================================
bool ET_DS3231::lostPower(void) 
{
  return (read_i2c_register(DS3231_ADDRESS,DS3231_REG_STATUS) >> 7);                         // Oscillator Stop Flag (OSF).
}

void ET_DS3231::adjust(const DateTime& dt) 
{
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(0));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();

  uint8_t statreg = read_i2c_register(DS3231_ADDRESS, DS3231_REG_STATUS);
  statreg &= ~0x80; // flip OSF bit
  write_i2c_register(DS3231_ADDRESS, DS3231_REG_STATUS, statreg);
}

DateTime ET_DS3231::now() 
{
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)0);  
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ();
  uint8_t d = bcd2bin(Wire._I2C_READ());
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (y, m, d, hh, mm, ss);
}

Ds3231SqwPinMode ET_DS3231::readSqwPinMode() 
{
  int mode;

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE(DS3231_REG_CONTROL);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x93;
  return static_cast<Ds3231SqwPinMode>(mode);
}

void ET_DS3231::writeSqwPinMode(Ds3231SqwPinMode mode) 
{
  uint8_t ctrl;
  ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_REG_CONTROL);

  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  if (mode == DS3231_OFF) 
  {
    ctrl |= 0x04; // turn on INTCN
  } 
  else 
  {
    ctrl |= mode;
  } 
  write_i2c_register(DS3231_ADDRESS, DS3231_REG_CONTROL, ctrl);

  //Serial.println( read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL), HEX);
}
//=============================================================================================
// End of Copy From : RTClib.cpp
//=============================================================================================

void ET_DS3231::setDateTime(uint16_t year, 
                            uint8_t month, 
                            uint8_t day, 
                            uint8_t hour, 
                            uint8_t minute, 
                            uint8_t second)
{
  Wire.beginTransmission(DS3231_ADDRESS);

  #if ARDUINO >= 100
    Wire.write(DS3231_REG_TIME);
  #else
    Wire.send(DS3231_REG_TIME);
  #endif

  #if ARDUINO >= 100
    Wire.write(dec2bcd(second));
    Wire.write(dec2bcd(minute));
    Wire.write(dec2bcd(hour));
    Wire.write(dec2bcd(dow(year, month, day)));
    Wire.write(dec2bcd(day));
    Wire.write(dec2bcd(month));
    Wire.write(dec2bcd(year-2000));
  #else
    Wire.send(dec2bcd(second));
    Wire.send(dec2bcd(minute));
    Wire.send(dec2bcd(hour));
    Wire.send(dec2bcd(dow(year, month, day)));
    Wire.send(dec2bcd(day));
    Wire.send(dec2bcd(month));
    Wire.send(dec2bcd(year-2000));
  #endif

  #if ARDUINO >= 100
    Wire.write(DS3231_REG_TIME);
  #else
    Wire.send(DS3231_REG_TIME);
  #endif

  Wire.endTransmission();
}


void ET_DS3231::setDateTime(uint32_t t)
{
  t -= 946681200;

  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;

  second = t % 60;
  t /= 60;

  minute = t % 60;
  t /= 60;

  hour = t % 24;
  uint16_t days = t / 24;
  uint8_t leap;

  for(year = 0; ; ++year)
  {
    leap = year % 4 == 0;
    if (days < 365 + leap)
    {
      break;
    }
    days -= 365 + leap;
  }

  for (month = 1; ; ++month)
  {
    uint8_t daysPerMonth = pgm_read_byte(daysArray + month - 1);

    if (leap && month == 2)
    {
      ++daysPerMonth;
    }

    if (days < daysPerMonth)
    {
      break;
    }
    days -= daysPerMonth;
  }
  
  day = days + 1;
  setDateTime(year+2000, month, day, hour, minute, second);
}

void ET_DS3231::setDateTime(const char* date, const char* time)
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;

  year = conv2d(date + 9);

  switch (date[0])
  {
    case 'J': month = date[1] == 'a' ? 1 : month = date[2] == 'n' ? 6 : 7; break;
    case 'F': month = 2; break;
    case 'A': month = date[2] == 'r' ? 4 : 8; break;
    case 'M': month = date[2] == 'r' ? 3 : 5; break;
    case 'S': month = 9; break;
    case 'O': month = 10; break;
    case 'N': month = 11; break;
    case 'D': month = 12; break;
  }

  day = conv2d(date + 4);
  hour = conv2d(time);
  minute = conv2d(time + 3);
  second = conv2d(time + 6);

  setDateTime(year+2000, month, day, hour, minute, second);
}

char* ET_DS3231::dateFormat(const char* dateFormat, RTCDateTime dt)
{
  char buffer[255];

  buffer[0] = 0;

  char helper[11];

  while (*dateFormat != '\0')
  {
    switch (dateFormat[0])
    {
      // Day decoder
      case 'd':
        sprintf(helper, "%02d", dt.day); 
        strcat(buffer, (const char *)helper); 
      break;
      
      case 'j':
        sprintf(helper, "%d", dt.day);
        strcat(buffer, (const char *)helper);
      break;
      
      case 'l':
        strcat(buffer, (const char *)strDayOfWeek(dt.dayOfWeek));
      break;
      
      case 'D':
        strncat(buffer, strDayOfWeek(dt.dayOfWeek), 3);
      break;
      
      case 'N':
        sprintf(helper, "%d", dt.dayOfWeek);
        strcat(buffer, (const char *)helper);
      break;
      
      case 'w':
        sprintf(helper, "%d", (dt.dayOfWeek + 7) % 7);
        strcat(buffer, (const char *)helper);
      break;
      
      case 'z':
        sprintf(helper, "%d", dayInYear(dt.year, dt.month, dt.day));
        strcat(buffer, (const char *)helper);
      break;
      
      case 'S':
        strcat(buffer, (const char *)strDaySufix(dt.day));
      break;

      // Month decoder
      case 'm':
        sprintf(helper, "%02d", dt.month);
        strcat(buffer, (const char *)helper);
      break;
      
      case 'n':
        sprintf(helper, "%d", dt.month);
        strcat(buffer, (const char *)helper);
      break;
      
      case 'F':
        strcat(buffer, (const char *)strMonth(dt.month));
      break;
      
      case 'M':
        strncat(buffer, (const char *)strMonth(dt.month), 3);
      break;
      
      case 't':
        sprintf(helper, "%d", daysInMonth(dt.year, dt.month));
        strcat(buffer, (const char *)helper);
      break;

      // Year decoder
      case 'Y':
        sprintf(helper, "%d", dt.year); 
        strcat(buffer, (const char *)helper); 
      break;
      
      case 'y': sprintf(helper, "%02d", dt.year-2000);
        strcat(buffer, (const char *)helper);
      break;
      
      case 'L':
        sprintf(helper, "%d", isLeapYear(dt.year)); 
        strcat(buffer, (const char *)helper); 
      break;

      // Hour decoder
      case 'H':
        sprintf(helper, "%02d", dt.hour);
        strcat(buffer, (const char *)helper);
      break;
      
      case 'G':
        sprintf(helper, "%d", dt.hour);
        strcat(buffer, (const char *)helper);
      break;
      case 'h':
        sprintf(helper, "%02d", hour12(dt.hour));
        strcat(buffer, (const char *)helper);
      break;
      
      case 'g':
        sprintf(helper, "%d", hour12(dt.hour));
        strcat(buffer, (const char *)helper);
      break;
      
      case 'A':
        strcat(buffer, (const char *)strAmPm(dt.hour, true));
      break;
      
      case 'a':
        strcat(buffer, (const char *)strAmPm(dt.hour, false));
      break;

      // Minute decoder
      case 'i': 
        sprintf(helper, "%02d", dt.minute);
        strcat(buffer, (const char *)helper);
      break;

      // Second decoder
      case 's':
        sprintf(helper, "%02d", dt.second); 
        strcat(buffer, (const char *)helper); 
      break;

      // Misc decoder
      case 'U': 
        sprintf(helper, "%lu", dt.unixtime);
        strcat(buffer, (const char *)helper);
      break;

      default: 
        strncat(buffer, dateFormat, 1);
      break;
    }
    dateFormat++;
  }
  return buffer;
}

char* ET_DS3231::dateFormat(const char* dateFormat, RTCAlarmTime dt)
{
  char buffer[255];
  buffer[0] = 0;

  char helper[11];

  while (*dateFormat != '\0')
  {
    switch (dateFormat[0])
    {
      // Day decoder
      case 'd':
        sprintf(helper, "%02d", dt.day); 
        strcat(buffer, (const char *)helper); 
      break;
      case 'j':
        sprintf(helper, "%d", dt.day);
        strcat(buffer, (const char *)helper);
      break;
      case 'l':
        strcat(buffer, (const char *)strDayOfWeek(dt.day));
        break;
      case 'D':
        strncat(buffer, strDayOfWeek(dt.day), 3);
      break;
      case 'N':
        sprintf(helper, "%d", dt.day);
        strcat(buffer, (const char *)helper);
      break;
      case 'w':
        sprintf(helper, "%d", (dt.day + 7) % 7);
        strcat(buffer, (const char *)helper);
      break;
      case 'S':
        strcat(buffer, (const char *)strDaySufix(dt.day));
      break;

      // Hour decoder
      case 'H':
        sprintf(helper, "%02d", dt.hour);
        strcat(buffer, (const char *)helper);
      break;
      case 'G':
        sprintf(helper, "%d", dt.hour);
        strcat(buffer, (const char *)helper);
      break;
      case 'h':
        sprintf(helper, "%02d", hour12(dt.hour));
        strcat(buffer, (const char *)helper);
      break;
      case 'g':
        sprintf(helper, "%d", hour12(dt.hour));
        strcat(buffer, (const char *)helper);
      break;
      case 'A':
        strcat(buffer, (const char *)strAmPm(dt.hour, true));
      break;
      case 'a':
        strcat(buffer, (const char *)strAmPm(dt.hour, false));
      break;

      // Minute decoder
      case 'i': 
        sprintf(helper, "%02d", dt.minute);
        strcat(buffer, (const char *)helper);
      break;

      // Second decoder
      case 's':
        sprintf(helper, "%02d", dt.second); 
        strcat(buffer, (const char *)helper); 
      break;

      default: 
        strncat(buffer, dateFormat, 1);
      break;
    }
    dateFormat++;
  }
  return buffer;
}

RTCDateTime ET_DS3231::getDateTime(void)
{
  int values[7];

  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(DS3231_REG_TIME);
  #else
    Wire.send(DS3231_REG_TIME);
  #endif
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 7);

  while(!Wire.available()) {};

  for (int i = 6; i >= 0; i--)
  {
    #if ARDUINO >= 100
      values[i] = bcd2dec(Wire.read());
    #else
      values[i] = bcd2dec(Wire.receive());
    #endif
  }

  Wire.endTransmission();

  t.year = values[0] + 2000;
  t.month = values[1];
  t.day = values[2];
  t.dayOfWeek = values[3];
  t.hour = values[4];
  t.minute = values[5];
  t.second = values[6];
  t.unixtime = unixtime();

  return t;
}

uint8_t ET_DS3231::isReady(void) 
{
  return true;
}

/***************************************
 * Enable Output on INT#/SWQ Pin
 */
void ET_DS3231::enableOutput(bool enabled)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_CONTROL);

  value &= 0b11111011;
  value |= (!enabled << 2);

  writeRegister8(DS3231_REG_CONTROL, value);
}

void ET_DS3231::setBattery(bool timeBattery, bool squareBattery)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_CONTROL);
  //===========================================================================================
  // BBSQW
  // -> 0 : 
  // -> 1 : 
  //===========================================================================================
  if(squareBattery)
  {
    value |= 0b01000000;
  } 
  else
  {
    value &= 0b10111111;
  }
  //===========================================================================================
  
  //===========================================================================================
  // EOSC
  // -> 0 : 
  // -> 1 : 
  //===========================================================================================
  if (timeBattery)
  {
    value &= 0b01111011;
  } 
  else
  {
    value |= 0b10000000;
  }
  //===========================================================================================
  writeRegister8(DS3231_REG_CONTROL, value);
  //===========================================================================================
}

bool ET_DS3231::isOutput(void)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_CONTROL);

  value &= 0b00000100;
  value >>= 2;                                                                               // Interrupt Control (INTCN).

  return !value;
}

void ET_DS3231::setOutput(DS3231_sqw_t mode)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_CONTROL);

  value &= 0b11100111;
  value |= (mode << 3);

  writeRegister8(DS3231_REG_CONTROL, value);
}

DS3231_sqw_t ET_DS3231::getOutput(void)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_CONTROL);

  value &= 0b00011000;
  value >>= 3;                                              // RS2:RS1 = SQUARE-WAVE OUTPUT FREQUENCY

  return (DS3231_sqw_t)value;
}

void ET_DS3231::enable32kHz(bool enabled)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_STATUS);

  value &= 0b11110111;
  value |= (enabled << 3);                                  // Enable 32kHz Output (EN32kHz).

  writeRegister8(DS3231_REG_STATUS, value);
}

bool ET_DS3231::is32kHz(void)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_STATUS);

  value &= 0b00001000;
  value >>= 3;                                             // Enable 32kHz Output (EN32kHz).
  return value;
}

void ET_DS3231::forceConversion(void)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_CONTROL);

  value |= 0b00100000;

  writeRegister8(DS3231_REG_CONTROL, value);
  do {} while ((readRegister8(DS3231_REG_CONTROL) & 0b00100000) != 0);
}

float ET_DS3231::readTemperature(void)
{
  uint8_t msb, lsb;

  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(DS3231_REG_TEMPERATURE);
  #else
    Wire.send(DS3231_REG_TEMPERATURE);
  #endif
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 2);

  while(!Wire.available()) {};

  #if ARDUINO >= 100
    msb = Wire.read();
    lsb = Wire.read();
  #else
    msb = Wire.receive();
    lsb = Wire.receive();
  #endif

  return ((((short)msb << 8) | (short)lsb) >> 6) / 4.0f;
}

RTCAlarmTime ET_DS3231::getAlarm1(void)
{
  uint8_t values[4];
  RTCAlarmTime a;

  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(DS3231_REG_ALARM_1);
  #else
    Wire.send(DS3231_REG_ALARM_1);
  #endif
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 4);

  while(!Wire.available()) {};

  for (int i = 3; i >= 0; i--)
  {
    #if ARDUINO >= 100
      values[i] = bcd2dec(Wire.read() & 0b01111111);
    #else
      values[i] = bcd2dec(Wire.receive() & 0b01111111);
    #endif
  }
    
  Wire.endTransmission();
    
  a.day = values[0];
  a.hour = values[1];
  a.minute = values[2];
  a.second = values[3];
    
  return a;
}

DS3231_alarm1_t ET_DS3231::getAlarmType1(void)
{
  uint8_t values[4];
  uint8_t mode = 0;

  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(DS3231_REG_ALARM_1);
  #else
    Wire.send(DS3231_REG_ALARM_1);
  #endif
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 4);

  while(!Wire.available()) {};

  for (int i = 3; i >= 0; i--)
  {
    #if ARDUINO >= 100
      values[i] = bcd2dec(Wire.read());
    #else
      values[i] = bcd2dec(Wire.receive());
    #endif
  }

  Wire.endTransmission();

  mode |= ((values[3] & 0b01000000) >> 6);
  mode |= ((values[2] & 0b01000000) >> 5);
  mode |= ((values[1] & 0b01000000) >> 4);
  mode |= ((values[0] & 0b01000000) >> 3);
  mode |= ((values[0] & 0b00100000) >> 1);

  return (DS3231_alarm1_t)mode;
}

void ET_DS3231::setAlarm1(uint8_t dydw,               // Day/Date
                          uint8_t hour,               // Hour
                          uint8_t minute,             // Minute
                          uint8_t second,             // Second
                          DS3231_alarm1_t mode,       // Mode
                          bool armed)                 // 
{
  second = dec2bcd(second);                           // BCD Second
  minute = dec2bcd(minute);                           // BCD Minute
  hour   = dec2bcd(hour);                             // BCD Hour
  dydw   = dec2bcd(dydw);                             // BCD Day/Date

  //=============================================================================================
  // DS3231_EVERY_SECOND   = Alarm Every Second
  // DS3231_MATCH_S        = Alarm on (Second) is Match
  // DS3231_MATCH_M_S      = Alarm on (Minute & Second) is Match
  // DS3231_MATCH_H_M_S    = Alarm on (Hour & Minute & Second) is Match
  // DS3231_MATCH_DT_H_M_S = Alarm on (Date & Hour & Minute & Second) is Match
  // DS3231_MATCH_DY_H_M_S = Alarm on (Day & Hour & Minute & Second) is Match
  //=============================================================================================
  // DS3231_EVERY_MINUTE   = Alarm once per minute (00 seconds of every minute)
  // DS3231_MATCH_M        = Alarm on (Minute) is Match
  // DS3231_MATCH_H_M      = Alarm on (Hour & Minute) is Match
  // DS3231_MATCH_DT_H_M   = Alarm on (Date & Hour & Minute) is Mathc
  // DS3231_MATCH_DY_H_M   = Alarm on (Day & Hour & Minute) is Match
  //=============================================================================================
  switch(mode)
  {
    case DS3231_EVERY_SECOND:
      second |= 0b10000000;
      minute |= 0b10000000;
      hour   |= 0b10000000;
      dydw   |= 0b10000000;
    break;

    case DS3231_MATCH_S:
      second &= 0b01111111;
      minute |= 0b10000000;
      hour   |= 0b10000000;
      dydw   |= 0b10000000;
    break;

    case DS3231_MATCH_M_S:
      second &= 0b01111111;
      minute &= 0b01111111;
      hour   |= 0b10000000;
      dydw   |= 0b10000000;
    break;

    case DS3231_MATCH_H_M_S:
      second &= 0b01111111;
      minute &= 0b01111111;
      hour   &= 0b01111111;
      dydw   |= 0b10000000;
    break;

    case DS3231_MATCH_DT_H_M_S:
      second &= 0b01111111;
      minute &= 0b01111111;
      hour   &= 0b01111111;
      dydw   &= 0b01111111;
    break;

    case DS3231_MATCH_DY_H_M_S:
      second &= 0b01111111;
      minute &= 0b01111111;
      hour   &= 0b01111111;
      dydw   &= 0b01111111;
      dydw   |= 0b01000000;
    break;
  }

  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(DS3231_REG_ALARM_1);
    Wire.write(second);
    Wire.write(minute);
    Wire.write(hour);
    Wire.write(dydw);
  #else
    Wire.send(DS3231_REG_ALARM_1);
    Wire.send(second);
    Wire.send(minute);
    Wire.send(hour);
    Wire.send(dydw);
  #endif

  Wire.endTransmission();

  armAlarm1(armed);

  clearAlarm1();
}

bool ET_DS3231::isAlarm1(bool clear)
{
  uint8_t alarm;

  alarm = readRegister8(DS3231_REG_STATUS);
  alarm &= 0b00000001;

  if (alarm && clear)
  {
    clearAlarm1();
  }

  return alarm;
}

void ET_DS3231::armAlarm1(bool armed)
{
  uint8_t value;
  
  value = readRegister8(DS3231_REG_CONTROL);

  if(armed)
  {
    value |= 0b00000001;          // Enable : Alarm 1 Interrupt Enable (A1IE)
  } 
  else
  {
    value &= 0b11111110;          // Disable : Alarm 1 Interrupt Enable (A1IE)
  }

  writeRegister8(DS3231_REG_CONTROL, value);
}

bool ET_DS3231::isArmed1(void)
{
  uint8_t value;
  value = readRegister8(DS3231_REG_CONTROL);
  value &= 0b00000001;
  return value;
}

void ET_DS3231::clearAlarm1(void)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_STATUS);
  value &= 0b11111110;

  writeRegister8(DS3231_REG_STATUS, value);
}

RTCAlarmTime ET_DS3231::getAlarm2(void)
{
  uint8_t values[3];
  RTCAlarmTime a;

  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(DS3231_REG_ALARM_2);
  #else
    Wire.send(DS3231_REG_ALARM_2);
  #endif
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 3);

  while(!Wire.available()) {};

  for(int i = 2; i >= 0; i--)
  {
    #if ARDUINO >= 100
      values[i] = bcd2dec(Wire.read() & 0b01111111);
    #else
      values[i] = bcd2dec(Wire.receive() & 0b01111111);
    #endif
  }

  Wire.endTransmission();

  a.day    = values[0];
  a.hour   = values[1];
  a.minute = values[2];
  a.second = 0;

  return a;
}

DS3231_alarm2_t ET_DS3231::getAlarmType2(void)
{
  uint8_t values[3];
  uint8_t mode = 0;

  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(DS3231_REG_ALARM_2);
  #else
    Wire.send(DS3231_REG_ALARM_2);
  #endif
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 3);

  while(!Wire.available()) {};

  for (int i = 2; i >= 0; i--)
  {
    #if ARDUINO >= 100
      values[i] = bcd2dec(Wire.read());
    #else
      values[i] = bcd2dec(Wire.receive());
    #endif
  }

  Wire.endTransmission();

  mode |= ((values[2] & 0b01000000) >> 5);
  mode |= ((values[1] & 0b01000000) >> 4);
  mode |= ((values[0] & 0b01000000) >> 3);
  mode |= ((values[0] & 0b00100000) >> 1);

  return (DS3231_alarm2_t)mode;
}

void ET_DS3231::setAlarm2(uint8_t dydw, 
                          uint8_t hour, 
                          uint8_t minute, 
                          DS3231_alarm2_t mode, 
                          bool armed)
{
  minute = dec2bcd(minute);
  hour   = dec2bcd(hour);
  dydw   = dec2bcd(dydw);

  switch(mode)
  {
    case DS3231_EVERY_MINUTE:
      minute |= 0b10000000;
      hour   |= 0b10000000;
      dydw   |= 0b10000000;
    break;

    case DS3231_MATCH_M:
      minute &= 0b01111111;
      hour   |= 0b10000000;
      dydw   |= 0b10000000;
    break;

    case DS3231_MATCH_H_M:
      minute &= 0b01111111;
      hour   &= 0b01111111;
      dydw   |= 0b10000000;
    break;

    case DS3231_MATCH_DT_H_M:
      minute &= 0b01111111;
      hour   &= 0b01111111;
      dydw   &= 0b01111111;
    break;

    case DS3231_MATCH_DY_H_M:
      minute &= 0b01111111;
      hour   &= 0b01111111;
      dydw   &= 0b01111111;
      dydw   |= 0b01000000;
    break;
  }

  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(DS3231_REG_ALARM_2);
    Wire.write(minute);
    Wire.write(hour);
    Wire.write(dydw);
  #else
    Wire.send(DS3231_REG_ALARM_2);
    Wire.send(minute);
    Wire.send(hour);
    Wire.send(dydw);
  #endif

  Wire.endTransmission();

  armAlarm2(armed);
  clearAlarm2();
}

void ET_DS3231::armAlarm2(bool armed)
{
  uint8_t value;
  value = readRegister8(DS3231_REG_CONTROL);

  if(armed)
  {
    value |= 0b00000010;
  } 
  else
  {
    value &= 0b11111101;
  }

  writeRegister8(DS3231_REG_CONTROL, value);
}

/*********************************
 * Verify Alarm[2] Status   
 *********************************
 */
bool ET_DS3231::isArmed2(void)
{
  uint8_t value;
  value = readRegister8(DS3231_REG_CONTROL);
  value &= 0b00000010;
  value >>= 1;
  return value;
}


void ET_DS3231::clearAlarm2(void)
{
  uint8_t value;

  value = readRegister8(DS3231_REG_STATUS);
  value &= 0b11111101;

  writeRegister8(DS3231_REG_STATUS, value);
}

/*********************************
 * Verify Alarm[2] & Reset Alarm   
 *********************************
 */
bool ET_DS3231::isAlarm2(bool clear)
{
  uint8_t alarm;

  alarm = readRegister8(DS3231_REG_STATUS);
  alarm &= 0b00000010;

  if (alarm && clear)
  {
    clearAlarm2();
  }

  return alarm;
}

/***************************
 * Convert 2BCD to Decimal *
 **************************/
uint8_t ET_DS3231::bcd2dec(uint8_t bcd)
{
  return ((bcd / 16) * 10) + (bcd % 16);
}

/***************************
 * Convert Decimal to 2BCD *
 **************************/
uint8_t ET_DS3231::dec2bcd(uint8_t dec)
{
  return ((dec / 10) * 16) + (dec % 10);
}

char *ET_DS3231::strDayOfWeek(uint8_t dayOfWeek)
{
  switch (dayOfWeek) 
  {
    case 1:
      return "Monday";
    break;
    
    case 2:
      return "Tuesday";
    break;
    
    case 3:
      return "Wednesday";
    break;
    
    case 4:
      return "Thursday";
    break;
    
    case 5:
      return "Friday";
    break;
    
    case 6:
      return "Saturday";
    break;
    
    case 7:
      return "Sunday";
    break;
    
    default:
      return "Unknown";
  }
}

char *ET_DS3231::strMonth(uint8_t month)
{
  switch (month) 
  {
    case 1:
      return "January";
    break;
    case 2:
      return "February";
    break;
    case 3:
      return "March";
    break;
    case 4:
      return "April";
    break;
    case 5:
      return "May";
    break;
    case 6:
      return "June";
    break;
    case 7:
      return "July";
    break;
    case 8:
      return "August";
    break;
    case 9:
      return "September";
    break;
    case 10:
      return "October";
    break;
    case 11:
      return "November";
    break;
    case 12:
      return "December";
    break;
    default:
      return "Unknown";
  }
}

char *ET_DS3231::strAmPm(uint8_t hour, bool uppercase)
{
  if(hour < 12)
  {
    if(uppercase)
    {
      return "AM";
    } 
    else
    {
      return "am";
    }
  } 
  else
  {
    if(uppercase)
    {
      return "PM";
    } 
    else
    {
      return "pm";
    }
  }
}

char *ET_DS3231::strDaySufix(uint8_t day)
{
  if(day % 10 == 1)
  {
    return "st";
  } 
  else
  if(day % 10 == 2)
  {
    return "nd";
  }
  if(day % 10 == 3)
  {
    return "rd";
  }

  return "th";
}

uint8_t ET_DS3231::hour12(uint8_t hour24)
{
  if (hour24 == 0)
  {
    return 12;
  }

  if(hour24 > 12)
  {
    return (hour24 - 12);
  }

  return hour24;
}

long ET_DS3231::time2long(uint16_t days, 
                          uint8_t hours, 
                          uint8_t minutes, 
                          uint8_t seconds)
{
  return ((days * 24L + hours) * 60 + minutes) * 60 + seconds;
}

uint16_t ET_DS3231::dayInYear(uint16_t year, uint8_t month, uint8_t day)
{
  uint16_t fromDate;
  uint16_t toDate;

  fromDate = date2days(year, 1, 1);
  toDate = date2days(year, month, day);

  return (toDate - fromDate);
}

bool ET_DS3231::isLeapYear(uint16_t year)
{
  return (year % 4 == 0);
}

uint8_t ET_DS3231::daysInMonth(uint16_t year, 
                               uint8_t month)
{
  uint8_t days;

  days = pgm_read_byte(daysArray + month - 1);

  if ((month == 2) && isLeapYear(year))
  {
    ++days;
  }

  return days;
}

uint16_t ET_DS3231::date2days(uint16_t year, 
                              uint8_t month, 
                              uint8_t day)
{
  year = year - 2000;

  uint16_t days16 = day;

  for (uint8_t i = 1; i < month; ++i)
  {
    days16 += pgm_read_byte(daysArray + i - 1);
  }

  if ((month == 2) && isLeapYear(year))
  {
    ++days16;
  }

  return days16 + 365 * year + (year + 3) / 4 - 1;
}

uint32_t ET_DS3231::unixtime(void)
{
  uint32_t u;

  u = time2long(date2days(t.year, 
                          t.month, 
                          t.day), 
                          t.hour, 
                          t.minute, 
                          t.second);
  u += 946681200;

  return u;
}

uint8_t ET_DS3231::conv2d(const char* p)
{
  uint8_t v = 0;

  if ('0' <= *p && *p <= '9')
  {
    v = *p - '0';
  }

  return 10 * v + *++p - '0';
}

uint8_t ET_DS3231::dow(uint16_t y, 
                       uint8_t m, 
                       uint8_t d)
{
  uint8_t dow;

  y -= m < 3;
  dow = ((y + y/4 - y/100 + y/400 + pgm_read_byte(dowArray+(m-1)) + d) % 7);

  if (dow == 0)
  {
    return 7;
  }

  return dow;
}

void ET_DS3231::writeRegister8(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(DS3231_ADDRESS);
  
  #if ARDUINO >= 100
    Wire.write(reg);
    Wire.write(value);
  #else
    Wire.send(reg);
    Wire.send(value);
  #endif
  
  Wire.endTransmission();
}

uint8_t ET_DS3231::readRegister8(uint8_t reg)
{
  uint8_t value;
  
  Wire.beginTransmission(DS3231_ADDRESS);
  #if ARDUINO >= 100
    Wire.write(reg);
  #else
    Wire.send(reg);
  #endif
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 1);
  while(!Wire.available()) {};
  
  #if ARDUINO >= 100
    value = Wire.read();
  #else
    value = Wire.receive();
  #endif;
  
  Wire.endTransmission();

  return value;
}
//=============================================================================================
// End of Class : ET_DS3231
//=============================================================================================

