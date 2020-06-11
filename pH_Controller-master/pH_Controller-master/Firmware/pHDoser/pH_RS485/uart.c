#include "uart.h"
#include <xc.h>

/**********************************************************************
* Function Name     : BusyUART1
* Description       : This returns status whether the transmission 
                      is in progress or not, by checking Status bit TRMT
* Parameters        : None
* Return Value      : char info whether transmission is in progress 
***********************************************************************/
char BusyUART1(void)
{  
    return(!U1STAbits.TRMT);
}

/*********************************************************************
* Function Name     : CloseUART1
* Description       : This function disables the UART and clears the 
*                     Interrupt enable & flag bits
* Parameters        : None
* Return Value      : None
*********************************************************************/
void CloseUART1(void)
{  
    U1MODEbits.UARTEN = 0;
	
    IEC0bits.U1RXIE = 0;
    IEC0bits.U1TXIE = 0;
	
    IFS0bits.U1RXIF = 0;
    IFS0bits.U1TXIF = 0;
}

/*********************************************************************
* Function Name     : ConfigIntUART1
* Description       : This function sets priority for RX,TX interrupt  
*                     and enable/disables the interrupt
* Parameters        : unsigned int config enable/disable and priority
* Return Value      : None
*********************************************************************/
void ConfigIntUART1(unsigned int config)
{
    /* clear IF flags */
    IFS0bits.U1RXIF = 0;
    IFS0bits.U1TXIF = 0;

    /* set priority */
    IPC2bits.U1RXIP = 0x0007 & config;
    IPC2bits.U1TXIP = (0x0070 & config) >> 4;

    /* enable/disable interrupt */
    IEC0bits.U1RXIE = (0x0008 & config) >> 3;
    IEC0bits.U1TXIE = (0x0080 & config) >> 7;
}

/*********************************************************************
* Function Name     : DataRdyUart1
* Description       : This function checks whether there is any data 
*                     that can be read from the input buffer, by 
*                     checking URXDA bit
* Parameters        : None
* Return Value      : char if any data available in buffer 
*********************************************************************/
char DataRdyUART1(void)
{
    return(U1STAbits.URXDA);
}

/****************************************************************************
* Function Name     : getsUART1
* Description       : This function gets a string of data of specified length  
*                     if available in the UxRXREG buffer into the buffer 
*                     specified.
* Parameters        : unsigned int length the length expected
*                     unsigned int *buffer  the received data to be 
*                                  recorded to this array
*                     unsigned int uart_data_wait timeout value
* Return Value      : unsigned int number of data bytes yet to be received
*************************************************************************/
unsigned int getsUART1(unsigned int length,unsigned int *buffer,
                       unsigned int uart_data_wait)
{
    int wait = 0;
    char *temp_ptr = (char *) buffer;

    while(length)                         /* read till length is 0 */
    {
        while(!DataRdyUART1())
        {
            if(wait < uart_data_wait)
                wait++ ;                  /*wait for more data */
            else
                return(length);           /*Time out- Return words/bytes to be read */
        }
        wait=0;
        if(U1MODEbits.PDSEL == 3)         /* check if TX/RX is 8bits or 9bits */
            *buffer++ = U1RXREG;          /* data word from HW buffer to SW buffer */
	else
            *temp_ptr++ = U1RXREG & 0xFF; /* data byte from HW buffer to SW buffer */

        length--;
    }

    return(length);                       /* number of data yet to be received i.e.,0 */
}

/*********************************************************************
* Function Name     : OpenUART1
* Description       : This function configures the UART mode,
*                     UART Interrupt modes and the Baud Rate
* Parameters        : unsigned int config1 operation setting
*                     unsigned int config2 TX & RX interrupt modes
*                     unsigned int ubrg baud rate setting
* Return Value      : None
*********************************************************************/
void OpenUART1(unsigned int config1,unsigned int config2, unsigned int ubrg)
{
    U1BRG  = ubrg;     /* baud rate */
    U1MODE = config1;  /* operation settings */
    U1STA = config2;   /* TX & RX interrupt modes */
}

