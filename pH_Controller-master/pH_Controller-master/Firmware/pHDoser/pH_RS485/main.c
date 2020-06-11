/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

/* Device header file */
#if defined(__XC16__)
    #include <xc.h>
#elif defined(__C30__)
    #if defined(__dsPIC30F__)
        #include <p30Fxxxx.h>
    #endif
#endif

// DS1820
#define DQ       PORTFbits.RF2
#define DQ_DIR   TRISFbits.TRISF2

#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "system.h"        /* System funct/params, like osc/peripheral config */

#include "uart.h"
#include <libpic30.h>
#include <float.h>
#include "lcd16x2.h"
#include "timer.h"
#include "adc10.h"
#include "qei.h"
#include "eeprom.h"
#include "i2c.h"
#include "ds1820.h"

//_FOSC(CSW_FSCM_OFF & FRC_PLL16)
_FOSC(CSW_FSCM_OFF & XT_PLL16)
_FWDT(WDT_OFF)        
_FBORPOR(PWRT_OFF & PBOR_OFF & MCLR_EN)        
        
        
char ph_address = 'A';        
int temp485;

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/
unsigned int packet[25];

char buf[16];
char bufUART1[30];
// ADC
unsigned int  adc_buff;										// ADC Result
unsigned int  adc_count;

int pH4_cal;
int pH7_cal;

int sensor_sim=0;
int sensor_menu=0;

float pH_old_control;
float EC_old_control;

float temperature_read;
    
float ph_USE;
float pH_old;
float pH_read;
float pH_diff;
float pH_old;
float pH_ADC;
float pH_SP=6.0;
float pH_LowSP=5.5;
float pH_HighSP=6.1;
int pHCalStatus;
float pH_Compensate;
int pH_flag=0;
float pH_FeedCount=0;
float pHCoeff=0;
int control_mode;
float tempCoeff_buff;
int pHFeedStatus=0;

volatile float pH_diff_cal;

int EC_flag=0;	 
float EC_diff;
int EC_sign;      
float EC_read;
float EC_adc;
float EC_raw;
float EC_SP=1.5;
float EC_LowSP=1.4;
float EC_HighSP=2.0;
int EC_temp=0;
int EC_CalStatus;
float EC_Cal1413;
int EC_tempOld;
float EC_readOld;

double phDouble;
//Alarm
int alarm_mode;

// Timer
int ph_minute;
int ph_second;
int ph_mille_sec;
int data_blink;

int pH_TimeInterval=2;
int EC_TimeInterval=5;
int ph_dose_time;
int ec_dose_time;
int menu_reset_min;
int menu_reset_sec;
int cal_status;
int debug_mode;

int menu_count=0;

float testFloat;


#define TRIS_RS485   TRISEbits.TRISE1
//#define DIR_RS485 LATEbits.LATE1
#define DIR_RS485 PORTEbits.RE1
/* RELAY */
#define RY1_DIR   TRISDbits.TRISD0
#define RY1       LATDbits.LATD0

/* ENCODER */
#define ENC_TRIS    TRISBbits.TRISB3
#define ENC_A       TRISBbits.TRISB4
#define ENC_B       TRISBbits.TRISB5
#define ENC_SW      PORTBbits.RB3

void init_uart();

float packet_use[50];
float speed=0;


char rx_data;	
int ll;

void convert_pHSetting()
{
	int c;
	int temp;		

	for(c=0;c<8;c++)    
	{
		   switch(packet[c]){
		   		case '0': packet_use[c] = 0.0; break;
		   		case '1': packet_use[c] = 1.0; break;
		   		case '2': packet_use[c] = 2.0; break;
		   		case '3': packet_use[c] = 3.0; break;
		   		case '4': packet_use[c] = 4.0; break;
		   		case '5': packet_use[c] = 5.0; break;
		   		case '6': packet_use[c] = 6.0; break;
		   		case '7': packet_use[c] = 7.0; break;
		   		case '8': packet_use[c] = 8.0; break;
		   		case '9': packet_use[c] = 9.0; break;
		   		default : packet_use[c] = 0.0;
		   }
	}

	speed = (packet_use[2]) + (packet_use[3]*0.1) ;
}


