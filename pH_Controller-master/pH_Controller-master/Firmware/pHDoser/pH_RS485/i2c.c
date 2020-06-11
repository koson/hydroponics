#include "i2c.h"
 #include <xc.h>

/*********************************************************************
*    Function Name:  AckI2C										
*    Description:    This routine generates acknowledge condition 
*                    during master receive.					 
*    Parameters:     void									
*	 Return Value:   void
*********************************************************************/
void AckI2C(void)
{
	I2CCONbits.ACKDT = 0;
	I2CCONbits.ACKEN = 1;
}

/*********************************************************************
*    Function Name:  CloseI2C										
*    Description:    This routine disables the I2C module by 
*                    clearing the I2CEN bit in I2CCON register.
*                    The MI2C and SI2C interrupts are disabled and 
*                    the corresponding IF flags are cleared
*    Parameters:     void									
*    Return Value:   void
*********************************************************************/
void CloseI2C(void)
{
	/* clear the I2CEN bit */
	I2CCONbits.I2CEN = 0;

	/* clear the SI2C & MI2C Interrupt enable bits */
	IEC0bits.SI2CIE = 0;
	IEC0bits.MI2CIE = 0;

	/* clear the SI2C & MI2C Interrupt flag bits */
	IFS0bits.SI2CIF = 0;
	IFS0bits.MI2CIF = 0;
}

/*********************************************************************
*    Function Name:  ConfigIntI2C										
*    Description:    This routine enables/disables the SI2C & MI2C  
*                    interrupts and sets their priorities
*    Parameters:     unsigned int : config	
*    Return Value:   void
*********************************************************************/
void ConfigIntI2C(unsigned int config)
{
     IFS0bits.SI2CIF = 0;		                 /* clear the MI2C & SI2C Interrupts */
     IFS0bits.MI2CIF = 0;

     IPC3bits.SI2CIP = (config & 0x0007);	     /* set the SI2C priority */
     IPC3bits.MI2CIP = (config & 0x0070) >> 4;    /* set the MI2C priority */

     IEC0bits.SI2CIE = (config & 0x0008)>> 3;	 /* enable/disable the SI2C Interrupt */
     IEC0bits.MI2CIE = (config & 0x0080) >> 7;    /* enable/disable the MI2C Interrupt */
}

/************************************************************************
*    Function Name:  DataRdyI2C	
*    Description:    This routine provides the status whether the receive 
*                    buffer is full by returning the RBF bit.					 
*    Parameters:     void									
*	 Return Value:   RBF bit status
*************************************************************************/
char DataRdyI2C(void)
{
     return I2CSTATbits.RBF;
}

/************************************************************************
*    Function Name:  IdleI2C	
*    Description:    This routine generates wait condition intil I2C 
*                    bus is Idle.					 
*    Parameters:     void
*    Return Value:   void
*************************************************************************/
void IdleI2C(void)
{
    /* Wait until I2C Bus is Inactive */
    while(I2CCONbits.SEN || I2CCONbits.PEN || I2CCONbits.RCEN || I2CCONbits.ACKEN || I2CSTATbits.TRSTAT);	
}

/************************************************************************
*    Function Name:  MastergetsI2C
*    Description:    This routine reads predetermined data string length
*                    from the I2C bus.
*    Parameters:     unsigned int    : length
*                    unsigned char * : rdptr
*    Return Value:   unsigned int
*************************************************************************/
unsigned int MastergetsI2C(unsigned int length, unsigned char * rdptr, unsigned int i2c_data_wait)
{
    int wait = 0;
    while(length)                    /* Receive the number of bytes specified by length */
    {
        I2CCONbits.RCEN = 1;
        while(!DataRdyI2C())
        {
            if(wait < i2c_data_wait)
                wait++ ;                 
            else
            return(length);          /* Time out, return number of byte/word to be read */			
        }
        wait = 0;
        *rdptr = I2CRCV;             /* save byte received */
        rdptr++;
        length--;
        if(length == 0)              /* If last char, generate NACK sequence */
        {
            I2CCONbits.ACKDT = 1;
            I2CCONbits.ACKEN = 1;
        }
        else                         /* For other chars,generate ACK sequence */
        {
            I2CCONbits.ACKDT = 0;
            I2CCONbits.ACKEN = 1;
        }
            while(I2CCONbits.ACKEN == 1);    /* Wait till ACK/NACK sequence is over */
    }
    return 0;    /* return status that number of bytes specified by length was received */
}

