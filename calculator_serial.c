#include <msp430fr6989.h>
#include <driverlib.h>

//Change based on LCD Memory locations
#define pos1 9   /* Digit A1 begins at S18 */
#define pos2 5   /* Digit A2 begins at S10 */
#define pos3 3   /* Digit A3 begins at S6  */
#define pos4 18  /* Digit A4 begins at S36 */
#define pos5 14  /* Digit A5 begins at S28 */
#define pos6 7   /* Digit A6 begins at S14 */

#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer

void Initialize_UART(void);
void uart_write_char(unsigned char ch);
unsigned char uart_read_char(void);
void init_LCD(void);
void clear_LCD(void);

void runCalculator(void);
void show_digit(int d, int position);
void displayResult(void);

// LCD memory map for numeric digits
const char Digit[10][2] =
{
    {0xFC, 0x28},  /* "0" LCD segments a+b+c+d+e+f+k+q */
    {0x60, 0x20},  /* "1" */
    {0xDB, 0x00},  /* "2" */
    {0xF3, 0x00},  /* "3" */
    {0x67, 0x00},  /* "4" */
    {0xB7, 0x00},  /* "5" */
    {0xBF, 0x00},  /* "6" */
    {0xE4, 0x00},  /* "7" */
    {0xFF, 0x00},  /* "8" */
    {0xF7, 0x00}   /* "9" */
};

// States for the calculator
enum c_states {
    FIRST_OP,
    OP,
    SECOND_OP,
    RESULT
} c_state = FIRST_OP;

// Variables to hold the operands and results
int first_op = 0;
int second_op = 0;
int result;

// Enum to hold the operation
enum operations {
    ADD,
    SUB,
    MULT,
    DIV
} operation;


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // Stop WDT
    PM5CTL0 &= ~LOCKLPM5; // Enable GPIO pins

    // eUSCI Module Configuration
    Initialize_UART();
    init_LCD();

    // Display new line at startup
    uart_write_char('\r');
    uart_write_char('\n');

    while (1)
    {
        runCalculator();  // Performs calculator functions
    }

}

void runCalculator(void)
{
    // Get serial input
    unsigned char input = uart_read_char();

    switch(c_state)
    {
    case FIRST_OP:

        if(input >= '0' && input <= '9') // If the input is a number
        {
            first_op = (input - '0');
            uart_write_char(input); // Echo the input digit
            c_state = OP; // go to next state
        }

        break;
    case OP:
        if(input == '+')
        {
            operation = ADD;
            c_state = SECOND_OP;
            uart_write_char(input);
        }
        if(input == '-')
        {
            operation = SUB;
            c_state = SECOND_OP;
            uart_write_char(input);
        }
        if(input == '*')
        {
            operation = MULT;
            c_state = SECOND_OP;
            uart_write_char(input);
        }
        if(input == '/')
        {
            operation = DIV;
            c_state = SECOND_OP;
            uart_write_char(input);
        }
        break;
    case SECOND_OP:
        if(input >= '0' && input <= '9') // If the input is a number
        {

            second_op = (input - '0');
            uart_write_char(input);
            c_state = RESULT;
        }
        break;
    case RESULT:
        // perform operation
        switch(operation)
        {
        case ADD:
            result = first_op + second_op;
            break;
        case SUB:
            result = first_op - second_op;
            break;
        case MULT:
            result = first_op * second_op;
        case DIV:
            result = 0;
            if(second_op != 0)
                result = first_op / second_op;
            break;
        default:
            result = 0;
            break;
        }
        clear_LCD();
        displayResult();

        uart_write_char('=');

        int temp_result = result;
        if(temp_result < 0)
        {
            temp_result *= -1; // convert to positive and print '-'
            uart_write_char('-');
        }
        // process result to be sent back over serial
        int tenths = temp_result/10;
        int ones = temp_result%10;
        if(tenths != 0)
        {
            uart_write_char('0' + tenths);
        }
        uart_write_char('0' + ones);
        uart_write_char('\r');  // print newline
        uart_write_char('\n');

        // return to first state
        c_state = FIRST_OP;

        break;
    default:
        break;
    }
}

void show_digit(int d, int position)
{
    // Write digits to the LCD
    LCDMEM[position] = Digit[d][0];
    LCDMEM[position+1] = Digit[d][1];
}