/*************************************************************************
* Function Name     : putsUART1
* Description       : This function puts the data string to be transmitted 
*                     into the transmit buffer (till NULL character)
* Parameters        : unsigned int * address of the string buffer to be 
*                     transmitted
* Return Value      : None
*********************************************************************/
void putsUART1(unsigned int *buffer)
{
    char * temp_ptr = (char *) buffer;

    /* transmit till NULL character is encountered */

    if(U1MODEbits.PDSEL == 3)        /* check if TX is 8bits or 9bits */
    {
        while(*buffer != '\0') 
        {
            while(U1STAbits.UTXBF); /* wait if the buffer is full */
            U1TXREG = *buffer++;    /* transfer data word to TX reg */
        }
    }
    else
    {
        while(*temp_ptr != '\0')
        {
            while(U1STAbits.UTXBF);  /* wait if the buffer is full */
            U1TXREG = *temp_ptr++;   /* transfer data byte to TX reg */
        }
    }
}

/*************************************************************************
* Function Name     : ReadUART1
* Description       : This function returns the contents of UxRXREG buffer
* Parameters        : None
* Return Value      : unsigned int value from UxRXREG receive buffer 
*************************************************************************/
unsigned int ReadUART1(void)
{
    if(U1MODEbits.PDSEL == 3)
        return (U1RXREG);
    else
        return (U1RXREG & 0xFF);
}

/*********************************************************************
* Function Name     : WriteUART1
* Description       : This function writes data into the UxTXREG,
* Parameters        : unsigned int data the data to be written
* Return Value      : None
*********************************************************************/
void WriteUART1(unsigned int data)
{
    if(U1MODEbits.PDSEL == 3)
        U1TXREG = data;
    else
        U1TXREG = data & 0xFF;
}

/* UART2 is defined in following devices */
#if defined(__dsPIC30F3011__) || defined(__dsPIC30F4011__) || defined(__dsPIC30F6010__) || \
    defined(__dsPIC30F3013__) || defined(__dsPIC30F3014__) || defined(__dsPIC30F5011__)	|| \
    defined(__dsPIC30F6011__) || defined(__dsPIC30F6012__) || defined(__dsPIC30F5013__) || \
    defined(__dsPIC30F6013__) || defined(__dsPIC30F6014__) || defined(__dsPIC30F4013__)

/***********************************************************************
* Function Name     : BusyUART2
* Description       : This returns status whether the transmission 
                      is in progress or not, by checking Status bit TRMT
* Parameters        : None
* Return Value      : char info whether transmission is in progress 
***********************************************************************/
char BusyUART2(void)
{  
    return(!U2STAbits.TRMT);
}

/*********************************************************************
* Function Name     : CloseUART2
* Description       : This function disables the UART and clears the 
*                     interrupt enable and flag bits
* Parameters        : None
* Return Value      : None
*********************************************************************/
void CloseUART2(void)
{  
    U2MODEbits.UARTEN = 0;
	
    IEC1bits.U2RXIE = 0;
    IEC1bits.U2TXIE = 0;
	
    IFS1bits.U2RXIF = 0;
    IFS1bits.U2TXIF = 0;
}

/*********************************************************************
* Function Name     : ConfigIntUART2
* Description       : This function sets priority for  RX and TX 
*                     interrupt and enable/disables the interrupt
* Parameters        : unsigned int config enable/disable and priority
* Return Value      : None
*********************************************************************/
void ConfigIntUART2(unsigned int config)
{
    /* clear IF flags */
    IFS1bits.U2RXIF = 0;
    IFS1bits.U2TXIF = 0;

    /* set priority */
    IPC6bits.U2RXIP = 0x0007 & config;
    IPC6bits.U2TXIP = (0x0070 & config) >> 4;

    /* enable/disable interrupt */
    IEC1bits.U2RXIE = (0x0008 & config) >> 3;
    IEC1bits.U2TXIE = (0x0080 & config) >> 7;
}

