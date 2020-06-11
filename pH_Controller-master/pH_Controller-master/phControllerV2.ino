
#include <Event.h>
#include <Timer.h>
#include <LiquidCrystal.h>
#include <OneWire.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
Timer t;
 
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to
const int analogOutPin = 9; // Analog output pin that the LED is attached to

float sensorValue = 0;        // value read from the pot
float sensorValueOld;
float voltage;
float pH;
float senSorValueSum;
const float pump_feed = 4.347826; // ml/sec

int time_min; // time_left
int time_sec; // time_left
int pHcount=0;



#define StartConvert 0
#define ReadTemperature 1

const byte numReadings = 20;     //the number of sample times
byte ECsensorPin = A1;  //EC Meter analog output,pin on analog 1
byte DS18B20_Pin = 6; //DS18B20 signal, pin on digital 2
unsigned int AnalogSampleInterval=25,printInterval=700,tempSampleInterval=850;  //analog sample interval;serial print interval;temperature sample interval
unsigned int readings[numReadings];      // the readings from the analog input
byte index = 0;                  // the index of the current reading
unsigned long AnalogValueTotal = 0;                  // the running total
unsigned int AnalogAverage = 0,averageVoltage=0;                // the average
unsigned long AnalogSampleTime,printTime,tempSampleTime;
float temperature,ECcurrent; 
 
//Temperature chip i/o
OneWire ds(DS18B20_Pin);  // on digital pin 2

void sensor_Read()
{
    senSorValueSum = 0;
  
  for(int l=1;l<=500;l++)
  {
    sensorValue = analogRead(analogInPin);            
    senSorValueSum = senSorValueSum + sensorValue;

  }

sensorValue =senSorValueSum/500;  

//pH = (-0.03529411764705882352941176470588*sensorValue)+29.9;  // without EC probe

//pH = (-0.03529411764705882352941176470588*sensorValue)+16.1;  // with EC probe
pH = (-0.0297*sensorValue)+24.117; // with EC probe

}

void setup() {
  Serial.begin(9600); 
    //int tickEvent = t.every(10800000, doSomething); // every 3 hours
   //int tickEvent = t.every(3600000, doSomething);   // every 1 hour
   //int tickEvent = t.every(30000, doSomething);   // every =3 seconds
   pinMode(7, OUTPUT);  
   digitalWrite(7, HIGH);

  lcd.begin(20, 4);
  lcd.print(" Bearing Hydroponics");
  
   for (byte thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0;
  TempProcess(StartConvert);   //let the DS18B20 start the convert
  AnalogSampleTime=millis();
  printTime=millis();
  tempSampleTime=millis();
  
}


void loop() {
  
  /*
   Every once in a while,sample the analog value and calculate the average.
  */
  if(millis()-AnalogSampleTime>=AnalogSampleInterval)  
  {
    AnalogSampleTime=millis();
     // subtract the last reading:
    AnalogValueTotal = AnalogValueTotal - readings[index];
    // read from the sensor:
    readings[index] = analogRead(ECsensorPin);
    // add the reading to the total:
    AnalogValueTotal = AnalogValueTotal + readings[index];
    // advance to the next position in the array:
    index = index + 1;
    // if we're at the end of the array...
    if (index >= numReadings)
    // ...wrap around to the beginning:
    index = 0;
    // calculate the average:
    AnalogAverage = AnalogValueTotal / numReadings;
  }
  /*
   Every once in a while,MCU read the temperature from the DS18B20 and then let the DS18B20 start the convert.
   Attention:The interval between start the convert and read the temperature should be greater than 750 millisecond,or the temperature is not accurate!
  */
   if(millis()-tempSampleTime>=tempSampleInterval) 
  {
    tempSampleTime=millis();
    temperature = TempProcess(ReadTemperature);  // read the current temperature from the  DS18B20
    TempProcess(StartConvert);                   //after the reading,start the convert for next reading
  }
   /*
   Every once in a while,print the information on the serial monitor.
  */
  if(millis()-printTime>=printInterval)
  {
    printTime=millis();
    averageVoltage=AnalogAverage*(float)5000/1024;
    Serial.print("Analog value:");
    Serial.print(AnalogAverage);   //analog average,from 0 to 1023
    Serial.print("    Voltage:");
    Serial.print(averageVoltage);  //millivolt average,from 0mv to 4995mV
    Serial.print("mV    ");
    Serial.print("temp:");
    Serial.print(temperature);    //current temperature
    Serial.print("^C     EC:");
    
    float TempCoefficient=1.0+0.0185*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.0185*(fTP-25.0));
    float CoefficientVolatge=(float)averageVoltage/TempCoefficient;   
    
   
    if(CoefficientVolatge<150)
      Serial.println("No solution!");   //25^C 1413us/cm<-->about 216mv  if the voltage(compensate)< 150,that is <1ms/cm,out of the range
    else if(CoefficientVolatge>3300)
      Serial.println("Out of the range!");  //>20ms/cm,out of the range
    else
    { 
      if(CoefficientVolatge<=448)
        //ECcurrent=6.84*CoefficientVolatge-64.32;   //1ms/cm<EC<=3ms/cm
        ECcurrent=7.6*CoefficientVolatge-64.32;   //1ms/cm<EC<=3ms/cm
      else if(CoefficientVolatge<=1457)
        ECcurrent=6.98*CoefficientVolatge-127;  //3ms/cm<EC<=10ms/cm
      else 
      ECcurrent=5.3*CoefficientVolatge+2278;                           //10ms/cm<EC<20ms/cm
      
      ECcurrent/=1000;    //convert us/cm to ms/cm
      
      Serial.println("CoefficientVolatge");
       Serial.print(CoefficientVolatge);
       Serial.println("  ");
      Serial.print(ECcurrent,2);  //two decimal
      Serial.println("ms/cm");
    }
  }

  t.update();
  
  sensor_Read(); // pH Reading

  Serial.print("pH = " );                       
  Serial.print(pH,1);      
  Serial.print("   ADC = " );                       
  Serial.println(sensorValue);

  lcd.setCursor(0, 1);
  lcd.print("pH : ");
  lcd.print(pH,1);
  lcd.print("        ");
  lcd.setCursor(0, 2);


    lcd.print("EC : ");
    lcd.print(ECcurrent,1);
    lcd.print(" mS/cm ");

  lcd.setCursor(0, 3);
  lcd.print("Temp : ");
  lcd.print(temperature,1);
  lcd.print(" C");  
  delay(100);
     
}

