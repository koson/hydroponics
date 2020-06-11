#include "Timer.h"
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#include "profiles.h"


#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Data wire is plugged into pin 4 on the Arduino
#define ONE_WIRE_BUS 4
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


#define DHTPIN 12     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

char inChar[10];
float pH_waterSP;
float EC_waterSP;
float Temp_waterSP;
float Temp_houseSP;
float Humid_houseSP;
int Co2_houseSP;

int GrowthDay;  // Day
int FogIntevalSP; // Minute
int FogOnSP;     // Second
int pHIntevalSP; // Minute
int pHOnSP;     // Second
int ECIntevalSP; // Minute
int ECOnSP;     // Second

String inputStr;
String pHStrSub;
String ecStrSub;
String TempWaterStrSub;
String GrowthDayStrSub;
String tempHouseStrSub;
String humidHouseStrSub;
String co2HouseStrSub;
String FogIntevalStrSub;
String FogOnStrSub;
String pHIntevalStrSub;  // ph dose inteval
String pHDoseStrSub;  // ph dose time
String ECIntevalStrSub;  // ec dose inteval
String ECDoseStrSub;  // ec dose time
String ecFactorStrSub;
String phFactorStrSub;
String phOffFactorStrSub;
String ecOffFactorStrSub;

void Task_GetHMIinput( void *pvParameters );
void Task_UpdateHMI( void *pvParameters );
void Task_CheckFOG( void *pvParameters );
void Task_DoserControl( void *pvParameters );
void Task_ReadDHT( void *pvParameters );
void Task_SerialSend( void *pvParameters );

Timer t;

int cntHour;
int cntMin;
int cntSec;

int cntFogIntevalSec;
int cntFogIntevalMin;
int cntFogIntevalHour;
int cntpHdoserSec;
int cntpHdoserMin;
int cntpHdoserHour;
int cntECdoserSec;
int cntECdoserMin;
int cntECdoserHour;

int AutoFog;
int AutoEC;
int AutopH;

// OUTPUT
int OUTPUT1;   // pH doser
int OUTPUT2;   // EC doser
int OUTPUT3;   // Fogger

const int AutFogPin =  26;  
const int AutpHPin =  27;  
const int AutECPin =  25;  

const int FogOutputpin = 2;
const int pHOutputpin = 15;
const int ECOutputpin = 13;

// Read data from sensors
float outside_temp;
float outside_humid;
float waterTemp;
int pH_RAW;
int EC_RAW;
float pH_Scaled;
float EC_Scaled;
float pHFactor;
float ECFactor;

float phVolt;
float ECVolt;

float pHVolt_Offset = 0.0;
float ECVolt_Offset = 0.0;