void displayResult(void)
{
    int temp_res = result;
    if(temp_res < 0)
    {
        temp_res *= -1;
        LCDMEM[10] |= 0x04;  // display NEG
    }

    show_digit((temp_res) % 10,pos6);
    if(result / 10 != 0)
        show_digit((temp_res) / 10,pos5);

    show_digit((second_op) % 10,pos4);
    if(second_op / 10 != 0)
        show_digit((second_op) / 10,pos3);

    show_digit((first_op) % 10  ,pos2);
    if(first_op / 10 != 0)
        show_digit((first_op) / 10  ,pos1);

    // Display the 2 colons
    LCDM7 |= 0x04;
    LCDM20 |= 0x04;
}



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

void clear_LCD()
{
    LCDMEM[pos1] = LCDBMEM[pos1] = 0;
    LCDMEM[pos1+1] = LCDBMEM[pos1+1] = 0;
    LCDMEM[pos2] = LCDBMEM[pos2] = 0;
    LCDMEM[pos2+1] = LCDBMEM[pos2+1] = 0;
    LCDMEM[pos3] = LCDBMEM[pos3] = 0;
    LCDMEM[pos3+1] = LCDBMEM[pos3+1] = 0;
    LCDMEM[pos4] = LCDBMEM[pos4] = 0;
    LCDMEM[pos4+1] = LCDBMEM[pos4+1] = 0;
    LCDMEM[pos5] = LCDBMEM[pos5] = 0;
    LCDMEM[pos5+1] = LCDBMEM[pos5+1] = 0;
    LCDMEM[pos6] = LCDBMEM[pos6] = 0;
    LCDMEM[pos6+1] = LCDBMEM[pos6+1] = 0;

    LCDM14 = LCDBM14 = 0x00;
    LCDM18 = LCDBM18 = 0x00;
    LCDM3 = LCDBM3 = 0x00;
}


void init_LCD()
{
    LCD_C_initParam initParams = {0};
    initParams.clockSource = LCD_C_CLOCKSOURCE_ACLK;
    initParams.clockDivider = LCD_C_CLOCKDIVIDER_1;
    initParams.clockPrescalar = LCD_C_CLOCKPRESCALAR_16;
    initParams.muxRate = LCD_C_4_MUX;
    initParams.waveforms = LCD_C_LOW_POWER_WAVEFORMS;
    initParams.segments = LCD_C_SEGMENTS_ENABLED;

    LCD_C_init(LCD_C_BASE, &initParams);
    // LCD Operation - VLCD generated internally, V2-V4 generated internally, v5 to ground

    /*  'FR6989 LaunchPad LCD1 uses Segments S4, S6-S21, S27-S31 and S35-S39 */
    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_4,
                                LCD_C_SEGMENT_LINE_4);
    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_6,
                                LCD_C_SEGMENT_LINE_21);
    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_27,
                                LCD_C_SEGMENT_LINE_31);
    LCD_C_setPinAsLCDFunctionEx(LCD_C_BASE, LCD_C_SEGMENT_LINE_35,
                                LCD_C_SEGMENT_LINE_39);

    LCD_C_setVLCDSource(LCD_C_BASE, LCD_C_VLCD_GENERATED_INTERNALLY,
                        LCD_C_V2V3V4_GENERATED_INTERNALLY_NOT_SWITCHED_TO_PINS,
                        LCD_C_V5_VSS);

    // Set VLCD voltage to 3.20v
    LCD_C_setVLCDVoltage(LCD_C_BASE,
                         LCD_C_CHARGEPUMP_VOLTAGE_3_02V_OR_2_52VREF);

    // Enable charge pump and select internal reference for it
    LCD_C_enableChargePump(LCD_C_BASE);
    LCD_C_selectChargePumpReference(LCD_C_BASE,
                                    LCD_C_INTERNAL_REFERENCE_VOLTAGE);

    LCD_C_configChargePump(LCD_C_BASE, LCD_C_SYNCHRONIZATION_ENABLED, 0);

    // Clear LCD memory
    LCD_C_clearMemory(LCD_C_BASE);

    //Turn LCD on
    LCD_C_on(LCD_C_BASE);

    LCD_C_selectDisplayMemory(LCD_C_BASE, LCD_C_DISPLAYSOURCE_MEMORY);
}