void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)   // Timer 0.1 s Base
{
	if(pH_TimeInterval != 0)
        ph_mille_sec++;

    if(ph_mille_sec == 5)
	{
		data_blink++;      
    }
	else if(ph_mille_sec == 10)
	{
		ph_mille_sec = 0;
		ph_second++;      
    }  
	_T1IF = 0;  // Clear Timer1 Interrupt flag
}


// UART1 Receiver Interrupt Service Routine
void __attribute__((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{   
    rx_data = ReadUART1();
    if(rx_data != '\n'  && rx_data != ' ')
    {
			packet[ll] = rx_data;	
            ll++;
    }
		
	if(rx_data == 'j')
	{
    	ll=0;	
		if(packet[0] == ph_address && packet[1] == 'S')
		{
			pH_SP = (packet[2]-48) + (packet[3]-48)*0.1; 
            Eeprom_WriteWord(0,(int)(pH_SP*10));
            __delay_ms(10);   
		}
        else if(packet[0] == ph_address && packet[1] == 'L')
		{
            pH_LowSP = (packet[2]-48) + (packet[3]-48)*0.1; 
            Eeprom_WriteWord(1,(int)(pH_LowSP*100));
            __delay_ms(10);    
		}
        else if(packet[0] == ph_address && packet[1] == 'H')
		{
            pH_HighSP = (packet[2]-48) + (packet[3]-48)*0.1; 
            Eeprom_WriteWord(2,(int)(pH_HighSP*100));
            __delay_ms(10);  
		}
        else if(packet[0] == ph_address && packet[1] == 'D')
		{
            ph_dose_time = (packet[2]-48)*10 + (packet[3]-48); 
            Eeprom_WriteWord(5,(int)(ph_dose_time));            
            __delay_ms(10);     
		}
        else if(packet[0] == ph_address && packet[1] == 'I')
		{
            pH_TimeInterval = (packet[2]-48)*10 + (packet[3]-48); 
              Eeprom_WriteWord(6,(int)(pH_TimeInterval));            
            __delay_ms(10);      
		}
        else if(packet[0] == ph_address && packet[1] == 'R')
		{
            DIR_RS485 = 1; // Send
            while(DataRdyUART1());
            sprintf(bufUART1,"phr%.1fj",phDouble);
            putsUART1(bufUART1);
            __delay_ms(300);
            DIR_RS485 = 0;
		}
        else if(packet[0] == ph_address && packet[1] == 'F')
		{
            DIR_RS485 = 1; // Send
            while(DataRdyUART1());
            sprintf(bufUART1,"%phf%04dj",(int)pH_FeedCount);
            putsUART1(bufUART1);
            __delay_ms(300);
            DIR_RS485 = 0;
		}
        else if(packet[0] == ph_address && packet[1] == 'T')
		{
            DIR_RS485 = 1; // Send
            while(DataRdyUART1());
            sprintf(bufUART1,"%pht%02dj",(int)temperature_read);
            putsUART1(bufUART1);
            __delay_ms(300);
            DIR_RS485 = 0;
		}
    }
  	
	_U1RXIF = 0;    // Clear interrupt flag RX
}



void Timer1_Init(void)
{
    // Timer
    //Fcy = 80000000 / 4 = 20000000
    // 1 instrunction = 1 / 20000000 = 0.05 uS
    // Overflow every 0.05 uS
    // if 0.1 second = 2000000 instruction 
    // Prescale 1:256 = 2000000 / 256 = 7812
    
    T1CONbits.TON = 0; // Disable Timer
    T1CONbits.TCS = 0; // Select internal instruction cycle clock
    T1CONbits.TGATE = 0; // Disable Gated Timer mode
    T1CONbits.TCKPS = 0b10; // Select 1:1 Prescaler
    T1CONbits.TCKPS = 3; // Select 1:256 Prescaler
    TMR1 = 0x00; // Clear timer register
    PR1 = 11489; // Load the period value
    //PR1 = 7812; // Load the period value 100 mS
    IPC0bits.T1IP = 0x01; // Set Timer1 Interrupt Priority Level
    IFS0bits.T1IF = 0; // Clear Timer1 Interrupt Flag
    IEC0bits.T1IE = 1; // Enable Timer1 interrupt
    T1CONbits.TON = 1; // Start Timer
}

void timer1_reset(void){
  ph_second = 0;
  ph_minute = 0;   
  TMR1 = 0x00;
}

void adc_init()
{
unsigned int Channel, PinConfig, Scanselect, Adcon3_reg, Adcon2_reg,Adcon1_reg;
                ADCON1bits.ADON = 0;                    			// Turn off ADC     
                Channel = ADC_CH0_POS_SAMPLEA_AN0 &					// Channel 0 positive input select AN0         
                          ADC_CH0_NEG_SAMPLEA_NVREF ;             	// Channel 0 negative VREF

                SetChanADC10(Channel);                               // Set channel configuration             
                ConfigIntADC10(ADC_INT_DISABLE);                     // Disable interrupt for ADC
                PinConfig = ENABLE_AN0_ANA ;                         // Enable AN0 analog port                                          
                Scanselect = SKIP_SCAN_AN6 &                         // Scan for AN0-AN3
                             SKIP_SCAN_AN7 &						 // Skip Scan for AN4-A7
                             SKIP_SCAN_AN8 &
                             SKIP_SCAN_AN9;                                                              
                Adcon3_reg = ADC_SAMPLE_TIME_1 &                 	// Sample for 10 time
                             ADC_CONV_CLK_INTERNAL_RC &             // Internal Clock
                             ADC_CONV_CLK_Tcy; 
                Adcon2_reg = ADC_VREF_AVDD_AVSS &                   // Vref at Vdd and Vss
                             ADC_SCAN_ON &                          // Enable scan for ADC
                             ADC_ALT_BUF_OFF &                      // Disable alternate buffer
                             ADC_ALT_INPUT_OFF &                    // Disable alternate input
                             ADC_CONVERT_CH0&                       // Select CH0 convert         
                             ADC_SAMPLES_PER_INT_16;                // 16 sample between interrupt
                Adcon1_reg = ADC_MODULE_ON &                        // Enable module ADC
                             ADC_IDLE_CONTINUE &                    // ADC run on idle mode
                             ADC_FORMAT_INTG &                      // Output value integer format
                             ADC_CLK_MANUAL &                       // ADC manual clock
                             ADC_SAMPLE_SIMULTANEOUS &              // ADC sampling simultaneous
                             ADC_AUTO_SAMPLING_ON;                  // ADC auto sampling    
                    
OpenADC10(Adcon1_reg, Adcon2_reg,Adcon3_reg,PinConfig, Scanselect);  // Turn on ADC module     
}

void pH_Read()
{
	int k;
	float temp2=0;	

	for(k=1;k<=10000;k++)  // Low pass
	{
		ADCON1bits.SAMP = 1;                            
        ConvertADC10();                 				
        temp2 = temp2 + ReadADC10(0);                    
	}
  		pH_ADC =  temp2/10000;
        ph_USE = (pH_ADC* pHCoeff)+pH_Compensate;
        phDouble = (double)ph_USE;
		//pH_read = ((temp2/5000) * pHCoeff) + 0.6;	

		
		//if((pH_read < 0) || (pH_read > 255) )
		//	pH_read = 0;
}

void Init_QEI(void)
{ 
 //QEI configuration register 
  POSCNT = 0;               // Set Current Position 
  MAXCNT = 904;              // Use QEI in 2x mode
 
  QEICONbits.QEIM = 7;           // x4 reset by MAXCNT pulse
  QEICONbits.POSRES = 0;         // doesn't Allow Index pulse to reset counter
  QEICONbits.SWPAB = 0;          // direction    
}

void feed_acid()
{
if(ph_USE > 0.0f && ph_USE < 255.0f)
	RY1 = 0;
}
void stop_acid()
{
if(ph_USE > 0.0f && ph_USE < 255.0f)
	RY1 = 1;
}


void pH_Control()
{
			if( (ph_USE*10) == (pH_SP*10))
			{
				pH_flag = 1;
				RY1 = 0; // Relay OFF
			}
			else if( (ph_USE*10) > (pH_SP*10) && (pH_flag == 0) )
			{
				RY1 = 1; // Relay ON
				pH_flag = 0;
			}
			else if( (ph_USE*10) > (pH_HighSP*10) && (pH_flag == 1) )
			{
				RY1 = 1; // Relay ON
				pH_flag = 0;
			}
			else if((ph_USE*10) <= (pH_LowSP*10)) // Lower limit alarm system
			{
				RY1 = 0; // Relay OFF
				pH_flag = 1;
			}
}

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

int main(void)
{

    ADPCFG = 0xFFFF;  // Set PORTB Digital input
    RY1_DIR = 0;
    ENC_TRIS = 1;
    ENC_A = 1;
    ENC_B = 1;
    
    LCDInit();
    LCDClrscr();      // Clear screen
    
    adc_init();
    Init_QEI();
    init_uart();
    
    TRIS_RS485 = 0; // Output
    DIR_RS485 = 0; // Read
    

    ph_minute = 0;
    ph_second = 0; 
    ph_mille_sec = 0;

    int menu = 1;
    
    ph_dose_time = 3;
    int aa;
    
    // Load data from EEPROM
    pH_SP = Eeprom_ReadWord(0);	
    pH_SP = (float)pH_SP/10;
    __delay_ms(10);
    pH_LowSP = Eeprom_ReadWord(1);
    pH_LowSP = (float)pH_LowSP/100;
    __delay_ms(10);
    pH_HighSP = Eeprom_ReadWord(2);
    pH_HighSP = (float)pH_HighSP/100;
    __delay_ms(10);
    pH4_cal = Eeprom_ReadWord(3);
    __delay_ms(10);
    pH7_cal = Eeprom_ReadWord(4);
    __delay_ms(10);
    ph_dose_time = Eeprom_ReadWord(5);	
    __delay_ms(10);
    pH_TimeInterval = Eeprom_ReadWord(6);
    __delay_ms(10);

    // For debug
    //pH4_cal = 152;
    //pH7_cal = 435;
    /*pH_SP = 6.0;
    pH_LowSP = 5.8;
    pH_HighSP = 6.2;
    pH_TimeInterval = 1;
    ph_dose_time = 3;*/
    
    if(pH4_cal == -1)
        pH4_cal = 583;
    if(pH7_cal == -1)
        pH7_cal = 498;

            
    pHCoeff = (3 /((float)(pH7_cal-pH4_cal)));
    pH_Compensate = 4-(pH4_cal*pHCoeff);
    //pHCoeff = (3 / (float) (pH4_cal-pH7_cal));
    //pH_Compensate = 4-(pH4_cal*pHCoeff);
    //pH_Compensate = 4-(pH4_cal*pHCoeff); // BearingHydro signal conditioning

    if(ph_dose_time <= 0)
        ph_dose_time = 5;
    if(ec_dose_time <= 0)
        ec_dose_time = 5;

    // Initial
     sprintf(buf,"BearingHydroponics"); 
     LCDPrintxy(1,1,0, buf);
     sprintf(buf," pH485  V.1.0   "); 
     LCDPrintxy(1,2,0, buf);      // Display second
     __delay_ms(2000);

     sprintf(buf,"Firmware ver.1.0 "); 
     LCDPrintxy(1,1,0, buf);
     sprintf(buf,"Update  09/09/17  "); 
     LCDPrintxy(1,2,0, buf);      // Display second
     __delay_ms(1000);

    // Show data for calibration
     sprintf(buf,"pH4=%d pH7=%d",pH4_cal,pH7_cal); 
     LCDPrintxy(1,1,0, buf);
     sprintf(buf,"ADDRESS=%c",ph_address); 
     LCDPrintxy(1,2,0, buf);      // Display second
     __delay_ms(1000);
 
     // Show data for calibration
     sprintf(buf,"pHCoeff = %f",pHCoeff); 
     LCDPrintxy(1,1,0, buf);
     sprintf(buf,"pHCompe = %f",pH_Compensate); 
     LCDPrintxy(1,2,0, buf);      // Display second
     __delay_ms(2000);
    
     Timer1_Init();
    
    ds1820_init();

unsigned int *temp_read;

int aa_test=0;

    while(1)
    {

        DIR_RS485 = 0; // Read  add to test fix bug LCD and software error
            
        if(pH_FeedCount>9999)
            pH_FeedCount = 0;
        
         pH_Read();
         temp_read = readt();
    
        temperature_read = ((temp_read[1]*256)+temp_read[0])/16;
        
        if(temperature_read > 99)
            temperature_read = 0;
        
        /*if(pH_read > 14)
            pH_read = 0;*/
              
         // Timer calculation
		 if(ph_second > 59)
		 {
		 	ph_second = 0;
		 	ph_minute++;
		 }
		 if(ph_minute > 59)
		 {
		 	ph_second = 0;
		 	ph_minute = 0;
		 }	
        
        
        if((ph_minute == pH_TimeInterval) && (ph_second == 0)) 
		{
		    pH_Control();
		}
		if(ph_minute == pH_TimeInterval && ph_second == ph_dose_time) 
	 	{	 
			RY1 = 0;
			ph_second = 0;
		 	ph_minute = 0;
		 	//if(pH_flag == 0)
		 		pH_FeedCount = pH_FeedCount+((float)ph_dose_time * 8.33); // pump feed @ 500ml/min = 8.33 ml/sec
	 	}
        //menu_count = POSCNT;
        
        if(POSCNT >=0 && POSCNT <= 3) 
        {
    
            if((ph_second % 2) == 0)
              sprintf(buf,"pH    : %2.1f    .",phDouble); 
            else
              sprintf(buf,"pH    : %2.1f     ",phDouble); 
                  
		      LCDPrintxy(1,1,0, buf);
			  sprintf(buf,"Temp. : %2d %cC   ",(int)temperature_read,223);              
			  LCDPrintxy(1,2,0, buf);      // Display second

        }// show read data menu
        
        if(POSCNT >=4 && POSCNT <= 7) 
        {
             sprintf(buf,"   pH Setting   "); 
             LCDPrintxy(1,1,0, buf);      // Display second
             sprintf(buf,"Setpoint : %.1f  ",pH_SP); 
             LCDPrintxy(1,2,0, buf);      // Display second
            if(!ENC_SW)
            {
                __delay_ms(250);
                POSCNT = pH_SP*40;
              while(ENC_SW)
              {
                  pH_SP=(float)POSCNT/40;
                  if(POSCNT<220)
                      POSCNT = 220;
                  if(POSCNT>400)
                      POSCNT = 400;
                  
                  if(pH_SP<= 5.5)
                      pH_SP = 5.5;
                  else if(pH_SP>= 10)
                      pH_SP = 10;


                  if(ph_mille_sec%2) // Blink data
                  {
                    sprintf(buf,"   pH Setting   "); 
                    LCDPrintxy(1,1,0, buf);      // Display second
                    sprintf(buf,"Setpoint : %.1f  ",pH_SP); 
                    LCDPrintxy(1,2,0, buf);      // Display second
                  }
                  else
                  {
                    sprintf(buf,"   pH Setting   "); 
                    LCDPrintxy(1,1,0, buf);      // Display second
                    sprintf(buf,"Setpoint :      "); 
                    LCDPrintxy(1,2,0, buf);      // Display second
                  }
              }
              Eeprom_WriteWord(0,(int)(pH_SP*10));
            __delay_ms(10);  
            Eeprom_WriteWord(0,(int)(pH_SP*10));
            __delay_ms(10);  
            }
        }// pH setting menu
        
        if(POSCNT >=8 && POSCNT <= 11)  // pH set Lower Limit
        {
          sprintf(buf,"  pH Low Limit  "); 
          LCDPrintxy(1,1,0, buf);      // Display second
          sprintf(buf,"Setpoint : %.1f  ",pH_LowSP); 
          LCDPrintxy(1,2,0, buf);      // Display second
          
 
            if(!ENC_SW)
            {
                __delay_ms(250);
                POSCNT = pH_LowSP*40;
              while(ENC_SW)
              {
                  pH_LowSP=(float)POSCNT/40;
                  if(POSCNT<220)
                      POSCNT = 220;
                  if(POSCNT>pH_SP*40)
                      POSCNT = pH_SP*40;
                  
                  if(pH_LowSP<= 5.5)
                      pH_LowSP = 5.5;
                  else if(pH_LowSP>= pH_SP)
                      pH_LowSP = pH_SP-0.1;



                  if(ph_mille_sec%2) // Blink data
                  {
                    sprintf(buf,"  pH Low Limit  "); 
                    LCDPrintxy(1,1,0, buf);      // Display second
                    sprintf(buf,"Setpoint : %.1f  ",pH_LowSP); 
                    LCDPrintxy(1,2,0, buf);      // Display second
                  }
                  else
                  {
                    sprintf(buf,"  pH Low Limit  "); 
                    LCDPrintxy(1,1,0, buf);      // Display second
                    sprintf(buf,"Setpoint :      ",pH_LowSP); 
                    LCDPrintxy(1,2,0, buf);      // Display second
                  }
              }
              Eeprom_WriteWord(1,(int)(pH_LowSP*100));
            __delay_ms(10);  
             Eeprom_WriteWord(1,(int)(pH_LowSP*100));
            __delay_ms(10);  
            }
        }
        if(POSCNT >=12 && POSCNT <= 15)  // pH set Higher Limit
        {
          sprintf(buf," pH High Limit  "); 
          LCDPrintxy(1,1,0, buf);      // Display second
          sprintf(buf,"Setpoint : %.1f  ",pH_HighSP); 
          LCDPrintxy(1,2,0, buf);      // Display second

            if(!ENC_SW)
            {
                __delay_ms(250);
                POSCNT = pH_HighSP*40;
              while(ENC_SW)
              {
                  pH_HighSP=(float)POSCNT/40;
                  if(POSCNT<pH_SP*40)
                      POSCNT = pH_SP*40;
                  if(POSCNT>400)
                      POSCNT = 400;
                  
                  if(pH_HighSP<= pH_SP)
                      pH_HighSP =pH_SP+0.1;
                  else if(pH_HighSP >= 10)
                      pH_HighSP = 10;


                  if(ph_mille_sec%2) // Blink data
                  {
                    sprintf(buf,"  pH High Limit ");
                    LCDPrintxy(1,1,0, buf);      // Display second
                    sprintf(buf,"Setpoint : %.1f  ",pH_HighSP); 
                    LCDPrintxy(1,2,0, buf);      // Display second
                  }
                  else
                  {
                    sprintf(buf,"  pH High Limit "); 
                    LCDPrintxy(1,1,0, buf);      // Display second
                    sprintf(buf,"Setpoint :      ",pH_HighSP); 
                    LCDPrintxy(1,2,0, buf);      // Display second
                  }
              }
              Eeprom_WriteWord(2,(int)(pH_HighSP*100));
            __delay_ms(10);
            Eeprom_WriteWord(2,(int)(pH_HighSP*100));
            __delay_ms(10);  
            }
        }
        if(POSCNT >=16 && POSCNT <= 19)  // pH Dose Time
        {
           sprintf(buf,"  pH Dose Time  "); 
           LCDPrintxy(1,1,0, buf);
           sprintf(buf," Time : %2d Sec  ",ph_dose_time); 
           LCDPrintxy(1,2,0, buf);  

            if(!ENC_SW)
            {
                __delay_ms(250);
                POSCNT = ph_dose_time*10;
              while(ENC_SW)
              {
                  ph_dose_time=(float)POSCNT/10;
                  if(POSCNT<=0)
                      POSCNT = 0;
                  if(POSCNT>=900)
                      POSCNT = 900;
                  
                  if(ph_dose_time <= 0)
                      ph_dose_time = 0 ;
                  if(ph_dose_time >= 90)
                      ph_dose_time = 90 ;


                  if(ph_mille_sec%2) 
                  {
                    sprintf(buf,"  pH Dose Time  ");
                    LCDPrintxy(1,1,0, buf);   
                    sprintf(buf," Time : %2d Sec  ",ph_dose_time); 
                    LCDPrintxy(1,2,0, buf);   
                  }
                  else
                  {
                    sprintf(buf,"  pH Dose Time  "); 
                    LCDPrintxy(1,1,0, buf);   
                    sprintf(buf," Time :    Sec ",ph_dose_time); 
                    LCDPrintxy(1,2,0, buf);   
                  }
              }
              Eeprom_WriteWord(5,(int)(ph_dose_time));            
            __delay_ms(10);  
            }
        }
        if(POSCNT >=20 && POSCNT <= 23)  // pH Interval Time
        {
           sprintf(buf,"   pH Interval  "); 
           LCDPrintxy(1,1,0, buf);
           sprintf(buf," Time : %2d Min. ",pH_TimeInterval); 
           LCDPrintxy(1,2,0, buf);  

            if(!ENC_SW)
            {
                __delay_ms(250);
                POSCNT = pH_TimeInterval*10;
              while(ENC_SW)
              {
                  pH_TimeInterval=(float)POSCNT/10;
                   if(POSCNT<=0)
                      POSCNT = 0;
                  if(POSCNT>=900)
                      POSCNT = 900;
                  
                  if(pH_TimeInterval <= 0)
                      pH_TimeInterval = 0 ;
                  if(pH_TimeInterval >= 90)
                      pH_TimeInterval = 90 ;


                  if(ph_mille_sec%2) 
                  {
                    sprintf(buf,"   pH Interval  "); 
                    LCDPrintxy(1,1,0, buf);
                    sprintf(buf," Time : %2d Min. ",pH_TimeInterval); 
                    LCDPrintxy(1,2,0, buf);  
                  }
                  else
                  {
                    sprintf(buf,"   pH Interval  "); 
                    LCDPrintxy(1,1,0, buf);
                    sprintf(buf," Time :    Min. "); 
                    LCDPrintxy(1,2,0, buf);    
                  }
              }
              Eeprom_WriteWord(6,(int)(pH_TimeInterval));            
            __delay_ms(10);  
            }
        }
         if(POSCNT >=24 && POSCNT <= 27)  // pH4 Cal
         {
            timer1_reset();			
        
               //pHCalStatus
		      sprintf(buf,"   pH 4  Cal.   "); 
		      LCDPrintxy(1,1,0, buf);
			  sprintf(buf,"  Press  Enter  "); 
			  LCDPrintxy(1,2,0, buf);      // Display second	 	
			  int cal4_cnt;
               if(!ENC_SW)
              {       
                    for(cal4_cnt=60;cal4_cnt>0;cal4_cnt--)
                    {
                        __delay_ms(1000); 
                        sprintf(buf,"   pH 4  Cal.   "); 
                        LCDPrintxy(1,1,0, buf);
                        sprintf(buf," Time Left %d ",cal4_cnt); 
                        LCDPrintxy(1,2,0, buf);      // Display second
                    }
                   Eeprom_WriteWord(3,(int)(pH_ADC));                    
                   __delay_ms(10);
                    Eeprom_WriteWord(3,(int)(pH_ADC));  
                   __delay_ms(10);
                    pH4_cal = pH_ADC;
              }

       }//pH 4 Calibration
	
        if(POSCNT >=28 && POSCNT <= 31)  // pH7 Cal
        {
            timer1_reset();			
        
               //pHCalStatus
		      sprintf(buf,"   pH 7  Cal.   "); 
		      LCDPrintxy(1,1,0, buf);
			  sprintf(buf,"  Press  Enter  "); 
			  LCDPrintxy(1,2,0, buf);      // Display second	 	
			  int cal7_cnt;
               if(!ENC_SW)
              {       
                    for(cal7_cnt=60;cal7_cnt>0;cal7_cnt--)
                    {
                        __delay_ms(1000); 
                        sprintf(buf,"   pH 7  Cal.   "); 
                        LCDPrintxy(1,1,0, buf);
                        sprintf(buf," Time Left %d ",cal7_cnt); 
                        LCDPrintxy(1,2,0, buf);      // Display second
                    }
                 Eeprom_WriteWord(4,(int)(pH_ADC));              
                   __delay_ms(10);
                   Eeprom_WriteWord(4,(int)(pH_ADC));             
                   __delay_ms(10);  
                  pH7_cal = pH_ADC;
              }
        }//pH 7 Calibration


	if(POSCNT >=32 && POSCNT <= 35)	// debug monitoring
	{ 
        debug_mode = 1;
      sprintf(buf,"   Debugging    "); 
      LCDPrintxy(1,1,0, buf);
	  sprintf(buf,"ADC=%1.1f %02d:%02d   ",pH_ADC,ph_minute,ph_second); 
	  LCDPrintxy(1,2,0, buf);
	}
	if(POSCNT >=36 & POSCNT <= 39)	// Feed count
	{ 	
      sprintf(buf," pH Feed count  "); 
      LCDPrintxy(1,1,0, buf);
	  sprintf(buf,"pH Down: %04d ml",(int)pH_FeedCount);
	  LCDPrintxy(1,2,0, buf);      // Display second	  	
	  
	  /*if((SW2) && (SW4))  // Reset pH count
	  {
		pH_FeedCount = 0;
		__delay_ms(100);
	  }	  
	  __delay_ms(100);*/
	}
	if(POSCNT >=40 && POSCNT <= 43)	// Pump Overide
	{
      sprintf(buf," Pump  Overide  "); 
      LCDPrintxy(1,1,0, buf);
	  sprintf(buf,"  PUSH TO ON    "); 
	  LCDPrintxy(1,2,0, buf);    
	 		
	  if(ENC_SW)
    	RY1 = 0; // Relay 1 ON
	  else if(!ENC_SW)
		RY1 = 1; // Relay 1 OFF
    }
        
        if(POSCNT > 45)
        {
            POSCNT = 0;
        }
   
    }

    return (EXIT_SUCCESS);
}

/*********************************/
/* Initial UART for dsPIC30F4011 */
/* 9600,N,8,1 / 117.9648MHz Fosc */
/*********************************/
void init_uart()
{		  
  CloseUART1();												// Disable UART1 Before New Config

  // Config UART1 Interrupt Control
  ConfigIntUART1(UART_RX_INT_EN &							// ENABLE RX Interrupt
    		     UART_RX_INT_PR2 &							// RX Interrupt Priority = 2
    		     UART_TX_INT_DIS &
    		     UART_TX_INT_PR3 );
     		    
    		     
  // Open UART1 = Mode,Status,Baudrate              
  OpenUART1(UART_EN	&										// Enable UART(UART Mode)
            UART_IDLE_STOP &								// Disable UART in IDLE Mode 
 			//UART_ALTRX_ALTTX & 								// Select U1TX=RC13,U1RX=RC14
            UART_DIS_WAKE &									// Disable Wake-Up
			UART_DIS_LOOPBACK &								// Disable Loop Back
			UART_DIS_ABAUD &								// Disable Auto Baudrate
  			UART_NO_PAR_8BIT &								// UART = 8 Bit, No Parity
 			UART_1STOPBIT,									// UART = 1 Stop Bit

	  		// Config UART1 Status
  			UART_INT_TX & 									// Select Interrupt After TX Complete
	 		UART_TX_PIN_NORMAL &							// Normal U1TX Mode
 			UART_TX_ENABLE &								// Enable U1TX
 	 		UART_INT_RX_CHAR &							// Flasg Set After RX Complete 
  			UART_ADR_DETECT_DIS &              				// Disable Check Address 
			UART_RX_OVERRUN_CLEAR,							// Clear Overrun Flag

  			// ET-BASE dsPIC30F4011 Hardware Board
  			// XTAL = 7.3728MHz
  			// Fosc = 7.3728 MHz x 16 = 117.9648 MHz
  			// Fcy(UART) = Fosc / 4 
  			//           = 117.9648 / 4 = 29.4912 MHz
  			// U1BRG = [Fcy/(16xBaud)]-1
  			//       = [29.4912 MHz / (16x9600)] - 1
  			//       = 191 = BFH			
  			191);	// ET-BASE dsPIC30F4011 UART Baudrate = 9600 BPS = 191 //  Buad Rate 19200 = 95  ,, Buadrate 38400 = 47 ,  Buadrate 115200 = 15
  			
}