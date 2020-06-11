/*
  ET_DS3231.h - Header file for the DS3231 Real-Time Clock

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

#ifndef ET_DS3231_h
#define ET_DS3231_h

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class TimeSpan;

#define DS3231_ADDRESS              (0x68)

#define DS3231_REG_TIME             (0x00)
#define DS3231_REG_ALARM_1          (0x07)
#define DS3231_REG_ALARM_2          (0x0B)
#define DS3231_REG_CONTROL          (0x0E)
#define DS3231_REG_STATUS           (0x0F)
#define DS3231_REG_TEMPERATURE      (0x11)

#define SECONDS_PER_DAY             86400L
#define SECONDS_FROM_1970_TO_2000   946684800

//=============================================================================================
#ifndef RTCDATETIME_STRUCT_H
#define RTCDATETIME_STRUCT_H
//=============================================================================================
struct RTCDateTime
{
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  uint8_t  dayOfWeek;
  uint32_t unixtime;
};
//=============================================================================================

//=============================================================================================
struct RTCAlarmTime
{
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};
//=============================================================================================
#endif    
//=============================================================================================
// RTCDATETIME_STRUCT_H
//=============================================================================================

//=============================================================================================
typedef enum
{
  DS3231_1HZ          = 0x00,
  DS3231_4096HZ       = 0x01,
  DS3231_8192HZ       = 0x02,
  DS3231_32768HZ      = 0x03
}DS3231_sqw_t;
//=============================================================================================

//=============================================================================================
typedef enum
{
  DS3231_EVERY_SECOND   = 0b00001111,         // Alarm Every Second
  DS3231_MATCH_S        = 0b00001110,         // Alarm on (Second) is Match
  DS3231_MATCH_M_S      = 0b00001100,         // Alarm on (Minute & Second) is Match
  DS3231_MATCH_H_M_S    = 0b00001000,         // Alarm on (Hour & Minute & Second) is Match
  DS3231_MATCH_DT_H_M_S = 0b00000000,         // Alarm on (Date & Hour & Minute & Second) is Match
  DS3231_MATCH_DY_H_M_S = 0b00010000          // Alarm on (Day & Hour & Minute & Second) is Match
}DS3231_alarm1_t;
//=============================================================================================

//=============================================================================================
typedef enum
{
  DS3231_EVERY_MINUTE   = 0b00001110,         // Alarm once per minute (00 seconds of every minute)
  DS3231_MATCH_M        = 0b00001100,         // Alarm on (Minute) is Match
  DS3231_MATCH_H_M      = 0b00001000,         // Alarm on (Hour & Minute) is Match
  DS3231_MATCH_DT_H_M   = 0b00000000,         // Alarm on (Date & Hour & Minute) is Mathc
  DS3231_MATCH_DY_H_M   = 0b00010000          // Alarm on (Day & Hour & Minute) is Match
}DS3231_alarm2_t;
//=============================================================================================

//=============================================================================================
// Start of Class : DateTime
//=============================================================================================
// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime 
{
public:
  DateTime (uint32_t t =0);
  DateTime (uint16_t year, uint8_t month, uint8_t day,
            uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
  DateTime (const DateTime& copy);
  DateTime (const char* date, const char* time);
  DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time);
  uint16_t year() const       { return 2000 + yOff; }
  uint8_t month() const       { return m; }
  uint8_t day() const         { return d; }
  uint8_t hour() const        { return hh; }
  uint8_t minute() const      { return mm; }
  uint8_t second() const      { return ss; }
  uint8_t dayOfTheWeek() const;

  // 32-bit times as seconds since 1/1/2000
  long secondstime() const;  
     
  // 32-bit times as seconds since 1/1/1970
  uint32_t unixtime(void) const;

  DateTime operator+(const TimeSpan& span);
  DateTime operator-(const TimeSpan& span);
  TimeSpan operator-(const DateTime& right);

protected:
  uint8_t yOff, m, d, hh, mm, ss;
};
//=============================================================================================
// End of Class : DateTime
//=============================================================================================

//=============================================================================================
// Start of Class : TimeSpan
//=============================================================================================
// Timespan which can represent changes in time with seconds accuracy.
class TimeSpan 
{
public:
  TimeSpan (int32_t seconds = 0);
  TimeSpan (int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
  TimeSpan (const TimeSpan& copy);
  int16_t days() const         { return _seconds / 86400L; }
  int8_t  hours() const        { return _seconds / 3600 % 24; }
  int8_t  minutes() const      { return _seconds / 60 % 60; }
  int8_t  seconds() const      { return _seconds % 60; }
  int32_t totalseconds() const { return _seconds; }

  TimeSpan operator+(const TimeSpan& right);
  TimeSpan operator-(const TimeSpan& right);

protected:
  int32_t _seconds;
};
//=============================================================================================
// End of Class : TimeSpan
//=============================================================================================

//=============================================================================================
// Start of Class : ET_DS3231
//=============================================================================================
enum Ds3231SqwPinMode { DS3231_OFF = 0x01, 
                        DS3231_SquareWave1Hz = 0x00, 
                        DS3231_SquareWave1kHz = 0x08, 
                        DS3231_SquareWave4kHz = 0x10, 
                        DS3231_SquareWave8kHz = 0x18 };
//=============================================================================================
class ET_DS3231
{
  //===========================================================================================
public:
  //===========================================================================================
	bool begin(void);
  //===========================================================================================
  static void adjust(const DateTime& dt);
  bool lostPower(void);
  static DateTime now();
  static Ds3231SqwPinMode readSqwPinMode();
  static void writeSqwPinMode(Ds3231SqwPinMode mode);
  //===========================================================================================
	void setDateTime(uint16_t year, 
	                 uint8_t month, 
	                 uint8_t day, 
	                 uint8_t hour, 
	                 uint8_t minute, 
	                 uint8_t second);
	void setDateTime(uint32_t t);
	void setDateTime(const char* date, 
	                 const char* time);
	RTCDateTime getDateTime(void);
	uint8_t isReady(void);

	DS3231_sqw_t getOutput(void);
	void setOutput(DS3231_sqw_t mode);
	void enableOutput(bool enabled);
	bool isOutput(void);
	void enable32kHz(bool enabled);
	bool is32kHz(void);

	void forceConversion(void);
	float readTemperature(void);

	void setAlarm1(uint8_t dydw, 
	               uint8_t hour, 
	               uint8_t minute, 
	               uint8_t second, 
	               DS3231_alarm1_t mode, 
	               bool armed = true);
	RTCAlarmTime getAlarm1(void);
	DS3231_alarm1_t getAlarmType1(void);
	bool isAlarm1(bool clear = true);
	void armAlarm1(bool armed);
	bool isArmed1(void);
	void clearAlarm1(void);

	void setAlarm2(uint8_t dydw, 
	               uint8_t hour, 
	               uint8_t minute, 
	               DS3231_alarm2_t mode, 
	               bool armed = true);
	RTCAlarmTime getAlarm2(void);
	DS3231_alarm2_t getAlarmType2(void);
	bool isAlarm2(bool clear = true);
	void armAlarm2(bool armed);
	bool isArmed2(void);
	void clearAlarm2(void);

	void setBattery(bool timeBattery, bool squareBattery);

	char* dateFormat(const char* dateFormat, RTCDateTime dt);
	char* dateFormat(const char* dateFormat, RTCAlarmTime dt);
  //===========================================================================================
  
private:
  //===========================================================================================
	RTCDateTime t;

	char *strDayOfWeek(uint8_t dayOfWeek);
	char *strMonth(uint8_t month);
	char *strAmPm(uint8_t hour, bool uppercase);
	char *strDaySufix(uint8_t day);

	uint8_t hour12(uint8_t hour24);
	uint8_t bcd2dec(uint8_t bcd);
	uint8_t dec2bcd(uint8_t dec);

	long time2long(uint16_t days, 
	               uint8_t hours, 
	               uint8_t minutes, 
	               uint8_t seconds);
                 
	uint16_t date2days(uint16_t year, 
	                   uint8_t month, 
	                   uint8_t day);
                     
	uint8_t daysInMonth(uint16_t year, 
	                    uint8_t month);
                     
	uint16_t dayInYear(uint16_t year, 
	                   uint8_t month, 
	                   uint8_t day);
                     
	bool isLeapYear(uint16_t year);
 
	uint8_t dow(uint16_t y, 
	            uint8_t m, 
	            uint8_t d);

	uint32_t unixtime(void);
	uint8_t conv2d(const char* p);

	void writeRegister8(uint8_t reg, uint8_t value);
	uint8_t readRegister8(uint8_t reg);
  //===========================================================================================
};
//=============================================================================================
// End of Class : ET_DS3231
//=============================================================================================

#endif
