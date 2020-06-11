/* 
 * File:   lcd16x2.h
 * Author: rachot
 *
 * Created on October 7, 2016, 1:17 PM
 */

#ifndef LCD16X2_H
#define	LCD16X2_H

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LCD16X2_H */


#define LCD_TRIS    TRISE     // Set direction control output  
#define LCD_DATA    LATE      // Dataport of LCD-Display (D4..D7)
#define LCD_RS      _LATE5    // Register Select of LCD-Display
#define LCD_E       _LATE4    // Enable of LCD-Display
#define CTRL_REG    0         // Select instruction register
#define DATA_REG    1         // Select data register
#define BLINK       0x01      // Alias for blinking cursor
#define NOBLINK     0x00      // Alias for non blinking cursor
#define SHOW        0x02      // Alias for cursor on
#define HIDE        0x00      // Alias for cursor off
#define ON          0x04      // Alias for display on
#define OFF         0x00      // Alias for display off

    
// Table to select DD-RAM address (including instruction and address)
// 0x00..0x0F -> Line 1, 0x40..0x4F -> Line 2
static unsigned char LOCATION[2][16] = { {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
                                          0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F},
                                         {0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
                                          0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF} 
                                        };


//----------------------------------------------------:LCD command
static void LCD_Command(unsigned char cmd)
{
    __delay_ms(10);              // Wait 20ms
	LCD_E  = 0;
	LCD_RS = CTRL_REG;      // Switch to inruction register

	// Set LCD_DATA to high nibble of Display On/Off Control
	LCD_DATA = (LCD_DATA&0xF0)|((cmd&0xF0)>>4);
	LCD_E = 1; LCD_E = 0;   // Write data to display

	// Set LCD_DATA to lower nibble of Display On/Off Control
	LCD_DATA = (LCD_DATA&0xF0)|(cmd&0x0F);
	LCD_E = 1; LCD_E = 0;   // Write data to display

	__delay_ms(5);  	            // Wait 1ms

	return;
}

//----------------------------------------------------:LCDControl
// Function    : LCDControl(dsp,blink,cursor)
// Input : unsigned char dsp    = ON     -> Display on
//                                OFF    -> Display off
//         unsigned char blink  = BLINK  -> Cursor blinks
//                                NOBLINK-> Cursor not blinks
//         unsigned char cursor = SHOW   -> Cursor visible
//                                HIDE   -> Cursor invisible
//
void LCDControl(unsigned char dsp,unsigned char blink,unsigned char cursor)
{
	unsigned char control;  // variable to generate instruction byte

	control = (0x08 + blink + cursor + dsp); // Cursor control
	LCD_Command(control);
	
	return;
}

//----------------------------------------------------:LCDClrscr
void LCDClrscr(void)
{
  LCD_Command(0x01);    // Clear screen
}  
  
//----------------------------------------------------:LCDGotoxy
// Function    : LCDGotoxy(x,y)
// Description : Sets the write position of the LCD display         
//                                                                  
//                 (1,1)         (16,1)                             
//                   |              |                               
//                   ################   -> line 1                   
//                   ################   -> line 2                   
//                   |              |                               
//                 (1,2)         (16,2)                             
// Input       : unsigned char x    -> x position (horizontal)      
//               unsigned char y    -> y position (vertical)        
//
void LCDGotoxy(unsigned char x,unsigned char y)
{
	__delay_ms(10);             	// Wait 20ms
	LCD_E  = 0;
	LCD_RS = CTRL_REG;      // Switch to inruction register

	// Set LCD_DATA to high nibble of position table value
	LCD_DATA = (LCD_DATA&0xF0)|(((LOCATION[y-1][x-1])&0xF0)>>4);
	LCD_E = 1; LCD_E = 0;       // Write data to display
	// Set LCD_DATA to lower nibble of position table value
	LCD_DATA = (LCD_DATA&0xF0)|((LOCATION[y-1][x-1])&0x0F);
	LCD_E = 1; LCD_E = 0;       // Write data to display

	__delay_ms(5);                   // Wait 1ms

	return;
}

//----------------------------------------------------:LCDPutchar
// Function    : LCDPutchar(value)                             
// Description : Writes the character value to the display
//
void LCDPutchar(unsigned char value)
{  
	LCD_RS = DATA_REG;              // Switch to data register

	// Set LCD_DATA to high nibble of value
	LCD_DATA = (LCD_DATA&0xF0)|((value&0xF0)>>4);
	LCD_E = 1; LCD_E = 0;           // Write data to display
	// Set LCD_DATA to lower nibble of value
	LCD_DATA = (LCD_DATA&0xF0)|(value&0x0F);
	LCD_E = 1; LCD_E = 0;           // Write data to display

	__delay_ms(5);                       // Wait 1ms

	return;
}

//----------------------------------------------------:LCDPrintxy
// Function    : LCDPrintxy(x,y,*text)                         
// Description : Prints text to position x/y of the display      
// Input       : unsigned char x     -> x position of the display 
//               unsigned char y     -> y position of the display  
//               unsigned char *text -> pointer to text             
//							 unsigned int dly	   -> delay time
//
void LCDPrintxy(unsigned char x,unsigned char y, unsigned int dly, unsigned char *text)
{
	LCDGotoxy(x,y);         // Set cursor position

	while( *text )          // while not end of text
  {
  	LCDPutchar(*text++);  // Write character and increment position
	__delay_ms(dly);           // time delay print text
  } 
	return;
}

//----------------------------------------------------:LCDInit
// Initialize the LCD display
void LCDInit(void)
{
    LCD_TRIS = 0x0000;      // set direction control RB output
    LCD_Command(0x33);      // set 4 bit mode
    LCD_Command(0x32);      // set 4 bit mode
    LCD_Command(0x28);      // 2 line 5x7 dot
    LCD_Command(0x01);      // clear screen lcd
    LCDControl(ON,NOBLINK,HIDE);           
}