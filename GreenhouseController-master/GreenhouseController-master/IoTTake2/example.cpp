#include <stdio.h>

#include <assert.h>
#include <errno.h>

#include <stdint.h>
#include <cstdint>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <exception>
#include "mraa.hpp"
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <math.h>
#include <mutex>          // std::mutex
//#include "greenhouse_mqtt.h"
#include <ctime>

#include <stdio.h>
#include <mosquitto.h>
#include <unistd.h> // sleep mS

#include "crc16.c"

using namespace std;
using namespace mraa;

double temp_weatherStation;
double humid_weatherStation;

int co2_read;
int co2_read_old;
int humid_read;
int humid_read_old;
int temperature_read;
int temperature_read_old;

int co2_scale;
int temp_scale;
int humid_scale;

int temp_nutrient;
double ph_nutrient;
double ec_nutrient;

int temp_nutrientSP;
double ph_nutrientSP;
double ec_nutrientSP;

double greenhouse_temp;
double greenhouse_humid;
int greenhouse_co2;

double greenhouse_tempSP=28;
double greenhouse_humidSP=70;
int greenhouse_light;
int greenhouse_co2SP;

// FAN Status
int fan1_on = 0;
int fan2_on = 0;
int fan3_on = 0;
int fan4_on = 0;
int fan_auto = 0;

char rs485Tx[8];
char rs232Tx[8];

int crop_date=0;
int system_year;
int system_month;
int system_day;

int system_hour;
int system_min;
int system_sec;

int state_read=0;

void* thread1(void* data);
void* thread2(void* data);
void* thread3(void* data);  // ph read

struct mosquitto *mosq = NULL;

int time_baseSec=0;
int time_baseMin=0;
int time_baseHour=0;

// Fogger Control
int fogger_Inteval = 1;  // minute
int fogger_onSec = 20;  // second
int fogger_auto = 0;
int fogger_ON=0;

mraa::Uart* rs485;
mraa::Uart* rs232;

std::mutex mtx;           // mutex for critical section
std::mutex mtx2;           // mutex for critical section
std::mutex mtx3;           // mutex for send data rs485


int delayOn_Fan=0;
int ThesholdFrame_Error=800;

/*Gpio* d_pin0 = NULL;
Gpio* d_pin1 = NULL;*/
Gpio* d_pin2 = NULL;
Gpio* d_pin3 = NULL;
Gpio* d_pin4 = NULL;
Gpio* d_pin5 = NULL;
Gpio* d_pin6 = NULL;
Gpio* d_pin7 = NULL;

int co2_error=0;
int light_error=0;
int temphumid_error=0;

int watt_error=0;

int tempErrorCount;

int fog_inteval_Min;
int fog_inteval_Sec;
int fog_TimeOn_Min;
int fog_TimeOn_Sec;
int fog_on=0;

int fan1_ON=0;
int fan2_ON=0;


int fan1_speed=0;
int fan2_speed=0;

int coolingPad_on=0;
int coolingPad_auto=0;

float sdm_watt=0;

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen)
	{

		 std::string BuffString = std::string((char*)message->payload);
		  //float f = s.toFloat();

		//printf("topic: %s data: %s\n", message->topic, message->payload);
		if(strcmp (message->topic,"/ambient_temperature")  == 0)
		{
			temp_weatherStation = atof(BuffString.c_str());
			printf("ambient temp. is : %.2f\r\n",temp_weatherStation);


		}
		else if(strcmp (message->topic,"/ambient_humidity")  == 0)
		{
			humid_weatherStation = atof(BuffString.c_str());
			printf("ambient humidity is : %.2f\r\n",humid_weatherStation);
		}

	}
	else
	{
		printf("%s (null)\n", message->topic);
	}
	fflush(stdout);
}


void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;
	if(!result)
	{
		mosquitto_subscribe(mosq, NULL, "/ambient_temperature", 0);
		mosquitto_subscribe(mosq, NULL, "/ambient_humidity", 0);
	}
	else
	{
		fprintf(stderr, "Connect failed\n");
	}
}

void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++)
	{
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	printf("%s\n", str);
}


