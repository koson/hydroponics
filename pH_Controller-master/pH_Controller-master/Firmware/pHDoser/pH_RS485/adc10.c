#include "xc.h"
#include "adc10.h"

#if defined(__dsPIC30F2010__) || defined(__dsPIC30F3010__) || defined(__dsPIC30F4012__) || defined(__dsPIC30F3011__) \
    || defined(__dsPIC30F4011__) || defined(__dsPIC30F6010__) || defined(__dsPIC30F5015__)

/*********************************************************************
* Function Name     : ConfigIntADC10
* Description       : This function configures ADC Interrupt.
* Parameters        : unsigned int config
* Return Value      : None
*********************************************************************/

void ConfigIntADC10(unsigned int config)
{
    /* Clearing the Interrupt Flag bit */
    IFS0bits.ADIF = 0;
  
    /* Setting Priority */
    IPC2bits.ADIP = config & 0x07;
  
    /* Setting the Interrupt enable bit */
    IEC0bits.ADIE = (config & 0x08)>>3;
}

/*********************************************************************
* Function Name     : ConvertADC10
* Description       : This function starts an A/D conversion by 
*                     clearing ADCON1<SAMP> bit.
* Parameters        : None
* Return Value      : None
*********************************************************************/

void ConvertADC10(void)
{
     ADCON1bits.SAMP = 0; /* clear SAMP to start conversion*/
}

/*********************************************************************
* Function Name     : OpenADC10
* Description       : Configures the ADC. This includes :
                    · Operating mode      // ADCON1<15> ADON bit
                    · Data o/p format     // ADCON1<9:8> FORM bits
                    · Sample Clk Source   // ADCON1<7:5> SSRC<2:0>bits
                    · Vref source         // ADCON2<15:13> VCFG<2:0> bits
                    . Channels utilized   // ADCON2<9:8> CHPS<1:0>bits
                    · No of samples/int   // ADCON2<4:2> SMPI<2:0>bits
                    · Buffer fill mode    // ADCON2<1> BUFM bit
                    · Alternate i/p sample mode // ADCON2<0> ALTS
                    · Auto sample time   //ADCON3<12:8> SAMC<4:0>bits
                    · Conv clock source  //ADCON3<6> ADRC
                    · Conv clock select bits //ADCON3<5:0> ADCS<5:0>
                    · Port config control bits.

* Parameters        : config1, config2, config3, configscan,configport
* Return Value      : None
*********************************************************************/

void OpenADC10(unsigned int config1, unsigned int config2, unsigned int
               config3,unsigned int configport, unsigned int configscan)
{
    /* digital/analog mode selection on the port bits */
    ADPCFG = configport;

    /* configures the input scan selection bits */
    ADCSSL = configscan;

    /* config ADCON3 */
    ADCON3 = config3;
    
    /* config ADCON2 */
    ADCON2 = config2;

    /* config ADCON1 */
    ADCON1 = config1;

    /* assign SAMP bit */
    ADCON1bits.SAMP = config1 >> 1;
}

/*********************************************************************
* Function Name     : BusyADC10
* Description       : This function returns the ADC conversion status.
* Parameters        : None
* Return Value      : DONE bit status
*********************************************************************/

char BusyADC10(void)
{
    return !(ADCON1bits.DONE);	/* returns the DONE bit status */
}

/************************************************************************
* Function Name     : ReadADC10
* Description       : This function reads from one of the 16 ADC Buffers
                      (ADCBUF0 - ADCBUFF)
* Parameters        : unsigned char bufIndex
* Return Value      : unsigned int
*************************************************************************/

unsigned int ReadADC10(unsigned char bufIndex)
{
     return(*(&ADCBUF0+bufIndex));
}

/*********************************************************************
* Function Name     : SetChanADC10
* Description       : This function sets the ADCHS reg to get the +ve 
*                     and -ve of sample inputs A and B.
* Parameters        : unsigned int channel
* Return Value      : None
*********************************************************************/

void SetChanADC10(unsigned int channel)
{
    ADCHS = channel;
}

/*********************************************************************
* Function Name     : CloseADC10
* Description       : This function turns off the A/D converter. 
*                     Also, the Interrupt enable (ADIE) and Interrupt 
*                     flag (ADIF) bits are cleared
* Parameters        : None
* Return Value      : None
*********************************************************************/

void CloseADC10(void)
{
    /* disable ADC interrupt */
    IEC0bits.ADIE = 0;

    /* turn off ADC */
    ADCON1bits.ADON = 0;    

	/* clear ADIF bit */
    IFS0bits.ADIF = 0;
}

#endif
