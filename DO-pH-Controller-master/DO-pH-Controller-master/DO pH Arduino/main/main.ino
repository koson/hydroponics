#include <millisDelay.h>
#include <loopTimer.h>
#include <Event.h>
#include <Wire.h> 
#include "DS2482.h"
#include <MCP3424.h>                        // MCP3424 Library
#include "EEPROM.h"
#include "profiles.h"
#include "ETT_PCF8574.h"
#include "ET_DS3231.h"
#include "FS.h"
#include "SD.h"
#include "Timer.h"
#include "esp_system.h"


Timer tSD;


/*
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>*/

const int wdtTimeout = 10000;  //time in ms to trigger the watchdog
hw_timer_t *timerwdt = NULL;

#define EEPROM_SIZE 64

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

ETT_PCF8574 master_relay(PCF8574_ID_DEV0);                                                        // ET-ESP32-RS485  : Output Relay(PCF8574:ID0)
ETT_PCF8574 master_input(PCF8574A_ID_DEV0);                                                       // ET-ESP32-RS485  : Input OPTO(PCF8574A:ID0) 


ET_DS3231 myRTC;
DateTime myTimeNow;

float UTCOffset = +7.0;    // Your timezone relative to UTC (http://en.wikipedia.org/wiki/UTC_offset)
char daysOfTheWeek[8][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

unsigned long lastSecondTime = 0;

const float SaturationValueTab[41] PROGMEM = {      //saturation dissolved oxygen concentrations at various temperatures   0 - 40 Celcius
14.46, 14.22, 13.82, 13.44, 13.09,
12.74, 12.42, 12.11, 11.81, 11.53,
11.26, 11.01, 10.77, 10.53, 10.30,
10.08, 9.86,  9.66,  9.46,  9.27,
9.08,  8.90,  8.73,  8.57,  8.41,
8.25,  8.11,  7.96,  7.82,  7.69,
7.56,  7.43,  7.30,  7.18,  7.07,
6.95,  6.84,  6.73,  6.63,  6.53,
6.41,
};

//=================================================================================================
// Start of Default Hardware : ET-ESP32-RS485
//=================================================================================================
// Remap Pin USART -> C:\Users\Admin\Documents\Arduino\hardware\espressif\esp32\cores\esp32\HardwareSerial.cpp
//                    C:\Users\Admin\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.0\cores\esp32\HardwareSerial.cpp
//=================================================================================================
#include <HardwareSerial.h>

MCP3424 MCP(2);                             // Setup MCP3424 Hardware Address = 0
long Voltage[4];                            // Array used to store results

#define SerialDebug  Serial                                                                       // USB Serial(Serial0)
#define SerialNBIOT_RX_PIN    14
#define SerialNBIOT_TX_PIN    13
#define SerialNBIOT  Serial1                                                                      // Serial1(IO13=TXD,IO14=RXD)

#define SerialRS485_RX_PIN    26
#define SerialRS485_TX_PIN    27
#define SerialRS485  Serial2                                                                      // Serial2(IO27=TXD,IO26=RXD)

#define RS485_DIRECTION_PIN   25                                                                  // ESP32-WROVER :IO25
#define RS485_RXD_SELECT      LOW
#define RS485_TXD_SELECT      HIGH

#define SIM7020E_PWRKEY_PIN   33                                                                  // ESP32-WROVER :IO33
#define SIM7020E_SLEEP_PIN    32                                                                  // ESP32-WROVER :IO32
#define SIM7020E_PWRKEY_LOW   LOW                                                                 // Start Power-ON
#define SIM7020E_PWRKEY_HIGH  HIGH                                                                // Release Signal
#define SIM7020E_SLEEP_LOW    LOW                                                                 // Pull-Up DTR to Enable Sleep
#define SIM7020E_SLEEP_HIGH   HIGH                                                                // DTR=Low(Wakeup From Sleep)

#define I2C_SCL1_PIN          22                                                                  // ESP32-WROVER : IO22(SCL1)
#define I2C_SDA1_PIN          21                                                                  // ESP32-WROVER : IO21(SDA1)

#define LED_PIN               2                                                                   // ESP-WROVER  : IO2
#define LedON                 1
#define LedOFF                0

#define CS_SD_CARD_PIN        4                                                                   // ESP-WROVER  : IO4
#define SD_CARD_DISABLE       1
#define SD_CARD_ENABLE        0


DS2482 ds(0);  //channels ds2482-800 is 0 to 7, DS2482-100 is just set 0

byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float celsius, fahrenheit;

unsigned long lastGetI2CSensorTime = 0;

// Control
float DO_Start_SP;
float DO_Stop_SP;
float pH_Start_SP;
float pH_Stop_SP;
float DOOK;
float pHOK;
int DO_Inteval;   // Minute
int pH_Inteval;   // Minute

String inputStr;
String DO_Start_Str;
String DO_Stop_Str;
String DO_OK_Str;
String pH_OK_Str;

int doSatTemp;
float doSatVolt;
float doCalLUT;
float DO_read;
float pH_read;

float pHSlope;
float pHConstant;
float pH4Cal;
float pH7Cal;
float pHVolt;

// SD CARD
char *dayChar;
char *monthChar;
char *yearChar;
char timeChar[6];
char tempChar[5];
char pHChar[5];
char DOChar[5];
int intevalRec = 30000;   // 30 seconds

char day[5];
char month[5];
char year[5];
char timeHH[5];
char timeMM[5];

int writeSDFlag;

int do_auto;
int pumpStatus;

File myFile;

TaskHandle_t Task1 = NULL;
TaskHandle_t Task2 = NULL;
TaskHandle_t Task3 = NULL;
TaskHandle_t Task4 = NULL;
TaskHandle_t Task5 = NULL;
TaskHandle_t Task6 = NULL;
TaskHandle_t Task7 = NULL;

int SD_inserted = 0;

long loopTask1;
long loopTask2;
long loopTask3;
long loopTask4;
long loopTask5;

char ptrTaskList[250];

/*
char ssid[] = "STECH_2G";
char pass[] = "stech1412";
WidgetLED led1(V3);
BlynkTimer timer;*/

int do_on = 0 ;

void IRAM_ATTR resetModule() 
{
  ets_printf("reboot\n");
   esp_restart();
}

void appendFile(fs::FS &fs, const char * path, const char * message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message))
  {
    Serial.println("Message appended");
  } 
  else 
  {
    Serial.println("Append failed");
  }
}