/*********************************************************************
* Function Name     : DataRdyUart2
* Description       : This function checks whether there is any data 
*                     that can be read from the input buffer, by 
*                     checking URXDA bit
* Parameters        : None
* Return Value      : char if any data available in buffer 
*********************************************************************/
char DataRdyUART2(void)
{
    return(U2STAbits.URXDA);
}

/****************************************************************************
* Function Name     : getsUART2
* Description       : This function gets a string of data of specified length  
*                     if available in the UxRXREG buffer into the buffer 
*                     specified.
* Parameters        : unsigned int length the length expected
*                     unsigned int *buffer  the received data to be 
*                                  recorded to this array
*                     unsigned int uart_data_wait timeout value
* Return Value      : unsigned int number of data bytes yet to be received
*************************************************************************/
unsigned int getsUART2(unsigned int length,unsigned int *buffer,
                       unsigned int uart_data_wait)

{
    int wait = 0;
    char *temp_ptr = (char *) buffer;

    while(length)                         /* read till length is 0 */
    {
        while(!DataRdyUART2())
        {
            if(wait < uart_data_wait)
                wait++ ;                  /*wait for more data */
            else
                return(length);           /*Time out- Return words/bytes to be read */
        }
        wait=0;
        if(U2MODEbits.PDSEL == 3)         /* check if TX/RX is 8bits or 9bits */
            *buffer++ = U2RXREG;          /* data word from HW buffer to SW buffer */
	else
            *temp_ptr++ = U2RXREG & 0xFF; /* data byte from HW buffer to SW buffer */

        length--;
    }

    return(length);                       /* number of data yet to be received i.e.,0 */
}

/*********************************************************************
* Function Name     : OpenUART2
* Description       : This function configures the UART mode,
*                     UART Interrupt modes and the Baud Rate
* Parameters        : unsigned int config1 operation setting
*                     unsigned int config2 TX & RX interrupt modes
*                     unsigned int ubrg baud rate setting
* Return Value      : None
*********************************************************************/
void OpenUART2(unsigned int config1,unsigned int config2, unsigned int ubrg)
{
    U2BRG  = ubrg;       /* baud rate */
    U2MODE = config1;    /* operation settings */
    U2STA = config2;     /* TX & RX interrupt modes */
}

/*************************************************************************
* Function Name     : putsUART2
* Description       : This function puts the data string to be transmitted 
*                     into the transmit buffer (till NULL character)
* Parameters        : unsigned int * address of the string buffer to be 
*                     transmitted 
* Return Value      : None
*********************************************************************/
void putsUART2(unsigned int *buffer)
{
    char * temp_ptr = (char *) buffer;

    /* transmit till NULL character is encountered */

    if(U2MODEbits.PDSEL == 3)        /* check if TX is 8bits or 9bits */
    {
        while(*buffer != '\0') 
        {
            while(U2STAbits.UTXBF); /* wait if the buffer is full */
            U2TXREG = *buffer++;    /* transfer data word to TX reg */
        }
    }
    else
    {
        while(*temp_ptr != '\0')
        {
            while(U2STAbits.UTXBF);  /* wait if the buffer is full */
            U2TXREG = *temp_ptr++;   /* transfer data byte to TX reg */
        }
    }
}

/*************************************************************************
* Function Name     : ReadUART2
* Description       : This function returns the contents of UxRXREG buffer
* Parameters        : None
* Return Value      : unsigned int value from UxRXREG receive buffer 
*************************************************************************/
unsigned int ReadUART2(void)
{
    if(U2MODEbits.PDSEL == 3)
        return (U2RXREG);
    else
        return (U2RXREG & 0xFF);
}

/*********************************************************************
* Function Name     : WriteUART2
* Description       : This function writes data into the UxTXREG,
* Parameters        : unsigned int data the data to be written
* Return Value      : None
*********************************************************************/
void WriteUART2(unsigned int data)
{
    if(U2MODEbits.PDSEL == 3)
        U2TXREG = data;
    else
        U2TXREG = data & 0xFF;  
}

#endif
























