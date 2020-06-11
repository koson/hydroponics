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

#include <stdint.h>          /* For uint32_t definition                       */
#include <stdbool.h>         /* For true/false definition                     */

#include "system.h"          /* variables/params used by system.c             */

/******************************************************************************/
/* System Level Functions                                                     */
/*                                                                            */
/* Custom oscillator configuration funtions, reset source evaluation          */
/* functions, and other non-peripheral microcontroller initialization         */
/* functions get placed in system.c                                           */
/*                                                                            */
/******************************************************************************/

/* Refer to the device Family Reference Manual Oscillator section for
information about available oscillator configurations.  Typically
this would involve configuring the oscillator tuning register or clock
switching using the compiler's __builtin_write_OSCCON functions.
Refer to the C Compiler for PIC24F MCUs and dsPIC DSCs User Guide in the
compiler installation directory /doc folder for documentation on the
__builtin functions.  Refer to the XC16 C Compiler User's Guide appendix G
 for a list of the XC16 compiler __builtin functions */

/* TODO Add clock switching code if appropriate.  An example stub is below.   */
void ConfigureOscillator(void)
{
     
 /*       // Disable the Watch Dog Timer
  RCONbits.SWDTEN = 0;


  // Initiate Clock Switch to Primary Oscillator with PLL
  __builtin_write_OSCCONH(0x03);  // Set OSCCONH for clock switch
  __builtin_write_OSCCONL(OSCCON | 0x01);  // Start clock switching
   //while (OSCCONbits.COSC != 0x3); // Wait for the PLL to lock*/
/*
  // Block until the clock switch has completed
  while (0b011 != OSCCONbits.COSC);

  // Block until the PLL has locked
  while (1 != OSCCONbits.LOCK);*/
  
}