void printDebug()
{
    SerialDebug.print("  Temperature = ");
    SerialDebug.println(celsius);

    SerialDebug.print("DO : ");   
    
    SerialDebug.print(DO_read);
    SerialDebug.println(" mg/L");
    SerialDebug.print("DO Volt : ");
    SerialDebug.println(Voltage[0]*0.000001*2.456521739130435);
    

    SerialDebug.print("pHSlope: ");             
    SerialDebug.print(pHSlope);
    SerialDebug.println("");
    SerialDebug.print("pHConstant: ");             
    SerialDebug.print(pHConstant);
    SerialDebug.println("");
    SerialDebug.print("pH7Cal: ");             
    SerialDebug.print(pH7Cal);
    SerialDebug.println("");
    SerialDebug.print("pH4Cal: ");             
    SerialDebug.print(pH4Cal);
    SerialDebug.println("");

    
    SerialDebug.print("pH Volt : ");             
    SerialDebug.print(pHVolt);
    SerialDebug.println("");

    SerialDebug.print("pH : ");             
    SerialDebug.print(pH_read);
    SerialDebug.println("");

     SerialDebug.print("intevalRec : ");             
    SerialDebug.print(intevalRec);
    SerialDebug.println("");
   

    SerialDebug.print("RTC Time : ");
    SerialDebug.print(daysOfTheWeek[myTimeNow.dayOfTheWeek()]);
    SerialDebug.print(',');
    SerialDebug.print(myTimeNow.day());
    SerialDebug.print('/');
    SerialDebug.print(myTimeNow.month());
    SerialDebug.print('/');
    SerialDebug.print(myTimeNow.year());
    SerialDebug.print(" ");
    SerialDebug.print(myTimeNow.hour());
    SerialDebug.print(':');
    SerialDebug.print(myTimeNow.minute());
    SerialDebug.print(':');
    SerialDebug.println(myTimeNow.second());

    SerialDebug.println(inputStr);

    SerialDebug.print("DO Start : ");
    SerialDebug.println(DO_Start_SP);
    SerialDebug.print("DO Stop : ");
    SerialDebug.println(DO_Stop_SP);

    SerialDebug.print("doCalLUT : ");
    SerialDebug.println(doCalLUT);
    SerialDebug.print("doCalTemp : ");
    SerialDebug.println(doSatTemp);
    SerialDebug.print("doSatVolt : ");
    SerialDebug.println(doSatVolt);
}



