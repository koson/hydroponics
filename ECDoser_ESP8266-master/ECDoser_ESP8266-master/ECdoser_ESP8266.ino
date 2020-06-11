#include <SoftwareSerial.h>
#include <SerialTransceiver.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Timer.h>
#include <Ticker.h>

//int coo;
Timer t;
//Timer t2;
Ticker secondTick;

volatile int watchDogCount =0;

int time_min;
int time_sec;
int min_set_inteval;
int sd_init=0;

//const char* ssid = "BearingHydroponics";
//const char* password = "stech1412";

const char* ssid = "TP-LINK_2A83FC";
const char* password = "33762746";

// Config MQTT Server
#define mqtt_server "m10.cloudmqtt.com"
#define mqtt_port 14598
#define mqtt_user "dpimlfme"
#define mqtt_password "XAEx91AVVa0m"

// Config MQTT Server
//#define mqtt_server "192.168.1.101"
//#define mqtt_port 1883
//#define mqtt_user "pi"
//#define mqtt_password "rsec2t"

#define LED_PIN 2


float env_temp;
String tempStr;
char temp[10];

float env_humid;
String humidStr;
char humid[10];

String pHStr;
String TempStr;
String FeedStr;

char pH_water[10];
char temp_water[10];
char feed_ph[10];

WiFiClient espClient;
PubSubClient client(espClient);

char rx_buff;
char text_data[14];
int lenght_rx=0;
float pH_Read;
int Temp_Read;
int feed_Read;
int a;
char a_input;
int try_to_connect=0;
float temper;


SoftwareSerial mySerial(13, 15); // RX2, TX2


void ISRWatchDogTimer()
{
  Serial.println("ISR watchdog tick");
  Serial.print("Watchdog sec : ");
  Serial.println(watchDogCount);
  watchDogCount++;
  if(watchDogCount > 40)
    ESP.reset();
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(A0, INPUT);
 // secondTick.attach(30,ISRWatchDogTimer);

  ESP.wdtDisable(); 
  ESP.wdtEnable(WDTO_8S);
  
   mySerial.begin(115200);
   
  int light_lux;
  int tickEvent = t.every(1000, doSomething);

  time_min = 0;
  time_sec = 0;

  min_set_inteval = 20;   // min
  Serial.begin(115200);
  delay(10);

  int status_con = 0;


  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED && status_con <= 10)
  {
    delay(200);
    Serial.print(".");
    status_con++;
  }

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);

 delay(5000);
}

void loop()
{  

    ESP.wdtFeed();
      if (mySerial.available())
      {
          rx_buff = mySerial.read();
          Serial.print(rx_buff);
    
      if(rx_buff != '\n')
      {
        text_data[lenght_rx] = rx_buff;
        lenght_rx++;
        if(lenght_rx>=13)
        lenght_rx = 13;
      }
      
      else // End of Line
      {
        int a_index;
        int b_index;
        int c_index;
        int d_index;

        int ph_unit;
        int temp_unit;
        int feed_unit;
              
     

        // search 'A'
        for(int i=0;i<lenght_rx;i++)
        {
         if(text_data[i] == 'A')
           a_index = i;
        }
        // search 'B'
        for(int i=0;i<lenght_rx;i++)
        {
         if(text_data[i] == 'B')
           b_index = i;
        }

        // search 'C'
        for(int i=0;i<lenght_rx;i++)
        {
         if(text_data[i] == 'C')
           c_index = i;
        }
        // search 'D'
        for(int i=0;i<lenght_rx;i++)
        {
         if(text_data[i] == 'D')
           d_index = i;
        }

        ph_unit = b_index - a_index;
        temp_unit = c_index - b_index;
        feed_unit = d_index - c_index;
               
        // pH convert char to float data
        if(ph_unit == 4)   // A6.5B
        {
          if(text_data[a_index+1] >= 48 && text_data[a_index+1] <= 57 && text_data[a_index+3] >= 48 && text_data[a_index+3] <= 57)
            pH_Read = (text_data[a_index+1]-48) + (text_data[a_index+3]-48)*0.1;
        }
        else if(ph_unit == 5)  // A12.2B
        {
          if(text_data[a_index+1] >= 48 && text_data[a_index+1] <= 57 && text_data[a_index+2] >= 48 && text_data[a_index+2] <= 57 && text_data[a_index+4] >= 48 && text_data[a_index+4] <= 57)
            pH_Read = (text_data[a_index+1]-48)*10 + (text_data[a_index+2]-48) + (text_data[a_index+4]-48)*0.1;
        }
        else
          pH_Read = 0; // error  
                  
        // Temperature convert char to float data
        if(temp_unit == 3)      //B21C
          if(text_data[b_index+1] >= 48 && text_data[b_index+1] <= 57 && text_data[b_index+2] >= 48 && text_data[b_index+2] <= 57)
            Temp_Read = (text_data[b_index+1]-48)*10 + (text_data[b_index+2]-48);
        else  //B1xcC
          Temp_Read = 0; // error  

        if(feed_unit == 2)
          feed_Read = (text_data[c_index+1]-48);
        else if(feed_unit == 3)
          feed_Read = (text_data[c_index+1]-48)*10 + (text_data[c_index+2]-48);
        else if(feed_unit == 4)
          feed_Read =  (text_data[c_index+1]-48)*100 + (text_data[c_index+2]-48)*10 + (text_data[c_index+3]-48);
        else if(feed_unit == 5)
          feed_Read =  (text_data[c_index+1]-48)*1000 + (text_data[c_index+2]-48)*100 + (text_data[c_index+3]-48)*10 + (text_data[c_index+4]-48);
        else 
          feed_Read = 0; // error
        
        lenght_rx=0;
      }
    }// recieved serial from uC PIC

  if (WiFi.status() == 3)  // Connected to WIFI
  {
    try_to_connect = 0;
    if (!client.connected())
    {   
      if (client.connect("BearingHydroponics", mqtt_user, mqtt_password))
      {
        digitalWrite(LED_BUILTIN, LOW);
      } 
      else
      {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(5000);
        try_to_connect++;
      }
    }
  }
  else
  {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED && try_to_connect > 25)
    {
      try_to_connect++;
      delay(500);
    }
  }
  
  t.update();