/***********************************************************************
*    Function Name:  MasterputsI2C
*    Description:    This routine is used to write out a string to the 
*                    I2C bus.If write collision occurs,-3 is sent.If 
*                    Nack is received, -2 is sent.If string is written 
*                    and null char reached, 0 is returned. 				 
*    Parameters:     unsigned char * : wrptr									
*    Return Value:   unsigned int 
************************************************************************/
unsigned int MasterputsI2C(unsigned char * wrptr)
{
    while(*wrptr)                           //transmit data until null char
    {
        if(MasterputcI2C(*wrptr) == -1)	    // write a byte
        return -3;                          //return with write collison error

        while(I2CSTATbits.TBF);             //Wait till data is transmitted.

        IdleI2C();
        wrptr++;
    }
    return 0;			
}

/******************************************************************************
*    Function Name:  MasterReadI2C										
*    Description:    This routine reads a single byte from the I2C Bus. 
*                    To enable master receive,RCEN bit is set.
*                    The RCEN bit is checked until it is cleared.When cleared,
*                    the receive register is full and it's contents are returned.
*    Parameters:     void									
*    Return Value:   unsigned char
********************************************************************************/
unsigned char MasterReadI2C(void)
{
    I2CCONbits.RCEN = 1;
    while(I2CCONbits.RCEN);
    I2CSTATbits.I2COV = 0;
    return(I2CRCV);
}

/************************************************************************
*    Function Name:  MasterWriteI2C
*    Description:    This routine is used to write a byte to the I2C bus.
*                    The input parameter data_out is written to the 
*                    I2CTRN register. If IWCOL bit is set,write collision 
*                    has occured and -1 is returned, else 0 is returned.			 
*    Parameters:     unsigned char : data_out									
*    Return Value:   unsigned int
*************************************************************************/
char MasterWriteI2C(unsigned char data_out)
{
    I2CTRN = data_out;

    if(I2CSTATbits.IWCOL)        /* If write collision occurs,return -1 */
        return -1;
    else
    {
        return 0;
    }
}

/*********************************************************************
*    Function Name:  NotAckI2C										
*    Description:    This routine generates not acknowledge condition 
*                    during master receive.					 
*    Parameters:     void									
*    Return Value:   void
*********************************************************************/
void NotAckI2C(void)
{
    I2CCONbits.ACKDT = 1;
    I2CCONbits.ACKEN = 1;
}

/******************************************************************************
*    Function Name:  OpenI2C
*    Description:    This function configures the I2C module for enable bit, 
*                    disable slew rate, SM bus input levels, SCL release,
*                    Intelligent Peripheral Management Interface enable, 
*                    sleep mode, general call enable,acknowledge data bit, 
*                    acknowledge sequence enable, receive enable, stop 
*                    condition enable, restart condition enable and start
*                    condition enable. The Baud rate  value is also configured  					 
*    Parameters:     unsigned int : config1
*                    unsigned int : config2 									
*    Return Value:   void
*******************************************************************************/
void OpenI2C(unsigned int config1,unsigned int config2)
{
    I2CBRG = config2;
    I2CCON = config1;
}

/*********************************************************************
*    Function Name:  RestartI2C										
*    Description:    This routine generates Restart condition 
*                    during master mode.					 
*    Parameters:     void									
*    Return Value:   void
*********************************************************************/
void RestartI2C(void)
{ 
    I2CCONbits.RSEN = 1;	/* initiate restart on SDA and SCL pins	*/
}