/*
void Task_Sendata(void *pvParameters)
{
  for (;;)
  {
        Blynk.run();
        Blynk.virtualWrite(V0, pH_read);
        Blynk.virtualWrite(V1, celsius);
        Blynk.virtualWrite(V2, DO_read);

        if(pumpStatus == 1)
        {
          led1.on();
          SerialDebug.println("LED ON");
        }
        else if(pumpStatus == 0)
        {
          led1.off();
          SerialDebug.println("LED OFF");
        }
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds  
}*/



void Task_writeSD(void *pvParameters)
{
  (void) pvParameters;
  for (;;)
  {    
  
    //timerWrite(timerwdt, 0); //reset timer (feed watchdog);
    SerialDebug.println("Task_writeSD");
     if(!SD.begin(4))
      {
        //=============================================================================================
        SerialDebug.println("Card Mount Failed");
        //=============================================================================================
        //return;
        SD_inserted = 0;
      }
      else
      {
        SD_inserted = 1;
      }

      if(SD_inserted == 1)
      {
        if(celsius > 0)
       {
              vTaskSuspend(Task4);
          vTaskSuspend(Task6);
          vTaskSuspend(Task5);
          vTaskSuspend(Task3);
             appendFile(SD, "/dataLog.txt", day );
             appendFile(SD, "/dataLog.txt", "/");
      
              appendFile(SD, "/dataLog.txt", month );
              appendFile(SD, "/dataLog.txt", "/");
             
              appendFile(SD, "/dataLog.txt", year );
              appendFile(SD, "/dataLog.txt", ",");
                    
              appendFile(SD, "/dataLog.txt", timeHH );
              appendFile(SD, "/dataLog.txt", ":");
      
              appendFile(SD, "/dataLog.txt", timeMM );
              appendFile(SD, "/dataLog.txt", ",");
                   
              appendFile(SD, "/dataLog.txt", DOChar);
              appendFile(SD, "/dataLog.txt", ",");
      
              appendFile(SD, "/dataLog.txt", pHChar);
              appendFile(SD, "/dataLog.txt", ","); 
          
              appendFile(SD, "/dataLog.txt", tempChar);
              appendFile(SD, "/dataLog.txt", "\n");
              
  
              vTaskResume(Task4);    
              vTaskResume(Task6);    
              vTaskResume(Task5);    
              vTaskResume(Task3);    
            
        }
      }


      delay(60000);    
      
      
  }


 // vTaskDelay(3000 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds  
  
}

