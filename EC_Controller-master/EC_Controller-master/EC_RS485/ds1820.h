/* 
 * File:   ds1820.h
 * Author: Rachot 
 *
 * Created on April 12, 2016, 3:41 PM
 */

#ifndef DS1820_H
#define	DS1820_H

#ifdef	__cplusplus
extern "C" {
#endif


void ds1820_init()
{
	DQ    = 1;
    DQ_DIR = 1;         
}
 
unsigned char resetp(void)
{
DQ_DIR = 0;
DQ = 0;           //Bit = 0
__delay_us(600);   // 600 uS
DQ_DIR = 1;
__delay_us(60); // 60 uS
 
if (DQ)
	return 0;

__delay_us(600);    //Delay 480 uSes minimum
return 1; 
}
void writep (unsigned char dat)
{
 
unsigned char i;
 
for (i = 0;i < 8;i++)
{
 
	DQ_DIR = 0;
	DQ = 0; //Bit = 0
	 
	__delay_us(15);  //Delay 15 uSec
	 
	if (dat & 0x01)
		DQ = 1;
	else
		DQ = 0;
	 
	__delay_us(60);                              //Delay 60 uSEC
	DQ = 1;
	dat >>= 1;
	 
	__delay_us(10);  //Delay 10 uSEC
	 
}
 
 
}
 
unsigned int  readp(void)
{
 
unsigned char dat,i;
 
for (i = 0;i < 8;i++)
{
 
	dat >>= 1;
	DQ_DIR = 0;
	DQ = 0; //Bit = 0
	 
	__delay_us(15); //Delay 15 uSec
	 
	DQ_DIR = 1;
	__delay_us(5);
	if (DQ)
		dat |= 0x80; //Input = 1
	else
		dat &= 0x7F; //Input = 0;
	 
	__delay_us(60); //Delay 60 uSEC
	 
}

return dat;
 
}
 
unsigned int *readt(void)
{
    unsigned char i;
    unsigned int temp_dat[9];
 
    if (resetp())
    {

        writep(0xCC); //Skip ROM
        writep(0x44); //Temperature Convert
        __delay_us(120);  // 500 mS

        resetp();
        writep(0xCC); //Skip ROM
        writep(0xBE); //Read scratchpad

            for(i=0;i<9;i++)
                temp_dat[i] = readp();

    return temp_dat;
    }
    return 0;
}


#ifdef	__cplusplus
}
#endif

#endif	/* DS1820_H */