/*
pH_Read = pH_Read + 0.1;
Temp_Read = Temp_Read +1;

if(pH_Read > 14)
pH_Read = 0;
if(Temp_Read > 50)
Temp_Read = 0;*/


  // Update and Send data to MQTT 
  if (time_min == 0  && time_sec == 20)
  {

    time_min = 0;
    time_sec = 0;
 
     
          
   if(WiFi.status() == WL_CONNECTED)
    {
      if (client.connect("BearingHydroponics", mqtt_user, mqtt_password))
      {
        if (!isnan(pH_Read))
        {
          pHStr =  String(pH_Read);
          pHStr.toCharArray(pH_water, pHStr.length() + 1);       
          client.publish("/melon/ec", pH_water);
          Serial.print("pH : ");
          Serial.println(pH_water);
          Serial.println("Send data to MQTT");      
          watchDogCount=0;  
        }
      }
    }
    
  //client.loop();
  delay(100);



    if(WiFi.status() == WL_CONNECTED)
    {
      if (client.connect("BearingHydroponics", mqtt_user, mqtt_password))
      {
        if (!isnan(Temp_Read))
        {
          TempStr = String(Temp_Read);
          TempStr.toCharArray(temp_water, TempStr.length() + 1);   
          client.publish("/melon/ECtemperature", temp_water);
          Serial.print("Temp : ");
          Serial.println(temp_water);     
          Serial.println("Send data to MQTT");          
          watchDogCount=0;
        }
      }
    }    
  //client.loop();
  delay(100);

    if(WiFi.status() == WL_CONNECTED)
    {
      if (client.connect("BearingHydroponics", mqtt_user, mqtt_password))
      {
        if (!isnan(feed_Read))
        {
          FeedStr =  String(feed_Read);
          FeedStr.toCharArray(feed_ph, FeedStr.length() + 1);         
          client.publish("/melon/feedec", feed_ph);
          Serial.print("Feed : ");
          Serial.println(feed_ph);
          Serial.println("Send data to MQTT");
          watchDogCount=0;        
        }
      }
    }
    
  client.loop();
  delay(100);

  
  
  } 


}

void doSomething()
{
  time_sec++;

 Serial.println("ISR watchdog tick");
  Serial.print("Watchdog sec : ");
  Serial.println(watchDogCount);
  watchDogCount++;
  if(watchDogCount > 40)
  {
    ESP.reset();   
    WiFi.begin(ssid, password);
    client.setServer(mqtt_server, mqtt_port);
  }
    
  if (time_sec > 59)
  {
    time_min++;
    time_sec = 0;
  }
}