void Task_DO_control(void *pvParameters)
{
  (void) pvParameters;
    for (;;) // A Task shall never return or exit.
  {
   SerialDebug.println("DO TASK CONTROL");

    pHSlope = 3/(pH7Cal-pH4Cal);
    pHConstant = 7 - (pHSlope*pH7Cal);
    pHVolt = Voltage[1]*0.000001*2.456521739130435;
    
    pH_read = (pHSlope*pHVolt) + pHConstant;
    DO_read = pgm_read_float_near(&SaturationValueTab[0] + (int)(doSatTemp+0.5) )  * (Voltage[0]*0.000001*2.456521739130435) / doSatVolt;   
    
     if(master_input.readPin(OPTO_IN1_PIN) == 0)
        pumpStatus = 1;
     else if(master_input.readPin(OPTO_IN1_PIN) == 1)
        pumpStatus = 0;
        
    //if(master_input.readPin(OPTO_IN0_PIN) == 0)
    if(1)
    {
      do_auto = 1;
      if(DO_Start_SP != DO_Stop_SP)
      {
        if(DO_read < DO_Start_SP)
        {
          do_on = 1;
          master_relay.writePin(RELAY_OUT0_PIN, RELAY_ON);
 
        }
        else if(DO_read > DO_Stop_SP)
        {
          do_on = 0;
          master_relay.writePin(RELAY_OUT0_PIN, RELAY_OFF);  // Contactor
     
        }
      }
    }
    else
    {
      do_auto = 0;
      do_on = 0;
      master_relay.writePin(RELAY_OUT0_PIN, RELAY_OFF);
      //SerialDebug.print("OPTO_IN0 OFF");
    }


    if(DO_read > DOOK)
      master_relay.writePin(RELAY_OUT1_PIN, RELAY_ON);  // DO OK
    else
      master_relay.writePin(RELAY_OUT1_PIN, RELAY_OFF);  // DO OK
    if(pH_read > pHOK)
      master_relay.writePin(RELAY_OUT2_PIN, RELAY_ON);  // pH OK
    else
      master_relay.writePin(RELAY_OUT2_PIN, RELAY_OFF);  // pH OK*/
      

    vTaskDelay(100 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds  
  }

}

void Task_readSensors(void *pvParameters)
{
  (void) pvParameters;
   for (;;) // A Task shall never return or exit.
  {
     SerialDebug.println("Task_readSensors");
    //timerWrite(timerwdt, 0); //reset timer (feed watchdog);
    
    if(ds.wireSearch(addr))
    {
      present = ds.wireReset(); 
      /*SerialDebug.print("Found Device(");
      SerialDebug.print(present, HEX);*/
    
      switch(addr[0]) 
      {
        case 0x10:
          //SerialDebug.print(":(DS18S20)");  // or old DS1820
          type_s = 1;
        break;
    
        case 0x28:
         // SerialDebug.print(":(DS18B20)");
          type_s = 0;
        break;
    
        case 0x22:
         // SerialDebug.print(":(DS1822)");
          type_s = 0;
        break;
    
        default:
       //   SerialDebug.print(":Unknow)");
        return;
      } 
     

      ds.wireReset();
      ds.wireSelect(addr);
      ds.wireWriteByte(0x44);  // start conversion, use ds.write(0x44,1) with parasite power on at the end
   
      delay(1000);     // maybe 750ms is enough, maybe not

      present = ds.wireReset(); //ds.reset(); 
      ds.wireSelect(addr);
      ds.wireWriteByte(0xBE);         // Read Scratchpad
     
     // SerialDebug.print(" Data :");
      for ( i = 0; i < 9; i++) 
      {           // we need 9 bytes
      //  SerialDebug.print(" ");
        data[i] = ds.wireReadByte();
        //if(data[i] < 16) SerialDebug.print("0");
        //SerialDebug.print(data[i], HEX);
      }
      //Serial.print(" CRC = ");
     // SerialDebug.print(":CRC(");
     // if(ds.crc8(data, 8) < 16) SerialDebug.print("0");
     // SerialDebug.print(ds.crc8(data, 8), HEX);
     // SerialDebug.print(")");
     
      int16_t raw = (data[1] << 8) | data[0];
     
      if(type_s) 
      {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) 
        {
          // "count remain" gives full 12 bit resolution
          raw = (raw & 0xFFF0) + 12 - data[6];
        }
      } 
      else 
      {
        //===========================================================================================
        byte cfg = (data[4] & 0x60);
        //===========================================================================================
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;                // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3;           // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1;           // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
        //=====celsius======================================================================================
      }
      celsius = (float)raw / 16.0;

    }
    else
    {
      ds.wireResetSearch();
  
    }

   ds.wireReset(); //ds.reset(); 
   
   for(int i=1; i<=2; i++)
  {
    MCP.Configuration(i,                    // ADC Channel(1,2,3,4) = 1..4
                      12,                   // Resolution(12,14,16,18) = 18-Bit
                      1,                    // ADC Mode(0,1) = 1:Continous mode
                      1);                   // Amplifier gain(1,2,4,8)

    Voltage[i-1] = MCP.Measure();           // Measure is stocked in array Voltage, note that the library will wait for a completed conversion that takes around 200 ms@18bits  
  }

 ds.wireReset(); //ds.reset(); 
 
    if(myRTC.isAlarm1(false))
    {
       myTimeNow = myRTC.now();
       myRTC.clearAlarm1();
    }
    
  }

  
        
  delay(1000);
  //vTaskDelay(1000 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds

  
}

