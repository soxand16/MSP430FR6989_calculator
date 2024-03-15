#include <msp430fr6989.h>
#include <driverlib.h>

// Defines for serial communication
#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer

void Initialize_UART(void);
void uart_write_char(unsigned char ch);
unsigned char uart_read_char(void);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT
    
    Initialize_UART();
    
    while (1)
    {
        // Simple echo code
        unsigned char ch;
        ch = uart_read_char();
        uart_write_char(ch);
    }

}

/******************************************************
************** SERIAL COMMUNICATION CODE **************
******************************************************/


// Configure UART to the popular configuration
// 9600 baud, 8-bit data, LSB first, no parity bits, 1 stop bit
// no flow control
// Initial clock: SMCLK @ 1.048 MHz with oversampling
void Initialize_UART(void){
    // Divert pins to UART functionality
    P3SEL1 &= ~(BIT4|BIT5);
    P3SEL0 |= (BIT4|BIT5);

    // Use SMCLK clock; leave other settings default
    UCA1CTLW0 |= UCSSEL_2;

    // Configure the clock dividers and modulators
    // UCBR=6, UCBRF=13, UCBRS=0x22, UCOS16=1 (oversampling)
    UCA1BRW = 6;
    UCA1MCTLW = UCBRS5|UCBRS1|UCBRF3|UCBRF2|UCBRF0|UCOS16;

    // Exit the reset state (so transmission/reception can begin)
    UCA1CTLW0 &= ~UCSWRST;
}

// Transmit a byte over UART
// When byte is copied to trasmit buffer, transmit flag = 0 automatically
// When transmission finishes, the transmit flag goes back to one
void uart_write_char(unsigned char ch){

    // Wait for any ongoing transmission to complete
    while ( (FLAGS & TXFLAG)==0 ) {}

    // Write the byte to the transmit buffer
    TXBUFFER = ch;
}

// Receives a byte over UART
// Returns the byte; if none received, returns NULL
// Byte is received, the receive flag becomes 1
// When copy the byte from receive buffer, the receive flag = 0 automatically
unsigned char uart_read_char(void){

    unsigned char temp;

    // Return NULL if no byte received
    if( (FLAGS & RXFLAG) == 0)
        return '\0';

    // Otherwise, copy the received byte (clears the flag) and return it
    temp = RXBUFFER;

    return temp;
}

/******************************************************
****************** KEYPAD INPUT CODE ******************
******************************************************/

/*
 * Reads the keypad and returns the character pressed (char)
 * The connection is as follows:
 *
 * ************************  MSP  *****************************
 * GND  |  P9.0  |  P9.1  |  P8.4  |  P8.5  |  P8.6  |  P8.7  |
 *
 *
 * Pin1 |   2    |   3    |   4    |   5    |   6    |   7    |
 * ***********************  KEYPAD  ***************************
 * Col0 | Col 1  | Col 2  | Row 0  | Row 1  | Row 2  | Row 3  |
 *
 * NOTE: The correct pins (P8 4-7) must be set as input pullup! 
 * Also, this function is designed to be run only every 10-20 milliseconds
 * for proper debouncing. 
 */
bool pressed = false; // for debouncing 
unsigned char read_keypad(void)
{
    // *** assumes P9.0 and P9.1 are already low ***
    unsigned char data = (P8IN >> 4) & 0x0F;
    int key = 0;

    // loop down through the rows until key held is found
    while((data & 0x01) != 0)
    {
        data >>= 1;
        key += 3;
    }

    if(key < 12) // key actually pressed
    {
        P9OUT |= 0x02; // turn on column 2
        if((P8IN & 0xF0) == 0xF0) // no short detected
        {
            key += 2; // column 2 must have been pressed
        }
        else
        {
            P9OUT |= 0x01; // turn on column 1 too
            if((P8IN & 0xF0) == 0xF0)
            {
                key += 1; // column 1 must have been pressed
            }
            // else it must have been column 0, and we have accounted for that

            P9OUT &= ~ 0x01; // pull back to low
        }
        P9OUT &= ~0x02; // pull back to low
        /* KEY
         * 0  1  2
         * 3  4  5
         * 6  7  8
         * 9  10 11
         */
        if(!pressed)
        {
            pressed = true; // debouncing flag

            if(key <= 8)
                return ('0' + key + 1); // these numbers are just offset by 1
            if(key == 10)
                return ('0'); // this is a 0
            if(key == 9)
                return ('*'); // this is a '*'
            if(key == 11)
                return ('#'); // this is a '#'
        }
    }
    else
    {
        // no key is pressed
        pressed = false;
    }
    return 0;
}