/************************************************************************
*    Function Name:  SlavegetsI2C	
*    Description:    This routine reads bytes from the I2C bus until  
*                    stop bit is received.
*    Parameters:     unsigned char * : rdptr
*                    unsigned int    : i2c_data_wait			
*    Return Value:   unsigned int    : number of bytes received
*************************************************************************/
unsigned int SlavegetsI2C(unsigned char * rdptr, unsigned int i2c_data_wait)
{
    int i = 0;                          /* i indicates number of bytes received */
    int wait = 0;
    unsigned char temp = I2CRCV;        /* flush out old data already on I2CRCV to clear RBF flag */

    I2CSTATbits.I2COV = 0;              /* clear OV flag */

    while(!I2CSTATbits.P)               /* check for stop bit */
    {
        while(!DataRdyI2C())
        {
            if(wait < i2c_data_wait)    /* timeout check */
                wait++ ;                 
            else
                return i;               /* return the number of bytes received */		
        }
        wait = 0;
        *rdptr++ = I2CRCV;              /* save byte received */

        i++;                            /* Increment the number of bytes read */

        I2CCONbits.ACKDT = 0;		/* generate ACK sequence */
        I2CCONbits.ACKEN = 1;
        while(I2CCONbits.ACKEN == 1);	/* Wait till ACK sequence is over */

        if((I2CCONbits.STREN) && (!I2CCONbits.SCLREL))
            I2CCONbits.SCLREL = 1;	/* Clock is released after ACK */

    }
    return i;				/* return the number of bytes received */
}

/**************************************************************************
*    Function Name:  SlaveputsI2C
*    Description:    This routine is used to write out a string to the 
*                    I2C bus.If write collision occurs,-3 is sent.If 
*                    string is written and null char reached, 0 is returned. 				 
*    Parameters:     unsigned char * : wrptr
*    Return Value:   unsigned int
****************************************************************************/
unsigned int SlaveputsI2C(unsigned char * wrptr)
{
    I2CCONbits.STREN = 1;            /* SCL clock stretch enable bit */
    while(*wrptr)	             /* transmit data until null char */
    {
        SlaveputcI2C(*wrptr++);	     /* Send a byte */
	while(I2CSTATbits.TBF);	     /* wait till the transmit buffer is clear */ 
        while(!IFS0bits.SI2CIF);     /* Wait till the ACK from master is received */
    }
    return 0;                        /* null char was reached */
}

/**********************************************************************
*    Function Name:  SlaveReadI2C										
*    Description:    This routine reads a single byte from the I2C Bus. 
*                    The RBF bit is checked until it is set.When set,
*                    the receive register is full and it's contents are
*                    returned.					 
*    Parameters:     void									
*    Return Value:   unsigned char
***********************************************************************/
unsigned char SlaveReadI2C(void)
{
     while(!I2CSTATbits.RBF);
     I2CSTATbits.I2COV = 0;
     return(I2CRCV);

}

/****************************************************************************
*    Function Name:  SlaveWriteI2C
*    Description:    This routine is used to write a byte to the I2C bus.
*                    The input parameter data_out is written to the 
*                    I2CTRN register.
*    Parameters:     unsigned char : data_out									
*    Return Value:   None
******************************************************************************/
void SlaveWriteI2C(unsigned char data_out)
{
     I2CTRN = data_out;      /* data transferred to I2CTRN reg */
     I2CCONbits.SCLREL = 1;	/* Release the clock */
}

/*********************************************************************
*    Function Name:  StartI2C										
*    Description:    This routine generates Start condition 
*                    during master mode.					 
*    Parameters:     void									
*    Return Value:   void
*********************************************************************/
void StartI2C(void)
{
     I2CCONbits.SEN = 1;	/* initiate Start on SDA and SCL pins */
}

/*********************************************************************
*    Function Name:  StopI2C										
*    Description:    This routine generates Stop condition 
*                    during master mode.					 
*    Parameters:     void									
*    Return Value:   void
*********************************************************************/
void StopI2C(void)
{
     I2CCONbits.PEN = 1;	/* initiate Stop on SDA and SCL pins */
}