void Task_GetHMIinput(void *pvParameters)  
{
  (void) pvParameters;
  for (;;) // A Task shall never return or exit.
  {
    SerialDebug.println("Task_GetHMIinput");

    //timerWrite(timerwdt, 0); //reset timer (feed watchdog);
     inputStr = Serial1.readString();

     if(inputStr[0] == 'D' && inputStr[1] == 'O' && inputStr[2] == 'O' && inputStr[3] == 'N')
     {

      DO_Start_Str = inputStr.substring(4,7);    
      DO_Start_SP = DO_Start_Str.toFloat();
      EEPROM.writeFloat(10, DO_Start_SP);
      EEPROM.commit();
     }
     if(inputStr[0] == 'D' && inputStr[1] == 'O' && inputStr[2] == 'O' && inputStr[3] == 'F')
     {

      DO_Stop_Str = inputStr.substring(4,7);    
      DO_Stop_SP = DO_Stop_Str.toFloat();
      EEPROM.writeFloat(14, DO_Stop_SP);
      EEPROM.commit();
     }

     if(inputStr[0] == 'd' && inputStr[1] == 'o' && inputStr[2] == 'c' && inputStr[3] == 'a' && inputStr[4] == 'l')
     {
        
        doCalLUT = SaturationValueTab[(int)celsius];
        doSatTemp = celsius;
        doSatVolt = Voltage[0]*0.000001*2.456521739130435;
        EEPROM.writeFloat(18, doSatTemp);
        EEPROM.commit();
        EEPROM.writeFloat(22, doSatVolt);
        EEPROM.commit();
     }
     if(inputStr[0] == 'p' && inputStr[1] == 'h' && inputStr[2] == '4' && inputStr[3] == 'c' && inputStr[4] == 'a' && inputStr[5] == 'l')
     {
        pH4Cal = Voltage[1]*0.000001*2.456521739130435;
        EEPROM.writeFloat(26, pH4Cal);
        EEPROM.commit();
     }
     if(inputStr[0] == 'p' && inputStr[1] == 'h' && inputStr[2] == '7' && inputStr[3] == 'c' && inputStr[4] == 'a' && inputStr[5] == 'l')
     {
        pH7Cal = Voltage[1]*0.000001*2.456521739130435;
        EEPROM.writeFloat(30, pH7Cal);
        EEPROM.commit();
     }
     
     if(inputStr[0] == 'S' && inputStr[1] == 'R' && inputStr[2] == 'E' && inputStr[3] == 'C')   // "SREC"
     {
       /*SerialDebug.println(int((char)inputStr[4]));
       SerialDebug.println(int((char)inputStr[5]));
       SerialDebug.println(intevalRec);*/
        intevalRec = (int((char)inputStr[4]) + int((char)inputStr[5]*256))*60*1000;
        
        EEPROM.writeFloat(34, intevalRec);
        EEPROM.commit();
     }
     if(inputStr[0] == 'P' && inputStr[1] == 'H' && inputStr[2] == 'O' && inputStr[3] == 'K')
     {

      pH_OK_Str = inputStr.substring(4,7);    
      pHOK = pH_OK_Str.toFloat();
      EEPROM.writeFloat(38, pHOK);
      EEPROM.commit();
     }
     if(inputStr[0] == 'D' && inputStr[1] == 'O' && inputStr[2] == 'O' && inputStr[3] == 'K')
     {

      DO_OK_Str = inputStr.substring(4,7);    
      DOOK = DO_OK_Str.toFloat();
      EEPROM.writeFloat(42, DOOK);
      EEPROM.commit();
     }

    vTaskDelay(150 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds  

  }//for loop 
}

void Task_UpdateHMI(void *pvParameters)  
{
  (void) pvParameters;
  for (;;) // A Task shall never return or exit.
  {
     SerialDebug.println("Task_UpdateHMI");
     //timerWrite(timerwdt, 0); //reset timer (feed watchdog);
      String cmd;
      String cmd2;
      String cmd3;
      String cmd4;
      String cmd5;
      String cmd6;
      String cmd7;
      String cmd8;
      String cmd9;
      String cmd10;
      String cmd11;

      // DO
      cmd += "t0" ;
      cmd += ".txt=\"";
      cmd += DO_read;
      cmd += "\"";
      Serial1.print(cmd.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      delay(150);
      // DO
      cmd = "";
      cmd += "t51" ;
      cmd += ".txt=\"";
      cmd += DO_read;
      cmd += "\"";
      Serial1.print(cmd.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      delay(150);
      // pH
      cmd2 += "t1" ;
      cmd2 += ".txt=\"";
      cmd2 += pH_read;
      cmd2 += "\"";
      Serial1.print(cmd2.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      delay(150);
      // Water temperature
      cmd3 += "t2" ;
      cmd3 += ".txt=\"";
      cmd3 += celsius;
      cmd3 += "\"";
      Serial1.print(cmd3.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);

         delay(150);   
      // Date & Time
      cmd4 += "t11" ;
      cmd4 += ".txt=\"";
      cmd4 += myTimeNow.day();
      cmd4 += "\"";
      Serial1.print(cmd4.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
       delay(150);     
      cmd5 += "t12" ;
      cmd5 += ".txt=\"";
      cmd5 += myTimeNow.month();
      cmd5 += "\"";
      Serial1.print(cmd5.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      delay(150);
      cmd6 += "t13" ;
      cmd6 += ".txt=\"";
      cmd6 += myTimeNow.year();
      cmd6 += "\"";
      Serial1.print(cmd6.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
       delay(150);     
      cmd7 += "t14" ;
      cmd7 += ".txt=\"";
      cmd7 += myTimeNow.hour();
      cmd7 += "\"";
      Serial1.print(cmd7.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      delay(150);
      cmd8 += "t15" ;
      cmd8 += ".txt=\"";
      cmd8 += myTimeNow.minute();
      cmd8 += "\"";
      Serial1.print(cmd8.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      delay(150);
       // DO start time
      cmd9 += "t23" ;
      cmd9 += ".txt=\"";
      cmd9 += DO_Start_SP;
      cmd9 += "\"";
      Serial1.print(cmd9.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      delay(150);
       // DO stop time
      cmd10 += "t24" ;
      cmd10 += ".txt=\"";
      cmd10 += DO_Stop_SP;
      cmd10 += "\"";
      Serial1.print(cmd10.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);  

      delay(150);

      /*// Record inteval time
      cmd11 += "n6" ;
      cmd11 += ".val=\"";
      cmd11 += (int)intevalRec/60000;
      cmd11 += "\"";
      Serial1.print(cmd11.c_str());
      Serial1.write(0xFF);
      Serial1.write(0xFF);
      Serial1.write(0xFF);  */

    

    vTaskDelay(4000 / portTICK_PERIOD_MS); //Transmit every 3 seconds  
    //vTaskDelay(5000 / portTICK_PERIOD_MS); //Transmit every 3 seconds  
  
  }
}

// the task method
void blinkLed13() {
  SerialDebug.println("Blink Task");
  digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second  
}


void setup() 
{ 
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LedOFF);
 
  pinMode(CS_SD_CARD_PIN, OUTPUT);
  digitalWrite(CS_SD_CARD_PIN, SD_CARD_DISABLE);

  Wire.begin(I2C_SDA1_PIN,I2C_SCL1_PIN);                                                      

  //pinMode(RS485_DIRECTION_PIN, OUTPUT);                                                          // RS485 Direction
  //digitalWrite(RS485_DIRECTION_PIN, RS485_RXD_SELECT);

  SerialDebug.begin(115200);
  while(!SerialDebug);

  SerialNBIOT.begin(115200, SERIAL_8N1, SerialNBIOT_RX_PIN, SerialNBIOT_TX_PIN);
  while(!SerialNBIOT);

  //SerialRS485.begin(9600, SERIAL_8N1, SerialRS485_RX_PIN, SerialRS485_TX_PIN);
  //while(!SerialRS485);

  lastGetI2CSensorTime = millis();

  timerwdt = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timerwdt, &resetModule, true);  //attach callback
  timerAlarmWrite(timerwdt, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timerwdt);                          //enable interrupt
  

   //tSD.every(5000, SDwrite);
   
/*
  // WIFI
   WiFi.begin(ssid, pass);
  int wifi_ctr = 0;
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    SerialDebug.print(".");
  }
  
  SerialDebug.println("WiFi connected"); 
 digitalWrite(2, HIGH);
  Blynk.begin("qzrRpWUaPQH32P4FowFvZZ-0zLVbVVIH", ssid, pass);*/


    // Initial EEPROM
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(3000);
  }
    
    DO_Start_SP = EEPROM.readFloat(10);
    DO_Stop_SP = EEPROM.readFloat(14);
    doSatTemp = EEPROM.readFloat(18);
    doSatVolt = EEPROM.readFloat(22);
    pH4Cal = EEPROM.readFloat(26);
    pH7Cal = EEPROM.readFloat(30);
    pHOK = EEPROM.readFloat(38);
    DOOK = EEPROM.readFloat(42);
    EEPROM.commit();

    ////////////////////////////////////////////////////////////// TEST
    pH7Cal = 1.82;
    pH4Cal = 0.44;
    doSatVolt = 1.77;
    doSatTemp = 28;
    DO_Start_SP = 5.0;
    DO_Stop_SP = 6.0;
    ////////////////////////////////////////////////////////////// TEST

   myRTC.begin();
  if(myRTC.lostPower()) 
  {
    SerialDebug.println("RTC lost power, lets set the time!");
    myRTC.adjust(DateTime(F(__DATE__), F(__TIME__)));                                            // Setup RTC from date & time this sketch was compiled
  }
  //myRTC.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
  //myRTC.adjust(DateTime(2020, 4, 15, 11, 53, 0));
  myRTC.armAlarm1(false);
  myRTC.clearAlarm1();
  myRTC.armAlarm2(false);
  myRTC.clearAlarm2();
  myRTC.setAlarm1(0, 0, 0, 0, DS3231_EVERY_SECOND);                                              // Alarm Every Second
  SerialDebug.println("Initial RTC:DS3231....Complete");

  master_relay.writePin(RELAY_OUT0_PIN, RELAY_OFF);
  master_relay.writePin(RELAY_OUT1_PIN, RELAY_OFF);
  master_relay.writePin(RELAY_OUT2_PIN, RELAY_OFF);
  master_relay.writePin(RELAY_OUT3_PIN, RELAY_OFF);
  master_relay.writePin(RELAY_OUT4_PIN, RELAY_OFF);
  master_relay.writePin(RELAY_OUT5_PIN, RELAY_OFF);
  master_relay.writePin(RELAY_OUT6_PIN, RELAY_OFF);
  master_relay.writePin(RELAY_OUT7_PIN, RELAY_OFF);

    xTaskCreatePinnedToCore(
    Task_GetHMIinput
    ,  "Task_GetHMIinput"   // A name just for humans
    ,  1024*2  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &Task3 
    ,  ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
    Task_DO_control
    ,  "Task_DO_control"   // A name just for humans
    ,  1024*2  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  6  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &Task5  
    ,  ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
    Task_readSensors
    ,  "Task_readSensors"   // A name just for humans
    ,  1024*2  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  5  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &Task6 
    ,  ARDUINO_RUNNING_CORE);

     xTaskCreatePinnedToCore(
    Task_UpdateHMI
    ,  "Task_UpdateHMI"   // A name just for humans
    ,  1024*2  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &Task4 
    ,  ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
    Task_writeSD
    ,  "Task_writeSD"   // A name just for humans
    ,  1024*4 // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &Task1
    ,  ARDUINO_RUNNING_CORE);

    
    /*xTaskCreatePinnedToCore(
    Task_Sendata
    ,  "Task_Sendata"   // A name just for humans
    ,  1024*2 // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);*/


}


void loop() 
{
  long tme = millis();
 
  timerWrite(timerwdt, 0); //reset timer (feed watchdog);
  loopTimer.check(&Serial);
  blinkLed13(); // call the method to blink the led
  taskYIELD();
           
  dayChar = (char*)myTimeNow.day();
  monthChar = (char*)myTimeNow.month();
  yearChar = (char*)myTimeNow.year();
  dtostrf(myTimeNow.day(), 2, 0, day);
  dtostrf(myTimeNow.year(), 4, 0, year);
  dtostrf(myTimeNow.hour(), 2, 0, timeHH);
  dtostrf(DO_read, 4, 1, DOChar);
  dtostrf(pH_read, 4, 1, pHChar);

  dtostrf(myTimeNow.month(), 2, 0, month);
  dtostrf(myTimeNow.minute(), 2, 0, timeMM);
  dtostrf(celsius, 4, 1, tempChar);

  /*SerialDebug.print("Time : ");
  SerialDebug.print(myTimeNow.day());SerialDebug.print("/");SerialDebug.print(myTimeNow.month());SerialDebug.print("/");SerialDebug.println(myTimeNow.year());
  SerialDebug.print(myTimeNow.hour());SerialDebug.print(":");SerialDebug.print(myTimeNow.minute());SerialDebug.print(":");SerialDebug.println(myTimeNow.second());*/
  SerialDebug.print("pH : ");
  SerialDebug.println(pH_read);
  SerialDebug.print("DO : ");
  SerialDebug.print(DO_read);
  SerialDebug.println(" mg/L");
  SerialDebug.print("DO STATUS : ");
  SerialDebug.println(do_on);
  SerialDebug.print("main loop time is : ");
  
  tme = millis() - tme;
  SerialDebug.print(tme);
  SerialDebug.println(" ms");

}