void setup() 
{
   InitialProfile();   // initial plant profile
    //Serial.println(plan1.ph[1]);

    //plan1.ph[1] = 6.2;

     dht.begin();
     sensors.begin();
    Serial.begin(112500);
    Serial2.begin(112500);
    Serial1.begin(112500);

    t.every(1000, cnt1Sec);
   
    xTaskCreatePinnedToCore(
    Task_GetHMIinput
    ,  "Task_GetHMIinput"   // A name just for humans
    ,  1024*2  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
    Task_UpdateHMI
    ,  "Task_UpdateHMI"   // A name just for humans
    ,  1024*2  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  4  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
    Task_CheckFOG
    ,  "Task_CheckFOG"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
    Task_DoserControl
    ,  "Task_DoserControl"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  5  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
    Task_ReadDHT
    ,  "Task_ReadDHT"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  6  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(
    Task_SerialSend
    ,  "Task_SerialSend"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  7  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
    
    // Initialize parameters
    cntSec = 0;
    pH_waterSP = 6.5;
    EC_waterSP = 1.5;
    Temp_waterSP = 28;
    Temp_houseSP = 25;
    Humid_houseSP = 75;
    Co2_houseSP = 500;
    GrowthDay = 0;  // Day
    FogIntevalSP = 1; // Minute
    FogOnSP = 10;     // Second

   ECIntevalSP = 1; // Minute
   pHOnSP = 5;     // Second
   pHIntevalSP = 1; // Minute
   ECOnSP = 20;     // Second
   
    AutoFog = 0;
    AutoEC = 0;
    AutopH = 0;

    pHFactor = 5.283;
    ECFactor = 7.5757;

    pinMode(AutFogPin, INPUT);
    pinMode(AutpHPin, INPUT);
    pinMode(AutECPin, INPUT);

    pinMode(FogOutputpin, OUTPUT);
    pinMode(ECOutputpin, OUTPUT);
    pinMode(pHOutputpin, OUTPUT);

    digitalWrite(pHOutputpin, LOW);
    digitalWrite(ECOutputpin, LOW);
    digitalWrite(FogOutputpin, LOW);
    
}
int testUpdateCO2=0;

void loop() {

  AutoFog = digitalRead(AutFogPin);
  AutopH = digitalRead(AutpHPin);
  AutoEC = digitalRead(AutECPin);


 if(AutoFog == 0)
 {
    Serial.println("AUTO FOG OFF");
 }
 else
 {
   Serial.println("AUTO FOG ON");
 }

 if(AutoEC == 0)
 {
    Serial.println("AUTO EC OFF");
 }
 else
 {
  Serial.println("AUTO EC ON");
 }

 if(AutopH == 0)
 {
        Serial.println("AUTO pH OFF");
 }
 else
 {
  
  Serial.println("AUTO pH ON");
 }
    
  t.update();


  // Debug
  /*AutoFog = 1;
  AutopH = 1;
  AutoEC = 1;*/
  
  if(AutoFog == 0){
    cntFogIntevalSec = 0;
    cntFogIntevalMin = 0;
    cntFogIntevalHour = 0;
  }
  if(AutopH == 0){
    cntpHdoserSec = 0;
    cntpHdoserMin = 0;
    cntpHdoserHour = 0;
  }
  if(AutoEC == 0){
    cntECdoserSec = 0;
    cntECdoserMin = 0;
    cntECdoserHour = 0;
  }



  Serial.print("Count time : ");  Serial.print(cntHour);  Serial.print(":");  Serial.print(cntMin);  Serial.print(":");  Serial.print(cntSec);
  Serial.print("  pH doser : ");  Serial.print(cntpHdoserHour);  Serial.print(":");  Serial.print(cntpHdoserMin);  Serial.print(":");  Serial.print(cntpHdoserSec);
  Serial.print("  EC doser : ");  Serial.print(cntECdoserHour);  Serial.print(":");  Serial.print(cntECdoserMin);  Serial.print(":");  Serial.print(cntECdoserSec);
  Serial.print("  FOGGER : ");  Serial.print(cntFogIntevalHour);  Serial.print(":");  Serial.print(cntFogIntevalMin);  Serial.print(":");  Serial.println(cntFogIntevalSec);

  Serial.print(F("Humidity: "));
  Serial.print(outside_humid);
  Serial.print(F("%  Temperature: "));
  Serial.print(outside_temp);
  Serial.print(F("°C "));
 

  if(OUTPUT3 == 1)
  {
    Serial.println("FOG ON");
    digitalWrite(FogOutputpin, HIGH);   
  }
  else if(OUTPUT3 == 0)
  {
   Serial.println("FOG OFF");
   digitalWrite(FogOutputpin, LOW);   
  }
  if(OUTPUT2 == 1)
  {
    Serial.println("EC ON");
    digitalWrite(ECOutputpin, HIGH);
  }
  else if(OUTPUT2 == 0)
  {
   Serial.println("EC OFF");
   digitalWrite(ECOutputpin, LOW);
  }
  if(OUTPUT1 == 1)
  {
    Serial.println("pH ON");
    digitalWrite(pHOutputpin, HIGH);
  }
  else if(OUTPUT1 == 0)
  {
   Serial.println("pH OFF");
   digitalWrite(pHOutputpin, LOW);



  }

   Serial.print("pH : ");
   Serial.print(pH_Scaled);
   Serial.print("  EC : ");
   Serial.println(EC_Scaled);
   Serial.print("  phVolt : ");
   Serial.println(phVolt);
   Serial.print("  EC Volt : ");
   Serial.println(ECVolt);

  /*Serial.print("ph setpoint : ");
  Serial.println(pH_waterSP);
  Serial.print("EC setpoint : ");
  Serial.println(EC_waterSP);
  Serial.print("Temp setpoint : ");
  Serial.println(Temp_waterSP);
  Serial.print("Day : ");
  Serial.println(GrowthDay);
  Serial.print("Green temp SP : ");
  Serial.println(Temp_houseSP);
  Serial.print("Green Humid SP : ");
  Serial.println(Humid_houseSP);
  Serial.print("Green co2 SP : ");
  Serial.println(Co2_houseSP);
  Serial.print("Fog Inteval SP : ");
  Serial.println(FogIntevalSP);
  Serial.print("Fog ON SP : ");
  Serial.println(FogOnSP);*/
  testUpdateCO2++;
}


void Task_GetHMIinput(void *pvParameters)  
{
  for (;;) // A Task shall never return or exit.
  {
    
     inputStr = Serial2.readString();

     if(inputStr[0] == 'p' && inputStr[1] == 'h'){
      //Serial.println("get pH sp data");
      pHStrSub = inputStr.substring(2,5);    
      pH_waterSP = pHStrSub.toFloat();
     }
     if(inputStr[0] == 'e' && inputStr[1] == 'c'){
     //Serial.println("get EC sp data");
      ecStrSub = inputStr.substring(2,5);    
      EC_waterSP = ecStrSub.toFloat();
     }
     if(inputStr[0] == 'n' && inputStr[1] == 't'){
     //Serial.println("get temp sp data");
      TempWaterStrSub = inputStr.substring(2,5);    
      Temp_waterSP = TempWaterStrSub.toFloat();
     }
     if(inputStr[0] == 'd'&& inputStr[1] == 'w'){
     Serial.println("get data of growth data");
      GrowthDay = int((char)inputStr[2]) + int((char)inputStr[3]*256);
     }
     if(inputStr[0] == 'g'&& inputStr[1] == 't'){
     Serial.println("get data of temp house");
      tempHouseStrSub = inputStr.substring(2,5);    
      Temp_houseSP = tempHouseStrSub.toFloat();
     }
     if(inputStr[0] == 'g'&& inputStr[1] == 'h'){
     Serial.println("get data of humid house");
      humidHouseStrSub = inputStr.substring(2,5);    
      Humid_houseSP = humidHouseStrSub.toFloat();
     }
     if(inputStr[0] == 'g'&& inputStr[1] == 'c'){
     Serial.println("get data of co2 house");
      Co2_houseSP = int((char)inputStr[2]) + int((char)inputStr[3]*256);
     }
     if(inputStr[0] == 'f'&& inputStr[1] == 'g' && inputStr[2] == 'I'){
     Serial.println("get data of fog inteval house");
     Serial.println(inputStr);
     FogIntevalStrSub =  inputStr.substring(3,5);
     FogIntevalSP = FogIntevalStrSub.toFloat();
     }
     if(inputStr[0] == 'f'&& inputStr[1] == 'g' && inputStr[2] == 'O'){
     Serial.println("get data of fog on house");
     Serial.println(inputStr);
     FogOnStrSub =  inputStr.substring(3,5);
     FogOnSP = FogOnStrSub.toFloat();
     }
     if(inputStr[0] == 'p'&& inputStr[1] == 'h' && inputStr[2] == 'I'){
     Serial.println("get data phg inteval");
     Serial.println(inputStr);
     pHIntevalStrSub =  inputStr.substring(3,5);
     pHIntevalSP = pHIntevalStrSub.toFloat();
     }
     if(inputStr[0] == 'p'&& inputStr[1] == 'h' && inputStr[2] == 'O'){
     Serial.println("get data ph dose");
     Serial.println(inputStr);
     pHDoseStrSub =  inputStr.substring(3,5);
     pHOnSP = pHDoseStrSub.toFloat();
     }
     if(inputStr[0] == 'e'&& inputStr[1] == 'c' && inputStr[2] == 'I'){
     Serial.println("get data ec inteval");
     Serial.println(inputStr);
     ECIntevalStrSub =  inputStr.substring(3,5);
     ECIntevalSP = ECIntevalStrSub.toFloat();
     }
     if(inputStr[0] == 'e'&& inputStr[1] == 'c' && inputStr[2] == 'O'){
     Serial.println("get data ec dose");
     Serial.println(inputStr);
     ECDoseStrSub =  inputStr.substring(3,5);
     ECOnSP = ECDoseStrSub.toFloat();
     }
     if(inputStr[0] == 'd'&& inputStr[1] == 'w' && inputStr[2] == 'O'){
     Serial.println("reset day");
     Serial.println(inputStr);
     GrowthDay = 0;
     }
     if(inputStr[0] == 'p'&& inputStr[1] == 'h' && inputStr[2] == 'S' && inputStr[3] == 'f'){    // phSf
     Serial.println("get data of humid house");
      phFactorStrSub = inputStr.substring(4,8);    
      pHFactor = phFactorStrSub.toFloat();
     }
     if(inputStr[0] == 'e'&& inputStr[1] == 'c' && inputStr[2] == 'S' && inputStr[3] == 'f'){    // ecSf
     Serial.println("get data of humid house");
      ecFactorStrSub = inputStr.substring(4,8);    
      ECFactor = ecFactorStrSub.toFloat();
     }
     if(inputStr[0] == 'p'&& inputStr[1] == 'h' && inputStr[2] == 'O' && inputStr[3] == 'f'){    // phOf
      phOffFactorStrSub = inputStr.substring(4,8);    
      pHVolt_Offset = phOffFactorStrSub.toFloat();
     }
     if(inputStr[0] == 'e'&& inputStr[1] == 'c' && inputStr[2] == 'O' && inputStr[3] == 'f'){    // ecOf
      ecOffFactorStrSub = inputStr.substring(4,8);    
      ECVolt_Offset = ecOffFactorStrSub.toFloat();
     }     


   
   
    vTaskDelay(100 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds
}
}



void Task_SerialSend(void *pvParameters)  
{
  (void) pvParameters;
  for (;;) // A Task shall never return or exit.
  {
    Serial1.print("ph");
    Serial1.print(pH_Scaled);
    Serial1.print("ec");


    vTaskDelay(1000 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds
  }
}

void Task_UpdateHMI(void *pvParameters)  
{
  (void) pvParameters;
  for (;;) // A Task shall never return or exit.
  {

      String cmd;
      String cmd2;
      String cmd3;
      String cmd4;
      String cmd5;
      String cmd6;
      String cmd7;
      String cmd8;

      // EC
      cmd += "t72" ;
      cmd += ".txt=\"";
      cmd += EC_waterSP;
      cmd += "\"";
      Serial2.print(cmd.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // pH
      cmd2 += "t71" ;
      cmd2 += ".txt=\"";
      cmd2 += pH_waterSP;
      cmd2 += "\"";
      Serial2.print(cmd2.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Water temp
      cmd3 += "t73" ;
      cmd3 += ".txt=\"";
      cmd3 += Temp_waterSP;
      cmd3 += "\"";
      Serial2.print(cmd3.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Green house temp
      cmd4 += "t60" ;
      cmd4 += ".txt=\"";
      cmd4 += Temp_houseSP;
      cmd4 += "\"";
      Serial2.print(cmd4.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Green house humidity
      cmd5 += "t61" ;
      cmd5 += ".txt=\"";
      cmd5 += Humid_houseSP;
      cmd5 += "\"";
      Serial2.print(cmd5.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Fog Inteval
      cmd6 += "t91" ;
      cmd6 += ".txt=\"";
      cmd6 += FogIntevalSP;
      cmd6 += "\"";
      Serial2.print(cmd6.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Fog Inteval
      cmd7 += "t92" ;
      cmd7 += ".txt=\"";
      cmd7 += FogOnSP;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // pH Inteval
      cmd7 = "";
      cmd7 += "t100" ;
      cmd7 += ".txt=\"";
      cmd7 += pHIntevalSP;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      // pH dose time
      cmd7 = "";
      cmd7 += "t101" ;
      cmd7 += ".txt=\"";
      cmd7 += pHOnSP;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      // EC dose time
      cmd7 = "";
      cmd7 += "t102" ;
      cmd7 += ".txt=\"";
      cmd7 += ECIntevalSP;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      
      // EC dose time
      cmd7 = "";
      cmd7 += "t103" ;
      cmd7 += ".txt=\"";
      cmd7 += ECOnSP;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Outside temp
      cmd7 = "";
      cmd7 += "t8" ;
      cmd7 += ".txt=\"";
      cmd7 += outside_temp;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Outside humidity
      cmd7 = "";
      cmd7 += "t9" ;
      cmd7 += ".txt=\"";
      cmd7 += outside_humid;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Water temperature
      cmd7 = "";
      cmd7 += "t12" ;
      cmd7 += ".txt=\"";
      cmd7 += waterTemp;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      // Water pH
      cmd7 = "";
      cmd7 += "t11" ;
      cmd7 += ".txt=\"";
      cmd7 += pH_Scaled;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

      cmd7 = "";
      cmd7 += "t210" ;
      cmd7 += ".txt=\"";
      cmd7 += pH_Scaled;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);
           
      // Water EC
      cmd7 = "";
      cmd7 += "t10" ;
      cmd7 += ".txt=\"";
      cmd7 += EC_Scaled;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);  
      cmd7 = "";
      cmd7 += "t214" ;
      cmd7 += ".txt=\"";
      cmd7 += EC_Scaled;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);  
      
      // pH Raw
      cmd7 = "";
      cmd7 += "t202" ;
      cmd7 += ".txt=\"";
      cmd7 += pH_RAW;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);  

      // EC Raw
      cmd7 = "";
      cmd7 += "t203" ;
      cmd7 += ".txt=\"";
      cmd7 += EC_RAW;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF); 

      // pH Volt
      cmd7 = "";
      cmd7 += "t204" ;
      cmd7 += ".txt=\"";
      cmd7 += phVolt;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      // EC Volt
      cmd7 = "";
      cmd7 += "t211" ;
      cmd7 += ".txt=\"";
      cmd7 += ECVolt;
      cmd7 += "\"";
      Serial2.print(cmd7.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);     

 
   char buf[10] = {0};    
    utoa(Co2_houseSP, buf, 10);
    cmd8 += "n16";
    cmd8 += ".val=";
    cmd8 += buf;
   Serial2.print(cmd8.c_str());
      Serial2.write(0xFF);
      Serial2.write(0xFF);
      Serial2.write(0xFF);

 
    utoa(GrowthDay, buf, 10);
    cmd8 = "";
    cmd8 += "n17";
    cmd8 += ".val=";
    cmd8 += buf;
    Serial2.print(cmd8.c_str());
    Serial2.write(0xFF);
    Serial2.write(0xFF);
    Serial2.write(0xFF);

    utoa(GrowthDay, buf, 10);
    cmd8 = "";
    cmd8 += "n6";
    cmd8 += ".val=";
    cmd8 += buf;
    Serial2.print(cmd8.c_str());
    Serial2.write(0xFF);
    Serial2.write(0xFF);
    Serial2.write(0xFF);
      
      vTaskDelay(5000 / portTICK_PERIOD_MS); //Transmit every 3 seconds  
  
  }
}

void Task_CheckFOG(void *pvParameters)  
{
  (void) pvParameters;
  for (;;) // A Task shall never return or exit.
  {
      if(cntFogIntevalMin >= FogIntevalSP)   // Minute
      {
        OUTPUT3 = 1;
        if(cntFogIntevalSec >= FogOnSP)
        {
          cntFogIntevalHour = 0;
          cntFogIntevalMin = 0;
          cntFogIntevalSec = 0;
          OUTPUT3 = 0;
        }         
      }

  vTaskDelay(100 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds    
  }
  
}

void Task_DoserControl(void *pvParameters) 
{
   (void) pvParameters;
  for (;;) // A Task shall never return or exit.
  {
    // Read data from sensor
     pH_RAW = analogRead(32);
     EC_RAW = analogRead(33);
  
     phVolt = (((float)pH_RAW/4095)*3.3)+pHVolt_Offset;
     ECVolt = (((float)EC_RAW/4095)*3.3)+ECVolt_Offset;

     pH_Scaled = (phVolt*pHFactor)-3.4339622;
     EC_Scaled = (ECVolt*ECFactor)-5;
     
    
    // pH
    if(cntpHdoserMin >= pHIntevalSP)   
      {
        if(pH_Scaled > pH_waterSP)
          OUTPUT1 = 1;
        if(cntpHdoserSec >= pHOnSP)
        {
          cntpHdoserHour = 0;
          cntpHdoserMin = 0;
          cntpHdoserSec = 0;
          OUTPUT1 = 0;
        } 
      } 
    if(cntECdoserMin >= ECIntevalSP)   
      {
        if(EC_Scaled < EC_waterSP)
          OUTPUT2 = 1;
        if(cntECdoserSec >= ECOnSP)
        {
          cntECdoserHour = 0;
          cntECdoserMin = 0;
          cntECdoserSec = 0;
          OUTPUT2 = 0;
        } 
      } 
    vTaskDelay(100 / portTICK_PERIOD_MS); //Transmit every 0.1 seconds   
  }
}


void Task_ReadDHT(void *pvParameters)
{
  for (;;) // A Task shall never return or exit.
  {
    //  DHT11
    outside_humid = dht.readHumidity();
    outside_temp = dht.readTemperature();

   // Water temperature DSB1820
   sensors.requestTemperatures();  
   float nanBuff_not;

   nanBuff_not = sensors.getTempCByIndex(0);
   if(!isnan(nanBuff_not))
    waterTemp = nanBuff_not;
  
    delay(2500);
  }
  
}

void cnt1Sec()
{
  cntSec++;
  if(AutoFog == 1)
    cntFogIntevalSec++;
  else
  {
    cntFogIntevalSec = 0;
    cntFogIntevalMin = 0;
    cntFogIntevalHour = 0;
  }
  if(AutopH == 1)
    cntpHdoserSec++;
  else
  {
    cntpHdoserSec = 0;
    cntpHdoserMin = 0;
    cntpHdoserHour = 0;
  }
  if(AutoEC == 1)
    cntECdoserSec++;
  else
  {
    cntECdoserSec = 0;
    cntECdoserMin = 0;
    cntECdoserHour =0 ;
  }
  
  if(cntSec==60)
  {
    cntSec = 0;
    cntFogIntevalSec=0;
    cntpHdoserSec=0;
    cntECdoserSec=0;
    
    cntMin++;
    cntFogIntevalMin++;
    cntpHdoserMin++;
    cntECdoserMin++;
  }
  if(cntMin==60)
  {
    cntMin = 0;
    cntFogIntevalMin=0;
    cntpHdoserMin= 0;
    cntECdoserMin= 0;
    
    cntHour++;
    cntFogIntevalHour++;
    cntpHdoserHour++;
    cntECdoserHour++;
  }
  if(cntHour == 24 && cntSec == 0)
    GrowthDay++;

}

  