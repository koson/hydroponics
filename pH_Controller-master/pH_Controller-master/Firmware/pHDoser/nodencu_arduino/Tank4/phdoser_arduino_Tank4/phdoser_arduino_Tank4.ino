#include <SoftwareSerial.h>
#include <SerialTransceiver.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Timer.h>

int coo;
Timer t;
Timer t2;

int time_min;
int time_sec;
int min_set_inteval;
int sd_init=0;

const char* ssid = "BearingHydroponics";
const char* password = "stech1412";

// Config MQTT Server
#define mqtt_server "m10.cloudmqtt.com"
#define mqtt_port 14598
#define mqtt_user "dpimlfme"
#define mqtt_password "XAEx91AVVa0m"

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


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A0, INPUT);


 mySerial.begin(115200);
 
  int light_lux;
  int tickEvent = t.every(1000, doSomething);
//  int tickEvent2 = t2.every(300, doSomething2);   // Update Monitor

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
  client.setCallback(callback);

 delay(5000);
}



void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String msg = "";
  int i = 0;
  while (i < length) msg += (char)payload[i++];
  if (msg == "GET")
  {
    char u[10] = "test1233";
    //client.publish("/bearinghydro/temp", thisString);
    Serial.println("Send !");
    return;
  }

  Serial.println(msg);

}



void loop()
{
  
      if (mySerial.available())
    {
      rx_buff = mySerial.read();
       Serial.print(rx_buff);
    
      if(rx_buff != '\n')
      {
        text_data[lenght_rx] = rx_buff;
        lenght_rx++;
        //if(lenght_rx>=9)
        //lenght_rx = 9;
        if(lenght_rx>=13)
        lenght_rx = 13;
      }
      
      else // End of Line
      {
        // Print all data
        // for(int i=0;i<=lenght_rx;i++)
         // Serial.print(text_data[i]);

           
        if(text_data[3] == '.')
        {   
          pH_Read = (text_data[1]-48)*10 + (text_data[2]-48) + (text_data[4]-48)*0.1;
          Temp_Read = (text_data[6]-48)*10 + (text_data[7]-48);
          
        }
                   
        else if(text_data[2] == '.')
        {   
          pH_Read = (text_data[1]-48) + (text_data[3]-48)*0.1;
          Temp_Read = (text_data[5]-48)*10 + (text_data[6]-48);
          
        }

        int c_position=0;
        int d_position=0;
        
        for(int i=0;i<lenght_rx;i++)
        {
           if(text_data[i] == 'C')
           {
            c_position = i;
             //Serial.print("C position = ");
             //Serial.println(c_position);
           }
        }

        for(int i=0;i<lenght_rx;i++)
        {
           if(text_data[i] == 'D')
           {
            d_position = i;
             //Serial.print("D position = ");
             //Serial.println(d_position);
           }
        }

        //A7.2B27C0D

        int unit_feed;

        unit_feed = d_position - c_position;
        if(unit_feed == 2)
        {
          
          Serial.println("1");
          Serial.print("Data is : ");
          Serial.println(text_data[c_position+1]);
          feed_Read = (text_data[c_position+1]-48);
        }
        else if(unit_feed == 3)
        {
          Serial.println("2"); 
          Serial.print("Data is : ");
          Serial.print(text_data[c_position+1]); 
          Serial.println(text_data[c_position+2]);
          
          feed_Read = (text_data[c_position+1]-48)*10 + (text_data[c_position+2]-48);
        }
        else if(unit_feed == 4)
        {
          Serial.println("3");
          Serial.print("Data is : ");
          Serial.print(text_data[c_position+1]); 
          Serial.print(text_data[c_position+2]); 
          Serial.println(text_data[c_position+3]); 
          
          feed_Read = (text_data[c_position+1]-48)*100 + (text_data[c_position+2]-48)*10 + (text_data[c_position+3]-48);
        }
        else if(unit_feed == 5)
        {
          Serial.println("4"); 
          Serial.print("Data is : ");
          Serial.print(text_data[c_position+1]); 
          Serial.print(text_data[c_position+2]); 
          Serial.print(text_data[c_position+3]); 
          Serial.println(text_data[c_position+4]);
          
          feed_Read = (text_data[c_position+1]-48)*1000 + (text_data[c_position+2]-48)*100 + (text_data[c_position+3]-48)*10 + (text_data[c_position+4]-48);
        }
        
                 
        /*Serial.println(text_data[8]);
        Serial.println(text_data[9]);
        Serial.println(text_data[10]);
        Serial.println(text_data[11]);
        Serial.println("");*/
        
        lenght_rx=0;
      }
    }

    
     // Serial.println(pH_Read);
       //   Serial.println(Temp_Read);  

 /*
  //Serial.print("WIFI :");
  if (WiFi.status() == 3)  // Connected to WIFI
  {
   // Serial.println("WiFi MODE");
   // Serial.print("IP address: ");
    //Serial.println(WiFi.localIP()); 
   
    try_to_connect = 0;
    // try to connect MQTT
    if (!client.connected())
    {
      //Serial.print("Attempting MQTT connection...");
      if (client.connect("BearingHydroponics", mqtt_user, mqtt_password))
      {
        //Serial.println("connected");
        digitalWrite(LED_BUILTIN, LOW);
      } 
      else
      {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
        try_to_connect++;
        //return;
      }
    }
  }
  else
  {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED && try_to_connect > 20)
    {
      try_to_connect++;
      delay(500);
      Serial.print(".");
      Serial.print("Try to connecting...");
      Serial.println(try_to_connect);
    }

    //Serial.println("OFFLINE MODE");
    
  }*/


   //Serial.print("MQQT Status: ");
  // if(client.connected() == 1)
  //  Serial.println("Connected");
   //else
   // Serial.println("FAIL");

  t.update();



  // Update and Send data to MQTT and Firebase 
  if (time_min == 0  && time_sec == 20)
  {

    time_min = 0;
    time_sec = 0;
 

          
   if(WiFi.status() == WL_CONNECTED)
    {
      if (client.connect("BearingHydroponics", mqtt_user, mqtt_password))
      {
        if (!isnan(pH_Read) && !isnan(Temp_Read))
        {

          pHStr =  String(pH_Read);
          pHStr.toCharArray(pH_water, pHStr.length() + 1);
          TempStr = String(Temp_Read);
          TempStr.toCharArray(temp_water, TempStr.length() + 1);   
          FeedStr =  String(feed_Read);
          FeedStr.toCharArray(feed_ph, FeedStr.length() + 1); 
          
          client.publish("/tank4/ph", pH_water);
          delay(10);
          client.publish("/tank4/temperature", temp_water);
          delay(10);
          client.publish("/tank4/feedml", feed_ph);
          delay(10);
          // /bearinghydro/feed_crop1

          Serial.print("pH : ");
          Serial.println(pH_water);
          Serial.print("Temp : ");
          Serial.println(temp_water);
          Serial.print("Feed : ");
          Serial.println(feed_ph);
          Serial.println("Send data to MQTT");
        }
      }
    }
    
  client.loop();
  delay(3000);

  }

}

void doSomething()
{
  time_sec++;

  if (time_sec > 59)
  {
    time_min++;
    time_sec = 0;
  }
  /*Serial.print("Time : ");
    Serial.print(time_min);
    Serial.print(":");
    Serial.println(time_sec);*/
}