void SetStatusFAN()
{
	if(fan1_ON == 1)
	{
		rs232->writeStr("b51.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b52.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("p17.pic=20");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF

	}
	else if (fan1_ON == 0)
	{
		rs232->writeStr("b51.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b52.bco=RED");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("p17.pic=19");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF

	}
	if(fan2_ON == 1)
	{
		rs232->writeStr("b53.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b54.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("p18.pic=20");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF

	}
	else if(fan2_ON == 0)
	{
		rs232->writeStr("b53.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b54.bco=RED");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("p18.pic=19");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
	}

	if(fan_auto == 0)
	{
		rs232->writeStr("b64.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b63.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF

	}
	else if(fan_auto == 1)
	{
		rs232->writeStr("b64.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b63.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF

	}

	char *SetFAN1_txtBar  = "n81.val=";
	rs232->writeStr(SetFAN1_txtBar);
	std::string data4 = to_string(fan1_speed);
	rs232->writeStr(data4);
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF

	char *SetFAN2_txtBar  = "n82.val=";
	rs232->writeStr(SetFAN2_txtBar);
	std::string data5 = to_string(fan2_speed);
	rs232->writeStr(data5);
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF



}

void SetCO2_HMI()
{
	// Waveform
	char *SetCO2_waveform  = "add 20,0,";     // add id,channel,data
	rs232Tx[0] = 255;  // ÿ
	char dQuote[10];   // "
	dQuote[0] = 34;

	rs232->writeStr(SetCO2_waveform);
	co2_scale = co2_read*0.051;			// Scale to 0-255
	std::string data1 = to_string(co2_scale);
	rs232->writeStr(data1);
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF

	// CO2 text on Overview page
	char *SetCO2_text  = "t6.txt=";
	rs232->writeStr(SetCO2_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(co2_read);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF

	// CO2 progress bar
	char *SetCO2_bar  = "j0.val=";
	rs232->writeStr(SetCO2_bar);
	std::string data3 = to_string(co2_scale);
	rs232->writeStr(data3);
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF

	char *SetCO2_txtBar  = "n50.val=";
	rs232->writeStr(SetCO2_txtBar);
	std::string data4 = to_string(co2_read);
	rs232->writeStr(data4);
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
}

void SetTempAmbient_HMI()
{
	char dQuote[10];   // "
	dQuote[0] = 34;

	char *SetTemperature_text  = "t8.txt=";
	rs232->writeStr(SetTemperature_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(temp_weatherStation);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
}


void SetpH_HMI()
{
	char dQuote[10];   // "
	dQuote[0] = 34;

	char *SetpH_text  = "t11.txt=";
	rs232->writeStr(SetpH_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(ph_nutrient);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
}

void SetEC_HMI()
{
	char dQuote[10];   // "
	dQuote[0] = 34;

	char *SetEC_text  = "t10.txt=";
	rs232->writeStr(SetEC_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(ec_nutrient);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
}

void SetECTemp_HMI()
{
	char dQuote[10];   // "
	dQuote[0] = 34;

	char *SetECTemp_text  = "t12.txt=";
	rs232->writeStr(SetECTemp_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(temp_nutrient);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
}

void SetHumidAmbient_HMI()
{
	char dQuote[10];   // "
	dQuote[0] = 34;

	char *SetTemperature_text  = "t9.txt=";
	rs232->writeStr(SetTemperature_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(humid_weatherStation);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
}

void SetTemperature_HMI()
{
	rs232Tx[0] = 255;  // ÿ
	char dQuote[10];   // "
	dQuote[0] = 34;

	// Temperature text on Overview page
	char *SetTemperature_text  = "t4.txt=";
	rs232->writeStr(SetTemperature_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(greenhouse_temp);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF

	// Waveform
	char *SetTemp_waveform  = "add 24,0,";     // add id,channel,data
	rs232->writeStr(SetTemp_waveform);
	temp_scale = greenhouse_temp*0.9775;			// Scale to 0-255
	std::string data1 = to_string((int)temp_scale);
	rs232->writeStr(data1);
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF

}

void SetHumid_HMI()
{
	rs232Tx[0] = 255;  // ÿ
	char dQuote[10];   // "
	dQuote[0] = 34;

	// humid text on Overview page
	char *SetTemperature_text  = "t5.txt=";
	rs232->writeStr(SetTemperature_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(greenhouse_humid);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF

	// Waveform
	char *SetHumid_waveform  = "add 29,0,";     // add id,channel,data
	rs232->writeStr(SetHumid_waveform);
	temp_scale = greenhouse_temp*2.55;			// Scale to 0-255
	std::string data1 = to_string((int)greenhouse_humid);
	rs232->writeStr(data1);
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
}

void SetLight_HMI()
{
	rs232Tx[0] = 255;  // ÿ
	char dQuote[10];   // "
	dQuote[0] = 34;

	// Light(Lux) text on Overview page
	char *SetTemperature_text  = "t7.txt=";
	rs232->writeStr(SetTemperature_text);
	rs232->write(dQuote,1); // "
	std::string data2 = to_string(greenhouse_light);
	rs232->writeStr(data2);
	rs232->write(dQuote,1); // "
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
	rs232->write(rs232Tx,1); // 0xFF
}

void SetDate_HMI(void)
{
	   // current date/time based on current system
	   time_t now = time(0);
	   std::string time_data;
	   rs232Tx[0] = 255;  // ÿ

	   tm *ltm = localtime(&now);
    /*cout << "Year" << 1900 + ltm->tm_year<<endl;
    cout << "Month: "<< 1 + ltm->tm_mon<< endl;
    cout << "Day: "<<  ltm->tm_mday << endl;
    cout << "Time: "<< ltm->tm_hour << ":";
    cout << ltm->tm_min << ":";
    cout << ltm->tm_sec << endl;*/

		// Year
		char *time_year  = "n5.val=";
		rs232->writeStr(time_year);
		time_data = to_string(1900 + ltm->tm_year);
		rs232->writeStr(time_data);
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		// Month
		char *time_month  = "n4.val=";
		rs232->writeStr(time_month);
		time_data = to_string(1 + ltm->tm_mon);
		rs232->writeStr(time_data);
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		// Day
		char *time_day  = "n3.val=";
		rs232->writeStr(time_day);
		time_data = to_string(ltm->tm_mday);
		rs232->writeStr(time_data);
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
    // Time Hour
		char *time_hour  = "n0.val=";
		rs232->writeStr(time_hour);
		time_data = to_string(ltm->tm_hour);
		rs232->writeStr(time_data);
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		// Time sec
		char *time_min  = "n1.val=";
		rs232->writeStr(time_min);
		time_data = to_string(ltm->tm_min);
		rs232->writeStr(time_data);
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		// Time sec
		char *time_sec  = "n2.val=";
		rs232->writeStr(time_sec);
		time_data = to_string(ltm->tm_sec);
		rs232->writeStr(time_data);
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
}

void SetFoggerStatus()
{
	if(fogger_auto == 0)
	{
		rs232->writeStr("b19.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b18.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF

	}
	else if(fogger_auto == 1)
	{
		rs232->writeStr("b18.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b19.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
	}
	if(fogger_ON == 0)
	{
		rs232->writeStr("b20.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b21.bco=RED");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
	}
	else if(fogger_ON == 1)
	{
		rs232->writeStr("b20.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b21.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
	}
}

void SetCoolingPadStatus()
{
	if(coolingPad_auto == 0)
	{
		rs232->writeStr("b15.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b14.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF

	}
	else if(coolingPad_auto == 1)
	{
		rs232->writeStr("b14.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b15.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
	}
	if(coolingPad_on == 0)
	{
		rs232->writeStr("b16.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b17.bco=RED");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
	}
	else if(coolingPad_on == 1)
	{
		rs232->writeStr("b16.bco=GREEN");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->writeStr("b17.bco=48631");
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
		rs232->write(rs232Tx,1); // 0xFF
	}
}


void ReadpH_485()
{
     rs485Tx[0] = 'A';
     rs485Tx[1] = 'R';
     rs485Tx[2] = 'j';
     rs485->write(rs485Tx,3);
}
void ReadEC_485()
{
    rs485Tx[0] = 'B';	// Light, Humidity and light Address
    rs485Tx[1] = 'R';
    rs485Tx[2] = 'j';
    rs485->write(rs485Tx,3);
}
void ReadTempEC_485()
{
     rs485Tx[0] = 'B';
     rs485Tx[1] = 'T';
     rs485Tx[2] = 'j';
     rs485->write(rs485Tx,3);
}
// function 04
void ReadSDM_Watt()
{
    rs485Tx[0] = 3;		//  Address
    rs485Tx[1] = 4; 	// Function mode
    rs485Tx[2] = 0;		// Data
    rs485Tx[3] = 12;	// Data
    rs485Tx[4] = 0;		// Data
    rs485Tx[5] = 2;		// Data
    rs485Tx[6] = 44;	// High CRC Modbus
    rs485Tx[7] = 208;	// Low CRC Modbus
    rs485->write(rs485Tx,8);

}
/*
// function 03
void ReadSDM_Watt()
{
    rs485Tx[0] = 3;		//  Address
    rs485Tx[1] = 3; 	// Function mode
    rs485Tx[2] = 0;		// Data
    rs485Tx[3] = 12;	// Data
    rs485Tx[4] = 0;		// Data
    rs485Tx[5] = 2;		// Data
    rs485Tx[6] = 230;	// High CRC Modbus
    rs485Tx[7] = 101;	// Low CRC Modbus
    rs485->write(rs485Tx,8);

}*/


void ReadCO2_485()
{
     rs485Tx[0] = 1;	// CO2 Address
     rs485Tx[1] = 3;
     rs485Tx[2] = 0;
     rs485Tx[3] = 0;
     rs485Tx[4] = 0;
     rs485Tx[5] = 2;
     rs485Tx[6] = 196;
     rs485Tx[7] = 11;
     rs485->write(rs485Tx,8);
}

void ReadTempNHumid_485()
{
     rs485Tx[0] = 2;	// Light, Humidity and light Address
     rs485Tx[1] = 3;
     rs485Tx[2] = 0;
     rs485Tx[3] = 0;
     rs485Tx[4] = 0;
     rs485Tx[5] = 2;
     rs485Tx[6] = 196;
     rs485Tx[7] = 56;
     rs485->write(rs485Tx,8);
}

void ReadLight_485()
{
     rs485Tx[0] = 2;	// Light, Humidity and light Address
     rs485Tx[1] = 3;
     rs485Tx[2] = 0;
     rs485Tx[3] = 6;
     rs485Tx[4] = 0;
     rs485Tx[5] = 1;
     rs485Tx[6] = 100;
     rs485Tx[7] = 56;
     rs485->write(rs485Tx,8);
}

void fan1_run(int fan1_on)
{
	uint16_t fan1_on_h = (0xFF00&fan1_on)>>8;
	uint16_t fan1_on_l = fan1_on& 0XFF;

	//printf("fan1 check sum   %d \r\n",fan1_on);
	unsigned char input_str_test[6];
	input_str_test[0] = 10;
	input_str_test[1] = 6;
	input_str_test[2] = 32;
	input_str_test[3] = 1;
	input_str_test[4] = fan1_on_h;
	input_str_test[5] = fan1_on_l;

	uint16_t chk_sum = crc_modbus(input_str_test,6);
	uint16_t aa = chk_sum & 0XFF;
	uint16_t bb =  (0xFF00&chk_sum)>>8;

	rs485Tx[0] = 10;
	rs485Tx[1] = 6;
	rs485Tx[2] = 32;
	rs485Tx[3] = 1;
	rs485Tx[4] = fan1_on_h;
	rs485Tx[5] = fan1_on_l;
	rs485Tx[6] = aa; //215
	rs485Tx[7] = bb; //57
	rs485->write(rs485Tx,8);

	//printf("fan1 check sum   %d %d\r\n",aa,bb);

	usleep(500000);
    rs485Tx[0] = 10;	// FAN 1 ADDRESS
    rs485Tx[1] = 6;
    rs485Tx[2] = 32;
    rs485Tx[3] = 0;
    rs485Tx[4] = 0;
    rs485Tx[5] = 10;
    rs485Tx[6] = 3;
    rs485Tx[7] = 118;
    rs485->write(rs485Tx,8);
}
void fan2_run(int fan2_on)
{
	uint16_t fan2_on_h = (0xFF00&fan2_on)>>8;
	uint16_t fan2_on_l = fan2_on& 0XFF;

	//printf("fan1 check sum   %d \r\n",fan1_on);

	unsigned char input_str_test[6];
	input_str_test[0] = 11;
	input_str_test[1] = 6;
	input_str_test[2] = 32;
	input_str_test[3] = 1;
	input_str_test[4] = fan2_on_h;
	input_str_test[5] = fan2_on_l;

	uint16_t chk_sum = crc_modbus(input_str_test,6);
	uint16_t aa = chk_sum & 0XFF;
	uint16_t bb =  (0xFF00&chk_sum)>>8;

	rs485Tx[0] = 11;
	rs485Tx[1] = 6;
	rs485Tx[2] = 32;
	rs485Tx[3] = 1;
	rs485Tx[4] = fan2_on_h;
	rs485Tx[5] = fan2_on_l;
	rs485Tx[6] = aa; //215
	rs485Tx[7] = bb; //57
	rs485->write(rs485Tx,8);

	//printf("fan1 check sum   %d %d\r\n",aa,bb);

	usleep(500000);

	rs485Tx[0] = 11;	// FAN 2 ADDRESS
	rs485Tx[1] = 6;
	rs485Tx[2] = 32;
	rs485Tx[3] = 0;
	rs485Tx[4] = 0;
	rs485Tx[5] = 10;
	rs485Tx[6] = 2;
	rs485Tx[7] = 167;
	rs485->write(rs485Tx,8);
}



void fan1_stop()
{
     rs485Tx[0] = 10;
     rs485Tx[1] = 6;
     rs485Tx[2] = 32;
     rs485Tx[3] = 0;
     rs485Tx[4] = 0;
     rs485Tx[5] = 9;
     rs485Tx[6] = 67;
     rs485Tx[7] = 119;
     rs485->write(rs485Tx,8);
}
void fan2_stop()
{
     rs485Tx[0] = 11;
     rs485Tx[1] = 6;
     rs485Tx[2] = 32;
     rs485Tx[3] = 0;
     rs485Tx[4] = 0;
     rs485Tx[5] = 9;
     rs485Tx[6] = 66;
     rs485Tx[7] = 166;
     rs485->write(rs485Tx,8);
}

void fogger_on()
{
	d_pin4->write(1);
	fogger_ON = 1;
}
void fogger_off()
{
	d_pin4->write(0);
	fogger_ON = 0;
}


void coolingpad_on()
{
	d_pin3->write(1);
	coolingPad_on = 1;
}

void coolingpad_off()
{
	d_pin3->write(0);
	coolingPad_on = 0;
}


int main(void) {
	// create threads
		pthread_t threadOne;
		pthread_t threadTwo;
		pthread_t threadThree;
		int data = 0;


		/*d_pin0 = new mraa::Gpio(0, true, false);
		d_pin1 = new mraa::Gpio(1, true, false);*/
		d_pin2 = new mraa::Gpio(2, true, false);
		d_pin3 = new mraa::Gpio(3, true, false);
		d_pin4 = new mraa::Gpio(4, true, false);
		d_pin5 = new mraa::Gpio(5, true, false);
		d_pin6 = new mraa::Gpio(6, true, false);
		d_pin7 = new mraa::Gpio(7, true, false);



    	/*d_pin0->dir(mraa::DIR_OUT);
    	d_pin1->dir(mraa::DIR_OUT);*/
    	d_pin2->dir(mraa::DIR_OUT);
    	d_pin3->dir(mraa::DIR_OUT);
    	d_pin4->dir(mraa::DIR_OUT);
    	d_pin5->dir(mraa::DIR_OUT);
    	d_pin6->dir(mraa::DIR_OUT);
    	d_pin7->dir(mraa::DIR_OUT);


    	/*d_pin0->write(0);
    	d_pin1->write(0);*/
    	d_pin2->write(0);
    	d_pin3->write(0);
    	d_pin4->write(0);
    	d_pin5->write(0);
    	d_pin6->write(0);
    	d_pin7->write(0);


	    try {
	        rs485 = new mraa::Uart(0);
	        rs232 = new mraa::Uart(1);
	    } catch (std::exception& e) {
	        std::cout << e.what() << ", likely invalid platform config" << std::endl;
	    }

	    try
	    {
	        rs485 = new mraa::Uart("/dev/ttyS3"); // X31
	        rs232 = new mraa::Uart("/dev/ttyS2"); // X30
	    }
	    catch (std::exception& e)
	    {
	        std::cout << "Error while setting up raw UART, do you have a uart?" << std::endl;
	        std::terminate();
	    }

	    if (rs485->setBaudRate(9600) != mraa::SUCCESS) {
	        std::cout << "Error setting parity on RS485" << std::endl;
	    }
	    if (rs232->setBaudRate(115200) != mraa::SUCCESS) {
	    	        std::cout << "Error setting parity on RS232" << std::endl;
	    	    }

	    if (rs485->setMode(8, mraa::UART_PARITY_NONE, 1) != mraa::SUCCESS) {
	        std::cout << "Error setting parity on RS485" << std::endl;
	    }
	    if (rs232->setMode(8, mraa::UART_PARITY_NONE, 1) != mraa::SUCCESS) {
	    	        std::cout << "Error setting parity on RS485" << std::endl;
	    	    }

	    if (rs485->setFlowcontrol(false, false) != mraa::SUCCESS) {
	        std::cout << "Error setting flow control RS485" << std::endl;
	    }
	    if (rs232->setFlowcontrol(false, false) != mraa::SUCCESS) {
	    	        std::cout << "Error setting flow control RS232" << std::endl;
	    	    }


		pthread_create(&threadOne, 0, &thread1, (void*)data);
		pthread_create(&threadTwo, 0, &thread2, (void*)data);
		pthread_create(&threadThree, 0, &thread3, (void*)data);



		pthread_detach(threadOne);
		pthread_detach(threadTwo);
		pthread_detach(threadThree);


		char *host = "192.168.1.101";	// raspberry pi
		int port = 1883;
		int keepalive = 60;
		bool clean_session = true;


		mosquitto_lib_init();
		mosq = mosquitto_new(NULL, clean_session, NULL);
		if(!mosq)
		{
			fprintf(stderr, "Error: Out of memory.\n");
			//return 1;
		}

		//mosquitto_log_callback_set(mosq, my_log_callback);
		mosquitto_connect_callback_set(mosq, my_connect_callback);
		mosquitto_message_callback_set(mosq, my_message_callback);
		mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);


		if(mosquitto_connect(mosq, host, port, keepalive))
		{
			fprintf(stderr, "Unable to connect.\n");
			//return 1;
		}

		mosquitto_loop_start(mosq);


		time_baseSec = 0;

	    while(true)
	    {
	    	rs232Tx[0] = 255;  // ÿ
	    	//fogger_auto = 1;
    		if(fogger_auto == 1)
    		{

    			printf("fog inteval  = %d\n",fogger_Inteval);
    			printf("fog on sec  = %d\n",fogger_onSec);
    			printf("fogger auto\n");
    			printf("%d:%d:%d\r\n",time_baseHour,time_baseMin,time_baseSec);
				//if( (int)(greenhouse_humid*10) < (int)(greenhouse_humidSP*10) && (int)(greenhouse_temp*10) > (int)(greenhouse_tempSP*10))  // TEMP READ > TEMP SETPOINT AND HUMID READ < HUMID SETPOINT
    			if( (int)(greenhouse_humid*10) < (int)(greenhouse_humidSP*10) )  // Humid control
				{
					//printf("high temp humid < 80\n");
					if( (time_baseMin == fogger_Inteval) && (time_baseSec == 0))
					{
						fogger_on();
						 printf("fogger ON\n");

					}
					else if( (time_baseMin == fogger_Inteval) && (time_baseSec == fogger_onSec))
					{
						fogger_off();
						printf("fogger OFF  and Reset timer\n");
						time_baseMin = 0;
						time_baseSec = 0;
					}
				}
    			else{
    				fogger_off();
    			}
    		}




	    	if(rs232->dataAvailable(50))
	    	{
	    		//std::cout << "RS232 Received data" << std::endl;
	    		std::string aRx = rs232->readStr(50);
	    		//std::cout <<  aRx << std::endl;
	    		//printf("%u",aRx);


    			if(aRx[0] == 'g' && aRx[1] == 't'  && aRx.length() == 6)
    			{
    				greenhouse_tempSP = (((int)aRx[2]-48)*10) + ((int)aRx[3]-48) + (((int)aRx[5]-48)*0.1);
    				std::cout << "Set greenhoouse temperature : "<<  greenhouse_tempSP  << std::endl;
    			}
    			if(aRx[0] == 'g' && aRx[1] == 'h'  && aRx.length() == 6)
    			{
    				greenhouse_humidSP = (((int)aRx[2]-48)*10) + ((int)aRx[3]-48) + (((int)aRx[5]-48)*0.1);
    				std::cout << "Set greenhoouse humid : "<<  greenhouse_humidSP  << std::endl;
    			}
    			if(aRx[0] == 'f' && aRx[1] == 'g' && aRx[2] == 'I'  && aRx.length() == 5)
    			{
    				fogger_Inteval = (((int)aRx[3]-48)*10) + ((int)aRx[4]-48);
    				printf("fogger Inteval : %d",fogger_Inteval);
    				time_baseSec=0;
    				time_baseMin=0;

    			}
    			if(aRx[0] == 'f' && aRx[1] == 'g' && aRx[2] == 'O'  && aRx.length() == 5)
    			{
    				fogger_onSec = (((int)aRx[3]-48)*10) + ((int)aRx[4]-48);
    				printf("fogger on %d sec",fogger_onSec);
    				time_baseSec=0;
    				time_baseMin=0;
    			}


	    		// Manual Mode
	    		if(fan_auto == 0)
	    		{
					if(aRx[0] == 'f' && aRx[1] == '1' && aRx[6] == 'o'  && aRx[7] == 'n' && aRx.length() == 8) // FAN 1 ON
					{
						std::cout << "FAN 1 RUN " << (int)aRx[2] << "%" << std::endl;
						fan1_on = (int)aRx[2]*100;
						uint16_t fan1_on_h = (0xFF00&fan1_on)>>8;
						uint16_t fan1_on_l = fan1_on& 0XFF;

						//printf("fan1 check sum   %d \r\n",fan1_on);

						fan1_speed = (int)aRx[2];

						unsigned char input_str_test[6];
						input_str_test[0] = 10;
						input_str_test[1] = 6;
						input_str_test[2] = 32;
						input_str_test[3] = 1;
						input_str_test[4] = fan1_on_h;
						input_str_test[5] = fan1_on_l;

	        			uint16_t chk_sum = crc_modbus(input_str_test,6);
	        			uint16_t aa = chk_sum & 0XFF;
	        			uint16_t bb =  (0xFF00&chk_sum)>>8;

					     rs485Tx[0] = 10;
					     rs485Tx[1] = 6;
					     rs485Tx[2] = 32;
					     rs485Tx[3] = 1;
					     rs485Tx[4] = fan1_on_h;
					     rs485Tx[5] = fan1_on_l;
					     rs485Tx[6] = aa; //215
					     rs485Tx[7] = bb; //57
					     rs485->write(rs485Tx,8);

						//printf("fan1 check sum   %d %d\r\n",aa,bb);

						usleep(500000);

					     rs485Tx[0] = 10;	// FAN 1 ADDRESS
					     rs485Tx[1] = 6;
					     rs485Tx[2] = 32;
					     rs485Tx[3] = 0;
					     rs485Tx[4] = 0;
					     rs485Tx[5] = 10;
					     rs485Tx[6] = 3;
					     rs485Tx[7] = 118;
					     rs485->write(rs485Tx,8);

					     fan1_ON = 1;
					}
					else if(aRx[0] == 'f' && aRx[1] == '2' && aRx[6] == 'o'  && aRx[7] == 'n' && aRx.length() == 8) // FAN 2 ON
					{
						std::cout << "FAN 2 RUN " << (int)aRx[2] << "%" << std::endl;
						fan2_on = (int)aRx[2]*100;
						uint16_t fan2_on_h = (0xFF00&fan2_on)>>8;
						uint16_t fan2_on_l = fan2_on& 0XFF;

						fan2_speed = (int)aRx[2];
						//printf("fan1 check sum   %d \r\n",fan1_on);

						unsigned char input_str_test[6];
						input_str_test[0] = 11;
						input_str_test[1] = 6;
						input_str_test[2] = 32;
						input_str_test[3] = 1;
						input_str_test[4] = fan2_on_h;
						input_str_test[5] = fan2_on_l;

	        			uint16_t chk_sum = crc_modbus(input_str_test,6);
	        			uint16_t aa = chk_sum & 0XFF;
	        			uint16_t bb =  (0xFF00&chk_sum)>>8;

					     rs485Tx[0] = 11;
					     rs485Tx[1] = 6;
					     rs485Tx[2] = 32;
					     rs485Tx[3] = 1;
					     rs485Tx[4] = fan2_on_h;
					     rs485Tx[5] = fan2_on_l;
					     rs485Tx[6] = aa; //215
					     rs485Tx[7] = bb; //57
					     rs485->write(rs485Tx,8);

						//printf("fan1 check sum   %d %d\r\n",aa,bb);

						usleep(500000);

					     rs485Tx[0] = 11;	// FAN 2 ADDRESS
					     rs485Tx[1] = 6;
					     rs485Tx[2] = 32;
					     rs485Tx[3] = 0;
					     rs485Tx[4] = 0;
					     rs485Tx[5] = 10;
					     rs485Tx[6] = 2;
					     rs485Tx[7] = 167;
					     rs485->write(rs485Tx,8);
					     fan2_ON = 1;
					}
					else if(aRx[0] == 'f' && aRx[1] == '1' && aRx[2] == 'o'  && aRx[3] == 'f'   && aRx[4] == 'f') // FAN 1 STOP
					{
						fan1_ON = 0;
					     rs485Tx[0] = 10;
					     rs485Tx[1] = 6;
					     rs485Tx[2] = 32;
					     rs485Tx[3] = 0;
					     rs485Tx[4] = 0;
					     rs485Tx[5] = 9;
					     rs485Tx[6] = 67;
					     rs485Tx[7] = 119;
					     rs485->write(rs485Tx,8);
					}
					else if(aRx[0] == 'f' && aRx[1] == '2' && aRx[2] == 'o'  && aRx[3] == 'f'   && aRx[4] == 'f') // FAN 2 STOP
					{
						fan2_ON = 0;
					     rs485Tx[0] = 11;
					     rs485Tx[1] = 6;
					     rs485Tx[2] = 32;
					     rs485Tx[3] = 0;
					     rs485Tx[4] = 0;
					     rs485Tx[5] = 9;
					     rs485Tx[6] = 66;
					     rs485Tx[7] = 166;
					     rs485->write(rs485Tx,8);
					}
	    		}// fan manual

	    		else if(fan_auto == 1)	// Auto ON
		    	{
	    			//printf("temp SP : %.1f\n",greenhouse_tempSP);
	    			//printf("temp PV : %.1f\n",greenhouse_temp);
	    			//std::cout << "Temp. PV : " << (int)(greenhouse_temp*10) << "Temp. SP : " <<  (int)(greenhouse_tempSP*10) << std::endl;
	    			if( (int)(greenhouse_temp*10) > (int)(greenhouse_tempSP*10) )
	    			{
	    				//printf("RUN BOTH\n");
	    				fan1_speed = 3500;
	    				fan2_speed = 3500;
	    				fan1_ON = 1;
	    				fan2_ON = 1;
	    			}
	    			else if( (int)(greenhouse_temp*10) <= (int)(greenhouse_tempSP*10) )
	    			{

	    				int date_chk = 0; // simulate date odd, oven
	    				if(date_chk)
	    				{
	    					//printf("RUN FAN1\n");
	    					fan1_ON =1;
	    					fan2_ON =0;
	    					fan1_speed = 2500;
	    					fan2_speed = 0;

	    				}
	    				else
	    				{
	    					//printf("RUN FAN2\n");
	    					fan1_ON = 0;
	    					fan2_ON = 1;
	    					fan2_speed = 2500;
	    					fan1_speed = 0;
	    				}
	    			}
		    	}


	    		if(aRx[0] == 'c' && aRx[1] == 'p' && aRx[2] == 'A')
	    		{
	    			std::cout << "Cooling pad AUTO MODE" << std::endl;
	    			coolingPad_auto = 1;
	    		}
	    		else if(aRx[0] == 'c' && aRx[1] == 'p' && aRx[2] == 'M')
	    		{
	    			std::cout << "Cooling pad MANUAL MODE" << std::endl;
	    			coolingPad_auto = 0;
	    		}

	    		if(coolingPad_auto == 0)
	    		{
	    			if(aRx[0] == 'c' && aRx[1] == 'p' && aRx[2] == 'm' && aRx[3] == 'o' && aRx[4] == 'n')
	    			{
	    				coolingpad_on();

	    			}
	    			else if(aRx[0] == 'c' && aRx[1] == 'p' && aRx[2] == 'm' && aRx[3] == 'o' && aRx[4] == 'f' && aRx[5] == 'f')
	    			{
	    				coolingpad_off();

	    			}
	    		}

	    		if(aRx[0] == 'f' && aRx[1] == 'o' && aRx[2] == 'g' && aRx[3] == 'A')
	    		{
	    			std::cout << "Fogger AUTO MODE" << std::endl;
	    			fogger_auto = 1;
	    		}
	    		else if(aRx[0] == 'f' && aRx[1] == 'o' && aRx[2] == 'g' && aRx[3] == 'M')
	    		{
	    			std::cout << "Fogger Manual MODE" << std::endl;
	    			fogger_auto = 0;
	    		}


	    		if(fogger_auto == 0)
	    		{
	    			if(aRx[0] == 'f' && aRx[1] == 'o' && aRx[2] == 'g' && aRx[3] == 'm' && aRx[4] == 'o' && aRx[5] == 'n')
	    				fogger_on();
	    			else if(aRx[0] == 'f' && aRx[1] == 'o' && aRx[2] == 'g' && aRx[3] == 'm' && aRx[4] == 'o' && aRx[5] == 'f' && aRx[6] == 'f')
	    				fogger_off();
	    		}




	    		if(aRx[0] == 'f' && aRx[1] == 'a' && aRx[2] == 't')
	    		{
	    			//std::cout << "FAN AUTO MODE" << std::endl;
	    			fan_auto = 1;
	    		}
	    		else if(aRx[0] == 'f' && aRx[1] == 'm' && aRx[2] == 't')
	    		{
	    			//std::cout << "FAN MANUAL MODE" << std::endl;
	    			fan_auto = 0;
	    		}
	    		else if(aRx[0] == 'd' && aRx[1] == 's' && aRx.length() == 18)  // Get date from HMI
	    		{
	    			system_year = (int)aRx[15]*256 + (int)aRx[14] + 256;
	    			system_month = (int)aRx[9]*256 + (int)aRx[8];
	    			system_day = (int)aRx[3]*256 + (int)aRx[2];

	    			/*std::cout << "Rx : " << aRx<< std::endl;
	    			std::cout << "Length :" << aRx.length() << std::endl;
	    			std::cout << "year : " << system_year << std::endl;
	    			std::cout << "month : " << system_month << std::endl;
	    			std::cout << "day : " << system_day << std::endl;*/
	    		    std::string date_set1 = "date +%Y%m%d -s ";
	    		    std::string yearStr = to_string(system_year);
	    		    std::string monthStr = to_string(system_month);
	    		    if(system_month<10)
	    		    	monthStr.insert(0,1,'0');
	    		    std::string dayStr = to_string(system_day);
	    		    if(system_day<10)
	    		    	dayStr.insert(0,1,'0');

	    		    std::string sumStr = date_set1+yearStr+monthStr+dayStr;

	    		    char tab2[30];
	    		    std::strncpy(tab2, sumStr.c_str(), sizeof(tab2));
	    		    tab2[sizeof(tab2) - 1] = 0;

	    		    std::cout << sumStr << std::endl;
	    			system(tab2);
	    		}
	    		else if(aRx[0] == 'h' && aRx[1] == 's' && aRx.length() == 18) // Get time from HMI
	    		{
	    			system_sec = (int)aRx[15]*256 + (int)aRx[14];
	    			system_min = (int)aRx[9]*256 + (int)aRx[8];
	    			system_hour = (int)aRx[3]*256 + (int)aRx[2];


	    			std::cout << "Time set" << std::endl;
	    			std::string time_set1 = "date +%T -s ";
	    			std::string hourStr = to_string(system_hour);
	    			std::string minStr = to_string(system_min);
	    			std::string secStr = to_string(system_sec);
	    			if(system_hour<10)
	    				hourStr.insert(0,1,'0');
	    			if(system_min<10)
	    				minStr.insert(0,1,'0');
	    			if(system_sec<10)
	    				secStr.insert(0,1,'0');

	    			hourStr.insert(2,1,':');
	    			minStr.insert(2,1,':');


	    			std::string sumTime = time_set1+hourStr+minStr+secStr;

	    			 char tab3[30];
	    			 std::strncpy(tab3, sumTime.c_str(), sizeof(tab3));
	    			 tab3[sizeof(tab3) - 1] = 0;
	    			 std::cout << sumTime << std::endl;
	    			 system(tab3);
	    		}

	    		else if(aRx[0] == 'p' && aRx[1] == 'h' && aRx.length() == 5) // Get pH Nutrient from HMI
	    		{
	    			//ph_nutrientSP = aRx[2]
	    			//printf("ph nutrient SP : %c.%c\r\n",aRx[2],aRx[4]);
	    			ph_nutrientSP = ((double)aRx[2]+ ((double)aRx[4]*0.1));
	    			printf("ph nutrient SP : %c.%c\r\n",aRx[2],aRx[4]);
	    			//mtx3.lock();
	    		     rs485Tx[0] = 'A';	// Light, Humidity and light Address
	    		     rs485Tx[1] = 'S';
	    		     rs485Tx[2] = aRx[2];
	    		     rs485Tx[3] = aRx[4];
	    		     rs485Tx[4] = 'j';
	    		     rs485->write(rs485Tx,5);
	    		     //mtx3.unlock();


	    		}
	    		else if(aRx[0] == 'e' && aRx[1] == 'c' && aRx.length() == 5) // Get EC Nutrient from HMI
	    		{
	    			//ph_nutrientSP = aRx[2]
	    			//printf("EC nutrient SP : %c.%c\r\n",aRx[2],aRx[4]);
	    			 ec_nutrientSP = ((double)aRx[2]+ ((double)aRx[4]*0.1));
	    			 printf("EC nutrient SP : %c.%c\r\n",aRx[2],aRx[4]);
	    			 //mtx3.lock();
	    		     rs485Tx[0] = 'B';	// Light, Humidity and light Address
	    		     rs485Tx[1] = 'S';
	    		     rs485Tx[2] = aRx[2];
	    		     rs485Tx[3] = aRx[4];
	    		     rs485Tx[4] = 'j';
	    		     rs485->write(rs485Tx,5);
	    		     //mtx3.unlock();
	    		}

	    	}

	    	if(rs485->dataAvailable(10))
	    	{
	    		std::string Rx485 = rs485->readStr(100);
	    		//printf("%s",Rx485);

	    		if(Rx485[0] == 1 && Rx485[1] == 3 &&  Rx485[2] == 4)
	    		{
	    			unsigned char input_str_test[8];
	    			input_str_test[0] = Rx485[0];
	    			input_str_test[1] = Rx485[1];
	    			input_str_test[2] = Rx485[2];
	    			input_str_test[3] = Rx485[3];
	    			input_str_test[4] = Rx485[4];
	    			input_str_test[5] = Rx485[5];
	    			input_str_test[6] = Rx485[6];

        			uint16_t chk_sum = crc_modbus(input_str_test,7);
        			uint16_t c = (uint16_t)Rx485[7] + (uint16_t)Rx485[8]*255;
        			uint16_t abs_sum_chk = abs(chk_sum-c);

        			if(abs_sum_chk < ThesholdFrame_Error) // 1000 error
        			{
        				co2_read = (unsigned char)Rx485[3]*255+(unsigned char)Rx485[4];
        				if(co2_read < 0 )
        					co2_read = 0;
        				if(co2_read > 5000)
        					co2_read =5000;
        			}
        			else
        				co2_error++;

	    		}
	    		else if(Rx485[0] == 2 && Rx485[1] == 3 && Rx485[2] == 4)
	    		{
	    			unsigned char input_str_test[8];
	    			input_str_test[0] = Rx485[0];
	    			input_str_test[1] = Rx485[1];
	    			input_str_test[2] = Rx485[2];
	    			input_str_test[3] = Rx485[3];
	    			input_str_test[4] = Rx485[4];
	    			input_str_test[5] = Rx485[5];
	    			input_str_test[6] = Rx485[6];

        			uint16_t chk_sum = crc_modbus(input_str_test,7);
        			uint16_t c = (uint16_t)Rx485[7] + (uint16_t)Rx485[8]*255;
        			uint16_t abs_sum_chk = abs(chk_sum-c);

        			if(abs_sum_chk < ThesholdFrame_Error) // 1000 error
        			{
    	    			humid_read = ((unsigned char)Rx485[3]*255 + (unsigned char)Rx485[4]);
    	    			temperature_read = (unsigned char)Rx485[5]*255+(unsigned char)Rx485[6];
    	    			//if(humid_read < 999)
    	    			//{
    	    				greenhouse_humid = (humid_read/10) + ((humid_read%10)*0.1);
    	    			//	printf("humid ok\r\n");
    	    			//}
    	    			//if(temperature_read < 999)
    	    			//{
    	    				greenhouse_temp = (temperature_read/10) + ((temperature_read%10)*0.1);
    	    			//	printf("temp ok\r\n");
    	    			//}

        			}
        			else
        				temphumid_error++;

        			//printf("chk_crc cal : %d : %d\r\n",chk_sum,c);


	    		}
	    		else if(Rx485[0] == 2 && Rx485[1] == 3 && Rx485[2] == 2)
	    		{
	    			unsigned char input_str_test[8];
	    			input_str_test[0] = Rx485[0];
	    			input_str_test[1] = Rx485[1];
	    			input_str_test[2] = Rx485[2];
	    			input_str_test[3] = Rx485[3];
	    			input_str_test[4] = Rx485[4];

        			uint16_t chk_sum = crc_modbus(input_str_test,5);
        			uint16_t c = (uint16_t)Rx485[5] + (uint16_t)Rx485[6]*255;
        			uint16_t abs_sum_chk = abs(chk_sum-c);

        			if(abs_sum_chk < ThesholdFrame_Error) // 1000 error
        				greenhouse_light = (unsigned char)Rx485[3]*255+(unsigned char)Rx485[4];
        			else
        				light_error++;

        			//printf("chk_crc cal : %d : %d\r\n",chk_sum,c);

	    		}
	    		else if(Rx485[0] == 'p' && Rx485[1] == 'h' && Rx485[2] == 'r' && Rx485[6] == 'j')
	    		{
	    			ph_nutrient = ((double)Rx485[3]-48 + ((int)(Rx485[5]-48)*0.1));
					printf("ph_nutrient : %.1f\r\n",ph_nutrient);
	    		}
	    		else if(Rx485[0] == 'e' && Rx485[1] == 'c' && Rx485[2] == 't' && Rx485[5] == 'j' && Rx485[6] == 'y' && Rx485[7] == 'v')
	    		{
	    			temp_nutrient = ((unsigned char)(Rx485[3]-48)*10 + ((unsigned char)Rx485[4]-48));
	    			if(temp_nutrient<0)
	    				temp_nutrient = 0;
	    		}
	    		else if(Rx485[0] == 'c' && Rx485[1] == 'e' && Rx485[2] == 't'  && Rx485[6] == 'j' && Rx485[7] == 'a' && Rx485[8] == 'b')
	    		{
	    			ec_nutrient = ((double)Rx485[3]-48 + ((int)(Rx485[5]-48)*0.1));
	    			if(ec_nutrient<0)
	    				ec_nutrient = 0;
	    		}

	    		//SDM-120 POWER METER
	    		else if(Rx485[0] == 3 && Rx485[1] == 4)
	    		{
	    			float reg=0;
	    			unsigned char input_str_test[8];
	    			input_str_test[0] = Rx485[0];
	    			input_str_test[1] = Rx485[1];
	    			input_str_test[2] = Rx485[2];
	    			input_str_test[3] = Rx485[3];
	    			input_str_test[4] = Rx485[4];

        			uint16_t chk_sum = crc_modbus(input_str_test,5);
        			uint16_t c = (uint16_t)Rx485[5] + (uint16_t)Rx485[6]*255;
        			uint16_t abs_sum_chk = abs(chk_sum-c);

        			if(abs_sum_chk < ThesholdFrame_Error) // 1000 error
        			{
        				((uint8_t*)&reg)[3] = (unsigned char)Rx485[3];
        				((uint8_t*)&reg)[2] = (unsigned char)Rx485[4];
        				((uint8_t*)&reg)[1] = (unsigned char)Rx485[5];
        				((uint8_t*)&reg)[0] = (unsigned char)Rx485[6];
        				sdm_watt = reg;
        			}
        			else
        				watt_error++;

        			//printf("chk_crc cal : %d : %d\r\n",chk_sum,c);

	    		}


	    	}

	    	//sleep(1);  // for debug @ low speed

	    }
	    return 0;
}

void* thread1(void* data){					// RS485 TX
	int i;
	unsigned int microseconds=1000000;
	while(1)
	{

		//mtx2.lock();
		ReadLight_485();
		usleep(100000);
		ReadTempNHumid_485();
		usleep(100000);
		ReadCO2_485();
		usleep(100000);
		/*ReadEC_485();
		usleep(100000);
		ReadEC_485();
		usleep(50000);
		ReadEC_485();
		usleep(50000);
		ReadTempEC_485();
		usleep(50000);
		ReadTempEC_485();
		usleep(50000);*/
		usleep(500000);
		//sleep(1);
		//mtx2.unlock();

		if(fan_auto == 1)
		{
			printf("f1 speed : %d\n",fan1_speed);
			printf("f2 speed : %d\n",fan2_speed);
			if(fan1_ON == 1)
			{
				fan1_run(fan1_speed);
				usleep(500000);

			}
			else{
				fan1_stop();
				usleep(500000);
			}
			if(fan2_ON == 1)
			{
				fan2_run(fan2_speed);
				usleep(500000);

			}
			else{
				fan2_stop();
				usleep(500000);
			}
		}
	}//while

}

void* thread2(void* data){								// HMI Update
	//time_t start = time(0);
	// Print every 2 seconds for at most, 30 seconds
	//while( (int)difftime(time(0), start) < 30 ){
	while(1)
	{
		mtx.lock();
		//printf("THREAD 2 Update HMI screen!\n");
    	SetCO2_HMI();
    	SetDate_HMI();
    	SetStatusFAN();
    	SetHumid_HMI();
    	SetTemperature_HMI();
    	SetLight_HMI();
    	SetHumidAmbient_HMI();
    	SetTempAmbient_HMI();
    	SetEC_HMI();
    	SetECTemp_HMI();
    	SetpH_HMI();
    	SetFoggerStatus();
    	SetCoolingPadStatus();
		//sleep(1);
    	usleep(500000);
		mtx.unlock();

		char buffer1[6];
		char buffer2[6];
		char buffer3[6];
		char buffer4[6];
		char buffer5[6];
		char buffer6[6];
		char buffer7[6];
		char buffer8[6];  // FAN 1
		char buffer9[6];  // FAN 2
		char buffer10[6];  // Fogger
		char buffer11[6];	// cooling pad
		char buffer12[6];	// power watt


		snprintf(buffer1, sizeof(buffer1), "%.1f", greenhouse_temp);
		snprintf(buffer2, sizeof(buffer2), "%.1f", greenhouse_humid);
		snprintf(buffer3, sizeof(buffer3), "%5d", greenhouse_light);
		snprintf(buffer4, sizeof(buffer4), "%5d", co2_read);
		snprintf(buffer5, sizeof(buffer5), "%.1f", ec_nutrient);
		snprintf(buffer6, sizeof(buffer6), "%.1f", ph_nutrient);
		snprintf(buffer7, sizeof(buffer7), "%2d", temp_nutrient);
		snprintf(buffer8, sizeof(buffer8), "%2d", fan1_on);
		snprintf(buffer9, sizeof(buffer9), "%2d", fan2_on);
		snprintf(buffer10, sizeof(buffer10), "%2d", fogger_ON);
		snprintf(buffer11, sizeof(buffer11), "%2d", coolingPad_on);
		snprintf(buffer12, sizeof(buffer12), "%2d", (int)sdm_watt);



		//printf("temp raw  from rs485 : %d\r\n",temperature_read);

		if(temperature_read>600)
			tempErrorCount++;
		//printf("temp > 60c  from rs485 : %d\r\n",tempErrorCount);

		if(temperature_read==255)
		{
			printf("Warning temp ");
			tempErrorCount++;
		}


		mosquitto_publish(mosq, NULL, "/greenhouse1/temperature", 4, buffer1 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/humidity", 4, buffer2 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/light", 5, buffer3 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/co2", 5, buffer4 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/ec", 4, buffer5 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/ph", 4, buffer6 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/watertemp", 2, buffer7 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/fan1", 2, buffer8 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/fan2", 2, buffer9 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/fogger", 2, buffer10 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/coolingpad", 2, buffer11 , 0, false);
		mosquitto_publish(mosq, NULL, "/greenhouse1/powerwatt", 2, buffer12 , 0, false);


		/*printf("temp = %.2f : %s\r\n",greenhouse_temp,buffer1);
		printf("humid = %.2f : %s\r\n",greenhouse_humid,buffer2);
		printf("light = %d : %s\r\n",greenhouse_light,buffer3);
		printf("co2 = %d : %s\r\n\n\n",co2_read,buffer4);*/

		/*printf("\ntemp = %.2f\r\n",greenhouse_temp);
		printf("humid = %.2f\r\n",greenhouse_humid);
		printf("light = %d\r\n",greenhouse_light);
		printf("co2 = %d\r\n",co2_read);
		printf("ec_nutrient : %.1f\r\n",ec_nutrient);
		printf("ph_nutrient : %.1f\r\n",ph_nutrient);
		printf("water temp : %d\r\n\n",temp_nutrient);
		printf("FAN1 : %d\r\n\n",fan1_on);
		printf("FAN2 : %d\r\n\n",fan2_on);

		printf("CO2 frame error %d\r\n",co2_error);
		printf("Light frame error %d\r\n",light_error);
		printf("Temp Humid frame error %d\r\n",temphumid_error);*/

		//sleep(3);
	}

}

void* thread3(void* data) // run time , fogger
{
	while(1)
	{
		/*time_baseSec++;
		if(time_baseSec==60)
		{
			time_baseSec=0;
			time_baseMin++;
			if(time_baseMin==60)
		    {
				time_baseSec=0;
		        time_baseMin=0;
		        time_baseHour++;
		    }
		}*/

		time_baseSec++;
				if(time_baseSec==60)
				{
					time_baseSec=0;
					time_baseMin++;
					if(time_baseMin==fogger_Inteval+1)
				    {
						time_baseSec=0;
				        time_baseMin=0;
				    }
		}

		printf("%d:%d:%d\r\n",time_baseHour,time_baseMin,time_baseSec);
		sleep(1);
	}
}