void doSomething() // do after tick 
{
  sensor_Read();
  //while(sensorValue<=515)  // pH = 6.2
  while(pH>6.2)  // pH = 6.2
  {
    
    float ml_feed=0;
    float pump_time=0;
    ml_feed = (pH - 6.0)/0.009;
    pump_time = ml_feed/pump_feed;
    
    
      Serial.print("pH = " );                       
      Serial.print(pH,1);      
      Serial.print("   ADC = " );                       
      Serial.println(sensorValue);
      Serial.print("Feed water = ");
      Serial.print(ml_feed);
      Serial.print("\r\n");
      Serial.print("Pump time = ");
      Serial.print((int)pump_time);
      Serial.print("\r\n");
      
      digitalWrite(7, LOW);
      lcd.setCursor(0, 1);
      lcd.print("     Pump  ON      ");
      
      for(int k=0;k<pump_time;k++)
       {
          lcd.setCursor(0, 2);
          lcd.print("Time Left ");
          lcd.print((int)pump_time-k);
          lcd.print(" s       ");
          lcd.setCursor(0, 3);
          lcd.print("                   ");
          delay(1000); 
       }
      digitalWrite(7, HIGH);
      
      // wait for mixing time
      Serial.print("Mixing....");
      Serial.print("\r\n");
      
      lcd.setCursor(0, 1);
      lcd.print("     Mixing Mode    ");
  
      int time_sec=20;  // Mixing time adjust
       for(int k=0;k<time_sec;k++)
       {
         lcd.setCursor(0, 2);
          lcd.print("Time Left ");
          lcd.print(time_sec-k);
          lcd.print(" s       ");
          lcd.setCursor(0, 3);
          lcd.print("                   ");  
        delay(1000); 
       }
     Serial.print("Finished Mixing");        
     sensor_Read();
  }
  Serial.print("pH Ready...");
}

/*
ch=0,let the DS18B20 start the convert;ch=1,MCU read the current temperature from the DS18B20.
*/
float TempProcess(bool ch)
{
  //returns the temperature from one DS18B20 in DEG Celsius
  static byte data[12];
  static byte addr[8];
  static float TemperatureSum;
  if(!ch){
          if ( !ds.search(addr)) {
              Serial.println("no more sensors on chain, reset search!");
              ds.reset_search();
              return 0;
          }      
          if ( OneWire::crc8( addr, 7) != addr[7]) {
              Serial.println("CRC is not valid!");
              return 0;
          }        
          if ( addr[0] != 0x10 && addr[0] != 0x28) {
              Serial.print("Device is not recognized!");
              return 0;
          }      
          ds.reset();
          ds.select(addr);
          ds.write(0x44,1); // start conversion, with parasite power on at the end
  }
  else{  
          byte present = ds.reset();
          ds.select(addr);    
          ds.write(0xBE); // Read Scratchpad            
          for (int i = 0; i < 9; i++) { // we need 9 bytes
            data[i] = ds.read();
          }         
          ds.reset_search();           
          byte MSB = data[1];
          byte LSB = data[0];        
          float tempRead = ((MSB << 8) | LSB); //using two's compliment
          TemperatureSum = tempRead / 16;
    }
          return TemperatureSum;  
}


